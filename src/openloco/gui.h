#pragma once

namespace openloco::ui
{
    class viewportmanager;
}

namespace openloco::gui
{
    void init(ui::viewportmanager& viewportmgr);
    void resize();
}
