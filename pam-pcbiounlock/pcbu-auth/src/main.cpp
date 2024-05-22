#include <iostream>

#ifdef LINUX
#include <unistd.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#endif
#ifdef APPLE
#include <CoreServices/CoreServices.h>
#endif

#include "handler/UnlockHandler.h"
#include "I18n.h"

static bool CheckPassword(const char* user, const char* password) {
#ifdef LINUX
    struct passwd *passwdEntry = getpwnam(user);
    if (!passwdEntry) {
        Logger::writeln("Failed to read passwd entry for user: ", std::string(user));
        return 1;
    }
    if (strcmp(passwdEntry->pw_passwd, "x") != 0)
        return strcmp(passwdEntry->pw_passwd, crypt(password, passwdEntry->pw_passwd));

    struct spwd *shadowEntry = getspnam(user);
    if (!shadowEntry) {
        Logger::writeln("Failed to read shadow entry for user: " + std::string(user));
        return 1;
    }
    return strcmp(shadowEntry->sp_pwdp, crypt(password, shadowEntry->sp_pwdp)) == 0;
#endif
#ifdef APPLE
    bool isValid = false;
    auto cfUsername = CFStringCreateWithCString(nullptr, user, kCFStringEncodingUTF8);
    auto cfPassword = CFStringCreateWithCString(nullptr, password, kCFStringEncodingUTF8);
    auto query = CSIdentityQueryCreateForName(kCFAllocatorDefault, cfUsername, kCSIdentityQueryStringEquals, kCSIdentityClassUser, CSGetDefaultIdentityAuthority());

    CSIdentityQueryExecute(query, kCSIdentityQueryGenerateUpdateEvents, nullptr);
    auto idArray = CSIdentityQueryCopyResults(query);
    if (CFArrayGetCount(idArray) == 1) {
        auto result = (CSIdentityRef) CFArrayGetValueAtIndex(idArray, 0);
        if (CSIdentityAuthenticateUsingPassword(result, cfPassword)) {
            isValid = true;
        }
    }

    CFRelease(cfUsername);
    CFRelease(cfPassword);
    CFRelease(idArray);
    CFRelease(query);
    return isValid;
#endif
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    if(setuid(0) != 0) {
        printf("setuid(0) failed.\n");
        return -1;
    }

    Logger::init();
    if(argc != 3) {
        printf("Invalid parameters.\n");
        return -1;
    }

    auto userName = argv[1];
    auto serviceName = argv[2];
    std::function<void (const std::string&)> printMessage = [](const std::string& s) {
        printf("%s\n", s.c_str());
    };
    auto handler = UnlockHandler(printMessage);
    auto result = handler.GetResult(userName, serviceName);
    if(result.state == UnlockState::SUCCESS) {
        if(strcmp(userName, result.device.userName.c_str()) == 0) {
            if(CheckPassword(userName, result.password.c_str())) {
                //pam_set_item(pamh, PAM_AUTHTOK, result.additionalData.c_str());
                return 0;
            }
        }

        auto errorMsg = I18n::Get("error_password");
        Logger::writeln(errorMsg);
        printf("%s\n", errorMsg.c_str());
        return 1;
    } else if(result.state == UnlockState::CANCELED) {
        return 1;
    } else if(result.state == UnlockState::TIMEOUT || result.state == UnlockState::CONNECT_ERROR) {
        return -1;
    }
    return 1;
}
