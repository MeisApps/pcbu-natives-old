#include "Utils.h"

#include <openssl/rand.h>

#ifdef _WIN32
#include <Windows.h>
#endif

bool Utils::IsLittleEndian() {
    short int number = 0x1;
    const auto numPtr = reinterpret_cast<char *>(&number);
    return numPtr[0] == 1;
}

int64_t Utils::GetCurrentTimeMillis() {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    );
    return ms.count();
}

std::string Utils::GetCurrentDateTime() {
    std::time_t rawTime;
    char buffer[80];

    std::time(&rawTime);
#ifdef _WIN32
    tm* timeInfo{};
    localtime_s(timeInfo, &rawTime);
#else
    std::tm* timeInfo = std::localtime(&rawTime);
#endif

    std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeInfo);
    return {buffer};
}

int Utils::GetRandomInt() {
    int val;
    RAND_bytes(reinterpret_cast<unsigned char *>(&val), sizeof(int));
    return val;
}

std::string Utils::GetRandomString(size_t length) {
    auto randChar = []() -> char {
        const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[ GetRandomInt() % max_index ];
    };
    std::string str(length, 0);
    std::generate_n(str.begin(), length, randChar);
    return str;
}

std::vector<std::string> Utils::SplitString(const std::string& s, char seperator) {
    std::vector<std::string> output;
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = s.find(seperator, pos)) != std::string::npos) {
        std::string substring(s.substr(prev_pos, pos - prev_pos));
        output.push_back(substring);
        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos - prev_pos));
    return output;
}

bool Utils::StringStartsWith(std::string const &value, std::string const &beginning) {
    if (beginning.size() > value.size()) return false;
    return value.rfind(beginning, 0) == 0;
}

bool Utils::StringStartsWith(std::wstring const& value, std::wstring const& beginning) {
    if (beginning.size() > value.size()) return false;
    return value.rfind(beginning, 0) == 0;
}

bool Utils::StringEndsWith(std::string const &value, std::string const &ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string Utils::ToHexString(const uint8_t *data, size_t data_len) {
    if(data == nullptr || data_len <= 0)
        return {};

    auto bufferSize = data_len * 2 + 1;
    auto buffer = static_cast<char *>(malloc(bufferSize));
    if (buffer == nullptr)
        return {};

    auto bufIdx = 0;
    for(size_t i = 0; i < data_len; i++) {
        snprintf(buffer + bufIdx, bufferSize - bufIdx, "%02X", data[i]);
        bufIdx += 2;
    }
    buffer[bufIdx] = '\0';

    auto result = std::string(buffer, data_len * 2 + 1);
    free(buffer);
    return result;
}

bool Utils::StringContains(const std::string &str, const std::string &val) {
    return str.find(val) != std::string::npos;
}

std::string Utils::ToLowerString(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return str;
}

#ifdef _WIN32
std::wstring Utils::StringToWideString(const std::string& string) {
    if (string.empty())
        return L"";
    const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, &string.at(0), static_cast<int>(string.size()), nullptr, 0);
    if (size_needed <= 0)
        return L"";
    std::wstring result(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &string.at(0), static_cast<int>(string.size()), &result.at(0), size_needed);
    return result;
}

std::string Utils::WideStringToString(const std::wstring& wide_string) {
    if (wide_string.empty())
        return "";
    const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), static_cast<int>(wide_string.size()), nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
        return "";
    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), static_cast<int>(wide_string.size()), &result.at(0), size_needed, nullptr, nullptr);
    return result;
}
#endif
