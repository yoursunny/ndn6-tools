#include "tap-tunnel_tun.hpp"
#include <boost/bind.hpp>
#include <sys/ioctl.h>
#include <fcntl.h>

namespace ndn {
namespace tap_tunnel {

Tun::Tun()
  : m_fd(-1)
{
}

Tun::~Tun()
{
  if (m_sd != nullptr) {
    boost::system::error_code error;
    m_sd->cancel(error);
    m_sd->close(error);
  }
  if (m_fd >= 0) {
    ::close(m_fd);
  }
}

void
Tun::enableAsync(boost::asio::io_service& io)
{
  m_sd.reset(new boost::asio::posix::stream_descriptor(io));
}

void
Tun::open(const std::string& ifname, Mode mode)
{
  ifreq ifr = {0};
  m_fd = ::open("/dev/net/tun", O_RDWR);
  if (m_fd < 0) {
    throw std::runtime_error("open error");
  }

  ifr.ifr_flags = static_cast<int>(mode) | IFF_NO_PI;
  std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ);

  int err = ::ioctl(m_fd, TUNSETIFF, reinterpret_cast<void*>(&ifr));
  if (err < 0) {
    ::close(m_fd);
    throw std::runtime_error("TUNSETIFF error");
  }

  m_ifname.assign(ifr.ifr_name);

  if (m_sd != nullptr) {
    m_sd->assign(m_fd);
  }
}

void
Tun::send(const uint8_t* pkt, const size_t size)
{
  if (size == 0) {
    throw std::range_error("cannot send empty packet");
  }

  int nBytes = ::write(m_fd, pkt, size);
  if (nBytes < 0) {
    throw std::runtime_error("send error");
  }
}

void
Tun::send(const Buffer& packet)
{
  this->send(packet.get(), packet.size());
}

ConstBufferPtr
Tun::receive()
{
  const size_t FRAMESIZE = 1514;
  BufferPtr packet = make_shared<Buffer>(FRAMESIZE);

  int nBytes = ::read(m_fd, packet->get(), packet->size());
  if (nBytes < 0) {
    throw std::runtime_error("receive error");
  }

  packet->resize(nBytes);
  return packet;
}

void
Tun::asyncReceive(const ReceiveCallback& callback)
{
  if (m_sd == nullptr) {
    throw std::logic_error("async is not enabled");
  }

  m_sd->async_read_some(boost::asio::null_buffers(),
                        boost::bind( // boost::asio::placeholders don't like std::bind
                          &Tun::asyncRead, this, callback,
                          boost::asio::placeholders::error,
                          boost::asio::placeholders::bytes_transferred));
}

void
Tun::asyncRead(const ReceiveCallback& callback, const boost::system::error_code& error, size_t)
{
  if (error) {
    if (error == boost::asio::error::operation_aborted) {
      return;
    }
    throw std::runtime_error("async receive failed: " + error.message());
  }

  callback(this->receive());
}

void
Tun::startReceive()
{
  this->asyncReceive(
    [this] (const ConstBufferPtr& packet) {
      this->afterReceive(packet);
      this->startReceive();
    });
}

} // namespace tap_tunnel
} // namespace ndn
