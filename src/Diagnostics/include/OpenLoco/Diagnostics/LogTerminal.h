#pragma once

#include <OpenLoco/Diagnostics/LogSink.h>

namespace OpenLoco::Diagnostics::Logging
{
    class LogTerminal final : public LogSink
    {
    public:
        LogTerminal();

        void print(Level level, std::string_view message) override;
    };
}
