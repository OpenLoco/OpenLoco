#pragma once

#include <OpenLoco/Diagnostics/LogLevel.h>
#include <fmt/format.h>
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
        template<typename TFmt, typename... TArgs>
        void printLevel(Level level, const TFmt& fmt, TArgs&&... args)
        {
            if (!passesLevelFilter(level))
                return;

            auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
            Detail::print(level, msg);
        }
    }

    template<typename TFmt, typename... TArgs>
    void info(const TFmt& fmt, TArgs&&... args)
    {
        Detail::printLevel(Level::info, fmt, std::forward<TArgs>(args)...);
    }

    template<typename TFmt, typename... TArgs>
    void warn(const TFmt& fmt, TArgs&&... args)
    {
        Detail::printLevel(Level::warning, fmt, std::forward<TArgs>(args)...);
    }

    template<typename TFmt, typename... TArgs>
    void error(const TFmt& fmt, TArgs&&... args)
    {
        Detail::printLevel(Level::error, fmt, std::forward<TArgs>(args)...);
    }

    template<typename TFmt, typename... TArgs>
    void verbose(const TFmt& fmt, TArgs&&... args)
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
