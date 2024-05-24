#ifndef PAM_PCBIOUNLOCK_BTUTILS_H
#define PAM_PCBIOUNLOCK_BTUTILS_H

#include <string>
#include <cstdint>

#ifdef _WIN32
#include <WinSock2.h>
#include <ws2bth.h>
#endif

class BTUtils {
public:
#ifdef LINUX
    static int FindChannelSDP(const std::string& deviceAddress, uint8_t *uuid);
#endif
#ifdef _WIN32
    static int str2ba(const char* straddr, BTH_ADDR* btaddr);
    static int ba2str(BTH_ADDR btaddr, char* straddr);
#endif

private:
    BTUtils() = default;
};

#endif //PAM_PCBIOUNLOCK_BTUTILS_H
