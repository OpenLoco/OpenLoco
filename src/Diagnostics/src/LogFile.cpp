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

        fmt::print(_file, "{}{}{:<{}}\n", timestamp, getLevelPrefix(level), message, intendSize);

        // Ensure we are not loosing anything because of buffering in case of a crash.
        _file.flush();
    }
}
