#pragma once
#include "../types.h"

bool HasObjExtension(const std::string& filename)
{
    size_t dot = filename.find_last_of('.');
    return dot != std::string::npos && filename.substr(dot) == ".obj";
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

std::vector<std::string> scanFolderForObjFiles(const std::string& folderPath)
{
    std::vector<std::string> result;

    std::string searchPath = folderPath + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE handle = FindFirstFileA(searchPath.c_str(), &findData);

    if (handle == INVALID_HANDLE_VALUE)
    {
        std::cout << "Folder not found: " << folderPath << std::endl;
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

        if (filename.size() > 4 && filename.substr(filename.size() - 4) == ".obj")
        {
            result.push_back(folderPath + "\\" + filename);
        }

        std::string fullPath = folderPath + "\\" + filename;

        size_t vertexCount = CountVerticesInObj(fullPath);

        std::cout << filename << " -> " << vertexCount << " vertices\n";

    } while (FindNextFileA(handle, &findData));

    FindClose(handle);

    return result;
}