#pragma once

#include <OpenLoco/Core/EnumFlags.hpp>

namespace OpenLoco::Ui
{
    enum class ScrollPart : int16_t
    {
        none = -1,
        view = 0,
        hscrollbarButtonLeft = 1,
        hscrollbarButtonRight = 2,
        hscrollbarTrackLeft = 3,
        hscrollbarTrackRight = 4,
        hscrollbarThumb = 5,
        vscrollbarButtonTop = 6,
        vscrollbarButtonBottom = 7,
        vscrollbarTrackTop = 8,
        vscrollbarTrackBottom = 9,
        vscrollbarThumb = 10,
    };

    namespace Scrollbars
    {
        constexpr uint8_t none = 0;
        constexpr uint8_t horizontal = (1 << 0);
        constexpr uint8_t vertical = (1 << 1);
        constexpr uint8_t both = horizontal | vertical;
    }

    enum class ScrollFlags : uint16_t
    {
        none = 0U,
        hscrollbarVisible = 1U << 0,
        hscrollbarThumbPressed = 1U << 1,
        hscrollbarLeftPressed = 1U << 2,
        hscrollbarRightPressed = 1U << 3,
        vscrollbarVisible = 1U << 4,
        vscrollbarThumbPressed = 1U << 5,
        vscrollbarUpPressed = 1U << 6,
        vscrollbarDownPressed = 1U << 7,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ScrollFlags);

    struct ScrollArea
    {
        ScrollFlags flags;      // 0x00
        int16_t contentOffsetX; // 0x02
        int16_t contentWidth;   // 0x04
        uint16_t hThumbLeft;    // 0x06
        uint16_t hThumbRight;   // 0x08
        int16_t contentOffsetY; // 0x0A
        int16_t contentHeight;  // 0x0C
        uint16_t vThumbTop;     // 0x0E
        uint16_t vThumbBottom;  // 0x10

        constexpr bool hasFlags(ScrollFlags flagsToTest) const
        {
            return (flags & flagsToTest) != ScrollFlags::none;
        }
    };
};
