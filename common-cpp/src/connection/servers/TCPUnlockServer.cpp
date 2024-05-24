#include "TCPUnlockServer.h"

#include <utility>

#ifdef _WIN32
#define read(x,y,z) recv(x, (char*)y, z, 0)
#define SAFE_CLOSE(x) if(x != 0) { closesocket(x); x = 0; }
#else
#define SAFE_CLOSE(x) if(x != 0) { close(x); x = 0; }
#endif

TCPUnlockServer::TCPUnlockServer(const std::string& ip, int port, const PairedDevice& device)
    : BaseUnlockServer(device) {
    m_IsRunning = false;
    m_HasConnection = false;
    m_UnlockState = UnlockState::UNKNOWN;
    m_ResponseData = {};

    m_ServerSocket = 0;
    m_ClientSocket = 0;

    m_Address.sin_family = AF_INET;
    m_Address.sin_addr.s_addr = INADDR_ANY;
    m_Address.sin_port = htons((u_short)port);
}

bool TCPUnlockServer::Start() {
    if (m_IsRunning)
        return true;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Logger::WriteLn("startup failed");
        return false;
    }
#endif

    if ((m_ServerSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        Logger::WriteLn("socket failed.");
        return false;
    }

    int opt = 1;
    if (setsockopt(m_ServerSocket, SOL_SOCKET,
                   SO_REUSEADDR, (char*)&opt,
                   sizeof(opt))) {
        Logger::WriteLn("setsockopt failed.");
        return false;
    }

    if (bind(m_ServerSocket, (struct sockaddr*)&m_Address, sizeof(m_Address)) < 0) {
        Logger::WriteLn("bind failed.");
        return false;
    }

    if (listen(m_ServerSocket, 1) == -1) {
        Logger::WriteLn("listen failed.");
        return false;
    }

    m_IsRunning = true;
    m_AcceptThread = std::thread(&TCPUnlockServer::AcceptThread, this);

    //Logger::println("Server started.\n");
    return true;
}

void TCPUnlockServer::Stop() {
    if (!m_IsRunning)
        return;

    SAFE_CLOSE(m_ClientSocket);
    SAFE_CLOSE(m_ServerSocket);

    m_IsRunning = false;
    m_HasConnection = false;
    m_AcceptThread.join();

    //Logger::println("Server stopped.\n");
}

void TCPUnlockServer::AcceptThread() {
    struct timeval timeout{};
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    // Accept
    SOCKET selectStatus = 0;
    while (m_IsRunning) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(m_ServerSocket, &read_fds);

        selectStatus = select((int)m_ServerSocket + 1, &read_fds, nullptr, nullptr, &timeout);
        if (selectStatus == -1) {
            m_IsRunning = false;
            SAFE_CLOSE(m_ServerSocket);
            return;
        } else if (selectStatus > 0) {
            break; // Connected
        }
    }

    // Close
    if(selectStatus == 0) {
        m_IsRunning = false;
        SAFE_CLOSE(m_ServerSocket);
        return;
    }

    // Connect
    socklen_t addrlen = sizeof(m_Address);
    if ((m_ClientSocket = accept(m_ServerSocket, (struct sockaddr*)&m_Address, &addrlen)) < 0) {
        m_IsRunning = false;
        SAFE_CLOSE(m_ServerSocket);
        return;
    }

    // Connected
    m_HasConnection = true;

    // Get data
    uint8_t buffer[1024] = {};
    auto bytesRead = read(m_ClientSocket, buffer, 1024);
    if(bytesRead <= 0) {
        Logger::WriteLn("Response read error !\n");
        m_UnlockState = UnlockState::CANCELED;

        SAFE_CLOSE(m_ClientSocket);
        SAFE_CLOSE(m_ServerSocket);
        return;
    }

    OnResponseReceived(buffer, bytesRead);

    //m_IsRunning = false;
    //m_HasConnection = false;
    SAFE_CLOSE(m_ClientSocket);
    SAFE_CLOSE(m_ServerSocket);
}
