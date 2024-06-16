#include "api.h"
#include "../utils/Utils.h"
#include "Logger.h"

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
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#ifdef LINUX
#include <netpacket/packet.h>
#endif
#endif

extern "C" {
    struct NetworkInterface {
        std::string ifName{};
        std::string ipAddress{};
        std::string macAddress{};
    };
    std::regex local_v4_regex(R"((^10\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.1[6-9]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.2[0-9]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^172\.3[0-1]{1}[0-9]{0,1}\.[0-9]{1,3}\.[0-9]{1,3}$)|(^192\.168\.[0-9]{1,3}\.[0-9]{1,3}$))");

    API void api_free(void *ptr) {
        if (ptr == nullptr)
            return;
        free(ptr);
    }

    API IpAndMac *get_local_ip_and_mac() {
        std::vector<NetworkInterface> result{};
#ifdef WINDOWS
        ULONG bufferSize = 0;
        GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, nullptr, &bufferSize);
        std::vector<BYTE> buffer(bufferSize);
        auto adapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        if (GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapterAddresses, &bufferSize))
            return {};

        for (PIP_ADAPTER_ADDRESSES adapter = adapterAddresses; adapter; adapter = adapter->Next) {
            if (adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK)
                continue;

            char macBuffer[18]{};
            snprintf(macBuffer, sizeof(macBuffer),
                     "%02X:%02X:%02X:%02X:%02X:%02X",
                     adapter->PhysicalAddress[0], adapter->PhysicalAddress[1],
                     adapter->PhysicalAddress[2], adapter->PhysicalAddress[3],
                     adapter->PhysicalAddress[4], adapter->PhysicalAddress[5]);
            auto ifName = std::wstring(adapter->FriendlyName);
            auto macAddr = std::string(macBuffer);

            auto netIf = NetworkInterface();
            netIf.ifName = StringUtils::FromWideString(ifName);
            netIf.macAddress = macAddr;
            for (PIP_ADAPTER_UNICAST_ADDRESS unicast = adapter->FirstUnicastAddress; unicast; unicast = unicast->Next) {
                auto family = unicast->Address.lpSockaddr->sa_family;
                if (family != AF_INET)
                    continue;
                auto ipv4 = reinterpret_cast<SOCKADDR_IN *>(unicast->Address.lpSockaddr);
                char strBuffer[INET_ADDRSTRLEN]{};
                inet_ntop(AF_INET, &(ipv4->sin_addr), strBuffer, INET_ADDRSTRLEN);
                auto ifAddr = std::string(strBuffer);
                netIf.ipAddress = ifAddr;
            }
            if(!netIf.ipAddress.empty())
                result.emplace_back(netIf);
        }
#elif defined(LINUX) || defined(APPLE)
        struct ifaddrs *ifaddr{};
        if(getifaddrs(&ifaddr))
            return {};

        std::map<std::string, NetworkInterface> ifMap{};
        for(auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
            if(ifa->ifa_addr == nullptr || ifa->ifa_flags & IFF_LOOPBACK)
                continue;
            int family = ifa->ifa_addr->sa_family;
            if(family == AF_INET || family == AF_PACKET) {
                char addr[NI_MAXHOST]{};
                auto res = getnameinfo(ifa->ifa_addr, (family == AF_INET) ?
                                                      sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
                                       addr, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
                if(res && family != AF_PACKET) {
                    printf("getnameinfo() failed: %s\n", gai_strerror(res));
                    continue;
                }

                auto ifName = ifa->ifa_name;
                if(ifMap.find(ifName) == ifMap.end()) {
                    ifMap[ifName] = NetworkInterface();
                    ifMap[ifName].ifName = ifName;
                }
                if(family == AF_INET)
                    ifMap[ifName].ipAddress = addr;
                if(family == AF_PACKET) {
#ifdef LINUX
                    auto sll = reinterpret_cast<struct sockaddr_ll*>(ifa->ifa_addr);
                    snprintf(addr, sizeof(addr),
                             "%02X:%02X:%02X:%02X:%02X:%02X",
                             sll->sll_addr[0], sll->sll_addr[1],
                             sll->sll_addr[2], sll->sll_addr[3],
                             sll->sll_addr[4], sll->sll_addr[5]);
#endif
                    ifMap[ifName].macAddress = addr;
                }

            }
        }

        for(const auto& pair : ifMap)
            if(!pair.second.ipAddress.empty())
                result.emplace_back(pair.second);
        freeifaddrs(ifaddr);
#endif

        auto rankIp = [](const NetworkInterface& netIf){
            auto rank = 0;
            if(std::regex_match(netIf.ipAddress, local_v4_regex))
                rank += 1000;
            if(Utils::StringStartsWith(netIf.ipAddress, "192.168.") || Utils::StringStartsWith(netIf.ipAddress, "10."))
                rank += 50;
#ifdef WINDOWS
            if(Utils::StringContains(netIf.ifName, "Bluetooth") || Utils::StringContains(netIf.ifName, "vEthernet") || Utils::StringContains(netIf.ifName, "VMware") || Utils::StringContains(netIf.ifName, "VirtualBox") || Utils::StringContains(netIf.ifName, "Docker"))
            rank -= 100;
#else
            if(Utils::StringStartsWith(netIf.ifName, "vir") || Utils::StringStartsWith(netIf.ifName, "docker") || Utils::StringStartsWith(netIf.ifName, "ham"))
                rank -= 100;
#endif
            return rank;
        };
        std::sort(result.begin(), result.end(), [rankIp](const NetworkInterface& a, const NetworkInterface& b) {
            auto rankA = rankIp(a);
            auto rankB = rankIp(b);
            if(rankA == rankB)
                return Utils::ToLowerString(a.ifName) < Utils::ToLowerString(b.ifName);
            return rankA > rankB;
        });
        for(auto netIf : result)
            Logger::WriteLn("Found network interface: Name={} IP={} Rank={}", netIf.ifName, netIf.ipAddress, rankIp(netIf));
        auto data = (IpAndMac *)malloc(sizeof(IpAndMac));
        if(data == nullptr || result.empty())
            return nullptr;
        auto resultIf = result[0];
        std::memset(data, 0, sizeof(IpAndMac));
        std::memcpy(data->ipAddr, resultIf.ipAddress.c_str(), std::min(sizeof(data->ipAddr), resultIf.ipAddress.size()));
        if(!resultIf.macAddress.empty()) {
            std::memcpy(data->macAddr, resultIf.macAddress.c_str(), std::min(sizeof(data->macAddr), resultIf.macAddress.size()));
        }
        return data;
    }

    API const char *crypt_shadow(char *pwd, char *salt) {
        return nullptr;
    }
}
