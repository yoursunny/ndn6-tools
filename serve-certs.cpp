#include "common.hpp"

namespace ndn6::serve_certs {

const auto FETCH_TIMEOUT = 7777_ms;
const auto FETCH_RETRY = 7222_ms;

class ServeCerts : boost::noncopyable {
public:
  explicit ServeCerts(Face& face, bool wantIntermediates)
    : m_face(face)
    , m_sched(face.getIoContext())
    , m_wantIntermediates(wantIntermediates) {}

  void add(const Data& data) {
    auto certName = data.getName();
    auto keyName = ndn::security::extractKeyNameFromCertName(data.getName());
    if (m_serving.count(keyName) > 0) {
      return;
    }

    std::cout << "<R\t" << keyName << std::endl;
    m_serving.emplace(keyName, m_face.setInterestFilter(
                                 keyName,
                                 [this, data](const Name&, const Interest& interest) {
                                   std::cout << ">I\t" << interest << std::endl;
                                   if (interest.matchesData(data)) {
                                     std::cout << "<D\t" << data.getName() << std::endl;
                                     m_face.put(data);
                                   } else {
                                     auto nack = Nack(interest).setReason(lp::NackReason::NO_ROUTE);
                                     std::cout << "<N\t" << interest << '~' << nack.getReason()
                                               << std::endl;
                                     m_face.put(nack);
                                   }
                                 },
                                 abortOnRegisterFail));

    if (m_wantIntermediates) {
      gatherIntermediate(data);
    }
  }

private:
  void gatherIntermediate(const Data& data) {
    auto issuer = data.getKeyLocator()->getName();
    bool isCertName = Certificate::isValidName(issuer);
    auto keyName = isCertName ? ndn::security::extractKeyNameFromCertName(issuer) : issuer;
    if (m_serving.count(keyName) > 0 || m_fetching.count(issuer) > 0) {
      return;
    }

    Interest interest(issuer);
    if (!isCertName) {
      interest.setCanBePrefix(true);
      interest.setMustBeFresh(true);
    }
    interest.setInterestLifetime(FETCH_TIMEOUT);
    fetchAdd(interest);
  }

  void fetchAdd(const Interest& interest) {
    std::cout << "<I\t" << interest.getName() << std::endl;
    m_fetching.emplace(interest.getName(), m_sched.schedule(FETCH_RETRY, [this, interest] {
      std::cout << ">T\t" << interest.getName() << std::endl;
      fetchAdd(interest);
    }));
    m_face.expressInterest(
      interest,
      [this](const Interest& interest, const Data& data) {
        std::cout << ">D\t" << data.getName() << std::endl;
        m_fetching.erase(interest.getName());

        try {
          Certificate cert(data);
          if (cert.getKeyLocator()->getName().isPrefixOf(data.getName())) {
            std::cout << "!S\t" << data.getName() << std::endl;
            return;
          }
        } catch (const tlv::Error&) {
          std::cout << "!C\t" << data.getName() << std::endl;
          return;
        }

        add(data);
      },
      nullptr, nullptr);
  }

private:
  Face& m_face;
  Scheduler m_sched;
  bool m_wantIntermediates;
  std::unordered_map<Name, ndn::ScopedRegisteredPrefixHandle> m_serving;
  std::unordered_map<Name, ndn::scheduler::ScopedEventId> m_fetching;
};

int
main(int argc, char** argv) {
  bool wantIntermediates = false;
  std::vector<std::string> certFiles;
  auto args = parseProgramOptions(
    argc, argv, "ndn6-serve-certs cert-file cert-file...\n",
    [&](auto addOption) {
      addOption("inter", po::bool_switch(&wantIntermediates), "gather and serve intermediates");
      addOption("cert-file", po::value(&certFiles)->required()->composing(),
                "base64 certificate file");
    },
    "cert-file");

  ndn::Face face;
  ServeCerts app(face, wantIntermediates);

  for (const auto& certFile : certFiles) {
    Certificate cert;
    try {
      std::ifstream is(certFile);
      cert = io::loadTlv<Certificate>(is);
    } catch (const io::Error& e) {
      std::cerr << certFile << ": " << e.what() << std::endl;
      return 1;
    }
    app.add(cert);
  }

  face.processEvents();

  return 0;
}

} // namespace ndn6::serve_certs

int
main(int argc, char** argv) {
  return ndn6::serve_certs::main(argc, argv);
}
