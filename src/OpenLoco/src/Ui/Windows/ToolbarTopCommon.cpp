#include "ToolbarTopCommon.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/WaterObject.h"
#include "Ui/Dropdown.h"
#include "Ui/Widget.h"
#include "Vehicles/Vehicle.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <map>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarTop::Common
{
    static loco_global<uint32_t, 0x009C86F8> _zoomTicks;

    static loco_global<uint8_t, 0x009C870C> _lastTownOption;

    // Temporary storage for road menu dropdown (populated in mouseDown, consumed in dropdown callback)
    static AvailableTracksAndRoads _roadMenuObjects;

    // 0x00439DE4
    void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        // Draw widgets.
        self.draw(drawingCtx);

        const auto companyColour = CompanyManager::getPlayerCompanyColour();

        auto lastRoadOption = getGameState().lastRoadOption;

        if (!self.widgets[Widx::road_menu].hidden && lastRoadOption != 0xFF)
        {
            uint32_t x = self.widgets[Widx::road_menu].left + self.x;
            uint32_t y = self.widgets[Widx::road_menu].top + self.y;
            uint32_t fgImage = 0;

            // Figure out what icon to show on the button face.
            bool isRoad = lastRoadOption & (1 << 7);
            if (isRoad)
            {
                auto obj = ObjectManager::get<RoadObject>(lastRoadOption & ~(1 << 7));
                fgImage = Gfx::recolour(obj->image, companyColour);
            }
            else
            {
                auto obj = ObjectManager::get<TrackObject>(lastRoadOption);
                fgImage = Gfx::recolour(obj->image + TrackObj::ImageIds::kUiPreviewImage0, companyColour);
            }

            y--;
            auto interface = ObjectManager::get<InterfaceSkinObject>();
            uint32_t bgImage = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_transparent, self.getColour(WindowColour::tertiary).c());

            if (Input::isDropdownActive(Ui::WindowType::topToolbar, self.number, Widx::road_menu))
            {
                y++;
                bgImage++;
            }

            drawingCtx.drawImage(x, y, fgImage);

            y = self.widgets[Widx::road_menu].top + self.y;
            drawingCtx.drawImage(x, y, bgImage);
        }
    }

    // 0x0043A78E
    void zoomMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_zoom_in, StringIds::menu_zoom_in });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_zoom_out, StringIds::menu_zoom_out });

        static constexpr uint32_t kMapSpritesByRotation[] = {
            InterfaceSkin::ImageIds::toolbar_menu_map_north,
            InterfaceSkin::ImageIds::toolbar_menu_map_west,
            InterfaceSkin::ImageIds::toolbar_menu_map_south,
            InterfaceSkin::ImageIds::toolbar_menu_map_east,
        };

        uint32_t mapSprite = kMapSpritesByRotation[WindowManager::getCurrentRotation()];

        Dropdown::add(2, StringIds::menu_sprite_stringid, { interface->img + mapSprite, StringIds::menu_map });
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
            _zoomTicks = 1000;
        }

        if (mainWindow->viewports[0]->zoom != 3 && _zoomTicks <= 32)
        {
            Dropdown::setHighlightedItem(1);
        }
    }

    // 0x0043A5C5
    void rotateMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_rotate_clockwise, StringIds::menu_rotate_clockwise });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_rotate_anti_clockwise, StringIds::menu_rotate_anti_clockwise });
        Dropdown::showBelow(window, widgetIndex, 2, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);
    }

    // 0x0043ADF6
    void viewMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_underground_view);
        Dropdown::add(1, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughTracks);
        Dropdown::add(2, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughRoads);
        Dropdown::add(3, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughTrees);
        Dropdown::add(4, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughBuildings);
        Dropdown::add(5, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughScenery);
        Dropdown::add(6, StringIds::dropdown_without_checkmark, StringIds::menuSeeThroughBridges);
        Dropdown::add(7, 0);
        Dropdown::add(8, StringIds::dropdown_without_checkmark, StringIds::menu_height_marks_on_land);
        Dropdown::add(9, StringIds::dropdown_without_checkmark, StringIds::menu_height_marks_on_tracks_roads);
        Dropdown::add(10, StringIds::dropdown_without_checkmark, StringIds::menu_one_way_direction_arrows);
        Dropdown::add(11, 0);
        Dropdown::add(12, StringIds::dropdown_without_checkmark, StringIds::menu_town_names_displayed);
        Dropdown::add(13, StringIds::dropdown_without_checkmark, StringIds::menu_station_names_displayed);
        Dropdown::showBelow(window, widgetIndex, 14, 0);

        ViewportFlags current_viewport_flags = WindowManager::getMainWindow()->viewports[0]->flags;

        if ((current_viewport_flags & ViewportFlags::underground_view) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(0);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughTracks) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(1);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughRoads) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(2);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughTrees) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(3);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughBuildings) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(4);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughScenery) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(5);
        }

        if ((current_viewport_flags & ViewportFlags::seeThroughBridges) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(6);
        }

        if ((current_viewport_flags & ViewportFlags::height_marks_on_land) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(8);
        }

        if ((current_viewport_flags & ViewportFlags::height_marks_on_tracks_roads) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(9);
        }

        if ((current_viewport_flags & ViewportFlags::one_way_direction_arrows) != ViewportFlags::none)
        {
            Dropdown::setItemSelected(10);
        }

        if ((current_viewport_flags & ViewportFlags::town_names_displayed) == ViewportFlags::none)
        {
            Dropdown::setItemSelected(12);
        }

        if ((current_viewport_flags & ViewportFlags::station_names_displayed) == ViewportFlags::none)
        {
            Dropdown::setItemSelected(13);
        }

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043A3C3
    void terraformMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();
        auto land = ObjectManager::get<LandObject>(getGameState().lastLandOption);
        auto water = ObjectManager::get<WaterObject>();

        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_bulldozer, StringIds::menu_clear_area });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { land->mapPixelImage + Land::ImageIds::toolbar_terraform_land, StringIds::menu_adjust_land });
        Dropdown::add(2, StringIds::menu_sprite_stringid, { water->image + Water::ImageIds::kToolbarTerraformWater, StringIds::menu_adjust_water });
        Dropdown::add(3, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_plant_trees, StringIds::menu_plant_trees });
        Dropdown::add(4, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_build_walls, StringIds::menu_build_walls });
        Dropdown::showBelow(window, widgetIndex, 5, 25, (1 << 6));
        Dropdown::setHighlightedItem(0);
    }

    // 0x0043A19F
    void roadMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        _roadMenuObjects = companyGetAvailableRoads(CompanyManager::getControllingId());

        // Sanity check: any objects available?
        if (_roadMenuObjects.empty())
        {
            return;
        }

        auto companyColour = CompanyManager::getPlayerCompanyColour();

        // Add available objects to Dropdown.
        uint16_t highlightedItem = 0;
        for (auto i = 0U; i < _roadMenuObjects.size(); i++)
        {
            uint32_t objImage;
            StringId objStringId;

            auto objIndex = _roadMenuObjects[i];
            if ((objIndex & (1 << 7)) != 0)
            {
                auto road = ObjectManager::get<RoadObject>(objIndex & 0x7F);
                objStringId = road->name;
                objImage = Gfx::recolour(road->image, companyColour);
            }
            else
            {
                auto track = ObjectManager::get<TrackObject>(objIndex);
                objStringId = track->name;
                objImage = Gfx::recolour(track->image + TrackObj::ImageIds::kUiPreviewImage0, companyColour);
            }

            Dropdown::add(i, StringIds::menu_sprite_stringid_construction, { objImage, objStringId });

            if (objIndex == getGameState().lastRoadOption)
            {
                highlightedItem = i;
            }
        }

        Dropdown::showBelow(window, widgetIndex, _roadMenuObjects.size(), 25, (1 << 6));
        Dropdown::setHighlightedItem(highlightedItem);
    }

    // 0x0043A8CE
    void townsMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        auto interface = ObjectManager::get<InterfaceSkinObject>();
        Dropdown::add(0, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_towns, StringIds::menu_towns });
        Dropdown::add(1, StringIds::menu_sprite_stringid, { interface->img + InterfaceSkin::ImageIds::toolbar_menu_industries, StringIds::menu_industries });
        Dropdown::showBelow(window, widgetIndex, 2, 25, (1 << 6));
        Dropdown::setHighlightedItem(_lastTownOption);
    }

    // 0x0043A86D
    void zoomMenuDropdown(Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        window = WindowManager::getMainWindow();

        if (itemIndex == 0)
        {
            window->viewportZoomIn(false);
            TownManager::updateLabels();
            StationManager::updateLabels();
        }
        else if (itemIndex == 1)
        {
            _zoomTicks = 0;
            window->viewportZoomOut(false);
            TownManager::updateLabels();
            StationManager::updateLabels();
        }
        else if (itemIndex == 2)
        {
            MapWindow::open();
        }
    }

    // 0x0043A624
    void rotateMenuDropdown(Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        auto mouseButtonUsed = Input::getLastKnownButtonState();
        window = WindowManager::getMainWindow();

        if (itemIndex == 1 || mouseButtonUsed == Input::MouseButton::rightPressed)
        {
            window->viewportRotateLeft();
            TownManager::updateLabels();
            StationManager::updateLabels();
            MapWindow::centerOnViewPoint();
        }
        else if (itemIndex == 0)
        {
            window->viewportRotateRight();
            TownManager::updateLabels();
            StationManager::updateLabels();
            MapWindow::centerOnViewPoint();
        }
    }

    // 0x0043AF37
    void viewMenuDropdown(Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        window = WindowManager::getMainWindow();
        auto viewport = WindowManager::getMainWindow()->viewports[0];

        if (itemIndex == 0)
        {
            viewport->flags ^= ViewportFlags::underground_view;
        }
        else if (itemIndex == 1)
        {
            viewport->flags ^= ViewportFlags::seeThroughTracks;
        }
        else if (itemIndex == 2)
        {
            viewport->flags ^= ViewportFlags::seeThroughRoads;
        }
        else if (itemIndex == 3)
        {
            viewport->flags ^= ViewportFlags::seeThroughTrees;
        }
        else if (itemIndex == 4)
        {
            viewport->flags ^= ViewportFlags::seeThroughBuildings;
        }
        else if (itemIndex == 5)
        {
            viewport->flags ^= ViewportFlags::seeThroughScenery;
        }
        else if (itemIndex == 6)
        {
            viewport->flags ^= ViewportFlags::seeThroughBridges;
        }
        else if (itemIndex == 8)
        {
            viewport->flags ^= ViewportFlags::height_marks_on_land;
        }
        else if (itemIndex == 9)
        {
            viewport->flags ^= ViewportFlags::height_marks_on_tracks_roads;
        }
        else if (itemIndex == 10)
        {
            viewport->flags ^= ViewportFlags::one_way_direction_arrows;
        }
        else if (itemIndex == 12)
        {
            viewport->flags ^= ViewportFlags::town_names_displayed;
        }
        else if (itemIndex == 13)
        {
            viewport->flags ^= ViewportFlags::station_names_displayed;
        }

        window->invalidate();
    }

    // 0x0043A4A8
    void terraformMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        switch (itemIndex)
        {
            case 0:
                Terraform::openClearArea();
                break;

            case 1:
                Terraform::openAdjustLand();
                break;

            case 2:
                Terraform::openAdjustWater();
                break;

            case 3:
                Terraform::openPlantTrees();
                break;

            case 4:
                Terraform::openBuildWalls();
                break;
        }
    }

    // 0x0043A28C
    void roadMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        if (itemIndex == -1)
        {
            return;
        }

        uint8_t objIndex = _roadMenuObjects[itemIndex];
        Construction::openWithFlags(objIndex);
    }

    // 0x0043A932
    void townsMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        if (itemIndex == 0)
        {
            TownList::open();
            _lastTownOption = 0;
        }
        else if (itemIndex == 1)
        {
            IndustryList::open();
            _lastTownOption = 1;
        }
    }

    void onDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
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
    void onMouseDown(Window* window, WidgetIndex_t widgetIndex)
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

    void onUpdate([[maybe_unused]] Window& window)
    {
        _zoomTicks++;
    }

    // 0x0043A17E
    void onResize(Window& window)
    {
        auto main = WindowManager::getMainWindow();
        if (main == nullptr)
        {
            window.setDisabledWidgetsAndInvalidate(Widx::zoom_menu | Widx::rotate_menu);
        }
        else
        {
            window.setDisabledWidgetsAndInvalidate(0);
        }
    }

    void rightAlignTabs(Window* window, uint32_t& x, const std::initializer_list<uint32_t> widxs)
    {
        for (const auto& widx : widxs)
        {
            window->widgets[widx].right = x;
            window->widgets[widx].left = x - 29;
            x -= 30;
        }
    }
}
