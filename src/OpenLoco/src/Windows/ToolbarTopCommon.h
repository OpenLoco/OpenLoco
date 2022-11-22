#pragma once

#include "Ui/WindowManager.h"

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

    void draw(Window& window, Gfx::RenderTarget* rt);

    void zoomMenuMouseDown(Window* window, WidgetIndex_t widgetIndex);
    void rotateMenuMouseDown(Window* window, WidgetIndex_t widgetIndex);
    void viewMenuMouseDown(Window* window, WidgetIndex_t widgetIndex);
    void terraformMenuMouseDown(Window* window, WidgetIndex_t widgetIndex);
    void roadMenuMouseDown(Window* window, WidgetIndex_t widgetIndex);
    void townsMenuMouseDown(Window* window, WidgetIndex_t widgetIndex);

    void zoomMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    void rotateMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    void viewMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    void terraformMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    void roadMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    void townsMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);

    void onUpdate(Window& window);
    void onResize(Window& window);
    void onMouseDown(Window* window, WidgetIndex_t widgetIndex);
    void onDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);

    void rightAlignTabs(Window* window, uint32_t& x, const std::initializer_list<uint32_t> widxs);
}
