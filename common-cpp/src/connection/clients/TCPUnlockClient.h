#ifndef PAM_PCBIOUNLOCK_TCPUNLOCKCLIENT_H
#define PAM_PCBIOUNLOCK_TCPUNLOCKCLIENT_H

#include "../BaseUnlockServer.h"

#ifdef _WIN32
typedef unsigned long long SOCKET;
#else
#define SOCKET int
#endif

class TCPUnlockClient : public BaseUnlockServer {
public:
    TCPUnlockClient(const std::string& ipAddress, int port, const PairedDevice& device);

    bool Start() override;
    void Stop() override;
private:
    void ConnectThread();

    std::string m_IP;
    int m_Port;
    SOCKET m_ClientSocket;
    std::string m_DeviceAddress;
};

#endif //PAM_PCBIOUNLOCK_TCPUNLOCKCLIENT_H
