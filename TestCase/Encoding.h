#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

inline std::string pathToUtf8(const fs::path& path)
{
    const std::u8string u8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

inline fs::path pathFromUtf8(const std::string& text)
{
    return fs::path(std::u8string(
        reinterpret_cast<const char8_t*>(text.data()),
        text.size()));
}

inline void setupConsoleUtf8()
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

inline void consoleWrite(const std::string& text)
{
#ifdef _WIN32
    if (text.empty())
    {
        return;
    }

    const int wideSize = MultiByteToWideChar(
        CP_UTF8,
        0,
        text.data(),
        static_cast<int>(text.size()),
        nullptr,
        0);
    if (wideSize <= 0)
    {
        return;
    }

    std::wstring wide(wideSize, L'\0');
    MultiByteToWideChar(
        CP_UTF8,
        0,
        text.data(),
        static_cast<int>(text.size()),
        wide.data(),
        wideSize);

    DWORD written = 0;
    WriteConsoleW(
        GetStdHandle(STD_OUTPUT_HANDLE),
        wide.data(),
        static_cast<DWORD>(wideSize),
        &written,
        nullptr);
#else
    std::cout << text;
#endif
}

inline void consoleWriteLine(const std::string& text = {})
{
    consoleWrite(text);
    consoleWrite("\n");
}

inline std::string readConsoleLine(const std::string& prompt)
{
    consoleWrite(prompt);

#ifdef _WIN32
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    wchar_t buffer[4096];
    DWORD charsRead = 0;

    if (!ReadConsoleW(input, buffer, 4095, &charsRead, nullptr))
    {
        return {};
    }

    while (charsRead > 0 &&
        (buffer[charsRead - 1] == L'\n' || buffer[charsRead - 1] == L'\r'))
    {
        --charsRead;
    }

    const std::wstring wide(buffer, charsRead);
    if (wide.empty())
    {
        return {};
    }

    const int utf8Size = WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.data(),
        static_cast<int>(wide.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    std::string result(utf8Size, '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wide.data(),
        static_cast<int>(wide.size()),
        result.data(),
        utf8Size,
        nullptr,
        nullptr);
    return result;
#else
    std::string line;
    std::getline(std::cin, line);
    return line;
#endif
}

inline void writeUtf8Bom(std::ofstream& file)
{
    file.write("\xEF\xBB\xBF", 3);
}

inline void stripUtf8Bom(std::string& line)
{
    if (line.size() >= 3 &&
        static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF)
    {
        line.erase(0, 3);
    }
}

inline std::ofstream openUtf8OutputFile(const fs::path& path)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    writeUtf8Bom(file);
    return file;
}
