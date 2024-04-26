#pragma once

#include "Graphics/Gfx.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include <cstdint>

namespace OpenLoco::Gfx
{
    enum class RectInsetFlags : uint8_t;
}

namespace OpenLoco::Ui
{
    using WidgetIndex_t = int8_t;

    struct Window;
    enum class WindowColour : uint8_t;

    enum class WidgetType : uint8_t
    {
        none = 0,
        panel = 1,
        frame = 2,
        wt_3,
        wt_4,
        slider,
        wt_6,
        toolbarTab = 7,
        tab = 8,
        buttonWithImage = 9,
        buttonWithColour = 10,
        button = 11,
        wt_12,
        wt_13,
        buttonTableHeader = 14,
        wt_15,
        groupbox = 16,
        textbox = 17,
        combobox = 18,
        viewport = 19,
        wt_20,
        wt_21,
        caption_22,
        caption_23,
        caption_24,
        caption_25,
        scrollview = 26,
        checkbox = 27,
        wt_28,
        wt_29,
        viewportCentreButton, // TODO: Make a better generic button so we get the same result.
        end,
    };

    struct WidgetState
    {
        Window* window;
        Gfx::RectInsetFlags flags;
        AdvancedColour colour;
        bool enabled;
        bool disabled;
        bool activated;
        bool hovered;
        int scrollviewIndex;
    };

#pragma pack(push, 1)
    struct Widget
    {
        // Indicates that the imageId has a colour set and not to replace it with the window colour
        // This reuses the ImageIdFlags::translucent flag for use in widget draw
        // Flag *MUST* be removed before passing to drawingCtx.drawImage functions
        static constexpr uint32_t kImageIdColourSet = (1U << 30);
        static constexpr uint32_t kContentNull = 0xFFFFFFFFU;
        static constexpr uint32_t kContentUnk = 0xFFFFFFFEU;

        WidgetType type;           // 0x00
        WindowColour windowColour; // 0x01
        int16_t left;              // 0x02
        int16_t right;             // 0x04
        int16_t top;               // 0x06
        int16_t bottom;            // 0x08
        union
        {
            uint32_t image{};
            StringId text;
            uint32_t content;
        };
        StringId tooltip; // 0x0E
        FormatArgumentsBuffer textArgs;

        int16_t midX() const;
        int16_t midY() const;
        uint16_t width() const;
        uint16_t height() const;

        // TODO: Make tabs actual widgets.
        static void drawTab(Window* w, Gfx::RenderTarget* ctx, uint32_t imageId, WidgetIndex_t index);

        // typical tab width, to be used in most (all?) cases
        static constexpr uint16_t kDefaultTabWidth = 30;
        static void leftAlignTabs(Window& window, uint8_t firstTabIndex, uint8_t lastTabIndex, uint16_t tabWidth = kDefaultTabWidth);

        void draw(Gfx::RenderTarget* rt, Window* window, const uint64_t pressedWidgets, const uint64_t toolWidgets, const uint64_t hoveredWidgets, uint8_t& scrollviewIndex);
    };
#pragma pack(pop)
    //static_assert(sizeof(Widget) == 0x10);

    static constexpr Widget makeWidget(Ui::Point origin, Ui::Size size, WidgetType type, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        Widget out{};
        out.left = origin.x;
        out.right = origin.x + size.width - 1;
        out.top = origin.y;
        out.bottom = origin.y + size.height - 1;
        out.type = type;
        out.windowColour = colour;
        out.content = content;
        out.tooltip = tooltip;

        return out;
    }

    constexpr Widget makeRemapWidget(Ui::Point origin, Ui::Size size, WidgetType type, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        Widget out = makeWidget(origin, size, type, colour, content, tooltip);

        // TODO: implement this as a constant.
        out.content |= (1U << 29);

        return out;
    }

#define makeDropdownWidgets(...) \
    makeWidget(__VA_ARGS__),     \
        makeDropdownButtonWidget(__VA_ARGS__)

    [[maybe_unused]] static constexpr Widget makeDropdownButtonWidget(Ui::Point origin, Ui::Size size, [[maybe_unused]] WidgetType type, WindowColour colour, [[maybe_unused]] uint32_t content = Widget::kContentNull, [[maybe_unused]] StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 12;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 11;
        const uint16_t height = 10;

        return makeWidget({ xPos, yPos }, { width, height }, WidgetType::button, colour, StringIds::dropdown, tooltip);
    }

#define makeStepperWidgets(...)                 \
    makeWidget(__VA_ARGS__),                    \
        makeStepperDecreaseWidget(__VA_ARGS__), \
        makeStepperIncreaseWidget(__VA_ARGS__)

    [[maybe_unused]] static constexpr Widget makeStepperDecreaseWidget(Ui::Point origin, Ui::Size size, [[maybe_unused]] WidgetType type, WindowColour colour, [[maybe_unused]] uint32_t content = Widget::kContentNull, [[maybe_unused]] StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 26;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 13;
        const uint16_t height = size.height - 2;

        return makeWidget({ xPos, yPos }, { width, height }, WidgetType::button, colour, StringIds::stepper_minus, tooltip);
    }

    [[maybe_unused]] static constexpr Widget makeStepperIncreaseWidget(Ui::Point origin, Ui::Size size, [[maybe_unused]] WidgetType type, WindowColour colour, [[maybe_unused]] uint32_t content = Widget::kContentNull, [[maybe_unused]] StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 13;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 12;
        const uint16_t height = size.height - 2;

        return makeWidget({ xPos, yPos }, { width, height }, WidgetType::button, colour, StringIds::stepper_plus, tooltip);
    }

    constexpr Widget makeTextWidget(Ui::Point origin, Ui::Size size, WidgetType type, WindowColour colour, StringId content, StringId tooltip = StringIds::null)
    {
        Widget out{};
        out.left = origin.x;
        out.right = origin.x + size.width - 1;
        out.top = origin.y;
        out.bottom = origin.y + size.height - 1;
        out.type = type;
        out.windowColour = colour;
        out.text = content;
        out.tooltip = tooltip;

        return out;
    }

    constexpr Widget widgetEnd()
    {
        Widget out{};
        out.type = WidgetType::end;

        return out;
    }

}
