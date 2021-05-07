#pragma once

#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarTop::Common
{
    namespace Widx
    {
        enum
        {
            loadsave_menu,
            audio_menu,
            w2,

            zoom_menu,
            rotate_menu,
            view_menu,

            terraform_menu,
            railroad_menu,
            road_menu,
            port_menu,
            build_vehicles_menu,

            vehicles_menu,
            stations_menu,
            towns_menu,
        };
    }

    void draw(window* window, Gfx::Context* context);

    void zoomMenuMouseDown(window* window, widget_index widgetIndex);
    void rotateMenuMouseDown(window* window, widget_index widgetIndex);
    void viewMenuMouseDown(window* window, widget_index widgetIndex);
    void terraformMenuMouseDown(window* window, widget_index widgetIndex);
    void roadMenuMouseDown(window* window, widget_index widgetIndex);
    void townsMenuMouseDown(window* window, widget_index widgetIndex);

    void zoomMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void rotateMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void viewMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void terraformMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void roadMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);
    void townsMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);

    void onUpdate(window* window);
    void onResize(window* window);
    void onMouseDown(window* window, widget_index widgetIndex);
    void onDropdown(window* window, widget_index widgetIndex, int16_t itemIndex);

    void rightAlignTabs(window* window, uint32_t& x, const std::initializer_list<uint32_t> widxs);
}
