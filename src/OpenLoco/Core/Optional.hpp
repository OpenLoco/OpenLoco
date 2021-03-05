/// @file
/// This file enables access to std::optional
/// for use with systems that have it under the experimental namespace

#pragma once

#if defined(__has_include) // For GCC/Clang check if the header exists.
#if __has_include(<optional>)
#include <optional>
#else
#include <experimental/optional>
namespace std
{
    using std::experimental::nullopt;
    using std::experimental::optional;
}
#endif
#else
#error "<optional> header not found"
#endif
