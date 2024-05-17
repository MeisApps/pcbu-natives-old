#include "CUnlockListener.h"

#include "CSampleProvider.h"
#include "CMessageCredential.h"

#include "storage/AppStorage.h"
#include "I18n.h"

#include "handler/UnlockHandler.h"
#include <SensAPI.h>
#pragma comment(lib, "SensAPI.lib")

void CUnlockListener::Initialize(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, CSampleProvider* pCredProv, CMessageCredential* pMessageCredential, const std::string& userDomain)
{
    m_ProviderUsage = cpus;
    m_CredProv = pCredProv;
    m_MessageCred = pMessageCredential;
    m_UserDomain = userDomain;
    m_ListenThread = std::thread(&CUnlockListener::ListenThread, this);
}

void CUnlockListener::Release()
{
    if(m_ListenThread.joinable())
        m_ListenThread.join();
}

bool CUnlockListener::HasResponse()
{
	return m_HasResponse;
}

#define KEY_RANGE 0xA6
void GetAllKeyState(byte* keys, size_t len)
{
    for (int i = 0; i < len; i++)
    {
        if (GetAsyncKeyState(i) < 0)
            keys[i] = 1;
        else
            keys[i] = 0;
    }
}

void CUnlockListener::ListenThread()
{
    // Init
    auto userSplit = Utils::SplitString(m_UserDomain, '\\');
    if (userSplit.size() != 2) {
        m_MessageCred->UpdateMessage(I18n::Get("error_invalid_user"));
        return;
    }

    // Wait
    auto devices = PairedDeviceStorage::GetDevices();
    auto waitForNetwork = std::any_of(devices.begin(), devices.end(), [](const PairedDevice& device)
    {
        return device.pairingMethod == PairingMethod::TCP || device.pairingMethod == PairingMethod::CLOUD_TCP;
    });
    if (m_ProviderUsage == CPUS_LOGON || m_ProviderUsage == CPUS_UNLOCK_WORKSTATION) {
        // Network
        if (waitForNetwork) {
            m_MessageCred->UpdateMessage(I18n::Get("wait_network"));
            while (true) {
                DWORD flags{};
                if (IsNetworkAlive(&flags) && GetLastError() == 0 || GetAsyncKeyState(VK_LCONTROL) < 0 && GetAsyncKeyState(VK_LMENU) < 0)
                    break;
                Sleep(10);
            }
        }

        // Key press
        Sleep(500);
        m_MessageCred->UpdateMessage(I18n::Get("wait_key_press"));

        byte lastKeys[KEY_RANGE];
        GetAllKeyState(lastKeys, KEY_RANGE);

        while (true) {
            byte keys[KEY_RANGE];
            GetAllKeyState(keys, KEY_RANGE);

            if (memcmp(keys, lastKeys, KEY_RANGE) != 0)
                break;
            Sleep(10);
        }
    }

    // Unlock
    std::function<void(const std::string&)> printMessage = [this](const std::string& s) {
        m_MessageCred->UpdateMessage(s);
    };

    auto handler = UnlockHandler(printMessage);
    auto result = handler.GetResult(m_UserDomain, "Windows-Login");

    m_HasResponse = true;
    m_CredProv->OnStatusChanged(result);
}
