#include "SoftwareDrawingContext.h"
#include "DrawSprite.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Localisation/Formatting.h"
#include "Ui.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <SDL2/SDL.h>
#include <algorithm>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Gfx;
using namespace OpenLoco::Ui;

namespace OpenLoco::Gfx
{
    namespace Impl
    {
        // Make this maybe public?
        enum class TextDrawFlags : uint16_t
        {
            none = 0U,
            inset = (1U << 0),
            outline = (1U << 1),
            dark = (1U << 2),
            extraDark = (1U << 3),
        };
        OPENLOCO_ENABLE_ENUM_OPERATORS(TextDrawFlags);

        // TODO: Move them into RenderContext once everything is implemented.
        static loco_global<TextDrawFlags, 0x112C824> _currentFontFlags;
        static loco_global<int16_t, 0x0112C876> _currentFontSpriteBase;
        static loco_global<uint8_t[224 * 4], 0x112C884> _characterWidths;
        static loco_global<AdvancedColour[4], 0x1136594> _windowColours;

        static loco_global<const uint8_t*, 0x009DA3D8> _noiseMaskImageData;
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

        // TODO: Store in drawing context.
        static PaletteMap::Buffer<8> _textColours{ 0 };
        static uint16_t getStringWidth(const char* str);
        static std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth);
        static uint16_t wrapStringTicker(char* buffer, uint16_t stringWidth, uint16_t numCharacters);
        static void drawRect(const RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags);
        static void drawImageSolid(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex);
        static void drawMaskedScalar(
            int32_t width, int32_t height, const uint8_t* maskSrc, const uint8_t* colourSrc, uint8_t* dst, int32_t maskWrap, int32_t colourWrap, int32_t dstWrap);

        // 0x0112C876
        static int16_t getCurrentFontSpriteBase()
        {
            return _currentFontSpriteBase;
        }

        static void setCurrentFontSpriteBase(int16_t base)
        {
            _currentFontSpriteBase = base;
        }

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

        // 0x004957C4
        static int16_t clipString(int16_t width, char* string)
        {
            if (width < 6)
            {
                *string = '\0';
                return 0;
            }

            // If width of the full string is less than allowed width then we don't need to clip
            auto clippedWidth = getStringWidth(string);
            if (clippedWidth <= width)
            {
                return clippedWidth;
            }

            // Append each character 1 by 1 with an ellipsis on the end until width is exceeded
            std::string bestString;
            std::string curString;

            for (const auto* chr = string; *chr != '\0'; ++chr)
            {
                curString.push_back(*chr);
                switch (static_cast<uint8_t>(*chr))
                {
                    case ControlCodes::moveX:
                        curString.push_back(*++chr);
                        break;

                    case ControlCodes::adjustPalette:
                    case 3:
                    case 4:
                        curString.push_back(*++chr);
                        break;

                    case ControlCodes::newline:
                    case ControlCodes::newlineSmaller:
                    case ControlCodes::Font::small:
                    case ControlCodes::Font::large:
                    case ControlCodes::Font::bold:
                    case ControlCodes::Font::regular:
                    case ControlCodes::Font::outline:
                    case ControlCodes::Font::outlineOff:
                    case ControlCodes::windowColour1:
                    case ControlCodes::windowColour2:
                    case ControlCodes::windowColour3:
                    case ControlCodes::windowColour4:
                        break;

                    case ControlCodes::inlineSpriteStr:
                        curString.push_back(*++chr);
                        curString.push_back(*++chr);
                        curString.push_back(*++chr);
                        curString.push_back(*++chr);
                        break;

                    case ControlCodes::newlineXY:
                        curString.push_back(*++chr);
                        curString.push_back(*++chr);
                        break;

                    default:
                        if (static_cast<uint8_t>(*chr) <= 0x16)
                        {
                            curString.push_back(*++chr);
                            curString.push_back(*++chr);
                        }
                        else if (static_cast<uint8_t>(*chr) < 32)
                        {
                            curString.push_back(*++chr);
                            curString.push_back(*++chr);
                            curString.push_back(*++chr);
                            curString.push_back(*++chr);
                        }
                        break;
                }

                auto ellipseString = curString;
                ellipseString.append("...");

                auto ellipsedWidth = getStringWidth(ellipseString.c_str());
                if (ellipsedWidth < width)
                {
                    // Keep best string with ellipse
                    bestString = ellipseString;
                }
                else
                {
                    StringManager::locoStrcpy(string, bestString.c_str());
                    return getStringWidth(string);
                }
            }
            return getStringWidth(string);
        }

        /**
         * 0x00495685
         *
         * @param buffer @<esi>
         * @return width @<cx>
         */
        static uint16_t getStringWidth(const char* str)
        {
            uint16_t width = 0;
            auto fontSpriteBase = getCurrentFontSpriteBase();

            while (*str != '\0')
            {
                const auto chr = static_cast<uint8_t>(*str);
                str++;

                if (chr >= 32)
                {
                    width += _characterWidths[chr - 32 + fontSpriteBase];
                    continue;
                }

                switch (chr)
                {
                    case ControlCodes::moveX:
                        width = *str;
                        str++;
                        break;

                    case ControlCodes::adjustPalette:
                    case 3:
                    case 4:
                        str++;
                        break;

                    case ControlCodes::newline:
                    case ControlCodes::newlineSmaller:
                        break;

                    case ControlCodes::Font::small:
                        fontSpriteBase = Font::small;
                        break;

                    case ControlCodes::Font::large:
                        fontSpriteBase = Font::large;
                        break;

                    case ControlCodes::Font::bold:
                        fontSpriteBase = Font::medium_bold;
                        break;

                    case ControlCodes::Font::regular:
                        fontSpriteBase = Font::medium_normal;
                        break;

                    case ControlCodes::Font::outline:
                    case ControlCodes::Font::outlineOff:
                    case ControlCodes::windowColour1:
                    case ControlCodes::windowColour2:
                    case ControlCodes::windowColour3:
                    case ControlCodes::windowColour4:
                        break;

                    case ControlCodes::newlineXY:
                        width = *str++;
                        str++;
                        break;

                    case ControlCodes::inlineSpriteStr:
                    {
                        const uint32_t image = reinterpret_cast<const uint32_t*>(str)[0];
                        const uint32_t imageId = image & 0x7FFFF;
                        str += 4;
                        width += getG1Element(imageId)->width;
                        break;
                    }

                    default:
                        if (chr <= 0x16)
                        {
                            str += 2;
                        }
                        else
                        {
                            str += 4;
                        }
                        break;
                }
            }

            return width;
        }

        static std::tuple<uint16_t, const char*, int16_t> getStringWidthOneLine(const char* ptr, int16_t font)
        {
            uint16_t lineWidth = 0;
            for (; *ptr != '\0'; ++ptr)
            {
                const auto chr = static_cast<uint8_t>(*ptr);
                if (chr >= ControlCodes::noArgBegin && chr < ControlCodes::noArgEnd)
                {
                    switch (chr)
                    {
                        case ControlCodes::newline:
                        {
                            return std::make_tuple(lineWidth, ptr, font);
                        }
                        case ControlCodes::Font::small:
                            font = Font::small;
                            break;
                        case ControlCodes::Font::large:
                            font = Font::large;
                            break;
                        case ControlCodes::Font::bold:
                            font = Font::medium_bold;
                            break;
                        case ControlCodes::Font::regular:
                            font = Font::medium_normal;
                            break;
                    }
                }
                else if (chr >= ControlCodes::oneArgBegin && chr < ControlCodes::oneArgEnd)
                {
                    switch (chr)
                    {
                        case ControlCodes::moveX:
                            lineWidth = static_cast<uint8_t>(ptr[1]);
                            break;
                    }
                    ptr += 1;
                }
                else if (chr >= ControlCodes::twoArgBegin && chr < ControlCodes::twoArgEnd)
                {
                    ptr += 2;
                }
                else if (chr >= ControlCodes::fourArgBegin && chr < ControlCodes::fourArgEnd)
                {
                    switch (chr)
                    {
                        case ControlCodes::inlineSpriteStr:
                        {
                            uint32_t image = *reinterpret_cast<const uint32_t*>(ptr);
                            ImageId imageId{ image & 0x7FFFF };
                            auto* el = getG1Element(imageId.getIndex());
                            if (el != nullptr)
                            {
                                lineWidth += el->width;
                            }
                            break;
                        }
                    }
                    ptr += 4;
                }
                else
                {
                    lineWidth += _characterWidths[font + (chr - 32)];
                }
            }
            return std::make_tuple(lineWidth, ptr, font);
        }

        // 0x00495715
        // @param buffer @<esi>
        // @return width @<cx>
        static uint16_t getStringWidthNewLined(const char* buffer)
        {
            auto font = *_currentFontSpriteBase;
            uint16_t maxWidth = 0;

            for (auto* ptr = buffer; *ptr != '\0'; ++ptr)
            {
                auto [lineWidth, nextLine, nextFont] = getStringWidthOneLine(ptr, font);
                ptr = nextLine;
                font = nextFont;
                maxWidth = std::max(maxWidth, lineWidth);
            }
            return maxWidth;
        }

        /**
         * 0x004955BC
         *
         * @param buffer @<esi>
         * @return width @<cx>
         */
        static uint16_t getMaxStringWidth(const char* str)
        {
            uint16_t width = 0;
            uint16_t maxWidth = 0;
            auto fontSpriteBase = getCurrentFontSpriteBase();

            while (*str != '\0')
            {
                const auto chr = static_cast<uint8_t>(*str);
                str++;

                if (chr >= 32)
                {
                    width += _characterWidths[chr - 32 + fontSpriteBase];
                    continue;
                }

                switch (chr)
                {
                    case ControlCodes::moveX:
                        maxWidth = std::max(width, maxWidth);
                        width = *str;
                        str++;
                        break;

                    case ControlCodes::adjustPalette:
                    case 3:
                    case 4:
                        str++;
                        break;

                    case ControlCodes::newline:
                    case ControlCodes::newlineSmaller:
                        continue;

                    case ControlCodes::Font::small:
                        fontSpriteBase = Font::small;
                        break;

                    case ControlCodes::Font::large:
                        fontSpriteBase = Font::large;
                        break;

                    case ControlCodes::Font::bold:
                        fontSpriteBase = Font::medium_bold;
                        break;

                    case ControlCodes::Font::regular:
                        fontSpriteBase = Font::medium_normal;
                        break;

                    case ControlCodes::Font::outline:
                    case ControlCodes::Font::outlineOff:
                    case ControlCodes::windowColour1:
                    case ControlCodes::windowColour2:
                    case ControlCodes::windowColour3:
                    case ControlCodes::windowColour4:
                        break;

                    case ControlCodes::newlineXY:
                        maxWidth = std::max(width, maxWidth);
                        width = *str;
                        str += 2;
                        break;

                    case ControlCodes::inlineSpriteStr:
                    {
                        const uint32_t image = reinterpret_cast<const uint32_t*>(str)[0];
                        const uint32_t imageId = image & 0x7FFFF;
                        str += 4;
                        width += getG1Element(imageId)->width;
                        break;
                    }

                    default:
                        if (chr <= 0x16)
                        {
                            str += 2;
                        }
                        else
                        {
                            str += 4;
                        }
                        break;
                }
            }
            maxWidth = std::max(width, maxWidth);
            return maxWidth;
        }

        static void setTextColours(PaletteIndex_t pal1, PaletteIndex_t pal2, PaletteIndex_t pal3)
        {
            if ((_currentFontFlags & TextDrawFlags::inset) != TextDrawFlags::none)
                return;

            _textColours[1] = pal1;
            _textColours[2] = PaletteIndex::transparent;
            _textColours[3] = PaletteIndex::transparent;
            if ((_currentFontFlags & TextDrawFlags::outline) != TextDrawFlags::none)
            {
                _textColours[2] = pal2;
                _textColours[3] = pal3;
            }
        }

        static void setTextColour(int colour)
        {
            const auto* el = getG1Element(ImageIds::text_palette);
            setTextColours(el->offset[colour * 4 + 0], el->offset[colour * 4 + 1], el->offset[colour * 4 + 2]);
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
                return std::nullopt;

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
                    return std::nullopt;
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
                return;

            const auto scaledWidth = imageWidth >> TZoomLevel;
            if (scaledWidth == 0)
                return;

            constexpr auto skip = (1 << TZoomLevel);

            // Calculate the row size.
            const auto scaledRowSize = [&]() {
                if constexpr (TZoomLevel == 0)
                {
                    return rowSize;
                }
                else
                {
                    constexpr auto shift = (1 << TZoomLevel) - 1;

                    const auto v7 = shift * static_cast<uint16_t>(rowSize + imageWidth) + rowSize;
                    const auto v8 = static_cast<uint16_t>(imageWidth) & shift;

                    return v8 + v7;
                }
            }();

            if constexpr (TZoomLevel > 0)
            {
                dstWrap -= scaledWidth;
            }

            auto remainingRows = scaledHeight;
            do
            {
                auto remainingWidth = scaledWidth;
                do
                {
                    uint8_t masked = *bytesMask & *bytesImage;
                    if (masked)
                        *dstBuf = masked;
                    bytesImage += skip;
                    bytesMask += skip;
                    dstBuf++;
                    remainingWidth--;
                    if (!remainingWidth)
                        break;

                    masked = *bytesMask & *bytesImage;
                    if (masked)
                        *dstBuf = masked;
                    bytesImage += skip;
                    bytesMask += skip;
                    dstBuf++;
                    remainingWidth--;
                    if (!remainingWidth)
                        break;

                    masked = *bytesMask & *bytesImage;
                    if (masked)
                        *dstBuf = masked;
                    bytesImage += skip;
                    bytesMask += skip;
                    dstBuf++;
                    remainingWidth--;
                    if (!remainingWidth)
                        break;

                    masked = *bytesMask & *bytesImage;
                    if (masked)
                        *dstBuf = masked;
                    bytesImage += skip;
                    bytesMask += skip;
                    dstBuf++;
                    remainingWidth--;

                } while (remainingWidth);

                bytesImage += scaledRowSize;
                bytesMask += scaledRowSize;
                dstBuf += dstWrap;

                --remainingRows;

            } while (remainingRows);
        }

        struct WH16
        {
            int16_t width;
            int16_t height;
        };

        template<int32_t TZoomLevel>
        static void drawImageMaskedZoom(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const ImageId& maskImage)
        {
            const auto* g1Image = Gfx::getG1Element(image.getIndex());
            const auto* g1ImageMask = Gfx::getG1Element(maskImage.getIndex());

            if constexpr (TZoomLevel > 0)
            {
                if (g1Image->hasFlags(G1ElementFlags::none))
                    return;

                if (g1Image->hasFlags(G1ElementFlags::hasZoomSprites))
                {
                    if (g1ImageMask->hasFlags(G1ElementFlags::noZoomDraw))
                        return;

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

            const auto* imageDataPos = g1Image->offset;
            WH16 imageSize = { g1Image->width, g1Image->height };
            WH16 gImageSize = imageSize;
            Ui::Point gImageOffsets = { g1Image->xOffset, g1Image->yOffset };
            auto* bits = rt.bits;
            int16_t imageHeight = gImageSize.height;

            constexpr uint16_t zoomMask = static_cast<uint16_t>(0xFFFFFFFFULL << TZoomLevel);
            constexpr int16_t offsetX = (1 << TZoomLevel) - 1;

            int16_t rtPosY = ((gImageOffsets.y + pos.y) & zoomMask) - rt.y;
            if (rtPosY >= 0)
            {
                auto scaledWidth = rt.width >> TZoomLevel;
                scaledWidth = rt.pitch + scaledWidth;
                bits += (rtPosY >> TZoomLevel) * scaledWidth;
            }
            else
            {
                if (rtPosY + imageHeight == 0)
                    return;
                imageHeight += rtPosY;
                if (imageHeight < 0)
                    return;
                const auto v34 = static_cast<uint16_t>(-rtPosY) * gImageSize.width;
                rtPosY = 0;
                imageDataPos = &g1Image->offset[v34];
            }

            int16_t v17 = imageHeight + rtPosY;
            bool v18 = v17 <= rt.height;
            int16_t v37 = v17 - rt.height;
            if (v18 || (v18 = imageHeight <= v37, imageHeight -= v37, !v18))
            {
                int16_t imageWidth = gImageSize.width;
                int16_t rowSize = 0;
                int16_t dstWrap = rt.pitch + (rt.width >> TZoomLevel);

                if constexpr (TZoomLevel == 0)
                {
                    dstWrap -= gImageSize.width;
                }

                int16_t rtPosX = ((gImageOffsets.x + pos.x + offsetX) & zoomMask) - rt.x;
                if (rtPosX < 0)
                {
                    if (rtPosX + imageWidth == 0)
                        return;
                    imageWidth += rtPosX;
                    if (imageWidth < 0)
                        return;
                    rowSize -= rtPosX;
                    imageDataPos -= rtPosX;
                    if constexpr (TZoomLevel == 0)
                    {
                        dstWrap -= rtPosX;
                    }
                    rtPosX = 0;
                }

                int16_t rtPosXUnscaled = rtPosX;
                rtPosX = rtPosX >> TZoomLevel;
                auto* dstBuf2 = &bits[rtPosX];
                int16_t v42 = imageWidth + rtPosXUnscaled - rt.width;
                if ((imageWidth + rtPosXUnscaled) > rt.width)
                {
                    v18 = imageWidth <= v42;
                    imageWidth -= v42;
                    if (v18)
                        return;
                    rowSize += v42;
                    if constexpr (TZoomLevel == 0)
                    {
                        dstWrap += v42;
                    }
                }

                auto imageOffset = imageDataPos - g1Image->offset;
                drawMaskedZoom<TZoomLevel>(
                    imageHeight,
                    imageWidth,
                    rowSize,
                    &g1ImageMask->offset[imageOffset],
                    dstWrap,
                    dstBuf2,
                    imageDataPos);
            }
        }

        // 0x00450705
        static void drawImageMasked(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const ImageId& maskImage)
        {
            switch (rt.zoomLevel)
            {
                case 0:
                    drawImageMaskedZoom<0>(rt, pos, image, maskImage);
                    break;
                case 1:
                    drawImageMaskedZoom<1>(rt, pos, image, maskImage);
                    break;
                case 2:
                    drawImageMaskedZoom<2>(rt, pos, image, maskImage);
                    break;
                case 3:
                    drawImageMaskedZoom<3>(rt, pos, image, maskImage);
                    break;
                default:
                    break;
            }
            // Unreachable
        }

        static void drawImageSolid(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex)
        {
            PaletteMap::Buffer<PaletteMap::kDefaultSize> palette;
            std::fill(palette.begin(), palette.end(), paletteIndex);
            palette[0] = 0;

            // Set the image primary flag to tell drawImagePaletteSet to recolour with the palette (Colour::black is not actually used)
            drawImagePaletteSet(rt, pos, image.withPrimary(Colour::black), PaletteMap::View{ palette }, {});
        }

        // 0x00451189
        static Ui::Point loopNewline(const RenderTarget* rt, Ui::Point origin, const char* str)
        {
            Ui::Point pos = origin;
            while (true)
            {
                // When off-screen in y dimension don't draw text
                // In original this check only performed if pos.y updated instead of every loop
                bool offscreen = true;
                if (pos.y + 19 > rt->y)
                {
                    if (rt->y + rt->height > pos.y)
                    {
                        offscreen = false;
                    }
                }

                const auto chr = static_cast<uint8_t>(*str);
                str++;

                switch (chr)
                {
                    case 0U:
                        return pos;

                    case ControlCodes::adjustPalette:
                        // This control character does not appear in the localisation files
                        assert(false);
                        str++;
                        break;

                    case ControlCodes::newlineSmaller:
                        pos.x = origin.x;
                        if (getCurrentFontSpriteBase() == Font::medium_normal || getCurrentFontSpriteBase() == Font::medium_bold)
                        {
                            pos.y += 5;
                        }
                        else if (getCurrentFontSpriteBase() == Font::small)
                        {
                            pos.y += 3;
                        }
                        else if (getCurrentFontSpriteBase() == Font::large)
                        {
                            pos.y += 9;
                        }
                        break;

                    case ControlCodes::newline:
                        pos.x = origin.x;
                        if (getCurrentFontSpriteBase() == Font::medium_normal || getCurrentFontSpriteBase() == Font::medium_bold)
                        {
                            pos.y += 10;
                        }
                        else if (getCurrentFontSpriteBase() == Font::small)
                        {
                            pos.y += 6;
                        }
                        else if (getCurrentFontSpriteBase() == Font::large)
                        {
                            pos.y += 18;
                        }
                        break;

                    case ControlCodes::moveX:
                    {
                        uint8_t offset = *str;
                        str++;
                        pos.x = origin.x + offset;

                        break;
                    }

                    case ControlCodes::newlineXY:
                    {
                        uint8_t offset = *str;
                        str++;
                        pos.x = origin.x + offset;

                        offset = *str;
                        str++;
                        pos.y = origin.y + offset;

                        break;
                    }

                    case ControlCodes::Font::small:
                        setCurrentFontSpriteBase(Font::small);
                        break;
                    case ControlCodes::Font::large:
                        setCurrentFontSpriteBase(Font::large);
                        break;
                    case ControlCodes::Font::regular:
                        setCurrentFontSpriteBase(Font::medium_normal);
                        break;
                    case ControlCodes::Font::bold:
                        setCurrentFontSpriteBase(Font::medium_bold);
                        break;
                    case ControlCodes::Font::outline:
                        _currentFontFlags = _currentFontFlags | TextDrawFlags::outline;
                        break;
                    case ControlCodes::Font::outlineOff:
                        _currentFontFlags = _currentFontFlags & ~TextDrawFlags::outline;
                        break;
                    case ControlCodes::windowColour1:
                    {
                        auto hue = _windowColours[0].c();
                        setTextColours(Colours::getShade(hue, 7), PaletteIndex::index_0A, PaletteIndex::index_0A);
                        break;
                    }
                    case ControlCodes::windowColour2:
                    {
                        auto hue = _windowColours[1].c();
                        setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                        break;
                    }
                    case ControlCodes::windowColour3:
                    {
                        auto hue = _windowColours[2].c();
                        setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                        break;
                    }
                    case ControlCodes::windowColour4:
                    {
                        auto hue = _windowColours[3].c();
                        setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                        break;
                    }

                    case ControlCodes::inlineSpriteStr:
                    {
                        uint32_t image = ((uint32_t*)str)[0];
                        ImageId imageId{ image & 0x7FFFF };
                        str += 4;

                        if ((_currentFontFlags & TextDrawFlags::inset) != TextDrawFlags::none)
                        {
                            drawImageSolid(*rt, pos, imageId, _textColours[3]);
                            drawImageSolid(*rt, pos + Ui::Point{ 1, 1 }, imageId, _textColours[1]);
                        }
                        else
                        {
                            drawImage(rt, pos.x, pos.y, image);
                        }

                        pos.x += getG1Element(imageId.getIndex())->width;
                        break;
                    }

                    case ControlCodes::Colour::black:
                        setTextColour(0);
                        break;

                    case ControlCodes::Colour::grey:
                        setTextColour(1);
                        break;

                    case ControlCodes::Colour::white:
                        setTextColour(2);
                        break;

                    case ControlCodes::Colour::red:
                        setTextColour(3);
                        break;

                    case ControlCodes::Colour::green:
                        setTextColour(4);
                        break;

                    case ControlCodes::Colour::yellow:
                        setTextColour(5);
                        break;

                    case ControlCodes::Colour::topaz:
                        setTextColour(6);
                        break;

                    case ControlCodes::Colour::celadon:
                        setTextColour(7);
                        break;

                    case ControlCodes::Colour::babyBlue:
                        setTextColour(8);
                        break;

                    case ControlCodes::Colour::paleLavender:
                        setTextColour(9);
                        break;

                    case ControlCodes::Colour::paleGold:
                        setTextColour(10);
                        break;

                    case ControlCodes::Colour::lightPink:
                        setTextColour(11);
                        break;

                    case ControlCodes::Colour::pearlAqua:
                        setTextColour(12);
                        break;

                    case ControlCodes::Colour::paleSilver:
                        setTextColour(13);
                        break;

                    default:
                        if (!offscreen)
                        {
                            // When off-screen in the y dimension there is no requirement to keep pos.x correct
                            if (chr >= 32)
                            {
                                // Use withPrimary to set imageId flag to use the correct palette code (Colour::black is not actually used)
                                drawImagePaletteSet(*rt, pos, ImageId(1116 + chr - 32 + getCurrentFontSpriteBase()).withPrimary(Colour::black), PaletteMap::View{ _textColours }, {});
                                pos.x += _characterWidths[chr - 32 + getCurrentFontSpriteBase()];
                            }
                            else
                            {
                                // Unhandled control code
                                assert(false);
                            }
                        }
                        break;
                }
            }

            return pos;
        }

        /**
         * 0x00451025
         *
         * @param x  @<cx>
         * @param y @<dx>
         * @param colour @<al>
         * @param rt @<edi>
         * @param text @<esi>
         */
        static Point drawString(const RenderTarget& rt, Ui::Point origin, AdvancedColour colour, const char* str)
        {
            if (colour.isFE())
            {
                return loopNewline(&rt, origin, str);
            }

            if (colour.isFD())
            {
                _currentFontFlags = TextDrawFlags::none;
                setTextColour(0);
                return loopNewline(&rt, origin, str);
            }

            if (origin.x >= rt.x + rt.width)
                return origin;

            if (origin.x < rt.x - 1280)
                return origin;

            if (origin.y >= rt.y + rt.height)
                return origin;

            if (origin.y < rt.y - 90)
                return origin;

            if (colour.isFF())
            {
                return loopNewline(&rt, origin, str);
            }

            _currentFontFlags = TextDrawFlags::none;
            if (getCurrentFontSpriteBase() == Font::m1)
            {
                setCurrentFontSpriteBase(Font::medium_bold);
                _currentFontFlags = _currentFontFlags | TextDrawFlags::dark;
            }
            else if (getCurrentFontSpriteBase() == Font::m2)
            {
                setCurrentFontSpriteBase(Font::medium_bold);
                _currentFontFlags = _currentFontFlags | TextDrawFlags::dark;
                _currentFontFlags = _currentFontFlags | TextDrawFlags::extraDark;
            }

            _textColours[0] = PaletteIndex::transparent;
            _textColours[1] = Colours::getShade(Colour::mutedDarkPurple, 5);
            _textColours[2] = Colours::getShade(Colour::mutedRed, 5);
            _textColours[3] = Colours::getShade(Colour::blue, 5);

            if (colour.isOutline())
            {
                colour = colour.clearOutline();
                _currentFontFlags = _currentFontFlags | TextDrawFlags::outline;
            }

            if (colour.isInset())
            {
                colour = colour.clearInset();
                _currentFontFlags = _currentFontFlags | TextDrawFlags::inset;
            }

            if ((_currentFontFlags & TextDrawFlags::inset) != TextDrawFlags::none)
            {
                if ((_currentFontFlags & TextDrawFlags::dark) != TextDrawFlags::none && (_currentFontFlags & TextDrawFlags::extraDark) != TextDrawFlags::none)
                {
                    _textColours[1] = Colours::getShade(colour.c(), 2);
                    _textColours[2] = PaletteIndex::transparent;
                    _textColours[3] = Colours::getShade(colour.c(), 4);
                }
                else if ((_currentFontFlags & TextDrawFlags::dark) != TextDrawFlags::none)
                {
                    _textColours[1] = Colours::getShade(colour.c(), 3);
                    _textColours[2] = PaletteIndex::transparent;
                    _textColours[3] = Colours::getShade(colour.c(), 5);
                }
                else
                {
                    _textColours[1] = Colours::getShade(colour.c(), 4);
                    _textColours[2] = PaletteIndex::transparent;
                    _textColours[3] = Colours::getShade(colour.c(), 6);
                }
            }
            else
            {
                setTextColours(Colours::getShade(colour.c(), 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
            }

            return loopNewline(&rt, origin, str);
        }

        // Use only with buffer mangled by wrapString
        static const char* advanceToNextLineWrapped(const char* str)
        {
            // Traverse the buffer for the next line
            const char* ptr = str;
            while (true)
            {
                const auto chr = static_cast<uint8_t>(*ptr++);
                if (chr == 0U)
                    return ptr;

                if (chr >= ControlCodes::oneArgBegin && chr < ControlCodes::oneArgEnd)
                {
                    // Skip argument
                    ptr++;
                }
                else if (chr >= ControlCodes::twoArgBegin && chr < ControlCodes::twoArgEnd)
                {
                    // Skip argument
                    ptr += 2;
                }
                else if (chr >= ControlCodes::fourArgBegin && chr < ControlCodes::fourArgEnd)
                {
                    // Skip argument
                    ptr += 4;
                }
            }

            return nullptr;
        }

        static uint16_t lineHeightFromFont(int16_t font)
        {
            if (font <= Font::medium_bold)
                return 10;
            else if (font == Font::small)
                return 6;
            else if (font == Font::large)
                return 18;
            return 0;
        }

        // 0x00495224
        // al: colour
        // bp: width
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Point drawStringLeftWrapped(
            const RenderTarget& rt,
            Point origin,
            int16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            // Setup the text colours (FIXME: This should be a separate function)
            char empty[1] = "";
            drawString(rt, origin, colour, empty);

            _currentFontSpriteBase = Font::medium_bold;
            auto wrapResult = wrapString(buffer, width);
            auto breakCount = wrapResult.second + 1;

            // wrapString might change the font due to formatting codes
            uint16_t lineHeight = lineHeightFromFont(_currentFontSpriteBase); // _112D404

            _currentFontFlags = TextDrawFlags::none;
            auto point = origin;

            const char* ptr = buffer;
            for (auto i = 0; ptr != nullptr && i < breakCount; i++)
            {
                drawString(rt, point, AdvancedColour::FE(), const_cast<char*>(ptr));
                ptr = advanceToNextLineWrapped(ptr);
                point.y += lineHeight;
            }

            return point;
        }

        /**
         * 0x00494B3F
         * @param rt @<edi>
         * @param origin {x @<cx>, y @<dx>}
         * @param colour @<al>
         * @param stringId  @<bx>
         * @param args @<edi>
         */
        static Point drawStringLeft(
            const RenderTarget& rt,
            Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            return drawString(rt, origin, colour, buffer);
        }

        // 0x00494BBF
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        // bp: width
        static Point drawStringLeftClipped(
            const RenderTarget& rt,
            Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            clipString(width, buffer);

            return drawString(rt, origin, colour, buffer);
        }

        // 0x00494C78
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Point drawStringRight(
            const RenderTarget& rt,
            Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            uint16_t width = getStringWidth(buffer);

            auto point = origin;
            point.x -= width;

            return drawString(rt, point, colour, buffer);
        }

        // 0x00494CB2
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Point drawStringRightUnderline(
            const RenderTarget& rt,
            Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            uint16_t width = getStringWidth(buffer);
            auto point = origin;
            point.x -= width;

            drawString(rt, point, colour, buffer);

            // Draw underline
            drawRect(rt, point.x, point.y + 11, width, 1, _textColours[1], RectFlags::none);
            if (_textColours[2] != 0)
                drawRect(rt, point.x, point.y + 12, width, 1, _textColours[2], RectFlags::none);

            return point;
        }

        // 0x00494D78
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Point drawStringLeftUnderline(
            const RenderTarget& rt,
            Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            uint16_t width = getStringWidth(buffer);

            auto point = drawString(rt, origin, colour, buffer);

            // Draw underline
            drawRect(rt, origin.x, origin.y + 11, width, 1, _textColours[1], RectFlags::none);
            if (_textColours[2] != 0)
                drawRect(rt, origin.x, origin.y + 12, width, 1, _textColours[2], RectFlags::none);

            return point;
        }

        // 0x00494DE8
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Point drawStringCentred(
            const RenderTarget& rt,
            Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            uint16_t width = getStringWidth(buffer);

            auto point = origin;
            point.x = origin.x - (width / 2);

            if (point.x < 0)
                return origin;

            return drawString(rt, point, colour, buffer);
        }

        // 0x00494C36
        // al: colour
        // bx: string id
        // bp: width
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Point drawStringCentredClipped(
            const RenderTarget& rt,
            Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            width = clipString(width, buffer);

            auto point = Point(origin.x - (width / 2), origin.y);
            return drawString(rt, point, colour, buffer);
        }

        /**
         * 0x00494ECF
         *
         * @param rt @<edi>
         * @param origin {x @<cx>, y @<dx>}
         * @param width @<bp>
         * @param colour @<al>
         * @param stringId @<bx>
         * @param args @<esi>
         * returns width @<ax>
         */
        static Point drawStringCentredWrapped(
            const RenderTarget& rt,
            Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            _currentFontSpriteBase = Font::medium_bold;
            // Setup the text colours (FIXME: This should be a separate function)
            char empty[1] = "";
            drawString(rt, origin, colour, empty);

            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            _currentFontSpriteBase = Font::medium_bold;
            auto wrapResult = wrapString(buffer, width);
            auto breakCount = wrapResult.second + 1;

            // wrapString might change the font due to formatting codes
            uint16_t lineHeight = lineHeightFromFont(_currentFontSpriteBase); // _112D404

            _currentFontFlags = TextDrawFlags::none;

            Ui::Point basePoint = origin;
            basePoint.y -= (lineHeight / 2) * (breakCount - 1);

            const char* ptr = buffer;
            uint16_t lineWidth{};

            for (auto i = 0; ptr != nullptr && i < breakCount; i++)
            {
                lineWidth = getStringWidth(ptr);

                auto point = basePoint;
                point.x -= lineWidth / 2;

                drawString(rt, point, AdvancedColour::FE(), const_cast<char*>(ptr));
                ptr = advanceToNextLineWrapped(ptr);

                basePoint.y += lineHeight;
            }

            return basePoint;
        }

        // 0x00494E33
        // al: colour
        // bx: string id
        // bp: width
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Point drawStringCentredRaw(
            const RenderTarget& rt,
            Point origin,
            uint16_t linebreakCount,
            AdvancedColour colour,
            const char* wrappedStr)
        {
            _currentFontSpriteBase = Font::medium_bold;
            // Setup the text colours (FIXME: This should be a separate function)
            char empty[1] = "";
            drawString(rt, origin, colour, empty);

            _currentFontSpriteBase = Font::medium_bold;
            _currentFontFlags = TextDrawFlags::none;

            const char* ptr = wrappedStr;
            auto basePoint = origin;

            for (auto i = 0; i < linebreakCount + 1 && ptr != nullptr; ++i)
            {
                uint16_t lineWidth = getStringWidth(ptr);

                auto point = basePoint;
                point.x -= lineWidth / 2;

                drawString(rt, point, AdvancedColour::FE(), const_cast<char*>(ptr));

                ptr = advanceToNextLineWrapped(ptr);
                basePoint.y += lineHeightFromFont(_currentFontSpriteBase);
            }

            return basePoint;
        }

        static void drawStringYOffsets(const RenderTarget& rt, const Ui::Point& loc, AdvancedColour colour, const void* args, const int8_t* yOffsets)
        {
            registers regs;
            regs.edi = X86Pointer(&rt);
            regs.esi = X86Pointer(args);
            regs.ebp = X86Pointer(yOffsets);
            regs.cx = loc.x;
            regs.dx = loc.y;
            regs.al = colour.u8();
            call(0x0045196C, regs);
        }

        // 0x00451582
        static int16_t drawStringMaxChars(const RenderTarget& rt, Ui::Point origin, const AdvancedColour colour, uint8_t* str, const int16_t numCharsRemaining)
        {
            // This function has been somewhat simplified removing unreachable parts
            if (!colour.isFE())
            {
                assert(false);
                return numCharsRemaining;
            }
            int16_t numChars = numCharsRemaining;
            Ui::Point pos = origin;
            while (true)
            {
                if (numChars == 0)
                {
                    break;
                }
                // When off-screen in y dimension don't draw text
                // In original this check only performed if pos.y updated instead of every loop
                bool offscreen = true;
                if (pos.y + 19 > rt.y)
                {
                    if (rt.y + rt.height > pos.y)
                    {
                        offscreen = false;
                    }
                }
                const auto chr = static_cast<uint8_t>(*str);
                str++;

                switch (chr)
                {
                    case 0U:
                        return numChars;

                    case ControlCodes::adjustPalette:
                        // This control character does not appear in the localisation files
                        assert(false);
                        str++;
                        break;

                    case ControlCodes::newlineSmaller:
                        pos.x = origin.x;
                        if (getCurrentFontSpriteBase() == Font::medium_normal || getCurrentFontSpriteBase() == Font::medium_bold)
                        {
                            pos.y += 5;
                        }
                        else if (getCurrentFontSpriteBase() == Font::small)
                        {
                            pos.y += 3;
                        }
                        else if (getCurrentFontSpriteBase() == Font::large)
                        {
                            pos.y += 9;
                        }
                        break;

                    case ControlCodes::newline:
                        pos.x = origin.x;
                        if (getCurrentFontSpriteBase() == Font::medium_normal || getCurrentFontSpriteBase() == Font::medium_bold)
                        {
                            pos.y += 10;
                        }
                        else if (getCurrentFontSpriteBase() == Font::small)
                        {
                            pos.y += 6;
                        }
                        else if (getCurrentFontSpriteBase() == Font::large)
                        {
                            pos.y += 18;
                        }
                        break;

                    case ControlCodes::moveX:
                    {
                        uint8_t offset = *str;
                        str++;
                        pos.x = origin.x + offset;

                        break;
                    }

                    case ControlCodes::newlineXY:
                    {
                        uint8_t offset = *str;
                        str++;
                        pos.x = origin.x + offset;

                        offset = *str;
                        str++;
                        pos.y = origin.y + offset;

                        break;
                    }

                    case ControlCodes::Font::small:
                        setCurrentFontSpriteBase(Font::small);
                        break;
                    case ControlCodes::Font::large:
                        setCurrentFontSpriteBase(Font::large);
                        break;
                    case ControlCodes::Font::regular:
                        setCurrentFontSpriteBase(Font::medium_normal);
                        break;
                    case ControlCodes::Font::bold:
                        setCurrentFontSpriteBase(Font::medium_bold);
                        break;
                    case ControlCodes::Font::outline:
                        _currentFontFlags = _currentFontFlags | TextDrawFlags::outline;
                        break;
                    case ControlCodes::Font::outlineOff:
                        _currentFontFlags = _currentFontFlags & ~TextDrawFlags::outline;
                        break;
                    case ControlCodes::windowColour1:
                    {
                        auto hue = _windowColours[0].c();
                        setTextColours(Colours::getShade(hue, 7), PaletteIndex::index_0A, PaletteIndex::index_0A);
                        break;
                    }
                    case ControlCodes::windowColour2:
                    {
                        auto hue = _windowColours[1].c();
                        setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                        break;
                    }
                    case ControlCodes::windowColour3:
                    {
                        auto hue = _windowColours[2].c();
                        setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                        break;
                    }
                    case ControlCodes::windowColour4:
                    {
                        auto hue = _windowColours[3].c();
                        setTextColours(Colours::getShade(hue, 9), PaletteIndex::index_0A, PaletteIndex::index_0A);
                        break;
                    }

                    case ControlCodes::inlineSpriteStr:
                    {
                        uint32_t image = ((uint32_t*)str)[0];
                        ImageId imageId{ image & 0x7FFFF };
                        str += 4;

                        drawImage(&rt, pos.x, pos.y, image);

                        // For some reason the wrapStringTicker doesn't do this??
                        numChars--;
                        pos.x += getG1Element(imageId.getIndex())->width;
                        break;
                    }

                    case ControlCodes::Colour::black:
                        setTextColour(0);
                        break;

                    case ControlCodes::Colour::grey:
                        setTextColour(1);
                        break;

                    case ControlCodes::Colour::white:
                        setTextColour(2);
                        break;

                    case ControlCodes::Colour::red:
                        setTextColour(3);
                        break;

                    case ControlCodes::Colour::green:
                        setTextColour(4);
                        break;

                    case ControlCodes::Colour::yellow:
                        setTextColour(5);
                        break;

                    case ControlCodes::Colour::topaz:
                        setTextColour(6);
                        break;

                    case ControlCodes::Colour::celadon:
                        setTextColour(7);
                        break;

                    case ControlCodes::Colour::babyBlue:
                        setTextColour(8);
                        break;

                    case ControlCodes::Colour::paleLavender:
                        setTextColour(9);
                        break;

                    case ControlCodes::Colour::paleGold:
                        setTextColour(10);
                        break;

                    case ControlCodes::Colour::lightPink:
                        setTextColour(11);
                        break;

                    case ControlCodes::Colour::pearlAqua:
                        setTextColour(12);
                        break;

                    case ControlCodes::Colour::paleSilver:
                        setTextColour(13);
                        break;

                    default:
                        if (chr >= 32)
                        {
                            numChars--;
                        }
                        if (!offscreen)
                        {
                            // When off-screen in the y dimension there is no requirement to keep pos.x correct
                            if (chr >= 32)
                            {
                                // Use withPrimary to set imageId flag to use the correct palette code (Colour::black is not actually used)
                                drawImagePaletteSet(rt, pos, ImageId(1116 + chr - 32 + getCurrentFontSpriteBase()).withPrimary(Colour::black), PaletteMap::View{ _textColours }, {});
                                pos.x += _characterWidths[chr - 32 + getCurrentFontSpriteBase()];
                            }
                            else
                            {
                                // Unhandled control code
                                assert(false);
                            }
                        }
                        break;
                }
            }

            return numChars;
        }

        // 0x004950EF
        static void drawStringTicker(const RenderTarget& rt, Ui::Point origin, StringId stringId, Colour colour, uint8_t numLinesToDisplay, uint16_t numCharactersToDisplay, uint16_t width)
        {
            _currentFontSpriteBase = Font::medium_bold;
            // Setup the text colours (FIXME: This should be a separate function)
            char empty[1] = "";
            drawString(rt, origin, colour, empty);

            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId);

            _currentFontSpriteBase = Font::medium_bold;
            const auto numLinesToDisplayAllChars = wrapStringTicker(buffer, width, numCharactersToDisplay);
            const auto lineToDisplayFrom = numLinesToDisplayAllChars - numLinesToDisplay;

            // wrapString might change the font due to formatting codes
            uint16_t lineHeight = lineHeightFromFont(_currentFontSpriteBase); // _112D404

            _currentFontFlags = TextDrawFlags::none;
            Ui::Point point = origin;
            if (lineToDisplayFrom > 0)
            {
                point.y -= lineHeight * lineToDisplayFrom;
            }
            const char* ptr = buffer;

            auto numChars = numCharactersToDisplay;
            for (auto i = 0; ptr != nullptr && i < numLinesToDisplayAllChars; i++)
            {
                uint16_t lineWidth = getStringWidth(ptr);

                // special drawstring
                numChars = drawStringMaxChars(rt, point - Ui::Point(lineWidth / 2, 0), AdvancedColour::FE(), reinterpret_cast<uint8_t*>(const_cast<char*>(ptr)), numChars);
                ptr = advanceToNextLineWrapped(ptr);
                point.y += lineHeight;
            }
        }

        // 0x00495301
        // Note: Returned break count is -1. TODO: Refactor out this -1.
        // @return maxWidth @<cx> (numLinesToDisplayAllChars-1) @<di>
        static std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth)
        {
            // std::vector<const char*> wrap; TODO: refactor to return pointers to line starts
            uint16_t wrapCount = 0;
            auto font = *_currentFontSpriteBase;
            uint16_t maxWidth = 0;

            for (auto* ptr = buffer; *ptr != '\0';)
            {
                auto* startLine = ptr;
                uint16_t lineWidth = 0;
                auto lastWordLineWith = lineWidth;
                auto* wordStart = ptr;
                for (; *ptr != '\0' && lineWidth < stringWidth; ++ptr)
                {
                    const auto chr = static_cast<uint8_t>(*ptr);
                    if (chr >= ControlCodes::noArgBegin && chr < ControlCodes::noArgEnd)
                    {
                        bool forceEndl = false;
                        switch (chr)
                        {
                            case ControlCodes::newline:
                            {
                                *ptr = '\0';
                                forceEndl = true;
                                ++ptr; // Skip over '\0' when forcing a new line
                                wrapCount++;
                                // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                                maxWidth = std::max(maxWidth, lineWidth);
                                break;
                            }
                            case ControlCodes::Font::small:
                                font = Font::small;
                                break;
                            case ControlCodes::Font::large:
                                font = Font::large;
                                break;
                            case ControlCodes::Font::bold:
                                font = Font::medium_bold;
                                break;
                            case ControlCodes::Font::regular:
                                font = Font::medium_normal;
                                break;
                        }
                        if (forceEndl)
                        {
                            break;
                        }
                    }
                    else if (chr >= ControlCodes::oneArgBegin && chr < ControlCodes::oneArgEnd)
                    {
                        switch (chr)
                        {
                            case ControlCodes::moveX:
                                lineWidth = static_cast<uint8_t>(ptr[1]);
                                break;
                        }
                        ptr += 1;
                    }
                    else if (chr >= ControlCodes::twoArgBegin && chr < ControlCodes::twoArgEnd)
                    {
                        ptr += 2;
                    }
                    else if (chr >= ControlCodes::fourArgBegin && chr < ControlCodes::fourArgEnd)
                    {
                        switch (chr)
                        {
                            case ControlCodes::inlineSpriteStr:
                            {
                                uint32_t image = *reinterpret_cast<const uint32_t*>(ptr);
                                ImageId imageId{ image & 0x7FFFF };
                                auto* el = getG1Element(imageId.getIndex());
                                if (el != nullptr)
                                {
                                    lineWidth += el->width;
                                }
                                break;
                            }
                        }
                        ptr += 4;
                    }
                    else
                    {
                        if (*ptr == ' ')
                        {
                            wordStart = ptr;
                            lastWordLineWith = lineWidth;
                        }
                        lineWidth += _characterWidths[font + (static_cast<uint8_t>(*ptr) - 32)];
                    }
                }
                if (lineWidth >= stringWidth || *ptr == '\0')
                {
                    if (startLine == wordStart || (*ptr == '\0' && lineWidth < stringWidth))
                    {
                        // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                        maxWidth = std::max(maxWidth, lineWidth);
                        if (startLine == wordStart && *ptr != '\0')
                        {
                            // Shuffle the string forward by one to make space for line ending
                            const auto len = StringManager::locoStrlen(ptr) + 1; // +1 for null termination
                            std::copy_backward(ptr, ptr + len, ptr + len + 1);
                            // Insert line ending
                            *ptr++ = '\0';
                        }
                    }
                    else
                    {
                        // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                        maxWidth = std::max(maxWidth, lastWordLineWith);
                        // Insert line ending instead of space character
                        *wordStart = '\0';
                        ptr = wordStart + 1;
                    }
                    wrapCount++;
                }
            }

            // Note that this is always the font used in the last line.
            // TODO: refactor to pair up with each line, and to not use a global.
            _currentFontSpriteBase = font;
            return std::make_pair(maxWidth, std::max(static_cast<uint16_t>(wrapCount) - 1, 0));
        }

        // 0x0049544E
        // Vanilla would also return maxWidth @<cx> (breakCount-1) @<di>
        // @return numLinesToDisplayAllChars @<ax>
        static uint16_t wrapStringTicker(char* buffer, uint16_t stringWidth, uint16_t numCharacters)
        {
            // std::vector<const char*> wrap; TODO: refactor to return pointers to line starts
            auto font = *_currentFontSpriteBase;
            uint16_t numLinesToDisplayAllChars = 1;

            int16_t charNum = numCharacters;
            for (auto* ptr = buffer; *ptr != '\0';)
            {
                auto* startLine = ptr;
                uint16_t lineWidth = 0;
                auto lastWordCharNum = charNum;
                auto* wordStart = ptr;
                for (; *ptr != '\0' && lineWidth < stringWidth; ++ptr)
                {
                    const auto chr = static_cast<uint8_t>(*ptr);
                    if (chr >= ControlCodes::noArgBegin && chr < ControlCodes::noArgEnd)
                    {
                        bool forceEndl = false;
                        switch (chr)
                        {
                            case ControlCodes::newline:
                            {
                                *ptr = '\0';
                                forceEndl = true;
                                ++ptr; // Skip over '\0' when forcing a new line
                                // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                                if (charNum > 0)
                                {
                                    numLinesToDisplayAllChars++;
                                }
                                break;
                            }
                            case ControlCodes::Font::small:
                                font = Font::small;
                                break;
                            case ControlCodes::Font::large:
                                font = Font::large;
                                break;
                            case ControlCodes::Font::bold:
                                font = Font::medium_bold;
                                break;
                            case ControlCodes::Font::regular:
                                font = Font::medium_normal;
                                break;
                        }
                        if (forceEndl)
                        {
                            break;
                        }
                    }
                    else if (chr >= ControlCodes::oneArgBegin && chr < ControlCodes::oneArgEnd)
                    {
                        switch (*ptr)
                        {
                            case ControlCodes::moveX:
                                lineWidth = static_cast<uint8_t>(ptr[1]);
                                break;
                        }
                        ptr += 1;
                    }
                    else if (chr >= ControlCodes::twoArgBegin && chr < ControlCodes::twoArgEnd)
                    {
                        ptr += 2;
                    }
                    else if (chr >= ControlCodes::fourArgBegin && chr < ControlCodes::fourArgEnd)
                    {
                        switch (chr)
                        {
                            case ControlCodes::inlineSpriteStr:
                            {
                                uint32_t image = *reinterpret_cast<const uint32_t*>(ptr);
                                ImageId imageId{ image & 0x7FFFF };
                                auto* el = getG1Element(imageId.getIndex());
                                if (el != nullptr)
                                {
                                    lineWidth += el->width;
                                }
                                break;
                            }
                        }
                        ptr += 4;
                    }
                    else
                    {
                        if (*ptr == ' ')
                        {
                            wordStart = ptr;
                            lastWordCharNum = charNum;
                        }

                        charNum--;
                        lineWidth += _characterWidths[font + (static_cast<uint8_t>(*ptr) - 32)];
                    }
                }
                if (lineWidth >= stringWidth || *ptr == '\0')
                {
                    if (startLine == wordStart || (*ptr == '\0' && lineWidth < stringWidth))
                    {
                        // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                        if (startLine == wordStart && *ptr != '\0')
                        {
                            // Shuffle the string forward by one to make space for line ending
                            const auto len = StringManager::locoStrlen(ptr) + 1; // +1 for null termination
                            std::copy_backward(ptr, ptr + len, ptr + len + 1);
                            // Insert line ending
                            *ptr++ = '\0';
                        }
                    }
                    else
                    {
                        // wrap.push_back(startLine); TODO: refactor to return pointers to line starts
                        charNum = lastWordCharNum;
                        if (charNum > 0)
                        {
                            numLinesToDisplayAllChars++;
                        }
                        // Insert line ending instead of space character
                        *wordStart = '\0';
                        ptr = wordStart + 1;
                    }
                }
            }

            // Note that this is always the font used in the last line.
            // TODO: refactor to pair up with each line, and to not use a global.
            _currentFontSpriteBase = font;
            return numLinesToDisplayAllChars;
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
                return;
            if (offset.y >= rt.height)
                return;
            // Check to make sure we are drawing at least a pixel
            if (length == 0)
                length++;

            // If x coord outside range leave
            if (offset.x < 0)
            {
                // Unless the number of pixels is enough to be in range
                length += offset.x;
                if (length <= 0)
                    return;
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
                    return;
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
                    length = 1;
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

    void SoftwareDrawingContext::clear(const RenderTarget& rt, uint32_t fill)
    {
        return Impl::clear(rt, fill);
    }

    void SoftwareDrawingContext::clearSingle(const RenderTarget& rt, uint8_t paletteId)
    {
        return Impl::clearSingle(rt, paletteId);
    }

    int16_t SoftwareDrawingContext::clipString(int16_t width, char* string)
    {
        return Impl::clipString(width, string);
    }

    uint16_t SoftwareDrawingContext::getStringWidth(const char* buffer)
    {
        return Impl::getStringWidth(buffer);
    }

    uint16_t SoftwareDrawingContext::getMaxStringWidth(const char* buffer)
    {
        return Impl::getMaxStringWidth(buffer);
    }

    Point SoftwareDrawingContext::drawString(const RenderTarget& rt, Ui::Point origin, AdvancedColour colour, const char* str)
    {
        return Impl::drawString(rt, origin, colour, str);
    }

    Point SoftwareDrawingContext::drawStringLeft(const RenderTarget& rt, Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringLeft(rt, origin, colour, stringId, args);
    }

    Point SoftwareDrawingContext::drawStringLeftClipped(const RenderTarget& rt, Ui::Point origin, uint16_t width, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringLeftClipped(rt, origin, width, colour, stringId, args);
    }

    Point SoftwareDrawingContext::drawStringLeftUnderline(const RenderTarget& rt, Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringLeftUnderline(rt, origin, colour, stringId, args);
    }

    Point SoftwareDrawingContext::drawStringLeftWrapped(const RenderTarget& rt, Ui::Point origin, uint16_t width, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringLeftWrapped(rt, origin, width, colour, stringId, args);
    }

    Point SoftwareDrawingContext::drawStringCentred(const RenderTarget& rt, Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringCentred(rt, origin, colour, stringId, args);
    }

    Point SoftwareDrawingContext::drawStringCentredClipped(const RenderTarget& rt, Ui::Point origin, uint16_t width, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringCentredClipped(rt, origin, width, colour, stringId, args);
    }

    Point SoftwareDrawingContext::drawStringCentredRaw(const RenderTarget& rt, Ui::Point origin, uint16_t linebreakCount, AdvancedColour colour, const char* wrappedStr)
    {
        return Impl::drawStringCentredRaw(rt, origin, linebreakCount, colour, wrappedStr);
    }

    Point SoftwareDrawingContext::drawStringCentredWrapped(const RenderTarget& rt, Ui::Point origin, uint16_t width, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringCentredWrapped(rt, origin, width, colour, stringId, args);
    }

    Point SoftwareDrawingContext::drawStringRight(const RenderTarget& rt, Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringRight(rt, origin, colour, stringId, args);
    }

    Point SoftwareDrawingContext::drawStringRightUnderline(const RenderTarget& rt, Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        return Impl::drawStringRightUnderline(rt, origin, colour, stringId, args);
    }

    void SoftwareDrawingContext::drawStringYOffsets(const RenderTarget& rt, Ui::Point loc, AdvancedColour colour, const void* args, const int8_t* yOffsets)
    {
        return Impl::drawStringYOffsets(rt, loc, colour, args, yOffsets);
    }

    void SoftwareDrawingContext::drawStringTicker(const RenderTarget& rt, Ui::Point origin, StringId stringId, Colour colour, uint8_t numLinesToDisplay, uint16_t numCharactersToDisplay, uint16_t width)
    {
        Impl::drawStringTicker(rt, origin, stringId, colour, numLinesToDisplay, numCharactersToDisplay, width);
    }

    uint16_t SoftwareDrawingContext::getStringWidthNewLined(const char* buffer)
    {
        return Impl::getStringWidthNewLined(buffer);
    }

    std::pair<uint16_t, uint16_t> SoftwareDrawingContext::wrapString(char* buffer, uint16_t stringWidth)
    {
        return Impl::wrapString(buffer, stringWidth);
    }

    void SoftwareDrawingContext::fillRect(const RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t colour, RectFlags flags)
    {
        return Impl::fillRect(rt, left, top, right, bottom, colour, flags);
    }

    void SoftwareDrawingContext::drawRect(const RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, uint8_t colour, RectFlags flags)
    {
        return Impl::drawRect(rt, x, y, dx, dy, colour, flags);
    }

    void SoftwareDrawingContext::fillRectInset(const RenderTarget& rt, int16_t left, int16_t top, int16_t right, int16_t bottom, AdvancedColour colour, RectInsetFlags flags)
    {
        return Impl::fillRectInset(rt, left, top, right, bottom, colour, flags);
    }

    void SoftwareDrawingContext::drawRectInset(const RenderTarget& rt, int16_t x, int16_t y, uint16_t dx, uint16_t dy, AdvancedColour colour, RectInsetFlags flags)
    {
        return Impl::drawRectInset(rt, x, y, dx, dy, colour, flags);
    }

    void SoftwareDrawingContext::drawLine(const RenderTarget& rt, const Ui::Point& a, const Ui::Point& b, PaletteIndex_t colour)
    {
        return Impl::drawLine(rt, a, b, colour);
    }

    void SoftwareDrawingContext::drawImage(const RenderTarget* rt, int16_t x, int16_t y, uint32_t image)
    {
        return Impl::drawImage(rt, x, y, image);
    }

    void SoftwareDrawingContext::drawImage(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image)
    {
        return Impl::drawImage(rt, pos, image);
    }

    void SoftwareDrawingContext::drawImageMasked(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, const ImageId& maskImage)
    {
        return Impl::drawImageMasked(rt, pos, image, maskImage);
    }

    void SoftwareDrawingContext::drawImageSolid(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteIndex_t paletteIndex)
    {
        return Impl::drawImageSolid(rt, pos, image, paletteIndex);
    }

    void SoftwareDrawingContext::drawImagePaletteSet(const RenderTarget& rt, const Ui::Point& pos, const ImageId& image, PaletteMap::View palette, const G1Element* noiseImage)
    {
        return Impl::drawImagePaletteSet(rt, pos, image, palette, noiseImage);
    }

    int16_t SoftwareDrawingContext::getCurrentFontSpriteBase()
    {
        return Impl::getCurrentFontSpriteBase();
    }

    void SoftwareDrawingContext::setCurrentFontSpriteBase(int16_t base)
    {
        return Impl::setCurrentFontSpriteBase(base);
    }

}
