#pragma once

#include "Graphics/Gfx.h"
#include "Localisation/StringManager.h"
#include "Window.h"
#include <cstdint>

namespace OpenLoco::Ui::Widget
{
    void sub_4CADE8(Gfx::drawpixelinfo_t* dpi, const window* window, const widget_t* widget, uint8_t colour, bool enabled, bool disabled, bool activated);
    void drawViewportCentreButton(Gfx::drawpixelinfo_t* dpi, const window* window, const widget_index widgetIndex);

    void drawPanel(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void drawFrame(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_3(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_5(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_9(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered);
    void draw_10(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered);

    void draw_11_a(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);
    void draw_13(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);

    void draw_15(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled);

    void draw_17(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);

    void draw_22_caption(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_23_caption(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_24_caption(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void draw_25_caption(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour);
    void drawScrollview(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated, bool hovered, int scrollview_index);
    void draw_27_checkbox(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool enabled, bool disabled, bool activated);
    void draw_27_label(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget, uint16_t flags, uint8_t colour, bool disabled);

    void draw_29(Gfx::drawpixelinfo_t* dpi, window* window, widget_t* widget);

    void drawGroupbox(Gfx::drawpixelinfo_t* const context, window* const window, widget_t* widget);

    void draw_tab(window* w, Gfx::drawpixelinfo_t* ctx, int32_t imageId, widget_index index);
}
