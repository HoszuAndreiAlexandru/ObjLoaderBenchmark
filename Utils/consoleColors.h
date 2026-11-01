#pragma once

#include <iostream>
#include <cstddef>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace ConsoleColors
{
    enum class Color
    {
        Default,
        Gray,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White
    };

#if defined(_WIN32)
    inline HANDLE outputHandle()
    {
        return GetStdHandle(STD_OUTPUT_HANDLE);
    }

    inline bool isTerminal()
    {
        static const bool terminal = [] {
            DWORD mode = 0;
            const HANDLE handle = outputHandle();
            return handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &mode) != 0;
        }();
        return terminal;
    }

    inline std::size_t terminalWidth()
    {
        if (!isTerminal())
            return 88;

        CONSOLE_SCREEN_BUFFER_INFO info = {};
        if (GetConsoleScreenBufferInfo(outputHandle(), &info) && info.srWindow.Right >= info.srWindow.Left)
            return static_cast<std::size_t>(info.srWindow.Right - info.srWindow.Left + 1);
        return 88;
    }

    inline WORD defaultAttributes()
    {
        static const WORD attributes = [] {
            CONSOLE_SCREEN_BUFFER_INFO info = {};
            const HANDLE handle = outputHandle();
            return GetConsoleScreenBufferInfo(handle, &info) ? info.wAttributes
                : static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }();
        return attributes;
    }

    inline WORD attributes(Color color)
    {
        const WORD bright = FOREGROUND_INTENSITY;
        const WORD background = defaultAttributes() &
            (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
        switch (color)
        {
        case Color::Gray:    return background | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        case Color::Red:     return background | FOREGROUND_RED | bright;
        case Color::Green:   return background | FOREGROUND_GREEN | bright;
        case Color::Yellow:  return background | FOREGROUND_RED | FOREGROUND_GREEN | bright;
        case Color::Blue:    return background | FOREGROUND_BLUE | bright;
        case Color::Magenta: return background | FOREGROUND_RED | FOREGROUND_BLUE | bright;
        case Color::Cyan:    return background | FOREGROUND_GREEN | FOREGROUND_BLUE | bright;
        case Color::White:   return background | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | bright;
        default:             return defaultAttributes();
        }
    }
#else
    inline bool isTerminal()
    {
        return isatty(STDOUT_FILENO) != 0;
    }

    inline std::size_t terminalWidth()
    {
        struct winsize size = {};
        if (isTerminal() && ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
            return static_cast<std::size_t>(size.ws_col);
        return 88;
    }

    inline const char* escapeCode(Color color)
    {
        switch (color)
        {
        case Color::Gray:    return "\033[90m";
        case Color::Red:     return "\033[91m";
        case Color::Green:   return "\033[92m";
        case Color::Yellow:  return "\033[93m";
        case Color::Blue:    return "\033[94m";
        case Color::Magenta: return "\033[95m";
        case Color::Cyan:    return "\033[96m";
        case Color::White:   return "\033[97m";
        default:             return "\033[0m";
        }
    }
#endif

    template <typename Value>
    inline void write(Color color, const Value& value)
    {
        if (!isTerminal() || color == Color::Default)
        {
            std::cout << value;
            return;
        }

        std::cout.flush();
#if defined(_WIN32)
        const HANDLE handle = outputHandle();
        const WORD resetAttributes = defaultAttributes();
        SetConsoleTextAttribute(handle, attributes(color));
        std::cout << value;
        std::cout.flush();
        SetConsoleTextAttribute(handle, resetAttributes);
#else
        std::cout << escapeCode(color) << value << "\033[0m";
#endif
    }

    inline void divider(char character = '-', std::size_t width = 88, Color color = Color::Gray)
    {
        write(color, std::string(width, character));
        std::cout << '\n';
    }

    inline void section(const char* title, Color color = Color::Cyan)
    {
        std::cout << '\n';
        divider('=', 88, color);
        write(Color::White, std::string("  ") + title);
        std::cout << '\n';
        divider('=', 88, color);
    }
}
