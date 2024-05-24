#include "KeyScanner.h"
#include "Logger.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef LINUX
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#include <filesystem>
namespace fs = std::filesystem;
#endif
#ifdef APPLE
#include <Carbon/Carbon.h>
#endif

KeyScanner::KeyScanner() = default;
KeyScanner::~KeyScanner() {
    Stop();
}

bool KeyScanner::GetKeyState(int key) {
#ifdef _WIN32
    return GetAsyncKeyState(key) < 0;
#endif
#ifdef LINUX
    for(auto keyMap : m_KeyMaps) {
        if(keyMap.count(key) && keyMap[key])
            return true;
    }
    return false;
#endif
#ifdef APPLE
    unsigned char keyMap[16];
    GetKeys((BigEndianUInt32*) &keyMap);
    return (0 != ((keyMap[key >> 3] >> (key & 7)) & 1));
#endif
}

std::map<int, bool> KeyScanner::GetAllKeys() {
#ifdef _WIN32
    auto map = std::map<int, bool>();
    for (int i = 0; i < 0xA6; i++) {
        map[i] = GetKeyState(i);
    }
    return map;
#endif
#ifdef LINUX
    return m_KeyMaps[0]; // ToDo
#endif
#ifdef APPLE
    auto map = std::map<int, bool>();
    for (int i = 0; i < 0x7E; i++) {
        map[i] = GetKeyState(i);
    }
    return map;
#endif
}

void KeyScanner::Start() {
#ifdef LINUX
    if (m_IsRunning)
        return;

    m_IsRunning = true;
    m_ScanThread = std::thread(&KeyScanner::ScanThread, this);
#endif
}

void KeyScanner::Stop() {
#ifdef LINUX
    if(!m_IsRunning)
        return;

    m_IsRunning = false;
    if(m_ScanThread.joinable())
        m_ScanThread.join();
#endif
}

#ifdef LINUX
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
        Logger::WriteLn("Warning: No keyboards found.");
        return;
    }

    while (m_IsRunning) {
        int idx{};
        for(auto kbd : kbds) {
            input_event ie{};
            if(read(kbd, &ie, sizeof(ie)) == sizeof(ie))
                m_KeyMaps[idx][ie.code] = ie.value != 0;
            idx++;
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
        if(Utils::StringEndsWith(entry.path().filename().string(), "0-event-kbd"))
            keyboards.push_back(entry.path().string());
    }
    return keyboards;
}
#endif
