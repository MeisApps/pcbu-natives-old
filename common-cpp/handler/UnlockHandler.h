#ifndef PAM_PCBIOUNLOCK_UNLOCKHANDLER_H
#define PAM_PCBIOUNLOCK_UNLOCKHANDLER_H

#include <functional>
#include <string>

#include "UnlockState.h"
#include "PairedDevice.h"

struct UnlockResult {
    UnlockState state;
    std::string additionalData;
};

class UnlockHandler {
public:
    explicit UnlockHandler(std::function<void (std::string)> printMessage);
    UnlockResult GetResult(PairedDevice device, const std::string& authUser, const std::string& authProgram);

    const UnlockResult RESULT_ERROR = {UnlockState::UNKNOWN, ""};
private:
    std::function<void (std::string)> m_PrintMessage;
};


#endif //PAM_PCBIOUNLOCK_UNLOCKHANDLER_H
