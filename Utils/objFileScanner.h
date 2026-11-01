#pragma once
#include "../types.h"
#include "consoleColors.h"

bool HasObjExtension(const std::string& filename)
{
    size_t dot = filename.find_last_of('.');
    return dot != std::string::npos && filename.substr(dot) == ".obj";
}

std::string ObjFileName(const std::string& path)
{
    const size_t separator = path.find_last_of("\\/");
    return separator == std::string::npos ? path : path.substr(separator + 1);
}

size_t CountVerticesInObj(const std::string& filePath)
{
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file)
    {
        return 0;
    }

    const size_t bufferSize = 1024 * 1024; // 1 MB buffer
    std::vector<char> buffer(bufferSize);

    size_t count = 0;
    bool newLine = true;

    while (file)
    {
        file.read(buffer.data(), bufferSize);
        size_t bytesRead = (size_t)file.gcount();

        for (size_t i = 0; i < bytesRead; i++)
        {
            char c = buffer[i];

            if (newLine && c == 'v')
            {
                if (i + 1 < bytesRead && buffer[i + 1] == ' ')
                {
                    count++;
                }
            }

            newLine = (c == '\n');
        }
    }

    return count;
}

std::vector<ObjFileInfo> scanFolderForObjFiles(const std::string& folderPath)
{
    std::vector<ObjFileInfo> result;

    std::string searchPath = folderPath + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE handle = FindFirstFileA(searchPath.c_str(), &findData);

    if (handle == INVALID_HANDLE_VALUE)
    {
        ConsoleColors::write(ConsoleColors::Color::Red, "Folder not found: ");
        ConsoleColors::write(ConsoleColors::Color::White, folderPath);
        std::cout << std::endl;
        return result;
    }

    do
    {
        std::string filename = findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            continue;
        }

        if (!HasObjExtension(filename))
        {
            continue;
        }

        std::string fullPath = folderPath + "\\" + filename;
        size_t vertexCount = CountVerticesInObj(fullPath);
        result.push_back({ fullPath, vertexCount });

    } while (FindNextFileA(handle, &findData));

    FindClose(handle);

    size_t widestName = 0;
    size_t widestCount = 1;
    for (const ObjFileInfo& file : result)
    {
        widestName = (std::max)(widestName, ObjFileName(file.path).size());
        widestCount = (std::max)(widestCount, std::to_string(file.vertexCount).size());
    }

    const size_t indentationWidth = 6;
    const size_t suffixWidth = 5 + widestCount + 9; // "  |  " + count + " vertices"
    const size_t terminalWidth = ConsoleColors::terminalWidth();
    const size_t availableNameWidth = terminalWidth > indentationWidth + suffixWidth
        ? terminalWidth - indentationWidth - suffixWidth
        : 1;
    const size_t nameWidth = (std::min)(widestName, availableNameWidth);

    for (const ObjFileInfo& file : result)
    {
        std::string displayName = ObjFileName(file.path);
        if (displayName.size() > nameWidth)
        {
            if (nameWidth > 3)
                displayName = displayName.substr(0, nameWidth - 3) + "...";
            else
                displayName = displayName.substr(0, nameWidth);
        }

        std::cout << "      ";
        ConsoleColors::write(ConsoleColors::Color::Cyan, displayName);
        if (displayName.size() < nameWidth)
            std::cout << std::string(nameWidth - displayName.size(), ' ');
        ConsoleColors::write(ConsoleColors::Color::Gray, "  |  ");
        ConsoleColors::write(ConsoleColors::Color::Green, file.vertexCount);
        std::cout << " vertices\n";
    }

    return result;
}
