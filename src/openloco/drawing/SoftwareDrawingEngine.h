#pragma once

#include "../graphics/gfx.h"
#include "../ui/Rect.h"
#include <algorithm>
#include <cstddef>

namespace openloco::drawing
{
    class SoftwareDrawingEngine
    {
    public:
        void drawDirtyBlocks();
        void drawRect(const ui::Rect& rect);

    private:
        void drawDirtyBlocks(size_t x, size_t y, size_t dx, size_t dy);
    };
}
