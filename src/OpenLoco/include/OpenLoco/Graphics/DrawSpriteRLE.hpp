#pragma once

#include "DrawSprite.h"
#include "DrawSpriteHelper.hpp"
#include "Graphics/Gfx.h"
#include "Graphics/RenderTarget.h"

namespace OpenLoco::Gfx
{
    template<DrawBlendOp TBlendOp>
    inline void drawRLESpriteMagnify(const RenderTarget& rt, ZoomLevel zoom, const DrawSpriteArgs& args)
    {
        const auto* src0 = args.sourceImage.offset;
        const int32_t width = args.size.width;
        const int32_t height = args.size.height;
        const auto dstLineWidth = static_cast<size_t>(rt.width) + rt.pitch;
        auto* dst = rt.bits + dstLineWidth * args.dstPos.y + args.dstPos.x;
        const auto& paletteMap = args.palMap;

        for (int32_t y = 0; y < height; y++)
        {
            auto* nextDst = dst + dstLineWidth;

            const auto srcY = zoom.applyTo(args.srcPos.y + y);
            const uint16_t lineOffset = src0[srcY * 2] | (src0[srcY * 2 + 1] << 8);
            const auto* nextRun = src0 + lineOffset;

            auto isEndOfLine = false;
            int32_t firstPixelX = 0;
            int32_t numPixels = 0;
            const uint8_t* runData = nullptr;

            for (int32_t x = 0; x < width; x++, dst++)
            {
                const auto srcX = zoom.applyTo(args.srcPos.x + x);

                while (srcX >= firstPixelX + numPixels && !isEndOfLine)
                {
                    const auto* src = nextRun;
                    auto dataSize = *src++;
                    firstPixelX = *src++;
                    isEndOfLine = (dataSize & 0x80) != 0;
                    dataSize &= 0x7F;
                    numPixels = dataSize;
                    runData = src;
                    nextRun = src + dataSize;
                }

                if (runData != nullptr && srcX >= firstPixelX && srcX < firstPixelX + numPixels)
                {
                    blitPixel<TBlendOp>(runData[srcX - firstPixelX], *dst, paletteMap, 0xFF);
                }
            }
            dst = nextDst;
        }
    }

    template<DrawBlendOp TBlendOp, uint8_t TZoomLevel>
    inline void drawRLESprite(const RenderTarget& rt, const DrawSpriteArgs& args)
    {
        auto src0 = args.sourceImage.offset;
        const auto srcX = args.srcPos.x;
        auto srcY = args.srcPos.y;
        const int32_t width = args.size.width;
        int32_t height = args.size.height;
        constexpr auto zoom = 1 << TZoomLevel;
        auto dstLineWidth = static_cast<size_t>(rt.width) + rt.pitch;
        auto* dst0 = rt.bits;
        // Move the pointer to the start point of the destination
        dst0 += dstLineWidth * args.dstPos.y + args.dstPos.x;

        // Move up to the first line of the image if source_y_start is negative. Why does this even occur?
        if (srcY < 0)
        {
            srcY += zoom;
            height -= zoom;
            dst0 += dstLineWidth;
        }

        // For every line in the image
        for (int32_t i = 0; i < height; i += zoom)
        {
            int32_t y = srcY + i;

            // The first part of the source pointer is a list of offsets to different lines
            // This will move the pointer to the correct source line.
            uint16_t lineOffset = src0[y * 2] | (src0[y * 2 + 1] << 8);
            auto nextRun = src0 + lineOffset;
            auto dstLineStart = dst0 + dstLineWidth * (i >> TZoomLevel);

            // For every data chunk in the line
            auto isEndOfLine = false;
            while (!isEndOfLine)
            {
                // Read chunk metadata
                auto src = nextRun;
                auto dataSize = *src++;
                auto firstPixelX = *src++;
                isEndOfLine = (dataSize & 0x80) != 0;
                dataSize &= 0x7F;

                // Have our next source pointer point to the next data section
                nextRun = src + dataSize;

                int32_t x = firstPixelX - srcX;
                int32_t numPixels = dataSize;
                if (x > 0)
                {
                    // If x is not a multiple of zoom, round it up to a multiple
                    auto mod = x & (zoom - 1);
                    if (mod != 0)
                    {
                        auto offset = zoom - mod;
                        x += offset;
                        src += offset;
                        numPixels -= offset;
                    }
                }
                else if (x < 0)
                {
                    // Clamp x to zero if negative
                    src += -x;
                    numPixels += x;
                    x = 0;
                }

                // If the end position is further out than the whole image
                // end position then we need to shorten the line again
                numPixels = std::min(numPixels, width - x);

                auto dst = dstLineStart + (x >> TZoomLevel);
                if constexpr ((TBlendOp & DrawBlendOp::src) == DrawBlendOp::none && (TBlendOp & DrawBlendOp::dst) == DrawBlendOp::none && TZoomLevel == 0)
                {
                    // Since we're sampling each pixel at this zoom level, just do a straight std::memcpy
                    if (numPixels > 0)
                    {
                        std::copy_n(src, numPixels, dst);
                    }
                }
                else
                {
                    auto& paletteMap = args.palMap;
                    while (numPixels > 0)
                    {
                        blitPixel<TBlendOp>(*src, *dst, paletteMap, 0xFF);
                        numPixels -= zoom;
                        src += zoom;
                        dst++;
                    }
                }
            }
        }
    }
}
