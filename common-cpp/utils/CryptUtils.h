#ifndef PAM_PCBIOUNLOCK_CRYPTUTILS_H
#define PAM_PCBIOUNLOCK_CRYPTUTILS_H

#include "openssl/evp.h"
#include "openssl/rand.h"

#include <string>
#include <cstring>
#include <random>
#include <algorithm>

#define SHA1_LEN 20
#ifdef _WIN32
#define PACKET_TIMEOUT (60000 * 2)
#else
#define PACKET_TIMEOUT 30000
#endif

class CryptUtils {
public:
    static uint8_t *EncryptAESPacket(const uint8_t* data, size_t data_len, size_t *enc_len, const std::string& pwd) {
        if(enc_len == nullptr)
            return nullptr;
        *enc_len = 0;

        auto dataBuf = (uint8_t *)malloc(data_len + 8);
        if(dataBuf == nullptr)
            return nullptr;

        auto timeMs = Utils::GetCurrentTimeMillis();
        memcpy(dataBuf, &timeMs, sizeof(int64_t));
        ReverseBytes(dataBuf, sizeof(int64_t));

        memcpy(&dataBuf[8], data, data_len);

        uint8_t encBuffer[1024] = {};
        auto encLen = EncryptAES(dataBuf, data_len + 8,encBuffer, sizeof(encBuffer), pwd.c_str());
        SAFE_FREE(dataBuf);
        if(encLen <= 8)
            return nullptr;

        auto encData = (uint8_t *)malloc(encLen);
        if(encData == nullptr)
            return nullptr;

        memcpy(encData, encBuffer, encLen);
        *enc_len = encLen;
        return encData;
    }

    static uint8_t *DecryptAESPacket(const uint8_t* src, size_t src_len, size_t *dec_len, const std::string& pwd) {
        if(dec_len == nullptr)
            return nullptr;
        *dec_len = 0;

        uint8_t decBuffer[1024] = {};
        auto decLen = DecryptAES(src, src_len, decBuffer, sizeof(decBuffer), pwd.c_str());
        if(decLen <= 8)
            return nullptr;

        int64_t timestamp;
        memcpy(&timestamp, decBuffer, sizeof(int64_t));
        ReverseBytes(&timestamp, sizeof(int64_t));

        auto timeDiff = Utils::GetCurrentTimeMillis() - timestamp;
        if(timeDiff < -PACKET_TIMEOUT || timeDiff > PACKET_TIMEOUT) {
            Logger::writeln("Invalid timestamp on AES data !");
            return nullptr;
        }

        auto decData = (uint8_t *)malloc(decLen - 8);
        if(decData == nullptr)
            return nullptr;

        memcpy(decData, &decBuffer[8], decLen - 8);
        *dec_len = decLen - 8;
        return decData;
    }

private:
    static size_t EncryptAES(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_len, const char *pwd) {
        int status;
        if (!src || !dst || dst_len < src_len + 64)
            return false;

        // Setup IV
        int iv_len = 12;
        auto iv = (unsigned char *)malloc(iv_len);
        if (!iv)
            return false;
        RAND_bytes(iv, iv_len);

        auto data_offset = 4 + iv_len;
        memcpy(dst, &iv_len, 4);
        memcpy(dst + 4, iv, iv_len);
        ReverseBytes(dst, 4);

        // Setup key
        auto key = GenerateKey(pwd, iv, iv_len);
        if (!key) {
            SAFE_FREE(iv);
            return false;
        }

        // Setup decrypt
        int numberOfBytes = 0;
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

        // Set the key and iv
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, nullptr);
        status = EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, key, iv);

        SAFE_FREE(key);
        SAFE_FREE(iv);
        if(!status) {
            Logger::writeln("Encrypt init failed.");
            return false;
        }

        // Encrypt
        status = EVP_EncryptUpdate(ctx, dst + data_offset, &numberOfBytes, src, (int)src_len);
        if (!status) {
            Logger::writeln("Encrypt failed.");
            return false;
        }

        auto encLen = numberOfBytes + data_offset;
        EVP_EncryptFinal_ex(ctx, dst + encLen, &numberOfBytes);
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, dst + encLen);

        encLen += 16;
        dst[encLen] = '\0'; // encrypted length

        EVP_CIPHER_CTX_free(ctx);
        return encLen;
    }

    static size_t DecryptAES(const uint8_t* src, size_t src_len, uint8_t* dst, size_t dst_len, const char *pwd) {
        int status;
        if (!src || !dst || src_len < 24 || dst_len < src_len - 64)
            return false;

        // Get iv size
        int iv_len;
        memcpy(&iv_len, src, 4);
        ReverseBytes(&iv_len, 4);

        //printf("IV Size: %i\n", iv_len);
        if(iv_len <= 0 || iv_len > 20)
            return false;

        // Get iv data
        auto iv = (unsigned char *)malloc(iv_len);
        if (!iv)
            return false;
        auto data_offset = 4 + iv_len;
        memcpy(iv, src + 4, iv_len);

        src_len -= data_offset;

        // Setup key
        auto key = GenerateKey(pwd, iv, iv_len);
        if (!key) {
            SAFE_FREE(iv);
            return false;
        }

        // Setup decrypt
        int numberOfBytes = 0;
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();

        // Set the key and iv
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, nullptr);
        status = EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, key, iv);

        SAFE_FREE(key);
        SAFE_FREE(iv);
        if(!status) {
            Logger::writeln("Decrypt init failed.");
            return false;
        }

        // Decrypt
        status = EVP_DecryptUpdate(ctx, dst, &numberOfBytes, src + data_offset, (int)src_len);
        if (!status) {
            Logger::writeln("Decrypt failed.");
            return false;
        }

        auto decLen = numberOfBytes - data_offset;
        dst[decLen] = '\0'; // decrypted length

        EVP_DecryptFinal_ex(ctx, nullptr, &numberOfBytes);
        EVP_CIPHER_CTX_free(ctx);
        return decLen;
    }

private:
    static uint8_t *GenerateKey(const char *pwd, const uint8_t *iv, size_t iv_len) {
        // this will be the result of PBKDF2-HMAC-SHA1
        auto out = (uint8_t *)malloc(SHA1_LEN * sizeof(uint8_t));
        PKCS5_PBKDF2_HMAC_SHA1(pwd, (int)strlen(pwd), iv, (int)iv_len, 65536, SHA1_LEN, out);
        return out;
    }

    static void ReverseBytes(void *dataPtr, size_t n) {
        if(!Utils::IsLittleEndian())
            return;

        size_t i;
        auto data = (uint8_t*)dataPtr;
        for (i=0; i < n/2; ++i) {
            auto tmp = data[i];
            data[i] = data[n - 1 - i];
            data[n - 1 - i] = tmp;
        }
    }

    CryptUtils() = default;
};

#endif //PAM_PCBIOUNLOCK_CRYPTUTILS_H
