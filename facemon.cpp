#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/face-monitor.hpp>

#include <iostream>
#include <time.h>

using ndn::Face;
using ndn::KeyChain;
using ndn::Name;
using ndn::Interest;

using ndn::nfd::Controller;
using ndn::nfd::ControlParameters;
using ndn::nfd::FaceUpdateCommand;
using ndn::nfd::BIT_LOCAL_FIELDS_ENABLED;

using ndn::nfd::FaceMonitor;
using ndn::nfd::FaceEventNotification;
using ndn::nfd::FACE_EVENT_CREATED;
using ndn::nfd::FACE_EVENT_DESTROYED;

static const std::string TAB = "\t";

void
printInterest(const Name& prefix, const Interest& interest)
{
  auto incomingFaceIdTag = interest.getTag<ndn::lp::IncomingFaceIdTag>();
  if (incomingFaceIdTag == nullptr) {
    return;
  }

  std::cout << time(0) << TAB;
  std::cout << "INTEREST";
  std::cout << TAB << *incomingFaceIdTag;
  for (size_t i = prefix.size(); i < interest.getName().size(); ++i) {
    std::cout << TAB << interest.getName().get(i);
  }
  std::cout << std::endl;
}

void
printNotification(const FaceEventNotification& n)
{
  std::cout << time(0) << TAB;
  switch (n.getKind()) {
  case FACE_EVENT_CREATED:
    std::cout << "CREATED";
    break;
  case FACE_EVENT_DESTROYED:
    std::cout << "DESTROYED";
    break;
  default:
    std::cout << "-";
    break;
  }
  std::cout << TAB << n.getFaceId();
  std::cout << TAB << n.getRemoteUri();
  std::cout << TAB << n.getLocalUri();
  std::cout << std::endl;
}

int
main()
{
  Face face;
  KeyChain keyChain;

  Controller controller(face, keyChain);
  ControlParameters p1;
  p1.setFlagBit(BIT_LOCAL_FIELDS_ENABLED, true);
  controller.start<FaceUpdateCommand>(p1,
    nullptr, std::bind(&std::exit, 1));

  face.setInterestFilter("/localhop/facemon",
    &printInterest, std::bind(&std::exit, 1));

  FaceMonitor fm(face);
  fm.onNotification.connect(&printNotification);
  fm.start();

  face.processEvents();

  return 0;
}
