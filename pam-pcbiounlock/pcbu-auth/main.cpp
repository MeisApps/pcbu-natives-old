#include <iostream>
#include <pwd.h>
#include <shadow.h>

#include "handler/UnlockHandler.h"
#include "servers/tcp/UnlockServer.h"
#include "AppStorage.h"
#include "I18n.h"

static int CheckPassword(const char* user, const char* password) {
    struct passwd *passwdEntry = getpwnam(user);
    if (!passwdEntry) {
        Logger::writeln("User doesn't exist: ", std::string(user));
        return 1;
    }

    if (0 != strcmp(passwdEntry->pw_passwd, "x")) {
        return strcmp(passwdEntry->pw_passwd, crypt(password, passwdEntry->pw_passwd));
    } else {
        // password is in shadow file
        struct spwd *shadowEntry = getspnam(user);
        if (!shadowEntry) {
            Logger::writeln("Failed to read shadow entry.");
            return 1;
        }

        return strcmp(shadowEntry->sp_pwdp, crypt(password, shadowEntry->sp_pwdp));
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    if(setuid(0) != 0) {
        printf("Could not setuid.\n");
        return -1;
    }

    Logger::init();
    if(argc != 3) {
        printf("Invalid parameters.\n");
        return -1;
    }

    auto userName = argv[1];
    auto serviceName = argv[2];

    auto pairedDevice = PairedDeviceStorage::GetDeviceData();
    if (pairedDevice == std::nullopt) {
        auto errorMsg = I18n::Get("error_not_paired");
        Logger::writeln(errorMsg);
        printf("%s\n", errorMsg.c_str());
        return 1;
    }

    std::function<void (const std::string&)> printMessage = [](const std::string& s) {
        printf("%s\n", s.c_str());
    };
    auto handler = UnlockHandler(printMessage);
    auto result = handler.GetResult(pairedDevice.value(), userName, serviceName);

    auto state = result.state;
    if(state == UnlockState::SUCCESS) {
        if(strcmp(userName, pairedDevice.value().userName.c_str()) == 0 || strcmp(userName, "root") == 0) {
            if(CheckPassword(userName, result.additionalData.c_str()) == 0) {
                //pam_set_item(pamh, PAM_AUTHTOK, result.additionalData.c_str());
                return 0;
            }
        }

        auto errorMsg = I18n::Get("error_password");
        Logger::writeln(errorMsg);
        printf("%s\n", errorMsg.c_str());
        return 1;
    } else if(state == UnlockState::CANCELED) {
        return 1;
    } else if(state == UnlockState::TIMEOUT || state == UnlockState::CONNECT_ERROR) {
        return -1;
    }

    return 1;
}
