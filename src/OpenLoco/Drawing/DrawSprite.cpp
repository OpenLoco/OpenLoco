#include "DrawSprite.h"
#include "../Graphics/Gfx.h"

namespace OpenLoco::Drawing
{
    DrawBlendOp getDrawBlendOp(const DrawSpriteArgs& args)
    {
        DrawBlendOp op = BLEND_NONE;

        // Image uses the palette pointer to remap the colours of the image
        if (args.image.hasPrimary())
        {
            if (args.image.isBlended())
            {
                // Copy non-transparent bitmap data but blend src and dst pixel using the palette map.
                op = BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST;
            }
            else
            {
                // Copy non-transparent bitmap data but re-colour using the palette map.
                op = BLEND_TRANSPARENT | BLEND_SRC;
            }
        }
        else if (args.image.isBlended())
        {
            // Image is only a transparency mask. Just colour the pixels using the palette map.
            // Used for glass.
            op = BLEND_TRANSPARENT | BLEND_DST;
        }
        else if (!(args.sourceImage.flags & Gfx::G1ElementFlags::hasTransparancy))
        {
            // Copy raw bitmap data to target
            op = BLEND_NONE;
        }
        else
        {
            // Copy raw bitmap data to target but exclude transparent pixels
            op = BLEND_TRANSPARENT;
        }
        if (args.treeWiltImage != nullptr)
        {
            op |= BLEND_TREEWILT;
        }
        return op;
    }
    void drawSprite2(Gfx::Context& context, const DrawSpriteArgs& args, const DrawBlendOp op)
    {
        switch (op)
        {
            case BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST:
                drawSprite2<BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST>(context, args);
                break;
            case BLEND_TRANSPARENT | BLEND_SRC:
                drawSprite2<BLEND_TRANSPARENT | BLEND_SRC>(context, args);
                break;
            case BLEND_TRANSPARENT | BLEND_DST:
                drawSprite2<BLEND_TRANSPARENT | BLEND_DST>(context, args);
                break;
            case BLEND_NONE:
                drawSprite2<BLEND_NONE>(context, args);
                break;
            case BLEND_TRANSPARENT:
                drawSprite2<BLEND_TRANSPARENT>(context, args);
                break;
            case BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST | BLEND_TREEWILT:
                drawSprite2<BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST | BLEND_TREEWILT>(context, args);
                break;
            case BLEND_TRANSPARENT | BLEND_SRC | BLEND_TREEWILT:
                drawSprite2<BLEND_TRANSPARENT | BLEND_SRC | BLEND_TREEWILT>(context, args);
                break;
            case BLEND_TRANSPARENT | BLEND_DST | BLEND_TREEWILT:
                drawSprite2<BLEND_TRANSPARENT | BLEND_DST | BLEND_TREEWILT>(context, args);
                break;
            case BLEND_NONE | BLEND_TREEWILT:
                drawSprite2<BLEND_NONE | BLEND_TREEWILT>(context, args);
                break;
            case BLEND_TRANSPARENT | BLEND_TREEWILT:
                drawSprite2<BLEND_TRANSPARENT | BLEND_TREEWILT>(context, args);
                break;
        }
    }
}
