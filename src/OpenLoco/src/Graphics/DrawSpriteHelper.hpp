#pragma once
#include "DrawSprite.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"

namespace OpenLoco::Gfx
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
        if constexpr ((TBlendOp & DrawBlendOp::noiseMask) != DrawBlendOp::none)
        {
            // noiseMask is either 0 or 0xFF
            src &= noiseMask;
        }
        if constexpr ((TBlendOp & DrawBlendOp::transparent) != DrawBlendOp::none)
        {
            // Ignore transparent pixels
            if (src == PaletteIndex::transparent)
            {
                return false;
            }
        }

        if constexpr (((TBlendOp & DrawBlendOp::src) != DrawBlendOp::none) && ((TBlendOp & DrawBlendOp::dst) != DrawBlendOp::none))
        {
            auto pixel = blend(paletteMap, src, dst);
            if constexpr ((TBlendOp & DrawBlendOp::transparent) != DrawBlendOp::none)
            {
                if (pixel == PaletteIndex::transparent)
                {
                    return false;
                }
            }
            dst = pixel;
            return true;
        }
        else if constexpr ((TBlendOp & DrawBlendOp::src) != DrawBlendOp::none)
        {
            auto pixel = paletteMap[src];
            if constexpr ((TBlendOp & DrawBlendOp::transparent) != DrawBlendOp::none)
            {
                if (pixel == PaletteIndex::transparent)
                {
                    return false;
                }
            }
            dst = pixel;
            return true;
        }
        else if constexpr ((TBlendOp & DrawBlendOp::dst) != DrawBlendOp::none)
        {
            auto pixel = paletteMap[dst];
            if constexpr ((TBlendOp & DrawBlendOp::transparent) != DrawBlendOp::none)
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
