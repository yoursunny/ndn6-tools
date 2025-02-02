#include "common.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <sys/stat.h>
#include <unistd.h>

namespace ndn6::file_server {

namespace fs = boost::filesystem;

static const uint32_t STATX_REQUIRED = STATX_TYPE | STATX_MODE | STATX_MTIME | STATX_SIZE;
static const uint32_t STATX_OPTIONAL = STATX_ATIME | STATX_CTIME | STATX_BTIME;
static const name::Component lsComponent(ndn::tlv::KeywordNameComponent, {'l', 's'});
#define ANY "[^<32=ls><32=metadata>]"

enum {
  TtSegmentSize = 0xF500,
  TtSize = 0xF502,
  TtMode = 0xF504,
  TtAtime = 0xF506,
  TtBtime = 0xF508,
  TtCtime = 0xF50A,
  TtMtime = 0xF50C,
};

class SegmentLimit {
public:
  static SegmentLimit parse(const Name& name, uint64_t size, uint64_t segmentSize) {
    SegmentLimit sl;
    if (size == 0) {
      sl.ok = true;
      return sl;
    }

    sl.segment = name[-1].toSegment();
    sl.seekTo = sl.segment * segmentSize;
    sl.lastSeg = computeLastSeg(size, segmentSize);
    sl.segLen =
      sl.segment == sl.lastSeg && size % segmentSize != 0 ? size % segmentSize : segmentSize;
    sl.ok = sl.segment <= sl.lastSeg;
    return sl;
  }

  static uint64_t computeLastSeg(uint64_t size, uint64_t segmentSize) {
    return size / segmentSize + static_cast<uint64_t>(size % segmentSize != 0) -
           static_cast<uint64_t>(size > 0);
  }

public:
  bool ok = false;
  uint64_t segment = 0;
  uint64_t seekTo = 0;
  uint64_t segLen = 0;
  uint64_t lastSeg = 0;
};

class FileInfo {
public:
  bool prepare(const fs::path& mountpoint, const ndn::PartialName& rel, uint64_t segmentSize) {
    this->segmentSize = segmentSize;

    path = mountpoint;
    for (const name::Component& comp : rel) {
      path /= std::string(reinterpret_cast<const char*>(comp.value()), comp.value_size());
      if (path.filename_is_dot() || path.filename_is_dot_dot()) {
        return false;
      }
    }

    int res = statx(-1, path.c_str(), 0, STATX_REQUIRED | STATX_OPTIONAL, &st);
    return res == 0 && has(STATX_REQUIRED);
  }

  size_t size() const {
    return st.stx_size;
  }

  bool isFile() const {
    return S_ISREG(st.stx_mode);
  }

  bool isDir() const {
    return S_ISDIR(st.stx_mode);
  }

  uint64_t mtime() const {
    return timestamp(st.stx_mtime);
  }

  bool checkSegmentInterestName(const Name& name) const {
    return versioned.isPrefixOf(name) && name[-1].isSegment();
  }

  Block buildMetadata() const {
    Block content(tlv::Content);
    content.push_back(versioned.wireEncode());
    if (isFile()) {
      Block finalBlockId(tlv::FinalBlockId);
      uint64_t lastSeg = SegmentLimit::computeLastSeg(size(), segmentSize);
      finalBlockId.push_back(name::Component::fromSegment(lastSeg).wireEncode());
      finalBlockId.encode();
      content.push_back(finalBlockId);
      content.push_back(ndn::encoding::makeNonNegativeIntegerBlock(TtSegmentSize, segmentSize));
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
  bool has(uint32_t bit) const {
    return (st.stx_mask & bit) == bit;
  }

  uint64_t timestamp(struct statx_timestamp t) const {
    return static_cast<uint64_t>(t.tv_sec) * 1000000000 + t.tv_nsec;
  }

public:
  fs::path path;
  struct statx st;
  Name versioned;
  uint64_t segmentSize;
};

class FileServer : boost::noncopyable {
public:
  explicit FileServer(Face& face, KeyChain& keyChain, const Name& servePrefix,
                      const Name& discoveryPrefix, const fs::path& directory, int segmentSize)
    : m_face(face)
    , m_keyChain(keyChain)
    , m_servePrefix(servePrefix)
    , m_directory(directory)
    , m_segmentSize(segmentSize) {
    std::vector<Name> prefixes{servePrefix};
    if (!discoveryPrefix.equals(servePrefix)) {
      prefixes.push_back(discoveryPrefix);
    }
    for (const Name& prefix : prefixes) {
      face.registerPrefix(prefix, nullptr, abortOnRegisterFail);
      face.setInterestFilter(InterestFilter(prefix, ANY "*<32=metadata>"),
                             std::bind(&FileServer::rdrFile, this, _1, _2));
      face.setInterestFilter(InterestFilter(prefix, ANY "*<32=ls><32=metadata>"),
                             std::bind(&FileServer::rdrDir, this, _1, _2));
    }
    face.setInterestFilter(InterestFilter(servePrefix, ANY "{2,}"),
                           std::bind(&FileServer::readFile, this, _1, _2));
    face.setInterestFilter(InterestFilter(servePrefix, ANY "*<32=ls>" ANY "{2}"),
                           std::bind(&FileServer::readDir, this, _1, _2));
  }

private:
  FileInfo parseInterestName(const ndn::InterestFilter& filter, const Name& name, int suffixLen) {
    size_t prefixLen = filter.getPrefix().size();
    auto rel = name.getSubName(prefixLen, name.size() - prefixLen - suffixLen);
    FileInfo info;
    if (!info.prepare(m_directory, rel, m_segmentSize)) {
      return FileInfo{};
    }
    info.versioned = m_servePrefix;
    info.versioned.append(rel);
    if (info.isDir()) {
      info.versioned.append(lsComponent);
    }
    info.versioned.appendVersion(info.mtime());
    return info;
  }

  void rdrFile(const ndn::InterestFilter& filter, const Interest& interest) {
    auto name = interest.getName();
    auto info = parseInterestName(filter, name, 1);
    replyRdr("RDR-FILE", name, info, info.isFile() || info.isDir());
  }

  void rdrDir(const ndn::InterestFilter& filter, const Interest& interest) {
    auto name = interest.getName();
    auto info = parseInterestName(filter, name, 2);
    replyRdr("RDR-DIR", name, info, info.isDir());
  }

  void replyRdr(const char* act, Name name, const FileInfo& info, bool found) {
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

  void readFile(const ndn::InterestFilter& filter, const Interest& interest) {
    auto name = interest.getName();
    auto info = parseInterestName(filter, name, 2);
    if (!info.isFile() || !info.checkSegmentInterestName(name)) {
      return;
    }

    auto sl = SegmentLimit::parse(name, info.size(), m_segmentSize);
    if (!sl.ok) {
      return;
    }

    fs::ifstream stream(info.path);
    replySegment("READ-FILE", name, info, sl, stream);
  }

  void readDir(const ndn::InterestFilter& filter, const Interest& interest) {
    auto name = interest.getName();
    auto info = parseInterestName(filter, name, 3);
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

    auto sl = SegmentLimit::parse(name, stream.tellp(), m_segmentSize);
    if (!sl.ok) {
      return;
    }

    replySegment("READ-DIR", name, info, sl, stream);
  }

  void replySegment(const char* act, const Name& name, const FileInfo& info, const SegmentLimit& sl,
                    std::istream& stream) {
    stream.seekg(sl.seekTo);
    uint8_t buf[m_segmentSize];
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

  void replyNack(const Name& name) {
    Data data(name);
    data.setContentType(tlv::ContentType_Nack);
    data.setFreshnessPeriod(1_ms);
    m_keyChain.sign(data);
    m_face.put(data);
  }

private:
  Face& m_face;
  KeyChain& m_keyChain;
  Name m_servePrefix;
  fs::path m_directory;
  uint64_t m_segmentSize;
};

int
main(int argc, char** argv) {
  Name servePrefix;
  Name discoveryPrefix;
  fs::path directory;
  int segmentSize = 6144;
  auto args = parseProgramOptions(
    argc, argv,
    "Usage: ndn6-file-server\n"
    "\n"
    "Serve files from a directory.\n"
    "\n",
    [&](auto addOption) {
      addOption("listen,b", po::value(&servePrefix)->required(), "serve prefix");
      addOption("discovery,D", po::value(&discoveryPrefix), "discovery prefix");
      addOption("directory,d", po::value(&directory)->required(), "local directory");
      addOption("segment-size,s", po::value(&segmentSize)->notifier([](uint64_t v) {
        if (!(v >= 1 && v <= 8192)) {
          throw std::range_error("segment-size must be between 1 and 8192");
        }
      }),
                "segment size");
    });
  if (args.count("discovery") == 0) {
    discoveryPrefix = servePrefix;
  }

  name::setConventionDecoding(name::Convention::TYPED);
  ndn::Face face;
  ndn::KeyChain keyChain;
  FileServer app(face, keyChain, servePrefix, discoveryPrefix, directory, segmentSize);
  face.processEvents();
  return 0;
}

} // namespace ndn6::file_server

int
main(int argc, char** argv) {
  return ndn6::file_server::main(argc, argv);
}
