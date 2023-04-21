#include "DrawSprite.h"
#include "DrawSpriteBMP.hpp"
#include "DrawSpriteRLE.hpp"
#include "Graphics/Gfx.h"
#include "Graphics/RenderTarget.h"

namespace OpenLoco::Drawing
{
    DrawBlendOp getDrawBlendOp(const ImageId image, const DrawSpriteArgs& args)
    {
        DrawBlendOp op = DrawBlendOp::none;

        // Image uses the palette pointer to remap the colours of the image
        if (image.hasPrimary())
        {
            if (image.isBlended())
            {
                // Copy non-transparent bitmap data but blend src and dst pixel using the palette map.
                op = DrawBlendOp::transparent | DrawBlendOp::src | DrawBlendOp::dst;
            }
            else
            {
                // Copy non-transparent bitmap data but re-colour using the palette map.
                op = DrawBlendOp::transparent | DrawBlendOp::src;
            }
        }
        else if (image.isBlended())
        {
            // Image is only a transparency mask. Just colour the pixels using the palette map.
            // Used for glass.
            op = DrawBlendOp::transparent | DrawBlendOp::dst;
        }
        else if (!args.sourceImage.hasFlags(Gfx::G1ElementFlags::hasTransparancy))
        {
            // Copy raw bitmap data to target
            op = DrawBlendOp::none;
        }
        else
        {
            // Copy raw bitmap data to target but exclude transparent pixels
            op = DrawBlendOp::transparent;
        }
        // Vanilla did not handle noise image for rle compressed images
        if (args.noiseImage != nullptr && (!args.sourceImage.hasFlags(Gfx::G1ElementFlags::isRLECompressed)))
        {
            op |= DrawBlendOp::noiseMask;
        }
        return op;
    }
#pragma warning(push)
#pragma warning(disable : 4063) // not a valid value for a switch of this enum
    template<uint8_t TZoomLevel, bool TIsRLE>
    inline void drawSpriteToBufferHelper(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        if constexpr (!TIsRLE)
        {
            switch (op)
            {
                case DrawBlendOp::transparent | DrawBlendOp::src | DrawBlendOp::dst:
                    drawBMPSprite<DrawBlendOp::transparent | DrawBlendOp::src | DrawBlendOp::dst, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::transparent | DrawBlendOp::src:
                    drawBMPSprite<DrawBlendOp::transparent | DrawBlendOp::src, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::transparent | DrawBlendOp::dst:
                    drawBMPSprite<DrawBlendOp::transparent | DrawBlendOp::dst, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::none:
                    drawBMPSprite<DrawBlendOp::none, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::transparent:
                    drawBMPSprite<DrawBlendOp::transparent, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::transparent | DrawBlendOp::src | DrawBlendOp::noiseMask:
                    drawBMPSprite<DrawBlendOp::transparent | DrawBlendOp::src | DrawBlendOp::noiseMask, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::none | DrawBlendOp::noiseMask:
                    drawBMPSprite<DrawBlendOp::none | DrawBlendOp::noiseMask, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::transparent | DrawBlendOp::noiseMask:
                    drawBMPSprite<DrawBlendOp::transparent | DrawBlendOp::noiseMask, TZoomLevel>(rt, args);
                    break;
                default:
                    assert(false);
                    break;
            }
        }
        else
        {
            switch (op)
            {
                case DrawBlendOp::transparent | DrawBlendOp::src | DrawBlendOp::dst:
                    drawRLESprite<DrawBlendOp::transparent | DrawBlendOp::src | DrawBlendOp::dst, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::transparent | DrawBlendOp::src:
                    drawRLESprite<DrawBlendOp::transparent | DrawBlendOp::src, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::transparent | DrawBlendOp::dst:
                    drawRLESprite<DrawBlendOp::transparent | DrawBlendOp::dst, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::none:
                    drawRLESprite<DrawBlendOp::none, TZoomLevel>(rt, args);
                    break;
                case DrawBlendOp::transparent:
                    drawRLESprite<DrawBlendOp::transparent, TZoomLevel>(rt, args);
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }
#pragma warning(pop)

    template<>
    void drawSpriteToBuffer<0, false>(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<0, false>(rt, args, op);
    }
    template<>
    void drawSpriteToBuffer<1, false>(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<1, false>(rt, args, op);
    }
    template<>
    void drawSpriteToBuffer<2, false>(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<2, false>(rt, args, op);
    }
    template<>
    void drawSpriteToBuffer<3, false>(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<3, false>(rt, args, op);
    }
    template<>
    void drawSpriteToBuffer<0, true>(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<0, true>(rt, args, op);
    }
    template<>
    void drawSpriteToBuffer<1, true>(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<1, true>(rt, args, op);
    }
    template<>
    void drawSpriteToBuffer<2, true>(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<2, true>(rt, args, op);
    }
    template<>
    void drawSpriteToBuffer<3, true>(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<3, true>(rt, args, op);
    }
}
