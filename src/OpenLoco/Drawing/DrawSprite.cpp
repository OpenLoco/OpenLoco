#include "DrawSprite.h"
#include "../Graphics/Gfx.h"
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
        // Vanilla did not handle tree wilt for rle compressed images
        if (args.treeWiltImage != nullptr && !(args.sourceImage.flags & Gfx::G1ElementFlags::isRLECompressed))
        {
            op |= BlendOp::treeWilt;
        }
        return op;
    }

    template<uint8_t TZoomLevel, bool TIsRLE>
    inline void drawSpriteToBufferHelper(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        if constexpr (!TIsRLE)
        {
            switch (op)
            {
                case BlendOp::transparent | BlendOp::src | BlendOp::dst:
                    drawBMPSprite<BlendOp::transparent | BlendOp::src | BlendOp::dst, TZoomLevel>(context, args);
                    break;
                case BlendOp::transparent | BlendOp::src:
                    drawBMPSprite<BlendOp::transparent | BlendOp::src, TZoomLevel>(context, args);
                    break;
                case BlendOp::transparent | BlendOp::dst:
                    drawBMPSprite<BlendOp::transparent | BlendOp::dst, TZoomLevel>(context, args);
                    break;
                case BlendOp::none:
                    drawBMPSprite<BlendOp::none, TZoomLevel>(context, args);
                    break;
                case BlendOp::transparent:
                    drawBMPSprite<BlendOp::transparent, TZoomLevel>(context, args);
                    break;
                case BlendOp::transparent | BlendOp::src | BlendOp::treeWilt:
                    drawBMPSprite<BlendOp::transparent | BlendOp::src | BlendOp::treeWilt, TZoomLevel>(context, args);
                    break;
                case BlendOp::none | BlendOp::treeWilt:
                    drawBMPSprite<BlendOp::none | BlendOp::treeWilt, TZoomLevel>(context, args);
                    break;
                case BlendOp::transparent | BlendOp::treeWilt:
                    drawBMPSprite<BlendOp::transparent | BlendOp::treeWilt, TZoomLevel>(context, args);
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
                    drawRLESprite<BlendOp::transparent | BlendOp::src | BlendOp::dst, TZoomLevel>(context, args);
                    break;
                case BlendOp::transparent | BlendOp::src:
                    drawRLESprite<BlendOp::transparent | BlendOp::src, TZoomLevel>(context, args);
                    break;
                case BlendOp::transparent | BlendOp::dst:
                    drawRLESprite<BlendOp::transparent | BlendOp::dst, TZoomLevel>(context, args);
                    break;
                case BlendOp::none:
                    drawRLESprite<BlendOp::none, TZoomLevel>(context, args);
                    break;
                case BlendOp::transparent:
                    drawRLESprite<BlendOp::transparent, TZoomLevel>(context, args);
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }

    template<>
    void drawSpriteToBuffer<0, false>(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<0, false>(context, args, op);
    }
    template<>
    void drawSpriteToBuffer<1, false>(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<1, false>(context, args, op);
    }
    template<>
    void drawSpriteToBuffer<2, false>(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<2, false>(context, args, op);
    }
    template<>
    void drawSpriteToBuffer<3, false>(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<3, false>(context, args, op);
    }
    template<>
    void drawSpriteToBuffer<0, true>(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<0, true>(context, args, op);
    }
    template<>
    void drawSpriteToBuffer<1, true>(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<1, true>(context, args, op);
    }
    template<>
    void drawSpriteToBuffer<2, true>(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<2, true>(context, args, op);
    }
    template<>
    void drawSpriteToBuffer<3, true>(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        drawSpriteToBufferHelper<3, true>(context, args, op);
    }
}
