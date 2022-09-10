#ifndef PAM_PCBIOUNLOCK_BASEUNLOCKSERVER_H
#define PAM_PCBIOUNLOCK_BASEUNLOCKSERVER_H

#include <string>
#include <thread>
#include <utility>

#include "../deps/json.hpp"
#include "Logger.h"
#include "UnlockState.h"
#include "../utils/Utils.h"
#include "../utils/CryptUtils.h"

class BaseUnlockServer {
public:
    BaseUnlockServer(std::string encryptionPwd, std::string unlockToken) {
        m_EncryptionPwd = std::move(encryptionPwd);
        m_UnlockToken = std::move(unlockToken);
        m_UnlockState = UnlockState::UNKNOWN;
    }

    virtual bool Start() = 0;
    virtual void Stop() = 0;

    void SetUnlockInfo(const std::string& authUser, const std::string& authProgram) {
        m_AuthUser = authUser;
        m_AuthProgram = authProgram;
    }

    bool HasClient() const {
        return m_HasConnection;
    }
    UnlockResponseData GetResponseData() {
        return m_ResponseData;
    }

    UnlockState PollResult() {
        return m_UnlockState;
    }
    UnlockState GetResult() {
        while (true) {
            if(m_UnlockState != UnlockState::UNKNOWN)
                return m_UnlockState;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

protected:
    void OnDataReceived(uint8_t *buffer, size_t buffer_size) {
        size_t decLen;
        auto decData = CryptUtils::DecryptAESPacket(buffer, buffer_size, &decLen, m_EncryptionPwd);
        if(decData == nullptr) {
            Logger::writeln("Invalid data received !");
            m_UnlockState = UnlockState::CANCELED;
            return;
        }

        auto dataStr = std::string((char *)decData, decLen);
        SAFE_FREE(decData);

        // Parse data
        try {
            auto json = nlohmann::json::parse(dataStr);
            auto response = UnlockResponseData();
            response.unlockToken = json["unlockToken"];
            response.additionalData = json["password"];

            m_ResponseData = response;
            if(response.unlockToken == m_UnlockToken) {
                m_UnlockState = UnlockState::SUCCESS;
            } else {
                m_UnlockState = UnlockState::CANCELED;
            }
        } catch(std::exception&) {
            Logger::writeln("Error parsing response data !");
            m_UnlockState = UnlockState::CANCELED;
        }
    }

    bool m_IsRunning{};
    std::thread m_AcceptThread;

    bool m_HasConnection{};

    UnlockState m_UnlockState;
    UnlockResponseData m_ResponseData;

    std::string m_AuthUser;
    std::string m_AuthProgram;

    std::string m_EncryptionPwd;
    std::string m_UnlockToken;
};


#endif //PAM_PCBIOUNLOCK_BASEUNLOCKSERVER_H
