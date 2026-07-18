#include "Encoding.h"
#include "Grouping.h"
#include "Object.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    const fs::path kDefaultDataFile = pathFromUtf8("objects.txt");
    const fs::path kDefaultResultFile = pathFromUtf8("result.txt");

    void clearInput()
    {
        std::cin.clear();
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    }

    std::string readLine(const std::string& prompt)
    {
        return readConsoleLine(prompt);
    }

    double readDouble(const std::string& prompt)
    {
        while (true)
        {
            const std::string line = readLine(prompt);
            try
            {
                size_t processed = 0;
                const double value = std::stod(line, &processed);
                if (processed == line.size())
                {
                    return value;
                }
            }
            catch (...)
            {
            }

            consoleWriteLine("Введите корректное число.");
        }
    }

    int readInt(const std::string& prompt)
    {
        while (true)
        {
            const std::string line = readLine(prompt);
            try
            {
                size_t processed = 0;
                const int value = std::stoi(line, &processed);
                if (processed == line.size())
                {
                    return value;
                }
            }
            catch (...)
            {
            }

            consoleWriteLine("Введите корректное целое число.");
        }
    }

    fs::path resolvePath(const std::string& pathText, const fs::path& fallback)
    {
        if (pathText.empty())
        {
            return fallback;
        }
        return pathFromUtf8(pathText);
    }

    void ensureParentDirectory(const fs::path& path)
    {
        const fs::path parent = path.parent_path();
        if (!parent.empty())
        {
            fs::create_directories(parent);
        }
    }

    void createDefaultDataFile(const fs::path& path)
    {
        ensureParentDirectory(path);

        std::ofstream file = openUtf8OutputFile(path);
        if (!file)
        {
            throw std::runtime_error("Не удалось создать файл: " + pathToUtf8(path));
        }

        file << "Кривой -37.23 13.44 Человек 1693235249.98678\n";
        file << "Магазин 0.163 2.119 Здание 1693135218.173\n";
        file << "Лада 118.3 -982.041 Машина 1692748121.63\n";
    }

    void ensureDataFileExists(const fs::path& path)
    {
        if (fs::exists(path))
        {
            return;
        }

        createDefaultDataFile(path);
        consoleWriteLine("Файл не найден, создан новый: " + pathToUtf8(fs::absolute(path)));
    }

    std::vector<Object> loadObjects(const fs::path& path)
    {
        ensureDataFileExists(path);

        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            throw std::runtime_error("Не удалось открыть файл: " + pathToUtf8(path));
        }

        std::vector<Object> objects;
        std::string line;
        int lineNumber = 0;

        while (std::getline(file, line))
        {
            ++lineNumber;
            if (lineNumber == 1)
            {
                stripUtf8Bom(line);
            }

            if (line.empty())
            {
                continue;
            }

            const auto object = parseObjectLine(line);
            if (!object)
            {
                consoleWriteLine("Пропущена строка " + std::to_string(lineNumber) + ": неверный формат.");
                continue;
            }

            objects.push_back(*object);
        }

        return objects;
    }

    void saveObjects(const fs::path& path, const std::vector<Object>& objects)
    {
        ensureParentDirectory(path);

        std::ofstream file = openUtf8OutputFile(path);
        if (!file)
        {
            throw std::runtime_error("Не удалось сохранить файл: " + pathToUtf8(path));
        }

        for (const auto& object : objects)
        {
            file << object.toLine() << '\n';
        }
    }

    void saveGroupedResult(
        const fs::path& path,
        const std::map<std::string, std::vector<Object>>& groups,
        GroupMode mode)
    {
        ensureParentDirectory(path);

        std::ofstream file = openUtf8OutputFile(path);
        if (!file)
        {
            throw std::runtime_error("Не удалось сохранить результат: " + pathToUtf8(path));
        }

        const auto orderedKeys = orderedGroupKeys(mode);
        if (!orderedKeys.empty())
        {
            for (const auto& key : orderedKeys)
            {
                const auto it = groups.find(key);
                if (it == groups.end() || it->second.empty())
                {
                    continue;
                }

                file << "=== " << key << " ===\n";
                for (const auto& object : it->second)
                {
                    file << object.toLine() << '\n';
                }
                file << '\n';
            }

            for (const auto& [key, items] : groups)
            {
                if (items.empty())
                {
                    continue;
                }

                if (std::find(orderedKeys.begin(), orderedKeys.end(), key) != orderedKeys.end())
                {
                    continue;
                }

                file << "=== " << key << " ===\n";
                for (const auto& object : items)
                {
                    file << object.toLine() << '\n';
                }
                file << '\n';
            }

            return;
        }

        for (const auto& [key, items] : groups)
        {
            if (items.empty())
            {
                continue;
            }

            file << "=== " << key << " ===\n";
            for (const auto& object : items)
            {
                file << object.toLine() << '\n';
            }
            file << '\n';
        }
    }

    void printObjects(const std::vector<Object>& objects)
    {
        if (objects.empty())
        {
            consoleWriteLine("Список пуст.");
            return;
        }

        for (size_t i = 0; i < objects.size(); ++i)
        {
            consoleWriteLine(std::to_string(i + 1) + ". " + objects[i].toLine());
        }
    }

    SortMode readSortMode()
    {
        consoleWriteLine("1. По имени");
        consoleWriteLine("2. По расстоянию");
        consoleWriteLine("3. По времени создания");
        consoleWriteLine("4. По типу");

        switch (readInt("Выберите сортировку: "))
        {
        case 1:
            return SortMode::Name;
        case 2:
            return SortMode::Distance;
        case 3:
            return SortMode::Time;
        case 4:
            return SortMode::Type;
        default:
            consoleWriteLine("Неверный выбор, использую сортировку по имени.");
            return SortMode::Name;
        }
    }

    GroupMode readGroupMode()
    {
        consoleWriteLine("1. По расстоянию");
        consoleWriteLine("2. По имени");
        consoleWriteLine("3. По времени создания");
        consoleWriteLine("4. По типу");

        switch (readInt("Выберите группировку: "))
        {
        case 1:
            return GroupMode::Distance;
        case 2:
            return GroupMode::Name;
        case 3:
            return GroupMode::Time;
        case 4:
            return GroupMode::Type;
        default:
            consoleWriteLine("Неверный выбор, использую группировку по расстоянию.");
            return GroupMode::Distance;
        }
    }

    Object readObjectFromUser()
    {
        Object object;
        object.name = readLine("Имя: ");
        object.x = readDouble("X: ");
        object.y = readDouble("Y: ");
        object.type = readLine("Тип: ");
        object.createdAt = readDouble("Время создания (Unix timestamp): ");
        return object;
    }

    void printMenu(const fs::path& dataFile)
    {
        
        consoleWriteLine("\n--- Меню ---");
        consoleWriteLine("Текущий файл данных: " + pathToUtf8(fs::absolute(dataFile)));
        consoleWriteLine("1. Загрузить объекты из файла");
        consoleWriteLine("2. Добавить объект");
        consoleWriteLine("3. Показать список");
        consoleWriteLine("4. Отсортировать список");
        consoleWriteLine("5. Сгруппировать, отсортировать и сохранить");
        consoleWriteLine("6. Сохранить текущий список в файл");
        consoleWriteLine("0. Выход");
    }
}

int main()
{
    setupConsoleUtf8();

    std::vector<Object> objects;
    fs::path dataFile = kDefaultDataFile;

    consoleWriteLine("Обработка списка объектов");

    try
    {
        objects = loadObjects(dataFile);
        consoleWriteLine("Загружено объектов: " + std::to_string(objects.size()));
    }
    catch (const std::exception& ex)
    {
        consoleWriteLine(std::string("Ошибка при старте: ") + ex.what());
    }

    while (true)
    {
        printMenu(dataFile);

        const int choice = readInt("Выберите действие: ");
        try
        {
            switch (choice)
            {
            case 0:
                consoleWriteLine("Выход.");
                return 0;

            case 1:
            {
                const std::string pathText = readLine("Путь к файлу (Enter = текущий): ");
                dataFile = resolvePath(pathText, dataFile);
                objects = loadObjects(dataFile);
                consoleWriteLine("Загружено объектов: " + std::to_string(objects.size()));
                break;
            }

            case 2:
            {
                objects.push_back(readObjectFromUser());
                saveObjects(dataFile, objects);
                consoleWriteLine("Объект добавлен и сохранён в " + pathToUtf8(fs::absolute(dataFile)));
                break;
            }

            case 3:
                printObjects(objects);
                break;

            case 4:
            {
                const SortMode mode = readSortMode();
                sortObjects(objects, mode);
                consoleWriteLine("Список отсортирован.");
                printObjects(objects);
                break;
            }

            case 5:
            {
                const GroupMode groupMode = readGroupMode();
                int typeThreshold = 2;
                if (groupMode == GroupMode::Type)
                {
                    typeThreshold = readInt("Порог N (тип попадает в группу, если объектов больше N): ");
                }

                const auto groups = groupObjects(objects, groupMode, typeThreshold);
                const std::string resultPathText = readLine(
                    "Файл результата (Enter = result.txt): ");
                const fs::path resultPath = resolvePath(resultPathText, kDefaultResultFile);

                saveGroupedResult(resultPath, groups, groupMode);
                consoleWriteLine("Результат сохранён: " + pathToUtf8(fs::absolute(resultPath)));
                break;
            }

            case 6:
                saveObjects(dataFile, objects);
                consoleWriteLine("Список сохранён: " + pathToUtf8(fs::absolute(dataFile)));
                break;

            default:
                consoleWriteLine("Неизвестная команда.");
                break;
            }
        }
        catch (const std::exception& ex)
        {
            consoleWriteLine(std::string("Ошибка: ") + ex.what());
        }
    }
}
