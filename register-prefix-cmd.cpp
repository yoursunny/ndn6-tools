#include "common.hpp"

#include <ndn-cxx/util/time-unit-test-clock.hpp>

namespace ndn6 {
namespace register_prefix_cmd {

int
main(int argc, char** argv)
{
  Name prefix;
  Name commandPrefix("/localhop/nfd");
  int faceId = -1;
  int origin = 0;
  SigningInfo si;
  int advanceClock = 0;

  auto args = parseProgramOptions(
    argc, argv,
    "Usage: register-prefix-cmd [-u] -r /laptop-prefix -f 256 -i /identity\n"
    "\n"
    "Prepare a prefix (un)registration command.\n"
    "\n",
    [&](auto addOption) {
      addOption("unregister,u", "unregister");
      addOption("prefix,p", po::value<Name>(&prefix)->required(), "prefix");
      addOption("command,P", po::value<Name>(&commandPrefix), "command prefix");
      addOption("face,f", po::value<int>(&faceId), "FaceId, default is self");
      addOption("origin,o", po::value<int>(&origin), "origin");
      addOption("no-inherit,I", "unset ChildInherit flag");
      addOption("capture,C", "set Capture flag");
      addOption("identity,i", po::value<Name>(), "signing identity");
      addOption("advance-clock", po::value<int>(&advanceClock), "advance clock (millis)");
    });

  if (args.count("identity") > 0) {
    si = signingByIdentity(args["identity"].as<Name>());
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
  if (args.count("unregister") > 0) {
    command = std::make_unique<nfd::RibUnregisterCommand>();
  } else {
    command = std::make_unique<nfd::RibRegisterCommand>();
    params.setFlags((args.count("no-inherit") > 0 ? 0 : nfd::ROUTE_FLAG_CHILD_INHERIT) |
                    (args.count("capture") > 0 ? nfd::ROUTE_FLAG_CAPTURE : 0));
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
