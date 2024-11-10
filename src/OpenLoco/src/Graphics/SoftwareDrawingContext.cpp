#include "SoftwareDrawingContext.h"
#include "DrawSprite.h"
#include "Font.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Localisation/Formatting.h"
#include "TextRenderer.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL2/SDL.h>
#include <algorithm>
#include <cassert>
#include <stack>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;

namespace OpenLoco::Gfx
{
    struct SoftwareDrawingContextState
    {
        std::stack<RenderTarget> rtStack;
    };

    namespace Impl
    {
        // 0x009DA3E0
        // Originally 0x009DA3E0 was an array of the image data pointers setup within 0x00452336
        // We have removed that step and instead work directly on the images.
        static constexpr std::array<uint32_t, 8> _noiseMaskImages = {
            ImageIds::null,
            ImageIds::noise_mask_1,
            ImageIds::noise_mask_2,
            ImageIds::noise_mask_3,
            ImageIds::noise_mask_4,
            ImageIds::noise_mask_5,
            ImageIds::noise_mask_6,
            ImageIds::noise_mask_7,
        };

        static void drawRect(const RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags);
        static void drawImageSolid(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex);

        // 0x00447485
        // edi: rt
        // ebp: fill
        static void clear(const RenderTarget& rt, uint32_t fill)
        {
            int32_t w = rt.width / (1 << rt.zoomLevel);
            int32_t h = rt.height / (1 << rt.zoomLevel);
            uint8_t* ptr = rt.bits;

            for (int32_t y = 0; y < h; y++)
            {
                std::fill_n(ptr, w, fill);
                ptr += w + rt.pitch;
            }
        }

        static void clearSingle(const RenderTarget& rt, uint8_t paletteId)
        {
            auto fill = (paletteId << 24) | (paletteId << 16) | (paletteId << 8) | paletteId;
            clear(rt, fill);
        }

        static const G1Element* getNoiseMaskImageFromImage(const ImageId image)
        {
            if (image.hasNoiseMask())
            {
                const auto noise = image.getNoiseMask();
                const auto* noiseImage = getG1Element(_noiseMaskImages[noise]);
                if (noiseImage == nullptr)
                {
                    return nullptr;
                }
                return noiseImage;
            }
            else
            {
                return nullptr;
            }
        }

        template<uint8_t TZoomLevel, bool TIsRLE>
        static std::optional<DrawSpritePosArgs> getDrawImagePosArgs(const RenderTarget& rt, const Ui::Point& pos, const G1Element& element)
        {
            if constexpr (TZoomLevel > 0)
            {
                if (element.hasFlags(G1ElementFlags::noZoomDraw))
                {
                    return std::nullopt;
                }
            }

            auto dispPos{ pos };
            // Its used super often so we will define it to a separate variable.
            constexpr auto zoomMask = static_cast<uint32_t>(0xFFFFFFFFULL << TZoomLevel);

            if constexpr (TZoomLevel > 0 && TIsRLE)
            {
                dispPos.x -= ~zoomMask;
                dispPos.y -= ~zoomMask;
            }

            // This will be the height of the drawn image
            auto height = element.height;

            // This is the start y coordinate on the destination
            auto dstTop = dispPos.y + element.yOffset;

            // For whatever reason the RLE version does not use
            // the zoom mask on the y coordinate but does on x.
            if constexpr (TIsRLE)
            {
                dstTop -= rt.y;
            }
            else
            {
                dstTop = (dstTop & zoomMask) - rt.y;
            }
            // This is the start y coordinate on the source
            auto srcY = 0;

            if (dstTop < 0)
            {
                // If the destination y is negative reduce the height of the
                // image as we will cut off the bottom
                height += dstTop;
                // If the image is no longer visible nothing to draw
                if (height <= 0)
                {
                    return std::nullopt;
                }
                // The source image will start a further up the image
                srcY -= dstTop;
                // The destination start is now reset to 0
                dstTop = 0;
            }
            else
            {
                if constexpr (TZoomLevel > 0 && TIsRLE)
                {
                    srcY -= dstTop & ~zoomMask;
                    height += dstTop & ~zoomMask;
                }
            }

            auto dstBottom = dstTop + height;

            if (dstBottom > rt.height)
            {
                // If the destination y is outside of the drawing
                // image reduce the height of the image
                height -= dstBottom - rt.height;
            }
            // If the image no longer has anything to draw
            if (height <= 0)
            {
                return std::nullopt;
            }

            dstTop = dstTop >> TZoomLevel;

            // This will be the width of the drawn image
            auto width = element.width;

            // This is the source start x coordinate
            auto srcX = 0;
            // This is the destination start x coordinate
            int32_t dstLeft = ((dispPos.x + element.xOffset + ~zoomMask) & zoomMask) - rt.x;

            if (dstLeft < 0)
            {
                // If the destination is negative reduce the width
                // image will cut off the side
                width += dstLeft;
                // If there is no image to draw
                if (width <= 0)
                {
                    return std::nullopt;
                }
                // The source start will also need to cut off the side
                srcX -= dstLeft;
                // Reset the destination to 0
                dstLeft = 0;
            }
            else
            {
                if constexpr (TZoomLevel > 0 && TIsRLE)
                {
                    srcX -= dstLeft & ~zoomMask;
                }
            }

            const auto dstRight = dstLeft + width;

            if (dstRight > rt.width)
            {
                // If the destination x is outside of the drawing area
                // reduce the image width.
                width -= dstRight - rt.width;
                // If there is no image to draw.
                if (width <= 0)
                {
                    return std::nullopt;
                }
            }

            dstLeft = dstLeft >> TZoomLevel;

            return DrawSpritePosArgs{ Ui::Point32{ srcX, srcY }, Ui::Point32{ dstLeft, dstTop }, Ui::Size(width, height) };
        }

        template<uint8_t TZoomLevel, bool TIsRLE>
        static void drawImagePaletteSet(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const G1Element& element, const PaletteMap::View palette, const G1Element* noiseImage)
        {
            auto args = getDrawImagePosArgs<TZoomLevel, TIsRLE>(rt, pos, element);
            if (args.has_value())
            {
                const DrawSpriteArgs fullArgs{ palette, element, args->srcPos, args->dstPos, args->size, noiseImage };
                const auto op = getDrawBlendOp(image, fullArgs);
                drawSpriteToBuffer<TZoomLevel, TIsRLE>(rt, fullArgs, op);
            }
        }

        // 0x00448D90
        static void drawImagePaletteSet(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const PaletteMap::View palette, const G1Element* noiseImage)
        {
            const auto* element = getG1Element(image.getIndex());
            if (element == nullptr)
            {
                return;
            }

            if (rt.zoomLevel > 0 && (element->hasFlags(G1ElementFlags::hasZoomSprites)))
            {
                auto zoomedrt{ rt };
                zoomedrt.bits = rt.bits;
                zoomedrt.x = rt.x >> 1;
                zoomedrt.y = rt.y >> 1;
                zoomedrt.height = rt.height >> 1;
                zoomedrt.width = rt.width >> 1;
                zoomedrt.pitch = rt.pitch;
                zoomedrt.zoomLevel = rt.zoomLevel - 1;

                const auto zoomCoords = Ui::Point(pos.x >> 1, pos.y >> 1);
                drawImagePaletteSet(
                    zoomedrt, zoomCoords, image.withIndexOffset(-element->zoomOffset), palette, noiseImage);
                return;
            }

            const bool isRLE = element->hasFlags(G1ElementFlags::isRLECompressed);
            if (isRLE)
            {
                switch (rt.zoomLevel)
                {
                    default:
                        drawImagePaletteSet<0, true>(rt, pos, image, *element, palette, noiseImage);
                        break;
                    case 1:
                        drawImagePaletteSet<1, true>(rt, pos, image, *element, palette, noiseImage);
                        break;
                    case 2:
                        drawImagePaletteSet<2, true>(rt, pos, image, *element, palette, noiseImage);
                        break;
                    case 3:
                        drawImagePaletteSet<3, true>(rt, pos, image, *element, palette, noiseImage);
                        break;
                }
            }
            else
            {
                switch (rt.zoomLevel)
                {
                    default:
                        drawImagePaletteSet<0, false>(rt, pos, image, *element, palette, noiseImage);
                        break;
                    case 1:
                        drawImagePaletteSet<1, false>(rt, pos, image, *element, palette, noiseImage);
                        break;
                    case 2:
                        drawImagePaletteSet<2, false>(rt, pos, image, *element, palette, noiseImage);
                        break;
                    case 3:
                        drawImagePaletteSet<3, false>(rt, pos, image, *element, palette, noiseImage);
                        break;
                }
            }
        }

        // 0x00448C79
        static void drawImage(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image)
        {
            const auto* noiseImage = getNoiseMaskImageFromImage(image);
            const auto palette = PaletteMap::getForImage(image);

            if (!palette.has_value())
            {
                drawImagePaletteSet(rt, pos, image, PaletteMap::getDefault(), noiseImage);
            }
            else
            {
                drawImagePaletteSet(rt, pos, image, *palette, noiseImage);
            }
        }

        // 0x00448C79
        static void drawImage(const RenderTarget* rt, int16_t x, int16_t y, uint32_t image)
        {
            drawImage(*rt, { x, y }, ImageId::fromUInt32(image));
        }

        // 0x00450890, 0x00450F87, 0x00450D1E, 0x00450ABA
        template<int32_t TZoomLevel>
        static void drawMaskedZoom(
            int16_t imageHeight,
            int16_t imageWidth,
            int16_t rowSize,
            const uint8_t* bytesMask,
            int16_t dstWrap,
            uint8_t* dstBuf,
            const uint8_t* bytesImage)
        {
            const auto scaledHeight = imageHeight >> TZoomLevel;
            if (scaledHeight == 0)
            {
                return;
            }

            const auto scaledWidth = imageWidth >> TZoomLevel;
            if (scaledWidth == 0)
            {
                return;
            }

            constexpr auto skip = (1U << TZoomLevel);

            // Calculate the row size.
            const auto scaledRowSize = [&]() {
                if constexpr (TZoomLevel == 0)
                {
                    return rowSize;
                }
                else
                {
                    constexpr auto zoomMask = (1 << TZoomLevel) - 1;

                    const auto scaledRowSize = zoomMask * static_cast<uint16_t>(rowSize + imageWidth) + rowSize;
                    const auto maskedRowSize = static_cast<uint16_t>(imageWidth) & zoomMask;

                    return maskedRowSize + scaledRowSize;
                }
            }();

            if constexpr (TZoomLevel > 0)
            {
                dstWrap -= scaledWidth;
            }

            for (auto y = 0; y < scaledHeight; y++)
            {
                for (auto x = 0; x < scaledWidth; x++)
                {
                    const auto masked = *bytesMask & *bytesImage;
                    if (masked)
                    {
                        *dstBuf = masked;
                    }

                    bytesImage += skip;
                    bytesMask += skip;
                    dstBuf++;
                }

                bytesImage += scaledRowSize;
                bytesMask += scaledRowSize;
                dstBuf += dstWrap;
            }
        }

        template<int32_t TZoomLevel>
        static void drawImageMaskedZoom(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const ImageId& maskImage)
        {
            const auto* g1Image = Gfx::getG1Element(image.getIndex());
            if (g1Image == nullptr)
            {
                return;
            }
            if (g1Image->hasFlags(G1ElementFlags::isRLECompressed))
            {
                // This is not supported.
                assert(false);

                return;
            }

            const auto* g1ImageMask = Gfx::getG1Element(maskImage.getIndex());
            if (g1ImageMask == nullptr)
            {
                return;
            }
            if (g1ImageMask->hasFlags(G1ElementFlags::isRLECompressed))
            {
                // This is not supported.
                assert(false);

                return;
            }

            if constexpr (TZoomLevel > 0)
            {
                if (g1Image->hasFlags(G1ElementFlags::none))
                {
                    return;
                }

                if (g1Image->hasFlags(G1ElementFlags::hasZoomSprites))
                {
                    if (g1ImageMask->hasFlags(G1ElementFlags::noZoomDraw))
                    {
                        return;
                    }

                    if (g1ImageMask->hasFlags(G1ElementFlags::hasZoomSprites))
                    {
                        auto newRt = rt;
                        --newRt.zoomLevel;
                        newRt.x >>= 1;
                        newRt.y >>= 1;
                        newRt.width >>= 1;
                        newRt.height >>= 1;
                        drawImageMaskedZoom<TZoomLevel - 1>(
                            newRt,
                            { static_cast<int16_t>(pos.x >> 1), static_cast<int16_t>(pos.y >> 1) },
                            image.withIndexOffset(-g1Image->zoomOffset),
                            maskImage.withIndexOffset(-g1ImageMask->zoomOffset));
                        return;
                    }
                }
            }

            int16_t imageHeight = g1Image->height;
            int16_t imageWidth = g1Image->width;

            const auto* imageDataPos = g1Image->offset;
            auto* dstBuf = rt.bits;

            constexpr uint16_t zoomMask = static_cast<uint16_t>(~0ULL << TZoomLevel);
            constexpr int16_t offsetX = (1 << TZoomLevel) - 1;

            int16_t dstTop = ((g1Image->yOffset + pos.y) & zoomMask) - rt.y;
            if (dstTop >= 0)
            {
                auto scaledWidth = rt.width >> TZoomLevel;
                scaledWidth = rt.pitch + scaledWidth;
                dstBuf += (dstTop >> TZoomLevel) * scaledWidth;
            }
            else
            {
                if (dstTop + imageHeight == 0)
                {
                    return;
                }

                imageHeight += dstTop;

                if (imageHeight < 0)
                {
                    return;
                }

                const auto startOffset = static_cast<uint16_t>(-dstTop) * g1Image->width;
                imageDataPos = &g1Image->offset[startOffset];

                dstTop = 0;
            }

            int16_t dstBottom = imageHeight + dstTop;
            if (dstBottom > rt.height)
            {
                imageHeight -= dstBottom - rt.height;

                if (imageHeight <= 0)
                {
                    return;
                }
            }

            int16_t rowSize = 0;
            int16_t dstWrap = rt.pitch + (rt.width >> TZoomLevel);

            if constexpr (TZoomLevel == 0)
            {
                dstWrap -= g1Image->width;
            }

            int16_t dstLeft = ((g1Image->xOffset + pos.x + offsetX) & zoomMask) - rt.x;
            if (dstLeft < 0)
            {
                imageWidth += dstLeft;
                if (imageWidth <= 0)
                {
                    return;
                }

                rowSize -= dstLeft;
                imageDataPos -= dstLeft;

                if constexpr (TZoomLevel == 0)
                {
                    dstWrap -= dstLeft;
                }

                dstLeft = 0;
            }

            int16_t dstRight = imageWidth + dstLeft - rt.width;
            if (imageWidth + dstLeft > rt.width)
            {
                imageWidth -= dstRight;
                if (imageWidth <= 0)
                {
                    return;
                }

                rowSize += dstRight;

                if constexpr (TZoomLevel == 0)
                {
                    dstWrap += dstRight;
                }
            }

            dstBuf += (dstLeft >> TZoomLevel);

            auto imageOffset = imageDataPos - g1Image->offset;
            drawMaskedZoom<TZoomLevel>(
                imageHeight,
                imageWidth,
                rowSize,
                &g1ImageMask->offset[imageOffset],
                dstWrap,
                dstBuf,
                imageDataPos);
        }

        // 0x00450705
        static void drawImageMasked(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const ImageId& maskImage)
        {
            switch (rt.zoomLevel)
            {
                case 0:
                    drawImageMaskedZoom<0>(rt, pos, image, maskImage);
                    return;
                case 1:
                    drawImageMaskedZoom<1>(rt, pos, image, maskImage);
                    return;
                case 2:
                    drawImageMaskedZoom<2>(rt, pos, image, maskImage);
                    return;
                case 3:
                    drawImageMaskedZoom<3>(rt, pos, image, maskImage);
                    return;
                default:
                    break;
            }

            // Unreachable
            assert(false);
        }

        static void drawImageSolid(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex)
        {
            PaletteMap::Buffer<PaletteMap::kDefaultSize> palette;
            std::fill(palette.begin(), palette.end(), paletteIndex);
            palette[0] = 0;

            // Set the image primary flag to tell drawImagePaletteSet to recolour with the palette (Colour::black is not actually used)
            drawImagePaletteSet(rt, pos, image.withPrimary(Colour::black), PaletteMap::View{ palette }, {});
        }

        // 0x004474BA
        // ax: left
        // bx: right
        // bp: width
        // cx: top
        // dx: bottom
        // ebp: colour | enumValue(flags)
        // edi: rt
        static void drawRectImpl(const RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, RectFlags flags)
        {
            if (left > right)
            {
                return;
            }
            if (top > bottom)
            {
                return;
            }
            if (right < rt.x)
            {
                return;
            }
            if (left >= (rt.x + rt.width))
            {
                return;
            }
            if (bottom < rt.y)
            {
                return;
            }
            if (top >= rt.y + rt.height)
            {
                return;
            }

            uint32_t crossPattern = 0;

            auto leftX = left - rt.x;
            if (leftX < 0)
            {
                crossPattern ^= leftX;
                leftX = 0;
            }

            auto rightX = right - rt.x + 1;
            if (rightX > rt.width)
            {
                rightX = rt.width;
            }

            auto topY = top - rt.y;
            if (topY < 0)
            {
                crossPattern ^= topY;
                topY = 0;
            }

            auto bottomY = bottom - rt.y + 1;
            if (bottomY > rt.height)
            {
                bottomY = rt.height;
            }

            auto drawRect = Rect::fromLTRB(leftX, topY, rightX, bottomY);

            if (flags == RectFlags::none) // Regular fill
            {
                auto* dst = drawRect.top() * (rt.width + rt.pitch) + drawRect.left() + rt.bits;
                const auto step = rt.width + rt.pitch;

                for (auto y = 0; y < drawRect.height(); y++)
                {
                    std::fill_n(dst, drawRect.width(), colour);
                    dst += step;
                }
            }
            else if ((flags & RectFlags::transparent) != RectFlags::none)
            {
                auto* dst = rt.bits
                    + static_cast<uint32_t>((drawRect.top() >> rt.zoomLevel) * ((rt.width >> rt.zoomLevel) + rt.pitch) + (drawRect.left() >> rt.zoomLevel));

                auto paletteMap = PaletteMap::getForColour(static_cast<ExtColour>(colour));
                if (paletteMap.has_value())
                {
                    const auto& paletteEntries = paletteMap.value();
                    const auto scaledWidth = drawRect.width() >> rt.zoomLevel;
                    const auto scaledHeight = drawRect.height() >> rt.zoomLevel;
                    const auto step = (rt.width >> rt.zoomLevel) + rt.pitch;

                    // Fill the rectangle with the colours from the colour table
                    for (auto y = 0; y < scaledHeight; y++)
                    {
                        auto* nextDst = dst + step * y;
                        for (auto x = 0; x < scaledWidth; x++)
                        {
                            auto index = *(nextDst + x);
                            *(nextDst + x) = paletteEntries[index];
                        }
                    }
                }
            }
            else if ((flags & RectFlags::crossHatching) != RectFlags::none)
            {
                auto* dst = (drawRect.top() * (rt.width + rt.pitch)) + drawRect.left() + rt.bits;
                const auto step = rt.width + rt.pitch;

                for (auto y = 0; y < drawRect.height(); y++)
                {
                    auto* nextDst = dst + step * y;
                    auto p = std::rotr(crossPattern, 1);

                    // Fill every other pixel with the colour
                    for (auto x = 0; x < drawRect.width(); x++)
                    {
                        p ^= 0x80000000;
                        if (p & 0x80000000)
                        {
                            *(nextDst + x) = colour;
                        }
                    }
                    crossPattern ^= 1;
                }
            }
            else if ((flags & RectFlags::g1Pattern) != RectFlags::none)
            {
                assert(false); // unused
            }
            else if ((flags & RectFlags::selectPattern) != RectFlags::none)
            {
                assert(false); // unused
            }
        }

        static void drawRectImpl(const RenderTarget& rt, const Ui::Rect& rect, uint8_t colour, RectFlags flags)
        {
            drawRectImpl(rt, rect.left(), rect.top(), rect.right(), rect.bottom(), colour, flags);
        }

        static void fillRect(const RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, RectFlags flags)
        {
            drawRectImpl(rt, left, top, right, bottom, colour, flags);
        }

        static void drawRect(const RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags)
        {
            // This makes the function signature more like a drawing application
            drawRectImpl(rt, x, y, x + dx - 1, y + dy - 1, colour, flags);
        }

        static void fillRectInset(const RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, AdvancedColour colour, RectInsetFlags flags)
        {
            const auto rect = Ui::Rect::fromLTRB(left, top, right, bottom);
            const auto baseColour = static_cast<Colour>(colour);

            assert(!colour.isOutline());
            assert(!colour.isInset());
            if (colour.isTranslucent())
            {
                // Must pass RectFlags::transparent to drawRectImpl for this code path
                if ((flags & RectInsetFlags::borderNone) != RectInsetFlags::none)
                {
                    drawRectImpl(rt, rect, enumValue(Colours::getTranslucent(baseColour, 1)), RectFlags::transparent);
                }
                else if ((flags & RectInsetFlags::borderInset) != RectInsetFlags::none)
                {
                    // Draw outline of box
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, top, left, bottom), enumValue(Colours::getTranslucent(baseColour, 2)), RectFlags::transparent);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, top, right, top), enumValue(Colours::getTranslucent(baseColour, 2)), RectFlags::transparent);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(right, top, right, bottom), enumValue(Colours::getTranslucent(baseColour, 0)), RectFlags::transparent);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, bottom, right, bottom), enumValue(Colours::getTranslucent(baseColour, 0)), RectFlags::transparent);

                    if ((flags & RectInsetFlags::fillNone) == RectInsetFlags::none)
                    {
                        drawRectImpl(rt, Ui::Rect::fromLTRB(left + 1, top + 1, right - 1, bottom - 1), enumValue(Colours::getTranslucent(baseColour, 1)), RectFlags::transparent);
                    }
                }
                else
                {
                    // Draw outline of box
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, top, left, bottom), enumValue(Colours::getTranslucent(baseColour, 0)), RectFlags::transparent);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, top, right, top), enumValue(Colours::getTranslucent(baseColour, 0)), RectFlags::transparent);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(right, top, right, bottom), enumValue(Colours::getTranslucent(baseColour, 2)), RectFlags::transparent);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, bottom, right, bottom), enumValue(Colours::getTranslucent(baseColour, 2)), RectFlags::transparent);

                    if ((flags & RectInsetFlags::fillNone) == RectInsetFlags::none)
                    {
                        drawRectImpl(
                            rt, Ui::Rect::fromLTRB(left + 1, top + 1, right - 1, bottom - 1), enumValue(Colours::getTranslucent(baseColour, 1)), RectFlags::transparent);
                    }
                }
            }
            else
            {
                PaletteIndex_t shadow, fill, fill2, hilight;
                if ((flags & RectInsetFlags::colourLight) != RectInsetFlags::none)
                {
                    shadow = Colours::getShade(baseColour, 1);
                    fill = Colours::getShade(baseColour, 3);
                    fill2 = Colours::getShade(baseColour, 4);
                    hilight = Colours::getShade(baseColour, 5);
                }
                else
                {
                    shadow = Colours::getShade(baseColour, 3);
                    fill = Colours::getShade(baseColour, 5);
                    fill2 = Colours::getShade(baseColour, 6);
                    hilight = Colours::getShade(baseColour, 7);
                }

                if ((flags & RectInsetFlags::borderNone) != RectInsetFlags::none)
                {
                    drawRectImpl(rt, rect, fill, RectFlags::none);
                }
                else if ((flags & RectInsetFlags::borderInset) != RectInsetFlags::none)
                {
                    // Draw outline of box
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, top, left, bottom), shadow, RectFlags::none);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left + 1, top, right, top), shadow, RectFlags::none);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(right, top + 1, right, bottom - 1), hilight, RectFlags::none);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left + 1, bottom, right, bottom), hilight, RectFlags::none);

                    if ((flags & RectInsetFlags::fillNone) == RectInsetFlags::none)
                    {
                        if ((flags & RectInsetFlags::fillDarker) == RectInsetFlags::none)
                        {
                            fill = fill2;
                        }
                        if ((flags & RectInsetFlags::fillTransparent) != RectInsetFlags::none)
                        {
                            fill = PaletteIndex::transparent;
                        }
                        drawRectImpl(rt, Ui::Rect::fromLTRB(left + 1, top + 1, right - 1, bottom - 1), fill, RectFlags::none);
                    }
                }
                else
                {
                    // Draw outline of box
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, top, left, bottom - 1), hilight, RectFlags::none);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left + 1, top, right - 1, top), hilight, RectFlags::none);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(right, top, right, bottom - 1), shadow, RectFlags::none);
                    drawRectImpl(rt, Ui::Rect::fromLTRB(left, bottom, right, bottom), shadow, RectFlags::none);

                    if ((flags & RectInsetFlags::fillNone) == RectInsetFlags::none)
                    {
                        if ((flags & RectInsetFlags::fillTransparent) != RectInsetFlags::none)
                        {
                            fill = PaletteIndex::transparent;
                        }
                        drawRectImpl(rt, Ui::Rect::fromLTRB(left + 1, top + 1, right - 1, bottom - 1), fill, RectFlags::none);
                    }
                }
            }
        }

        static void drawRectInset(const RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, AdvancedColour colour, RectInsetFlags flags)
        {
            // This makes the function signature more like a drawing application
            fillRectInset(rt, x, y, x + dx - 1, y + dy - 1, colour, flags);
        }

        /**
         * Draws a horizontal line of specified colour to a buffer.
         *  0x0045308A
         */
        static void drawHorizontalLine(const RenderTarget& rt, PaletteIndex_t colour, const Ui::Point& startCoord, int32_t length)
        {
            Ui::Point offset(startCoord.x - rt.x, startCoord.y - rt.y);

            // Check to make sure point is in the y range
            if (offset.y < 0)
            {
                return;
            }
            if (offset.y >= rt.height)
            {
                return;
            }
            // Check to make sure we are drawing at least a pixel
            if (length == 0)
            {
                length++;
            }

            // If x coord outside range leave
            if (offset.x < 0)
            {
                // Unless the number of pixels is enough to be in range
                length += offset.x;
                if (length <= 0)
                {
                    return;
                }
                // Resets starting point to 0 as we don't draw outside the range
                offset.x = 0;
            }

            // Ensure that the end point of the line is within range
            if (offset.x + length - rt.width > 0)
            {
                // If the end point has any pixels outside range
                // cut them off. If there are now no pixels return.
                length -= offset.x + length - rt.width;
                if (length <= 0)
                {
                    return;
                }
            }

            // Get the buffer we are drawing to and move to the first coordinate.
            uint8_t* buffer = rt.bits
                + offset.y * static_cast<int32_t>(rt.pitch + rt.width) + offset.x;

            // Draw the line to the specified colour
            std::fill_n(buffer, length, colour);
        }

        // 0x00452DA4
        static void drawLine(const RenderTarget& rt, Ui::Point a, Ui::Point b, const PaletteIndex_t colour)
        {
            const auto bounding = Rect::fromLTRB(a.x, a.y, b.x, b.y);
            // Check to make sure the line is within the drawing area
            if (!rt.getUiRect().intersects(bounding))
            {
                return;
            }

            // Bresenham's algorithm

            // If vertical plot points upwards
            const bool isSteep = std::abs(a.y - b.y) > std::abs(a.x - b.x);
            if (isSteep)
            {
                std::swap(b.y, a.x);
                std::swap(a.y, b.x);
            }

            // If line is right to left swap direction
            if (a.x > b.x)
            {
                std::swap(a.x, b.x);
                std::swap(b.y, a.y);
            }

            const auto deltaX = b.x - a.x;
            const auto deltaY = std::abs(b.y - a.y);
            auto error = deltaX / 2;
            const auto yStep = a.y < b.y ? 1 : -1;
            auto y = a.y;

            for (auto x = a.x, xStart = a.x, length = static_cast<int16_t>(1); x < b.x; ++x, ++length)
            {
                // Vertical lines are drawn 1 pixel at a time
                if (isSteep)
                {
                    drawHorizontalLine(rt, colour, { y, x }, 1);
                }

                error -= deltaY;
                if (error < 0)
                {
                    // Non vertical lines are drawn with as many pixels in a horizontal line as possible
                    if (!isSteep)
                    {
                        drawHorizontalLine(rt, colour, { xStart, y }, length);
                    }

                    // Reset non vertical line vars
                    xStart = x + 1;
                    length = 0; // NB: will be incremented in next iteration
                    y += yStep;
                    error += deltaX;
                }

                // Catch the case of the last line
                if (x + 1 == b.x && !isSteep)
                {
                    drawHorizontalLine(rt, colour, { xStart, y }, length);
                }
            }
        }
    }

    SoftwareDrawingContext::SoftwareDrawingContext()
        : _state{ std::make_unique<SoftwareDrawingContextState>() }
    {
    }

    SoftwareDrawingContext::~SoftwareDrawingContext()
    {
        // Need to keep the empty destructor to allow for unique_ptr to delete the actual type.
    }

    void SoftwareDrawingContext::clear(uint32_t fill)
    {
        auto& rt = currentRenderTarget();
        return Impl::clear(rt, fill);
    }

    void SoftwareDrawingContext::clearSingle(uint8_t paletteId)
    {
        auto& rt = currentRenderTarget();
        return Impl::clearSingle(rt, paletteId);
    }

    void SoftwareDrawingContext::fillRect(int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, RectFlags flags)
    {
        auto& rt = currentRenderTarget();
        return Impl::fillRect(rt, left, top, right, bottom, colour, flags);
    }

    void SoftwareDrawingContext::drawRect(int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags)
    {
        auto& rt = currentRenderTarget();
        return Impl::drawRect(rt, x, y, dx, dy, colour, flags);
    }

    void SoftwareDrawingContext::fillRectInset(int16_t left, int16_t top, int16_t right, int16_t bottom, AdvancedColour colour, RectInsetFlags flags)
    {
        auto& rt = currentRenderTarget();
        return Impl::fillRectInset(rt, left, top, right, bottom, colour, flags);
    }

    void SoftwareDrawingContext::drawRectInset(int16_t x, int16_t y, uint16_t dx, uint16_t dy, AdvancedColour colour, RectInsetFlags flags)
    {
        auto& rt = currentRenderTarget();
        return Impl::drawRectInset(rt, x, y, dx, dy, colour, flags);
    }

    void SoftwareDrawingContext::drawLine(const Ui::Point& a, const Ui::Point& b, PaletteIndex_t colour)
    {
        auto& rt = currentRenderTarget();
        return Impl::drawLine(rt, a, b, colour);
    }

    void SoftwareDrawingContext::drawImage(int16_t x, int16_t y, uint32_t image)
    {
        auto& rt = currentRenderTarget();
        return Impl::drawImage(&rt, x, y, image);
    }

    void SoftwareDrawingContext::drawImage(const Ui::Point& pos, const ImageId& image)
    {
        auto& rt = currentRenderTarget();
        return Impl::drawImage(rt, pos, image);
    }

    void SoftwareDrawingContext::drawImageMasked(const Ui::Point& pos, const ImageId& image, const ImageId& maskImage)
    {
        auto& rt = currentRenderTarget();
        return Impl::drawImageMasked(rt, pos, image, maskImage);
    }

    void SoftwareDrawingContext::drawImageSolid(const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex)
    {
        auto& rt = currentRenderTarget();
        return Impl::drawImageSolid(rt, pos, image, paletteIndex);
    }

    void SoftwareDrawingContext::drawImagePaletteSet(const Ui::Point& pos, const ImageId& image, PaletteMap::View palette, const G1Element* noiseImage)
    {
        auto& rt = currentRenderTarget();
        return Impl::drawImagePaletteSet(rt, pos, image, palette, noiseImage);
    }

    void SoftwareDrawingContext::pushRenderTarget(const RenderTarget& rt)
    {
        _state->rtStack.push(rt);

        // In case this leaks it will trigger an assert, the stack should ordinarily be really small.
        assert(_state->rtStack.size() < 10);
    }

    void SoftwareDrawingContext::popRenderTarget()
    {
        // Should not be empty before pop.
        assert(_state->rtStack.empty() == false);

        _state->rtStack.pop();

        // Should not be empty after pop
        assert(_state->rtStack.empty() == false);
    }

    const RenderTarget& SoftwareDrawingContext::currentRenderTarget() const
    {
        // Should not be empty.
        assert(_state->rtStack.empty() == false);

        return _state->rtStack.top();
    }

    void SoftwareDrawingContext::reset()
    {
        _state->rtStack = {};
    }

}
