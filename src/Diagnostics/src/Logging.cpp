#include "Logging.h"
#include "LogSink.h"
#include <algorithm>
#include <cstdio>
#include <fmt/color.h>
#include <stdarg.h>
#include <vector>

namespace OpenLoco::Diagnostics::Logging
{
    static std::vector<std::shared_ptr<LogSink>> _sinks;

    namespace Detail
    {
        void print(Level level, std::string_view message)
        {
            if (_sinks.empty())
            {
                // Its possible that the logging interface is used from static initializers and
                // may not have any sinks installed at this point, we redirect the output to
                // stdout/stderr in this case.
                fmt::print(level == Level::error ? stderr : stdout, "{}", message);
                return;
            }

            for (auto& sink : _sinks)
            {
                if (!sink->passesLevelFilter(level))
                    continue;

                sink->print(level, message);
            }
        }

        bool passesLevelFilter(Level level)
        {
            if (_sinks.empty())
            {
                return true;
            }

            for (auto& sink : _sinks)
            {
                if (sink->passesLevelFilter(level))
                    return true;
            }

            return false;
        }

    }

    void installSink(std::shared_ptr<LogSink> sink)
    {
        _sinks.push_back(sink);
    }

    void removeSink(std::shared_ptr<LogSink> sink)
    {
        auto it = std::find(_sinks.begin(), _sinks.end(), sink);
        if (it != _sinks.end())
        {
            _sinks.erase(it);
        }
    }

    void incrementIntend()
    {
        for (auto& sink : _sinks)
        {
            sink->incrementIntendSize();
        }
    }

    void decrementIntend()
    {
        for (auto& sink : _sinks)
        {
            sink->decrementIntendSize();
        }
    }

    void enableLevel(Level level)
    {
        for (auto& sink : _sinks)
        {
            sink->enableLevel(level);
        }
    }

    void disableLevel(Level level)
    {
        for (auto& sink : _sinks)
        {
            sink->disableLevel(level);
        }
    }

}
