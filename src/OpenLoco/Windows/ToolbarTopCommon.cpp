#include "ToolbarTopCommon.h"
#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/WaterObject.h"
#include "../Ptr.h"
#include "../StationManager.h"
#include "../Things/ThingManager.h"
#include "../TownManager.h"
#include "../Ui/Dropdown.h"
#include "../Vehicles/Vehicle.h"
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarTop::Common
{
    static loco_global<uint8_t, 0x00525FAB> last_road_option;

    static loco_global<uint32_t, 0x009C86F8> zoom_ticks;

    static loco_global<uint8_t, 0x009C870C> last_town_option;

    static loco_global<int8_t[18], 0x0050A006> available_objects;

    // 0x00439DE4
    void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        self->draw(dpi);

        uint32_t company_colour = CompanyManager::getPlayerCompanyColour();

        if (self->widgets[Widx::road_menu].type != widget_type::none && last_road_option != 0xFF)
        {
            uint32_t x = self->widgets[Widx::road_menu].left + self->x;
            uint32_t y = self->widgets[Widx::road_menu].top + self->y;
            uint32_t fgImage = 0;

            // Figure out what icon to show on the button face.
            bool isRoad = last_road_option & (1 << 7);
            if (isRoad)
            {
                auto obj = ObjectManager::get<road_object>(last_road_option & ~(1 << 7));
                fgImage = Gfx::recolour(obj->image, company_colour);
            }
            else
            {
                auto obj = ObjectManager::get<track_object>(last_road_option);
                fgImage = Gfx::recolour(obj->image, company_colour);
            }

            y--;
            auto interface = ObjectManager::get<interface_skin_object>();
            uint32_t bgImage = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, self->colours[2]);

            if (Input::isDropdownActive(Ui::WindowType::topToolbar, Widx::road_menu))
            {
                y++;
                bgImage++;
            }

            Gfx::drawImage(dpi, x, y, fgImage);

            y = self->widgets[Widx::road_menu].top + self->y;
            Gfx::drawImage(dpi, x, y, bgImage);
        }
    }

    // 0x0043A78E
    void zoomMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = ObjectManager::get<interface_skin_object>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_zoom_in, StringIds::menu_zoom_in });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_zoom_out, StringIds::menu_zoom_out });

        static const uint32_t map_sprites_by_rotation[] = {
            InterfaceSkin::ImageIds::toolbar_menu_map_north,
            InterfaceSkin::ImageIds::toolbar_menu_map_west,
            InterfaceSkin::ImageIds::toolbar_menu_map_south,
            InterfaceSkin::ImageIds::toolbar_menu_map_east,
        };

        loco_global<int32_t, 0x00e3f0b8> current_rotation;
        uint32_t map_sprite = map_sprites_by_rotation[current_rotation];

        Dropdown::add(2, StringIds::menu_sprite_stringid, { interface->img + map_sprite, StringIds::menu_map });
        Dropdown::showBelow(window, widgetIndex, 3, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);

        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow->viewports[0]->zoom == 0)
        {
            Dropdown::setItemDisabled(0);
            Dropdown::setHighlightedItem(1);
        }

        if (mainWindow->viewports[0]->zoom == 3)
        {
            Dropdown::setItemDisabled(1);
            zoom_ticks = 1000;
        }

        if (mainWindow->viewports[0]->zoom != 3 && zoom_ticks <= 32)
            Dropdown::setHighlightedItem(1);
    }

    // 0x0043A5C5
    void rotateMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = ObjectManager::get<interface_skin_object>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_rotate_clockwise, StringIds::menu_rotate_clockwise });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_rotate_anti_clockwise, StringIds::menu_rotate_anti_clockwise });
        Dropdown::showBelow(window, widgetIndex, 2, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);
    }

    // 0x0043ADF6
    void viewMenuMouseDown(window* window, widget_index widgetIndex)
    {
        Dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_underground_view);
        Dropdown::add(1, StringIds::dropdown_without_checkmark, StringIds::menu_hide_foreground_tracks_roads);
        Dropdown::add(2, StringIds::dropdown_without_checkmark, StringIds::menu_hide_foreground_scenery_buildings);
        Dropdown::add(3, 0);
        Dropdown::add(4, StringIds::dropdown_without_checkmark, StringIds::menu_height_marks_on_land);
        Dropdown::add(5, StringIds::dropdown_without_checkmark, StringIds::menu_height_marks_on_tracks_roads);
        Dropdown::add(6, StringIds::dropdown_without_checkmark, StringIds::menu_one_way_direction_arrows);
        Dropdown::add(7, 0);
        Dropdown::add(8, StringIds::dropdown_without_checkmark, StringIds::menu_town_names_displayed);
        Dropdown::add(9, StringIds::dropdown_without_checkmark, StringIds::menu_station_names_displayed);
        Dropdown::showBelow(window, widgetIndex, 10, 0);

        uint32_t current_viewport_flags = WindowManager::getMainWindow()->viewports[0]->flags;

        if (current_viewport_flags & ViewportFlags::underground_view)
            Dropdown::setItemSelected(0);

        if (current_viewport_flags & ViewportFlags::hide_foreground_tracks_roads)
            Dropdown::setItemSelected(1);

        if (current_viewport_flags & ViewportFlags::hide_foreground_scenery_buildings)
            Dropdown::setItemSelected(2);

        if (current_viewport_flags & ViewportFlags::height_marks_on_tracks_roads)
            Dropdown::setItemSelected(4);

        if (current_viewport_flags & ViewportFlags::height_marks_on_land)
            Dropdown::setItemSelected(5);

        if (current_viewport_flags & ViewportFlags::one_way_direction_arrows)
            Dropdown::setItemSelected(6);

        if (!(current_viewport_flags & ViewportFlags::town_names_displayed))
            Dropdown::setItemSelected(8);

        if (!(current_viewport_flags & ViewportFlags::station_names_displayed))
            Dropdown::setItemSelected(9);

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043A3C3
    void terraformMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = ObjectManager::get<interface_skin_object>();
        auto land = ObjectManager::get<land_object>(addr<0x00525FB6, uint8_t>());
        auto water = ObjectManager::get<water_object>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_bulldozer, StringIds::menu_clear_area });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { land->var_16 + Land::ImageIds::toolbar_terraform_land, StringIds::menu_adjust_land });
        Dropdown::add(2, StringIds::menu_sprite_stringid, { water->image + Water::ImageIds::toolbar_terraform_water, StringIds::menu_adjust_water });
        Dropdown::add(3, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_plant_trees, StringIds::menu_plant_trees });
        Dropdown::add(4, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_build_walls, StringIds::menu_build_walls });
        Dropdown::showBelow(window, widgetIndex, 5, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);
    }

    // 0x0043A19F
    void roadMenuMouseDown(window* window, widget_index widgetIndex)
    {
        // Load objects.
        registers regs;
        regs.edi = ToInt(&available_objects[0]);
        call(0x00478265, regs);

        // Sanity check: any objects available?
        uint32_t i = 0;
        while (available_objects[i] != -1 && i < std::size(available_objects))
            i++;
        if (i == 0)
            return;

        auto company_colour = CompanyManager::getPlayerCompanyColour();

        // Add available objects to Dropdown.
        uint16_t highlighted_item = 0;
        for (i = 0; available_objects[i] != -1 && i < std::size(available_objects); i++)
        {
            uint32_t obj_image;
            string_id obj_string_id;

            int objIndex = available_objects[i];
            if ((objIndex & (1 << 7)) != 0)
            {
                auto road = ObjectManager::get<road_object>(objIndex & 0x7F);
                obj_string_id = road->name;
                obj_image = Gfx::recolour(road->image, company_colour);
            }
            else
            {
                auto track = ObjectManager::get<track_object>(objIndex);
                obj_string_id = track->name;
                obj_image = Gfx::recolour(track->image, company_colour);
            }

            Dropdown::add(i, StringIds::menu_sprite_stringid_construction, { obj_image, obj_string_id });

            if (objIndex == last_road_option)
                highlighted_item = i;
        }

        Dropdown::showBelow(window, widgetIndex, i, 25, (1 << 6));
        Dropdown::setHighlightedItem(highlighted_item);
    }

    // 0x0043A8CE
    void townsMenuMouseDown(window* window, widget_index widgetIndex)
    {
        auto interface = ObjectManager::get<interface_skin_object>();
        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_towns, StringIds::menu_towns });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, StringIds::menu_industries });
        Dropdown::showBelow(window, widgetIndex, 2, 25, (1 << 6));
        Dropdown::setHighlightedItem(last_town_option);
    }

    // 0x0043A86D
    void zoomMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        window = WindowManager::getMainWindow();

        if (itemIndex == 0)
        {
            window->viewportZoomIn(false);
            TownManager::updateLabels();
            StationManager::updateLabels();
        }
        else if (itemIndex == 1)
        {
            zoom_ticks = 0;
            window->viewportZoomOut(false);
            TownManager::updateLabels();
            StationManager::updateLabels();
        }
        else if (itemIndex == 2)
        {
            Windows::Map::open();
        }
    }

    // 0x0043A624
    void rotateMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        auto mouseButtonUsed = Input::getLastKnownButtonState();
        window = WindowManager::getMainWindow();

        if (itemIndex == 1 || mouseButtonUsed == Input::mouse_button::right_pressed)
        {
            window->viewportRotateLeft();
            TownManager::updateLabels();
            StationManager::updateLabels();
            Windows::Map::centerOnViewPoint();
        }
        else if (itemIndex == 0)
        {
            window->viewportRotateRight();
            TownManager::updateLabels();
            StationManager::updateLabels();
            Windows::Map::centerOnViewPoint();
        }
    }

    // 0x0043AF37
    void viewMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        window = WindowManager::getMainWindow();
        auto viewport = WindowManager::getMainWindow()->viewports[0];

        if (itemIndex == 0)
            viewport->flags ^= ViewportFlags::underground_view;
        else if (itemIndex == 1)
            viewport->flags ^= ViewportFlags::hide_foreground_tracks_roads;
        else if (itemIndex == 2)
            viewport->flags ^= ViewportFlags::hide_foreground_scenery_buildings;
        else if (itemIndex == 4)
            viewport->flags ^= ViewportFlags::height_marks_on_tracks_roads;
        else if (itemIndex == 5)
            viewport->flags ^= ViewportFlags::height_marks_on_land;
        else if (itemIndex == 6)
            viewport->flags ^= ViewportFlags::one_way_direction_arrows;
        else if (itemIndex == 8)
            viewport->flags ^= ViewportFlags::town_names_displayed;
        else if (itemIndex == 9)
            viewport->flags ^= ViewportFlags::station_names_displayed;

        window->invalidate();
    }

    // 0x0043A4A8
    void terraformMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                Windows::Terraform::openClearArea();
                break;

            case 1:
                Windows::Terraform::openAdjustLand();
                break;

            case 2:
                Windows::Terraform::openAdjustWater();
                break;

            case 3:
                Windows::Terraform::openPlantTrees();
                break;

            case 4:
                Windows::Terraform::openBuildWalls();
                break;
        }
    }

    // 0x0043A28C
    void roadMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex == -1)
            return;

        uint8_t objIndex = available_objects[itemIndex];
        Construction::openWithFlags(objIndex);
    }

    // 0x0043A932
    void townsMenuDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (itemIndex == 0)
        {
            Windows::TownList::open();
            last_town_option = 0;
        }
        else if (itemIndex == 1)
        {
            Windows::IndustryList::open();
            last_town_option = 1;
        }
    }

    void onDropdown(window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case Widx::zoom_menu:
                zoomMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Widx::rotate_menu:
                rotateMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Widx::view_menu:
                viewMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Widx::terraform_menu:
                terraformMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Widx::road_menu:
                roadMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Widx::towns_menu:
                townsMenuDropdown(window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x0043A071
    void onMouseDown(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::zoom_menu:
                zoomMenuMouseDown(window, widgetIndex);
                break;

            case Widx::rotate_menu:
                rotateMenuMouseDown(window, widgetIndex);
                break;

            case Widx::view_menu:
                viewMenuMouseDown(window, widgetIndex);
                break;

            case Widx::terraform_menu:
                terraformMenuMouseDown(window, widgetIndex);
                break;

            case Widx::road_menu:
                roadMenuMouseDown(window, widgetIndex);
                break;

            case Widx::towns_menu:
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
            window->setDisabledWidgetsAndInvalidate(Widx::zoom_menu | Widx::rotate_menu);
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
