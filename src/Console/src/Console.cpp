#include "Console.h"
#include <cstdio>
#include <stdarg.h>
#include <varargs.h>

namespace OpenLoco::Console
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

    void logVerbose(const char* format, ...)
    {
#ifdef VERBOSE
        va_list args;
        va_start(args, format);
        vwrite(stdout, format, args);
        va_end(args);
#endif
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

    void groupEnd()
    {
        _group--;
    }
}
