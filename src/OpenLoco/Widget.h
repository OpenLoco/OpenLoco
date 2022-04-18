#pragma once

#include "Graphics/Gfx.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "Window.h"
#include <cstdint>

namespace OpenLoco::Ui
{
#pragma pack(push, 1)
    struct Widget
    {
        // Indicates that the imageId has a colour set and not to replace it with the window colour
        // This reuses the ImageIdFlags::translucent flag for use in widget draw
        // Flag *MUST* be removed before passing to Gfx::drawImage functions
        static constexpr uint32_t imageIdColourSet = (1 << 30);

        WidgetType type;           // 0x00
        WindowColour windowColour; // 0x01
        int16_t left;              // 0x02
        int16_t right;             // 0x04
        int16_t top;               // 0x06
        int16_t bottom;            // 0x08
        union
        {
            uint32_t image;
            string_id text;
            int32_t content;
        };
        string_id tooltip; // 0x0E

        int16_t mid_x() const;
        int16_t mid_y() const;
        uint16_t width() const;
        uint16_t height() const;

        static void drawViewportCentreButton(Gfx::Context* context, const Window* window, const WidgetIndex_t widgetIndex);
        static void drawTab(Window* w, Gfx::Context* ctx, int32_t imageId, WidgetIndex_t index);

        void draw(Gfx::Context* context, Window* window, const uint64_t pressedWidgets, const uint64_t toolWidgets, const uint64_t hoveredWidgets, uint8_t& scrollviewIndex);

    private:
        void sub_4CADE8(Gfx::Context* context, const Window* window, AdvancedColour colour, bool enabled, bool disabled, bool activated);

        void drawPanel(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour);
        void drawFrame(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour);

        void draw_3(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated);
        void drawTab(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated);
        void drawButtonWithImage(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered);
        void drawButtonWithColour(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered);
        void drawButton(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated);
        void draw_13(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated);
        void draw_15(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool disabled);
        void drawTextBox(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour);
        void draw_22_caption(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour);
        void draw_23_caption(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour);
        void draw_24_caption(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour);
        void draw_25_caption(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour);
        void drawScrollview(Gfx::Context* context, Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index);
        void draw_27_checkbox(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool enabled, bool disabled, bool activated);
        void draw_27_label(Gfx::Context* context, const Window* window, uint16_t flags, AdvancedColour colour, bool disabled);
        void draw_29(Gfx::Context* context, const Window* window);

        void drawGroupbox(Gfx::Context* const context, const Window* window);
    };
#pragma pack(pop)
    static_assert(sizeof(Widget) == 0x10);

    static constexpr Widget makeWidget(Ui::Point origin, Ui::Size size, WidgetType type, WindowColour colour, uint32_t content = 0xFFFFFFFF, string_id tooltip = StringIds::null)
    {
        Widget out = {};
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

    constexpr Widget makeRemapWidget(Ui::Point origin, Ui::Size size, WidgetType type, WindowColour colour, uint32_t content = 0xFFFFFFFF, string_id tooltip = StringIds::null)
    {
        Widget out = makeWidget(origin, size, type, colour, content, tooltip);

        // TODO: implement this as a constant.
        out.content |= (1 << 29);

        return out;
    }

#define makeDropdownWidgets(...) \
    makeWidget(__VA_ARGS__),     \
        makeDropdownButtonWidget(__VA_ARGS__)

    [[maybe_unused]] static constexpr Widget makeDropdownButtonWidget(Ui::Point origin, Ui::Size size, [[maybe_unused]] WidgetType type, WindowColour colour, [[maybe_unused]] uint32_t content = 0xFFFFFFFF, [[maybe_unused]] string_id tooltip = StringIds::null)
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

    [[maybe_unused]] static constexpr Widget makeStepperDecreaseWidget(Ui::Point origin, Ui::Size size, [[maybe_unused]] WidgetType type, WindowColour colour, [[maybe_unused]] uint32_t content = 0xFFFFFFFF, [[maybe_unused]] string_id tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 26;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 13;
        const uint16_t height = size.height - 2;

        return makeWidget({ xPos, yPos }, { width, height }, WidgetType::button, colour, StringIds::stepper_minus, tooltip);
    }

    [[maybe_unused]] static constexpr Widget makeStepperIncreaseWidget(Ui::Point origin, Ui::Size size, [[maybe_unused]] WidgetType type, WindowColour colour, [[maybe_unused]] uint32_t content = 0xFFFFFFFF, [[maybe_unused]] string_id tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 13;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 12;
        const uint16_t height = size.height - 2;

        return makeWidget({ xPos, yPos }, { width, height }, WidgetType::button, colour, StringIds::stepper_plus, tooltip);
    }

    constexpr Widget makeTextWidget(Ui::Point origin, Ui::Size size, WidgetType type, WindowColour colour, string_id content, string_id tooltip = StringIds::null)
    {
        Widget out = {};
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
        Widget out = {};
        out.type = WidgetType::end;

        return out;
    }

}
