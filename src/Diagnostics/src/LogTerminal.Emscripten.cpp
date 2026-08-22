#include "OpenLoco/Diagnostics/LogTerminal.h"
#include <emscripten.h>

namespace OpenLoco::Diagnostics::Logging
{
    LogTerminal::LogTerminal()
    {
    }

    void LogTerminal::print(Level level, std::string_view message)
    {
        if (!passesLevelFilter(level))
        {
            return;
        }

        const char* prefix = getLevelPrefix(level).data();

        // Use EM_ASM to call JavaScript console methods based on log level
        // This outputs directly to the browser's developer console
        switch (level)
        {
            case Level::info:
            case Level::verbose:
                EM_ASM(
                    {
                        const msg = UTF8ToString($0) + UTF8ToString($1);
                        console.info(msg);
                    },
                    prefix,
                    message.data());
                break;

            case Level::warning:
                EM_ASM(
                    {
                        const msg = UTF8ToString($0) + UTF8ToString($1);
                        console.warn(msg);
                    },
                    prefix,
                    message.data());
                break;

            case Level::error:
                EM_ASM(
                    {
                        const msg = UTF8ToString($0) + UTF8ToString($1);
                        console.error(msg);
                    },
                    prefix,
                    message.data());
                break;

            default:
                EM_ASM(
                    {
                        const msg = UTF8ToString($0) + UTF8ToString($1);
                        console.log(msg);
                    },
                    prefix,
                    message.data());
                break;
        }
    }
}
