#pragma once

#include <OpenLoco/Diagnostics/LogLevel.h>
#include <fmt/format.h>
#include <string_view>

namespace OpenLoco::Diagnostics::Logging
{
    class LogSink
    {
    public:
        virtual ~LogSink() = default;

        virtual void print(Level level, std::string_view message) = 0;

        template<typename TFmt, typename... TArgs>
        void info(const TFmt& fmt, TArgs&&... args)
        {
            printLevel(Level::info, fmt, std::forward<TArgs>(args)...);
        }

        template<typename TFmt, typename... TArgs>
        void warn(const TFmt& fmt, TArgs&&... args)
        {
            printLevel(Level::warning, fmt, std::forward<TArgs>(args)...);
        }

        template<typename TFmt, typename... TArgs>
        void error(const TFmt& fmt, TArgs&&... args)
        {
            printLevel(Level::error, fmt, std::forward<TArgs>(args)...);
        }

        template<typename TFmt, typename... TArgs>
        void verbose(const TFmt& fmt, TArgs&&... args)
        {
            printLevel(Level::verbose, fmt, std::forward<TArgs>(args)...);
        }

        bool getWriteTimestamps() const noexcept;
        void setWriteTimestamps(bool value);

        int getIntendSize() const noexcept;
        void setIntendSize(int size);
        void incrementIntendSize();
        void decrementIntendSize();

        void enableLevel(Level level);
        void disableLevel(Level level);
        void setLevelMask(LevelMask mask);

        bool passesLevelFilter(Level level) const noexcept;

    private:
        // Helper function to avoid code duplication.
        template<typename TFmt, typename... TArgs>
        void printLevel(Level level, const TFmt& fmt, TArgs&&... args)
        {
            if (!passesLevelFilter(level))
                return;

            auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
            print(level, msg);
        }

    private:
        int _intendSize{};
        bool _writeTimestamps{};
        LevelMask _levelMask = getLevelMask(Level::all);
    };
}
