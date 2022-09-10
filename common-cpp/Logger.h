#ifndef PAM_PCBIOUNLOCK_LOGGER_H
#define PAM_PCBIOUNLOCK_LOGGER_H

#include <string>
#include <fstream>
#include <filesystem>

#include "utils/Utils.h"

#ifdef _WIN32
#define LOG_FILE "C:/ProgramData/PCBioUnlock/module.log"
#else
#define LOG_FILE "/etc/pc-bio-unlock/module.log"
#endif

class Logger {
public:
    static void init() {
        std::ifstream file(LOG_FILE, std::ifstream::ate | std::ifstream::binary);
        if(file) {
            auto sizeKb = file.tellg() / 1000;
            if(sizeKb > 1000) {
                std::filesystem::remove(LOG_FILE);
            }
        }

        writeln("[Start of Log ", Utils::GetCurrentDateTime(), "]");
    }

    // Write
    static void write(const std::string& message) {
        std::ofstream file;
        file.open(LOG_FILE, std::ios::app);
        if(!file) {
            printf("Error: Could not open log file.\n");
            return;
        }

        file << message;
    }

    template<typename ...T>
    static void write(const std::string& arg, T... args) {
        write(arg);
        write(args...);
    }

    static void writeln(const std::string& message) {
        write(message + "\n");
    }

    template<typename ...T>
    static void writeln(const std::string& arg, T... args) {
        write(arg, args..., "\n");
    }

    // Print
    static void print(const std::string& message) {
        printf("%s", message.c_str());
        write(message);
    }

    template<typename ...T>
    static void print(const std::string& arg, T... args) {
        print(arg);
        print(args...);
    }

    static void println(const std::string& message) {
        print(message + "\n");
    }

    template<typename ...T>
    static void println(const std::string& arg, T... args) {
        print(arg, args..., "\n");
    }
private:
    Logger() = default;
};

#endif //PAM_PCBIOUNLOCK_LOGGER_H
