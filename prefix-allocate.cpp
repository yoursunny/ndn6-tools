#include "common.hpp"

namespace ndn6::prefix_allocate {

static const int ORIGIN_ALLOCATE = 22804;

class PrefixAllocate : boost::noncopyable
{
public:
  explicit PrefixAllocate(const Name& prefix)
    : m_controller(m_face, m_keyChain)
    , m_prefix(prefix)
  {}

  void run()
  {
    enableLocalFields(m_controller);
    m_face.setInterestFilter("/localhop/prefix-allocate",
                             std::bind(&PrefixAllocate::processCommand, this, _2),
                             abortOnRegisterFail);
    m_face.processEvents();
  }

private:
  void processCommand(const Interest& interest)
  {
    auto incomingFaceIdTag = interest.getTag<lp::IncomingFaceIdTag>();
    if (incomingFaceIdTag == nullptr) {
      return;
    }

    char suffix[30];
    snprintf(suffix, sizeof(suffix), "%d_%d", static_cast<int>(::time(nullptr)),
             static_cast<int>(*incomingFaceIdTag));
    Name prefix(m_prefix);
    prefix.append(suffix);

    nfd::ControlParameters p;
    p.setName(prefix);
    p.setFaceId(*incomingFaceIdTag);
    p.setOrigin(static_cast<nfd::RouteOrigin>(ORIGIN_ALLOCATE));
    p.setCost(800);
    m_controller.start<nfd::RibRegisterCommand>(
      p, bind(&PrefixAllocate::onRegisterSucceed, this, _1, interest),
      bind(&PrefixAllocate::onRegisterFail, this, p, _1));
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
  Name m_prefix;
};

int
main(int argc, char** argv)
{
  if (argc != 2) {
    std::cerr << "ndn6-prefix-allocate /prefix" << std::endl;
    return -1;
  }

  PrefixAllocate app(argv[1]);
  app.run();

  return 0;
}

} // namespace ndn6::prefix_allocate

int
main(int argc, char** argv)
{
  return ndn6::prefix_allocate::main(argc, argv);
}
