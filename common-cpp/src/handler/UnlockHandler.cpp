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
#else
#include <linux/input.h>
#endif

UnlockHandler::UnlockHandler(const std::function<void(std::string)>& printMessage) {
    m_PrintMessage = printMessage;
}

UnlockResult UnlockHandler::GetResult(const std::string& authUser, const std::string& authProgram) {
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
            Logger::writeln("Invalid pairing method.");
            continue;
        }
        servers.emplace_back(server);
    }
    if(servers.empty()) {
        auto errorMsg = I18n::Get("error_not_paired", authUser);
        Logger::writeln(errorMsg);
        m_PrintMessage(errorMsg);
        return UnlockResult(UnlockState::NOT_PAIRED_ERROR);
    }

    auto result = UnlockResult(UnlockState::UNKNOWN);
    for(auto server : servers) {
        server->SetUnlockInfo(authUser, authProgram);
        result = RunServer(server); // ToDo
        if(result.state == UnlockState::SUCCESS)
            break;
    }

    for(auto server : servers)
        delete server;
    return result;
}

UnlockResult UnlockHandler::RunServer(BaseUnlockServer *server) {
    if(!server->Start()) {
        auto errorMsg = I18n::Get("error_start_handler");
        Logger::writeln(errorMsg);
        m_PrintMessage(errorMsg);
        return RESULT_ERROR;
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
