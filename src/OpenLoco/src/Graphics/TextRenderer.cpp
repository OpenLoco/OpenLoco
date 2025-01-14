#include "TextRenderer.h"
#include "DrawingContext.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Localisation/Formatting.h"
#include "RenderTarget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Gfx
{
    namespace Impl
    {

        // TODO: Store in drawing context.
        static PaletteMap::Buffer<8> _textColours{ 0 };
        static uint16_t getStringWidth(Font font, const char* str);
        static std::pair<uint16_t, uint16_t> wrapString(Font font, char* buffer, uint16_t stringWidth);
        static uint16_t wrapStringTicker(Font font, char* buffer, uint16_t stringWidth, uint16_t numCharacters);
        static int16_t clipString(Font font, int16_t width, char* string);

        static void setTextColours(TextDrawFlags fontFlags, PaletteIndex_t pal1, PaletteIndex_t pal2, PaletteIndex_t pal3)
        {
            if ((fontFlags & TextDrawFlags::inset) != TextDrawFlags::none)
            {
                return;
            }

            _textColours[PaletteIndex::textRemap0] = pal1;
            _textColours[PaletteIndex::textRemap1] = PaletteIndex::transparent;
            _textColours[PaletteIndex::textRemap2] = PaletteIndex::transparent;
            if ((fontFlags & TextDrawFlags::outline) != TextDrawFlags::none)
            {
                _textColours[PaletteIndex::textRemap1] = pal2;
                _textColours[PaletteIndex::textRemap2] = pal3;
            }
        }

        static void setTextColour(TextDrawFlags fontFlags, int colour)
        {
            const auto* el = getG1Element(ImageIds::text_palette);
            setTextColours(fontFlags, el->offset[colour * 4 + 0], el->offset[colour * 4 + 1], el->offset[colour * 4 + 2]);
        }

        // 0x00451189
        static Ui::Point loopNewline(Font font, TextDrawFlags fontFlags, DrawingContext& ctx, const RenderTarget* rt, Ui::Point origin, const char* str)
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
                        if (font == Font::medium_normal || font == Font::medium_bold)
                        {
                            pos.y += 5;
                        }
                        else if (font == Font::small)
                        {
                            pos.y += 3;
                        }
                        else if (font == Font::large)
                        {
                            pos.y += 9;
                        }
                        break;

                    case ControlCodes::newline:
                        pos.x = origin.x;
                        if (font == Font::medium_normal || font == Font::medium_bold)
                        {
                            pos.y += 10;
                        }
                        else if (font == Font::small)
                        {
                            pos.y += 6;
                        }
                        else if (font == Font::large)
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
                        font = Font::small;
                        break;
                    case ControlCodes::Font::large:
                        font = Font::large;
                        break;
                    case ControlCodes::Font::regular:
                        font = Font::medium_normal;
                        break;
                    case ControlCodes::Font::bold:
                        font = Font::medium_bold;
                        break;
                    case ControlCodes::Font::outline:
                        fontFlags |= TextDrawFlags::outline;
                        break;
                    case ControlCodes::Font::outlineOff:
                        fontFlags &= ~TextDrawFlags::outline;
                        break;
                    case ControlCodes::windowColour1:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::primary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 7), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour2:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::secondary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour3:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::tertiary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour4:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::quaternary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }

                    case ControlCodes::inlineSpriteStr:
                    {
                        uint32_t image = ((uint32_t*)str)[0];
                        ImageId imageId{ image & 0x7FFFF };
                        str += 4;

                        if ((fontFlags & TextDrawFlags::inset) != TextDrawFlags::none)
                        {
                            ctx.drawImageSolid(pos, imageId, _textColours[PaletteIndex::textRemap2]);
                            ctx.drawImageSolid(pos + Ui::Point{ 1, 1 }, imageId, _textColours[PaletteIndex::textRemap0]);
                        }
                        else
                        {
                            ctx.drawImage(pos.x, pos.y, image);
                        }

                        pos.x += getG1Element(imageId.getIndex())->width;
                        break;
                    }

                    case ControlCodes::Colour::black:
                        setTextColour(fontFlags, 0);
                        break;

                    case ControlCodes::Colour::grey:
                        setTextColour(fontFlags, 1);
                        break;

                    case ControlCodes::Colour::white:
                        setTextColour(fontFlags, 2);
                        break;

                    case ControlCodes::Colour::red:
                        setTextColour(fontFlags, 3);
                        break;

                    case ControlCodes::Colour::green:
                        setTextColour(fontFlags, 4);
                        break;

                    case ControlCodes::Colour::yellow:
                        setTextColour(fontFlags, 5);
                        break;

                    case ControlCodes::Colour::topaz:
                        setTextColour(fontFlags, 6);
                        break;

                    case ControlCodes::Colour::celadon:
                        setTextColour(fontFlags, 7);
                        break;

                    case ControlCodes::Colour::babyBlue:
                        setTextColour(fontFlags, 8);
                        break;

                    case ControlCodes::Colour::paleLavender:
                        setTextColour(fontFlags, 9);
                        break;

                    case ControlCodes::Colour::paleGold:
                        setTextColour(fontFlags, 10);
                        break;

                    case ControlCodes::Colour::lightPink:
                        setTextColour(fontFlags, 11);
                        break;

                    case ControlCodes::Colour::pearlAqua:
                        setTextColour(fontFlags, 12);
                        break;

                    case ControlCodes::Colour::paleSilver:
                        setTextColour(fontFlags, 13);
                        break;

                    default:
                        if (!offscreen)
                        {
                            // When off-screen in the y dimension there is no requirement to keep pos.x correct
                            if (chr >= 32)
                            {
                                const auto chrImage = getImageForCharacter(font, chr);
                                // Use withPrimary to set imageId flag to use the correct palette code (Colour::black is not actually used)
                                ctx.drawImagePaletteSet(pos, chrImage.withPrimary(Colour::black), PaletteMap::View{ _textColours }, {});
                                pos.x += Gfx::getCharacterWidth(font, chr);
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
        static Ui::Point drawString(Font font, TextDrawFlags fontFlags, DrawingContext& ctx, const RenderTarget& rt, Ui::Point origin, AdvancedColour colour, const char* str)
        {
            if (colour.isFE())
            {
                return loopNewline(font, fontFlags, ctx, &rt, origin, str);
            }

            if (colour.isFD())
            {
                fontFlags = TextDrawFlags::none;
                setTextColour(fontFlags, 0);
                return loopNewline(font, fontFlags, ctx, &rt, origin, str);
            }

            if (origin.x >= rt.x + rt.width)
            {
                return origin;
            }

            if (origin.x < rt.x - 1280)
            {
                return origin;
            }

            if (origin.y >= rt.y + rt.height)
            {
                return origin;
            }

            if (origin.y < rt.y - 90)
            {
                return origin;
            }

            if (colour.isFF())
            {
                return loopNewline(font, fontFlags, ctx, &rt, origin, str);
            }

            fontFlags = TextDrawFlags::none;
            if (font == Font::m1)
            {
                font = Font::medium_bold;
                fontFlags |= TextDrawFlags::dark;
            }
            else if (font == Font::m2)
            {
                font = Font::medium_bold;
                fontFlags |= TextDrawFlags::dark;
                fontFlags |= TextDrawFlags::extraDark;
            }

            _textColours[PaletteIndex::transparent] = PaletteIndex::transparent;
            _textColours[PaletteIndex::textRemap0] = Colours::getShade(Colour::mutedDarkPurple, 5);
            _textColours[PaletteIndex::textRemap1] = Colours::getShade(Colour::mutedRed, 5);
            _textColours[PaletteIndex::textRemap2] = Colours::getShade(Colour::blue, 5);

            if (colour.isOutline())
            {
                colour = colour.clearOutline();
                fontFlags |= TextDrawFlags::outline;
            }

            if (colour.isInset())
            {
                colour = colour.clearInset();
                fontFlags |= TextDrawFlags::inset;
            }

            if ((fontFlags & TextDrawFlags::inset) != TextDrawFlags::none)
            {
                if ((fontFlags & TextDrawFlags::dark) != TextDrawFlags::none && (fontFlags & TextDrawFlags::extraDark) != TextDrawFlags::none)
                {
                    _textColours[PaletteIndex::textRemap0] = Colours::getShade(colour.c(), 2);
                    _textColours[PaletteIndex::textRemap1] = PaletteIndex::transparent;
                    _textColours[PaletteIndex::textRemap2] = Colours::getShade(colour.c(), 4);
                }
                else if ((fontFlags & TextDrawFlags::dark) != TextDrawFlags::none)
                {
                    _textColours[PaletteIndex::textRemap0] = Colours::getShade(colour.c(), 3);
                    _textColours[PaletteIndex::textRemap1] = PaletteIndex::transparent;
                    _textColours[PaletteIndex::textRemap2] = Colours::getShade(colour.c(), 5);
                }
                else
                {
                    _textColours[PaletteIndex::textRemap0] = Colours::getShade(colour.c(), 4);
                    _textColours[PaletteIndex::textRemap1] = PaletteIndex::transparent;
                    _textColours[PaletteIndex::textRemap2] = Colours::getShade(colour.c(), 6);
                }
            }
            else
            {
                setTextColours(fontFlags, Colours::getShade(colour.c(), 9), PaletteIndex::black0, PaletteIndex::black0);
            }

            return loopNewline(font, fontFlags, ctx, &rt, origin, str);
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
                {
                    return ptr;
                }

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

        static uint16_t lineHeightFromFont(Font font)
        {
            if (font <= Font::medium_bold)
            {
                return 10;
            }
            else if (font == Font::small)
            {
                return 6;
            }
            else if (font == Font::large)
            {
                return 18;
            }
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
        static Ui::Point drawStringLeftWrapped(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            int16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            // Setup the text colours (FIXME: This should be a separate function)
            char empty[1] = "";
            drawString(font, fontFlags, ctx, rt, origin, colour, empty);

            font = Font::medium_bold;
            auto wrapResult = wrapString(font, buffer, width);
            auto breakCount = wrapResult.second + 1;

            // wrapString might change the font due to formatting codes
            uint16_t lineHeight = lineHeightFromFont(font); // _112D404

            fontFlags = TextDrawFlags::none;
            auto point = origin;

            const char* ptr = buffer;
            for (auto i = 0; ptr != nullptr && i < breakCount; i++)
            {
                drawString(font, fontFlags, ctx, rt, point, AdvancedColour::FE(), const_cast<char*>(ptr));
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
        static Ui::Point drawStringLeft(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            return drawString(font, fontFlags, ctx, rt, origin, colour, buffer);
        }

        // 0x00494BBF
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        // bp: width
        static Ui::Point drawStringLeftClipped(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            clipString(font, width, buffer);

            return drawString(font, fontFlags, ctx, rt, origin, colour, buffer);
        }

        // 0x00494C78
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Ui::Point drawStringRight(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            uint16_t width = getStringWidth(font, buffer);

            auto point = origin;
            point.x -= width;

            return drawString(font, fontFlags, ctx, rt, point, colour, buffer);
        }

        // 0x00494CB2
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Ui::Point drawStringRightUnderline(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            uint16_t width = getStringWidth(font, buffer);
            auto point = origin;
            point.x -= width;

            drawString(font, fontFlags, ctx, rt, point, colour, buffer);

            // Draw underline
            ctx.drawRect(point.x, point.y + 11, width, 1, _textColours[PaletteIndex::textRemap0], RectFlags::none);
            if (_textColours[PaletteIndex::textRemap1] != 0)
            {
                ctx.drawRect(point.x, point.y + 12, width, 1, _textColours[PaletteIndex::textRemap1], RectFlags::none);
            }

            return point;
        }

        // 0x00494D78
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Ui::Point drawStringLeftUnderline(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            uint16_t width = getStringWidth(font, buffer);

            auto point = drawString(font, fontFlags, ctx, rt, origin, colour, buffer);

            // Draw underline
            ctx.drawRect(origin.x, origin.y + 11, width, 1, _textColours[PaletteIndex::textRemap0], RectFlags::none);
            if (_textColours[PaletteIndex::textRemap1] != 0)
            {
                ctx.drawRect(origin.x, origin.y + 12, width, 1, _textColours[PaletteIndex::textRemap1], RectFlags::none);
            }

            return point;
        }

        // 0x00494DE8
        // al: colour
        // bx: string id
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Ui::Point drawStringCentred(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            uint16_t width = getStringWidth(font, buffer);

            auto point = origin;
            point.x = origin.x - (width / 2);

            if (point.x < 0)
            {
                return origin;
            }

            return drawString(font, fontFlags, ctx, rt, point, colour, buffer);
        }

        // 0x00494C36
        // al: colour
        // bx: string id
        // bp: width
        // cx: x
        // dx: y
        // esi: args
        // edi: rt
        static Ui::Point drawStringCentredClipped(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            width = clipString(font, width, buffer);

            auto point = Ui::Point(origin.x - (width / 2), origin.y);
            return drawString(font, fontFlags, ctx, rt, point, colour, buffer);
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
        static Ui::Point drawStringCentredWrapped(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args)
        {
            font = Font::medium_bold;
            // Setup the text colours (FIXME: This should be a separate function)
            char empty[1] = "";
            drawString(font, fontFlags, ctx, rt, origin, colour, empty);

            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId, args);

            font = Font::medium_bold;
            auto wrapResult = wrapString(font, buffer, width);
            auto breakCount = wrapResult.second + 1;

            // wrapString might change the font due to formatting codes
            uint16_t lineHeight = lineHeightFromFont(font); // _112D404

            fontFlags = TextDrawFlags::none;

            Ui::Point basePoint = origin;
            basePoint.y -= (lineHeight / 2) * (breakCount - 1);

            const char* ptr = buffer;
            uint16_t lineWidth{};

            for (auto i = 0; ptr != nullptr && i < breakCount; i++)
            {
                lineWidth = getStringWidth(font, ptr);

                auto point = basePoint;
                point.x -= lineWidth / 2;

                drawString(font, fontFlags, ctx, rt, point, AdvancedColour::FE(), const_cast<char*>(ptr));
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
        static Ui::Point drawStringCentredRaw(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t linebreakCount,
            AdvancedColour colour,
            const char* wrappedStr)
        {
            font = Font::medium_bold;
            // Setup the text colours (FIXME: This should be a separate function)
            char empty[1] = "";
            drawString(font, fontFlags, ctx, rt, origin, colour, empty);

            font = Font::medium_bold;
            fontFlags = TextDrawFlags::none;

            const char* ptr = wrappedStr;
            auto basePoint = origin;

            for (auto i = 0; i < linebreakCount + 1 && ptr != nullptr; ++i)
            {
                uint16_t lineWidth = getStringWidth(font, ptr);

                auto point = basePoint;
                point.x -= lineWidth / 2;

                drawString(font, fontFlags, ctx, rt, point, AdvancedColour::FE(), const_cast<char*>(ptr));

                ptr = advanceToNextLineWrapped(ptr);
                basePoint.y += lineHeightFromFont(font);
            }

            return basePoint;
        }

        static void drawStringYOffsets(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            const Ui::Point& loc,
            AdvancedColour colour,
            const char* str,
            const int8_t* yOffsets)
        {
            // This function has been somewhat simplified removing unreachable parts
            if (colour.isFE())
            {
                assert(false);
                return;
            }
            if (loc.x >= rt.x + rt.width)
            {
                return;
            }

            if (loc.x < rt.x - 1280)
            {
                return;
            }

            if (loc.y >= rt.y + rt.height)
            {
                return;
            }

            // Note: 60 not 90 like drawString
            if (loc.y < rt.y - 60)
            {
                return;
            }

            fontFlags = TextDrawFlags::none;

            setTextColours(fontFlags, Colours::getShade(colour.c(), 9), PaletteIndex::black0, PaletteIndex::black0);

            Ui::Point pos = loc;
            while (true)
            {
                // When off-screen in y dimension don't draw text
                // In original this check only performed if pos.y updated instead of every loop
                bool offscreen = true;
                // Note: 39 unlike drawString
                if (pos.y + 39 > rt.y)
                {
                    if (rt.y + rt.height > pos.y - 20)
                    {
                        offscreen = false;
                    }
                }

                const auto chr = static_cast<uint8_t>(*str);
                str++;

                switch (chr)
                {
                    case 0U:
                        return;

                    case ControlCodes::adjustPalette:
                        // This control character does not appear in the localisation files
                        assert(false);
                        str++;
                        break;

                    case ControlCodes::newlineSmaller:
                        pos.x = loc.x;
                        if (font == Font::medium_normal || font == Font::medium_bold)
                        {
                            pos.y += 5;
                        }
                        else if (font == Font::small)
                        {
                            pos.y += 3;
                        }
                        else if (font == Font::large)
                        {
                            pos.y += 9;
                        }
                        break;

                    case ControlCodes::newline:
                        pos.x = loc.x;
                        if (font == Font::medium_normal || font == Font::medium_bold)
                        {
                            pos.y += 10;
                        }
                        else if (font == Font::small)
                        {
                            pos.y += 6;
                        }
                        else if (font == Font::large)
                        {
                            pos.y += 18;
                        }
                        break;

                    case ControlCodes::moveX:
                    {
                        uint8_t offset = *str;
                        str++;
                        pos.x = loc.x + offset;

                        break;
                    }

                    case ControlCodes::newlineXY:
                    {
                        uint8_t offset = *str;
                        str++;
                        pos.x = loc.x + offset;

                        offset = *str;
                        str++;
                        pos.y = loc.y + offset;

                        break;
                    }

                    case ControlCodes::Font::small:
                        font = Font::small;
                        break;
                    case ControlCodes::Font::large:
                        font = Font::large;
                        break;
                    case ControlCodes::Font::regular:
                        font = Font::medium_normal;
                        break;
                    case ControlCodes::Font::bold:
                        font = Font::medium_bold;
                        break;
                    case ControlCodes::Font::outline:
                        fontFlags |= TextDrawFlags::outline;
                        break;
                    case ControlCodes::Font::outlineOff:
                        fontFlags &= ~TextDrawFlags::outline;
                        break;
                    case ControlCodes::windowColour1:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::primary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 7), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour2:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::secondary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour3:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::tertiary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour4:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::quaternary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }

                    case ControlCodes::inlineSpriteStr:
                    {
                        str += 4;
                        // Not handled for YOffsets code
                        assert(false);
                        break;
                    }

                    case ControlCodes::Colour::black:
                        setTextColour(fontFlags, 0);
                        break;

                    case ControlCodes::Colour::grey:
                        setTextColour(fontFlags, 1);
                        break;

                    case ControlCodes::Colour::white:
                        setTextColour(fontFlags, 2);
                        break;

                    case ControlCodes::Colour::red:
                        setTextColour(fontFlags, 3);
                        break;

                    case ControlCodes::Colour::green:
                        setTextColour(fontFlags, 4);
                        break;

                    case ControlCodes::Colour::yellow:
                        setTextColour(fontFlags, 5);
                        break;

                    case ControlCodes::Colour::topaz:
                        setTextColour(fontFlags, 6);
                        break;

                    case ControlCodes::Colour::celadon:
                        setTextColour(fontFlags, 7);
                        break;

                    case ControlCodes::Colour::babyBlue:
                        setTextColour(fontFlags, 8);
                        break;

                    case ControlCodes::Colour::paleLavender:
                        setTextColour(fontFlags, 9);
                        break;

                    case ControlCodes::Colour::paleGold:
                        setTextColour(fontFlags, 10);
                        break;

                    case ControlCodes::Colour::lightPink:
                        setTextColour(fontFlags, 11);
                        break;

                    case ControlCodes::Colour::pearlAqua:
                        setTextColour(fontFlags, 12);
                        break;

                    case ControlCodes::Colour::paleSilver:
                        setTextColour(fontFlags, 13);
                        break;

                    default:
                        if (pos.x >= rt.x + rt.width)
                        {
                            offscreen = true;
                        }
                        if (!offscreen)
                        {
                            if (pos.x + 26 < rt.x)
                            {
                                pos.x += Gfx::getCharacterWidth(font, chr);
                                yOffsets++;
                            }
                            else
                            {
                                if (chr >= 32)
                                {
                                    // Use withPrimary to set imageId flag to use the correct palette code (Colour::black is not actually used)
                                    const auto chrImage = getImageForCharacter(font, chr);
                                    ctx.drawImagePaletteSet(pos + Ui::Point(0, *yOffsets), chrImage.withPrimary(Colour::black), PaletteMap::View{ _textColours }, {});
                                    pos.x += Gfx::getCharacterWidth(font, chr);
                                    yOffsets++;
                                }
                                else
                                {
                                    // Unhandled control code
                                    assert(false);
                                }
                            }
                        }
                        break;
                }
            }
        }

        // 0x00451582
        static int16_t drawStringMaxChars(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            const AdvancedColour colour,
            uint8_t* str,
            const int16_t numCharsRemaining)
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
                        if (font == Font::medium_normal || font == Font::medium_bold)
                        {
                            pos.y += 5;
                        }
                        else if (font == Font::small)
                        {
                            pos.y += 3;
                        }
                        else if (font == Font::large)
                        {
                            pos.y += 9;
                        }
                        break;

                    case ControlCodes::newline:
                        pos.x = origin.x;
                        if (font == Font::medium_normal || font == Font::medium_bold)
                        {
                            pos.y += 10;
                        }
                        else if (font == Font::small)
                        {
                            pos.y += 6;
                        }
                        else if (font == Font::large)
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
                        font = Font::small;
                        break;
                    case ControlCodes::Font::large:
                        font = Font::large;
                        break;
                    case ControlCodes::Font::regular:
                        font = Font::medium_normal;
                        break;
                    case ControlCodes::Font::bold:
                        font = Font::medium_bold;
                        break;
                    case ControlCodes::Font::outline:
                        fontFlags |= TextDrawFlags::outline;
                        break;
                    case ControlCodes::Font::outlineOff:
                        fontFlags &= ~TextDrawFlags::outline;
                        break;
                    case ControlCodes::windowColour1:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::primary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 7), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour2:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::secondary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour3:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::tertiary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }
                    case ControlCodes::windowColour4:
                    {
                        auto hue = Ui::WindowManager::getWindowColour(Ui::WindowColour::quaternary).c();
                        setTextColours(fontFlags, Colours::getShade(hue, 9), PaletteIndex::black0, PaletteIndex::black0);
                        break;
                    }

                    case ControlCodes::inlineSpriteStr:
                    {
                        uint32_t image = ((uint32_t*)str)[0];
                        ImageId imageId{ image & 0x7FFFF };
                        str += 4;

                        ctx.drawImage(pos.x, pos.y, image);

                        // For some reason the wrapStringTicker doesn't do this??
                        numChars--;
                        pos.x += getG1Element(imageId.getIndex())->width;
                        break;
                    }

                    case ControlCodes::Colour::black:
                        setTextColour(fontFlags, 0);
                        break;

                    case ControlCodes::Colour::grey:
                        setTextColour(fontFlags, 1);
                        break;

                    case ControlCodes::Colour::white:
                        setTextColour(fontFlags, 2);
                        break;

                    case ControlCodes::Colour::red:
                        setTextColour(fontFlags, 3);
                        break;

                    case ControlCodes::Colour::green:
                        setTextColour(fontFlags, 4);
                        break;

                    case ControlCodes::Colour::yellow:
                        setTextColour(fontFlags, 5);
                        break;

                    case ControlCodes::Colour::topaz:
                        setTextColour(fontFlags, 6);
                        break;

                    case ControlCodes::Colour::celadon:
                        setTextColour(fontFlags, 7);
                        break;

                    case ControlCodes::Colour::babyBlue:
                        setTextColour(fontFlags, 8);
                        break;

                    case ControlCodes::Colour::paleLavender:
                        setTextColour(fontFlags, 9);
                        break;

                    case ControlCodes::Colour::paleGold:
                        setTextColour(fontFlags, 10);
                        break;

                    case ControlCodes::Colour::lightPink:
                        setTextColour(fontFlags, 11);
                        break;

                    case ControlCodes::Colour::pearlAqua:
                        setTextColour(fontFlags, 12);
                        break;

                    case ControlCodes::Colour::paleSilver:
                        setTextColour(fontFlags, 13);
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
                                const auto chrImage = getImageForCharacter(font, chr);
                                ctx.drawImagePaletteSet(pos, chrImage.withPrimary(Colour::black), PaletteMap::View{ _textColours }, {});
                                pos.x += Gfx::getCharacterWidth(font, chr);
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
        static void drawStringTicker(
            Font font,
            TextDrawFlags fontFlags,
            DrawingContext& ctx,
            const RenderTarget& rt,
            Ui::Point origin,
            StringId stringId,
            Colour colour,
            uint8_t numLinesToDisplay,
            uint16_t numCharactersToDisplay,
            uint16_t width)
        {
            font = Font::medium_bold;
            // Setup the text colours (FIXME: This should be a separate function)
            char empty[1] = "";
            drawString(font, fontFlags, ctx, rt, origin, colour, empty);

            char buffer[512];
            StringManager::formatString(buffer, std::size(buffer), stringId);

            font = Font::medium_bold;
            const auto numLinesToDisplayAllChars = wrapStringTicker(font, buffer, width, numCharactersToDisplay);
            const auto lineToDisplayFrom = numLinesToDisplayAllChars - numLinesToDisplay;

            // wrapString might change the font due to formatting codes
            uint16_t lineHeight = lineHeightFromFont(font); // _112D404

            fontFlags = TextDrawFlags::none;
            Ui::Point point = origin;
            if (lineToDisplayFrom > 0)
            {
                point.y -= lineHeight * lineToDisplayFrom;
            }
            const char* ptr = buffer;

            auto numChars = numCharactersToDisplay;
            for (auto i = 0; ptr != nullptr && i < numLinesToDisplayAllChars; i++)
            {
                uint16_t lineWidth = getStringWidth(font, ptr);

                // special drawstring
                numChars = drawStringMaxChars(font, fontFlags, ctx, rt, point - Ui::Point(lineWidth / 2, 0), AdvancedColour::FE(), reinterpret_cast<uint8_t*>(const_cast<char*>(ptr)), numChars);
                ptr = advanceToNextLineWrapped(ptr);
                point.y += lineHeight;
            }
        }

        // 0x00495301
        // Note: Returned break count is -1. TODO: Refactor out this -1.
        // @return maxWidth @<cx> (numLinesToDisplayAllChars-1) @<di>
        static std::pair<uint16_t, uint16_t> wrapString(Font font, char* buffer, uint16_t stringWidth)
        {
            // std::vector<const char*> wrap; TODO: refactor to return pointers to line starts
            uint16_t wrapCount = 0;
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
                        lineWidth += Gfx::getCharacterWidth(font, static_cast<uint8_t>(*ptr));
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

            return std::make_pair(maxWidth, std::max(static_cast<uint16_t>(wrapCount) - 1, 0));
        }

        // 0x0049544E
        // Vanilla would also return maxWidth @<cx> (breakCount-1) @<di>
        // @return numLinesToDisplayAllChars @<ax>
        static uint16_t wrapStringTicker(Font font, char* buffer, uint16_t stringWidth, uint16_t numCharacters)
        {
            // std::vector<const char*> wrap; TODO: refactor to return pointers to line starts
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
                        lineWidth += Gfx::getCharacterWidth(font, static_cast<uint8_t>(*ptr));
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

            return numLinesToDisplayAllChars;
        }

        // 0x004957C4
        static int16_t clipString(Font font, int16_t width, char* string)
        {
            if (width < 6)
            {
                *string = '\0';
                return 0;
            }

            // If width of the full string is less than allowed width then we don't need to clip
            auto clippedWidth = getStringWidth(font, string);
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

                auto ellipsedWidth = getStringWidth(font, ellipseString.c_str());
                if (ellipsedWidth < width)
                {
                    // Keep best string with ellipse
                    bestString = ellipseString;
                }
                else
                {
                    StringManager::locoStrcpy(string, bestString.c_str());
                    return getStringWidth(font, string);
                }
            }
            return getStringWidth(font, string);
        }

        /**
         * 0x00495685
         *
         * @param buffer @<esi>
         * @return width @<cx>
         */
        static uint16_t getStringWidth(Font font, const char* str)
        {
            uint16_t width = 0;
            while (*str != '\0')
            {
                const auto chr = static_cast<uint8_t>(*str);
                str++;

                if (chr >= 32)
                {
                    width += Gfx::getCharacterWidth(font, chr);
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

        static std::tuple<uint16_t, const char*, Font> getStringWidthOneLine(const char* ptr, Font font)
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
                    lineWidth += Gfx::getCharacterWidth(font, chr);
                }
            }
            return std::make_tuple(lineWidth, ptr, font);
        }

        // 0x00495715
        // @param buffer @<esi>
        // @return width @<cx>
        static uint16_t getStringWidthNewLined(Font font, const char* buffer)
        {
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
        static uint16_t getMaxStringWidth(Font font, const char* str)
        {
            uint16_t width = 0;
            uint16_t maxWidth = 0;

            while (*str != '\0')
            {
                const auto chr = static_cast<uint8_t>(*str);
                str++;

                if (chr >= 32)
                {
                    width += Gfx::getCharacterWidth(font, chr);
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

    } // namespace Impl

    TextRenderer::TextRenderer(DrawingContext& ctx)
        : _ctx(ctx)
    {
    }

    Font TextRenderer::getCurrentFont()
    {
        return _currentFontSpriteBase;
    }

    void TextRenderer::setCurrentFont(Font base)
    {
        _currentFontSpriteBase = base;
    }

    int16_t TextRenderer::clipString(int16_t width, char* string)
    {
        return Impl::clipString(_currentFontSpriteBase, width, string);
    }

    int16_t TextRenderer::clipString(Font font, int16_t width, char* string)
    {
        return Impl::clipString(font, width, string);
    }

    uint16_t TextRenderer::getStringWidth(const char* buffer)
    {
        return Impl::getStringWidth(_currentFontSpriteBase, buffer);
    }

    uint16_t TextRenderer::getStringWidth(const Font base, const char* buffer)
    {
        return Impl::getStringWidth(base, buffer);
    }

    uint16_t TextRenderer::getMaxStringWidth(const char* buffer)
    {
        return Impl::getMaxStringWidth(_currentFontSpriteBase, buffer);
    }

    uint16_t TextRenderer::getMaxStringWidth(Font font, const char* buffer)
    {
        return Impl::getMaxStringWidth(font, buffer);
    }

    Ui::Point TextRenderer::drawString(Ui::Point origin, AdvancedColour colour, const char* str)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawString(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, colour, str);
    }

    Ui::Point TextRenderer::drawStringLeft(Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringLeft(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, colour, stringId, args);
    }

    Ui::Point TextRenderer::drawStringLeftClipped(Ui::Point origin, uint16_t width, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringLeftClipped(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, width, colour, stringId, args);
    }

    Ui::Point TextRenderer::drawStringLeftUnderline(Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringLeftUnderline(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, colour, stringId, args);
    }

    Ui::Point TextRenderer::drawStringLeftWrapped(Ui::Point origin, uint16_t width, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringLeftWrapped(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, width, colour, stringId, args);
    }

    Ui::Point TextRenderer::drawStringCentred(Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringCentred(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, colour, stringId, args);
    }

    Ui::Point TextRenderer::drawStringCentredClipped(Ui::Point origin, uint16_t width, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringCentredClipped(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, width, colour, stringId, args);
    }

    Ui::Point TextRenderer::drawStringCentredRaw(Ui::Point origin, uint16_t linebreakCount, AdvancedColour colour, const char* wrappedStr)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringCentredRaw(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, linebreakCount, colour, wrappedStr);
    }

    Ui::Point TextRenderer::drawStringCentredWrapped(Ui::Point origin, uint16_t width, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringCentredWrapped(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, width, colour, stringId, args);
    }

    Ui::Point TextRenderer::drawStringRight(Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringRight(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, colour, stringId, args);
    }

    Ui::Point TextRenderer::drawStringRightUnderline(Ui::Point origin, AdvancedColour colour, StringId stringId, FormatArgumentsView args /* = {}*/)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringRightUnderline(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, colour, stringId, args);
    }

    void TextRenderer::drawStringYOffsets(Ui::Point loc, AdvancedColour colour, const char* str, const int8_t* yOffsets)
    {
        auto& rt = _ctx.currentRenderTarget();
        return Impl::drawStringYOffsets(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, loc, colour, str, yOffsets);
    }

    void TextRenderer::drawStringTicker(Ui::Point origin, StringId stringId, Colour colour, uint8_t numLinesToDisplay, uint16_t numCharactersToDisplay, uint16_t width)
    {
        auto& rt = _ctx.currentRenderTarget();
        Impl::drawStringTicker(_currentFontSpriteBase, _currentFontFlags, _ctx, rt, origin, stringId, colour, numLinesToDisplay, numCharactersToDisplay, width);
    }

    uint16_t TextRenderer::getStringWidthNewLined(const char* buffer)
    {
        return Impl::getStringWidthNewLined(_currentFontSpriteBase, buffer);
    }

    uint16_t TextRenderer::getStringWidthNewLined(Font font, const char* buffer)
    {
        return Impl::getStringWidthNewLined(font, buffer);
    }

    std::pair<uint16_t, uint16_t> TextRenderer::wrapString(char* buffer, uint16_t stringWidth)
    {
        return Impl::wrapString(_currentFontSpriteBase, buffer, stringWidth);
    }

    std::pair<uint16_t, uint16_t> TextRenderer::wrapString(Font font, char* buffer, uint16_t stringWidth)
    {
        return Impl::wrapString(font, buffer, stringWidth);
    }

}
