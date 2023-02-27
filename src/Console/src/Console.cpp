#include "Console.h"
#include <cstdio>
#include <fmt/color.h>
#include <stdarg.h>

// TODO: Remove this when the VT100 terminal initialiation is moved into Platform
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace OpenLoco::Console
{
    namespace Detail
    {
        static const auto colourInfo = fmt::fg(fmt::color::light_gray);
        static const auto colourWarning = fmt::fg(fmt::color::yellow);
        static const auto colourError = fmt::fg(fmt::color::red);
        static const auto colourVerbose = fmt::fg(fmt::color::gray);

        void Detail::print(Level level, std::string_view message)
        {
            // TODO: Move this into the Terminal sink.
            switch (level)
            {
                case Level::info:
                    fmt::print(stdout, colourInfo, "{}\n", message);
                    return;
                case Level::warning:
                    fmt::print(stdout, colourWarning, "{}\n", message);
                    return;
                case Level::error:
                    fmt::print(stderr, colourError, "{}\n", message);
                    return;
                case Level::verbose:
                    fmt::print(stdout, colourVerbose, "{}\n", message);
                    return;
                default:
                    break;
            }
        }
    }

    void initialize()
    {
        // TODO: Move this into Platform.Terminal
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
#endif
    }

    static int _group = 0;

    static void vwrite(FILE* buffer, const char* format, va_list args)
    {
        for (int i = 0; i < _group; i++)
        {
            fprintf(buffer, "  ");
        }

        vprintf(format, args);
        fprintf(buffer, "\n");
    }

    void logDeprecated(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        vwrite(stdout, format, args);
        va_end(args);
    }

    void logVerboseDeprecated([[maybe_unused]] const char* format, ...)
    {
#ifdef VERBOSE
        va_list args;
        va_start(args, format);
        vwrite(stdout, format, args);
        va_end(args);
#endif
    }

    void errorDeprecated(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        vwrite(stderr, format, args);
        va_end(args);
    }

    void groupDeprecated(const char* format, ...)
    {
        for (int i = 0; i < _group; i++)
        {
            printf("  ");
        }

        printf("> ");
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");

        _group++;
    }

    void groupEndDeprecated()
    {
        _group--;
    }

}
