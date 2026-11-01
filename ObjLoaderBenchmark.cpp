#pragma once

const bool deduplicateVertices = false;

#include "Utils/objFileScanner.h"
#include "Utils/implementationsRunner.h"
#include "Utils/resultsDisplayer.h"
#include "Utils/meshComparator.h"
#include "Utils/consoleColors.h"

const char* objFolderPath = "Objs";

int main()
{
    ConsoleColors::section("OBJ LOADER BENCHMARK", ConsoleColors::Color::Blue);
    ConsoleColors::write(ConsoleColors::Color::Cyan, "    Scanning ");
    ConsoleColors::write(ConsoleColors::Color::White, "OBJ files");
    std::cout << "\n\n";

    std::vector<ObjFileInfo> paths = scanFolderForObjFiles(objFolderPath);

    ConsoleColors::section("RUNNING IMPLEMENTATIONS", ConsoleColors::Color::Magenta);

    std::vector<Results> results = runImplementations(paths);

    MeshComparator::checkMeshes(results, paths);

    showResults(results);

    ConsoleColors::section("BENCHMARK COMPLETE", ConsoleColors::Color::Green);

    system("pause");
};
