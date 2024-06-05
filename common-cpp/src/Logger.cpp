#include "Logger.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

void Logger::Init() {
    std::ifstream file(LOG_FILE, std::ifstream::ate | std::ifstream::binary);
    if(file) {
        auto sizeKb = file.tellg() / 1000;
        if(sizeKb > 2000)
            std::filesystem::remove(LOG_FILE);
    }

    try {
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(LOG_FILE, false);
        auto fileLogger = std::make_shared<spdlog::logger>("file_logger", fileSink);
        fileLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] %v");
        spdlog::set_default_logger(fileLogger);
#ifdef _DEBUG
        spdlog::flush_on(spdlog::level::info);
#endif
        WriteLn("Module Init");
    } catch(const std::exception& ex) {
        PrintLn("Error initializing logger: {}", ex.what());
    }
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
