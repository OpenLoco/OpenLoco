#include "OpenLoco/Diagnostics/LogTerminal.h"
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace OpenLoco::Diagnostics::Logging
{
    static const auto kColourInfo = fmt::fg(fmt::color::light_gray);
    static const auto kColourWarning = fmt::fg(fmt::color::yellow);
    static const auto kColourError = fmt::fg(fmt::color::red);
    static const auto kColourVerbose = fmt::fg(fmt::color::gray);

    LogTerminal::LogTerminal()
    {
#ifdef _WIN32
        // Enable colors.
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
#endif
    }

    void LogTerminal::print(Level level, std::string_view message)
    {
        if (!passesLevelFilter(level))
        {
            return;
        }
        
        std::string timestamp;
        if (getWriteTimestamps())
        {
            timestamp = fmt::format("[{:%Y-%m-%d %H:%M:%S}] ", fmt::localtime(std::time(nullptr)));
        }

        const int intendSize = getIntendSize();
        switch (level)
        {
            case Level::info:
                fmt::print(stdout, kColourInfo, "{}{:>{}}\n", timestamp, message, intendSize);
                return;
            case Level::warning:
                fmt::print(stdout, kColourWarning, "{}{:>{}}\n", timestamp, message, intendSize);
                return;
            case Level::error:
                fmt::print(stderr, kColourError, "{}{:>{}}\n", timestamp, message, intendSize);
                return;
            case Level::verbose:
                fmt::print(stdout, kColourVerbose, "{}{:>{}}\n", timestamp, message, intendSize);
                return;
            default:
                break;
        }
    }

}
