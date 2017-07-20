#include "tap-tunnel_consumer.hpp"
#include <ndn-cxx/util/logger.hpp>
#include <ndn-cxx/util/random.hpp>

namespace ndn {
namespace tap_tunnel {

NDN_LOG_INIT(taptunnel.Consumer);

Consumer::Consumer(const ConsumerOptions& options, PayloadQueue& payloads, Face& face)
  : m_options(options)
  , m_face(face)
  , m_payloads(payloads)
  , m_nOutstanding(0)
  , m_seq(random::generateWord64())
{
}

void
Consumer::start()
{
  this->next();
}

void
Consumer::addInterest()
{
  ++m_nOutstanding;

  Interest interest(Name(m_options.remotePrefix).appendSequenceNumber(m_seq));
  interest.setInterestLifetime(m_options.interestLifetime);
  interest.setMustBeFresh(true);

  if (!m_payloads.empty() && m_payloads.isSmall()) {
    NDN_LOG_TRACE("Interest-piggyback seq=" << m_seq << " outstanding=" << m_nOutstanding <<
                  " payloads=" << m_payloads.size());
    interest.setExclude(Exclude().excludeOne(m_payloads.dequeue(tlv::NameComponent)));
  }
  else {
    NDN_LOG_TRACE("Interest-plain seq=" << m_seq << " outstanding=" << m_nOutstanding);
  }

  m_face.expressInterest(interest,
    [this] (const Interest& interest, const Data& data) {
      --m_nOutstanding;
      NDN_LOG_TRACE("Data seq=" << interest.getName().at(-1).toSequenceNumber() <<
                    " outstanding=" << m_nOutstanding);
      if (data.getContent().value_size() > 0) {
        this->afterReceive(data.getContent());
      }
      this->next();
    },
    [this] (const Interest& interest, const lp::Nack& nack) {
      --m_nOutstanding;
      NDN_LOG_TRACE("Nack-" << nack.getReason() <<
                    " seq=" << interest.getName().at(-1).toSequenceNumber() <<
                    " outstanding=" << m_nOutstanding);
      this->next();
    },
    [this] (const Interest& interest) {
      --m_nOutstanding;
      NDN_LOG_TRACE("timeout seq=" << interest.getName().at(-1).toSequenceNumber() <<
                    " outstanding=" << m_nOutstanding);
      this->next();
    });

  ++m_seq;
}

void
Consumer::next()
{
  while (m_nOutstanding < m_options.maxOutstanding) {
    this->addInterest();
  }
}

} // namespace tap_tunnel
} // namespace ndn
