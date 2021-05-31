#include "common.hpp"
#include <ndn-cxx/util/sha256.hpp>

namespace ndn6 {
namespace prefix_request {

static const int ORIGIN_PREFIX_REQUEST = 19438;
static const int CODE_WRONG_ANSWER = 8401;

class PrefixRequest : boost::noncopyable
{
public:
  explicit PrefixRequest(const std::string& secret)
    : m_controller(m_face, m_keyChain)
    , m_secret(secret)
  {}

  void run()
  {
    enableLocalFields(m_controller);
    m_face.setInterestFilter("/localhop/prefix-request",
                             std::bind(&PrefixRequest::processCommand, this, _2),
                             abortOnRegisterFail);

    m_face.processEvents();
  }

private:
  void processCommand(const Interest& interest)
  {
    // /localhop/prefix-request/<prefix-uri>/<answer>/<random>
    // <answer> == SHA256(<secret> <prefix-uri>) in upper case
    if (interest.getName().size() != 5) {
      return; // bad command
    }

    // parse prefix
    const auto& prefixUriArg = interest.getName().at(2);
    std::string prefixUri(reinterpret_cast<const char*>(prefixUriArg.value()),
                          prefixUriArg.value_size());

    Name prefix;
    try {
      prefix = Name(prefixUri);
    } catch (const Name::Error&) {
      return; // cannot decode Name
    }
    if (prefix.toUri() != prefixUri) {
      return; // URI not canonical
    }

    auto incomingFaceIdTag = interest.getTag<lp::IncomingFaceIdTag>();
    if (incomingFaceIdTag == nullptr) {
      return;
    }

    nfd::ControlParameters p;
    p.setName(prefix);
    p.setFaceId(*incomingFaceIdTag);
    p.setOrigin(static_cast<nfd::RouteOrigin>(ORIGIN_PREFIX_REQUEST));
    p.setCost(800);

    // verify answer
    if (!verifyAnswer(interest)) {
      std::cout << ::time(nullptr) << '\t' << CODE_WRONG_ANSWER << '\t' << p.getFaceId() << '\t'
                << p.getName() << std::endl;
      return;
    }

    m_controller.start<nfd::RibRegisterCommand>(
      p, bind(&PrefixRequest::onRegisterSucceed, this, _1, interest),
      bind(&PrefixRequest::onRegisterFail, this, p, _1));
  }

  bool verifyAnswer(const Interest& interest)
  {
    const auto& prefixUriArg = interest.getName().at(2);

    const auto& answerArg = interest.getName().at(3);
    std::string answer(reinterpret_cast<const char*>(answerArg.value()), answerArg.value_size());

    ndn::util::Sha256 digest;
    digest << m_secret;
    digest.update(prefixUriArg.value(), prefixUriArg.value_size());
    std::string answer2 = digest.toString();

    if (answer.size() != answer2.size()) {
      return false;
    }

    int diff = 0;
    for (size_t pos = 0; pos < answer.size(); ++pos) {
      diff |= answer[pos] ^ answer2[pos];
    }
    return diff == 0;
  }

  void onRegisterSucceed(const nfd::ControlParameters& p, const Interest& interest)
  {
    auto data = std::make_shared<Data>(interest.getName());
    data->setContent(p.getName().wireEncode());
    m_keyChain.sign(*data);
    m_face.put(*data);

    std::cout << ::time(nullptr) << '\t' << 0 << '\t' << p.getFaceId() << '\t' << p.getName()
              << std::endl;
  }

  void onRegisterFail(const nfd::ControlParameters& p, const nfd::ControlResponse& resp)
  {
    std::cout << ::time(nullptr) << '\t' << resp.getCode() << '\t' << p.getFaceId() << '\t'
              << p.getName() << std::endl;
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  nfd::Controller m_controller;
  std::string m_secret;
};

int
main(int argc, char** argv)
{
  if (argc != 2) {
    std::cerr << "prefix-request secret" << std::endl;
    return -1;
  }

  PrefixRequest app(argv[1]);
  app.run();

  return 0;
}

} // namespace prefix_request
} // namespace ndn6

int
main(int argc, char** argv)
{
  return ndn6::prefix_request::main(argc, argv);
}
