#pragma once

#include "Font.h"
#include "Graphics/PaletteMap.h"
#include <OpenLoco/Engine/Ui/Point.hpp>
#include <cstdint>

namespace OpenLoco::Gfx
{
    class DrawingContext;

    namespace UnicodeFont
    {
        int16_t getAdvanceWidth(Font font, uint32_t codepoint);
        void draw(DrawingContext& ctx, Ui::Point& pos, Font font, uint32_t codepoint, PaletteMap::View palette, int8_t yOffset = 0);
    }
}
