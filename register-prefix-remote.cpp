#include "common.hpp"

namespace ndn6 {
namespace register_prefix_remote {

static Face face;
static Scheduler sched(face.getIoService());
static KeyChain keyChain;
static nfd::Controller controller(face, keyChain);
static InterestSigner cis(keyChain);
static SigningInfo si;

static std::string faceUri;
static std::shared_ptr<ndn::lp::NextHopFaceIdTag> nexthopTag;

static Name commandPrefix("/localhop/nfd");
static nfd::RibRegisterCommand ribRegister;
static nfd::RibUnregisterCommand ribUnregister;
static std::vector<std::tuple<const char*, const nfd::ControlCommand*, nfd::ControlParameters>>
  commands;
static size_t commandPos = 0;

static void
updateNexthop()
{
  sched.schedule(60_s, updateNexthop);

  controller.fetch<nfd::FaceQueryDataset>(
    nfd::FaceQueryFilter().setRemoteUri(faceUri),
    [](const std::vector<nfd::FaceStatus>& faces) {
      if (faces.empty()) {
        std::cerr << "FaceQuery face not found" << std::endl;
        nexthopTag = nullptr;
        return;
      }

      auto faceId = faces.front().getFaceId();
      if (nexthopTag == nullptr || faceId != nexthopTag->get()) {
        std::cerr << "FaceQuery found " << faceId << std::endl;
        nexthopTag = std::make_shared<lp::NextHopFaceIdTag>(faceId);
      }
    },
    [](uint32_t code, const std::string& reason) {
      std::cerr << "FaceQuery error " << code << " " << reason << std::endl;
    });
}

static void
sendOneCommand()
{
  if (nexthopTag == nullptr) {
    sched.schedule(5_s, sendOneCommand);
    return;
  }
  if (commandPos >= commands.size()) {
    commandPos = 0;
    sched.schedule(180_s, sendOneCommand);
    return;
  }

  const char* verb = nullptr;
  const nfd::ControlCommand* command = nullptr;
  nfd::ControlParameters param;
  std::tie(verb, command, param) = commands.at(commandPos);
  ++commandPos;
  sched.schedule(5_s, sendOneCommand);

  auto interest = cis.makeCommandInterest(command->getRequestName(commandPrefix, param), si);
  interest.setTag(nexthopTag);
  face.expressInterest(
    interest,
    [=](const Interest&, const Data& data) {
      try {
        nfd::ControlResponse response;
        response.wireDecode(data.getContent().blockFromValue());
        std::cerr << verb << " " << param.getName() << " " << response.getCode() << std::endl;
      } catch (const tlv::Error&) {
        std::cerr << verb << " " << param.getName() << " bad-response" << std::endl;
      }
    },
    [=](const Interest&, const lp::Nack& nack) {
      std::cerr << verb << " " << param.getName() << " Nack~" << nack.getReason() << std::endl;
    },
    [=](const Interest&) {
      std::cerr << verb << " " << param.getName() << " timeout" << std::endl;
    });
}

int
main(int argc, char** argv)
{
  auto args = parseProgramOptions(
    argc, argv,
    "Usage: register-prefix-remote -f udp4://192.0.2.1:6363 -p /prefix -i /identity\n"
    "\n"
    "Register and keep prefixes on a remote router.\n"
    "\n",
    [&](auto addOption) {
      addOption("face,f", po::value<std::string>(&faceUri)->required(), "remote FaceUri");
      addOption("prefix,p", po::value<std::vector<Name>>()->composing(), "register prefixes");
      addOption("origin,o", po::value<int>()->default_value(nfd::ROUTE_ORIGIN_CLIENT), "origin");
      addOption("cost,c", po::value<int>()->default_value(0), "cost");
      addOption("no-inherit", "unset ChildInherit flag");
      addOption("capture", "set Capture flag");
      addOption("undo-autoreg", po::value<std::vector<Name>>()->composing(),
                "unregister autoreg prefixes");
      addOption("identity,i", po::value<Name>(), "signing identity");
    });

  if (args.count("identity") > 0) {
    si = signingByIdentity(args["identity"].as<Name>());
  }
  if (args.count("undo-autoreg") > 0) {
    for (const Name& prefix : args["undo-autoreg"].as<std::vector<Name>>()) {
      commands.emplace_back(
        "RibUnregister", &ribUnregister,
        nfd::ControlParameters().setName(prefix).setOrigin(nfd::ROUTE_ORIGIN_AUTOREG));
    }
  }
  if (args.count("prefix") > 0) {
    for (const Name& prefix : args["prefix"].as<std::vector<Name>>()) {
      commands.emplace_back(
        "RibRegister", &ribRegister,
        nfd::ControlParameters()
          .setName(prefix)
          .setOrigin(static_cast<nfd::RouteOrigin>(args["origin"].as<int>()))
          .setCost(args["cost"].as<int>())
          .setFlagBit(nfd::ROUTE_FLAG_CHILD_INHERIT, args.count("no-inherit") == 0, false)
          .setFlagBit(nfd::ROUTE_FLAG_CAPTURE, args.count("capture") > 0, false));
    }
  }

  enableLocalFields(controller, [] {
    updateNexthop();
    sendOneCommand();
  });
  face.processEvents();
  return 0;
}

} // namespace register_prefix_remote
} // namespace ndn6

int
main(int argc, char** argv)
{
  return ndn6::register_prefix_remote::main(argc, argv);
}
