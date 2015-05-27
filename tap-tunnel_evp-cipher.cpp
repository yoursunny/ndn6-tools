#include "tap-tunnel_evp-cipher.hpp"

namespace ndn {
namespace tap_tunnel {

EvpCipher::~EvpCipher()
{
  EVP_CIPHER_CTX_cleanup(&m_encrypt);
  EVP_CIPHER_CTX_cleanup(&m_decrypt);
}

void
EvpCipher::initialize(const std::string& password,
                      const uint64_t salt, const int nRounds,
                      const EVP_CIPHER* cipher, const EVP_MD* md)
{
  const size_t keyLength = EVP_CIPHER_key_length(cipher);
  const size_t ivLength = EVP_CIPHER_iv_length(cipher);

  uint8_t key[keyLength], iv[ivLength];
  int keySize = EVP_BytesToKey(cipher, md, reinterpret_cast<const uint8_t*>(&salt),
                               reinterpret_cast<const uint8_t*>(password.data()), password.size(),
                               nRounds, key, iv);
  if (keySize != static_cast<int>(keyLength)) {
    throw std::runtime_error("EVP_BytesToKey failed");
  }

  EVP_CIPHER_CTX_init(&m_encrypt);
  EVP_EncryptInit_ex(&m_encrypt, cipher, nullptr, key, iv);
  EVP_CIPHER_CTX_init(&m_decrypt);
  EVP_DecryptInit_ex(&m_decrypt, cipher, nullptr, key, iv);
}

Buffer
EvpCipher::encrypt(const Buffer& plaintext)
{
  int ciphertextSize = plaintext.size() + AES_BLOCK_SIZE;
  Buffer ciphertext(ciphertextSize);

  EVP_EncryptInit_ex(&m_encrypt, nullptr, nullptr, nullptr, nullptr);
  EVP_EncryptUpdate(&m_encrypt, ciphertext.get(), &ciphertextSize,
                    plaintext.get(), plaintext.size());
  int finalSize = 0;
  EVP_EncryptFinal_ex(&m_encrypt, ciphertext.get() + ciphertextSize, &finalSize);

  ciphertextSize += finalSize;
  ciphertext.resize(ciphertextSize);
  return ciphertext;
}

Buffer
EvpCipher::decrypt(const Buffer& ciphertext)
{
  int plaintextSize = ciphertext.size();
  Buffer plaintext(plaintextSize);

  EVP_DecryptInit_ex(&m_decrypt, nullptr, nullptr, nullptr, nullptr);
  EVP_DecryptUpdate(&m_decrypt, plaintext.get(), &plaintextSize,
                    ciphertext.get(), ciphertext.size());
  int finalSize = 0;
  EVP_DecryptFinal_ex(&m_decrypt, plaintext.get() + plaintextSize, &finalSize);

  plaintextSize += finalSize;
  plaintext.resize(plaintextSize);
  return plaintext;
}

} // namespace tap_tunnel
} // namespace ndn
