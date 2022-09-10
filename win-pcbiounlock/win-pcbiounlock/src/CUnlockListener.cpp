#include "CUnlockListener.h"

#include "CSampleProvider.h"
#include "CMessageCredential.h"

#include "AppStorage.h"
#include "FCMUtils.h"
#include "I18n.h"

#include "handler/UnlockHandler.h"

void CUnlockListener::Initialize(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, CSampleProvider* pCredProv, CMessageCredential* pMessageCredential)
{
    m_ProviderUsage = cpus;
    m_CredProv = pCredProv;
    m_MessageCred = pMessageCredential;
    m_ListenThread = std::thread(&CUnlockListener::ListenThread, this);
}

void CUnlockListener::Release()
{

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
    auto pairedDevice = PairedDeviceStorage::GetDeviceData();
    if (pairedDevice == std::nullopt) {
        m_MessageCred->UpdateMessage(I18n::Get("error_not_paired"));
        return;
    }

    auto userDomain = pairedDevice.value().userName;
    auto userSplit = Utils::SplitString(userDomain, '\\');
    if (userSplit.size() != 2) {
        m_MessageCred->UpdateMessage(I18n::Get("error_invalid_user"));
        return;
    }

    // Wait
    if (m_ProviderUsage == CPUS_LOGON || m_ProviderUsage == CPUS_UNLOCK_WORKSTATION)
    {
        // Key listener
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
    auto result = handler.GetResult(pairedDevice.value(), userSplit[1], "Windows-Login");

    m_HasResponse = true;
    m_CredProv->OnStatusChanged(result.state, result.additionalData);
}