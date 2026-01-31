#pragma once
#include "../types.h"

class LoaderTemplate
{
    public:
        virtual ~LoaderTemplate() {}

        virtual const char* Name() const = 0;

        Mesh loadObj(const std::string& filename)
        {
            auto start = std::chrono::high_resolution_clock::now();
            std::cout << "Loading: " << filename << " ";

            Mesh mesh = this->loadObjImplementation(filename);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            std::cout << " done. Took " << duration.count() << " ms.\n";

            return mesh;
        };

        std::vector<Result> loadAllObjs(const std::vector<std::string> paths)
        {
            std::vector<Result> results;
            results.reserve(paths.size());

            for (const std::string& path : paths)
            {
                auto start = std::chrono::high_resolution_clock::now();
                Mesh mesh = this->loadObjImplementation(path);
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

                std::cout << "Loaded " << path << " in " << duration.count() << " ms.\n";

                results.push_back({ mesh, duration });
            }

            return results;
        }

        virtual Mesh loadObjImplementation(const std::string& filename) = 0;
};