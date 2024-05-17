#pragma once

#include <string>

#include <WinSock2.h>
#include <Windows.h>

class WinUtils {
public:
    static std::wstring StringToWideString(const std::string& string) {
        if (string.empty())
        {
            return L"";
        }
        const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), nullptr, 0);
        if (size_needed <= 0)
        {
            return L"";
        }

        std::wstring result(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), &result.at(0), size_needed);
        return result;
    }

    static std::string WideStringToString(const std::wstring& wide_string)
    {
        if (wide_string.empty())
        {
            return "";
        }
        const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), nullptr, 0, nullptr, nullptr);
        if (size_needed <= 0)
        {
            return "";
        }

        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), &result.at(0), size_needed, nullptr, nullptr);
        return result;
    }

private:
	WinUtils() = default;
};