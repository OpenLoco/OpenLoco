#pragma once

#include <OpenLoco/Diagnostics/LogSink.h>

namespace OpenLoco::Diagnostics::Logging
{
    class LogTerminal final : public LogSink
    {
        bool _vt100Enabled{};

    public:
        LogTerminal();

        void print(Level level, std::string_view message) override;
    };
}
