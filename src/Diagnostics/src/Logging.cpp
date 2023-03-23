#include "Logging.h"
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
            for (auto& sink : _sinks)
            {
                sink->print(level, message);
            }
        }
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

}
