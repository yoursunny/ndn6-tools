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
  PayloadQueueOptions payloadQueueOptions;
  ConsumerOptions consumerOptions;
  ProducerOptions producerOptions;
  std::string ifname;

  po::options_description options("Options");
  options.add_options()
    ("help,h", "print help message")
    ("local-prefix,l", po::value<Name>(&producerOptions.localPrefix)->required(), "local prefix")
    ("remote-prefix,r", po::value<Name>(&consumerOptions.remotePrefix)->required(), "remote prefix")
    ("ifname,i", po::value<std::string>(&ifname)->required(), "TAP interface name")
    ("payloads", po::value<size_t>(&payloadQueueOptions.capacity), "payload queue length")
    ("outstandings", po::value<int>(&consumerOptions.maxOutstanding), "max outstanding Interests")
    ("lifetime", po::value<uint16_t>(), "InterestLifetime (millis)")
    ("ansdlr", po::value<uint16_t>(), "answer deadline deduction (millis)")
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

  if (vm.count("lifetime") > 0) {
    consumerOptions.interestLifetime = time::milliseconds(vm["lifetime"].as<uint16_t>());
  }
  if (vm.count("ansdlr") > 0) {
    producerOptions.answerDeadlineReduction = time::milliseconds(vm["ansdlr"].as<uint16_t>());
  }

  Face face;
  KeyChain keyChain;

  Tun tun;
  tun.enableAsync(face.getIoService());
  tun.open(ifname);

  PayloadQueue payloads(payloadQueueOptions);
  Consumer consumer(consumerOptions, payloads, face);
  Producer producer(producerOptions, payloads, face, keyChain);

  tun.afterReceive.connect(
    [&] (ConstBufferPtr packet) {
      NDN_LOG_TRACE("send " << packet->size());
      payloads.enqueue(packet);
    });

  auto recvCallback = [&] (const Block& payload) {
    NDN_LOG_TRACE("recv " << payload.value_size());
    tun.send(payload.value(), payload.value_size());
  };
  consumer.afterReceive.connect(recvCallback);
  producer.afterReceive.connect(recvCallback);

  tun.startReceive();
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
