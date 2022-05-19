#pragma once
#include "../Graphics/Gfx.h"
#include "DrawSprite.h"

namespace OpenLoco::Drawing
{

    template<DrawBlendOp TBlendOp>
    bool BlitPixel(const uint8_t src, uint8_t& dst, const Gfx::PaletteMap& paletteMap, const uint8_t treeWilt)
    {
        if constexpr ((TBlendOp & BLEND_TREEWILT) != 0)
        {
            if (treeWilt == 0)
            {
                return false;
            }
        }
        if constexpr (TBlendOp & BLEND_TRANSPARENT)
        {
            // Ignore transparent pixels
            if (src == 0)
            {
                return false;
            }
        }

        if constexpr (((TBlendOp & BLEND_SRC) != 0) && ((TBlendOp & BLEND_DST) != 0))
        {
            auto pixel = paletteMap.blend(src, dst);
            if constexpr (TBlendOp & BLEND_TRANSPARENT)
            {
                if (pixel == 0)
                {
                    return false;
                }
            }
            dst = pixel;
            return true;
        }
        else if constexpr ((TBlendOp & BLEND_SRC) != 0)
        {
            auto pixel = paletteMap[src];
            if constexpr (TBlendOp & BLEND_TRANSPARENT)
            {
                if (pixel == 0)
                {
                    return false;
                }
            }
            dst = pixel;
            return true;
        }
        else if constexpr ((TBlendOp & BLEND_DST) != 0)
        {
            auto pixel = paletteMap[dst];
            if constexpr (TBlendOp & BLEND_TRANSPARENT)
            {
                if (pixel == 0)
                {
                    return false;
                }
            }
            dst = pixel;
            return true;
        }
        else
        {
            dst = src;
            return true;
        }
    }
}
