#include "api.h"
#include "../utils/Utils.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <regex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#ifdef LINUX
#include <netpacket/packet.h>
#endif
#endif

extern "C" {
    std::regex local_v4_regex(R"((^10\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.1[6-9]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.2[0-9]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.3[0-1]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^192\.168\.[0-9]{1,3}\.[0-9]{1,3}$))");

    API void api_free(void *ptr) {
        if (ptr == nullptr)
            return;
        free(ptr);
    }

    API IpAndMac *get_local_ip_and_mac() {
    #ifdef _WIN32
        IP_ADAPTER_ADDRESSES* adapter_addresses(nullptr);
        IP_ADAPTER_ADDRESSES* adapter(nullptr);
        std::vector<std::wstring> filterAdapterNames = { L"vEthernet (WSL", L"VMware Network Adapter", L"VirtualBox" };

        DWORD adapter_addresses_buffer_size = 16 * 1024;
        for (int attempts = 0; attempts != 3; ++attempts) {
            adapter_addresses = static_cast<IP_ADAPTER_ADDRESSES *>(malloc(adapter_addresses_buffer_size));
            if (adapter_addresses == nullptr)
                return nullptr;

            DWORD error = ::GetAdaptersAddresses(
                AF_UNSPEC,
                GAA_FLAG_SKIP_ANYCAST |
                GAA_FLAG_SKIP_MULTICAST |
                GAA_FLAG_SKIP_DNS_SERVER |
                GAA_FLAG_SKIP_FRIENDLY_NAME,
                nullptr,
                adapter_addresses,
                &adapter_addresses_buffer_size);
            if (ERROR_SUCCESS == error) {
                break;
            } else if (ERROR_BUFFER_OVERFLOW == error) {
                free(adapter_addresses);
                adapter_addresses = nullptr;
                continue;
            } else {
                free(adapter_addresses);
                adapter_addresses = nullptr;
                return nullptr;
            }
        }

        for (adapter = adapter_addresses; nullptr != adapter; adapter = adapter->Next) {
            if (IF_TYPE_SOFTWARE_LOOPBACK == adapter->IfType)
                continue;

            auto ifName = std::wstring(adapter->FriendlyName);
            for (const auto& filterStart : filterAdapterNames)
                if (Utils::StringStartsWith(ifName, filterStart))
                    goto adapterEnd;

            for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; nullptr != address; address = address->Next) {
                auto family = address->Address.lpSockaddr->sa_family;
                if (AF_INET == family) { // IPv4
                    auto ipv4 = reinterpret_cast<SOCKADDR_IN *>(address->Address.lpSockaddr);
                    char str_buffer[INET_ADDRSTRLEN] = { 0 };
                    inet_ntop(AF_INET, &(ipv4->sin_addr), str_buffer, INET_ADDRSTRLEN);

                    auto ifAddr = std::string(str_buffer);
                    if (!std::regex_match(ifAddr, local_v4_regex))
                        continue;

                    auto data = static_cast<IpAndMac *>(malloc(sizeof(IpAndMac)));
                    if (data == nullptr)
                        goto end;
                    strncpy_s(data->ipAddr, ifAddr.c_str(), sizeof(data->ipAddr));
                    snprintf(data->macAddr, sizeof(data->macAddr),
                        "%02X:%02X:%02X:%02X:%02X:%02X",
                        adapter->PhysicalAddress[0], adapter->PhysicalAddress[1],
                        adapter->PhysicalAddress[2], adapter->PhysicalAddress[3],
                        adapter->PhysicalAddress[4], adapter->PhysicalAddress[5]);

                    free(adapter_addresses);
                    adapter_addresses = nullptr;
                    return data;
                }
            }
            adapterEnd:
            continue;
        }

        end:
        free(adapter_addresses);
        adapter_addresses = nullptr;
        return nullptr;
    #else
        struct ifaddrs* ifAddrStruct = nullptr;
        getifaddrs(&ifAddrStruct);

        for (auto ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr || ifa->ifa_flags & IFF_LOOPBACK)
                continue;

            if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
                void *tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

                auto ifName = std::string(ifa->ifa_name);
                auto ifAddr = std::string(addressBuffer);
                if (Utils::StringStartsWith(ifName, "vir") || Utils::StringStartsWith(ifName, "ham"))
                    continue;
                if (!std::regex_match(ifAddr, local_v4_regex))
                    continue;

                auto data = (IpAndMac *)malloc(sizeof(IpAndMac));
                if (data == nullptr)
                    goto end;
                strncpy(data->ipAddr, ifAddr.c_str(), sizeof(data->ipAddr));

#ifdef LINUX
                for (auto ifa2 = ifAddrStruct; ifa2 != nullptr; ifa2 = ifa2->ifa_next) {
                    if (ifa2->ifa_addr && ifa2->ifa_addr->sa_family == AF_PACKET && strcmp(ifa->ifa_name, ifa2->ifa_name) == 0) {
                        auto sll = reinterpret_cast<struct sockaddr_ll*>(ifa2->ifa_addr);
                        snprintf(data->macAddr, sizeof(data->macAddr),
                            "%02X:%02X:%02X:%02X:%02X:%02X",
                            sll->sll_addr[0], sll->sll_addr[1],
                            sll->sll_addr[2], sll->sll_addr[3],
                            sll->sll_addr[4], sll->sll_addr[5]);
                        break;
                    }
                }
#elif APPLE
#warning Not implemented on Apple.
#endif

                freeifaddrs(ifAddrStruct);
                return data;
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6) {} // IPv6
        }

        end:
        if (ifAddrStruct != nullptr)
            freeifaddrs(ifAddrStruct);
        return nullptr;
    #endif
    }

    API const char *crypt_shadow(char *pwd, char *salt) {
        return nullptr;
    }
}
