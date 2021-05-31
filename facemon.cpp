#include "common.hpp"

#include <ndn-cxx/mgmt/nfd/face-monitor.hpp>

namespace ndn6 {
namespace facemon {

void
printInterest(const Name& prefix, const Interest& interest)
{
  auto incomingFace = interest.getTag<lp::IncomingFaceIdTag>();
  if (incomingFace == nullptr) {
    return;
  }

  std::cout << ::time(0) << '\t' << "INTEREST" << '\t' << *incomingFace;
  for (size_t i = prefix.size(); i < interest.getName().size(); ++i) {
    std::cout << '\t' << interest.getName().get(i);
  }
  std::cout << std::endl;
}

void
printNotification(const nfd::FaceEventNotification& n)
{
  std::cout << ::time(0) << '\t';
  switch (n.getKind()) {
  case nfd::FACE_EVENT_CREATED:
    std::cout << "CREATED";
    break;
  case nfd::FACE_EVENT_DESTROYED:
    std::cout << "DESTROYED";
    break;
  default:
    std::cout << "-";
    break;
  }
  std::cout << '\t' << n.getFaceId()
            << '\t' << n.getRemoteUri()
            << '\t' << n.getLocalUri()
            << std::endl;
}

int
main()
{
  Face face;
  KeyChain keyChain;

  nfd::Controller controller(face, keyChain);
  enableLocalFields(controller);
  face.setInterestFilter("/localhop/facemon", printInterest, abortOnRegisterFail);

  nfd::FaceMonitor fm(face);
  fm.onNotification.connect(&printNotification);
  fm.start();

  face.processEvents();
  return 0;
}

} // namespace facemon
} // namespace ndn6

int
main(int argc, char** argv)
{
  return ndn6::facemon::main();
}
