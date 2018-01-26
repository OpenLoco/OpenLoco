#include "viewportmgr.h"
#include "interop/interop.hpp"
#include "ui.h"
#include "window.h"
#include <algorithm>

using namespace openloco::ui;
using namespace openloco::interop;

namespace openloco::ui::viewportmgr
{
    loco_global<viewport * [max_viewports], 0x0113D820> _viewports;

    std::array<viewport*, max_viewports> viewports()
    {
        auto arr = (std::array<viewport*, max_viewports>*)_viewports.get();
        return *arr;
    }
}
