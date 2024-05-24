#include "PairedDevice.h"

std::vector<PairedDevice> PairedDeviceStorage::GetDevicesForUser(const std::string &userName) {
    std::vector<PairedDevice> result{};
    for(const auto& device : GetDevices()) {
        if(device.userName == userName)
            result.emplace_back(device);
    }
    return result;
}

std::vector<PairedDevice> PairedDeviceStorage::GetDevices() {
    std::vector<PairedDevice> result{};
    try {
        std::ifstream inFile(PAIRED_DEVICE_FILE_PATH);
        nlohmann::json json;
        inFile >> json;
        for(auto entry : json) {
            auto device = PairedDevice();
            device.pairingId = entry["pairingId"];
            device.deviceName = entry["deviceName"];
            device.userName = entry["userName"];
            device.encryptionKey = entry["encryptionKey"];

            device.ipAddress = entry["ipAddress"];
            device.bluetoothAddress = entry["bluetoothAddress"];
            device.cloudToken = entry["cloudToken"];

            auto methodStr = std::string(entry["pairingMethod"]);
            if(methodStr == "TCP")
                device.pairingMethod = PairingMethod::TCP;
            else if(methodStr == "BLUETOOTH")
                device.pairingMethod = PairingMethod::BLUETOOTH;
            else if(methodStr == "CLOUD_TCP")
                device.pairingMethod = PairingMethod::CLOUD_TCP;
            else
                throw std::runtime_error("Invalid pairing method.");
            result.emplace_back(device);
        }
    }
    catch (const std::exception& ex) {
        Logger::WriteLn("Failed reading paired device data. {}", std::string(ex.what()));
    }
    return result;
}
