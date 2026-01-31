#pragma once

const bool deduplicateVertices = false;

#include "Utils/objFileScanner.h"
#include "Utils/implementationsRunner.h"
#include "Utils/resultsDisplayer.h"

const char* objFolderPath = "Objs";

int main()
{
    writeNewLine("Welcome to my tiny benchmark.");
    writeNewLine("Scanning obj files.");

    std::vector<std::string> paths = scanFolderForObjFiles(objFolderPath);

    writeNewLine("Running implementations.");

    std::vector<Results> results = runImplementations(paths);

    writeNewLine("Finished.\n\n");

    showResults(results);

    system("pause");
};