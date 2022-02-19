#include "common.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/optional.hpp>

#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef STATX_TYPE
// Debian 10: sys/stat.h declares STATX_TYPE etc, linux/stat.h would cause conflict
// Ubuntu 18: sys/stat.h does not declare STATX_TYPE etc, linux/stat.h is needed and does not conflict
#include <linux/stat.h>
#endif

namespace ndn6 {
namespace file_server {

namespace fs = boost::filesystem;

static const uint32_t STATX_REQUIRED = STATX_TYPE | STATX_MODE | STATX_MTIME | STATX_SIZE;
static const uint32_t STATX_OPTIONAL = STATX_ATIME | STATX_CTIME | STATX_BTIME;
static const uint64_t SEGMENT_SIZE = 6144;
static const name::Component lsComponent = name::Component::fromEscapedString("32=ls");
#define ANY "[^<32=ls><32=metadata>]"

enum
{
  TtSegmentSize = 0xF500,
  TtSize = 0xF502,
  TtMode = 0xF504,
  TtAtime = 0xF506,
  TtBtime = 0xF508,
  TtCtime = 0xF50A,
  TtMtime = 0xF50C,
};

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
    return size / SEGMENT_SIZE + static_cast<uint64_t>(size % SEGMENT_SIZE != 0) -
           static_cast<uint64_t>(size > 0);
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
  bool prepare(const fs::path& mountpoint, const ndn::PartialName& rel)
  {
    path = mountpoint;
    for (const name::Component& comp : rel) {
      path /= std::string(reinterpret_cast<const char*>(comp.value()), comp.value_size());
      if (path.filename_is_dot() || path.filename_is_dot_dot()) {
        return false;
      }
    }

    int res = syscall(__NR_statx, -1, path.c_str(), 0, STATX_REQUIRED | STATX_OPTIONAL, &st);
    return res == 0 && has(STATX_REQUIRED);
  }

  size_t size() const
  {
    return st.stx_size;
  }

  bool isFile() const
  {
    return S_ISREG(st.stx_mode);
  }

  bool isDir() const
  {
    return S_ISDIR(st.stx_mode);
  }

  uint64_t mtime() const
  {
    return timestamp(st.stx_mtime);
  }

  bool checkSegmentInterestName(const Name& name) const
  {
    return versioned.isPrefixOf(name) && name[-1].isSegment();
  }

  Block buildMetadata() const
  {
    Block content(tlv::Content);
    content.push_back(versioned.wireEncode());
    if (isFile()) {
      Block finalBlockId(tlv::FinalBlockId);
      uint64_t lastSeg = SegmentLimit::computeLastSeg(size());
      finalBlockId.push_back(name::Component::fromSegment(lastSeg).wireEncode());
      finalBlockId.encode();
      content.push_back(finalBlockId);
      content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(TtSegmentSize, SEGMENT_SIZE));
      content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(TtSize, size()));
    }
    content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(TtMode, st.stx_mode));
    if (has(STATX_ATIME)) {
      content.push_back(
        ndn::encoding::makeNonNegativeIntegerBlock(TtAtime, timestamp(st.stx_atime)));
    }
    if (has(STATX_BTIME)) {
      content.push_back(
        ndn::encoding::makeNonNegativeIntegerBlock(TtBtime, timestamp(st.stx_btime)));
    }
    if (has(STATX_CTIME)) {
      content.push_back(
        ndn::encoding::makeNonNegativeIntegerBlock(TtCtime, timestamp(st.stx_ctime)));
    }
    content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(TtMtime, mtime()));
    content.encode();
    return content;
  }

private:
  bool has(uint32_t bit) const
  {
    return (st.stx_mask & bit) == bit;
  }

  uint64_t timestamp(struct statx_timestamp t) const
  {
    return static_cast<uint64_t>(t.tv_sec) * 1000000000 + t.tv_nsec;
  }

public:
  fs::path path;
  struct statx st;
  Name versioned;
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
  FileInfo parseInterestName(const Name& name, int suffixLen)
  {
    auto rel = name.getSubName(m_prefix.size(), name.size() - m_prefix.size() - suffixLen);
    FileInfo info;
    if (!info.prepare(m_directory, rel)) {
      return FileInfo{};
    }
    info.versioned = name.getPrefix(-suffixLen);
    if (info.isDir()) {
      info.versioned.append(lsComponent);
    }
    info.versioned.appendVersion(info.mtime());
    return info;
  }

  void rdrFile(const Interest& interest)
  {
    auto name = interest.getName();
    auto info = parseInterestName(name, 1);
    replyRdr("RDR-FILE", name, info, info.isFile() || info.isDir());
  }

  void rdrDir(const Interest& interest)
  {
    auto name = interest.getName();
    auto info = parseInterestName(name, 2);
    replyRdr("RDR-DIR", name, info, info.isDir());
  }

  void replyRdr(const char* act, Name name, const FileInfo& info, bool found)
  {
    if (!found) {
      replyNack(name);
      std::cout << act << "-NOT-FOUND" << '\t' << info.path << std::endl;
      return;
    }

    Data data(name.appendVersion().appendSegment(0));
    data.setFreshnessPeriod(1_ms);
    data.setFinalBlock(data.getName().get(-1));
    data.setContent(info.buildMetadata());
    m_keyChain.sign(data);
    m_face.put(data);
    std::cout << act << "-OK" << '\t' << info.path << '\t' << info.versioned << std::endl;
  }

  void readFile(const Interest& interest)
  {
    auto name = interest.getName();
    auto info = parseInterestName(name, 2);
    if (!info.isFile() || !info.checkSegmentInterestName(name)) {
      return;
    }

    auto sl = SegmentLimit::parse(name, info.size());
    if (!sl.ok) {
      return;
    }

    fs::ifstream stream(info.path);
    replySegment("READ-FILE", name, info, sl, stream);
  }

  void readDir(const Interest& interest)
  {
    auto name = interest.getName();
    auto info = parseInterestName(name, 3);
    if (!info.isDir() || !info.checkSegmentInterestName(name)) {
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
      return;
    }

    replySegment("READ-DIR", name, info, sl, stream);
  }

  void replySegment(const char* act, const Name& name, const FileInfo& info, const SegmentLimit& sl,
                    std::istream& stream)
  {
    stream.seekg(sl.seekTo);
    uint8_t buf[SEGMENT_SIZE];
    stream.read(reinterpret_cast<char*>(buf), sl.segLen);
    if (!stream) {
      std::cout << act << "-ERROR" << '\t' << info.path << '\t' << sl.segment << std::endl;
      return;
    }

    Data data(name);
    data.setFinalBlock(name::Component::fromSegment(sl.lastSeg));
    data.setContent(ndn::make_span(buf, sl.segLen));
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
