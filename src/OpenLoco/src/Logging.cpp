#include "Logging.h"

#include <OpenLoco/Diagnostics/LogFile.h>
#include <OpenLoco/Diagnostics/LogTerminal.h>
#include <OpenLoco/Diagnostics/Logging.h>

namespace OpenLoco::Diagnostics::Logging
{
    static std::shared_ptr<LogTerminal> _terminalLogSink{};
    static std::shared_ptr<LogFile> _fileLogSink{};

    static fs::path getLogFilePath()
    {
        // TODO: Use the user's Documents folder.
        return fs::path{ "logs/openloco.log" };
    }

    void initialize()
    {
        _terminalLogSink = std::make_shared<LogTerminal>();
        Logging::installSink(_terminalLogSink);

        _fileLogSink = std::make_shared<LogFile>(getLogFilePath(), true);
        Logging::installSink(_fileLogSink);
    }

    void shutdown()
    {
        Logging::removeSink(_fileLogSink);
        Logging::removeSink(_terminalLogSink);
    }
}
