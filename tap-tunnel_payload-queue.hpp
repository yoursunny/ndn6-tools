#ifndef TAP_TUNNEL_PAYLOAD_QUEUE_HPP
#define TAP_TUNNEL_PAYLOAD_QUEUE_HPP

#include <queue>
#include <ndn-cxx/encoding/block.hpp>

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
  enqueue(Block&& payload);

  bool
  isSmall() const;

  Block
  dequeue();

private:
  PayloadQueueOptions m_options;
  std::queue<Block> m_payloads;
};

} // namespace tap_tunnel
} // namespace ndn

#endif // TAP_TUNNEL_PAYLOAD_QUEUE_HPP
