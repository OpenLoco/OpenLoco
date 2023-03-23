#include "OpenLoco/Diagnostics/LogFile.h"
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace OpenLoco::Diagnostics::Logging
{
    LogFile::LogFile(const fs::path& file, bool writeTimestamps)
        : _writeTimestamps{ writeTimestamps }
    {
        // Ensure the directory exists in case the filepath is a relative path and contains a sub directory.
        fs::create_directories(file.parent_path());

        _file.open(file, std::ios::binary | std::ios::app);
    }

    void LogFile::print(Level level, std::string_view message)
    {
        if (!_file.is_open())
        {
            return;
        }
        std::string timestamp;
        if (_writeTimestamps)
        {
            timestamp = fmt::format("[{:%Y-%m-%d %H:%M:%S}] ", fmt::localtime(std::time(nullptr)));
        }
        switch (level)
        {
            case Level::info:
                fmt::print(_file, "{}[INF] {}\n", timestamp, message);
                return;
            case Level::warning:
                fmt::print(stdout, "{}[WARN] {}\n", timestamp, message);
                return;
            case Level::error:
                fmt::print(stderr, "{}[ERR] {}\n", timestamp, message);
                return;
            case Level::verbose:
                fmt::print(stdout, "{}[VER] {}\n", timestamp, message);
                return;
            default:
                break;
        }
    }

    void LogFile::setWriteTimestamps(bool value)
    {
        _writeTimestamps = value;
    }
}
