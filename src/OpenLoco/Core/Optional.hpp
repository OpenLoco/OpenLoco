/// @file
/// This file enables access to std::optional
/// for use with systems that have it under the experimental namespace

#pragma once

// Find out where std::optional is:
#if defined(__APPLE__) // XCode has the header in experimental namespace
#define NORMAL_OPTIONAL 1
#else // By default assume supported.
#define NORMAL_OPTIONAL 1
#endif

#if NORMAL_OPTIONAL
#include <optional>
#else
#include <experimental/optional>
namespace std
{
    using std::experimental::nullopt;
    using std::experimental::optional;
}
#endif

#undef NORMAL_OPTIONAL // Not needed any more, don't make it public.
