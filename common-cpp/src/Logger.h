#ifndef PAM_PCBIOUNLOCK_LOGGER_H
#define PAM_PCBIOUNLOCK_LOGGER_H

#include <string>
#include <fstream>
#include <filesystem>

#include "spdlog/spdlog.h"
#include "utils/Utils.h"

#ifdef _WIN32
#define LOG_FILE "C:/ProgramData/PCBioUnlock/module.log"
#else
#define LOG_FILE "/etc/pc-bio-unlock/module.log"
#endif

class Logger {
public:
    static void init();

    static void writeln(const std::string& message);
    template<typename ...T>
    static void writeln(const std::string& arg, T&&... args) {
        writeln(fmt::format(arg, args...));
    }

    static void println(const std::string& message);
    template<typename ...T>
    static void println(const std::string& arg, T&&... args) {
        println(fmt::format(arg, args...));
    }
private:
    Logger() = default;
};

#endif //PAM_PCBIOUNLOCK_LOGGER_H
