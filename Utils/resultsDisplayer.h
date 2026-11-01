#pragma once
#include "../types.h"
#include "consoleColors.h"

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
    ConsoleColors::section("BENCHMARK SUMMARY", ConsoleColors::Color::Cyan);

    for (size_t i = 0; i < summaries.size(); ++i)
    {
        if (i > 0)
            ConsoleColors::divider('-', 88, ConsoleColors::Color::Gray);

        const ConsoleColors::Color rankColor = i == 0 ? ConsoleColors::Color::Green
            : (i == 1 ? ConsoleColors::Color::Cyan
                : (i == 2 ? ConsoleColors::Color::Yellow : ConsoleColors::Color::White));

        ConsoleColors::write(rankColor, "    " + std::to_string(i + 1) + ". ");
        ConsoleColors::write(ConsoleColors::Color::White, summaries[i].name);
        std::cout << '\n' << "      ";

        ConsoleColors::write(ConsoleColors::Color::Cyan, "Vertices");
        std::cout << ": ";
        ConsoleColors::write(ConsoleColors::Color::White, summaries[i].totalVertices);
        std::cout << "    ";

        ConsoleColors::write(ConsoleColors::Color::Cyan, "Indices");
        std::cout << ": ";
        ConsoleColors::write(ConsoleColors::Color::White, summaries[i].totalIndices);
        std::cout << "    ";

        ConsoleColors::write(ConsoleColors::Color::Yellow, "Time");
        std::cout << ": ";
        ConsoleColors::write(ConsoleColors::Color::Yellow, summaries[i].totalTime.count());
        std::cout << " ms\n";
    }

    ConsoleColors::divider('=', 88, ConsoleColors::Color::Cyan);
}

void showResults(std::vector<Results> results)
{
    std::vector<ImplSummary> summaries = getSummaries(results);

    sortSummaries(summaries);

    displaySummaries(summaries);
};
