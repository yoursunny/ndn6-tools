#ifndef TAP_TUNNEL_CONSUMER_HPP
#define TAP_TUNNEL_CONSUMER_HPP

#include "tap-tunnel_payload-queue.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/signal.hpp>

namespace ndn {
namespace tap_tunnel {

struct ConsumerOptions
{
  Name remotePrefix;
  int maxOutstanding = 4; ///< number of outstanding Interests
  time::milliseconds interestLifetime = time::seconds(2);
};

class Consumer : noncopyable
{
public:
  Consumer(const ConsumerOptions& options, PayloadQueue& payloads, Face& face);

  void
  start();

  util::signal::Signal<Consumer, Block> afterReceive;

private:
  void
  addInterest();

  void
  next();

private:
  const ConsumerOptions m_options;
  Face& m_face;
  PayloadQueue& m_payloads;

  int m_nOutstanding; ///< number of outstanding Interests
  uint64_t m_seq; ///< next sequence number
};

} // namespace tap_tunnel
} // namespace ndn

#endif // TAP_TUNNEL_CONSUMER_HPP
