#ifndef PAM_PCBIOUNLOCK_UNLOCKSERVER_H
#define PAM_PCBIOUNLOCK_UNLOCKSERVER_H

#include "../BaseUnlockServer.h"

#ifdef _WIN32
#include <WinSock2.h>
#define socklen_t int
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#endif

class UnlockServer : public BaseUnlockServer {
public:
    UnlockServer(std::string ip, int port,
                 std::string encryptionPwd, std::string unlockToken);
    ~UnlockServer();

    bool Start() override;
    void Stop() override;

private:
    void AcceptThread();

    SOCKET m_ServerSocket;
    struct sockaddr_in m_Address{};

    SOCKET m_ClientSocket;
};


#endif //PAM_PCBIOUNLOCK_UNLOCKSERVER_H
