#include "BaseUnlockServer.h"

BaseUnlockServer::BaseUnlockServer(const PairedDevice& device) {
    m_PairedDevice = device;
    m_UnlockToken = Utils::GetRandomString(64);
    m_UnlockState = UnlockState::UNKNOWN;
}

BaseUnlockServer::~BaseUnlockServer() {
    if(m_AcceptThread.joinable())
        m_AcceptThread.join();
}

void BaseUnlockServer::SetUnlockInfo(const std::string& authUser, const std::string& authProgram) {
    m_AuthUser = authUser;
    m_AuthProgram = authProgram;
}

PairedDevice BaseUnlockServer::GetDevice() {
    return m_PairedDevice;
}

UnlockResponseData BaseUnlockServer::GetResponseData() {
    return m_ResponseData;
}

bool BaseUnlockServer::HasClient() const {
    return m_HasConnection;
}

UnlockState BaseUnlockServer::PollResult() {
    return m_UnlockState;
}

std::string BaseUnlockServer::GetUnlockInfoPacket() {
    // Unlock info
    nlohmann::json encServerData = {
            {"authUser", m_AuthUser},
            {"authProgram", m_AuthProgram},
            {"unlockToken", m_UnlockToken}
    };
    auto encServerDataStr = encServerData.dump();
    std::vector<uint8_t> myVector(encServerDataStr.begin(), encServerDataStr.end());
    uint8_t *serverDataPtr = &myVector[0];

    uint8_t *encData{};
    size_t encLen{};
    auto cryptResult = CryptUtils::EncryptAESPacket(serverDataPtr, encServerDataStr.size(), &encLen, m_PairedDevice.encryptionKey, &encData);
    if(cryptResult != PacketCryptResult::OK) {
        Logger::WriteLn("Encrypt error.");
        return {};
    }

    auto encStr = Utils::ToHexString(encData, encLen);
    SAFE_FREE(encData);
    nlohmann::json serverData = {
            {"pairingId", m_PairedDevice.pairingId},
            {"encData", encStr}
    };
    return serverData.dump();
}

void BaseUnlockServer::OnResponseReceived(uint8_t *buffer, size_t buffer_size) {
    if(buffer == nullptr || buffer_size == 0) {
        Logger::WriteLn("Empty data received!", buffer_size);
        m_UnlockState = UnlockState::DATA_ERROR;
        return;
    }

    uint8_t *decData{};
    size_t decLen{};
    auto cryptResult = CryptUtils::DecryptAESPacket(buffer, buffer_size, &decLen, m_PairedDevice.encryptionKey, &decData);
    if(cryptResult != PacketCryptResult::OK) {
        auto respStr = std::string((const char *)buffer, buffer_size);
        if(respStr == "CANCEL") {
            m_UnlockState = UnlockState::CANCELED;
            return;
        } else if(respStr == "NOT_PAIRED") {
            m_UnlockState = UnlockState::NOT_PAIRED_ERROR;
            return;
        } else if(respStr == "ERROR") {
            m_UnlockState = UnlockState::APP_ERROR;
            return;
        } else if(respStr == "TIME_ERROR") {
            m_UnlockState = UnlockState::TIME_ERROR;
            return;
        }

        switch (cryptResult) {
            case INVALID_TIMESTAMP: {
                Logger::WriteLn("Invalid timestamp on AES data!");
                m_UnlockState = UnlockState::TIME_ERROR;
                break;
            }
            default: {
                Logger::WriteLn("Invalid data received! (Size={})", buffer_size);
                m_UnlockState = UnlockState::DATA_ERROR;
                break;
            }
        }
        return;
    }

    // Parse data
    auto dataStr = std::string((char *)decData, decLen);
    SAFE_FREE(decData);
    try {
        auto json = nlohmann::json::parse(dataStr);
        auto response = UnlockResponseData();
        response.unlockToken = json["unlockToken"];
        response.password = json["password"];

        m_ResponseData = response;
        if(response.unlockToken == m_UnlockToken) {
            m_UnlockState = UnlockState::SUCCESS;
        } else {
            m_UnlockState = UnlockState::UNK_ERROR;
        }
    } catch(std::exception&) {
        Logger::WriteLn("Error parsing response data!");
        m_UnlockState = UnlockState::DATA_ERROR;
    }
}
