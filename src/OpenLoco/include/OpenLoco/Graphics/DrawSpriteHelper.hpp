#pragma once
#include "DrawSprite.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"

namespace OpenLoco::Gfx
{
    inline uint8_t blend(const PaletteMap::View paletteMap, uint8_t src, uint8_t dst)
    {
        // src = 0 would be transparent so there is no blend palette for that, hence src - 1
        assert(src != 0);

        // src is treated as a row in the palette map, validate its in range.
        const auto row = src - 1u;
        assert(row < (paletteMap.size() / PaletteMap::kDefaultSize));

        const auto idx = (row * PaletteMap::kDefaultSize) + dst;
        assert(idx < paletteMap.size());

        return paletteMap[idx];
    }

    template<DrawBlendOp TBlendOp>
    bool blitPixel(uint8_t src, uint8_t& dst, [[maybe_unused]] const PaletteMap::View paletteMap, const uint8_t noiseMask)
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

    // Draws the sampled pixel of a block of source pixels of a zoomed out sprite. Mask images (water,
    // glass, ...) are drawn by transforming the pixels that are already in the render target and only
    // use the source image as a mask, so for those all that matters is that the sampled pixel is not
    // transparent. The sampled pixel of a block can be transparent for the diamond shaped sprites the
    // map is made of, which would leave the mask image (such as water) undrawn. In that case the
    // block of source pixels is searched for a pixel that is not transparent instead. A block length
    // of one is used where the block cannot be searched (RLE compressed images), in which case only
    // the pixel at blockStart is tried as an alternative.
    template<DrawBlendOp TBlendOp>
    bool blitSampledPixel(const uint8_t src, uint8_t& dst, const PaletteMap::View paletteMap, const uint8_t noiseMask, const uint8_t* blockStart, const int32_t blockLength)
    {
        if (blitPixel<TBlendOp>(src, dst, paletteMap, noiseMask))
        {
            return true;
        }

        if constexpr (((TBlendOp & DrawBlendOp::dst) != DrawBlendOp::none) && ((TBlendOp & DrawBlendOp::src) == DrawBlendOp::none))
        {
            // Noise masks are never used by mask images
            if (noiseMask != 0xFF)
            {
                return false;
            }

            for (auto i = 0; i < blockLength; i++)
            {
                if (blockStart[i] != PaletteIndex::transparent)
                {
                    return blitPixel<TBlendOp>(blockStart[i], dst, paletteMap, noiseMask);
                }
            }
        }

        return false;
    }
}
