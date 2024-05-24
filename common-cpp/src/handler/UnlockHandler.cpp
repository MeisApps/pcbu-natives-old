#include "UnlockHandler.h"
#include "storage/AppStorage.h"
#include "KeyScanner.h"
#include "I18n.h"

#include "../connection/clients/TCPUnlockClient.h"
#include "../connection/clients/BTUnlockClient.h"
#include "../connection/servers/TCPUnlockServer.h"

#ifdef _WIN32
#include <Windows.h>
#define KEY_LEFTCTRL VK_LCONTROL
#define KEY_LEFTALT VK_LMENU
#endif
#ifdef LINUX
#include <linux/input.h>
#endif
#ifdef APPLE
#include <Carbon/Carbon.h>
#define KEY_LEFTCTRL kVK_Control
#define KEY_LEFTALT kVK_Option
#endif

UnlockHandler::UnlockHandler(const std::function<void(std::string)>& printMessage) {
    m_PrintMessage = printMessage;
}

UnlockResult UnlockHandler::GetResult(const std::string& authUser, const std::string& authProgram, std::atomic<bool> *isRunning) {
    auto appSettings = AppStorage::Get();
    std::vector<BaseUnlockServer *> servers{};
    for(const auto& device : PairedDeviceStorage::GetDevicesForUser(authUser)) {
        BaseUnlockServer *server{};
        switch (device.pairingMethod) {
            case PairingMethod::TCP:
                server = new TCPUnlockClient(device.ipAddress, 43298, device);
                break;
            case PairingMethod::BLUETOOTH:
                server = new BTUnlockClient(device.bluetoothAddress, device);
                break;
            case PairingMethod::CLOUD_TCP:
                server = new TCPUnlockServer("0.0.0.0", appSettings.unlockServerPort, device);
                break;
        }
        if(server == nullptr) {
            Logger::WriteLn("Invalid pairing method.");
            continue;
        }
        server->SetUnlockInfo(authUser, authProgram);
        servers.emplace_back(server);
    }
    if(servers.empty()) {
        auto errorMsg = I18n::Get("error_not_paired", authUser);
        Logger::WriteLn(errorMsg);
        m_PrintMessage(errorMsg);
        return UnlockResult(UnlockState::NOT_PAIRED_ERROR);
    }

    // Start servers
    std::vector<std::thread> threads{};
    std::promise<UnlockResult> promise{};
    std::atomic completed(0);
    std::mutex mutex{};
    std::condition_variable cv{};
    std::shared_future future(promise.get_future());
    auto numServers = servers.size();
    for (auto server : servers) {
        threads.emplace_back([this, server, numServers, future, isRunning, &promise, &completed, &cv, &mutex]() {
            auto serverResult = RunServer(server, future, isRunning);
            completed.fetch_add(1);
            if(serverResult.state == UnlockState::SUCCESS || completed.load() == numServers) {
                promise.set_value(serverResult);
                std::lock_guard l(mutex);
                cv.notify_one();
            }
        });
    }

    // Wait
    std::unique_lock lock(mutex);
    cv.wait(lock, [&] {
        return future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready || completed.load() == numServers;
    });

    // Cleanup
    auto result = future.get();
    for (auto& thread : threads) {
        if (thread.joinable())
            thread.join();
    }
    for(const auto server : servers)
        delete server;
    return result;
}

UnlockResult UnlockHandler::RunServer(BaseUnlockServer *server, const std::shared_future<UnlockResult>& future, std::atomic<bool> *isRunning) {
    if(!server->Start()) {
        auto errorMsg = I18n::Get("error_start_handler");
        Logger::WriteLn(errorMsg);
        m_PrintMessage(errorMsg);
        return UnlockResult(UnlockState::START_ERROR);
    }

    m_PrintMessage(I18n::Get("wait_unlock"));
    auto keyScanner = KeyScanner();
    keyScanner.Start();

    auto timeoutMs = PACKET_TIMEOUT;
    auto state = UnlockState::UNKNOWN;
    auto startTime = Utils::GetCurrentTimeMillis();
    while (true) {
        state = server->PollResult();
        if(state != UnlockState::UNKNOWN)
            break;
        if(!server->HasClient() && Utils::GetCurrentTimeMillis() - startTime > timeoutMs) {
            state = UnlockState::TIMEOUT;
            break;
        }
        if(keyScanner.GetKeyState(KEY_LEFTCTRL) && keyScanner.GetKeyState(KEY_LEFTALT)) {
            state = UnlockState::CANCELED;
            break;
        }
        if(future.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready
            || (isRunning != nullptr && !isRunning->load())) {
            state = UnlockState::CANCELED;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server->Stop();
    keyScanner.Stop();
    m_PrintMessage(UnlockStateUtils::ToString(state));

    auto result = UnlockResult();
    result.state = state;
    result.device = server->GetDevice();
    result.password = server->GetResponseData().password;
    return result;
}
