#include "bt_api.h"

#include <cstdlib>
#include <vector>

#include "Logger.h"
#include "utils/BTUtils.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2bth.h>
#include <bluetoothapis.h>
#define DEVICE_LIMIT 20
#endif
#ifdef LINUX
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#endif

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
#endif
#ifdef LINUX
        int dev_id = hci_get_route(nullptr);
        int sock = hci_open_dev(dev_id);
        if (dev_id < 0 || sock < 0) {
            return false;
        }
        close(sock);
        return true;
#endif
#ifdef APPLE
#warning Not implemented on Apple.
        return false;
#endif
    }

    API BluetoothDevice *bt_scan_devices(int *count) {
        if(count == nullptr)
            return nullptr;
        *count = 0;
#ifdef _WIN32
        WSADATA data;
        int result = WSAStartup(MAKEWORD(2, 2), &data);
        if (result != 0) {
            return nullptr;
        }

        WSAQUERYSETW queryset{};
        queryset.dwSize = sizeof(WSAQUERYSETW);
        queryset.dwNameSpace = NS_BTH;

        HANDLE hLookup;
        result = WSALookupServiceBeginW(&queryset, LUP_RETURN_NAME | LUP_CONTAINERS | LUP_RETURN_ADDR | LUP_FLUSHCACHE | LUP_RETURN_TYPE | LUP_RETURN_BLOB | LUP_RES_SERVICE, &hLookup);
        if (result != 0) {
            return nullptr;
        }

        BYTE buffer[4096]{};
        DWORD bufferLength = sizeof(buffer);
        auto pResults = reinterpret_cast<WSAQUERYSETW*>(&buffer);

        auto devicesPtr = static_cast<BluetoothDevice*>(malloc(sizeof(BluetoothDevice) * DEVICE_LIMIT));
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
                auto pBtAddr = reinterpret_cast<SOCKADDR_BTH*>(pResults->lpcsaBuffer->RemoteAddr.lpSockaddr);
                char addr[18] = { 0 };
                BTUtils::ba2str(pBtAddr->btAddr, addr);

                auto name = Utils::WideStringToString(pResults->lpszServiceInstanceName);
                auto address = std::string(addr);

                strcpy_s(devicesPtr[idx].name, 255, name.c_str());
                strcpy_s(devicesPtr[idx].address, 18, address.c_str());
                idx++;
            }
        }

        *count = idx;
        WSALookupServiceEnd(hLookup);
        return devicesPtr;
#endif
#ifdef LINUX
        inquiry_info* ii = nullptr;
        int max_rsp, num_rsp;
        int dev_id, sock, len, flags;
        int i;
        char addr[18] = { 0 };
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
#ifdef APPLE
#warning Not implemented on Apple.
        return nullptr;
#endif
    }

    API BluetoothDevice *bt_get_paired_devices(int *count) {
        if(count == nullptr)
            return nullptr;
        *count = 0;
#ifdef _WIN32
        auto devicesPtr = static_cast<BluetoothDevice*>(malloc(sizeof(BluetoothDevice) * DEVICE_LIMIT));
        if(devicesPtr == nullptr)
            return nullptr;
        memset(devicesPtr, 0, sizeof(BluetoothDevice) * DEVICE_LIMIT);

        BLUETOOTH_DEVICE_INFO_STRUCT deviceInfo;
        deviceInfo.dwSize = sizeof(BLUETOOTH_DEVICE_INFO_STRUCT);
        BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = {
                sizeof(searchParams),
                TRUE,
                TRUE,
                FALSE,
                FALSE,
                FALSE,
                5,
                nullptr
        };
        HANDLE hFind = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
        if(hFind == nullptr) {
            return nullptr;
        }

        int idx{};
        do {
            char addr[18] = { 0 };
            snprintf(addr, sizeof(addr), "%02x:%02x:%02x:%02x:%02x:%02x",
                     deviceInfo.Address.rgBytes[5],
                     deviceInfo.Address.rgBytes[4],
                     deviceInfo.Address.rgBytes[3],
                     deviceInfo.Address.rgBytes[2],
                     deviceInfo.Address.rgBytes[1],
                     deviceInfo.Address.rgBytes[0]);
            auto name = Utils::WideStringToString(deviceInfo.szName);
            auto address = std::string(addr);

            strcpy_s(devicesPtr[idx].name, 255, name.c_str());
            strcpy_s(devicesPtr[idx].address, 18, address.c_str());
            idx++;
        } while (BluetoothFindNextDevice(hFind, &deviceInfo));
        BluetoothFindDeviceClose(hFind);

        *count = idx;
        return devicesPtr;
#else
        return nullptr;
#endif
    }

    API bool bt_pair_device(BluetoothDevice *device) {
#ifdef _WIN32
        BLUETOOTH_DEVICE_INFO deviceInfo = { sizeof(deviceInfo) };
        BLUETOOTH_ADDRESS deviceAddress;
        sscanf_s(device->address, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                 &deviceAddress.rgBytes[5], &deviceAddress.rgBytes[4], &deviceAddress.rgBytes[3],
                 &deviceAddress.rgBytes[2], &deviceAddress.rgBytes[1], &deviceAddress.rgBytes[0]);

        Logger::PrintLn("Scanning for pairing device...");
        BLUETOOTH_DEVICE_SEARCH_PARAMS searchParams = {
                sizeof(searchParams),
                TRUE,
                FALSE,
                TRUE,
                TRUE,
                TRUE,
                5,
                nullptr
        };
        HANDLE searchHandle = BluetoothFindFirstDevice(&searchParams, &deviceInfo);
        if (!searchHandle) {
            Logger::PrintLn("Error: Bluetooth device search failed.");
            return false;
        }
        bool deviceFound = false;
        do {
            if (memcmp(&deviceInfo.Address, &deviceAddress, sizeof(deviceAddress)) == 0) {
                deviceFound = true;
                break;
            }
        } while (BluetoothFindNextDevice(searchHandle, &deviceInfo));
        BluetoothFindDeviceClose(searchHandle);
        if (!deviceFound) {
            Logger::PrintLn("Error: Device with address {} not found.", device->address);
            return false;
        }

        Logger::PrintLn("Starting pairing...");
        HWND hwnd = FindWindowA(nullptr, "PC Bio Unlock");
        DWORD result = BluetoothAuthenticateDevice(hwnd, nullptr, &deviceInfo, nullptr, 0);
        if (result != ERROR_SUCCESS && result != ERROR_NO_MORE_ITEMS) {
            Logger::PrintLn("Error: Device pairing failed with error code {}.", result);
            return false;
        }

        Logger::PrintLn("Device paired successfully.");
        return true;
#else
        return true;
#endif
    }
}
