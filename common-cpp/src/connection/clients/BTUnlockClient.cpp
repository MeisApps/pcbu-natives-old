#include "BTUnlockClient.h"

#include "utils/BTUtils.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2bth.h>

#define AF_BLUETOOTH AF_BTH
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM
#define read(x,y,z) recv(x, (char*)y, z, 0)
#define write(x,y,z) send(x, y, z, 0)
#define SAFE_CLOSE(x) if(x != (SOCKET)-1) { closesocket(x); x = (SOCKET)-1; }
#endif
#ifdef LINUX
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>

#define SAFE_CLOSE(x) if(x != -1) { close(x); x = -1; }
#endif

BTUnlockClient::BTUnlockClient(const std::string& deviceAddress, const PairedDevice& device)
    : BaseUnlockServer(device) {
    m_DeviceAddress = deviceAddress;
    m_Channel = -1;
    m_ClientSocket = (SOCKET)-1;
    m_IsRunning = false;
}

bool BTUnlockClient::Start() {
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
    m_AcceptThread = std::thread(&BTUnlockClient::ConnectThread, this);
    return true;
}

void BTUnlockClient::Stop() {
    if(!m_IsRunning)
        return;

    if(m_ClientSocket != -1 && m_HasConnection)
        write(m_ClientSocket, "CLOSE", 5);

    m_IsRunning = false;
    m_HasConnection = false;
    SAFE_CLOSE(m_ClientSocket);
    if(m_AcceptThread.joinable())
        m_AcceptThread.join();
}

void BTUnlockClient::ConnectThread() {
#ifdef LINUX
    // 62182bf7-97c8-45f9-aa2c-53c5f2008bdf
    static uint8_t CHANNEL_UUID[16] = { 0x62, 0x18, 0x2b, 0xf7, 0x97, 0xc8,
                                        0x45, 0xf9, 0xaa, 0x2c, 0x53, 0xc5, 0xf2, 0x00, 0x8b, 0xdf };

    m_Channel = BTUtils::FindChannelSDP(m_DeviceAddress, CHANNEL_UUID);
    if (m_Channel == -1) {
        Logger::writeln("Bluetooth channel failed.");
        m_IsRunning = false;
        m_UnlockState = UnlockState::CONNECT_ERROR;
        return;
    }
#endif
    m_ClientSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if(m_ClientSocket < 0) {
        Logger::writeln("Bluetooth socket failed.");
        m_IsRunning = false;
        m_UnlockState = UnlockState::UNK_ERROR;
        return;
    }

#ifdef _WIN32
    GUID guid = { 0x62182bf7, 0x97c8, 0x45f9, { 0xaa, 0x2c, 0x53, 0xc5, 0xf2, 0x00, 0x8b, 0xdf } };
    BTH_ADDR addr;
    BTUtils::str2ba2(m_DeviceAddress.c_str(), &addr);

    SOCKADDR_BTH address;
    memset(&address, 0, sizeof(address));
    address.addressFamily = AF_BTH;
    address.serviceClassId = guid;
    address.btAddr = addr;
#endif
#ifdef LINUX
    struct sockaddr_rc address = { 0 };
    address.rc_family = AF_BLUETOOTH;
    address.rc_channel = m_Channel;
    str2ba(m_DeviceAddress.c_str(), &address.rc_bdaddr);
#endif

    int result = connect(m_ClientSocket, (struct sockaddr*)&address, sizeof(address));
    if(result == 0) {
        m_HasConnection = true;
        auto serverDataStr = GetUnlockInfoPacket();
        if(serverDataStr.empty()) {
            m_IsRunning = false;
            m_UnlockState = UnlockState::UNK_ERROR;
            SAFE_CLOSE(m_ClientSocket);
            return;
        }
        write(m_ClientSocket, serverDataStr.c_str(), (int)serverDataStr.size());

        // Read response
        char buffer[1024] = { 0 };
        long bytesRead = read(m_ClientSocket, buffer, sizeof(buffer));
        OnResponseReceived((uint8_t *)buffer, bytesRead);
    } else {
        Logger::writeln("Bluetooth connect failed.");
        m_UnlockState = UnlockState::CONNECT_ERROR;
    }

    m_IsRunning = false;
    SAFE_CLOSE(m_ClientSocket);
}
