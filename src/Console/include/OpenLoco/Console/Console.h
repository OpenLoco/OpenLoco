#pragma once

#include <fmt/format.h>
#include <string_view>

namespace OpenLoco::Console
{
    enum class Level
    {
        info,
        warning,
        error,
        verbose,
    };

    namespace Detail
    {
        void print(Level level, std::string_view message);
    }

    void initialize();

    template<typename... TArgs>
    void info(const char* fmt, TArgs&&... args)
    {
        auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
        Detail::print(Level::info, msg);
    }

    template<typename... TArgs>
    void warn(const char* fmt, TArgs&&... args)
    {
        auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
        Detail::print(Level::warning, msg);
    }

    template<typename... TArgs>
    void error(const char* fmt, TArgs&&... args)
    {
        auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
        Detail::print(Level::error, msg);
    }

    template<typename... TArgs>
    void verbose([[maybe_unused]] const char* fmt, [[maybe_unused]] TArgs&&... args)
    {
#ifdef VERBOSE
        auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
        Detail::print(Level::verbose, msg);
#endif
    }

    // Use the functions above, those should be phased out.
    void logDeprecated(const char* format, ...);
    void logVerboseDeprecated(const char* format, ...);
    void errorDeprecated(const char* format, ...);

    void groupDeprecated(const char* format, ...);
    void groupEndDeprecated();
}
