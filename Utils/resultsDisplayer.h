#pragma once
#include "../types.h"

std::vector<ImplSummary> getSummaries(std::vector<Results> results)
{
    std::vector<ImplSummary> summaries;
    summaries.reserve(results.size());

    for (const Results& implResults : results)
    {
        size_t totalVertices = 0;
        size_t totalIndices = 0;
        std::chrono::milliseconds totalTime(0);

        for (const Result& r : implResults.data)
        {
            totalVertices += r.mesh.vertices.size();
            totalIndices += r.mesh.indices.size();
            totalTime += r.elapsed;
        }

        summaries.push_back({ implResults.implementationName, totalVertices, totalIndices, totalTime });
    }

    return summaries;
}

void sortSummaries(std::vector<ImplSummary> &summaries)
{
    std::sort(summaries.begin(), summaries.end(),
        [](const ImplSummary& a, const ImplSummary& b) {
            return a.totalTime < b.totalTime;
        });
}

void displaySummaries(std::vector<ImplSummary> summaries)
{
    std::cout << "===== Benchmark Summary =====\n\n";

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    std::vector<WORD> colors = { 3, 2, 6, 4, 7 };

    for (size_t i = 0; i < summaries.size(); ++i)
    {
        WORD color = (i < colors.size()) ? colors[i] : 7;

        SetConsoleTextAttribute(hConsole, color);

        std::cout << i + 1 << ". " << summaries[i].name << "\n";
        std::cout << "   Total Vertices: " << summaries[i].totalVertices
            << ", Total Indices: " << summaries[i].totalIndices
            << ", Total Time: " << summaries[i].totalTime.count() << " ms\n\n";
    }

    SetConsoleTextAttribute(hConsole, 7);
}

void showResults(std::vector<Results> results)
{
    std::vector<ImplSummary> summaries = getSummaries(results);

    sortSummaries(summaries);

    displaySummaries(summaries);
};