#ifndef PAM_PCBIOUNLOCK_UNLOCKHANDLER_H
#define PAM_PCBIOUNLOCK_UNLOCKHANDLER_H

#include <functional>
#include <future>
#include <string>

#include "UnlockState.h"
#include "storage/PairedDevice.h"
#include "../connection/BaseUnlockServer.h"

struct UnlockResult {
    UnlockResult() = default;
    explicit UnlockResult(UnlockState state) {
        this->state = state;
    }

    UnlockState state{};
    PairedDevice device{};
    std::string password{};
};

class UnlockHandler {
public:
    explicit UnlockHandler(const std::function<void (std::string)>& printMessage);
    UnlockResult GetResult(const std::string& authUser, const std::string& authProgram);

    const UnlockResult RESULT_ERROR = UnlockResult(UnlockState::UNKNOWN);
private:
    UnlockResult RunServer(BaseUnlockServer *server, const std::shared_future<UnlockResult>& future);

    std::function<void (std::string)> m_PrintMessage;
};


#endif //PAM_PCBIOUNLOCK_UNLOCKHANDLER_H
