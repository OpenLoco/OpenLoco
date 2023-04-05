#include "OpenLoco/Diagnostics/LogTerminal.h"
#include <OpenLoco/Platform/Platform.h>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>

namespace OpenLoco::Diagnostics::Logging
{
    static const auto kColourInfo = fmt::fg(fmt::color::light_gray);
    static const auto kColourWarning = fmt::fg(fmt::color::yellow);
    static const auto kColourError = fmt::fg(fmt::color::red);
    static const auto kColourVerbose = fmt::fg(fmt::color::gray);

    LogTerminal::LogTerminal()
    {
        _vt100Enabled = Platform::enableVT100TerminalMode();
    }

    static FILE* getOutputStream(Level level)
    {
        switch (level)
        {
            case Level::info:
            case Level::warning:
            case Level::verbose:
                return stdout;
            case Level::error:
                return stderr;
            default:
                break;
        }
        return stdout;
    }

    static fmt::text_style getTextStyle(Level level)
    {
        fmt::text_style textStyle{};
        switch (level)
        {
            case Level::info:
                textStyle |= kColourInfo;
                break;
            case Level::warning:
                textStyle |= kColourWarning;
                break;
            case Level::error:
                textStyle |= kColourError;
                break;
            case Level::verbose:
                textStyle |= kColourVerbose;
                break;
            default:
                break;
        }
        return textStyle;
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

        if (_vt100Enabled)
        {
            fmt::print(getOutputStream(level), getTextStyle(level), "{}{:>{}}\n", timestamp, message, intendSize);
        }
        else
        {
            fmt::print(getOutputStream(level), "{}{}{:<{}}\n", timestamp, getLevelPrefix(level), message, intendSize);
        }
    }

}
