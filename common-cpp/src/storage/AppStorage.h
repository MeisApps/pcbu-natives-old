#ifndef PAM_PCBIOUNLOCK_APPSTORAGE_H
#define PAM_PCBIOUNLOCK_APPSTORAGE_H

#include <fstream>
#include "../Logger.h"
#include "json.hpp"

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
    static PCBUAppSettings Get();

private:
    AppStorage() = default;
};

#endif //PAM_PCBIOUNLOCK_APPSTORAGE_H
