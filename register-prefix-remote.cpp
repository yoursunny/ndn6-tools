#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <iostream>

namespace ndn {
namespace register_prefix_directed {

static Face face;
static Scheduler sched(face.getIoService());
static KeyChain keyChain;
static nfd::Controller controller(face, keyChain);
static security::CommandInterestSigner cis(keyChain);
static security::SigningInfo si;

static std::string faceUri;
static std::shared_ptr<lp::NextHopFaceIdTag> nexthopTag;

static Name commandPrefix("/localhop/nfd");
static nfd::RibRegisterCommand command;
static std::vector<nfd::ControlParameters> commandParams;
static size_t commandPos = 0;

namespace po = boost::program_options;

static void
usage(std::ostream& os, const po::options_description& options)
{
  os << "Usage: register-prefix-remote -f udp4://192.0.2.1:6363 -p /prefix -i /identity\n"
        "\n"
        "Register and keep prefixes on a remote router.\n"
        "\n"
     << options;
}

static void
enableLocalFields(const std::function<void()>& cont)
{
  controller.start<nfd::FaceUpdateCommand>(
    nfd::ControlParameters()
      .setFlagBit(nfd::FaceFlagBit::BIT_LOCAL_FIELDS_ENABLED, true),
    [&] (const auto& cp) {
      std::cerr << "EnableLocalFields OK" << std::endl;
      cont();
    },
    [] (const auto& cr) {
      std::cerr << "EnableLocalFields error " << cr << std::endl;
      std::exit(1);
    });
}

static void
updateNexthop()
{
  sched.schedule(60_s, updateNexthop);

  controller.fetch<nfd::FaceQueryDataset>(
    nfd::FaceQueryFilter().setRemoteUri(faceUri),
    [] (const std::vector<nfd::FaceStatus>& faces) {
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
    [] (uint32_t code, const std::string& reason) {
      std::cerr << "FaceQuery error " << code << " " << reason << std::endl;
    }
  );
}

static void
sendRegisterCommand()
{
  if (nexthopTag == nullptr) {
    sched.schedule(5_s, sendRegisterCommand);
    return;
  }

  auto param = commandParams.at(commandPos);
  ++commandPos;
  if (commandPos >= commandParams.size()) {
    sched.schedule(180_s, sendRegisterCommand);
    commandPos = 0;
  } else {
    sched.schedule(5_s, sendRegisterCommand);
  }

  auto interest = cis.makeCommandInterest(command.getRequestName(commandPrefix, param), si);
  interest.setTag(nexthopTag);
  face.expressInterest(interest,
    [=] (const Interest&, const Data&) {
      std::cerr << "RibRegister " << param.getName() << " OK" << std::endl;
    },
    [=] (const Interest&, const lp::Nack&) {
      std::cerr << "RibRegister " << param.getName() << " Nack" << std::endl;
    },
    [=] (const Interest&) {
      std::cerr << "RibRegister " << param.getName() << " timeout" << std::endl;
    });
}

int
main(int argc, char** argv)
{
  po::options_description options("Options");
  options.add_options()
    ("help,h", "print help message")
    ("face,f", po::value<std::string>(&faceUri)->required(), "remote FaceUri")
    ("prefix,p", po::value<std::vector<Name>>()->required()->composing(), "prefix (repeatable)")
    ("origin,o", po::value<int>()->default_value(0), "origin")
    ("cost,c", po::value<int>()->default_value(0), "cost")
    ("no-inherit,I", "unset ChildInherit flag")
    ("capture,C", "set Capture flag")
    ("identity,i", po::value<Name>(), "signing identity")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  try {
    po::notify(vm);
  }
  catch (const boost::program_options::error&) {
    usage(std::cerr, options);
    return 2;
  }
  if (vm.count("help") > 0) {
    usage(std::cout, options);
    return 0;
  }
  if (vm.count("identity") > 0) {
    si = signingByIdentity(vm["identity"].as<Name>());
  }
  for (const Name& prefix : vm["prefix"].as<std::vector<Name>>()) {
    commandParams.push_back(
      nfd::ControlParameters()
        .setName(prefix)
        .setOrigin(static_cast<nfd::RouteOrigin>(vm["origin"].as<int>()))
        .setCost(vm["cost"].as<int>())
        .setFlagBit(nfd::ROUTE_FLAG_CHILD_INHERIT, vm.count("no-inherit") == 0, false)
        .setFlagBit(nfd::ROUTE_FLAG_CAPTURE, vm.count("capture") > 0, false)
    );
  }

  enableLocalFields([] {
    updateNexthop();
    sendRegisterCommand();
  });
  face.processEvents();
  return 0;
}

} // namespace register_prefix_directed
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::register_prefix_directed::main(argc, argv);
}
