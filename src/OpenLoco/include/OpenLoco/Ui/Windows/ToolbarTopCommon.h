#pragma once

#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::ToolbarTop::Common
{
    enum widx
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

    namespace Widx
    {
        constexpr WidgetId kLoadsaveMenu{ "loadsave_menu" };
        constexpr WidgetId kAudioMenu{ "audio_menu" };
        constexpr WidgetId kZoomMenu{ "zoom_menu" };
        constexpr WidgetId kRotateMenu{ "rotate_menu" };
        constexpr WidgetId kViewMenu{ "view_menu" };
        constexpr WidgetId kTerraformMenu{ "terraform_menu" };
        constexpr WidgetId kRailroadMenu{ "railroad_menu" };
        constexpr WidgetId kRoadMenu{ "road_menu" };
        constexpr WidgetId kPortMenu{ "port_menu" };
        constexpr WidgetId kBuildVehiclesMenu{ "build_vehicles_menu" };
        constexpr WidgetId kVehiclesMenu{ "vehicles_menu" };
        constexpr WidgetId kStationsMenu{ "stations_menu" };
        constexpr WidgetId kTownsMenu{ "towns_menu" };
    }

    void prepareTownWidget(Window& self);

    void draw(Window& window, Gfx::DrawingContext& drawingCtx);

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

    void onOpen(Window& window);
    void onUpdate(Window& window);
    void onResize(Window& window);
    void onMouseDown(Window* window, WidgetIndex_t widgetIndex);
    void onDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);

    void rightAlignTabs(Window* window, uint32_t& x, const std::initializer_list<uint32_t> widxs);
}
