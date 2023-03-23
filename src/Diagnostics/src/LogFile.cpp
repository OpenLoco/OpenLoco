#include "OpenLoco/Diagnostics/LogFile.h"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace OpenLoco::Diagnostics::Logging
{
    LogFile::LogFile(const fs::path& file)
    {
        // Ensure the directory exists in case the filepath is a relative path and contains a sub directory.
        fs::create_directories(file.parent_path());

        _file.open(file, std::ios::binary | std::ios::app);
    }

    void LogFile::print(Level level, std::string_view message)
    {
        if (!passesLevelFilter(level))
        {
            return;
        }
        
        if (!_file.is_open())
        {
            return;
        }
        
        std::string timestamp;
        if (getWriteTimestamps())
        {
            timestamp = fmt::format("[{:%Y-%m-%d %H:%M:%S}] ", fmt::localtime(std::time(nullptr)));
        }

        const int intendSize = getIntendSize();
        switch (level)
        {
            case Level::info:
                fmt::print(_file, "{}[INF] {:<{}}\n", timestamp, message, intendSize);
                return;
            case Level::warning:
                fmt::print(_file, "{}[WARN] {:<{}}\n", timestamp, message, intendSize);
                return;
            case Level::error:
                fmt::print(_file, "{}[ERR] {:<{}}\n", timestamp, message, intendSize);
                return;
            case Level::verbose:
                fmt::print(_file, "{}[VER] {:<{}}\n", timestamp, message, intendSize);
                return;
            default:
                break;
        }
    }
}
