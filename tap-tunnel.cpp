#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/notification-stream.hpp>
#include <ndn-cxx/util/notification-subscriber.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/logging.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "tap-tunnel_tun.hpp"

namespace ndn {
namespace tap_tunnel {

NDN_LOG_INIT(taptunnel.Main);

typedef name::Component TunPacket;

namespace po = boost::program_options;

static void
usage(std::ostream& os, const po::options_description& options)
{
  os << "Usage: tap-tunnel -i tap0 -p passw0rd -l ndn:/prefix1 -r ndn:/prefix2\n"
        "\n"
        "Establish a TAP/TUN tunnel over NDN.\n"
        "\n"
     << options;
}

int
main(int argc, char** argv)
{
  Name localPrefix;
  Name remotePrefix;
  std::string ifname;
  std::string password;

  po::options_description options("Required options");
  options.add_options()
    ("help,h", "print help message")
    ("local-prefix,l", po::value<Name>(&localPrefix)->required(), "local prefix")
    ("remote-prefix,r", po::value<Name>(&remotePrefix)->required(), "remote prefix")
    ("ifname,i", po::value<std::string>(&ifname)->required(), "TAP interface name")
    ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  try {
    po::notify(vm);
  }
  catch (boost::program_options::error&) {
    usage(std::cerr, options);
    return 2;
  }
  if (vm.count("help") > 0) {
    usage(std::cout, options);
    return 0;
  }

  util::Logging::setLevel("*", util::LogLevel::TRACE);

  Face face;
  KeyChain keyChain;
  util::NotificationStream<TunPacket> sender(face, localPrefix, keyChain);
  util::NotificationSubscriber<TunPacket> receiver(face, remotePrefix, time::milliseconds(5000));

  Tun tun;
  tun.enableAsync(face.getIoService());
  tun.open(ifname);

  tun.afterReceive.connect(
    [&] (const Buffer& packet) {
      NDN_LOG_TRACE("send " << packet.size());
      TunPacket tp(packet);
      sender.postNotification(tp);
    });
  tun.startReceive();

  receiver.onNotification.connect(
    [&] (const TunPacket& tp) {
      NDN_LOG_TRACE("recv " << tp.value_size());
      tun.send(tp.value(), tp.value_size());
    });
  receiver.start();

  face.registerPrefix(localPrefix, nullptr,
    bind([] { NDN_LOG_FATAL("prefix-registration-failure"); }));

  face.processEvents();

  return 0;
}

} // namespace tap_tunnel
} // namespace ndn

int
main(int argc, char** argv)
{
  return ndn::tap_tunnel::main(argc, argv);
}
