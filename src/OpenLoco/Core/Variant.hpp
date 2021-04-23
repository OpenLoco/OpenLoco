// This file enables access to std::visit

#pragma once

#if !defined(__APPLE__)

#include <variant>
namespace stdx
{
    using std::variant;
    using std::visit;
}
#else

#include "../../Thirdparty/variant.hpp"
namespace stdx
{
    using mpark::variant;
    using mpark::visit;
}
#endif
