#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/logging.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "tap-tunnel_consumer.hpp"
#include "tap-tunnel_tun.hpp"
#include "tap-tunnel_producer.hpp"

namespace ndn {
namespace tap_tunnel {

NDN_LOG_INIT(taptunnel.Main);

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
  ConsumerOptions consumerOptions;
  ProducerOptions producerOptions;
  std::string ifname;

  po::options_description options("Options");
  options.add_options()
    ("help,h", "print help message")
    ("local-prefix,l", po::value<Name>(&producerOptions.localPrefix)->required(), "local prefix")
    ("remote-prefix,r", po::value<Name>(&consumerOptions.remotePrefix)->required(), "remote prefix")
    ("ifname,i", po::value<std::string>(&ifname)->required(), "TAP interface name")
    ("outstandings", po::value<int>(&consumerOptions.maxOutstanding), "max outstanding Interests")
    ("payloads", po::value<size_t>(&producerOptions.maxPayloads), "payload queue size")
    ;
  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, options), vm);
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

  Tun tun;
  tun.enableAsync(face.getIoService());
  tun.open(ifname);

  Consumer consumer(consumerOptions, face);
  Producer producer(producerOptions, face, keyChain);

  tun.afterReceive.connect(
    [&] (const Buffer& packet) {
      NDN_LOG_TRACE("send " << packet.size());
      producer.enqueue(makeBinaryBlock(tlv::Content, packet.get(), packet.size()));
    });
  tun.startReceive();

  consumer.afterReceive.connect(
    [&] (const Block& payload) {
      NDN_LOG_TRACE("recv " << payload.value_size());
      tun.send(payload.value(), payload.value_size());
    });
  consumer.start();

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
