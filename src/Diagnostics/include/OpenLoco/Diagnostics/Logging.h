#pragma once

#include <OpenLoco/Diagnostics/LogSink.h>
#include <fmt/format.h>
#include <memory>
#include <string_view>

namespace OpenLoco::Diagnostics::Logging
{
    namespace Detail
    {
        void print(Level level, std::string_view message);
    }

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

    void installSink(std::shared_ptr<LogSink> sink);

    void removeSink(std::shared_ptr<LogSink> sink);

    void groupDeprecated(const char* format, ...);
    void groupEndDeprecated();
}
