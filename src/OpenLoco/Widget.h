#pragma once

#include "Graphics/Gfx.h"
#include "Localisation/StringManager.h"
#include "Window.h"
#include <cstdint>

namespace OpenLoco::Ui::Widget
{
    void sub_4CADE8(Gfx::Context* context, const window* window, const widget_t* widget, uint8_t colour, bool enabled, bool disabled, bool activated);
    void drawViewportCentreButton(Gfx::Context* context, const window* window, const widget_index widgetIndex);

    void drawPanel(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void drawFrame(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_3(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_5(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_9(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered);
    void draw_10(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered);

    void draw_11_a(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);
    void draw_13(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_15(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled);

    void draw_17(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour);

    void draw_22_caption(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_23_caption(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_24_caption(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_25_caption(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void drawScrollview(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index);
    void draw_27_checkbox(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);
    void draw_27_label(Gfx::Context* context, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled);

    void draw_29(Gfx::Context* context, window* window, widget_t* widget);

    void drawGroupbox(Gfx::Context* const context, window* const window, widget_t* widget);

    void draw_tab(window* w, Gfx::Context* ctx, int32_t imageId, widget_index index);
}
