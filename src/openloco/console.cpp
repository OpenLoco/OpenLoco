#include "console.h"
#include "utility/string.hpp"
#include <stdarg.h>

namespace openloco::console
{
    static int _group = 0;

    static void vwrite(FILE* buffer, const char* format, va_list args)
    {
        for (int i = 0; i < _group; i++)
        {
            fprintf(buffer, "  ");
        }

        vprintf(format, args);
        fprintf(buffer, "\n");
    }

    void log(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        vwrite(stdout, format, args);
        va_end(args);
    }

    void error(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        vwrite(stderr, format, args);
        va_end(args);
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
