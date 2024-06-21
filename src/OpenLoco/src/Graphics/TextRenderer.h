#pragma once

#include "DrawingContext.h"
#include "Font.h"
#include "Graphics/Gfx.h"
#include "Graphics/PaletteMap.h"
#include "Types.hpp"
#include <OpenLoco/Engine/Ui/Rect.hpp>
#include <cstdint>

namespace OpenLoco::Gfx
{
    class DrawingContext;

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

    class TextRenderer
    {
        DrawingContext& _ctx;

    public:
        TextRenderer(DrawingContext& ctx);

        Font getCurrentFont();

        void setCurrentFont(Font base);

        int16_t clipString(int16_t width, char* string);
        uint16_t getStringWidth(const char* buffer);
        uint16_t getMaxStringWidth(const char* buffer);

        Ui::Point drawString(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            const char* str);

        Ui::Point drawStringLeft(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        Ui::Point drawStringLeftClipped(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        Ui::Point drawStringLeftUnderline(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        Ui::Point drawStringLeftWrapped(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        Ui::Point drawStringCentred(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        Ui::Point drawStringCentredClipped(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        Ui::Point drawStringCentredRaw(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t linebreakCount,
            AdvancedColour colour,
            const char* wrappedStr);

        Ui::Point drawStringCentredWrapped(
            const RenderTarget& rt,
            Ui::Point origin,
            uint16_t width,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        Ui::Point drawStringRight(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        Ui::Point drawStringRightUnderline(
            const RenderTarget& rt,
            Ui::Point origin,
            AdvancedColour colour,
            StringId stringId,
            FormatArgumentsView args = {});

        void drawStringYOffsets(const RenderTarget& rt, Ui::Point loc, AdvancedColour colour, const char* str, const int8_t* yOffsets);
        void drawStringTicker(const RenderTarget& rt, Ui::Point origin, StringId stringId, Colour colour, uint8_t numLinesToDisplay, uint16_t numCharactersToDisplay, uint16_t width);
        uint16_t getStringWidthNewLined(const char* buffer);
        std::pair<uint16_t, uint16_t> wrapString(char* buffer, uint16_t stringWidth);
    };
}
