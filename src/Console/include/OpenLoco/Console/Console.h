#pragma once

#include <fmt/format.h>

namespace OpenLoco::Console
{
    void logDeprecated(const char* format, ...);
    void logVerboseDeprecated(const char* format, ...);
    void errorDeprecated(const char* format, ...);

    void groupDeprecated(const char* format, ...);
    void groupEndDeprecated();
}
