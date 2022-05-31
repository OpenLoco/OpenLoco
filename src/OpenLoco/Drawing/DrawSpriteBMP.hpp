#pragma once

#include "../Graphics/Gfx.h"
#include "DrawSprite.h"
#include "DrawSpriteHelper.hpp"

namespace OpenLoco::Drawing
{
    template<DrawBlendOp TBlendOp, uint8_t TZoomLevel>
    inline void drawBMPSprite(Gfx::Context& context, const DrawSpriteArgs& args)
    {
        const auto& g1 = args.sourceImage;
        const auto* src = g1.offset + ((static_cast<size_t>(g1.width) * args.srcPos.y) + args.srcPos.x);
        const auto& paletteMap = args.palMap;
        const int32_t width = args.size.width;
        int32_t height = args.size.height;
        const size_t srcLineWidth = g1.width << TZoomLevel;
        const size_t dstLineWidth = (static_cast<size_t>(context.width) >> TZoomLevel) + context.pitch;
        auto* dst = context.bits;
        // Move the pointer to the start point of the destination
        dst += dstLineWidth * args.dstPos.y + args.dstPos.x;

        constexpr auto zoom = 1 << TZoomLevel;
        if constexpr ((TBlendOp & BlendOp::treeWilt) != 0)
        {
            const auto* treeWilt = args.treeWiltImage->offset + ((static_cast<size_t>(g1.width) * args.srcPos.y) + args.srcPos.x);
            for (; height > 0; height -= zoom)
            {
                auto* nextSrc = src + srcLineWidth;
                auto* nextDst = dst + dstLineWidth;
                auto* nextTreeWilt = treeWilt + srcLineWidth;
                for (int32_t widthRemaining = width; widthRemaining > 0; widthRemaining -= zoom, src += zoom, treeWilt += zoom, dst++)
                {
                    blitPixel<TBlendOp>(*src, *dst, paletteMap, *treeWilt);
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
                auto* nextSrc = src + srcLineWidth;
                auto* nextDst = dst + dstLineWidth;
                for (int32_t widthRemaining = width; widthRemaining > 0; widthRemaining -= zoom, src += zoom, dst++)
                {
                    blitPixel<TBlendOp>(*src, *dst, paletteMap, 0xFF);
                }
                src = nextSrc;
                dst = nextDst;
            }
        }
    }
}
