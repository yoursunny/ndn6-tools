#include "tap-tunnel_producer.hpp"
#include <cstdlib>
#include <ndn-cxx/util/logger.hpp>

namespace ndn {
namespace tap_tunnel {

NDN_LOG_INIT(taptunnel.Producer);

Producer::Producer(const ProducerOptions& options, PayloadQueue& payloads, Face& face, KeyChain& keyChain)
  : m_options(options)
  , m_face(face)
  , m_keyChain(keyChain)
  , m_sched(m_face.getIoService())
  , m_tickEvt(m_sched)
  , m_payloads(payloads)
{
  face.setInterestFilter(m_options.localPrefix,
    bind(&Producer::processInterest, this, _2),
    bind([] {
      NDN_LOG_FATAL("prefix-registration-failure");
      std::exit(3);
    }));

  this->tick();
}

void
Producer::processInterest(const Interest& interest)
{
  if (interest.getName().size() != m_options.localPrefix.size() + 1 ||
      !interest.getName()[-1].isSequenceNumber()) {
    return;
  }

  uint64_t seq = interest.getName()[-1].toSequenceNumber();
  auto deadline = time::steady_clock::now() + interest.getInterestLifetime() -
                  m_options.answerDeadlineReduction;
  m_requests.push({seq, deadline}); // XXX should sort by deadline
  NDN_LOG_TRACE("add-request seq=" << seq << " payloads=" << m_payloads.size() <<
                " requests=" << m_requests.size());

  if (!interest.getExclude().empty()) {
    auto excluded = interest.getExclude().begin();
    if (excluded->isSingular()) {
      NDN_LOG_TRACE("received-piggyback seq=" << seq);
      this->afterReceive(excluded->from);
    }
  }

  this->reply();
}

void
Producer::reply()
{
  while (!m_payloads.empty() && !m_requests.empty()) {
    uint64_t seq = m_requests.front().seq;
    NDN_LOG_TRACE("reply-payload seq=" << seq <<
                  " payloads=" << (m_payloads.size() - 1) <<
                  " requests=" << (m_requests.size() - 1));
    this->sendData(seq, m_payloads.dequeue());
    m_requests.pop();
  }

  auto now = time::steady_clock::now();
  while (!m_requests.empty() && m_requests.front().deadline <= now) {
    uint64_t seq = m_requests.front().seq;
    NDN_LOG_TRACE("reply-empty seq=" << seq << " payloads=0" <<
                  " requests=" << (m_requests.size() - 1));
    this->sendData(seq, Block(tlv::Content));
    m_requests.pop();
  }
}

void
Producer::sendData(uint64_t seq, const Block& payload)
{
  Data data(Name(m_options.localPrefix).appendSequenceNumber(seq));
  data.setContent(payload);
  m_keyChain.sign(data, m_options.signer);
  m_face.put(data);
}

void
Producer::tick()
{
  this->reply();

  m_tickEvt = m_sched.scheduleEvent(m_options.tickInterval, bind(&Producer::tick, this));
}

} // namespace tap_tunnel
} // namespace ndn
