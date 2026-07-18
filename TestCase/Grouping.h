#pragma once

#include "Object.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

enum class SortMode
{
    Name,
    Distance,
    Time,
    Type
};

enum class GroupMode
{
    Distance,
    Name,
    Time,
    Type
};

inline void sortObjects(std::vector<Object>& objects, SortMode mode)
{
    switch (mode)
    {
    case SortMode::Name:
        std::sort(objects.begin(), objects.end(), [](const Object& a, const Object& b) {
            return a.name < b.name;
        });
        break;
    case SortMode::Distance:
        std::sort(objects.begin(), objects.end(), [](const Object& a, const Object& b) {
            return a.distance() < b.distance();
        });
        break;
    case SortMode::Time:
        std::sort(objects.begin(), objects.end(), [](const Object& a, const Object& b) {
            return a.createdAt < b.createdAt;
        });
        break;
    case SortMode::Type:
        std::sort(objects.begin(), objects.end(), [](const Object& a, const Object& b) {
            if (a.type != b.type)
            {
                return a.type < b.type;
            }
            return a.name < b.name;
        });
        break;
    }
}

inline std::string distanceGroupKey(double distance)
{
    if (distance <= 100.0)
    {
        return "До 100";
    }
    if (distance <= 1000.0)
    {
        return "До 1000";
    }
    if (distance <= 10000.0)
    {
        return "До 10000";
    }
    return "Слишком далеко";
}

inline bool sameDay(const tm& left, const tm& right)
{
    return left.tm_year == right.tm_year &&
        left.tm_mon == right.tm_mon &&
        left.tm_mday == right.tm_mday;
}

inline tm startOfDay(const tm& source)
{
    tm result = source;
    result.tm_hour = 0;
    result.tm_min = 0;
    result.tm_sec = 0;
    result.tm_isdst = -1;
    mktime(&result);
    return result;
}

inline tm addDays(tm source, int days)
{
    source.tm_mday += days;
    source.tm_isdst = -1;
    mktime(&source);
    return source;
}

inline tm startOfWeekMonday(const tm& source)
{
    tm day = startOfDay(source);
    const int weekday = (day.tm_wday + 6) % 7;
    return addDays(day, -weekday);
}

inline std::string timeGroupKey(double createdAt)
{
    const time_t objectTime = static_cast<time_t>(createdAt);
    tm objectDate {};
    localtime_s(&objectDate, &objectTime);

    const time_t nowTime = std::time(nullptr);
    tm nowDate {};
    localtime_s(&nowDate, &nowTime);

    const tm objectDay = startOfDay(objectDate);
    const tm today = startOfDay(nowDate);
    const tm yesterday = addDays(today, -1);
    const tm weekStart = startOfWeekMonday(nowDate);

    tm objectDayMutable = objectDay;
    tm todayMutable = today;
    tm yesterdayMutable = yesterday;
    tm weekStartMutable = weekStart;

    const time_t objectDayTime = mktime(&objectDayMutable);
    const time_t todayTime = mktime(&todayMutable);
    const time_t yesterdayTime = mktime(&yesterdayMutable);
    const time_t weekStartTime = mktime(&weekStartMutable);

    if (objectDayTime == todayTime)
    {
        return "Сегодня";
    }
    if (objectDayTime == yesterdayTime)
    {
        return "Вчера";
    }
    if (objectDayTime >= weekStartTime)
    {
        return "На этой неделе";
    }
    if (objectDate.tm_year == nowDate.tm_year && objectDate.tm_mon == nowDate.tm_mon)
    {
        return "В этом месяце";
    }
    if (objectDate.tm_year == nowDate.tm_year)
    {
        return "В этом году";
    }
    return "Ранее";
}

inline std::map<std::string, std::vector<Object>> groupByDistance(const std::vector<Object>& objects)
{
    std::map<std::string, std::vector<Object>> groups;
    for (const auto& object : objects)
    {
        groups[distanceGroupKey(object.distance())].push_back(object);
    }

    for (auto& [_, items] : groups)
    {
        sortObjects(items, SortMode::Distance);
    }

    return groups;
}

inline std::map<std::string, std::vector<Object>> groupByName(const std::vector<Object>& objects)
{
    std::map<std::string, std::vector<Object>> groups;
    for (const auto& object : objects)
    {
        groups[nameGroupKey(object.name)].push_back(object);
    }

    for (auto& [_, items] : groups)
    {
        sortObjects(items, SortMode::Name);
    }

    return groups;
}

inline std::map<std::string, std::vector<Object>> groupByTime(const std::vector<Object>& objects)
{
    std::map<std::string, std::vector<Object>> groups;
    for (const auto& object : objects)
    {
        groups[timeGroupKey(object.createdAt)].push_back(object);
    }

    for (auto& [_, items] : groups)
    {
        sortObjects(items, SortMode::Time);
    }

    return groups;
}

inline std::map<std::string, std::vector<Object>> groupByType(const std::vector<Object>& objects, int threshold)
{
    std::unordered_map<std::string, int> typeCounts;
    for (const auto& object : objects)
    {
        ++typeCounts[object.type];
    }

    std::map<std::string, std::vector<Object>> groups;
    for (const auto& object : objects)
    {
        const std::string key = typeCounts[object.type] > threshold ? object.type : "Разное";
        groups[key].push_back(object);
    }

    for (auto& [_, items] : groups)
    {
        sortObjects(items, SortMode::Name);
    }

    return groups;
}

inline std::vector<std::string> orderedGroupKeys(GroupMode mode)
{
    switch (mode)
    {
    case GroupMode::Distance:
        return {"До 100", "До 1000", "До 10000", "Слишком далеко"};
    case GroupMode::Name:
        return {};
    case GroupMode::Time:
        return {"Сегодня", "Вчера", "На этой неделе", "В этом месяце", "В этом году", "Ранее"};
    case GroupMode::Type:
        return {};
    }
    return {};
}

inline std::map<std::string, std::vector<Object>> groupObjects(
    const std::vector<Object>& objects,
    GroupMode mode,
    int typeThreshold = 2)
{
    switch (mode)
    {
    case GroupMode::Distance:
        return groupByDistance(objects);
    case GroupMode::Name:
        return groupByName(objects);
    case GroupMode::Time:
        return groupByTime(objects);
    case GroupMode::Type:
        return groupByType(objects, typeThreshold);
    }
    return {};
}
