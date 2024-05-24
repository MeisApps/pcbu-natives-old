#include "AppStorage.h"

PCBUAppSettings AppStorage::Get() {
    try
    {
        std::ifstream inFile(APP_STORAGE_FILE_PATH);
        nlohmann::json json;
        inFile >> json;

        auto settings = PCBUAppSettings();
        settings.language = json["language"];
        settings.serverIP = json["serverIP"];
        settings.unlockServerPort = json["unlockServerPort"];
        settings.waitForKeyPress = json["waitForKeyPress"];
        return settings;
    }
    catch (...) {
        Logger::WriteLn("Failed reading app storage!");
        auto def = PCBUAppSettings();
        def.language = "auto";
        def.serverIP = "auto";
        def.unlockServerPort = 43296;
        def.waitForKeyPress = false;
        return def;
    }
}
