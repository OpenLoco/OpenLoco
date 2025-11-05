#include "OpenLoco/Diagnostics/LogFile.h"
#include <fmt/chrono.h>
#include <fmt/format.h>
#ifdef _MSC_VER
// Disable warning C4127: conditional expression is constant, libfmt has some code that triggers this warning.
#pragma warning(disable : 4127)
#endif
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
            auto now = std::chrono::system_clock::now();
            timestamp = fmt::format("[{:%Y-%m-%d %H:%M:%S}] ", std::chrono::floor<std::chrono::seconds>(now));
        }

        const int intendSize = getIntendSize();

        fmt::print(_file, "{}{}{:<{}}\n", timestamp, getLevelPrefix(level), message, intendSize);

        // Ensure we are not loosing anything because of buffering in case of a crash.
        _file.flush();
    }
}
