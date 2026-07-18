#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

struct Object
{
    std::string name;
    double x = 0.0;
    double y = 0.0;
    std::string type;
    double createdAt = 0.0;

    double distance() const
    {
        return std::hypot(x, y);
    }

    std::string toLine() const
    {
        std::ostringstream out;
        out << name << ' ' << x << ' ' << y << ' ' << type << ' ' << createdAt;
        return out.str();
    }
};

inline std::optional<char32_t> firstCodepoint(const std::string& text)
{
    if (text.empty())
    {
        return std::nullopt;
    }

    const unsigned char c0 = static_cast<unsigned char>(text[0]);
    if (c0 < 0x80)
    {
        return c0;
    }

    if ((c0 & 0xE0) == 0xC0 && text.size() >= 2)
    {
        const unsigned char c1 = static_cast<unsigned char>(text[1]);
        return ((c0 & 0x1F) << 6) | (c1 & 0x3F);
    }

    if ((c0 & 0xF0) == 0xE0 && text.size() >= 3)
    {
        const unsigned char c1 = static_cast<unsigned char>(text[1]);
        const unsigned char c2 = static_cast<unsigned char>(text[2]);
        return ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
    }

    return std::nullopt;
}

inline std::string codepointToUtf8(char32_t codepoint)
{
    std::string result;
    if (codepoint <= 0x7F)
    {
        result.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint <= 0x7FF)
    {
        result.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    else
    {
        result.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return result;
}

inline std::string nameGroupKey(const std::string& name)
{
    const auto codepoint = firstCodepoint(name);
    if (!codepoint)
    {
        return "#";
    }

    char32_t letter = *codepoint;
    if (letter >= 0x0430 && letter <= 0x044F)
    {
        letter -= 0x20;
    }

    if (letter >= 0x0410 && letter <= 0x042F)
    {
        return codepointToUtf8(letter);
    }

    return "#";
}

inline std::optional<Object> parseObjectLine(const std::string& line)
{
    if (line.empty())
    {
        return std::nullopt;
    }

    std::istringstream stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (stream >> token)
    {
        tokens.push_back(token);
    }

    if (tokens.size() < 5)
    {
        return std::nullopt;
    }

    Object object;
    try
    {
        object.createdAt = std::stod(tokens.back());
        object.type = tokens[tokens.size() - 2];
        object.y = std::stod(tokens[tokens.size() - 3]);
        object.x = std::stod(tokens[tokens.size() - 4]);
    }
    catch (...)
    {
        return std::nullopt;
    }

    object.name = tokens.front();
    for (size_t i = 1; i < tokens.size() - 4; ++i)
    {
        object.name += ' ';
        object.name += tokens[i];
    }

    return object;
}
