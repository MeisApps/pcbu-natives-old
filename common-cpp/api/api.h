#ifndef PAM_PCBIOUNLOCK_API_H
#define PAM_PCBIOUNLOCK_API_H

#ifdef _WIN32
#define API __declspec(dllexport)
#else
#define API __attribute__((visibility("default")))
#endif

extern "C" {
    struct IpAndMac {
        char ipAddr[16]{};
        char macAddr[18]{};
    };

    API void api_free(void *ptr);

    API IpAndMac *get_local_ip_and_mac();
    API const char *crypt_shadow(char *pwd, char *salt);
}

#endif //PAM_PCBIOUNLOCK_API_H
