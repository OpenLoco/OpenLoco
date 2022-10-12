// This file enables access to stdx::variant stdx::visit

#pragma once

// TODO: use a more fine-grained approach to detecting whether <variant>
//       contains support for std::visit.
#if !defined(__APPLE__)

#include <variant>
namespace stdx
{
    using std::variant;
    using std::visit;
}
#else

#include <variant.hpp>
namespace stdx
{
    using mpark::variant;
    using mpark::visit;
}
#endif
