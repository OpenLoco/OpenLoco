#pragma once

#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Diagnostics/LogSink.h>
#include <fstream>

namespace OpenLoco::Diagnostics::Logging
{
    class LogFile final : public LogSink
    {
        std::fstream _file;

    public:
        LogFile(const fs::path& file);

        void print(Level level, std::string_view message) override;
    };
}
