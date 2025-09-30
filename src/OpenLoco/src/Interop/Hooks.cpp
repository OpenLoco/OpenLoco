#include "Hooks.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <system_error>
#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif

void OpenLoco::Interop::loadSections()
{
}
