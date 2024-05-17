#ifndef PAM_PCBIOUNLOCK_PAIREDDEVICE_H
#define PAM_PCBIOUNLOCK_PAIREDDEVICE_H

#include <string>
#include <fstream>
#include <optional>

#include "json.hpp"
#include "../Logger.h"

#ifdef _WIN32
#define PAIRED_DEVICE_FILE_PATH "C:/ProgramData/PCBioUnlock/paired_device.json"
#else
#define PAIRED_DEVICE_FILE_PATH "/etc/pc-bio-unlock/paired_device.json"
#endif

enum class PairingMethod {
    TCP,
    BLUETOOTH,
    CLOUD_TCP
};

struct PairedDevice {
    std::string pairingId{};
    PairingMethod pairingMethod{};
    std::string deviceName{};
    std::string userName{};
    std::string encryptionKey{};

    std::string ipAddress{};
    std::string bluetoothAddress{};
    std::string cloudToken{};
};

class PairedDeviceStorage {
public:
    static std::vector<PairedDevice> GetDevicesForUser(const std::string& userName);
    static std::vector<PairedDevice> GetDevices();

private:
    PairedDeviceStorage() = default;
};

#endif //PAM_PCBIOUNLOCK_PAIREDDEVICE_H
