#ifndef PAM_PCBIOUNLOCK_PAIREDDEVICE_H
#define PAM_PCBIOUNLOCK_PAIREDDEVICE_H

#include <string>
#include <fstream>
#include <optional>

#include "deps/json.hpp"
#include "Logger.h"

#ifdef _WIN32
#define PAIRED_DEVICE_FILE_PATH "C:/ProgramData/PCBioUnlock/paired_device.json"
#else
#define PAIRED_DEVICE_FILE_PATH "/etc/pc-bio-unlock/paired_device.json"
#endif

struct PairedDevice {
    std::string pairingId;
    std::string messagingToken;
    std::string ipAddress;
    std::string bluetoothAddress;
    std::string encryptionKey;
    std::string userName;
};

class PairedDeviceStorage {
public:
    static std::optional<PairedDevice> GetDeviceData() {
        try {
            std::ifstream inFile(PAIRED_DEVICE_FILE_PATH);
            nlohmann::json json;
            inFile >> json;

            auto device = PairedDevice();
            device.pairingId = json["pairingId"];
            device.messagingToken = json["messagingToken"];
            device.ipAddress = json["ipAddress"];
            device.bluetoothAddress = json["bluetoothAddress"];
            device.encryptionKey = json["encryptionKey"];
            device.userName = json["userName"];
            return device;
        }
        catch (...) {
            Logger::writeln("Failed reading paired device data !");
            return std::nullopt;
        }
    }

private:
    PairedDeviceStorage() = default;
};

#endif //PAM_PCBIOUNLOCK_PAIREDDEVICE_H
