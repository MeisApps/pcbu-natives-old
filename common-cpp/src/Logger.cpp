#include "Logger.h"

#include <spdlog/sinks/basic_file_sink.h>

void Logger::Init() {
    std::ifstream file(LOG_FILE, std::ifstream::ate | std::ifstream::binary);
    if(file) {
        auto sizeKb = file.tellg() / 1000;
        if(sizeKb > 2000)
            std::filesystem::remove(LOG_FILE);
    }

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(LOG_FILE, false);
    auto fileLogger = std::make_shared<spdlog::logger>("file_logger", fileSink);
    fileLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
    spdlog::set_default_logger(fileLogger);
    WriteLn("Module Init");
}

void Logger::Destroy() {
    WriteLn("Module Destroy");
    spdlog::shutdown();
}

void Logger::WriteLn(const std::string& message) {
    spdlog::info(message);
}

void Logger::PrintLn(const std::string& message) {
    printf("%s\n", message.c_str());
    WriteLn(message);
}
