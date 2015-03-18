#include <ndn-cxx/management/nfd-controller.hpp>
#include <stdio.h>
#include <time.h>

namespace ndn {
namespace prefix_allocate {

static const std::string TAB = "\t";
static const int ORIGIN_ALLOCATE = 22804;

class PrefixAllocate : noncopyable
{
public:
  explicit
  PrefixAllocate(const Name& prefix)
    : m_controller(m_face, m_keyChain)
    , m_prefix(prefix)
  {
  }

  void
  run()
  {
    nfd::ControlParameters p1;
    p1.setLocalControlFeature(nfd::LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID);
    m_controller.start<nfd::FaceEnableLocalControlCommand>(p1,
      nfd::Controller::CommandSucceedCallback(), bind(&std::exit, 1));

    m_face.setInterestFilter("ndn:/localhop/prefix-allocate",
      bind(&PrefixAllocate::processCommand, this, _2), bind(&std::exit, 1));

    m_face.processEvents();
  }

private:
  void
  processCommand(const Interest& interest)
  {
    char suffix[30];
    snprintf(suffix, sizeof(suffix), "%d_%d",
             static_cast<int>(::time(nullptr)),
             static_cast<int>(interest.getLocalControlHeader().getIncomingFaceId()));
    Name prefix(m_prefix);
    prefix.append(suffix);

    nfd::ControlParameters p;
    p.setName(prefix);
    p.setFaceId(interest.getLocalControlHeader().getIncomingFaceId());
    p.setOrigin(ORIGIN_ALLOCATE);
    p.setCost(800);
    m_controller.start<nfd::RibRegisterCommand>(p,
      bind(&PrefixAllocate::onRegisterSucceed, this, _1, interest),
      bind(&PrefixAllocate::onRegisterFail, this, p, _1));
  }

  void
  onRegisterSucceed(const nfd::ControlParameters& p, const Interest& interest)
  {
    shared_ptr<Data> data = make_shared<Data>(interest.getName());
    data->setContent(p.getName().wireEncode());
    m_keyChain.sign(*data);
    m_face.put(*data);

    std::cout << ::time(nullptr) << TAB
              << 0 << TAB
              << p.getFaceId() << TAB
              << p.getName() << std::endl;
  }

  void
  onRegisterFail(const nfd::ControlParameters& p, uint32_t code)
  {
    std::cout << ::time(nullptr) << TAB
              << code << TAB
              << p.getFaceId() << TAB
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
    std::cerr << "prefix-allocate /prefix" << std::endl;
    return -1;
  }

  PrefixAllocate app(argv[1]);
  app.run();

  return 0;
}

} // namespace prefix_allocate
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::prefix_allocate::main(argc, argv);
}
