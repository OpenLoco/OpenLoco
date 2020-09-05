#include "ToolbarTopCommon.h"
#include "../audio/audio.h"
#include "../CompanyManager.h"
#include "../config.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/land_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/track_object.h"
#include "../objects/water_object.h"
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include <map>

using namespace openloco::interop;

namespace openloco::ui::windows::toolbar_top::common
{
    static loco_global<uint8_t, 0x00525FAB> last_road_option;

    static loco_global<uint32_t, 0x009C86F8> zoom_ticks;

    static loco_global<uint8_t, 0x009C870C> last_town_option;

    static loco_global<int8_t[18], 0x0050A006> available_objects;

    // 0x00439DE4
    void draw(window* self, gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        self->draw(dpi);

        uint32_t company_colour = companymgr::getPlayerCompanyColour();

        if (self->widgets[widx::road_menu].type != widget_type::none && last_road_option != 0xFF)
        {
            uint32_t x = self->widgets[widx::road_menu].left + self->x;
            uint32_t y = self->widgets[widx::road_menu].top + self->y;
            uint32_t fgImage = 0;

            // Figure out what icon to show on the button face.
            bool isRoad = last_road_option & (1 << 7);
            if (isRoad)
            {
                auto obj = objectmgr::get<road_object>(last_road_option & ~(1 << 7));
                fgImage = gfx::recolour(obj->var_0E, company_colour);
            }
            else
            {
                auto obj = objectmgr::get<track_object>(last_road_option);
                fgImage = gfx::recolour(obj->var_1E, company_colour);
            }

            y--;
            auto interface = objectmgr::get<interface_skin_object>();
            uint32_t bgImage = gfx::recolour(interface->img + interface_skin::image_ids::toolbar_empty_transparent, self->colours[2]);

            if (input::isDropdownActive(ui::WindowType::topToolbar, widx::road_menu))
            {
                y++;
                bgImage++;
            }

            gfx::drawImage(dpi, x, y, fgImage);

            y = self->widgets[widx::road_menu].top + self->y;
            gfx::drawImage(dpi, x, y, bgImage);
        }
    }

    // 0x0043A78E
    void zoomMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();

        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_zoom_in, string_ids::menu_zoom_in });
        dropdown::add(1, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_zoom_out, string_ids::menu_zoom_out });

        static const uint32_t map_sprites_by_rotation[] = {
            interface_skin::image_ids::toolbar_menu_map_north,
            interface_skin::image_ids::toolbar_menu_map_west,
            interface_skin::image_ids::toolbar_menu_map_south,
            interface_skin::image_ids::toolbar_menu_map_east,
        };

        loco_global<int32_t, 0x00e3f0b8> current_rotation;
        uint32_t map_sprite = map_sprites_by_rotation[current_rotation];

        dropdown::add(2, string_ids::menu_sprite_stringid, { interface->img + map_sprite, string_ids::menu_map });
        dropdown::showBelow(window, widgetIndex, 3, 25, (1 << 6));
        dropdown::setHighlightedItem(0);

        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow->viewports[0]->zoom == 0)
        {
            dropdown::setItemDisabled(0);
            dropdown::setHighlightedItem(1);
        }

        if (mainWindow->viewports[0]->zoom == 3)
        {
            dropdown::setItemDisabled(1);
            zoom_ticks = 1000;
        }

        if (mainWindow->viewports[0]->zoom != 3 && zoom_ticks <= 32)
            dropdown::setHighlightedItem(1);
    }

    // 0x0043A5C5
    void rotateMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();

        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_rotate_clockwise, string_ids::menu_rotate_clockwise });
        dropdown::add(1, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_rotate_anti_clockwise, string_ids::menu_rotate_anti_clockwise });
        dropdown::showBelow(window, widgetIndex, 2, 25, (1 << 6));
        dropdown::setHighlightedItem(0);
    }

    // 0x0043ADF6
    void viewMenuMouseDown(window* window, widget_index widgetIndex)
    {
        dropdown::add(0, string_ids::dropdown_without_checkmark, string_ids::menu_underground_view);
        dropdown::add(1, string_ids::dropdown_without_checkmark, string_ids::menu_hide_foreground_tracks_roads);
        dropdown::add(2, string_ids::dropdown_without_checkmark, string_ids::menu_hide_foreground_scenery_buildings);
        dropdown::add(3, 0);
        dropdown::add(4, string_ids::dropdown_without_checkmark, string_ids::menu_height_marks_on_land);
        dropdown::add(5, string_ids::dropdown_without_checkmark, string_ids::menu_height_marks_on_tracks_roads);
        dropdown::add(6, string_ids::dropdown_without_checkmark, string_ids::menu_one_way_direction_arrows);
        dropdown::add(7, 0);
        dropdown::add(8, string_ids::dropdown_without_checkmark, string_ids::menu_town_names_displayed);
        dropdown::add(9, string_ids::dropdown_without_checkmark, string_ids::menu_station_names_displayed);
        dropdown::showBelow(window, widgetIndex, 10, 0);

        uint32_t current_viewport_flags = WindowManager::getMainWindow()->viewports[0]->flags;

        if (current_viewport_flags & viewport_flags::underground_view)
            dropdown::setItemSelected(0);

        if (current_viewport_flags & viewport_flags::hide_foreground_tracks_roads)
            dropdown::setItemSelected(1);

        if (current_viewport_flags & viewport_flags::hide_foreground_scenery_buildings)
            dropdown::setItemSelected(2);

        if (current_viewport_flags & viewport_flags::height_marks_on_tracks_roads)
            dropdown::setItemSelected(4);

        if (current_viewport_flags & viewport_flags::height_marks_on_land)
            dropdown::setItemSelected(5);

        if (current_viewport_flags & viewport_flags::one_way_direction_arrows)
            dropdown::setItemSelected(6);

        if (!(current_viewport_flags & viewport_flags::town_names_displayed))
            dropdown::setItemSelected(8);

        if (!(current_viewport_flags & viewport_flags::station_names_displayed))
            dropdown::setItemSelected(9);

        dropdown::setHighlightedItem(0);
    }

    // 0x0043A3C3
    void terraformMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        auto land = objectmgr::get<land_object>(addr<0x00525FB6, uint8_t>());
        auto water = objectmgr::get<water_object>();

        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_bulldozer, string_ids::menu_clear_area });
        dropdown::add(1, string_ids::menu_sprite_stringid, { land->var_16 + land::image_ids::toolbar_terraform_land, string_ids::menu_adjust_land });
        dropdown::add(2, string_ids::menu_sprite_stringid, { water->var_06 + water::image_ids::toolbar_terraform_water, string_ids::menu_adjust_water });
        dropdown::add(3, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_plant_trees, string_ids::menu_plant_trees });
        dropdown::add(4, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_build_walls, string_ids::menu_build_walls });
        dropdown::showBelow(window, widgetIndex, 5, 25, (1 << 6));
        dropdown::setHighlightedItem(0);
    }

    // 0x0043A19F
    void roadMenuMouseDown(window* window, widget_index widgetIndex)
    {
        // Load objects.
        registers regs;
        regs.edi = (uint32_t)&available_objects[0];
        call(0x00478265, regs);

        // Sanity check: any objects available?
        uint32_t i = 0;
        while (available_objects[i] != -1 && i < std::size(available_objects))
            i++;
        if (i == 0)
            return;

        auto company_colour = companymgr::getPlayerCompanyColour();

        // Add available objects to dropdown.
        uint16_t highlighted_item = 0;
        for (i = 0; available_objects[i] != -1 && i < std::size(available_objects); i++)
        {
            uint32_t obj_image;
            string_id obj_string_id;

            int objIndex = available_objects[i];
            if ((objIndex & (1 << 7)) != 0)
            {
                auto road = objectmgr::get<road_object>(objIndex & 0x7F);
                obj_string_id = road->name;
                obj_image = gfx::recolour(road->var_0E, company_colour);
            }
            else
            {
                auto track = objectmgr::get<track_object>(objIndex);
                obj_string_id = track->name;
                obj_image = gfx::recolour(track->var_1E, company_colour);
            }

            dropdown::add(i, string_ids::menu_sprite_stringid_construction, { obj_image, obj_string_id });

            if (objIndex == last_road_option)
                highlighted_item = i;
        }

        dropdown::showBelow(window, widgetIndex, i, 25, (1 << 6));
        dropdown::setHighlightedItem(highlighted_item);
    }

    // 0x0043A8CE
    void townsMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_towns, string_ids::menu_towns });
        dropdown::add(1, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_industries, string_ids::menu_industries });
        dropdown::showBelow(window, widgetIndex, 2, 25, (1 << 6));
        dropdown::setHighlightedItem(last_town_option);
    }

    // 0x0043A86D
    void zoomMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        window = WindowManager::getMainWindow();

        if (itemIndex == 0)
        {
            window->viewportZoomIn(false);
            townmgr::updateLabels();
            stationmgr::updateLabels();
        }
        else if (itemIndex == 1)
        {
            zoom_ticks = 0;
            window->viewportZoomOut(false);
            townmgr::updateLabels();
            stationmgr::updateLabels();
        }
        else if (itemIndex == 2)
        {
            windows::map::open();
        }
    }

    // 0x0043A624
    void rotateMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        auto mouseButtonUsed = input::getLastKnownButtonState();
        window = WindowManager::getMainWindow();

        if (itemIndex == 1 || mouseButtonUsed == input::mouse_button::right_pressed)
        {
            window->viewportRotateLeft();
            townmgr::updateLabels();
            stationmgr::updateLabels();
            windows::map::centerOnViewPoint();
        }
        else if (itemIndex == 0)
        {
            window->viewportRotateRight();
            townmgr::updateLabels();
            stationmgr::updateLabels();
            windows::map::centerOnViewPoint();
        }
    }

    // 0x0043AF37
    void viewMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        window = WindowManager::getMainWindow();
        auto viewport = WindowManager::getMainWindow()->viewports[0];

        if (itemIndex == 0)
            viewport->flags ^= viewport_flags::underground_view;
        else if (itemIndex == 1)
            viewport->flags ^= viewport_flags::hide_foreground_tracks_roads;
        else if (itemIndex == 2)
            viewport->flags ^= viewport_flags::hide_foreground_scenery_buildings;
        else if (itemIndex == 4)
            viewport->flags ^= viewport_flags::height_marks_on_tracks_roads;
        else if (itemIndex == 5)
            viewport->flags ^= viewport_flags::height_marks_on_land;
        else if (itemIndex == 6)
            viewport->flags ^= viewport_flags::one_way_direction_arrows;
        else if (itemIndex == 8)
            viewport->flags ^= viewport_flags::town_names_displayed;
        else if (itemIndex == 9)
            viewport->flags ^= viewport_flags::station_names_displayed;

        window->invalidate();
    }

    // 0x0043A4A8
    void terraformMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                windows::terraform::openClearArea();
                break;

            case 1:
                windows::terraform::openAdjustLand();
                break;

            case 2:
                windows::terraform::openAdjustWater();
                break;

            case 3:
                windows::terraform::openPlantTrees();
                break;

            case 4:
                windows::terraform::openBuildWalls();
                break;
        }
    }

    // 0x0043A28C
    void roadMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        uint8_t objIndex = available_objects[itemIndex];
        construction::openWithFlags(objIndex);
    }

    // 0x0043A932
    void townsMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        if (itemIndex == 0)
        {
            windows::town_list::open();
            last_town_option = 0;
        }
        else if (itemIndex == 1)
        {
            windows::industry_list::open();
            last_town_option = 1;
        }
    }

    void onDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case widx::zoom_menu:
                zoomMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case widx::rotate_menu:
                rotateMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case widx::view_menu:
                viewMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case widx::terraform_menu:
                terraformMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case widx::road_menu:
                roadMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case widx::towns_menu:
                townsMenuDropdown(window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x0043A071
    void onMouseDown(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::zoom_menu:
                zoomMenuMouseDown(window, widgetIndex);
                break;

            case widx::rotate_menu:
                rotateMenuMouseDown(window, widgetIndex);
                break;

            case widx::view_menu:
                viewMenuMouseDown(window, widgetIndex);
                break;

            case widx::terraform_menu:
                terraformMenuMouseDown(window, widgetIndex);
                break;

            case widx::road_menu:
                roadMenuMouseDown(window, widgetIndex);
                break;

            case widx::towns_menu:
                townsMenuMouseDown(window, widgetIndex);
                break;
        }
    }

    void onUpdate(window* window)
    {
        zoom_ticks++;
    }

    // 0x0043A17E
    void onResize(window* window)
    {
        auto main = WindowManager::getMainWindow();
        if (main == nullptr)
            window->setDisabledWidgetsAndInvalidate(widx::zoom_menu | widx::rotate_menu);
        else
            window->setDisabledWidgetsAndInvalidate(0);
    }

    void rightAlignTabs(window* window, uint32_t& x, const std::initializer_list<uint32_t> widxs)
    {
        for (const auto& widx : widxs)
        {
            window->widgets[widx].right = x;
            window->widgets[widx].left = x - 29;
            x -= 30;
        }
    }
}
