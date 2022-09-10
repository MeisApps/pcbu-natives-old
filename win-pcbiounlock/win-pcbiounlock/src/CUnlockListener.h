#pragma once

#include <thread>

#include <WinSock2.h>
#include <credentialprovider.h>

class CSampleProvider;
class CMessageCredential;
class CUnlockListener
{
public:
	CUnlockListener() = default;
	void Initialize(CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus, CSampleProvider *pCredProv, CMessageCredential *pMessageCredential);
	void Release();

	bool HasResponse();

private:
	void ListenThread();

	std::thread m_ListenThread;
	bool m_HasResponse;

	CREDENTIAL_PROVIDER_USAGE_SCENARIO m_ProviderUsage;
	CSampleProvider* m_CredProv;
	CMessageCredential* m_MessageCred;
};
