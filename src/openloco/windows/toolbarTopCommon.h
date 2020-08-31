#pragma once

#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::toolbar_top::common
{
    namespace widx
    {
        enum
        {
            loadsave_menu,
            audio_menu,
            zoom_menu,
            rotate_menu,
            view_menu,

            terraform_menu,
            w6,
            road_menu,
            port_menu,
            build_vehicles_menu,

            vehicles_menu,
            stations_menu,
            towns_menu,
        };
    }

    void draw(window* window, gfx::drawpixelinfo_t* dpi);

    void zoom_menu_mouse_down(window* window, widget_index widgetIndex);
    void rotate_menu_mouse_down(window* window, widget_index widgetIndex);
    void view_menu_mouse_down(window* window, widget_index widgetIndex);
    void terraform_menu_mouse_down(window* window, widget_index widgetIndex);
    void road_menu_mouse_down(window* window, widget_index widgetIndex);
    void towns_menu_mouse_down(window* window, widget_index widgetIndex);

    void zoom_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void rotate_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void view_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void terraform_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void road_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void towns_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);

    void on_update(window* window);
    void on_resize(window* window);
    void on_mouse_down(window* window, widget_index widgetIndex);
    void on_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex);

    void rightAlignTabs(window* window, uint32_t& x, const std::initializer_list<uint32_t> widxs);
}
