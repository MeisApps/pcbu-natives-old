#ifndef PAM_PCBIOUNLOCK_CRYPTUTILS_H
#define PAM_PCBIOUNLOCK_CRYPTUTILS_H

#include "Utils.h"

#ifdef _WIN32
#define PACKET_TIMEOUT (60000 * 2)
#else
#define PACKET_TIMEOUT 30000
#endif

enum PacketCryptResult {
    OK,
    INVALID_TIMESTAMP,
    OTHER_ERROR
};

class CryptUtils {
public:
    static PacketCryptResult EncryptAESPacket(const uint8_t* data, size_t dataLen, size_t *encLen, const std::string& pwd, uint8_t** encData);
    static PacketCryptResult DecryptAESPacket(const uint8_t* src, size_t srcLen, size_t *decLen, const std::string& pwd, uint8_t** decData);

private:
    static size_t EncryptAES(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstLen, const char* pwd);
    static size_t DecryptAES(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstLen, const char* pwd);

    static uint8_t* GenerateKey(const char* pwd, unsigned char* salt);
    static void ReverseBytes(void *dataPtr, size_t n);

    CryptUtils() = default;
};

#endif //PAM_PCBIOUNLOCK_CRYPTUTILS_H
