#pragma once

#include "graphics/gfx.h"
#include "localisation/stringmgr.h"
#include "window.h"
#include <cstdint>

namespace openloco::ui::widget
{
    void sub_4CADE8(gfx::drawpixelinfo_t* dpi, const window* window, const widget_t* widget, uint8_t colour, bool enabled, bool disabled, bool activated);
    void drawViewportCentreButton(gfx::drawpixelinfo_t* dpi, const window* window, const widget_index widgetIndex);

    void draw_1(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_2(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_3(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_5(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_9(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered);
    void draw_10(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered);

    void draw_11_a(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);
    void draw_13(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_15(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled);

    void draw_17(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);

    void draw_22_caption(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_23_caption(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_24_caption(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_25_caption(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_26(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index);
    void draw_27_checkbox(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);
    void draw_27_label(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled);

    void draw_29(gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget);

    void draw_tab(window* w, gfx::drawpixelinfo_t* ctx, int32_t imageId, widget_index index);
}
