#pragma once

#include "Window.h"
#include "graphics/gfx.h"
#include "localisation/stringmgr.h"
#include <cstdint>

namespace openloco::ui::widget
{
    void sub_4CADE8(gfx::GraphicsContext* context, const Window* window, const widget_t* widget, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_1(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_2(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_3(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_5(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_9(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered);
    void draw_10(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered);

    void draw_11_a(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);
    void draw_13(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_15(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled);

    void draw_17(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour);

    void draw_22_caption(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_23_caption(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_24_caption(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_25_caption(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_26(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index);
    void draw_27_checkbox(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);
    void draw_27_label(gfx::GraphicsContext* context, Window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled);

    void draw_29(gfx::GraphicsContext* context, Window* window, widget_t* widget);
}
