#include <algorithm>
#include "interop/interop.hpp"
#include "ui.h"
#include "window.h"
#include "viewportmgr.h"

using namespace openloco::ui;
using namespace openloco::interop;

loco_global<viewport *, 0x0113D820> _viewports;

viewport * openloco::ui::viewportmgr::begin()
{
    return _viewports;
}

viewport * openloco::ui::viewportmgr::end()
{
    return nullptr;
}
