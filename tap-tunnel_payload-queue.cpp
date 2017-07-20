#include "tap-tunnel_payload-queue.hpp"

namespace ndn {
namespace tap_tunnel {

PayloadQueue::PayloadQueue(const PayloadQueueOptions& options)
  : m_options(options)
{
}

bool
PayloadQueue::enqueue(ConstBufferPtr payload)
{
  if (size() >= m_options.capacity) {
    return false;
  }

  m_payloads.push(payload);
  return true;
}

bool
PayloadQueue::isSmall() const
{
  BOOST_ASSERT(size() > 0);
  return m_payloads.front()->size() <= m_options.smallThreshold;
}

Block
PayloadQueue::dequeue(uint32_t tlvType)
{
  Block block(tlvType, m_payloads.front());
  m_payloads.pop();
  return block;
}

} // namespace tap_tunnel
} // namespace ndn
