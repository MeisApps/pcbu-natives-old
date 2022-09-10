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
#pragma comment(lib, "IPHLPAPI.lib")
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#endif

extern "C" {
    std::regex local_v4_regex(R"((^10\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.1[6-9]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.2[0-9]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.3[0-1]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^192\.168\.[0-9]{1,3}\.[0-9]{1,3}$))");

    API void api_free(void *ptr) {
        if (ptr == nullptr)
            return;

        free(ptr);
        ptr = nullptr;
    }

    API const char *get_local_ip() {
    #ifdef _WIN32
        IP_ADAPTER_ADDRESSES* adapter_addresses(NULL);
        IP_ADAPTER_ADDRESSES* adapter(NULL);

        DWORD adapter_addresses_buffer_size = 16 * 1024;
        for (int attempts = 0; attempts != 3; ++attempts) {
            adapter_addresses = (IP_ADAPTER_ADDRESSES*)malloc(adapter_addresses_buffer_size);
            if (adapter_addresses == 0)
                return nullptr;

            DWORD error = ::GetAdaptersAddresses(
                AF_UNSPEC,
                GAA_FLAG_SKIP_ANYCAST |
                GAA_FLAG_SKIP_MULTICAST |
                GAA_FLAG_SKIP_DNS_SERVER |
                GAA_FLAG_SKIP_FRIENDLY_NAME,
                NULL,
                adapter_addresses,
                &adapter_addresses_buffer_size);

            if (ERROR_SUCCESS == error) {
                break;
            } else if (ERROR_BUFFER_OVERFLOW == error) {
                // Try again with the new size
                free(adapter_addresses);
                adapter_addresses = NULL;
                continue;
            } else {
                // Unexpected error code - log and throw
                free(adapter_addresses);
                adapter_addresses = NULL;

                return nullptr;
            }
        }

        // Iterate through all of the adapters
        for (adapter = adapter_addresses; NULL != adapter; adapter = adapter->Next) {
            if (IF_TYPE_SOFTWARE_LOOPBACK == adapter->IfType)
                continue;

            for (IP_ADAPTER_UNICAST_ADDRESS* address = adapter->FirstUnicastAddress; NULL != address; address = address->Next) {
                auto family = address->Address.lpSockaddr->sa_family;
                if (AF_INET == family) { // IPv4
                    SOCKADDR_IN* ipv4 = reinterpret_cast<SOCKADDR_IN*>(address->Address.lpSockaddr);
                    char str_buffer[INET_ADDRSTRLEN] = { 0 };
                    inet_ntop(AF_INET, &(ipv4->sin_addr), str_buffer, INET_ADDRSTRLEN);

                    auto ifAddr = std::string(str_buffer);
                    if (!std::regex_match(ifAddr, local_v4_regex))
                        continue;

                    auto addrStr = malloc(sizeof(char) * ifAddr.size() + 1);
                    if (addrStr == nullptr)
                        continue;

                    strcpy((char *)addrStr, ifAddr.c_str());

                    free(adapter_addresses);
                    adapter_addresses = NULL;
                    return (const char *)addrStr;
                } else {
                    // Skip all other types of addresses
                    continue;
                }
            }
        }

        free(adapter_addresses);
        adapter_addresses = NULL;
        return nullptr;
    #else
        struct ifaddrs* ifAddrStruct = nullptr;
        struct ifaddrs* ifa = nullptr;
        void* tmpAddrPtr = nullptr;

        getifaddrs(&ifAddrStruct);

        for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr || ifa->ifa_flags & IFF_LOOPBACK)
                continue;

            if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
                tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                char addressBuffer[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

                auto ifName = std::string(ifa->ifa_name);
                auto ifAddr = std::string(addressBuffer);
                if (Utils::StringStartsWith(ifName, "vir") || Utils::StringStartsWith(ifName, "ham"))
                    continue;
                if (!std::regex_match(ifAddr, local_v4_regex))
                    continue;

                auto addrStr = malloc(sizeof(char) * ifAddr.size() + 1);
                strcpy((char *)addrStr, ifAddr.c_str());
                return (const char *)addrStr;
            }
            else if (ifa->ifa_addr->sa_family == AF_INET6) { // IPv6
                // is a valid IP6 Address
                /*tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
                char addressBuffer[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
                printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);*/
            }
        }

        if (ifAddrStruct != nullptr)
            freeifaddrs(ifAddrStruct);

        return nullptr;
    #endif
    }

    API const char *crypt_shadow(char *pwd, char *salt) {
        return nullptr;
    }
}