#include "DrawSprite.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/RenderTarget.h"
#include "DrawSpriteBMP.hpp"
#include "DrawSpriteRLE.hpp"

namespace OpenLoco::Drawing
{
    DrawBlendOp getDrawBlendOp(const ImageId image, const DrawSpriteArgs& args)
    {
        DrawBlendOp op = BlendOp::none;

        // Image uses the palette pointer to remap the colours of the image
        if (image.hasPrimary())
        {
            if (image.isBlended())
            {
                // Copy non-transparent bitmap data but blend src and dst pixel using the palette map.
                op = BlendOp::transparent | BlendOp::src | BlendOp::dst;
            }
            else
            {
                // Copy non-transparent bitmap data but re-colour using the palette map.
                op = BlendOp::transparent | BlendOp::src;
            }
        }
        else if (image.isBlended())
        {
            // Image is only a transparency mask. Just colour the pixels using the palette map.
            // Used for glass.
            op = BlendOp::transparent | BlendOp::dst;
        }
        else if (!(args.sourceImage.flags & Gfx::G1ElementFlags::hasTransparancy))
        {
            // Copy raw bitmap data to target
            op = BlendOp::none;
        }
        else
        {
            // Copy raw bitmap data to target but exclude transparent pixels
            op = BlendOp::transparent;
        }
        // Vanilla did not handle noise image for rle compressed images
        if (args.noiseImage != nullptr && !(args.sourceImage.flags & Gfx::G1ElementFlags::isRLECompressed))
        {
            op |= BlendOp::noiseMask;
        }
        return op;
    }

    template<uint8_t TZoomLevel, bool TIsRLE>
    inline void drawSpriteToBufferHelper(Gfx::RenderTarget& rt, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        if constexpr (!TIsRLE)
        {
            switch (op)
            {
                case BlendOp::transparent | BlendOp::src | BlendOp::dst:
                    drawBMPSprite<BlendOp::transparent | BlendOp::src | BlendOp::dst, TZoomLevel>(rt, args);
                    break;
                case BlendOp::transparent | BlendOp::src:
                    drawBMPSprite<BlendOp::transparent | BlendOp::src, TZoomLevel>(rt, args);
                    break;
                case BlendOp::transparent | BlendOp::dst:
                    drawBMPSprite<BlendOp::transparent | BlendOp::dst, TZoomLevel>(rt, args);
                    break;
                case BlendOp::none:
                    drawBMPSprite<BlendOp::none, TZoomLevel>(rt, args);
                    break;
                case BlendOp::transparent:
                    drawBMPSprite<BlendOp::transparent, TZoomLevel>(rt, args);
                    break;
                case BlendOp::transparent | BlendOp::src | BlendOp::noiseMask:
                    drawBMPSprite<BlendOp::transparent | BlendOp::src | BlendOp::noiseMask, TZoomLevel>(rt, args);
                    break;
                case BlendOp::none | BlendOp::noiseMask:
                    drawBMPSprite<BlendOp::none | BlendOp::noiseMask, TZoomLevel>(rt, args);
                    break;
                case BlendOp::transparent | BlendOp::noiseMask:
                    drawBMPSprite<BlendOp::transparent | BlendOp::noiseMask, TZoomLevel>(rt, args);
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
                case BlendOp::transparent | BlendOp::src | BlendOp::dst:
                    drawRLESprite<BlendOp::transparent | BlendOp::src | BlendOp::dst, TZoomLevel>(rt, args);
                    break;
                case BlendOp::transparent | BlendOp::src:
                    drawRLESprite<BlendOp::transparent | BlendOp::src, TZoomLevel>(rt, args);
                    break;
                case BlendOp::transparent | BlendOp::dst:
                    drawRLESprite<BlendOp::transparent | BlendOp::dst, TZoomLevel>(rt, args);
                    break;
                case BlendOp::none:
                    drawRLESprite<BlendOp::none, TZoomLevel>(rt, args);
                    break;
                case BlendOp::transparent:
                    drawRLESprite<BlendOp::transparent, TZoomLevel>(rt, args);
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }

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
