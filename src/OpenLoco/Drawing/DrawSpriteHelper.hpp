#pragma once
#include "../Graphics/Gfx.h"
#include "../Graphics/PaletteMap.h"
#include "DrawSprite.h"

namespace OpenLoco::Drawing
{

    static uint8_t blend(const Gfx::PaletteMapView paletteMap, uint8_t src, uint8_t dst)
    {
        // src = 0 would be transparent so there is no blend palette for that, hence (src - 1)
        assert(src != 0 && (src - 1u) < paletteMap.size());
        assert(dst < paletteMap.size());
        auto idx = ((src - 1u) * 256u) + dst;
        return paletteMap[idx];
    }

    template<DrawBlendOp TBlendOp>
    bool blitPixel(uint8_t src, uint8_t& dst, const Gfx::PaletteMapView paletteMap, const uint8_t noiseMask)
    {
        if constexpr ((TBlendOp & BlendOp::noiseMask) != 0)
        {
            // noiseMask is either 0 or 0xFF
            src &= noiseMask;
        }
        if constexpr (TBlendOp & BlendOp::transparent)
        {
            // Ignore transparent pixels
            if (src == PaletteIndex::transparent)
            {
                return false;
            }
        }

        if constexpr (((TBlendOp & BlendOp::src) != 0) && ((TBlendOp & BlendOp::dst) != 0))
        {
            auto pixel = blend(paletteMap, src, dst);
            if constexpr (TBlendOp & BlendOp::transparent)
            {
                if (pixel == PaletteIndex::transparent)
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
                if (pixel == PaletteIndex::transparent)
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
                if (pixel == PaletteIndex::transparent)
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
