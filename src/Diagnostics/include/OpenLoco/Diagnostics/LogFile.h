#pragma once

#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Diagnostics/LogSink.h>
#include <fstream>

namespace OpenLoco::Diagnostics::Logging
{
    class LogFile final : public LogSink
    {
        std::fstream _file;
        bool _writeTimestamps{};

    public:
        LogFile(const fs::path& file, bool writeTimestamps);

        void setWriteTimestamps(bool value);

        void print(Level level, std::string_view message) override;
    };
}
