#ifndef PAM_PCBIOUNLOCK_LOGGER_H
#define PAM_PCBIOUNLOCK_LOGGER_H

#include <string>
#include <fstream>
#include <filesystem>

#include <spdlog/fmt/fmt.h>
#include "utils/Utils.h"

#ifdef _WIN32
#define LOG_FILE "C:/ProgramData/PCBioUnlock/module.log"
#else
#define LOG_FILE "/etc/pc-bio-unlock/module.log"
#endif

class Logger {
public:
    static void Init();
    static void Destroy();

    static void WriteLn(const std::string& message);
    template<typename ...T>
    static void WriteLn(const std::string& arg, T&&... args) {
        WriteLn(fmt::format(arg, args...));
    }

    static void PrintLn(const std::string& message);
    template<typename ...T>
    static void PrintLn(const std::string& arg, T&&... args) {
        PrintLn(fmt::format(arg, args...));
    }
private:
    Logger() = default;
};

#endif //PAM_PCBIOUNLOCK_LOGGER_H
