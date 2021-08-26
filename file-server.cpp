#include "common.hpp"

#include <ndn-cxx/metadata-object.hpp>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/optional.hpp>

namespace ndn6 {
namespace file_server {

namespace fs = boost::filesystem;

using MetadataObject = ndn::MetadataObject;

static const uint64_t SEGMENT_SIZE = 5120;
static const ndn::PartialName lsSuffix("32=ls");
#define ANY "[^<32=ls><32=metadata>]"

class SegmentLimit
{
public:
  static SegmentLimit parse(const Name& name, uint64_t size)
  {
    SegmentLimit sl;
    if (size == 0) {
      sl.ok = true;
      return sl;
    }

    sl.segment = name[-1].toSegment();
    sl.seekTo = sl.segment * SEGMENT_SIZE;
    sl.lastSeg = computeLastSeg(size);
    sl.segLen =
      sl.segment == sl.lastSeg && size % SEGMENT_SIZE != 0 ? size % SEGMENT_SIZE : SEGMENT_SIZE;
    sl.ok = sl.segment <= sl.lastSeg;
    return sl;
  }

  static uint64_t computeLastSeg(uint64_t size)
  {
    return size / SEGMENT_SIZE + static_cast<int>(size % SEGMENT_SIZE != 0) - 1;
  }

public:
  bool ok = false;
  uint64_t segment = 0;
  uint64_t seekTo = 0;
  uint64_t segLen = 0;
  uint64_t lastSeg = 0;
};

class FileInfo
{
public:
  bool checkSegmentInterestName(const Name& name)
  {
    return versioned.isPrefixOf(name) && name[-1].isSegment();
  }

public:
  bool isFile = false;
  bool isDir = false;
  fs::path path;
  Name versioned;
  boost::optional<uint64_t> size;
};

class FileServer : boost::noncopyable
{
public:
  explicit FileServer(Face& face, KeyChain& keyChain, const Name& prefix, const fs::path& directory)
    : m_face(face)
    , m_keyChain(keyChain)
    , m_prefix(prefix)
    , m_directory(directory)
  {
    face.registerPrefix(prefix, nullptr, abortOnRegisterFail);
    face.setInterestFilter(InterestFilter(prefix, ANY "*<32=metadata>"),
                           [this](const auto&, const auto& interest) { rdrFile(interest); });
    face.setInterestFilter(InterestFilter(prefix, ANY "{2,}"),
                           [this](const auto&, const auto& interest) { readFile(interest); });
    face.setInterestFilter(InterestFilter(prefix, ANY "*<32=ls><32=metadata>"),
                           [this](const auto&, const auto& interest) { rdrDir(interest); });
    face.setInterestFilter(InterestFilter(prefix, ANY "*<32=ls>" ANY "{2}"),
                           [this](const auto&, const auto& interest) { readDir(interest); });
  }

private:
  FileInfo parseInterestName(const Name& name, int suffixLen,
                             const ndn::PartialName& suffixInsert = ndn::PartialName())
  {
    FileInfo info;

    auto rel = name.getSubName(m_prefix.size(), name.size() - m_prefix.size() - suffixLen);
    info.path = m_directory;
    for (const name::Component& comp : rel) {
      info.path /= std::string(reinterpret_cast<const char*>(comp.value()), comp.value_size());
      if (info.path.filename_is_dot() || info.path.filename_is_dot_dot()) {
        return FileInfo{};
      }
    }

    boost::system::error_code ec;
    fs::file_status stat = fs::status(info.path, ec);
    if (ec) {
      return FileInfo{};
    }
    info.isFile = fs::is_regular_file(stat);
    info.isDir = fs::is_directory(stat);

    std::time_t writeTime = fs::last_write_time(info.path, ec);
    if (ec) {
      return FileInfo{};
    }
    info.versioned =
      name.getPrefix(-suffixLen).append(suffixInsert).appendVersion(writeTime * 1000000);

    if (info.isFile) {
      info.size = static_cast<uint64_t>(fs::file_size(info.path, ec));
      if (ec) {
        return FileInfo{};
      }
    }
    return info;
  }

  void rdrFile(const Interest& interest)
  {
    auto name = interest.getName();
    auto info = parseInterestName(name, 1);
    replyRdr("RDR-FILE", name, info, info.isFile);
  }

  void rdrDir(const Interest& interest)
  {
    auto name = interest.getName();
    auto info = parseInterestName(name, 2, lsSuffix);
    replyRdr("RDR-DIR", name, info, info.isDir);
  }

  void replyRdr(const char* act, Name name, const FileInfo& info, bool found)
  {
    if (!found) {
      replyNack(name);
      std::cout << act << "-NOT-FOUND" << '\t' << info.path << std::endl;
      return;
    }

    Block content(tlv::Content);
    content.push_back(info.versioned.wireEncode());
    if (info.size) {
      content.push_back(name::Component::fromByteOffset(*info.size).wireEncode());
      uint64_t lastSeg = SegmentLimit::computeLastSeg(*info.size);
      content.push_back(name::Component::fromSegment(lastSeg).wireEncode());
    }
    content.encode();

    Data data(name.appendVersion().appendSegment(0));
    data.setFreshnessPeriod(1_ms);
    data.setFinalBlock(data.getName().get(-1));
    data.setContent(content);
    m_keyChain.sign(data);
    m_face.put(data);
    std::cout << act << "-OK" << '\t' << info.path << '\t' << info.versioned << std::endl;
  }

  void readFile(const Interest& interest)
  {
    auto name = interest.getName();
    auto info = parseInterestName(name, 2);
    if (!info.isFile || !info.checkSegmentInterestName(name)) {
      replyNack(name);
      return;
    }

    auto sl = SegmentLimit::parse(name, *info.size);
    if (!sl.ok) {
      replyNack(name);
      return;
    }

    fs::ifstream stream(info.path);
    replySegment("READ-FILE", name, info, sl, stream);
  }

  void readDir(const Interest& interest)
  {
    auto name = interest.getName();
    auto info = parseInterestName(name, 3, lsSuffix);
    if (!info.isDir || !info.checkSegmentInterestName(name)) {
      replyNack(name);
      return;
    }

    std::set<std::string> filenames;
    try {
      for (const auto& entry : fs::directory_iterator(info.path)) {
        auto stat = entry.status();
        if (fs::is_directory(stat)) {
          filenames.insert(entry.path().filename().string() + "/");
        } else if (fs::is_regular_file(stat)) {
          filenames.insert(entry.path().filename().string());
        }
      }
    } catch (const fs::filesystem_error& err) {
      std::cout << "READ-DIR-ERROR" << '\t' << info.path << '\t' << err.what() << std::endl;
      return;
    }

    std::stringstream stream;
    for (const auto& filename : filenames) {
      stream << filename << '\0';
    }

    auto sl = SegmentLimit::parse(name, stream.tellp());
    if (!sl.ok) {
      replyNack(name);
      return;
    }

    replySegment("READ-DIR", name, info, sl, stream);
  }

  void replySegment(const char* act, const Name& name, const FileInfo& info, const SegmentLimit& sl,
                    std::istream& stream)
  {
    stream.seekg(sl.seekTo);
    char buf[SEGMENT_SIZE];
    stream.read(buf, sl.segLen);
    if (!stream) {
      std::cout << act << "-ERROR" << '\t' << info.path << '\t' << sl.segment << std::endl;
      return;
    }

    Data data(name);
    data.setFinalBlock(name::Component::fromSegment(sl.lastSeg));
    data.setContent(reinterpret_cast<const uint8_t*>(buf), sl.segLen);
    m_keyChain.sign(data);
    m_face.put(data);
    std::cout << act << "-OK" << '\t' << info.path << '\t' << sl.segment << std::endl;
  }

  void replyNack(const Name& name)
  {
    Data data(name);
    data.setContentType(tlv::ContentType_Nack);
    data.setFreshnessPeriod(1_ms);
    m_keyChain.sign(data);
    m_face.put(data);
  }

private:
  Face& m_face;
  KeyChain& m_keyChain;
  Name m_prefix;
  fs::path m_directory;
};

int
main(int argc, char** argv)
{
  if (argc != 3) {
    std::cerr << "file-server prefix directory" << std::endl;
    return 2;
  }

  name::setConventionEncoding(name::Convention::TYPED);
  name::setConventionDecoding(name::Convention::TYPED);
  ndn::Face face;
  ndn::KeyChain keyChain;
  FileServer app(face, keyChain, argv[1], argv[2]);
  face.processEvents();
  return 0;
}

} // namespace file_server
} // namespace ndn6

int
main(int argc, char** argv)
{
  return ndn6::file_server::main(argc, argv);
}
