#ifndef PAM_PCBIOUNLOCK_BTUNLOCKCLIENT_H
#define PAM_PCBIOUNLOCK_BTUNLOCKCLIENT_H


#include "../servers/BaseUnlockServer.h"

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
#define SOCKET int
#endif

class BTUnlockClient : public BaseUnlockServer {
public:
    BTUnlockClient(std::string deviceAddress, const std::string& pairingId, const std::string& encryptionPwd);

    bool Start() override;
    void Stop() override;
private:
    void ConnectThread();

    int m_Channel;
    SOCKET m_ClientSocket;

    std::string m_DeviceAddress;
    std::string m_PairingId;
};


#endif //PAM_PCBIOUNLOCK_BTUNLOCKCLIENT_H
