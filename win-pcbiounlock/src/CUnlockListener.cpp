#include "CUnlockListener.h"

#include <SensAPI.h>

#include "CSampleProvider.h"
#include "handler/UnlockHandler.h"
#include "storage/AppStorage.h"
#include "I18n.h"

void CUnlockListener::Initialize(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,  CSampleProvider *pCredentialProvider, CUnlockCredential *pCredential, const std::wstring& userDomain)
{
    m_ProviderUsage = cpus;
    m_CredentialProvider = pCredentialProvider;
    m_Credential = pCredential;
    m_UserDomain = userDomain;
}

void CUnlockListener::Release()
{
    Stop();
}

void CUnlockListener::Start()
{
    if(m_IsRunning)
        return;
    Stop();
    m_IsRunning = true;
    m_ListenThread = std::thread(&CUnlockListener::ListenThread, this);
}

void CUnlockListener::Stop()
{
    if(!m_IsRunning)
        return;
    m_IsRunning = false;
    if(m_ListenThread.joinable())
        m_ListenThread.join();
}

bool CUnlockListener::HasResponse() const
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
    m_Credential->UpdateMessage(I18n::Get("initializing"));
    const auto userDomainStr = Utils::WideStringToString(m_UserDomain);
    const auto userSplit = Utils::SplitString(userDomainStr, '\\');
    if (userSplit.size() != 2) {
        m_Credential->UpdateMessage(I18n::Get("error_invalid_user"));
        return;
    }

    // Wait
    Sleep(500);
    auto storage = AppStorage::Get();
    auto devices = PairedDeviceStorage::GetDevices();
    const auto waitForNetwork = std::any_of(devices.begin(), devices.end(), [](const PairedDevice& device) {
        return device.pairingMethod == PairingMethod::TCP || device.pairingMethod == PairingMethod::CLOUD_TCP;
    });
    if (m_ProviderUsage == CPUS_LOGON || m_ProviderUsage == CPUS_UNLOCK_WORKSTATION) {
        // Network
        if (waitForNetwork) {
            m_Credential->UpdateMessage(I18n::Get("wait_network"));
            while (m_IsRunning) {
                DWORD flags{};
                if (IsNetworkAlive(&flags) && GetLastError() == 0 || GetAsyncKeyState(VK_LCONTROL) < 0 && GetAsyncKeyState(VK_LMENU) < 0)
                    break;
                Sleep(10);
            }
        }

        // Key press
        if(storage.waitForKeyPress) {
            Sleep(500);
            m_Credential->UpdateMessage(I18n::Get("wait_key_press"));
            byte lastKeys[KEY_RANGE];
            GetAllKeyState(lastKeys, KEY_RANGE);

            while (m_IsRunning) {
                byte keys[KEY_RANGE];
                GetAllKeyState(keys, KEY_RANGE);
                if (memcmp(keys, lastKeys, KEY_RANGE) != 0)
                    break;
                Sleep(10);
            }
        }
    }

    // Unlock
    std::function printMessage = [this](const std::string& s) {
        m_Credential->UpdateMessage(s);
    };
    auto handler = UnlockHandler(printMessage);
    const auto result = handler.GetResult(userDomainStr, "Windows-Login", &m_IsRunning);

    m_HasResponse = true;
    m_Credential->SetUnlockData(result);
    m_CredentialProvider->UpdateCredsStatus();
}
