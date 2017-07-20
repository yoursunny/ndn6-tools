#ifndef TAP_TUNNEL_PAYLOAD_QUEUE_HPP
#define TAP_TUNNEL_PAYLOAD_QUEUE_HPP

#include <queue>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/buffer.hpp>
#include <ndn-cxx/encoding/tlv.hpp>

namespace ndn {
namespace tap_tunnel {

struct PayloadQueueOptions
{
  size_t capacity = 16;
  size_t smallThreshold = 14 + 40 + 20; // Ethernet+IPv6+TCP headers
};

class PayloadQueue : noncopyable
{
public:
  explicit
  PayloadQueue(const PayloadQueueOptions& options);

  bool
  empty() const
  {
    return m_payloads.empty();
  }

  size_t
  size() const
  {
    return m_payloads.size();
  }

  bool
  enqueue(ConstBufferPtr payload);

  bool
  isSmall() const;

  Block
  dequeue(uint32_t tlvType = tlv::Content);

private:
  PayloadQueueOptions m_options;
  std::queue<ConstBufferPtr> m_payloads;
};

} // namespace tap_tunnel
} // namespace ndn

#endif // TAP_TUNNEL_PAYLOAD_QUEUE_HPP
