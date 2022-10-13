#pragma once
#include "../Graphics/Gfx.h"
#include "../Graphics/PaletteMap.h"
#include "DrawSprite.h"

namespace OpenLoco::Drawing
{
    inline uint8_t blend(const Gfx::PaletteMap::View paletteMap, uint8_t src, uint8_t dst)
    {
        // src = 0 would be transparent so there is no blend palette for that, hence src - 1
        assert(src != 0);

        // src is treated as a row in the palette map, validate its in range.
        const auto row = src - 1u;
        assert(row < (paletteMap.size() / Gfx::PaletteMap::kDefaultSize));

        const auto idx = (row * Gfx::PaletteMap::kDefaultSize) + dst;
        assert(idx < paletteMap.size());

        return paletteMap[idx];
    }

    template<DrawBlendOp TBlendOp>
    bool blitPixel(uint8_t src, uint8_t& dst, [[maybe_unused]] const Gfx::PaletteMap::View paletteMap, const uint8_t noiseMask)
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
