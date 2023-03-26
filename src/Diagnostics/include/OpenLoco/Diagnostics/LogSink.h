#pragma once

#include <fmt/format.h>
#include <string_view>

namespace OpenLoco::Diagnostics::Logging
{
    enum class Level
    {
        info,
        warning,
        error,
        verbose,
    };

    class LogSink
    {
    public:
        virtual ~LogSink() = default;

        virtual void print(Level level, std::string_view message) = 0;

        template<typename... TArgs>
        void info(const char* fmt, TArgs&&... args)
        {
            printLevel(Level::info, fmt, std::forward<TArgs>(args)...);
        }

        template<typename... TArgs>
        void warn(const char* fmt, TArgs&&... args)
        {
            printLevel(Level::warning, fmt, std::forward<TArgs>(args)...);
        }

        template<typename... TArgs>
        void error(const char* fmt, TArgs&&... args)
        {
            printLevel(Level::error, fmt, std::forward<TArgs>(args)...);
        }

        template<typename... TArgs>
        void verbose([[maybe_unused]] const char* fmt, [[maybe_unused]] TArgs&&... args)
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
        void setLevelMask(uint32_t mask);

        bool passesLevelFilter(Level level) const noexcept;

    private:
        // Helper function to avoid code duplication.
        template<typename... TArgs>
        void printLevel(Level level, const char* fmt, TArgs&&... args)
        {
            if (!passesLevelFilter(level))
                return;

            auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
            print(level, msg);
        }

    private:
        int _intendSize{};
        bool _writeTimestamps{};
        uint32_t _levelMask{ ~0U };
    };
}
