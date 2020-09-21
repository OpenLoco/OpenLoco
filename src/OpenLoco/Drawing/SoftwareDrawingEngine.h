#pragma once

#include "../Graphics/Gfx.h"
#include "../Ui/Rect.h"
#include <algorithm>
#include <cstddef>

namespace OpenLoco::Drawing
{
    class SoftwareDrawingEngine
    {
    public:
        void drawDirtyBlocks();
        void drawRect(const ui::Rect& rect);
        void setDirtyBlocks(int32_t left, int32_t top, int32_t right, int32_t bottom);

    private:
        void drawDirtyBlocks(size_t x, size_t y, size_t dx, size_t dy);
    };
}
