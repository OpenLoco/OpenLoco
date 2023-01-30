#pragma once

namespace OpenLoco::Console
{
    void log(const char* format, ...);
    void logVerbose(const char* format, ...);
    void error(const char* format, ...);

    void group(const char* format, ...);
    void groupEnd();
}
