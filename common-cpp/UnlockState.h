#ifndef PAM_PCBIOUNLOCK_UNLOCKSTATE_H
#define PAM_PCBIOUNLOCK_UNLOCKSTATE_H

#include <string>

enum UnlockState {
    UNKNOWN = 0,
    SUCCESS = 1,
    CANCELED = 2,
    TIMEOUT = 3,
    CONNECT_ERROR = 4
};

struct UnlockResponseData {
    std::string unlockToken;
    std::string additionalData;
};

#endif //PAM_PCBIOUNLOCK_UNLOCKSTATE_H
