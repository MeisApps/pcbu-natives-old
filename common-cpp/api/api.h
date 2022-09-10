#ifndef PAM_PCBIOUNLOCK_API_H
#define PAM_PCBIOUNLOCK_API_H

#ifdef _WIN32
#define API __declspec(dllexport)
#else
#define API __attribute__((visibility("default")))
#endif

extern "C" {
    API void api_free(void *ptr);

    API const char *get_local_ip();
    API const char *crypt_shadow(char *pwd, char *salt);
}

#endif //PAM_PCBIOUNLOCK_API_H
