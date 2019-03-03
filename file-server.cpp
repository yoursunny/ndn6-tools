#include <ndn-cxx/face.hpp>
#include <ndn-cxx/metadata-object.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <cstdlib>
#include <iostream>

namespace ndn {
namespace file_server {

namespace fs = boost::filesystem;

static const uint64_t SEGMENT_SIZE = 5120;

class FileServer : noncopyable
{
public:
  explicit
  FileServer(Face& face, KeyChain& keyChain, const Name& prefix, const fs::path& directory)
    : m_face(face)
    , m_keyChain(keyChain)
    , m_prefix(prefix)
    , m_directory(directory)
  {
    face.registerPrefix(prefix,
      [] (const Name&) {},
      [] (const Name&, const std::string& err) {
        std::cerr << "REGISTER-PREFIX-ERR\t" << err << std::endl;
        std::exit(3);
      });

    face.setInterestFilter(
      InterestFilter(prefix, "[^<32=ls>]*<32=metadata>"),
      std::bind(&FileServer::readRdr, this, _2));

    face.setInterestFilter(
      InterestFilter(prefix, "[^<32=ls><32=metadata>]{2,}"),
      std::bind(&FileServer::readSegment, this, _2));
  }

private:
  std::tuple<fs::path, Name>
  parseRead(const Name& interestName, int suffixLen)
  {
    PartialName rel = interestName.getSubName(m_prefix.size(),
                      interestName.size() - m_prefix.size() - suffixLen);
    fs::path path = m_directory / fs::path(rel.toUri());

    fs::file_status stat = fs::status(path);
    boost::system::error_code ec;
    std::time_t writeTime = fs::last_write_time(path, ec);

    if (!fs::is_regular_file(stat) || ec) {
      return std::make_tuple(path, Name());
    }

    Name versioned = interestName.getPrefix(-suffixLen);
    versioned.appendVersion(writeTime * 1000000);
    return std::make_tuple(path, versioned);
  }

  void
  readRdr(const Interest& interest)
  {
    const Name& name = interest.getName();
    fs::path path;
    Name versioned;
    std::tie(path, versioned) = parseRead(name, 1);
    if (versioned.empty()) {
      replyNack(interest);
      std::cerr << "READ-RDR-NOT-FOUND\t" << path << std::endl;
      return;
    }

    MetadataObject mo;
    mo.setVersionedName(versioned);
    Data data = mo.makeData(interest.getName(), m_keyChain);
    m_face.put(data);
    std::cerr << "READ-RDR\t" << path << '\t' << versioned << std::endl;
  }

  void
  readSegment(const Interest& interest)
  {
    const Name& name = interest.getName();
    fs::path path;
    Name versioned;
    std::tie(path, versioned) = parseRead(name, 2);
    if (versioned.empty() || !versioned.isPrefixOf(name) || !name[-1].isSegment()) {
      replyNack(interest);
      return;
    }

    uint64_t segment = name[-1].toSegment();
    boost::system::error_code ec;
    uint64_t size = static_cast<uint64_t>(fs::file_size(path, ec));
    uint64_t lastSeg = size / SEGMENT_SIZE + static_cast<int>(size % SEGMENT_SIZE != 0) - 1;
    if (ec || segment > lastSeg) {
      replyNack(interest);
      return;
    }

    fs::ifstream stream(path);
    stream.seekg(segment * SEGMENT_SIZE);
    char buf[SEGMENT_SIZE];
    uint64_t segLen = segment == lastSeg && size % SEGMENT_SIZE != 0 ?
                      size % SEGMENT_SIZE : SEGMENT_SIZE;
    stream.read(buf, segLen);

    Data data(name);
    data.setFinalBlock(name::Component::fromSegment(lastSeg));
    data.setContent(reinterpret_cast<const uint8_t*>(buf), segLen);
    m_keyChain.sign(data);
    m_face.put(data);
    std::cerr << "READ-SEGMENT\t" << path << '\t' << segment << '\t' << segLen << std::endl;
  }

  void
  replyNack(const Interest& interest)
  {
    Data data(interest.getName());
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

  ndn::Face face;
  ndn::KeyChain keyChain;
  FileServer app(face, keyChain, argv[1], argv[2]);
  face.processEvents();
  return 0;
}

} // namespace file_server
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::file_server::main(argc, argv);
}

