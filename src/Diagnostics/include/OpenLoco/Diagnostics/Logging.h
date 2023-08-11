#pragma once

#include <OpenLoco/Diagnostics/LogLevel.h>
#include <fmt/format.h>
// Enable printing of fs::path
#include <fmt/std.h>
#include <memory>
#include <string_view>

namespace OpenLoco::Diagnostics::Logging
{
    class LogSink;

    namespace Detail
    {
        // Sends the message to each registered sink. Sinks may print more than just the
        // message.
        void print(Level level, std::string_view message);

        // Returns true if one of the sinks passes the level filter, we want
        // avoid formatting strings when it wouldn't be printed into any sink.
        bool passesLevelFilter(Level level);

        // Helper function to avoid code duplication.
        template<typename... TArgs>
        void printLevel(Level level, fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            if (!passesLevelFilter(level))
                return;

            auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
            Detail::print(level, msg);
        }
    }

    template<typename... TArgs>
    void info(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        Detail::printLevel(Level::info, fmt, std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    void warn(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        Detail::printLevel(Level::warning, fmt, std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    void error(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        Detail::printLevel(Level::error, fmt, std::forward<TArgs>(args)...);
    }

    template<typename... TArgs>
    void verbose(fmt::format_string<TArgs...> fmt, TArgs&&... args)
    {
        Detail::printLevel(Level::verbose, fmt, std::forward<TArgs>(args)...);
    }

    void incrementIntend();

    void decrementIntend();

    void enableLevel(Level level);

    void disableLevel(Level level);

    void installSink(std::shared_ptr<LogSink> sink);

    void removeSink(std::shared_ptr<LogSink> sink);
}
