#include "KeyScanner.h"
#include "Logger.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#include <filesystem>
namespace fs = std::filesystem;
#endif

KeyScanner::KeyScanner() = default;
KeyScanner::~KeyScanner() {
    Stop();
}

bool KeyScanner::GetKeyState(int key) {
#ifdef _WIN32
    return GetAsyncKeyState(key) < 0;
#else
    if (!m_KeyMap.count(key))
        return false;
    return m_KeyMap[key];
#endif
}

std::map<int, bool> KeyScanner::GetAllKeys() {
#ifdef _WIN32
    auto map = std::map<int, bool>();
    for (int i = 0; i < 0xA6; i++) {
        map[i] = GetKeyState(i);
    }
    return map;
#else
    return m_KeyMap;
#endif
}

void KeyScanner::Start() {
#ifndef _WIN32
    if (m_IsRunning)
        return;

    m_IsRunning = true;
    m_ScanThread = std::thread(&KeyScanner::ScanThread, this);
#endif
}

void KeyScanner::Stop() {
#ifndef _WIN32
    if(!m_IsRunning)
        return;

    m_IsRunning = false;
    if(m_ScanThread.joinable())
        m_ScanThread.join();
#endif
}

#ifndef _WIN32
void KeyScanner::ScanThread() {
    auto kbds = std::vector<int>();
    for(const auto& keyboard : GetKeyboards()) {
        int kbd = open(keyboard.c_str(), O_RDONLY);
        if(kbd == -1)
            continue;

        int flags = fcntl(kbd, F_GETFL, 0);
        fcntl(kbd, F_SETFL, flags | O_NONBLOCK);

        kbds.push_back(kbd);
    }

    if(kbds.empty()) {
        Logger::writeln("Warning: No keyboards found.");
        return;
    }

    while (m_IsRunning) {
        m_KeyMap.clear();
        for(auto kbd : kbds) {
            input_event ie{};
            if(read(kbd, &ie, sizeof(ie)) > 0) {
                m_KeyMap[ie.code] |= ie.value != 0;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for(auto kbd : kbds) {
        close(kbd);
    }
}

std::vector<std::string> KeyScanner::GetKeyboards() {
    auto keyboards = std::vector<std::string>();
    for (const auto & entry : fs::directory_iterator("/dev/input/by-path/")) {
        if(Utils::StringEndsWith(entry.path().filename().string(), "event-kbd"))
            keyboards.push_back(entry.path().string());
    }
    return keyboards;
}
#endif
