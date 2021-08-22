// This file enables access to std::variant std::visit

#pragma once

// TODO: use a more fine-grained approach to detecting whether <variant>
//       contains support for std::visit.
#if !defined(__APPLE__)

#include <variant>

#else

#include "../../Thirdparty/variant.hpp"
namespace std
{
    using mpark::variant;
    using mpark::visit;
}
#endif
