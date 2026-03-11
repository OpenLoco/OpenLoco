#pragma once

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
// This is a macro on purpose to avoid introducing another callsite for debug breaks.
// It is used by the assertion code.
#ifdef _MSC_VER
#include <intrin.h>
#define OPENLOCO_DEBUG_BREAK() __debugbreak()
#else
#include <csignal>
#define OPENLOCO_DEBUG_BREAK() std::raise(SIGTRAP)
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)
