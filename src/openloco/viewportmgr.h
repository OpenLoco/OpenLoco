#pragma once

#include "window.h"
#include <array>

namespace openloco::ui::viewportmgr
{
    constexpr size_t max_viewports = 10;
    std::array<viewport*, max_viewports> viewports();
}