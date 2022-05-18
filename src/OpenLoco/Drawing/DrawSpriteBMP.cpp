#include "../Graphics/Gfx.h"
#include "DrawSprite.h"

namespace OpenLoco::Drawing
{

    template<DrawBlendOp TBlendOp>
    bool BlitPixel(const uint8_t* src, uint8_t* dst, const Gfx::PaletteMap& paletteMap)
    {
        if constexpr (TBlendOp & BLEND_TRANSPARENT)
        {
            // Ignore transparent pixels
            if (*src == 0)
            {
                return false;
            }
        }

        if constexpr (((TBlendOp & BLEND_SRC) != 0) && ((TBlendOp & BLEND_DST) != 0))
        {
            auto pixel = paletteMap.blend(*src, *dst);
            if constexpr (TBlendOp & BLEND_TRANSPARENT)
            {
                if (pixel == 0)
                {
                    return false;
                }
            }
            *dst = pixel;
            return true;
        }
        else if constexpr ((TBlendOp & BLEND_SRC) != 0)
        {
            auto pixel = paletteMap[*src];
            if constexpr (TBlendOp & BLEND_TRANSPARENT)
            {
                if (pixel == 0)
                {
                    return false;
                }
            }
            *dst = pixel;
            return true;
        }
        else if constexpr ((TBlendOp & BLEND_DST) != 0)
        {
            auto pixel = paletteMap[*dst];
            if constexpr (TBlendOp & BLEND_TRANSPARENT)
            {
                if (pixel == 0)
                {
                    return false;
                }
            }
            *dst = pixel;
            return true;
        }
        else
        {
            *dst = *src;
            return true;
        }
    }

    template<DrawBlendOp TBlendOp>
    static void DrawBMPSprite(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        auto& g1 = args.sourceImage;
        auto src = g1.offset + ((static_cast<size_t>(g1.width) * args.srcY) + args.srcX);
        auto dst = args.destinationBits;
        auto& paletteMap = args.palMap;
        auto width = args.width;
        auto height = args.height;
        auto zoomLevel = context.zoom_level;
        size_t srcLineWidth = g1.width << zoomLevel;
        size_t dstLineWidth = (static_cast<size_t>(context.width) >> zoomLevel) + context.pitch;
        uint8_t zoom = 1 << zoomLevel;
        for (; height > 0; height -= zoom)
        {
            auto nextSrc = src + srcLineWidth;
            auto nextDst = dst + dstLineWidth;
            for (int32_t widthRemaining = width; widthRemaining > 0; widthRemaining -= zoom, src += zoom, dst++)
            {
                BlitPixel<TBlendOp>(src, dst, paletteMap);
            }
            src = nextSrc;
            dst = nextDst;
        }
    }

    /**
     * Copies a sprite onto the buffer. There is no compression used on the sprite
     * image.
     *  rct2: 0x0067A690
     * @param imageId Only flags are used.
     */
    void drawSpriteToBufferBMP(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        auto imageId = args.image;

        // Image uses the palette pointer to remap the colours of the image
        if (imageId.hasPrimary())
        {
            if (imageId.isBlended())
            {
                // Copy non-transparent bitmap data but blend src and dst pixel using the palette map.
                DrawBMPSprite<BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST>(context, args);
            }
            else
            {
                // Copy non-transparent bitmap data but re-colour using the palette map.
                DrawBMPSprite<BLEND_TRANSPARENT | BLEND_SRC>(context, args);
            }
        }
        else if (imageId.isBlended())
        {
            // Image is only a transparency mask. Just colour the pixels using the palette map.
            // Used for glass.
            DrawBMPSprite<BLEND_TRANSPARENT | BLEND_DST>(context, args);
        }
        else if (!(args.sourceImage.flags & Gfx::G1ElementFlags::hasTransparancy))
        {
            // Copy raw bitmap data to target
            DrawBMPSprite<BLEND_NONE>(context, args);
        }
        else
        {
            // Copy raw bitmap data to target but exclude transparent pixels
            DrawBMPSprite<BLEND_TRANSPARENT>(context, args);
        }
    }
}
