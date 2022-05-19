#include "../Graphics/Gfx.h"
#include "DrawSprite.h"
#include "DrawSpriteHelper.hpp"

namespace OpenLoco::Drawing
{
    template<DrawBlendOp TBlendOp>
    static void DrawBMPSprite(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        auto& g1 = args.sourceImage;
        const auto* src = g1.offset + ((static_cast<size_t>(g1.width) * args.srcY) + args.srcX);
        auto* dst = args.destinationBits;
        auto& paletteMap = args.palMap;
        auto width = args.width;
        auto height = args.height;
        auto zoomLevel = context.zoom_level;
        size_t srcLineWidth = g1.width << zoomLevel;
        size_t dstLineWidth = (static_cast<size_t>(context.width) >> zoomLevel) + context.pitch;
        uint8_t zoom = 1 << zoomLevel;
        if constexpr ((TBlendOp & BLEND_TREEWILT) != 0)
        {
            const auto* treeWilt = args.treeWiltImage != nullptr ? args.treeWiltImage->offset + ((static_cast<size_t>(g1.width) * args.srcY) + args.srcX) : nullptr;
            for (; height > 0; height -= zoom)
            {
                auto nextSrc = src + srcLineWidth;
                auto nextDst = dst + dstLineWidth;
                auto nextTreeWilt = treeWilt + srcLineWidth;
                for (int32_t widthRemaining = width; widthRemaining > 0; widthRemaining -= zoom, src += zoom, treeWilt += zoom, dst++)
                {
                    BlitPixel<TBlendOp>(*src, *dst, paletteMap, *treeWilt);
                }
                src = nextSrc;
                dst = nextDst;
                treeWilt = nextTreeWilt;
            }
        }
        else
        {
            for (; height > 0; height -= zoom)
            {
                auto nextSrc = src + srcLineWidth;
                auto nextDst = dst + dstLineWidth;
                for (int32_t widthRemaining = width; widthRemaining > 0; widthRemaining -= zoom, src += zoom, dst++)
                {
                    BlitPixel<TBlendOp>(*src, *dst, paletteMap, 0xFF);
                }
                src = nextSrc;
                dst = nextDst;
            }
        }
    }

    template<>
    void drawSprite2<BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST>(context, args);
    }
    template<>
    void drawSprite2<BLEND_TRANSPARENT | BLEND_SRC>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_TRANSPARENT | BLEND_SRC>(context, args);
    }
    template<>
    void drawSprite2<BLEND_TRANSPARENT | BLEND_DST>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_TRANSPARENT | BLEND_DST>(context, args);
    }
    template<>
    void drawSprite2<BLEND_NONE>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_NONE>(context, args);
    }
    template<>
    void drawSprite2<BLEND_TRANSPARENT>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_TRANSPARENT>(context, args);
    }
    template<>
    void drawSprite2<BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST | BLEND_TREEWILT>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_TRANSPARENT | BLEND_SRC | BLEND_DST | BLEND_TREEWILT>(context, args);
    }
    template<>
    void drawSprite2<BLEND_TRANSPARENT | BLEND_SRC | BLEND_TREEWILT>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_TRANSPARENT | BLEND_SRC | BLEND_TREEWILT>(context, args);
    }
    template<>
    void drawSprite2<BLEND_TRANSPARENT | BLEND_DST | BLEND_TREEWILT>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_TRANSPARENT | BLEND_DST | BLEND_TREEWILT>(context, args);
    }
    template<>
    void drawSprite2<BLEND_NONE | BLEND_TREEWILT>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_NONE | BLEND_TREEWILT>(context, args);
    }
    template<>
    void drawSprite2<BLEND_TRANSPARENT | BLEND_TREEWILT>(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        DrawBMPSprite<BLEND_TRANSPARENT | BLEND_TREEWILT>(context, args);
    }
}
