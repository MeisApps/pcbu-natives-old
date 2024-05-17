#include "Utils.h"

#include <openssl/rand.h>

bool Utils::IsLittleEndian() {
    short int number = 0x1;
    char *numPtr = (char*)&number;
    return (numPtr[0] == 1);
}

int64_t Utils::GetCurrentTimeMillis() {
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
    );

    return ms.count();
}

std::string Utils::GetCurrentDateTime() {
    std::time_t rawTime;
    std::tm* timeInfo;
    char buffer[80];

    std::time(&rawTime);
    timeInfo = std::localtime(&rawTime);

    std::strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeInfo);
    return {buffer};
}

int Utils::GetRandomInt() {
    int val;
    RAND_bytes((unsigned char *)&val, sizeof(int));
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

bool Utils::StringEndsWith(std::string const &value, std::string const &ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

std::string Utils::ToHexString(const uint8_t *data, size_t data_len) {
    if(data == nullptr || data_len <= 0)
        return {};

    auto buffer = (char *)malloc(data_len * 2 + 1);
    if (buffer == nullptr)
        return {};

    auto bufIdx = 0;
    for(size_t i = 0; i < data_len; i++) {
        sprintf(buffer + bufIdx, "%02X", data[i]);
        bufIdx += 2;
    }
    buffer[bufIdx] = '\0';

    auto result = std::string(buffer, data_len * 2 + 1);
    free(buffer);
    return result;
}
