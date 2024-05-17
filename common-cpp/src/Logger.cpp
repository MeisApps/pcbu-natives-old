#include "Logger.h"

#include <spdlog/sinks/basic_file_sink.h>

void Logger::init() {
    std::ifstream file(LOG_FILE, std::ifstream::ate | std::ifstream::binary);
    if(file) {
        auto sizeKb = file.tellg() / 1000;
        if(sizeKb > 1000) {
            std::filesystem::remove(LOG_FILE);
        }
    }

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(LOG_FILE, false);
    auto fileLogger = std::make_shared<spdlog::logger>("file_logger", fileSink);
    fileLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
    spdlog::set_default_logger(fileLogger);
    writeln("Module Init");
}

void Logger::writeln(const std::string& message) {
    spdlog::info(message);
}

void Logger::println(const std::string& message) {
    printf("%s\n", message.c_str());
    writeln(message);
}
