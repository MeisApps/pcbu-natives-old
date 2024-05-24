#ifndef PAM_PCBIOUNLOCK_UTILS_H
#define PAM_PCBIOUNLOCK_UTILS_H

#define SAFE_FREE(x) if(x != nullptr) free(x); x = nullptr;

#include <chrono>
#include <algorithm>

#include <string>
#include <vector>
#include <cstdio>

class Utils {
public:
    static bool IsLittleEndian();

    static int64_t GetCurrentTimeMillis();
    static std::string GetCurrentDateTime();

    static int GetRandomInt();
    static std::string GetRandomString(size_t length);

    static std::vector<std::string> SplitString(const std::string& s, char seperator);
    static bool StringStartsWith(std::string const& value, std::string const& beginning);
    static bool StringStartsWith(std::wstring const& value, std::wstring const& beginning);
    static bool StringEndsWith(std::string const &value, std::string const &ending);
    static std::string ToHexString(const uint8_t *data, size_t data_len);

#ifdef _WIN32
    static std::wstring StringToWideString(const std::string& string);
    static std::string WideStringToString(const std::wstring& wide_string);
#endif

private:
    Utils() = default;
};

#endif //PAM_PCBIOUNLOCK_UTILS_H
