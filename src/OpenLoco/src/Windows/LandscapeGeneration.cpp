#include "Audio/Audio.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Environment.h"
#include "Game.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "LastGameOptionManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/MapGenerator/MapGenerator.h"
#include "Objects/HillShapesObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/WaterObject.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "Ui/Dropdown.h"
#include "Ui/ToolManager.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "World/IndustryManager.h"
#include "World/TownManager.h"
#include <OpenLoco/Diagnostics/Logging.h>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Diagnostics;
using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::LandscapeGeneration
{
    static constexpr Ui::Size kWindowSize = { 366, 217 };
    static constexpr Ui::Size kLandTabSize = { 366, 247 };

    static constexpr uint8_t kRowHeight = 22; // CJK: 22

    static constexpr size_t kMaxLandObjects = ObjectManager::getMaxObjects(ObjectType::land);

    namespace Common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_options,
            tab_land,
            tab_water,
            tab_forests,
            tab_towns,
            tab_industries,
        };

        const uint64_t enabled_widgets = (1 << widx::close_button) | (1 << tab_options) | (1 << tab_land) | (1 << tab_water) | (1 << tab_forests) | (1 << tab_towns) | (1 << tab_industries);

#define common_options_widgets(frame_height, window_caption_id)                                                                                            \
    makeWidget({ 0, 0 }, { 366, frame_height }, WidgetType::frame, WindowColour::primary),                                                                 \
        makeWidget({ 1, 1 }, { 364, 13 }, WidgetType::caption_25, WindowColour::primary, window_caption_id),                                               \
        makeWidget({ 351, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),   \
        makeWidget({ 0, 41 }, { 366, 175 }, WidgetType::panel, WindowColour::secondary),                                                                   \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_landscape_generation_options),  \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_landscape_generation_land),    \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_landscape_generation_water),   \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_landscape_generation_forests), \
        makeRemapWidget({ 127, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_landscape_generation_towns),  \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_landscape_generation_industries)

        // Defined at the bottom of this file.
        static void switchTabWidgets(Window* window);
        static void switchTab(Window* window, WidgetIndex_t widgetIndex);

        // 0x0043ECA4
        static void drawTabs(Window* window, Gfx::RenderTarget* rt)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Options tab
            {
                static const uint32_t optionTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_cogs_frame0,
                    InterfaceSkin::ImageIds::tab_cogs_frame1,
                    InterfaceSkin::ImageIds::tab_cogs_frame2,
                    InterfaceSkin::ImageIds::tab_cogs_frame3,
                };

                uint32_t imageId = skin->img;
                if (window->currentTab == widx::tab_options - widx::tab_options)
                    imageId += optionTabImageIds[(window->frameNo / 2) % std::size(optionTabImageIds)];
                else
                    imageId += optionTabImageIds[0];

                Widget::drawTab(window, rt, imageId, widx::tab_options);
            }

            // Land tab
            {
                auto land = ObjectManager::get<LandObject>(LastGameOptionManager::getLastLand());
                const uint32_t imageId = land->mapPixelImage + Land::ImageIds::toolbar_terraform_land;
                Widget::drawTab(window, rt, imageId, widx::tab_land);
            }

            // Water tab
            {
                const auto waterObj = ObjectManager::get<WaterObject>();
                uint32_t imageId = waterObj->image + Water::ImageIds::kToolbarTerraformWater;
                if (window->currentTab == widx::tab_water - widx::tab_options)
                    imageId += (window->frameNo / 2) % 16;

                Widget::drawTab(window, rt, imageId, widx::tab_water);
            }

            // Forest tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::toolbar_menu_plant_trees;
                Widget::drawTab(window, rt, imageId, widx::tab_forests);
            }

            // Towns tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::toolbar_menu_towns;
                Widget::drawTab(window, rt, imageId, widx::tab_towns);
            }

            // Industries tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::toolbar_menu_industries;
                Widget::drawTab(window, rt, imageId, widx::tab_industries);
            }
        }

        static void draw(Window& window, Gfx::RenderTarget* rt)
        {
            window.draw(rt);
            drawTabs(&window, rt);
        }

        static void prepareDraw(Window& window)
        {
            switchTabWidgets(&window);

            window.widgets[widx::frame].right = window.width - 1;
            window.widgets[widx::frame].bottom = window.height - 1;

            window.widgets[widx::panel].right = window.width - 1;
            window.widgets[widx::panel].bottom = window.height - 1;

            window.widgets[widx::caption].right = window.width - 2;

            window.widgets[widx::close_button].left = window.width - 15;
            window.widgets[widx::close_button].right = window.width - 3;
        }

        static void update(Window& window)
        {
            window.frameNo++;
            window.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::landscapeGeneration, window.number, window.currentTab + widx::tab_options);
        }
    }

    namespace Options
    {
        enum widx
        {
            groupGeneral = Common::widx::tab_industries + 1,
            start_year,
            start_year_down,
            start_year_up,
            heightMapBox,
            heightMapDropdown,

            groupGenerator,
            change_heightmap_btn,
            terrainSmoothingNum,
            terrainSmoothingNumDown,
            terrainSmoothingNumUp,
            generate_when_game_starts,

            heightmap_poc,

            generate_now,
        };

        // clang-format off
        const uint64_t holdable_widgets =
            (1 << widx::start_year_up) |
            (1 << widx::start_year_down) |
            (1 << widx::terrainSmoothingNumUp) |
            (1 << widx::terrainSmoothingNumDown);

        const uint64_t enabled_widgets = Common::enabled_widgets |
            (1 << widx::start_year_up) |
            (1 << widx::start_year_down) |
            (1 << widx::heightMapBox) |
            (1 << widx::heightMapDropdown) |
            (1 << widx::change_heightmap_btn) |
            (1 << widx::terrainSmoothingNumUp) |
            (1 << widx::terrainSmoothingNumDown) |
            (1 << widx::generate_when_game_starts) |
            (1 << widx::generate_now);
        // clang-format on

        static Widget widgets[] = {
            common_options_widgets(217, StringIds::title_landscape_generation_options),

            // General options
            makeWidget({ 4, 50 }, { 358, 50 }, WidgetType::groupbox, WindowColour::secondary, StringIds::landscapeOptionsGroupGeneral),
            makeStepperWidgets({ 256, 65 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::start_year_value),
            makeDropdownWidgets({ 176, 81 }, { 180, 12 }, WidgetType::combobox, WindowColour::secondary),

            // Generator options
            makeWidget({ 4, 105 }, { 358, 50 }, WidgetType::groupbox, WindowColour::secondary, StringIds::landscapeOptionsGroupGenerator),
            makeWidget({ 280, 120 }, { 75, 12 }, WidgetType::button, WindowColour::secondary, StringIds::change),
            makeStepperWidgets({ 256, 120 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary),
            makeWidget({ 10, 136 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::label_generate_random_landscape_when_game_starts, StringIds::tooltip_generate_random_landscape_when_game_starts),

            // PNG browser
            makeWidget({ 280, 120 }, { 75, 12 }, WidgetType::button, WindowColour::secondary, StringIds::change),

            // Generate button
            makeWidget({ 196, 200 }, { 160, 12 }, WidgetType::button, WindowColour::secondary, StringIds::button_generate_landscape, StringIds::tooltip_generate_random_landscape),
            widgetEnd()
        };

        // 0x0043DC30
        static void draw(Window& window, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            Common::draw(window, rt);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::start_year].top,
                Colour::black,
                StringIds::start_year);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::heightMapBox].top,
                Colour::black,
                StringIds::height_map_source);

            auto& options = S5::getOptions();
            switch (options.generator)
            {
                case S5::LandGeneratorType::Original:
                {
                    auto* obj = ObjectManager::get<HillShapesObject>();
                    FormatArguments args{};
                    args.push(obj->name);

                    auto pos = Point(window.x + 10, window.y + window.widgets[widx::change_heightmap_btn].top);
                    drawingCtx.drawStringLeft(*rt, &pos, Colour::black, StringIds::landscapeOptionsCurrentHillObject, &args);
                    break;
                }

                case S5::LandGeneratorType::Simplex:
                {
                    // Draw label
                    auto& widget = window.widgets[widx::terrainSmoothingNum];
                    drawingCtx.drawStringLeft(
                        *rt,
                        window.x + 10,
                        window.y + widget.top,
                        Colour::black,
                        StringIds::landscapeOptionsSmoothingPasses);

                    // Prepare value
                    FormatArguments args{};
                    args.push(StringIds::uint16_raw);
                    args.push<uint16_t>(options.numTerrainSmoothingPasses);

                    // Draw value
                    auto pos = Point(window.x + widget.left + 1, window.y + widget.top);
                    drawingCtx.drawStringLeft(*rt, &pos, Colour::black, StringIds::black_stringid, &args);
                }

                case S5::LandGeneratorType::PngHeightMap:
                {
                    // TODO: show current filename
                }
            }
        }

        static const StringId generatorIds[] = {
            StringIds::generator_original,
            StringIds::generator_simplex,
            StringIds::generator_png_heightmap,
        };

        // 0x0043DB76
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            auto& options = S5::getOptions();

            auto args = FormatArguments::common();
            args.push<uint16_t>(options.scenarioStartYear);

            self.widgets[widx::heightMapBox].text = generatorIds[enumValue(options.generator)];

            switch (options.generator)
            {
                case S5::LandGeneratorType::Original:
                {
                    self.enabledWidgets |= (1 << widx::change_heightmap_btn);
                    self.enabledWidgets &= ~((1 << widx::terrainSmoothingNum) | (1 << widx::terrainSmoothingNumUp) | (1 << widx::terrainSmoothingNumDown));
                    self.enabledWidgets &= ~(1 << widx::heightmap_poc);

                    self.widgets[widx::change_heightmap_btn].type = WidgetType::button;
                    self.widgets[widx::terrainSmoothingNum].type = WidgetType::none;
                    self.widgets[widx::terrainSmoothingNumUp].type = WidgetType::none;
                    self.widgets[widx::terrainSmoothingNumDown].type = WidgetType::none;
                    self.widgets[widx::heightmap_poc].type = WidgetType::none;
                    break;
                }

                case S5::LandGeneratorType::Simplex:
                {
                    self.enabledWidgets &= ~(1 << widx::change_heightmap_btn);
                    self.enabledWidgets |= ((1 << widx::terrainSmoothingNum) | (1 << widx::terrainSmoothingNumUp) | (1 << widx::terrainSmoothingNumDown));
                    self.enabledWidgets &= ~(1 << widx::heightmap_poc);

                    self.widgets[widx::change_heightmap_btn].type = WidgetType::none;
                    self.widgets[widx::terrainSmoothingNum].type = WidgetType::combobox;
                    self.widgets[widx::terrainSmoothingNumUp].type = WidgetType::button;
                    self.widgets[widx::terrainSmoothingNumDown].type = WidgetType::button;
                    self.widgets[widx::heightmap_poc].type = WidgetType::none;
                    break;
                }

                case S5::LandGeneratorType::PngHeightMap:
                {
                    self.enabledWidgets &= ~(1 << widx::change_heightmap_btn);
                    self.enabledWidgets &= ~((1 << widx::terrainSmoothingNum) | (1 << widx::terrainSmoothingNumUp) | (1 << widx::terrainSmoothingNumDown));
                    self.enabledWidgets |= (1 << widx::heightmap_poc);

                    self.widgets[widx::change_heightmap_btn].type = WidgetType::none;
                    self.widgets[widx::terrainSmoothingNum].type = WidgetType::none;
                    self.widgets[widx::terrainSmoothingNumUp].type = WidgetType::none;
                    self.widgets[widx::terrainSmoothingNumDown].type = WidgetType::none;
                    self.widgets[widx::heightmap_poc].type = WidgetType::button;
                    break;
                }
            }

            if (options.generator == S5::LandGeneratorType::PngHeightMap)
            {
                self.activatedWidgets &= ~(1 << widx::generate_when_game_starts);
                self.disabledWidgets |= (1 << widx::generate_when_game_starts);
                if (!World::MapGenerator::getPngHeightmapPath().empty())
                    self.disabledWidgets |= (1 << widx::generate_now);
                else
                    self.disabledWidgets &= ~(1 << widx::generate_now);
            }
            else if ((options.scenarioFlags & Scenario::ScenarioFlags::landscapeGenerationDone) == Scenario::ScenarioFlags::none)
            {
                self.activatedWidgets |= (1 << widx::generate_when_game_starts);
                self.disabledWidgets &= ~(1 << widx::generate_when_game_starts);
                self.disabledWidgets |= (1 << widx::generate_now);
            }
            else
            {
                self.activatedWidgets &= ~(1 << widx::generate_when_game_starts);
                self.disabledWidgets &= ~((1 << widx::generate_now) | (1 << widx::generate_when_game_starts));
            }
        }

        static void confirmResetLandscape(int32_t promptType)
        {
            if (S5::getOptions().madeAnyChanges)
            {
                LandscapeGenerationConfirm::open(promptType);
            }
            else
            {
                WindowManager::close(WindowType::landscapeGenerationConfirm, 0);

                if (promptType == 0)
                    Scenario::generateLandscape();
                else
                    Scenario::eraseLandscape();
            }
        }

        // 0x0043E1BA
        static void onDropdown(Window& window, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::heightMapDropdown:
                    if (itemIndex != -1)
                    {
                        S5::getOptions().generator = static_cast<S5::LandGeneratorType>(itemIndex);
                        window.invalidate();
                    }
                    break;
            }
        }

        // 0x0043DC83
        static void onMouseDown(Window& window, WidgetIndex_t widgetIndex)
        {
            auto& options = S5::getOptions();

            switch (widgetIndex)
            {
                case widx::start_year_up:
                    if (options.scenarioStartYear + 1 <= Scenario::kMaxYear)
                        options.scenarioStartYear += 1;
                    window.invalidate();
                    break;

                case widx::start_year_down:
                    if (options.scenarioStartYear - 1 >= Scenario::kMinYear)
                        options.scenarioStartYear -= 1;
                    window.invalidate();
                    break;

                case widx::terrainSmoothingNumUp:
                    options.numTerrainSmoothingPasses = std::clamp(options.numTerrainSmoothingPasses + 1, 1, 5);
                    window.invalidate();
                    break;

                case widx::terrainSmoothingNumDown:
                    options.numTerrainSmoothingPasses = std::clamp(options.numTerrainSmoothingPasses - 1, 1, 5);
                    window.invalidate();
                    break;

                case widx::heightMapDropdown:
                {
                    Widget& target = window.widgets[widx::heightMapBox];
                    Dropdown::show(window.x + target.left, window.y + target.top, target.width() - 4, target.height(), window.getColour(WindowColour::secondary), std::size(generatorIds), 0x80);

                    for (size_t i = 0; i < std::size(generatorIds); i++)
                        Dropdown::add(i, generatorIds[i]);

                    Dropdown::setHighlightedItem(static_cast<uint8_t>(options.generator));
                    break;
                }
            }
        }

        // 0x0043DC58
        static void onMouseUp(Window& window, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&window);
                    break;

                case Common::widx::tab_options:
                case Common::widx::tab_land:
                case Common::widx::tab_water:
                case Common::widx::tab_forests:
                case Common::widx::tab_towns:
                case Common::widx::tab_industries:
                    Common::switchTab(&window, widgetIndex);
                    break;

                case widx::generate_when_game_starts:
                    if ((S5::getOptions().scenarioFlags & Scenario::ScenarioFlags::landscapeGenerationDone) == Scenario::ScenarioFlags::none)
                    {
                        S5::getOptions().scenarioFlags |= Scenario::ScenarioFlags::landscapeGenerationDone;
                        Scenario::generateLandscape();
                    }
                    else
                    {
                        WindowManager::closeConstructionWindows();
                        confirmResetLandscape(1);
                    }
                    break;

                case widx::change_heightmap_btn:
                    EditorController::goToPreviousStep();
                    ObjectSelectionWindow::openInTab(ObjectType::hillShapes);
                    break;

                case widx::heightmap_poc:
                {
                    fs::path basePath = Environment::getPath(Environment::PathId::userHome);

                    static loco_global<char[512], 0x0112CE04> _savePath;
                    strncpy(&_savePath[0], basePath.make_preferred().u8string().c_str(), std::size(_savePath));

                    // NB: this is a proof-of-concept
                    // TODO: make named constant for filter
                    const auto browseType = Ui::Windows::PromptBrowse::browse_type::load;
                    if (Game::openBrowsePrompt(StringIds::title_load_png_heightmap_file, browseType, "*.png"))
                    {
                        World::MapGenerator::setPngHeightmapPath(fs::u8path(&*_savePath));
                        Logging::info("Selected height map: {}", &*_savePath);
                    }
                    else
                    {
                        Logging::info("Height map browser aborted");
                    }
                    break;
                }

                case widx::generate_now:
                    confirmResetLandscape(0);
                    break;
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = Common::update,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 0x0043DA43
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::landscapeGeneration, 0);
        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
                ToolManager::toolCancel();

            window = WindowManager::bringToFront(WindowType::landscapeGeneration, 0);
        }

        // Start of 0x0043DAEA
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(WindowType::landscapeGeneration, kWindowSize, WindowFlags::none, Options::getEvents());
            window->widgets = Options::widgets;
            window->enabledWidgets = Options::enabled_widgets;
            window->number = 0;
            window->currentTab = 0;
            window->frameNo = 0;
            window->rowHover = -1;

            auto interface = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::primary, interface->colour_0B);
            window->setColour(WindowColour::secondary, interface->colour_0E);
        }
        // End of 0x0043DAEA

        window->width = kWindowSize.width;
        window->height = kWindowSize.height;

        window->invalidate();

        window->activatedWidgets = 0;
        window->holdableWidgets = Options::holdable_widgets;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        return window;
    }

    namespace Land
    {
        enum widx
        {
            topography_style = Common::widx::tab_industries + 1,
            topography_style_btn,
            min_land_height,
            min_land_height_down,
            min_land_height_up,
            hill_density,
            hill_density_down,
            hill_density_up,
            hillsEdgeOfMap,
            scrollview,
        };

        const uint64_t enabled_widgets = Common::enabled_widgets | (1 << widx::min_land_height_up) | (1 << widx::min_land_height_down) | (1 << widx::topography_style) | (1 << widx::topography_style_btn) | (1 << widx::hill_density_up) | (1 << widx::hill_density_down) | (1 << widx::hillsEdgeOfMap);
        const uint64_t holdable_widgets = (1 << widx::min_land_height_up) | (1 << widx::min_land_height_down) | (1 << widx::hill_density_up) | (1 << widx::hill_density_down);

        static Widget widgets[] = {
            common_options_widgets(247, StringIds::title_landscape_generation_land),
            makeDropdownWidgets({ 176, 52 }, { 180, 12 }, WidgetType::combobox, WindowColour::secondary),
            makeStepperWidgets({ 256, 68 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::min_land_height_units),
            makeStepperWidgets({ 256, 84 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::hill_density_percent),
            makeWidget({ 10, 100 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::create_hills_right_up_to_edge_of_map),
            makeWidget({ 4, 116 }, { 358, 126 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
            widgetEnd()
        };

        // 0x0043DF89
        static void draw(Window& window, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            Common::draw(window, rt);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::min_land_height].top,
                Colour::black,
                StringIds::min_land_height);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::topography_style].top,
                Colour::black,
                StringIds::topography_style);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::hill_density].top,
                Colour::black,
                StringIds::hill_density);
        }

        static const StringId landDistributionLabelIds[] = {
            StringIds::land_distribution_everywhere,
            StringIds::land_distribution_nowhere,
            StringIds::land_distribution_far_from_water,
            StringIds::land_distribution_near_water,
            StringIds::land_distribution_on_mountains,
            StringIds::land_distribution_far_from_mountains,
            StringIds::land_distribution_in_small_random_areas,
            StringIds::land_distribution_in_large_random_areas,
            StringIds::land_distribution_around_cliffs,
        };

        // 0x0043E01C
        static void drawScroll(Ui::Window& window, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            uint16_t yPos = 0;
            for (uint16_t i = 0; i < kMaxLandObjects; i++)
            {
                if (yPos + kRowHeight < rt.y)
                {
                    yPos += kRowHeight;
                    continue;
                }
                else if (yPos > rt.y + rt.height)
                {
                    break;
                }

                auto landObject = ObjectManager::get<LandObject>(i);
                if (landObject == nullptr)
                    continue;

                // Draw tile icon.
                const uint32_t imageId = landObject->mapPixelImage + OpenLoco::Land::ImageIds::landscape_generator_tile_icon;
                drawingCtx.drawImage(&rt, 2, yPos + 1, imageId);

                // Draw land description.
                {
                    FormatArguments args{};
                    args.push(landObject->name);
                    drawingCtx.drawStringLeftClipped(rt, 24, yPos + 5, 121, Colour::black, StringIds::wcolour2_stringid, &args);
                }

                // Draw rectangle.
                drawingCtx.fillRectInset(rt, 150, yPos + 5, 340, yPos + 16, window.getColour(WindowColour::secondary), Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillDarker);

                // Draw current distribution setting.
                {
                    FormatArguments args{};
                    const StringId distributionId = landDistributionLabelIds[enumValue(S5::getOptions().landDistributionPatterns[i])];
                    args.push(distributionId);
                    drawingCtx.drawStringLeftClipped(rt, 151, yPos + 5, 177, Colour::black, StringIds::black_stringid, &args);
                }

                // Draw rectangle (knob).
                const Drawing::RectInsetFlags flags = window.rowHover == i ? Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillDarker : Drawing::RectInsetFlags::none;
                drawingCtx.fillRectInset(rt, 329, yPos + 6, 339, yPos + 15, window.getColour(WindowColour::secondary), flags);

                // Draw triangle (knob).
                drawingCtx.drawStringLeft(rt, 330, yPos + 6, Colour::black, StringIds::dropdown, nullptr);

                yPos += kRowHeight;
            }
        }

        // 0x0043E2AC
        static void getScrollSize([[maybe_unused]] Ui::Window& window, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = 0;

            for (uint16_t i = 0; i < kMaxLandObjects; i++)
            {
                auto landObject = ObjectManager::get<LandObject>(i);
                if (landObject == nullptr)
                    continue;

                *scrollHeight += kRowHeight;
            }
        }

        static const StringId topographyStyleIds[] = {
            StringIds::flat_land,
            StringIds::small_hills,
            StringIds::mountains,
            StringIds::half_mountains_half_hills,
            StringIds::half_mountains_half_flat,
        };

        // 0x0043E1BA
        static void onDropdown(Window& window, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::topography_style_btn:
                    if (itemIndex != -1)
                    {
                        S5::getOptions().topographyStyle = static_cast<S5::TopographyStyle>(itemIndex);
                        window.invalidate();
                    }
                    break;

                case widx::scrollview:
                    if (itemIndex != -1 && window.rowHover != -1)
                    {
                        S5::getOptions().landDistributionPatterns[window.rowHover] = static_cast<S5::LandDistributionPattern>(itemIndex);
                        window.invalidate();
                    }
                    break;
            }
        }

        // 0x0043E173
        static void onMouseDown(Window& window, WidgetIndex_t widgetIndex)
        {
            auto& options = S5::getOptions();

            switch (widgetIndex)
            {
                case widx::min_land_height_up:
                    options.minLandHeight = std::min<int8_t>(options.minLandHeight + 1, Scenario::kMaxBaseLandHeight);
                    break;

                case widx::min_land_height_down:
                    options.minLandHeight = std::max<int8_t>(Scenario::kMinBaseLandHeight, options.minLandHeight - 1);
                    break;

                case widx::topography_style_btn:
                {
                    Widget& target = window.widgets[widx::topography_style];
                    Dropdown::show(window.x + target.left, window.y + target.top, target.width() - 4, target.height(), window.getColour(WindowColour::secondary), std::size(topographyStyleIds), 0x80);

                    for (size_t i = 0; i < std::size(topographyStyleIds); i++)
                        Dropdown::add(i, topographyStyleIds[i]);

                    Dropdown::setHighlightedItem(static_cast<uint8_t>(options.topographyStyle));
                    break;
                }

                case widx::hill_density_up:
                    options.hillDensity = std::min<int8_t>(options.hillDensity + 1, Scenario::kMaxHillDensity);
                    break;

                case widx::hill_density_down:
                    options.hillDensity = std::max<int8_t>(Scenario::kMinHillDensity, options.hillDensity - 1);
                    break;

                default:
                    // Nothing was changed, don't invalidate.
                    return;
            }

            // After changing any of the options, invalidate the window.
            window.invalidate();
        }

        // 0x0043E14E
        static void onMouseUp(Window& window, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&window);
                    break;

                case Common::widx::tab_options:
                case Common::widx::tab_land:
                case Common::widx::tab_water:
                case Common::widx::tab_forests:
                case Common::widx::tab_towns:
                case Common::widx::tab_industries:
                    Common::switchTab(&window, widgetIndex);
                    break;

                case widx::hillsEdgeOfMap:
                    S5::getOptions().scenarioFlags ^= Scenario::ScenarioFlags::hillsEdgeOfMap;
                    window.invalidate();
                    break;
            }
        }

        // 0x0043E421
        static int16_t scrollPosToLandIndex(int16_t xPos, int16_t yPos)
        {
            if (xPos < 150)
                return -1;

            for (uint16_t i = 0; i < kMaxLandObjects; i++)
            {
                auto landObject = ObjectManager::get<LandObject>(i);
                if (landObject == nullptr)
                    continue;

                yPos -= kRowHeight;
                if (yPos < 0)
                    return i;
            }

            return -1;
        }

        // 0x0043E1CF
        static void scrollMouseDown(Window& window, int16_t xPos, int16_t yPos, [[maybe_unused]] uint8_t scrollIndex)
        {
            int16_t landIndex = scrollPosToLandIndex(xPos, yPos);
            if (landIndex == -1)
                return;

            window.rowHover = landIndex;

            Audio::playSound(Audio::SoundId::clickDown, window.widgets[widx::scrollview].right);

            const Widget& target = window.widgets[widx::scrollview];
            const int16_t dropdownX = window.x + target.left + 151;
            const int16_t dropdownY = window.y + target.top + 6 + landIndex * kRowHeight - window.scrollAreas[0].contentOffsetY;
            Dropdown::show(dropdownX, dropdownY, 188, 12, window.getColour(WindowColour::secondary), std::size(landDistributionLabelIds), 0x80);

            for (size_t i = 0; i < std::size(landDistributionLabelIds); i++)
                Dropdown::add(i, StringIds::dropdown_stringid, landDistributionLabelIds[i]);

            Dropdown::setItemSelected(enumValue(S5::getOptions().landDistributionPatterns[landIndex]));
        }

        // 0x0043DEBF
        static void prepareDraw(Window& window)
        {
            Common::prepareDraw(window);

            auto args = FormatArguments::common();
            auto& options = S5::getOptions();
            args.push<uint16_t>(options.minLandHeight);
            args.push<uint16_t>(options.hillDensity);

            window.widgets[widx::topography_style].text = topographyStyleIds[static_cast<uint8_t>(options.topographyStyle)];

            if ((options.scenarioFlags & Scenario::ScenarioFlags::hillsEdgeOfMap) != Scenario::ScenarioFlags::none)
                window.activatedWidgets |= (1 << widx::hillsEdgeOfMap);
            else
                window.activatedWidgets &= ~(1 << widx::hillsEdgeOfMap);
        }

        // 0x0043E2A2
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_list);
            return args;
        }

        // 0x0043E3D9
        static void update(Window& window)
        {
            Common::update(window);

            auto dropdown = WindowManager::find(WindowType::dropdown, 0);
            if (dropdown == nullptr && window.rowHover != -1)
            {
                window.rowHover = -1;
                window.invalidate();
            }
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = update,
            .getScrollSize = getScrollSize,
            .scrollMouseDown = scrollMouseDown,
            .tooltip = tooltip,
            .prepareDraw = prepareDraw,
            .draw = draw,
            .drawScroll = drawScroll,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Water
    {
        enum widx
        {
            sea_level = Common::widx::tab_industries + 1,
            sea_level_down,
            sea_level_up,
        };

        const uint64_t enabled_widgets = Common::enabled_widgets | (1 << widx::sea_level_up) | (1 << widx::sea_level_down);
        const uint64_t holdable_widgets = (1 << widx::sea_level_up) | (1 << widx::sea_level_down);

        static Widget widgets[] = {
            common_options_widgets(247, StringIds::title_landscape_generation_water),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::sea_level_units),
            widgetEnd()
        };

        // 0x0043DF89
        static void draw(Window& window, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            Common::draw(window, rt);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::sea_level].top,
                Colour::black,
                StringIds::sea_level);
        }

        // 0x0043E173
        static void onMouseDown(Window& window, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::sea_level_up:
                    getGameState().seaLevel = std::min<int8_t>(getGameState().seaLevel + 1, Scenario::kMaxSeaLevel);
                    break;

                case widx::sea_level_down:
                    getGameState().seaLevel = std::max<int8_t>(Scenario::kMinSeaLevel, getGameState().seaLevel - 1);
                    break;

                default:
                    // Nothing was changed, don't invalidate.
                    return;
            }

            // After changing any of the options, invalidate the window.
            window.invalidate();
        }

        // 0x0043E14E
        static void onMouseUp(Window& window, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&window);
                    break;

                case Common::widx::tab_options:
                case Common::widx::tab_land:
                case Common::widx::tab_water:
                case Common::widx::tab_forests:
                case Common::widx::tab_towns:
                case Common::widx::tab_industries:
                    Common::switchTab(&window, widgetIndex);
                    break;
            }
        }

        // 0x0043DEBF
        static void prepareDraw(Window& window)
        {
            Common::prepareDraw(window);

            auto args = FormatArguments::common();
            args.push(getGameState().seaLevel);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onUpdate = Common::update,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Forests
    {
        enum widx
        {
            number_of_forests = Common::widx::tab_industries + 1,
            number_of_forests_down,
            number_of_forests_up,
            minForestRadius,
            min_forest_radius_down,
            min_forest_radius_up,
            maxForestRadius,
            max_forest_radius_down,
            max_forest_radius_up,
            minForestDensity,
            min_forest_density_down,
            min_forest_density_up,
            maxForestDensity,
            max_forest_density_down,
            max_forest_density_up,
            number_random_trees,
            number_random_trees_down,
            number_random_trees_up,
            min_altitude_for_trees,
            min_altitude_for_trees_down,
            min_altitude_for_trees_up,
            max_altitude_for_trees,
            max_altitude_for_trees_down,
            max_altitude_for_trees_up,
        };

        const uint64_t enabled_widgets = Common::enabled_widgets | (1ULL << widx::number_of_forests_up) | (1ULL << widx::number_of_forests_down) | (1ULL << widx::min_forest_radius_up) | (1ULL << widx::min_forest_radius_down) | (1ULL << widx::max_forest_radius_up) | (1ULL << widx::max_forest_radius_down) | (1ULL << widx::min_forest_density_up) | (1ULL << widx::min_forest_density_down) | (1 << widx::max_forest_density_up) | (1ULL << widx::max_forest_density_down) | (1ULL << widx::number_random_trees_up) | (1ULL << widx::number_random_trees_down) | (1ULL << widx::min_altitude_for_trees_up) | (1ULL << widx::min_altitude_for_trees_down) | (1ULL << widx::max_altitude_for_trees_down) | (1ULL << widx::max_altitude_for_trees_up);
        const uint64_t holdable_widgets = (1ULL << widx::number_of_forests_up) | (1ULL << widx::number_of_forests_down) | (1ULL << widx::min_forest_radius_up) | (1ULL << widx::min_forest_radius_down) | (1ULL << widx::max_forest_radius_up) | (1ULL << widx::max_forest_radius_down) | (1ULL << widx::min_forest_density_up) | (1ULL << widx::min_forest_density_down) | (1ULL << widx::max_forest_density_up) | (1 << widx::max_forest_density_down) | (1ULL << widx::number_random_trees_up) | (1ULL << widx::number_random_trees_down) | (1ULL << widx::min_altitude_for_trees_up) | (1ULL << widx::min_altitude_for_trees_down) | (1ULL << widx::max_altitude_for_trees_down) | (1ULL << widx::max_altitude_for_trees_up);

        static Widget widgets[] = {
            common_options_widgets(217, StringIds::title_landscape_generation_forests),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::number_of_forests_value),
            makeStepperWidgets({ 256, 67 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::min_forest_radius_blocks),
            makeStepperWidgets({ 256, 82 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::max_forest_radius_blocks),
            makeStepperWidgets({ 256, 97 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::min_forest_density_percent),
            makeStepperWidgets({ 256, 112 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::max_forest_density_percent),
            makeStepperWidgets({ 256, 127 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::number_random_trees_value),
            makeStepperWidgets({ 256, 142 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::min_altitude_for_trees_height),
            makeStepperWidgets({ 256, 157 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::max_altitude_for_trees_height),
            widgetEnd()
        };

        // 0x0043E53A
        static void draw(Window& window, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            Common::draw(window, rt);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::number_of_forests].top,
                Colour::black,
                StringIds::number_of_forests);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::minForestRadius].top,
                Colour::black,
                StringIds::min_forest_radius);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::maxForestRadius].top,
                Colour::black,
                StringIds::max_forest_radius);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::minForestDensity].top,
                Colour::black,
                StringIds::min_forest_density);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::maxForestDensity].top,
                Colour::black,
                StringIds::max_forest_density);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::number_random_trees].top,
                Colour::black,
                StringIds::number_random_trees);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::min_altitude_for_trees].top,
                Colour::black,
                StringIds::min_altitude_for_trees);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::max_altitude_for_trees].top,
                Colour::black,
                StringIds::max_altitude_for_trees);
        }

        // 0x0043E670
        static void onMouseDown(Window& window, WidgetIndex_t widgetIndex)
        {
            auto& options = S5::getOptions();

            switch (widgetIndex)
            {
                case widx::number_of_forests_up:
                {
                    options.numberOfForests = std::min<int16_t>(options.numberOfForests + 10, Scenario::kMaxNumForests);
                    break;
                }
                case widx::number_of_forests_down:
                {
                    options.numberOfForests = std::max<int16_t>(Scenario::kMinNumForests, options.numberOfForests - 10);
                    break;
                }
                case widx::min_forest_radius_up:
                {
                    options.minForestRadius = std::min<int16_t>(options.minForestRadius + 1, Scenario::kMaxForestRadius);
                    if (options.minForestRadius > options.maxForestRadius)
                        options.maxForestRadius = options.minForestRadius;
                    break;
                }
                case widx::min_forest_radius_down:
                {
                    options.minForestRadius = std::max<int8_t>(Scenario::kMinForestRadius, options.minForestRadius - 1);
                    break;
                }
                case widx::max_forest_radius_up:
                {
                    options.maxForestRadius = std::clamp<int8_t>(options.maxForestRadius + 1, Scenario::kMinForestRadius, Scenario::kMaxForestRadius);
                    break;
                }
                case widx::max_forest_radius_down:
                {
                    options.maxForestRadius = std::clamp<int8_t>(options.maxForestRadius - 1, Scenario::kMinForestRadius, Scenario::kMaxForestRadius);
                    if (options.maxForestRadius < options.minForestRadius)
                        options.minForestRadius = options.maxForestRadius;
                    break;
                }
                case widx::min_forest_density_up:
                {
                    options.minForestDensity = std::min<int8_t>(options.minForestDensity + 1, Scenario::kMaxForestDensity);
                    if (options.minForestDensity > options.maxForestDensity)
                        options.maxForestDensity = options.minForestDensity;
                    break;
                }
                case widx::min_forest_density_down:
                {
                    options.minForestDensity = std::max<int8_t>(Scenario::kMinForestDensity, options.minForestDensity - 1);
                    break;
                }
                case widx::max_forest_density_up:
                {
                    options.maxForestDensity = std::min<int8_t>(options.maxForestDensity + 1, Scenario::kMaxForestDensity);
                    break;
                }
                case widx::max_forest_density_down:
                {
                    options.maxForestDensity = std::max<int8_t>(Scenario::kMinForestDensity, options.maxForestDensity - 1);
                    if (options.maxForestDensity < options.minForestDensity)
                        options.minForestDensity = options.maxForestDensity;
                    break;
                }
                case widx::number_random_trees_up:
                {
                    options.numberRandomTrees = std::min<int16_t>(options.numberRandomTrees + 25, Scenario::kMaxNumTrees);
                    break;
                }
                case widx::number_random_trees_down:
                {
                    options.numberRandomTrees = std::max<int16_t>(Scenario::kMinNumTrees, options.numberRandomTrees - 25);
                    break;
                }
                case widx::min_altitude_for_trees_up:
                {
                    options.minAltitudeForTrees = std::min<int8_t>(options.minAltitudeForTrees + 1, Scenario::kMaxAltitudeTrees);
                    if (options.minAltitudeForTrees > options.maxAltitudeForTrees)
                        options.maxAltitudeForTrees = options.minAltitudeForTrees;
                    break;
                }
                case widx::min_altitude_for_trees_down:
                {
                    options.minAltitudeForTrees = std::max<int8_t>(Scenario::kMinAltitudeTrees, options.minAltitudeForTrees - 1);
                    break;
                }
                case widx::max_altitude_for_trees_up:
                {
                    options.maxAltitudeForTrees = std::min<int8_t>(options.maxAltitudeForTrees + 1, Scenario::kMaxAltitudeTrees);
                    break;
                }
                case widx::max_altitude_for_trees_down:
                {
                    options.maxAltitudeForTrees = std::max<int8_t>(Scenario::kMinAltitudeTrees, options.maxAltitudeForTrees - 1);
                    if (options.maxAltitudeForTrees < options.minAltitudeForTrees)
                        options.minAltitudeForTrees = options.maxAltitudeForTrees;
                    break;
                }
                default:
                    // Nothing was changed, don't invalidate.
                    return;
            }

            // After changing any of the options, invalidate the window.
            window.invalidate();
        }

        // 0x0043E655
        static void onMouseUp(Window& window, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&window);
                    break;

                case Common::widx::tab_options:
                case Common::widx::tab_land:
                case Common::widx::tab_water:
                case Common::widx::tab_forests:
                case Common::widx::tab_towns:
                case Common::widx::tab_industries:
                    Common::switchTab(&window, widgetIndex);
                    break;
            }
        }

        // 0x0043E44F
        static void prepareDraw(Window& window)
        {
            Common::prepareDraw(window);

            auto& options = S5::getOptions();
            auto args = FormatArguments::common();

            args.push<uint16_t>(options.numberOfForests);
            args.push<uint16_t>(options.minForestRadius);
            args.push<uint16_t>(options.maxForestRadius);
            args.push<uint16_t>(options.minForestDensity * 14);
            args.push<uint16_t>(options.maxForestDensity * 14);
            args.push<uint16_t>(options.numberRandomTrees);
            args.push<uint16_t>(options.minAltitudeForTrees);
            args.push<uint16_t>(options.maxAltitudeForTrees);
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onUpdate = Common::update,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Towns
    {
        enum widx
        {
            number_of_towns = Common::widx::tab_industries + 1,
            number_of_towns_down,
            number_of_towns_up,
            max_town_size,
            max_town_size_btn,
        };

        const uint64_t enabled_widgets = Common::enabled_widgets | (1 << widx::number_of_towns_up) | (1 << widx::number_of_towns_down) | (1 << widx::max_town_size) | (1 << widx::max_town_size_btn);
        const uint64_t holdable_widgets = (1 << widx::number_of_towns_up) | (1 << widx::number_of_towns_down);

        static Widget widgets[] = {
            common_options_widgets(217, StringIds::title_landscape_generation_towns),
            makeStepperWidgets({ 256, 52 }, { 100, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::number_of_towns_value),
            makeDropdownWidgets({ 176, 67 }, { 180, 12 }, WidgetType::combobox, WindowColour::secondary),
            widgetEnd()
        };

        // 0x0043E9A3
        static void draw(Window& window, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            Common::draw(window, rt);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::number_of_towns].top,
                Colour::black,
                StringIds::number_of_towns);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::max_town_size].top,
                Colour::black,
                StringIds::max_town_size);
        }

        static const StringId townSizeLabels[] = {
            StringIds::town_size_1,
            StringIds::town_size_2,
            StringIds::town_size_3,
            StringIds::town_size_4,
            StringIds::town_size_5,
            StringIds::town_size_6,
            StringIds::town_size_7,
            StringIds::town_size_8,
        };

        // 0x0043EA28
        static void onDropdown(Window& window, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::max_town_size_btn || itemIndex == -1)
                return;

            S5::getOptions().maxTownSize = itemIndex + 1;
            window.invalidate();
        }

        // 0x0043EA0D
        static void onMouseDown(Window& window, WidgetIndex_t widgetIndex)
        {
            auto& options = S5::getOptions();

            switch (widgetIndex)
            {
                case widx::number_of_towns_up:
                {
                    uint16_t newNumTowns = std::min<uint16_t>(options.numberOfTowns + 1, Limits::kMaxTowns);
                    options.numberOfTowns = newNumTowns;
                    window.invalidate();
                    break;
                }

                case widx::number_of_towns_down:
                {
                    // Vanilla behaviour: Zero-town map generation is allowed in the scenario editor and
                    // is checked on the editor stage progression for non-zero. It is required to be non-zero
                    // for gameplay since industries must have at least one associated town. The user must
                    // manually place at least one town if they generate a landscape with zero towns.
                    if (options.numberOfTowns > 0)
                    {
                        uint16_t newNumTowns = std::max<uint16_t>(Limits::kMinTowns - 1, options.numberOfTowns - 1);
                        options.numberOfTowns = newNumTowns;
                    }
                    window.invalidate();
                    break;
                }

                case widx::max_town_size_btn:
                {
                    Widget& target = window.widgets[widx::max_town_size];
                    Dropdown::show(window.x + target.left, window.y + target.top, target.width() - 4, target.height(), window.getColour(WindowColour::secondary), std::size(townSizeLabels), 0x80);

                    for (size_t i = 0; i < std::size(townSizeLabels); i++)
                        Dropdown::add(i, townSizeLabels[i]);

                    Dropdown::setHighlightedItem(options.maxTownSize - 1);
                    break;
                }
            }
        }

        // 0x0043EBCA
        static void onMouseUp(Window& window, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&window);
                    break;

                case Common::widx::tab_options:
                case Common::widx::tab_land:
                case Common::widx::tab_water:
                case Common::widx::tab_forests:
                case Common::widx::tab_towns:
                case Common::widx::tab_industries:
                    Common::switchTab(&window, widgetIndex);
                    break;
            }
        }

        // 0x0043E90D
        static void prepareDraw(Window& window)
        {
            Common::prepareDraw(window);

            auto args = FormatArguments::common();
            args.push<uint16_t>(S5::getOptions().numberOfTowns);

            widgets[widx::max_town_size].text = townSizeLabels[S5::getOptions().maxTownSize - 1];
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = Common::update,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Industries
    {
        enum widx
        {
            num_industries = Common::widx::tab_industries + 1,
            num_industries_btn,
            check_allow_industries_close_down,
            check_allow_industries_start_up,
        };

        const uint64_t enabled_widgets = Common::enabled_widgets | (1 << widx::num_industries) | (1 << widx::num_industries_btn) | (1 << widx::check_allow_industries_close_down) | (1 << widx::check_allow_industries_start_up);
        const uint64_t holdable_widgets = 0;

        static Widget widgets[] = {
            common_options_widgets(217, StringIds::title_landscape_generation_industries),
            makeDropdownWidgets({ 176, 52 }, { 180, 12 }, WidgetType::combobox, WindowColour::secondary),
            makeWidget({ 10, 68 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::allow_industries_to_close_down_during_game),
            makeWidget({ 10, 83 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::allow_new_industries_to_start_up_during_game),
            widgetEnd()
        };

        // 0x0043EB9D
        static void draw(Window& window, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            Common::draw(window, rt);

            drawingCtx.drawStringLeft(
                *rt,
                window.x + 10,
                window.y + window.widgets[widx::num_industries].top,
                Colour::black,
                StringIds::number_of_industries);
        }

        static const StringId numIndustriesLabels[] = {
            StringIds::industry_size_low,
            StringIds::industry_size_medium,
            StringIds::industry_size_high,
        };

        // 0x0043EBF8
        static void onDropdown(Window& window, WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::num_industries_btn || itemIndex == -1)
                return;

            S5::getOptions().numberOfIndustries = itemIndex;
            window.invalidate();
        }

        // 0x0043EBF1
        static void onMouseDown(Window& window, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex != widx::num_industries_btn)
                return;

            Widget& target = window.widgets[widx::num_industries];
            Dropdown::show(window.x + target.left, window.y + target.top, target.width() - 4, target.height(), window.getColour(WindowColour::secondary), std::size(numIndustriesLabels), 0x80);

            for (size_t i = 0; i < std::size(numIndustriesLabels); i++)
                Dropdown::add(i, numIndustriesLabels[i]);

            Dropdown::setHighlightedItem(S5::getOptions().numberOfIndustries);
        }

        // 0x0043EBCA
        static void onMouseUp(Window& window, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&window);
                    break;

                case Common::widx::tab_options:
                case Common::widx::tab_land:
                case Common::widx::tab_water:
                case Common::widx::tab_forests:
                case Common::widx::tab_towns:
                case Common::widx::tab_industries:
                    Common::switchTab(&window, widgetIndex);
                    break;

                case widx::check_allow_industries_close_down:
                    IndustryManager::setFlags(IndustryManager::getFlags() ^ IndustryManager::Flags::disallowIndustriesCloseDown);
                    window.invalidate();
                    break;

                case widx::check_allow_industries_start_up:
                    IndustryManager::setFlags(IndustryManager::getFlags() ^ IndustryManager::Flags::disallowIndustriesStartUp);
                    window.invalidate();
                    break;
            }
        }

        // 0x0043EAEB
        static void prepareDraw(Window& window)
        {
            Common::prepareDraw(window);

            widgets[widx::num_industries].text = numIndustriesLabels[S5::getOptions().numberOfIndustries];
            window.activatedWidgets &= ~((1 << widx::check_allow_industries_close_down) | (1 << widx::check_allow_industries_start_up));
            if (!IndustryManager::hasFlags(IndustryManager::Flags::disallowIndustriesCloseDown))
                window.activatedWidgets |= 1 << widx::check_allow_industries_close_down;
            if (!IndustryManager::hasFlags(IndustryManager::Flags::disallowIndustriesStartUp))
                window.activatedWidgets |= 1 << widx::check_allow_industries_start_up;
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = Common::update,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    };

    namespace Common
    {
        static void switchTabWidgets(Window* window)
        {
            window->activatedWidgets = 0;

            static Widget* widgetCollectionsByTabId[] = {
                Options::widgets,
                Land::widgets,
                Water::widgets,
                Forests::widgets,
                Towns::widgets,
                Industries::widgets,
            };

            Widget* newWidgets = widgetCollectionsByTabId[window->currentTab];
            if (window->widgets != newWidgets)
            {
                window->widgets = newWidgets;
                window->initScrollWidgets();
            }

            static const widx tabWidgetIdxByTabId[] = {
                tab_options,
                tab_land,
                tab_water,
                tab_forests,
                tab_towns,
                tab_industries,
            };

            window->activatedWidgets &= ~((1 << tab_options) | (1 << tab_land) | (1 << tab_water) | (1 << tab_forests) | (1 << tab_towns) | (1 << tab_industries));
            window->activatedWidgets |= (1ULL << tabWidgetIdxByTabId[window->currentTab]);
        }

        // 0x0043DC98
        static void switchTab(Window* window, WidgetIndex_t widgetIndex)
        {
            if (ToolManager::isToolActive(window->type, window->number))
                ToolManager::toolCancel();

            window->currentTab = widgetIndex - widx::tab_options;
            window->frameNo = 0;
            window->flags &= ~(WindowFlags::flag_16);
            window->disabledWidgets = 0;

            static const uint64_t* enabledWidgetsByTab[] = {
                &Options::enabled_widgets,
                &Land::enabled_widgets,
                &Water::enabled_widgets,
                &Forests::enabled_widgets,
                &Towns::enabled_widgets,
                &Industries::enabled_widgets,
            };

            window->enabledWidgets = *enabledWidgetsByTab[window->currentTab];

            static const uint64_t* holdableWidgetsByTab[] = {
                &Options::holdable_widgets,
                &Land::holdable_widgets,
                &Water::holdable_widgets,
                &Forests::holdable_widgets,
                &Towns::holdable_widgets,
                &Industries::holdable_widgets,
            };

            window->holdableWidgets = *holdableWidgetsByTab[window->currentTab];

            static const WindowEventList* eventsByTab[] = {
                &Options::getEvents(),
                &Land::getEvents(),
                &Water::getEvents(),
                &Forests::getEvents(),
                &Towns::getEvents(),
                &Industries::getEvents(),
            };

            window->eventHandlers = eventsByTab[window->currentTab];

            switchTabWidgets(window);

            window->invalidate();

            const Ui::Size* newSize = &kWindowSize;
            if (widgetIndex == widx::tab_land)
                newSize = &kLandTabSize;

            window->setSize(*newSize);

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();
            window->invalidate();
            window->moveInsideScreenEdges();
        }
    }
}
