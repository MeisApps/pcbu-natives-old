/*
#include "FCMUtils.h"
#include "utils/CryptUtils.h"
#include "api/api.h"
#include "AppStorage.h"
#include "Logger.h"

#include "deps/json.hpp"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "deps/httplib.h"

bool FCMUtils::sendMessage(std::string pairingId, std::string messagingToken,
                            std::string unlockToken, std::string encryptionKey,
                            std::string authUser, std::string authProgram) {
    auto settings = AppStorage::Get();
    auto address = settings.serverIP;
    if(address.empty() || address == "auto") {
        auto detectedAddr = get_local_ip();
        if (detectedAddr == nullptr) {
            Logger::writeln("Error getting local IP address.");
            return false;
        }

        address = std::string(detectedAddr);
        api_free((void *)detectedAddr);
        Logger::writeln("Local IP: ", address);
    }

    try {
        nlohmann::json encryptedServerData = {
            {"ip", address},
            {"port", settings.unlockServerPort},
            {"unlockToken", unlockToken},
            {"authUser", authUser},
            {"authProgram", authProgram}
        };

        uint8_t encBuffer[1024] = {};
        auto encServerDataStr = encryptedServerData.dump();
        std::vector<uint8_t> myVector(encServerDataStr.begin(), encServerDataStr.end());
        uint8_t *serverDataPtr = &myVector[0];

        auto encLen = CryptUtils::EncryptAES(serverDataPtr, encServerDataStr.size(), encBuffer, sizeof(encBuffer), encryptionKey.c_str());
        auto encStr = Utils::ToHexString(encBuffer, encLen);

        nlohmann::json serverData = {
            {"pairingId", pairingId},
            {"encryptedData", encStr}
        };

        nlohmann::json msgBody = {
            {"clientToken", messagingToken},
            {"serverData", serverData.dump()},
        };

        httplib::Client cli("https://api.meis-apps.com");
        //cli.enable_server_certificate_verification(false);

        auto res = cli.Post("/rest/test", msgBody.dump(), "application/json");
        if (!res) {
            Logger::writeln("Error sending message: ", to_string(res.error()));
            return false;
        }

        //printf("Response: %s\n", res->body.c_str());
        if (res->status == 200)
            return true;

        return false;
    }
    catch (...) {
        Logger::writeln("Error sending message !");
        return false;
    }
}
 */