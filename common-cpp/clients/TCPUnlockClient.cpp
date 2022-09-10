#include "TCPUnlockClient.h"
#include "utils/CryptUtils.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>

#define read(x,y,z) recv(x, (char*)y, z, 0)
#define write(x,y,z) send(x, y, z, 0)
#define SAFE_CLOSE(x) if(x != (SOCKET)-1) { closesocket(x); x = (SOCKET)-1; }
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SAFE_CLOSE(x) if(x != -1) { close(x); x = -1; }
#endif

TCPUnlockClient::TCPUnlockClient(const std::string& ipAddress, int port, const std::string& pairingId, const std::string& encryptionPassword)
    : BaseUnlockServer("", "") {
    m_IP = ipAddress;
    m_Port = port;
    m_PairingId = pairingId;
    m_EncryptionPwd = encryptionPassword;
    m_ClientSocket = (SOCKET)-1;
    m_IsRunning = false;
}

bool TCPUnlockClient::Start() {
    if(m_IsRunning)
        return true;

#ifdef _WIN32
    WSADATA wsa;
    memset(&wsa, 0, sizeof(wsa));

    int error = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (error != 0) {
        Logger::writeln("WSAStartup failed.");
        return false;
    }
#endif

    m_IsRunning = true;
    m_AcceptThread = std::thread(&TCPUnlockClient::ConnectThread, this);
    return true;
}

void TCPUnlockClient::Stop() {
    if(!m_IsRunning)
        return;

    if(m_ClientSocket != -1 && m_HasConnection)
        write(m_ClientSocket, "CLOSE", 5);

    m_IsRunning = false;
    m_HasConnection = false;
    SAFE_CLOSE(m_ClientSocket);
}

void TCPUnlockClient::ConnectThread() {
    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((u_short)m_Port);

    if (inet_pton(AF_INET, m_IP.c_str(), &serv_addr.sin_addr) <= 0) {
        Logger::writeln("Invalid IP address.");
        m_IsRunning = false;
        return;
    }

    if ((m_ClientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        Logger::writeln("socket failed.");
        m_IsRunning = false;
        return;
    }

    int result = connect(m_ClientSocket, (struct sockaddr*)&serv_addr,sizeof(serv_addr));
    if (result == 0) {
        m_HasConnection = true;

        // Unlock info
        nlohmann::json encServerData = {
                {"authUser", m_AuthUser},
                {"authProgram", m_AuthProgram}
        };

        auto encServerDataStr = encServerData.dump();
        std::vector<uint8_t> myVector(encServerDataStr.begin(), encServerDataStr.end());
        uint8_t *serverDataPtr = &myVector[0];

        size_t encLen;
        auto encData = CryptUtils::EncryptAESPacket(serverDataPtr, encServerDataStr.size(), &encLen, m_EncryptionPwd);
        auto encStr = Utils::ToHexString(encData, encLen);
        SAFE_FREE(encData);

        nlohmann::json serverData = {
                {"pairingId", m_PairingId},
                {"encData", encStr}
        };
        auto serverDataStr = serverData.dump();
        write(m_ClientSocket, serverDataStr.c_str(), (int)serverDataStr.size());

        // Read response
        char buffer[1024] = { 0 };
        long bytesRead = read(m_ClientSocket, buffer, sizeof(buffer));

        OnDataReceived((uint8_t *)buffer, bytesRead);
    } else {
        Logger::writeln("Connect failed.");
        m_UnlockState = UnlockState::CONNECT_ERROR;
        return;
    }

    m_IsRunning = false;
    SAFE_CLOSE(m_ClientSocket)
}