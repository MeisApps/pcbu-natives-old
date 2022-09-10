#include "UnlockHandler.h"
#include "Logger.h"
#include "AppStorage.h"
#include "KeyScanner.h"
#include "I18n.h"

#include "clients/TCPUnlockClient.h"
#include "clients/BTUnlockClient.h"

#ifdef _WIN32
#include <Windows.h>
#define KEY_LEFTCTRL VK_LCONTROL
#define KEY_LEFTALT VK_LMENU
#else
#include <linux/input.h>
#endif

UnlockHandler::UnlockHandler(std::function<void(std::string)> printMessage) {
    m_PrintMessage = printMessage;
}

UnlockResult UnlockHandler::GetResult(PairedDevice device, const std::string& authUser, const std::string& authProgram) {
    auto appSettings = AppStorage::Get();
    auto useBluetooth = !device.bluetoothAddress.empty();

    /*auto unlockToken = Utils::GetRandomString(64);
    if(!useBluetooth && !FCMUtils::sendMessage(pairedDevice.value().pairingId, pairedDevice.value().messagingToken,
                              unlockToken, pairedDevice.value().encryptionKey,
                              userName, serviceName)) {
        auto errorMsg = "Error: Failed to send unlock message.";
        Logger::writeln(errorMsg);
        print_pam(conv, errorMsg);
        return PAM_IGNORE;
    }*/

    BaseUnlockServer *server;
    if(useBluetooth) {
        server = new BTUnlockClient(device.bluetoothAddress, device.pairingId, device.encryptionKey);
    } else {
        //server = new UnlockServer("0.0.0.0", appSettings.unlockServerPort, pairedDevice.value().encryptionKey, unlockToken);
        server = new TCPUnlockClient(device.ipAddress, 43298, device.pairingId, device.encryptionKey);
    }

    server->SetUnlockInfo(authUser, authProgram);
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

    if(state == UnlockState::SUCCESS) {
        m_PrintMessage(I18n::Get("unlock_success"));
    } else if(state == UnlockState::CANCELED) {
        m_PrintMessage(I18n::Get("unlock_canceled"));
    } else if(state == UnlockState::TIMEOUT) {
        m_PrintMessage(I18n::Get("unlock_timeout"));
    } else if(state == UnlockState::CONNECT_ERROR) {
        m_PrintMessage(I18n::Get("error_connect"));
    } else {
        m_PrintMessage(I18n::Get("error_unknown"));
    }

    auto result = UnlockResult();
    result.state = state;
    result.additionalData = server->GetResponseData().additionalData;
    return result;
}