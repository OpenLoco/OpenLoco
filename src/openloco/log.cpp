#include "log.h"
#include "utility/string.hpp"

namespace openloco::console
{
    static int _group = 0;

    void log(const char* format, ...)
    {
        for (int i = 0; i < _group; i++)
        {
            printf("  ");
        }

        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }

    void group(const char* format, ...)
    {
        for (int i = 0; i < _group; i++)
        {
            printf("  ");
        }

        printf("> ");
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");

        _group++;
    }

    void group_end()
    {
        _group--;
    }
}
