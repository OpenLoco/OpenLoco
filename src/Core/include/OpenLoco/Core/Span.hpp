/// @file
/// This file enables access to nonstd::span as `stdx` namespace

#pragma once

#include <nonstd/span.hpp>

namespace stdx
{
    using nonstd::span;
}
