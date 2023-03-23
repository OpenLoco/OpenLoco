#pragma once

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
            auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
            print(Level::info, msg);
        }

        template<typename... TArgs>
        void warn(const char* fmt, TArgs&&... args)
        {
            auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
            print(Level::warning, msg);
        }

        template<typename... TArgs>
        void error(const char* fmt, TArgs&&... args)
        {
            auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
            print(Level::error, msg);
        }

        template<typename... TArgs>
        void verbose([[maybe_unused]] const char* fmt, [[maybe_unused]] TArgs&&... args)
        {
#ifdef VERBOSE
            auto msg = fmt::format(fmt, std::forward<TArgs>(args)...);
            print(Level::verbose, msg);
#endif
        }

        bool getWriteTimestamps() const noexcept;
        void setWriteTimestamps(bool value);

        int getIntendSize() const noexcept;
        void setIntendSize(int size);
        void incrementIntendSize();
        void decrementIntendSize();

        void enableLevel(Level level);
        void disableLevel(Level level);
        bool passesLevelFilter(Level level) const noexcept;

    private:
        int _intendSize{};
        bool _writeTimestamps{};
        uint32_t _levelMask{ ~0U };
    };
}
