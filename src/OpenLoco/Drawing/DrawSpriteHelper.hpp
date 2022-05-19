#pragma once
#include "../Graphics/Gfx.h"
#include "DrawSprite.h"

namespace OpenLoco::Drawing
{

    template<DrawBlendOp TBlendOp>
    bool blitPixel(uint8_t src, uint8_t& dst, const Gfx::PaletteMap& paletteMap, const uint8_t treeWilt)
    {
        if constexpr ((TBlendOp & BlendOp::treeWilt) != 0)
        {
            // TreeWilt is either 0 or 0xFF
            src &= treeWilt;
        }
        if constexpr (TBlendOp & BlendOp::transparent)
        {
            // Ignore transparent pixels
            if (src == 0)
            {
                return false;
            }
        }

        if constexpr (((TBlendOp & BlendOp::src) != 0) && ((TBlendOp & BlendOp::dst) != 0))
        {
            auto pixel = paletteMap.blend(src, dst);
            if constexpr (TBlendOp & BlendOp::transparent)
            {
                if (pixel == 0)
                {
                    return false;
                }
            }
            dst = pixel;
            return true;
        }
        else if constexpr ((TBlendOp & BlendOp::src) != 0)
        {
            auto pixel = paletteMap[src];
            if constexpr (TBlendOp & BlendOp::transparent)
            {
                if (pixel == 0)
                {
                    return false;
                }
            }
            dst = pixel;
            return true;
        }
        else if constexpr ((TBlendOp & BlendOp::dst) != 0)
        {
            auto pixel = paletteMap[dst];
            if constexpr (TBlendOp & BlendOp::transparent)
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
