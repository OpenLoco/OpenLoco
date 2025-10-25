#include "Audio/Audio.h"
#include "Config.h"
#include "EditorController.h"
#include "Entities/EntityManager.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackObject.h"
#include "Objects/WaterObject.h"
#include "ScenarioOptions.h"
#include "ToolbarTopCommon.h"
#include "Ui/Dropdown.h"
#include "Ui/Screenshot.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "World/CompanyManager.h"
#include "World/StationManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolbarTop::Editor
{
    static loco_global<uint8_t, 0x009C870C> _lastTownOption;

    namespace Widx
    {
        enum
        {
            map_generation_menu = Common::Widx::w2,
        };
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::ImageButton({ 0, 0 }, { 30, 28 }, WindowColour::primary),  // 0
        Widgets::ImageButton({ 30, 0 }, { 30, 28 }, WindowColour::primary), // 1
        Widgets::ImageButton({ 60, 0 }, { 30, 28 }, WindowColour::primary), // 2

        Widgets::ImageButton({ 104, 0 }, { 30, 28 }, WindowColour::secondary), // 3
        Widgets::ImageButton({ 134, 0 }, { 30, 28 }, WindowColour::secondary), // 4
        Widgets::ImageButton({ 164, 0 }, { 30, 28 }, WindowColour::secondary), // 5

        Widgets::ImageButton({ 267, 0 }, { 30, 28 }, WindowColour::tertiary), // 6
        Widgets::ImageButton({ 0, 0 }, { 1, 1 }, WindowColour::primary),      // 7
        Widgets::ImageButton({ 357, 0 }, { 30, 28 }, WindowColour::tertiary), // 8
        Widgets::ImageButton({ 0, 0 }, { 1, 1 }, WindowColour::primary),      // 9
        Widgets::ImageButton({ 0, 0 }, { 1, 1 }, WindowColour::primary),      // 10

        Widgets::ImageButton({ 0, 0 }, { 1, 1 }, WindowColour::primary),       // 11
        Widgets::ImageButton({ 0, 0 }, { 1, 1 }, WindowColour::primary),       // 12
        Widgets::ImageButton({ 460, 0 }, { 30, 28 }, WindowColour::quaternary) // 13
    );

    static const WindowEventList& getEvents();

    // 0x0043CC2C
    void open()
    {
        auto window = WindowManager::createWindow(
            WindowType::topToolbar,
            { 0, 0 },
            { Ui::width(), 28 },
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground,
            getEvents());
        window->setWidgets(_widgets);
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, Colour::grey);
        window->setColour(WindowColour::secondary, Colour::grey);
        window->setColour(WindowColour::tertiary, Colour::grey);
        window->setColour(WindowColour::quaternary, Colour::grey);

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, skin->topToolbarPrimaryColour);
            window->setColour(WindowColour::secondary, skin->topToolbarSecondaryColour);
            window->setColour(WindowColour::tertiary, skin->topToolbarTertiaryColour);
            window->setColour(WindowColour::quaternary, skin->topToolbarQuaternaryColour);
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
        Dropdown::add(6, StringIds::menu_giant_screenshot);
        Dropdown::add(7, 0);
        Dropdown::add(8, StringIds::menu_quit_to_menu);
        Dropdown::add(9, StringIds::menu_exit_openloco);
        Dropdown::showBelow(window, widgetIndex, 9, 0);
        Dropdown::setHighlightedItem(1);
    }

    // 0x0043D695
    static void loadsaveMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        switch (itemIndex)
        {
            case 0:
                // Load Landscape
                {
                    GameCommands::LoadSaveQuitGameArgs args{};
                    args.loadQuitMode = LoadOrQuitMode::loadGamePrompt;
                    args.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
                    GameCommands::doCommand(args, GameCommands::Flags::apply);
                }
                break;

            case 1:
            {
                if (Scenario::getOptions().editorStep == EditorController::Step::objectSelection)
                {
                    if (!ObjectSelectionWindow::tryCloseWindow())
                    {
                        // Try close has failed so do not open save window!
                        return;
                    }
                }
                WindowManager::closeAllFloatingWindows();
                ToolManager::toolCancel();

                // Save Landscape
                if (auto res = OpenLoco::Game::saveLandscapeOpen())
                {
                    OpenLoco::Game::saveLandscape(*res);
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
                triggerScreenshotCountdown(10, ScreenshotType::regular);
                break;

            case 6:
                triggerScreenshotCountdown(10, ScreenshotType::giant);
                break;

            case 8:
                // Return to title screen
                {
                    GameCommands::LoadSaveQuitGameArgs quitToMenuArgs{};
                    quitToMenuArgs.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
                    quitToMenuArgs.loadQuitMode = LoadOrQuitMode::returnToTitlePrompt;
                    GameCommands::doCommand(quitToMenuArgs, GameCommands::Flags::apply);
                }
                break;

            case 9:
                // Exit to desktop
                {
                    GameCommands::LoadSaveQuitGameArgs quitToDesktopArgs{};
                    quitToDesktopArgs.saveMode = GameCommands::LoadSaveQuitGameArgs::SaveMode::promptSave;
                    quitToDesktopArgs.loadQuitMode = LoadOrQuitMode::quitGamePrompt;
                    GameCommands::doCommand(quitToDesktopArgs, GameCommands::Flags::apply);
                }
                break;
        }
    }

    // 0x0043D789
    static void audioMenuMouseDown(Window* window, WidgetIndex_t widgetIndex)
    {
        Dropdown::add(0, StringIds::dropdown_without_checkmark, StringIds::menu_mute);
        Dropdown::showBelow(window, widgetIndex, 1, 0);

        if (!Audio::isAudioEnabled())
        {
            Dropdown::setItemSelected(0);
        }

        Dropdown::setHighlightedItem(0);
    }

    // 0x0043D7C1
    static void audioMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

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
        auto numItems = 1;

        if (Config::get().cheatsMenuEnabled)
        {
            Dropdown::add(1, StringIds::tile_inspector);
            numItems += 1;
        }

        Dropdown::showBelow(window, widgetIndex, numItems, 0);
        Dropdown::setHighlightedItem(0);
    }

    // 0x004402DA
    static void mapGenerationMenuDropdown([[maybe_unused]] Window* window, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
        {
            itemIndex = Dropdown::getHighlightedItem();
        }

        switch (itemIndex)
        {
            case 0:
                Windows::LandscapeGeneration::open();
                break;

            case 1:
                TileInspector::open();
                break;
        }
    }

    // 0x0043D541
    static void onMouseDown(Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuMouseDown(&window, widgetIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuMouseDown(&window, widgetIndex);
                break;

            case Widx::map_generation_menu:
                mapGenerationMenuMouseDown(&window, widgetIndex);
                break;

            default:
                Common::onMouseDown(&window, widgetIndex);
                break;
        }
    }

    // 0x0043D5A6
    static void onDropdown(Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case Common::Widx::loadsave_menu:
                loadsaveMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Common::Widx::audio_menu:
                audioMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            case Widx::map_generation_menu:
                mapGenerationMenuDropdown(&window, widgetIndex, itemIndex);
                break;

            default:
                Common::onDropdown(&window, widgetIndex, itemIndex);
                break;
        }
    }

    // 0x0043D2F3
    static void prepareDraw(Window& window)
    {
        uint32_t x = std::max(640, Ui::width()) - 1;

        Common::rightAlignTabs(&window, x, { Common::Widx::towns_menu });
        x -= 11;
        Common::rightAlignTabs(&window, x, { Common::Widx::road_menu, Common::Widx::terraform_menu });

        const bool isLandscapeEditor = EditorController::getCurrentStep() == EditorController::Step::landscapeEditor;

        window.widgets[Common::Widx::zoom_menu].hidden = !isLandscapeEditor;
        window.widgets[Common::Widx::rotate_menu].hidden = !isLandscapeEditor;
        window.widgets[Common::Widx::view_menu].hidden = !isLandscapeEditor;
        window.widgets[Common::Widx::terraform_menu].hidden = !isLandscapeEditor;
        window.widgets[Widx::map_generation_menu].hidden = !isLandscapeEditor;
        window.widgets[Common::Widx::towns_menu].hidden = !isLandscapeEditor;
        window.widgets[Common::Widx::road_menu].hidden = !(isLandscapeEditor && getGameState().lastRoadOption != 0xFF);

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        if (!Audio::isAudioEnabled())
        {
            window.activatedWidgets |= (1 << Common::Widx::audio_menu);
            window.widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_inactive, window.getColour(WindowColour::primary).c());
        }
        else
        {
            window.activatedWidgets &= ~(1 << Common::Widx::audio_menu);
            window.widgets[Common::Widx::audio_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_audio_active, window.getColour(WindowColour::primary).c());
        }

        window.widgets[Common::Widx::loadsave_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_loadsave);
        window.widgets[Common::Widx::zoom_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_zoom);
        window.widgets[Common::Widx::rotate_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_rotate);
        window.widgets[Common::Widx::view_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_view);

        window.widgets[Common::Widx::terraform_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_terraform);
        window.widgets[Widx::map_generation_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_cogwheels);
        window.widgets[Common::Widx::road_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_empty_opaque);

        if (_lastTownOption == 0)
        {
            window.widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_towns);
        }
        else
        {
            window.widgets[Common::Widx::towns_menu].image = Gfx::recolour(interface->img + InterfaceSkin::ImageIds::toolbar_industries);
        }
    }

    static constexpr WindowEventList kEvents = {
        .onResize = Common::onResize,
        .onMouseHover = onMouseDown,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = Common::onUpdate,
        .prepareDraw = prepareDraw,
        .draw = Common::draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
