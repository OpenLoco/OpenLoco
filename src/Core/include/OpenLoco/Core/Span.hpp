/// @file
/// This file enables access to nonstd::span as `stdx` namespace

#pragma once

#include <span>

namespace stdx
{
    using std::span;
}
