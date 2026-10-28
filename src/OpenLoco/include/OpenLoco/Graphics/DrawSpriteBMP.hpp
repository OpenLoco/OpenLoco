#pragma once

#include "DrawSprite.h"
#include "DrawSpriteHelper.hpp"
#include "Graphics/Gfx.h"
#include "Graphics/RenderTarget.h"

namespace OpenLoco::Gfx
{
    template<DrawBlendOp TBlendOp>
    inline void drawBMPSpriteMagnify(const RenderTarget& rt, ZoomLevel zoom, const DrawSpriteArgs& args)
    {
        const auto& g1 = args.sourceImage;
        const auto& paletteMap = args.palMap;
        const int32_t width = args.size.width;
        const int32_t height = args.size.height;
        const size_t srcLineWidth = g1.width;
        const size_t dstLineWidth = static_cast<size_t>(rt.width) + rt.pitch;
        auto* dst = rt.bits + dstLineWidth * args.dstPos.y + args.dstPos.x;

        for (int32_t y = 0; y < height; y++)
        {
            auto* nextDst = dst + dstLineWidth;
            const auto srcY = zoom.applyTo(args.srcPos.y + y);
            const auto* srcLine = g1.offset + srcLineWidth * srcY;

            if constexpr ((TBlendOp & DrawBlendOp::noiseMask) != DrawBlendOp::none)
            {
                const auto* noiseLine = args.noiseImage->offset + srcLineWidth * srcY;
                for (int32_t x = 0; x < width; x++, dst++)
                {
                    const auto srcX = zoom.applyTo(args.srcPos.x + x);
                    blitPixel<TBlendOp>(srcLine[srcX], *dst, paletteMap, noiseLine[srcX]);
                }
            }
            else
            {
                for (int32_t x = 0; x < width; x++, dst++)
                {
                    const auto srcX = zoom.applyTo(args.srcPos.x + x);
                    blitPixel<TBlendOp>(srcLine[srcX], *dst, paletteMap, 0xFF);
                }
            }
            dst = nextDst;
        }
    }

    template<DrawBlendOp TBlendOp, uint8_t TZoomLevel>
    inline void drawBMPSprite(const RenderTarget& rt, const DrawSpriteArgs& args)
    {
        constexpr auto zoom = 1 << TZoomLevel;

        const auto& g1 = args.sourceImage;
        // Take the sample of each block of source pixels from the middle of the block (see
        // zoomOutSampleOffset); the corners of the diamond shaped map sprites are transparent
        const auto sampleOffsetX = zoomOutSampleOffset(zoom, args.size.width);
        const auto sampleOffsetY = zoomOutSampleOffset(zoom, args.size.height);

        const auto* src = g1.offset + ((static_cast<size_t>(g1.width) * (args.srcPos.y + sampleOffsetY)) + (args.srcPos.x + sampleOffsetX));
        const auto& paletteMap = args.palMap;
        const int32_t width = args.size.width;
        int32_t height = args.size.height;
        const size_t srcLineWidth = g1.width << TZoomLevel;
        const size_t dstLineWidth = static_cast<size_t>(rt.width) + rt.pitch;
        auto* dst = rt.bits;
        // Move the pointer to the start point of the destination
        dst += dstLineWidth * args.dstPos.y + args.dstPos.x;

        if constexpr ((TBlendOp & DrawBlendOp::noiseMask) != DrawBlendOp::none)
        {
            const auto* noiseMask = args.noiseImage->offset + ((static_cast<size_t>(g1.width) * (args.srcPos.y + sampleOffsetY)) + (args.srcPos.x + sampleOffsetX));
            for (; height > 0; height -= zoom)
            {
                auto* nextSrc = src + srcLineWidth;
                auto* nextDst = dst + dstLineWidth;
                auto* nextNoiseMask = noiseMask + srcLineWidth;
                for (int32_t widthRemaining = width; widthRemaining > 0; widthRemaining -= zoom, src += zoom, noiseMask += zoom, dst++)
                {
                    blitSampledPixel<TBlendOp>(*src, *dst, paletteMap, *noiseMask, src - sampleOffsetX, std::min<int32_t>(zoom, widthRemaining));
                }
                src = nextSrc;
                dst = nextDst;
                noiseMask = nextNoiseMask;
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
                    blitSampledPixel<TBlendOp>(*src, *dst, paletteMap, 0xFF, src - sampleOffsetX, std::min<int32_t>(zoom, widthRemaining));
                }
                src = nextSrc;
                dst = nextDst;
            }
        }
    }
}
