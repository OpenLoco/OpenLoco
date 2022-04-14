#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../EditorController.h"
#include "../Entities/EntityManager.h"
#include "../Game.h"
#include "../GameCommands/GameCommands.h"
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
#include "../S5/S5.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include "../Widget.h"
#include "ToolbarTopCommon.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarTop::Editor
{
    static loco_global<uint8_t, 0x00525FAB> last_road_option;
    static loco_global<uint8_t, 0x009C870C> last_town_option;

    namespace Widx
    {
        enum
        {
            map_generation_menu = Common::Widx::w2,
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::primary),  // 0
        makeWidget({ 30, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::primary), // 1
        makeWidget({ 60, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::primary), // 2

        makeWidget({ 104, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::secondary), // 3
        makeWidget({ 134, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::secondary), // 4
        makeWidget({ 164, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::secondary), // 5

        makeWidget({ 267, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::tertiary), // 6
        makeWidget({ 0, 0 }, { 1, 1 }, WidgetType::none, WindowColour::primary),            // 7
        makeWidget({ 357, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::tertiary), // 8
        makeWidget({ 0, 0 }, { 1, 1 }, WidgetType::none, WindowColour::primary),            // 9
        makeWidget({ 0, 0 }, { 1, 1 }, WidgetType::none, WindowColour::primary),            // 10

        makeWidget({ 0, 0 }, { 1, 1 }, WidgetType::none, WindowColour::primary),              // 11
        makeWidget({ 0, 0 }, { 1, 1 }, WidgetType::none, WindowColour::primary),              // 12
        makeWidget({ 460, 0 }, { 30, 28 }, WidgetType::toolbarTab, WindowColour::quaternary), // 13
        widgetEnd(),
    };

    static WindowEventList _events;

    static void onMouseDown(Window* window, WidgetIndex_t widgetIndex);
    static void onDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    static void prepareDraw(Window* window);

    static void initEvents()
    {
        _events.onResize = Common::onResize;
        _events.event_03 = onMouseDown;
        _events.onMouseDown = onMouseDown;
        _events.onDropdown = onDropdown;
        _events.onUpdate = Common::onUpdate;
        _events.prepareDraw = prepareDraw;
        _events.draw = Common::draw;
    }

    // 0x0043CC2C
    void open()
    {
        initEvents();

        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            Ui::Size(Ui::width(), 28),
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            &_events);
        window->widgets = _widgets;
        window->enabledWidgets = (1 << Common::Widx::loadsave_menu) | (1 << Common::Widx::audio_menu) | (1 << Common::Widx::zoom_menu) | (1 << Common::Widx::rotate_menu) | (1 << Common::Widx::view_menu) | (1 << Common::Widx::terraform_menu) | (1 << Widx::map_generation_menu) | (1 << Common::Widx::road_menu) | (1 << Common::Widx::towns_menu);
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, Colour::grey);
        window->setColour(WindowColour::secondary, Colour::grey);
        window->setColour(WindowColour::tertiary, Colour::grey);
        window->setColour(WindowColour::quaternary, Colour::grey);

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, skin->colour_12);
            window->setColour(WindowColour::secondary, skin->colour_13);
            window->setColour(WindowColour::tertiary, skin->colour_14);
            window->setColour(WindowColour::quaternary, skin->colour_15);
        }
    }

    // 0x0043D638
    static void loadsaveMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::load_landscape);
        Dropdown::add(1, StringIds::save_landscape);
        Dropdown::add(2, 0);
        Dropdown::add(3, StringIds::menu_about);
        Dropdown::add(4, StringIds::options);
        Dropdown::add(5, StringIds::menu_screenshot);
        Dropdown::add(6, 0);
        Dropdown::add(7, StringIds::menu_quit_to_menu);
        Dropdown::add(8, StringIds::menu_exit_openloco);
        Dropdown::showBelow(window, widgetIndex, 9, 0);
        Dropdown::setHighlightedItem(1);
    }

    // 0x0043D695
    static void loadsaveMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                // Load Landscape
                GameCommands::do_21(0, 0);
                break;

            case 1:
            {
                if (S5::getOptions().editorStep == EditorController::Step::objectSelection)
                    ObjectSelectionWindow::closeWindow();

                WindowManager::closeAllFloatingWindows();
                Input::toolCancel();

                // Save Landscape
                if (OpenLoco::Game::saveLandscapeOpen())
                {
                    OpenLoco::Game::saveLandscape();
                    Gfx::invalidateScreen();
                }
                break;
            }

            case 3:
                About::open();
                break;

            case 4:
                Options::open();
                break;

            case 5:
            {
                loco_global<uint8_t, 0x00508F16> screenshot_countdown;
                screenshot_countdown = 10;
                break;
            }

            case 7:
                // Return to title screen
                GameCommands::do_21(0, 1);
                break;

            case 8:
                // Exit to desktop
                GameCommands::do_21(0, 2);
                break;
        }
    }

    // 0x0043D789
    static void audioMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_mute);
        Dropdown::showBelow(window, widgetIndex, 1, 0);

        if (!Audio::isAudioEnabled())
            Dropdown::setItemSelected(0);

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043D7C1
    static void audioMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                Audio::toggleSound();
                break;
        }
    }

    // 0x004402BC
    static void mapGenerationMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::landscape_generation_options);
        Dropdown::showBelow(window, widgetIndex, 1, 0);
        Dropdown::setHighlightedItem(0);
    }

    // 0x004402DA
    static void mapGenerationMenuDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        switch (itemIndex)
        {
            case 0:
                Windows::LandscapeGeneration::open();
                break;
        }
    }

    // 0x0043D541
    static void onMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuMouseDown(window, widgetIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuMouseDown(window, widgetIndex);
                break;

            case Widx::map_generation_menu:
                mapGenerationMenuMouseDown(window, widgetIndex);
                break;

            default:
                Common::onMouseDown(window, widgetIndex);
                break;
        }
    }

    // 0x0043D5A6
    static void onDropdown(Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuDropdown(window, widgetIndex, itemIndex);
                break;

            case Widx::map_generation_menu:
                mapGenerationMenuDropdown(window, widgetIndex, itemIndex);
                break;

            default:
                Common::onDropdown(window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x0043D2F3
    static void prepareDraw(Window* window)
    {
        uint32_t x = std::max(640, Ui::width()) - 1;

        Common::rightAlignTabs(window, x, { Common::Widx::towns_menu });
        x -= 11;
        Common::rightAlignTabs(window, x, { Common::Widx::road_menu, Common::Widx::terraform_menu });

        if (EditorController::getCurrentStep() == EditorController::Step::landscapeEditor)
        {
            window->widgets[Common::Widx::zoom_menu].type = WidgetType::toolbarTab;
            window->widgets[Common::Widx::rotate_menu].type = WidgetType::toolbarTab;
            window->widgets[Common::Widx::view_menu].type = WidgetType::toolbarTab;
            window->widgets[Common::Widx::terraform_menu].type = WidgetType::toolbarTab;
            window->widgets[Widx::map_generation_menu].type = WidgetType::toolbarTab;
            window->widgets[Common::Widx::towns_menu].type = WidgetType::toolbarTab;
            if (last_road_option != 0xFF)
            {
                window->widgets[Common::Widx::road_menu].type = WidgetType::toolbarTab;
            }
            else
            {
                window->widgets[Common::Widx::road_menu].type = WidgetType::none;
            }
        }
        else
        {
            window->widgets[Common::Widx::zoom_menu].type = WidgetType::none;
            window->widgets[Common::Widx::rotate_menu].type = WidgetType::none;
            window->widgets[Common::Widx::view_menu].type = WidgetType::none;
            window->widgets[Common::Widx::terraform_menu].type = WidgetType::none;
            window->widgets[Widx::map_generation_menu].type = WidgetType::none;
            window->widgets[Common::Widx::road_menu].type = WidgetType::none;
            window->widgets[Common::Widx::towns_menu].type = WidgetType::none;
        }

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        if (!Audio::isAudioEnabled())
        {
            window->activatedWidgets |= (1 << Common::Widx::audio_menu);
            window->widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_inactive, window->getColour(WindowColour::primary).c());
        }
        else
        {
            window->activatedWidgets &= ~(1 << Common::Widx::audio_menu);
            window->widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_active, window->getColour(WindowColour::primary).c());
        }

        window->widgets[Common::Widx::loadsave_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_loadsave);
        window->widgets[Common::Widx::zoom_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_zoom);
        window->widgets[Common::Widx::rotate_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_rotate);
        window->widgets[Common::Widx::view_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_view);

        window->widgets[Common::Widx::terraform_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_terraform);
        window->widgets[Widx::map_generation_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_cogwheels);
        window->widgets[Common::Widx::road_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);

        if (last_town_option == 0)
            window->widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_towns);
        else
            window->widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_industries);
    }
}
