#ifndef TAP_TUNNEL_TUN_HPP
#define TAP_TUNNEL_TUN_HPP

#include <ndn-cxx/encoding/buffer.hpp>
#include <ndn-cxx/util/signal.hpp>
#include <boost/asio.hpp>
#include <linux/if_tun.h>

namespace ndn {
namespace tap_tunnel {

/** \brief TUN/TAP device
 */
class Tun : noncopyable
{
public:
  Tun();

  ~Tun();

  void
  enableAsync(boost::asio::io_service& io);

  enum Mode {
    TUN = IFF_TUN,
    TAP = IFF_TAP
  };

  void
  open(const std::string& ifname, Mode mode = TAP);

  int
  getFd() const
  {
    return m_fd;
  }

  const std::string&
  getIfname() const
  {
    return m_ifname;
  }

  void
  send(const uint8_t* pkt, const size_t size);

  void
  send(const Buffer& packet);

public:
  /** \brief synchronous receive
   */
  ConstBufferPtr
  receive();

  using ReceiveCallback = std::function<void(ConstBufferPtr)>;

  /** \brief asynchronous single receive
   */
  void
  asyncReceive(const ReceiveCallback& callback);

  ndn::util::signal::Signal<Tun, ConstBufferPtr> afterReceive;

  /** \brief asynchronous multiple receive using signal
   */
  void
  startReceive();

private:
  void
  asyncRead(const ReceiveCallback& callback, const boost::system::error_code& error, size_t);

private:
  int m_fd;
  std::string m_ifname;
  unique_ptr<boost::asio::posix::stream_descriptor> m_sd;
};

} // namespace tap_tunnel
} // namespace ndn

#endif // TAP_TUNNEL_TUN_HPP
