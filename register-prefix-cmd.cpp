#include "common.hpp"

#include <ndn-cxx/util/time-unit-test-clock.hpp>

namespace ndn6 {
namespace register_prefix_cmd {

static void
usage(std::ostream& os, const po::options_description& options)
{
  os << "Usage: register-prefix-cmd [-u] -r /laptop-prefix -f 256 -i /identity\n"
        "\n"
        "Prepare a prefix (un)registration command.\n"
        "\n"
     << options;
}

int
main(int argc, char** argv)
{
  Name prefix;
  Name commandPrefix("/localhop/nfd");
  int faceId = -1;
  int origin = 0;
  SigningInfo si;
  int advanceClock = 0;

  po::options_description options("Options");
  options.add_options()
    ("help,h", "print help message")
    ("unregister,u", "unregister")
    ("prefix,p", po::value<Name>(&prefix)->required(), "prefix")
    ("command,P", po::value<Name>(&commandPrefix), "command prefix")
    ("face,f", po::value<int>(&faceId), "FaceId, default is self")
    ("origin,o", po::value<int>(&origin), "origin")
    ("no-inherit,I", "unset ChildInherit flag")
    ("capture,C", "set Capture flag")
    ("identity,i", po::value<Name>(), "signing identity")
    ("advance-clock", po::value<int>(&advanceClock), "advance clock (millis)")
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
  if (advanceClock > 0) {
    auto now = time::system_clock::now();
    auto clock = std::make_shared<time::UnitTestSystemClock>();
    clock->setNow(now.time_since_epoch() + time::milliseconds(advanceClock));
    time::setCustomClocks(nullptr, clock);
  }

  nfd::ControlParameters params;
  params.setName(prefix);
  if (faceId >= 0) {
    params.setFaceId(faceId);
  }
  params.setOrigin(static_cast<nfd::RouteOrigin>(origin));
  std::unique_ptr<nfd::ControlCommand> command;
  if (vm.count("unregister") > 0) {
    command = std::make_unique<nfd::RibUnregisterCommand>();
  }
  else {
    command = std::make_unique<nfd::RibRegisterCommand>();
    params.setFlags(
      (vm.count("no-inherit") > 0 ? 0 : nfd::ROUTE_FLAG_CHILD_INHERIT) |
      (vm.count("capture") > 0 ? nfd::ROUTE_FLAG_CAPTURE : 0)
    );
  }

  KeyChain keyChain;
  InterestSigner cis(keyChain);
  Interest interest = cis.makeCommandInterest(command->getRequestName(commandPrefix, params), si);

  Block wire = interest.wireEncode();
  std::cout.write(reinterpret_cast<const char*>(wire.wire()), wire.size());
  return 0;
}

} // namespace register_prefix_cmd
} // namespace ndn6

int
main(int argc, char** argv)
{
  return ndn6::register_prefix_cmd::main(argc, argv);
}
