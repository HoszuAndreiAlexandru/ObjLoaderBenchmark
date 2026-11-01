#pragma once
#include "../types.h"
#include "../Utils/consoleColors.h"

#include <atomic>
#include <condition_variable>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <thread>

namespace BenchmarkConsole
{
    class LoadingIndicator
    {
    public:
        explicit LoadingIndicator(const ObjFileInfo& file)
            : fileName_(baseName(file.path)),
            vertexCount_(file.vertexCount),
            animated_(ConsoleColors::isTerminal()),
            stopRequested_(false),
            startTime_(std::chrono::steady_clock::now()),
            lastLineWidth_(0),
            finished_(false)
        {
            if (animated_)
            {
                worker_ = std::thread(&LoadingIndicator::animate, this);
            }
            else
            {
                std::cout << "      ";
                ConsoleColors::write(ConsoleColors::Color::Yellow, "Loading");
                std::cout << ' ';
                ConsoleColors::write(ConsoleColors::Color::White, fileName_);
                std::cout << ' ';
                ConsoleColors::write(ConsoleColors::Color::Cyan, vertexCount_);
                std::cout << " vertices\n";
            }
        }

        LoadingIndicator(const LoadingIndicator&) = delete;
        LoadingIndicator& operator=(const LoadingIndicator&) = delete;

        ~LoadingIndicator()
        {
            stopAndJoin();
            if (animated_ && !finished_)
                std::cout << '\n';
        }

        void finish(std::chrono::milliseconds loadTime)
        {
            stopAndJoin();

            if (animated_)
                drawCompletedLine(loadTime);
            else
            {
                std::cout << "      ";
                ConsoleColors::write(ConsoleColors::Color::Green, "Loaded ");
                ConsoleColors::write(ConsoleColors::Color::White, fileName_);
                std::cout << ' ';
                ConsoleColors::write(ConsoleColors::Color::Cyan, vertexCount_);
                ConsoleColors::write(ConsoleColors::Color::Gray, " vertices  done in ");
                ConsoleColors::write(ConsoleColors::Color::Yellow, loadTime.count());
                std::cout << " ms\n";
            }

            finished_ = true;
        }

    private:
        static std::string baseName(const std::string& path)
        {
            const size_t separator = path.find_last_of("\\/");
            return separator == std::string::npos ? path : path.substr(separator + 1);
        }

        static std::string formatElapsed(std::chrono::steady_clock::duration elapsed)
        {
            const long long milliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            const long long seconds = milliseconds / 1000;

            std::ostringstream text;
            text << std::setfill('0') << std::setw(2) << seconds / 60
                << ':' << std::setw(2) << seconds % 60
                << '.' << (milliseconds / 100) % 10 << 's';
            return text.str();
        }

        void animate()
        {
            const char frames[] = { '-', '\\', '|', '/' };
            size_t frame = 0;
            while (!stopRequested_.load())
            {
                drawProgressLine(frames[frame]);
                frame = (frame + 1) % (sizeof(frames) / sizeof(frames[0]));

                std::unique_lock<std::mutex> lock(stopMutex_);
                if (stopCondition_.wait_for(lock, std::chrono::milliseconds(100),
                    [this] { return stopRequested_.load(); }))
                    break;
            }
        }

        void drawProgressLine(char spinner)
        {
            const std::string elapsed = formatElapsed(std::chrono::steady_clock::now() - startTime_);
            const std::string line = "      Loading " + fileName_ + " " + std::to_string(vertexCount_) +
                " vertices  elapsed " + elapsed + "  " + spinner;

            std::cout << '\r' << "      ";
            ConsoleColors::write(ConsoleColors::Color::Yellow, "Loading");
            std::cout << ' ';
            ConsoleColors::write(ConsoleColors::Color::White, fileName_);
            std::cout << ' ';
            ConsoleColors::write(ConsoleColors::Color::Cyan, vertexCount_);
            ConsoleColors::write(ConsoleColors::Color::Gray, " vertices  elapsed ");
            ConsoleColors::write(ConsoleColors::Color::Yellow, elapsed);
            ConsoleColors::write(ConsoleColors::Color::Gray, "  ");
            ConsoleColors::write(ConsoleColors::Color::Magenta, spinner);

            if (line.size() < lastLineWidth_)
                std::cout << std::string(lastLineWidth_ - line.size(), ' ');
            lastLineWidth_ = (std::max)(lastLineWidth_, line.size());
            std::cout.flush();
        }

        void drawCompletedLine(std::chrono::milliseconds loadTime)
        {
            const std::string time = std::to_string(loadTime.count()) + " ms";
            const std::string line = "      Loaded " + fileName_ + " " + std::to_string(vertexCount_) +
                " vertices  done in " + time;

            std::cout << '\r' << "      ";
            ConsoleColors::write(ConsoleColors::Color::Green, "Loaded");
            std::cout << ' ';
            ConsoleColors::write(ConsoleColors::Color::White, fileName_);
            std::cout << ' ';
            ConsoleColors::write(ConsoleColors::Color::Cyan, vertexCount_);
            ConsoleColors::write(ConsoleColors::Color::Gray, " vertices  done in ");
            ConsoleColors::write(ConsoleColors::Color::Yellow, loadTime.count());
            ConsoleColors::write(ConsoleColors::Color::Gray, " ms");

            if (line.size() < lastLineWidth_)
                std::cout << std::string(lastLineWidth_ - line.size(), ' ');
            std::cout << '\n' << std::flush;
        }

        void stopAndJoin()
        {
            stopRequested_.store(true);
            stopCondition_.notify_one();
            if (worker_.joinable())
                worker_.join();
        }

        std::string fileName_;
        size_t vertexCount_;
        bool animated_;
        std::atomic<bool> stopRequested_;
        std::mutex stopMutex_;
        std::condition_variable stopCondition_;
        std::chrono::steady_clock::time_point startTime_;
        size_t lastLineWidth_;
        bool finished_;
        std::thread worker_;
    };
}

class LoaderTemplate
{
    public:
        virtual ~LoaderTemplate() {}

        virtual const char* Name() const = 0;

        Mesh loadObj(const std::string& filename)
        {
            auto start = std::chrono::high_resolution_clock::now();
            ConsoleColors::write(ConsoleColors::Color::Cyan, "Loading: ");
            ConsoleColors::write(ConsoleColors::Color::White, filename);
            std::cout << ' ';

            Mesh mesh = this->loadObjImplementation(filename);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            ConsoleColors::write(ConsoleColors::Color::Gray, "done. Took ");
            ConsoleColors::write(ConsoleColors::Color::Yellow, duration.count());
            std::cout << " ms.\n";

            return mesh;
        };

        std::vector<Result> loadAllObjs(const std::vector<ObjFileInfo>& paths)
        {
            std::vector<Result> results;
            results.reserve(paths.size());

            for (const ObjFileInfo& file : paths)
            {
                BenchmarkConsole::LoadingIndicator indicator(file);
                auto start = std::chrono::high_resolution_clock::now();
                Mesh mesh = this->loadObjImplementation(file.path);
                auto end = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                indicator.finish(duration);

                results.push_back({ mesh, duration });
            }

            return results;
        }

        virtual Mesh loadObjImplementation(const std::string& filename) = 0;
};
