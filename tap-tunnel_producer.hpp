#ifndef TAP_TUNNEL_PRODUCER_HPP
#define TAP_TUNNEL_PRODUCER_HPP

#include "tap-tunnel_payload-queue.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-info.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/scheduler-scoped-event-id.hpp>
#include <ndn-cxx/util/signal.hpp>

namespace ndn {
namespace tap_tunnel {

struct ProducerOptions
{
  Name localPrefix;
  security::SigningInfo signer = security::signingWithSha256();
  time::milliseconds answerDeadlineReduction = time::seconds(1); ///< Data must be sent within (InterestLifetime - reduction)
  time::nanoseconds tickInterval = time::milliseconds(10);
};

class Producer : noncopyable
{
public:
  Producer(const ProducerOptions& options, PayloadQueue& payloads, Face& face, KeyChain& keyChain);

  util::signal::Signal<Producer, Block> afterReceive;

private:
  void
  processInterest(const Interest& interest);

  void
  reply();

  void
  sendData(uint64_t seq, const Block& payload);

  void
  tick();

private:
  struct PendingInterest
  {
    uint64_t seq;
    time::steady_clock::TimePoint deadline;
  };

  const ProducerOptions m_options;
  Face& m_face;
  KeyChain& m_keyChain;
  util::scheduler::Scheduler m_sched;
  util::scheduler::ScopedEventId m_tickEvt;

  PayloadQueue& m_payloads; ///< payloads to send
  std::queue<PendingInterest> m_requests; ///< unexpired pending Interests
};

} // namespace tap_tunnel
} // namespace ndn

#endif // TAP_TUNNEL_PRODUCER_HPP
