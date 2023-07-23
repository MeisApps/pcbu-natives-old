#include "bt_api.h"
#include <cstdlib>

#ifdef _WIN32

#include <iostream>
#include <vector>

#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>

#include <locale>
#include <codecvt>

#pragma comment(lib, "Bthprops.lib")

#define DEVICE_LIMIT 20

#else

#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#endif

#ifdef _WIN32
int str2ba(const char* straddr, BTH_ADDR* btaddr)
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

int ba2str(const BTH_ADDR btaddr, char* straddr)
{
    unsigned char bytes[6];
    for (int i = 0; i < 6; i++) {
        bytes[5 - i] = (unsigned char)((btaddr >> (i * 8)) & 0xff);
    }

    if (std::sprintf(straddr, "%02X:%02X:%02X:%02X:%02X:%02X",
        bytes[0], bytes[1], bytes[2],
        bytes[3], bytes[4], bytes[5]) != 6)
        return 1;
    return 0;
}
#endif // _WIN32


extern "C" {
    API bool bt_is_available() {
#ifdef _WIN32
        BLUETOOTH_FIND_RADIO_PARAMS params;
        ZeroMemory(&params, sizeof(params));
        params.dwSize = sizeof(params);

        HANDLE hRadio = nullptr;
        HANDLE result = BluetoothFindFirstRadio(&params, &hRadio);
        if (result) {
            CloseHandle(hRadio);
        }
        return result;
#else
        int dev_id = hci_get_route(nullptr);
        int sock = hci_open_dev(dev_id);
        if (dev_id < 0 || sock < 0) {
            return false;
        }
        close(sock);
        return true;
#endif
    }

    API BluetoothDevice *bt_scan_devices(int *count) {
        if(count == nullptr)
            return nullptr;
        *count = 0;

#ifdef _WIN32
        WSADATA data;
        int result;
        result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
            return nullptr;
        }

        WSAQUERYSETW queryset;
        memset(&queryset, 0, sizeof(WSAQUERYSETW));
        queryset.dwSize = sizeof(WSAQUERYSETW);
        queryset.dwNameSpace = NS_BTH;

        HANDLE hLookup;
        result = WSALookupServiceBeginW(&queryset, LUP_RETURN_NAME | LUP_CONTAINERS | LUP_RETURN_ADDR | LUP_FLUSHCACHE | LUP_RETURN_TYPE | LUP_RETURN_BLOB | LUP_RES_SERVICE, &hLookup);
        if (result != 0) {
            return nullptr;
        }

        BYTE buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        DWORD bufferLength = sizeof(buffer);
        WSAQUERYSETW* pResults = (WSAQUERYSETW*)&buffer;

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        auto devicesPtr = (BluetoothDevice*)malloc(sizeof(BluetoothDevice) * DEVICE_LIMIT);
        if (devicesPtr == nullptr) {
            WSALookupServiceEnd(hLookup);
            return nullptr;
        }
        memset(devicesPtr, 0, sizeof(BluetoothDevice) * DEVICE_LIMIT);

        int idx = 0;
        while (result == 0) {
            if (idx >= DEVICE_LIMIT)
                break;

            result = WSALookupServiceNextW(hLookup, LUP_RETURN_NAME | LUP_CONTAINERS | LUP_RETURN_ADDR | LUP_FLUSHCACHE | LUP_RETURN_TYPE | LUP_RETURN_BLOB | LUP_RES_SERVICE, &bufferLength, pResults);
            if (result == 0) {
                auto pBtAddr = (SOCKADDR_BTH*)(pResults->lpcsaBuffer->RemoteAddr.lpSockaddr);
                char addr[19] = { 0 };
                ba2str(pBtAddr->btAddr, addr);

                auto name = converter.to_bytes(pResults->lpszServiceInstanceName);
                auto address = std::string(addr);

                strcpy_s(devicesPtr[idx].name, 255, name.c_str());
                strcpy_s(devicesPtr[idx].address, 19, address.c_str());
                idx++;
            }
        }

        *count = idx;
        WSALookupServiceEnd(hLookup);
        return devicesPtr;
#else
        inquiry_info* ii = nullptr;
        int max_rsp, num_rsp;
        int dev_id, sock, len, flags;
        int i;
        char addr[19] = { 0 };
        char name[248] = { 0 };

        dev_id = hci_get_route(nullptr);
        sock = hci_open_dev(dev_id);
        if (dev_id < 0 || sock < 0) {
            return nullptr;
        }

        len = 8;
        max_rsp = 255;
        flags = IREQ_CACHE_FLUSH;
        ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

        num_rsp = hci_inquiry(dev_id, len, max_rsp, nullptr, &ii, flags);
        if (num_rsp < 0) {
            free(ii);
            close(sock);
            return nullptr;
        }

        *count = num_rsp;
        if (num_rsp == 0) {
            free(ii);
            close(sock);
            return nullptr;
        }

        auto devices = (BluetoothDevice*)malloc(sizeof(BluetoothDevice) * num_rsp);
        memset(devices, 0, sizeof(BluetoothDevice) * num_rsp);
        for (i = 0; i < num_rsp; i++) {
            ba2str(&(ii + i)->bdaddr, addr);
            memset(name, 0, sizeof(name));
            if (hci_read_remote_name(sock, &(ii + i)->bdaddr, sizeof(name),
                name, 0) < 0)
                strcpy(name, "[unknown]");

            strcpy(devices[i].name, name);
            strcpy(devices[i].address, addr);
        }

        free(ii);
        close(sock);
        return devices;
#endif
    }
}