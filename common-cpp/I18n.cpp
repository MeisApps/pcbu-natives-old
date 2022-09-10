#include "I18n.h"
#include "AppStorage.h"

#include <filesystem>

#ifdef _WIN32
#pragma warning(disable : 4244)
#include <Windows.h>
#endif


int I18n::m_Locale = -1;

std::string I18n::Get(const std::string& key) {
    if(m_Locale == -1)
        FindLocale();

    std::map<std::string, std::string> map;
    if(m_Locale == 1)
        map = m_DeLangMap;
    else
        map = m_EnLangMap;

    if(!map.count(key)) {
        Logger::writeln("Missing I18n key ", key);
        return {};
    }

    return map[key];
}

void I18n::FindLocale() {
    auto settingsLang = AppStorage::Get().language;
    if(settingsLang != "auto") {
        if(settingsLang == "de_DE")
            m_Locale = 1;
        else
            m_Locale = 0;
        return;
    }

#ifdef _WIN32
    LCID lcid = GetThreadLocale();
    wchar_t name[LOCALE_NAME_MAX_LENGTH];
    if (LCIDToLocaleName(lcid, name, LOCALE_NAME_MAX_LENGTH, 0) == 0) {
        m_Locale = 0;
        return;
    }

    auto ws = std::wstring(name);
    auto locale = std::string(ws.begin(), ws.end());
#else
    std::string locale;
    if(std::filesystem::exists("/etc/locale.conf")) {
        std::ifstream is_file("/etc/locale.conf");
        std::string line;
        while(std::getline(is_file, line)) {
            std::istringstream is_line(line);
            std::string key;
            if(std::getline(is_line, key, '=')) {
                std::string value;
                if(std::getline(is_line, value)) {
                    if(key == "LANG") {
                        locale = value;
                        break;
                    }
                }
            }
        }
    } else {
        auto lang = setlocale(LC_ALL, nullptr);
        locale = std::string(lang);
    }
#endif

    if(Utils::StringStartsWith(locale, "de"))
        m_Locale = 1;
    else
        m_Locale = 0;
}