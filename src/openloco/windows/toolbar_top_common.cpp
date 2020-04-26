#include "toolbar_top_common.h"
#include "../audio/audio.h"
#include "../companymgr.h"
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

    // 0x0043A78E
    void zoom_menu_mouse_down(window* window, widget_index widgetIndex)
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
        dropdown::show_below(window, widgetIndex, 3, 25);
        dropdown::set_highlighted_item(0);

        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow->viewports[0]->zoom == 0)
        {
            dropdown::set_item_disabled(0);
            dropdown::set_highlighted_item(1);
        }

        if (mainWindow->viewports[0]->zoom == 3)
        {
            dropdown::set_item_disabled(1);
            zoom_ticks = 1000;
        }

        if (mainWindow->viewports[0]->zoom != 3 && zoom_ticks <= 32)
            dropdown::set_highlighted_item(1);
    }

    // 0x0043A5C5
    void rotate_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();

        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_rotate_clockwise, string_ids::menu_rotate_clockwise });
        dropdown::add(1, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_rotate_anti_clockwise, string_ids::menu_rotate_anti_clockwise });
        dropdown::show_below(window, widgetIndex, 2, 25);
        dropdown::set_highlighted_item(0);
    }

    // 0x0043ADF6
    void view_menu_mouse_down(window* window, widget_index widgetIndex)
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
        dropdown::show_below(window, widgetIndex, 10);

        uint32_t current_viewport_flags = WindowManager::getMainWindow()->viewports[0]->flags;

        if (current_viewport_flags & viewport_flags::underground_view)
            dropdown::set_item_selected(0);

        if (current_viewport_flags & viewport_flags::hide_foreground_tracks_roads)
            dropdown::set_item_selected(1);

        if (current_viewport_flags & viewport_flags::hide_foreground_scenery_buildings)
            dropdown::set_item_selected(2);

        if (current_viewport_flags & viewport_flags::height_marks_on_tracks_roads)
            dropdown::set_item_selected(4);

        if (current_viewport_flags & viewport_flags::height_marks_on_land)
            dropdown::set_item_selected(5);

        if (current_viewport_flags & viewport_flags::one_way_direction_arrows)
            dropdown::set_item_selected(6);

        if (!(current_viewport_flags & viewport_flags::town_names_displayed))
            dropdown::set_item_selected(8);

        if (!(current_viewport_flags & viewport_flags::station_names_displayed))
            dropdown::set_item_selected(9);

        dropdown::set_highlighted_item(0);
    }

    // 0x0043A3C3
    void terraform_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        auto land = objectmgr::get<land_object>(addr<0x00525FB6, uint8_t>());
        auto water = objectmgr::get<water_object>();

        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_bulldozer, string_ids::menu_clear_area });
        dropdown::add(1, string_ids::menu_sprite_stringid, { land->var_16 + land::image_ids::toolbar_terraform_land, string_ids::menu_adjust_land });
        dropdown::add(2, string_ids::menu_sprite_stringid, { water->var_06 + water::image_ids::toolbar_terraform_water, string_ids::menu_adjust_water });
        dropdown::add(3, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_plant_trees, string_ids::menu_plant_trees });
        dropdown::add(4, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_build_walls, string_ids::menu_build_walls });
        dropdown::show_below(window, widgetIndex, 5, 25);
        dropdown::set_highlighted_item(0);
    }

    // 0x0043A19F
    void road_menu_mouse_down(window* window, widget_index widgetIndex)
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

        auto company_colour = companymgr::get_player_company_colour();

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

        dropdown::show_below(window, widgetIndex, i, 25);
        dropdown::set_highlighted_item(highlighted_item);
    }

    // 0x0043A8CE
    void towns_menu_mouse_down(window* window, widget_index widgetIndex)
    {
        auto interface = objectmgr::get<interface_skin_object>();
        dropdown::add(0, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_towns, string_ids::menu_towns });
        dropdown::add(1, string_ids::menu_sprite_stringid, { interface->img + interface_skin::image_ids::toolbar_menu_industries, string_ids::menu_industries });
        dropdown::show_below(window, widgetIndex, 2, 25);
        dropdown::set_highlighted_item(last_town_option);
    }

    // 0x0043A86D
    void zoom_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        window = WindowManager::getMainWindow();

        if (itemIndex == 0)
        {
            window->viewport_zoom_in(false);
            townmgr::update_labels();
            stationmgr::update_labels();
        }
        else if (itemIndex == 1)
        {
            zoom_ticks = 0;
            window->viewport_zoom_out(false);
            townmgr::update_labels();
            stationmgr::update_labels();
        }
        else if (itemIndex == 2)
        {
            windows::map::open();
        }
    }

    // 0x0043A624
    void rotate_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        auto mouseButtonUsed = input::getLastKnownButtonState();
        window = WindowManager::getMainWindow();

        if (itemIndex == 1 || mouseButtonUsed == input::mouse_button::right_pressed)
        {
            window->viewport_rotate_left();
            townmgr::update_labels();
            stationmgr::update_labels();
            windows::map::center_on_view_point();
        }
        else if (itemIndex == 0)
        {
            window->viewport_rotate_right();
            townmgr::update_labels();
            stationmgr::update_labels();
            windows::map::center_on_view_point();
        }
    }

    // 0x0043AF37
    void view_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

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
    void terraform_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        switch (itemIndex)
        {
            case 0:
                windows::terraform::open_clear_area();
                break;

            case 1:
                windows::terraform::open_adjust_land();
                break;

            case 2:
                windows::terraform::open_adjust_water();
                break;

            case 3:
                windows::terraform::open_plant_trees();
                break;

            case 4:
                windows::terraform::open_build_walls();
                break;
        }
    }

    // 0x0043A28C
    void road_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

        if (itemIndex == -1)
            return;

        uint8_t objIndex = available_objects[itemIndex];
        construction::open_with_flags(objIndex);
    }

    // 0x0043A932
    void towns_menu_dropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::get_highlighted_item();

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

    void on_update(window* window)
    {
        zoom_ticks++;
    }
}
