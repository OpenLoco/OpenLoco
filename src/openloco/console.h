#pragma once

namespace openloco::console
{
    void log(const char* format, ...);
    void error(const char* format, ...);

    void group(const char* format, ...);
    void group_end();
}
