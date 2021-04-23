// This file enables access to std::visit

#pragma once

#if !defined(__APPLE__)

#include <variant>
namespace stdv = std;

#else

#include "../../Thirdparty/variant.hpp"
namespace stdv = mpark;

#endif
