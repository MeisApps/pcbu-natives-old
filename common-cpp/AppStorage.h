#ifndef PAM_PCBIOUNLOCK_APPSTORAGE_H
#define PAM_PCBIOUNLOCK_APPSTORAGE_H

#include <fstream>
#include "Logger.h"
#include "deps/json.hpp"

#ifdef _WIN32
#define APP_STORAGE_FILE_PATH "C:/ProgramData/PCBioUnlock/app_settings.json"
#else
#define APP_STORAGE_FILE_PATH "/etc/pc-bio-unlock/app_settings.json"
#endif

struct PCBUAppSettings {
    std::string language;
    std::string serverIP;
    int unlockServerPort{};
};

class AppStorage {
public:
    static PCBUAppSettings Get() {
        try {
            std::ifstream inFile(APP_STORAGE_FILE_PATH);
            nlohmann::json json;
            inFile >> json;

            auto settings = PCBUAppSettings();
            settings.language = json["language"];
            settings.serverIP = json["serverIP"];
            settings.unlockServerPort = json["unlockServerPort"];
            return settings;
        }
        catch (...) {
            Logger::writeln("Failed reading app storage !");
            auto def = PCBUAppSettings();
            def.language = "auto";
            def.serverIP = "auto";
            def.unlockServerPort = 43296;
            return def;
        }
    }

private:
    AppStorage() = default;
};

#endif //PAM_PCBIOUNLOCK_APPSTORAGE_H
