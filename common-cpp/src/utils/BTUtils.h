#ifndef PAM_PCBIOUNLOCK_BTUTILS_H
#define PAM_PCBIOUNLOCK_BTUTILS_H

#include <string>
#include <cstdint>

class BTUtils {
public:
#ifdef LINUX
    static int FindChannelSDP(const std::string& deviceAddress, uint8_t *uuid);
#endif
#ifdef _WIN32
    int str2ba2(const char* straddr, BTH_ADDR* btaddr);
#endif

private:
    BTUtils() = default;
};

#endif //PAM_PCBIOUNLOCK_BTUTILS_H
