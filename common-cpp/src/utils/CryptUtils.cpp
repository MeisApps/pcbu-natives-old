#include "CryptUtils.h"

#include <cstring>

#include <openssl/evp.h>
#include <openssl/rand.h>

#define AES_KEY_SIZE 256
#define IV_SIZE 16
#define SALT_SIZE 16
#define ITERATIONS 65535

PacketCryptResult CryptUtils::EncryptAESPacket(const uint8_t* data, size_t dataLen, size_t *encLen, const std::string& pwd, uint8_t** encData) {
    if(encLen == nullptr || encData == nullptr)
        return PacketCryptResult::OTHER_ERROR;
    *encLen = 0;

    auto dataBuf = (uint8_t *)malloc(dataLen + 8);
    if(dataBuf == nullptr)
        return PacketCryptResult::OTHER_ERROR;

    auto timeMs = Utils::GetCurrentTimeMillis();
    memcpy(dataBuf, &timeMs, sizeof(int64_t));
    ReverseBytes(dataBuf, sizeof(int64_t));

    memcpy(&dataBuf[8], data, dataLen);

    uint8_t encBuffer[1024] = {};
    *encLen = EncryptAES(dataBuf, dataLen + 8,encBuffer, sizeof(encBuffer), pwd.c_str());
    SAFE_FREE(dataBuf);
    if(*encLen <= 8)
        return PacketCryptResult::OTHER_ERROR;

    *encData = (uint8_t *)malloc(*encLen);
    if(*encData == nullptr)
        return PacketCryptResult::OTHER_ERROR;

    memcpy(*encData, encBuffer, *encLen);
    return PacketCryptResult::OK;
}

PacketCryptResult CryptUtils::DecryptAESPacket(const uint8_t* src, size_t srcLen, size_t *decLen, const std::string& pwd, uint8_t** decData) {
    if(decLen == nullptr || decData == nullptr)
        return PacketCryptResult::OTHER_ERROR;
    *decLen = 0;

    uint8_t decBuffer[1024] = {};
    *decLen = DecryptAES(src, srcLen, decBuffer, sizeof(decBuffer), pwd.c_str());
    if(*decLen <= 8)
        return PacketCryptResult::OTHER_ERROR;
    *decLen = *decLen - 8;

    int64_t timestamp;
    memcpy(&timestamp, decBuffer, sizeof(int64_t));
    ReverseBytes(&timestamp, sizeof(int64_t));

    auto timeDiff = Utils::GetCurrentTimeMillis() - timestamp;
    if(timeDiff < -PACKET_TIMEOUT || timeDiff > PACKET_TIMEOUT)
        return PacketCryptResult::INVALID_TIMESTAMP;

    *decData = (uint8_t *)malloc(*decLen);
    if(*decData == nullptr)
        return PacketCryptResult::OTHER_ERROR;

    memcpy(*decData, &decBuffer[8], *decLen);
    return PacketCryptResult::OK;
}

size_t CryptUtils::EncryptAES(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstLen, const char* pwd) {
    int status;
    if (!src || !dst || dstLen < srcLen + SALT_SIZE + IV_SIZE)
        return false;

    unsigned char iv[IV_SIZE];
    if (RAND_bytes(iv, IV_SIZE) != 1)
        return false;
    memcpy(dst, iv, IV_SIZE);

    unsigned char salt[SALT_SIZE];
    if (RAND_bytes(salt, SALT_SIZE) != 1)
        return false;
    memcpy(dst + IV_SIZE, salt, SALT_SIZE);

    unsigned char* key = GenerateKey(pwd, salt);
    if (!key)
        return false;

    int numberOfBytes = 0;
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if(!ctx) {
        free(key);
        return false;
    }

    status = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    if (!status) {
        free(key);
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    status = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr);
    if (!status) {
        free(key);
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    status = EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, iv);
    free(key);
    if (!status) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    status = EVP_EncryptUpdate(ctx, dst + IV_SIZE + SALT_SIZE, &numberOfBytes, src, (int)srcLen);
    if (!status) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int encryptedLen = numberOfBytes;
    status = EVP_EncryptFinal_ex(ctx, dst + IV_SIZE + SALT_SIZE + encryptedLen, &numberOfBytes);
    if (!status) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, dst + IV_SIZE + SALT_SIZE + encryptedLen);
    EVP_CIPHER_CTX_free(ctx);
    return IV_SIZE + SALT_SIZE + encryptedLen + 16;
}

size_t CryptUtils::DecryptAES(const uint8_t* src, size_t srcLen, uint8_t* dst, size_t dstLen, const char* pwd) {
    int status;
    if (!src || !dst || srcLen < IV_SIZE + SALT_SIZE || dstLen < srcLen - IV_SIZE - SALT_SIZE)
        return false;

    unsigned char iv[IV_SIZE];
    memcpy(iv, src, IV_SIZE);

    unsigned char salt[SALT_SIZE];
    memcpy(salt, src + IV_SIZE, SALT_SIZE);

    unsigned char* key = GenerateKey(pwd, salt);
    if (!key)
        return false;

    int numberOfBytes = 0;
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if(!ctx) {
        free(key);
        return false;
    }

    status = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    if (!status) {
        free(key);
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    status = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr);
    if (!status) {
        free(key);
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    status = EVP_DecryptInit_ex(ctx, nullptr, nullptr, key, iv);
    free(key);
    if (!status) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    status = EVP_DecryptUpdate(ctx, dst, &numberOfBytes, src + IV_SIZE + SALT_SIZE, (int)srcLen - IV_SIZE - SALT_SIZE - 16);
    if (!status) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int decryptedLen = numberOfBytes;
    status = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, (void*)(src + srcLen - 16));
    if (!status) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    status = EVP_DecryptFinal_ex(ctx, dst + decryptedLen, &numberOfBytes);
    EVP_CIPHER_CTX_free(ctx);
    if (!status)
        return false;
    return decryptedLen;
}

uint8_t* CryptUtils::GenerateKey(const char* pwd, unsigned char* salt) {
    auto key = (unsigned char*)malloc(AES_KEY_SIZE / 8);
    if (!key)
        return nullptr;
    if (PKCS5_PBKDF2_HMAC(pwd, (int)strlen(pwd), salt, SALT_SIZE, ITERATIONS, EVP_sha256(), AES_KEY_SIZE / 8, key) != 1) {
        free(key);
        return nullptr;
    }
    return key;
}

void CryptUtils::ReverseBytes(void *dataPtr, size_t n) {
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
