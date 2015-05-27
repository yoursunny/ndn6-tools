#ifndef TAP_TUNNEL_EVP_CIPHER_HPP
#define TAP_TUNNEL_EVP_CIPHER_HPP

#include <ndn-cxx/encoding/buffer.hpp>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/sha.h>

namespace ndn {
namespace tap_tunnel {

/** \brief symmetric cipher using OpenSSL EVP
 */
class EvpCipher : noncopyable
{
public:
  ~EvpCipher();

  void
  initialize(const std::string& password,
             const uint64_t salt = 0, const int nRounds = 5,
             const EVP_CIPHER* cipher = EVP_aes_256_cbc(),
             const EVP_MD* md = EVP_sha256());

  Buffer
  encrypt(const Buffer& plaintext);

  Buffer
  decrypt(const Buffer& ciphertext);

private:
  EVP_CIPHER_CTX m_encrypt;
  EVP_CIPHER_CTX m_decrypt;
};

} // namespace tap_tunnel
} // namespace ndn

#endif // TAP_TUNNEL_EVP_CIPHER_HPP
