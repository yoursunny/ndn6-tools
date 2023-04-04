#include "common.hpp"

#include <queue>

namespace ndn6::register_prefix_remote {

static Face face;
static Scheduler sched(face.getIoService());
static KeyChain keyChain;
static nfd::Controller controller(face, keyChain);
static InterestSigner cis(keyChain);
static SigningInfo si;

static std::string faceUri;
static std::shared_ptr<ndn::lp::NextHopFaceIdTag> nexthopTag;

static std::optional<time::milliseconds> regExpiration;
static bool toLocal = false;
static Name nlsrRouter;
static std::vector<Name> nlsrNamesFilter;
static std::set<Name> nlsrNames;

class LsdbNamesDataset : public nfd::StatusDataset
{
public:
  using Base = nfd::StatusDataset;

  LsdbNamesDataset()
    : Base("nlsr/lsdb/names")
  {}

  using ResultType = std::set<Name>;

  ResultType parseResult(ndn::ConstBufferPtr payload) const
  {
    std::set<Name> names;
    size_t offset = 0;
    while (offset < payload->size()) {
      bool isOk = false;
      Block lsa;
      std::tie(isOk, lsa) = Block::fromBuffer(payload, offset);
      if (!isOk) {
        break;
      }
      offset += lsa.size();

      lsa.parse();
      for (const auto& element : lsa.elements()) {
        if (element.type() == tlv::Name) {
          names.emplace(element);
        }
      }
    }
    return names;
  }
};

static Name commandPrefix("/localhop/nfd");
static nfd::RibRegisterCommand ribRegister;
static nfd::RibUnregisterCommand ribUnregister;

enum class CommandKind
{
  SENTINEL,
  UPDATE_NEXTHOP,
  REGISTER,
  UNDO_AUTOREG,
  UPDATE_NLSR_DATASET,
  NLSR_SYNC,
};

struct Command
{
  CommandKind kind;
  Name prefix;
};

static std::queue<Command> commands;

static void
scheduleNext(bool recur = true);

static void
updateNexthop()
{
  controller.fetch<nfd::FaceQueryDataset>(
    nfd::FaceQueryFilter().setRemoteUri(faceUri),
    [](const std::vector<nfd::FaceStatus>& faces) {
      if (faces.empty()) {
        std::cerr << "FaceQuery face not found" << std::endl;
        nexthopTag = nullptr;
        scheduleNext();
        return;
      }

      auto faceId = faces.front().getFaceId();
      if (nexthopTag == nullptr || faceId != nexthopTag->get()) {
        std::cerr << "FaceQuery found " << faceId << std::endl;
        nexthopTag = std::make_shared<lp::NextHopFaceIdTag>(faceId);
      }
      scheduleNext();
    },
    [](uint32_t code, const std::string& reason) {
      std::cerr << "FaceQuery error " << code << " " << reason << std::endl;
      scheduleNext();
    });
}

static void
updateNlsrDataset()
{
  controller.fetch<LsdbNamesDataset>(
    [](const std::set<Name>& dataset) {
      std::set<Name> acceptNames;
      for (const auto& name : dataset) {
        if (std::none_of(nlsrNamesFilter.begin(), nlsrNamesFilter.end(),
                         [=](const Name& prefix) { return prefix.isPrefixOf(name); })) {
          continue;
        }
        acceptNames.insert(name);
        if (nlsrNames.count(name) == 0) {
          std::cerr << "LSDB-names new " << name << std::endl;
          commands.push({ CommandKind::NLSR_SYNC, name });
        }
      }
      nlsrNames.swap(acceptNames);
      scheduleNext();
    },
    [](uint32_t code, const std::string& reason) {
      std::cerr << "LSDB-names error " << code << " " << reason << std::endl;
      scheduleNext();
    },
    nfd::CommandOptions().setPrefix(nlsrRouter));
}

static void
regUnregPrefix(const Command& cmd)
{
  if (nexthopTag == nullptr) {
    scheduleNext();
    return;
  }

  bool recur = true;
  const char* verb = nullptr;
  const nfd::ControlCommand* cc = nullptr;
  nfd::ControlParameters param;
  param.setName(cmd.prefix);
  param.setOrigin(nfd::ROUTE_ORIGIN_CLIENT);
  if (toLocal) {
    param.setFaceId(nexthopTag->get());
  }
  switch (cmd.kind) {
    case CommandKind::REGISTER:
      verb = "register";
      cc = &ribRegister;
      break;
    case CommandKind::UNDO_AUTOREG:
      verb = "undo-autoreg";
      cc = &ribUnregister;
      param.setOrigin(nfd::ROUTE_ORIGIN_AUTOREG);
      break;
    case CommandKind::NLSR_SYNC:
      if (nlsrNames.count(cmd.prefix) > 0) {
        verb = "nlsr-advertise";
        cc = &ribRegister;
      } else {
        verb = "nlsr-withdraw";
        cc = &ribUnregister;
        recur = false;
      }
      break;
    default:
      BOOST_ASSERT(false);
      break;
  }
  if (cc == &ribRegister) {
    param.setFlagBit(nfd::ROUTE_FLAG_CAPTURE, true, false);
    if (regExpiration) {
      param.setExpirationPeriod(*regExpiration);
    }
  }

  Interest interest(cc->getRequestName(commandPrefix, param));
  cis.makeSignedInterest(interest, si);
  if (!toLocal) {
    interest.setTag(nexthopTag);
  }
  face.expressInterest(
    interest,
    [=](const Interest&, const Data& data) {
      try {
        nfd::ControlResponse response;
        response.wireDecode(data.getContent().blockFromValue());
        std::cerr << verb << " " << param.getName() << " " << response.getCode() << std::endl;
      } catch (const tlv::Error& e) {
        std::cerr << verb << " " << param.getName() << " bad-response " << e.what() << std::endl;
      }
      scheduleNext(recur);
    },
    [=](const Interest&, const lp::Nack& nack) {
      std::cerr << verb << " " << param.getName() << " Nack~" << nack.getReason() << std::endl;
      scheduleNext(recur);
    },
    [=](const Interest&) {
      std::cerr << verb << " " << param.getName() << " timeout" << std::endl;
      scheduleNext(recur);
    });
}

static void
runFrontCommand()
{
  const auto& cmd = commands.front();
  switch (cmd.kind) {
    case CommandKind::SENTINEL:
      scheduleNext();
      break;
    case CommandKind::UPDATE_NEXTHOP:
      updateNexthop();
      break;
    case CommandKind::UPDATE_NLSR_DATASET:
      updateNlsrDataset();
      break;
    case CommandKind::REGISTER:
    case CommandKind::UNDO_AUTOREG:
    case CommandKind::NLSR_SYNC:
      regUnregPrefix(cmd);
      break;
  }
}

static void
scheduleNext(bool recur)
{
  auto cmd = commands.front();
  commands.pop();
  auto delay = cmd.kind == CommandKind::SENTINEL ? 60_s : 2_s;
  if (recur) {
    commands.push(std::move(cmd));
  }

  sched.schedule(delay, runFrontCommand);
}

int
main(int argc, char** argv)
{
  auto args = parseProgramOptions(
    argc, argv,
    "Usage: ndn6-register-prefix-remote -f udp4://192.0.2.1:6363 -p /prefix -i /identity\n"
    "\n"
    "Register and keep prefixes on a remote router.\n"
    "\n",
    [&](auto addOption) {
      addOption("face,f", po::value<std::string>(&faceUri)->required(), "remote FaceUri");
      addOption("prefix,p", po::value<std::vector<Name>>()->composing(), "register prefixes");
      addOption("undo-autoreg", po::value<std::vector<Name>>()->composing(),
                "unregister autoreg prefixes");
      addOption("nlsr-router", po::value<Name>(&nlsrRouter)->default_value("/localhost"),
                "NLSR router name");
      addOption("nlsr-readvertise", po::value<std::vector<Name>>(&nlsrNamesFilter)->composing(),
                "readvertise NLSR prefixes");
      addOption("nlsr-to-local", po::bool_switch(&toLocal), "readvertise to local NLSR instead");
      addOption("identity,i", po::value<Name>(), "signing identity");
      addOption("expiry", po::value<uint32_t>(), "registration expiration (seconds)");
    });

  commands.push({ CommandKind::UPDATE_NEXTHOP, "" });
  if (args.count("undo-autoreg") > 0) {
    for (const Name& prefix : args["undo-autoreg"].as<std::vector<Name>>()) {
      commands.push({ CommandKind::UNDO_AUTOREG, prefix });
    }
  }
  if (args.count("prefix") > 0) {
    for (const Name& prefix : args["prefix"].as<std::vector<Name>>()) {
      commands.push({ CommandKind::REGISTER, prefix });
    }
  }
  if (!nlsrNamesFilter.empty()) {
    commands.push({ CommandKind::UPDATE_NLSR_DATASET, "" });
  }
  commands.push({ CommandKind::SENTINEL, "" });

  if (toLocal) {
    commandPrefix = "/localhost/nfd";
  }
  if (args.count("identity") > 0) {
    si = signingByIdentity(args["identity"].as<Name>());
  }
  if (args.count("expiry") > 0) {
    regExpiration.emplace(1000 * args["expiry"].as<uint32_t>());
  }

  enableLocalFields(controller, runFrontCommand);
  face.processEvents();
  return 0;
}

} // namespace ndn6::register_prefix_remote

int
main(int argc, char** argv)
{
  return ndn6::register_prefix_remote::main(argc, argv);
}
