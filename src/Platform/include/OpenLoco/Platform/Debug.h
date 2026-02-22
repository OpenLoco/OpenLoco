#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#define OPENLOCO_DEBUG_BREAK() __debugbreak()
#else
#include <csignal>
#define OPENLOCO_DEBUG_BREAK() std::raise(SIGTRAP)
#endif
