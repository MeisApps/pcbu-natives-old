#include "BTUnlockClient.h"
#include "utils/CryptUtils.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2bth.h>

#define AF_BLUETOOTH AF_BTH
#define BTPROTO_RFCOMM BTHPROTO_RFCOMM

#define read(x,y,z) recv(x, (char*)y, z, 0)
#define write(x,y,z) send(x, y, z, 0)
#define SAFE_CLOSE(x) if(x != (SOCKET)-1) { closesocket(x); x = (SOCKET)-1; }
#else
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <sys/socket.h>

#include "../BTUtils.h"

#define SAFE_CLOSE(x) if(x != -1) { close(x); x = -1; }
#endif

#include <utility>
#include "../api/bt_api.h"

#ifdef _WIN32
int str2ba2(const char* straddr, BTH_ADDR* btaddr)
{
    int i;
    unsigned int aaddr[6];
    BTH_ADDR tmpaddr = 0;

    if (std::sscanf(straddr, "%02x:%02x:%02x:%02x:%02x:%02x",
        &aaddr[0], &aaddr[1], &aaddr[2],
        &aaddr[3], &aaddr[4], &aaddr[5]) != 6)
        return 1;
    *btaddr = 0;
    for (i = 0; i < 6; i++) {
        tmpaddr = (BTH_ADDR)(aaddr[i] & 0xff);
        *btaddr = ((*btaddr) << 8) + tmpaddr;
    }
    return 0;
}
#endif


BTUnlockClient::BTUnlockClient(std::string deviceAddress, const std::string& pairingId, const std::string &encryptionPwd)
    : BaseUnlockServer(encryptionPwd, "") {
    m_DeviceAddress = std::move(deviceAddress);
    m_PairingId = pairingId;

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
}

void BTUnlockClient::ConnectThread() {
#ifndef _WIN32
    // 62182bf7-97c8-45f9-aa2c-53c5f2008bdf
    static uint8_t CHANNEL_UUID[16] = { 0x62, 0x18, 0x2b, 0xf7, 0x97, 0xc8,
                                        0x45, 0xf9, 0xaa, 0x2c, 0x53, 0xc5, 0xf2, 0x00, 0x8b, 0xdf };

    m_Channel = BTUtils::FindChannelSDP(m_DeviceAddress, CHANNEL_UUID);
    if (m_Channel == -1) {
        Logger::writeln("Bluetooth channel failed.");
        m_UnlockState = UnlockState::CONNECT_ERROR;
        m_IsRunning = false;
        return;
    }
#endif

    m_ClientSocket = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
    if(m_ClientSocket < 0) {
        Logger::writeln("Bluetooth socket failed.");
        m_IsRunning = false;
        return;
    }

#ifdef _WIN32
    GUID guid = { 0x62182bf7, 0x97c8, 0x45f9, { 0xaa, 0x2c, 0x53, 0xc5, 0xf2, 0x00, 0x8b, 0xdf } };
    BTH_ADDR addr;
    str2ba2(m_DeviceAddress.c_str(), &addr);

    SOCKADDR_BTH address;
    memset(&address, 0, sizeof(address));
    address.addressFamily = AF_BTH;
    address.serviceClassId = guid;
    address.btAddr = addr;
#else
    struct sockaddr_rc address = { 0 };
    address.rc_family = AF_BLUETOOTH;
    address.rc_channel = m_Channel;
    str2ba(m_DeviceAddress.c_str(), &address.rc_bdaddr);
#endif

    int result = connect(m_ClientSocket, (struct sockaddr*)&address, sizeof(address));
    if(result == 0) {
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
        Logger::writeln("Bluetooth connect failed.");
        m_UnlockState = UnlockState::CONNECT_ERROR;
    }

    m_IsRunning = false;
    SAFE_CLOSE(m_ClientSocket);
}