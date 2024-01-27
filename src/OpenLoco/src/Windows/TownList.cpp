#include "Audio/Audio.h"
#include "Config.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "GameCommands/Buildings/CreateBuilding.h"
#include "GameCommands/Buildings/RemoveBuilding.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Town/CreateTown.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "LastGameOptionManager.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/MapSelection.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Objects/BuildingObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "SceneManager.h"
#include "Ui/Dropdown.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "World/Town.h"
#include "World/TownManager.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TownList
{
    static loco_global<currency32_t, 0x01135C34> _dword_1135C34;
    static loco_global<bool, 0x01135C60> _buildingGhostPlaced;
    static loco_global<World::Pos3, 0x01135C50> _buildingGhostPos;
    static loco_global<Colour, 0x01135C61> _buildingColour;
    static loco_global<uint8_t, 0x01135C62> _buildingGhostType;
    static loco_global<uint8_t, 0x01135C63> _buildingRotation;
    static loco_global<uint8_t, 0x01135C64> _buildingGhostRotation;
    static loco_global<uint8_t, 0x01135C65> _buildingVariation;
    static loco_global<uint8_t, 0x01135C66> _townSize;

    namespace Common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_town_list,
            tab_build_town,
            tab_build_buildings,
            tab_build_misc_buildings,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_town_list) | (1 << widx::tab_build_town) | (1 << widx::tab_build_buildings) | (1 << widx::tab_build_misc_buildings);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                                      \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                                     \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_25, WindowColour::primary, windowCaptionId),                                                \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 155 }, WidgetType::panel, WindowColour::secondary),                                                                      \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_town_list),                               \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_town),                             \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_buildings),                        \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_misc_buildings)

        static void prepareDraw(Window& self);
        static void drawTabs(Window* self, Gfx::RenderTarget* rt);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void initEvents();
        static void refreshTownList(Window* self);

    }

    namespace TownList
    {
        static constexpr Ui::Size kWindowSize = { 600, 197 };
        static constexpr Ui::Size kMaxDimensions = { 600, 900 };
        static constexpr Ui::Size kMinDimensions = { 192, 100 };

        static constexpr uint8_t kRowHeight = 10;

        enum widx
        {
            sort_town_name = 8,
            sort_town_type,
            sort_town_population,
            sort_town_stations,
            scrollview,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << sort_town_name) | (1 << sort_town_type) | (1 << sort_town_population) | (1 << sort_town_stations) | (1 << scrollview);

        Widget widgets[] = {
            commonWidgets(600, 197, StringIds::title_towns),
            makeWidget({ 4, 43 }, { 200, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_sort_by_name),
            makeWidget({ 204, 43 }, { 80, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_sort_town_type),
            makeWidget({ 284, 43 }, { 70, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_sort_population),
            makeWidget({ 354, 43 }, { 70, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_sort_stations),
            makeWidget({ 3, 56 }, { 594, 126 }, WidgetType::scrollview, WindowColour::secondary, 2),
            widgetEnd(),
        };

        static WindowEventList events;

        enum SortMode : uint16_t
        {
            Name,
            Type,
            Population,
            Stations,
        };

        // 0x00499F53
        static void prepareDraw(Ui::Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 4;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            // Reposition header buttons
            self.widgets[widx::sort_town_name].right = std::min(203, self.width - 8);

            self.widgets[widx::sort_town_type].left = std::min(204, self.width - 8);
            self.widgets[widx::sort_town_type].right = std::min(283, self.width - 8);

            self.widgets[widx::sort_town_population].left = std::min(284, self.width - 8);
            self.widgets[widx::sort_town_population].right = std::min(353, self.width - 8);

            self.widgets[widx::sort_town_stations].left = std::min(354, self.width - 8);
            self.widgets[widx::sort_town_stations].right = std::min(423, self.width - 8);

            // Set header button captions
            self.widgets[widx::sort_town_name].text = self.sortMode == SortMode::Name ? StringIds::table_header_name_desc : StringIds::table_header_name;
            self.widgets[widx::sort_town_type].text = self.sortMode == SortMode::Type ? StringIds::table_header_town_type_desc : StringIds::table_header_town_type;
            self.widgets[widx::sort_town_population].text = self.sortMode == SortMode::Population ? StringIds::table_header_population_desc : StringIds::table_header_population;
            self.widgets[widx::sort_town_stations].text = self.sortMode == SortMode::Stations ? StringIds::table_header_stations_desc : StringIds::table_header_stations;

            Widget::leftAlignTabs(self, Common::widx::tab_town_list, Common::widx::tab_build_misc_buildings);
        }

        // 0x0049A0F8
        static void drawScroll(Ui::Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            drawingCtx.clearSingle(rt, shade);

            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                const auto townId = TownId(self.rowInfo[i]);

                // Skip items outside of view, or irrelevant to the current filter.
                if (yPos + kRowHeight < rt.y || yPos >= yPos + kRowHeight + rt.height || townId == TownId::null)
                {
                    yPos += kRowHeight;
                    continue;
                }

                StringId text_colour_id = StringIds::black_stringid;

                // Highlight selection.
                if (townId == TownId(self.rowHover))
                {
                    drawingCtx.drawRect(rt, 0, yPos, self.width, kRowHeight, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
                    text_colour_id = StringIds::wcolour2_stringid;
                }

                if (townId == TownId::null)
                    continue;
                auto town = TownManager::get(townId);

                // Town Name
                {
                    auto args = FormatArguments();
                    args.push(town->name);

                    drawingCtx.drawStringLeftClipped(rt, 0, yPos, 198, Colour::black, text_colour_id, &args);
                }
                // Town Type
                {
                    auto args = FormatArguments();
                    args.push(town->getTownSizeString());

                    drawingCtx.drawStringLeftClipped(rt, 200, yPos, 278, Colour::black, text_colour_id, &args);
                }
                // Town Population
                {
                    auto args = FormatArguments();
                    args.push(StringIds::int_32);
                    args.push(town->population);

                    drawingCtx.drawStringLeftClipped(rt, 280, yPos, 68, Colour::black, text_colour_id, &args);
                }
                // Town Stations
                {
                    auto args = FormatArguments();
                    args.push(StringIds::int_32);
                    args.push<int32_t>(town->numStations);

                    drawingCtx.drawStringLeftClipped(rt, 350, yPos, 68, Colour::black, text_colour_id, &args);
                }
                yPos += kRowHeight;
            }
        }

        // 0x0049A0A7
        static void draw(Ui::Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);
            auto args = FormatArguments();
            auto xPos = self.x + 4;
            auto yPos = self.y + self.height - 12;

            if (self.var_83C == 1)
                args.push(StringIds::status_towns_singular);
            else
                args.push(StringIds::status_towns_plural);
            args.push(self.var_83C);

            drawingCtx.drawStringLeft(*rt, xPos, yPos, Colour::black, StringIds::black_stringid, &args);
        }

        // 0x0049A27F
        static void onMouseUp(Ui::Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_town_list:
                case Common::widx::tab_build_town:
                case Common::widx::tab_build_buildings:
                case Common::widx::tab_build_misc_buildings:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::sort_town_name:
                case widx::sort_town_type:
                case widx::sort_town_population:
                case widx::sort_town_stations:
                {
                    auto sortMode = widgetIndex - widx::sort_town_name;
                    if (self.sortMode == sortMode)
                        return;

                    self.sortMode = sortMode;
                    self.invalidate();
                    self.var_83C = 0;
                    self.rowHover = -1;

                    Common::refreshTownList(&self);
                    break;
                }
            }
        }

        // 0x0049A56D
        static void onScrollMouseDown(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            uint16_t currentRow = y / kRowHeight;
            if (currentRow > self.var_83C)
                return;

            int16_t currentTown = self.rowInfo[currentRow];
            if (currentTown == -1)
                return;

            Town::open(currentTown);
        }

        // 0x0049A532
        static void onScrollMouseOver(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            self.flags &= ~(WindowFlags::notScrollView);

            uint16_t currentRow = y / kRowHeight;
            int16_t currentTown = -1;

            if (currentRow < self.var_83C)
                currentTown = self.rowInfo[currentRow];

            if (self.rowHover == currentTown)
                return;

            self.rowHover = currentTown;
            self.invalidate();
        }

        // 0x00499EC9
        static bool orderByName(const OpenLoco::Town& lhs, const OpenLoco::Town& rhs)
        {
            char lhsString[256] = { 0 };
            StringManager::formatString(lhsString, lhs.name);

            char rhsString[256] = { 0 };
            StringManager::formatString(rhsString, rhs.name);

            return strcmp(lhsString, rhsString) < 0;
        }

        // 0x00499F28
        static bool orderByPopulation(const OpenLoco::Town& lhs, const OpenLoco::Town& rhs)
        {
            auto lhsPopulation = lhs.population;

            auto rhsPopulation = rhs.population;

            return rhsPopulation < lhsPopulation;
        }

        // 0x00499F0A Left this in to match the x86 code. can be replaced with orderByPopulation
        static bool orderByType(const OpenLoco::Town& lhs, const OpenLoco::Town& rhs)
        {
            auto lhsSize = lhs.size;

            auto rhsSize = rhs.size;

            if (rhsSize != lhsSize)
            {
                return rhsSize < lhsSize;
            }
            else
            {
                return orderByPopulation(lhs, rhs);
            }
        }

        // 0x00499F3B
        static bool orderByStations(const OpenLoco::Town& lhs, const OpenLoco::Town& rhs)
        {
            auto lhsStations = lhs.numStations;

            auto rhsStations = rhs.numStations;

            return rhsStations < lhsStations;
        }

        // 0x00499EC9, 0x00499F0A, 0x00499F28, 0x00499F3B
        static bool getOrder(const SortMode mode, OpenLoco::Town& lhs, OpenLoco::Town& rhs)
        {
            switch (mode)
            {
                case SortMode::Name:
                    return orderByName(lhs, rhs);

                case SortMode::Type:
                    return orderByType(lhs, rhs);

                case SortMode::Population:
                    return orderByPopulation(lhs, rhs);

                case SortMode::Stations:
                    return orderByStations(lhs, rhs);
            }

            return false;
        }

        // 0x00499E0B
        static void updateTownList(Window* self)
        {
            TownId chosenTown = TownId::null;
            for (auto& town : TownManager::towns())
            {
                if ((town.flags & TownFlags::sorted) != TownFlags::none)
                    continue;

                if (chosenTown == TownId::null)
                {
                    chosenTown = town.id();
                    continue;
                }

                if (getOrder(SortMode(self->sortMode), town, *TownManager::get(chosenTown)))
                {
                    chosenTown = town.id();
                }
            }

            if (chosenTown != TownId::null)
            {
                bool shouldInvalidate = false;

                TownManager::get(chosenTown)->flags |= TownFlags::sorted;

                if (chosenTown != TownId(self->rowInfo[self->rowCount]))
                {
                    self->rowInfo[self->rowCount] = enumValue(chosenTown);
                    shouldInvalidate = true;
                }

                self->rowCount += 1;
                if (self->rowCount > self->var_83C)
                {
                    self->var_83C = self->rowCount;
                    shouldInvalidate = true;
                }

                if (shouldInvalidate)
                {
                    self->invalidate();
                }
            }
            else
            {
                if (self->var_83C != self->rowCount)
                {
                    self->var_83C = self->rowCount;
                    self->invalidate();
                }

                Common::refreshTownList(self);
            }
        }

        // 0x0049A4A0
        static void onUpdate(Window& self)
        {
            self.frameNo++;

            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self.number, self.currentTab + Common::widx::tab_town_list);

            // Add three towns every tick.
            updateTownList(&self);
            updateTownList(&self);
            updateTownList(&self);
        }

        // 0x0049A4D0
        static void event_08(Window& self)
        {
            self.flags |= WindowFlags::notScrollView;
        }

        // 0x0049A4D8
        static void event_09(Window& self)
        {
            if (!self.hasFlags(WindowFlags::notScrollView))
                return;

            if (self.rowHover == -1)
                return;

            self.rowHover = -1;
            self.invalidate();
        }

        // 0x0049A4FA
        static void getScrollSize(Ui::Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = kRowHeight * self.var_83C;
        }

        // 0x00491841
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_town_list);
            return args;
        }

        // 0x004919A4
        static Ui::CursorId cursor(Window& self, int16_t widgetIdx, [[maybe_unused]] int16_t xPos, int16_t yPos, Ui::CursorId fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / kRowHeight;
            if (currentIndex < self.var_83C && self.rowInfo[currentIndex] != -1)
                return CursorId::handPointer;

            return fallback;
        }

        // 0x0049A37E
        static void tabReset(Window* self)
        {
            self->minWidth = kMinDimensions.width;
            self->minHeight = kMinDimensions.height;
            self->maxWidth = kMaxDimensions.width;
            self->maxHeight = kMaxDimensions.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
            self->var_83C = 0;
            self->rowHover = -1;

            Common::refreshTownList(self);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.cursor = cursor;
            events.drawScroll = drawScroll;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.getScrollSize = getScrollSize;
            events.onMouseUp = onMouseUp;
            events.onUpdate = onUpdate;
            events.scrollMouseDown = onScrollMouseDown;
            events.scrollMouseOver = onScrollMouseOver;
            events.prepareDraw = prepareDraw;
            events.tooltip = tooltip;
        }
    }

    // 0x00499C83
    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::townList, 0);
        if (window != nullptr)
        {
            window->callOnMouseUp(Common::widx::tab_town_list);
        }
        else
        {
            // 0x00499CFC
            auto origin = Ui::Point(Ui::width() - TownList::kWindowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::townList,
                origin,
                TownList::kWindowSize,
                WindowFlags::resizable,
                &TownList::events);

            window->number = 0;
            window->currentTab = 0;
            window->frameNo = 0;
            window->sortMode = 0;
            window->var_83C = 0;
            window->rowHover = -1;

            Common::refreshTownList(window);

            WindowManager::sub_4CEE0B(*window);

            window->minWidth = TownList::kMinDimensions.width;
            window->minHeight = TownList::kMinDimensions.height;
            window->maxWidth = TownList::kMaxDimensions.width;
            window->maxHeight = TownList::kMaxDimensions.height;
            window->flags |= WindowFlags::resizable;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::primary, skin->colour_0B);
            window->setColour(WindowColour::secondary, skin->colour_0C);

            // 0x00499CFC end

            // TODO: only needs to be called once.
            window->width = TownList::kWindowSize.width;
            window->height = TownList::kWindowSize.height;

            Common::initEvents();

            window->invalidate();

            window->widgets = TownList::widgets;
            window->enabledWidgets = TownList::enabledWidgets;

            if (isEditorMode() || isSandboxMode())
                window->disabledWidgets = 0;
            else
                window->disabledWidgets |= (1 << Common::widx::tab_build_town) | (1 << Common::widx::tab_build_buildings) | (1 << Common::widx::tab_build_misc_buildings);

            window->activatedWidgets = 0;
            window->holdableWidgets = 0;

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();
        }
        return window;
    }

    // 0x00496B50
    void reset()
    {
        LastGameOptionManager::setLastBuildingOption(LastGameOptionManager::kNoLastOption);
        LastGameOptionManager::setLastMiscBuildingOption(LastGameOptionManager::kNoLastOption);
        _buildingRotation = 2;
        _townSize = 3;
    }

    // 0x00499DAE
    void removeTown(TownId townId)
    {
        auto* window = WindowManager::find(WindowType::townList);
        if (window == nullptr)
            return;

        for (auto i = 0; i < window->var_83C; i++)
        {
            if (window->rowInfo[i] == enumValue(townId))
                window->rowInfo[i] = -1;
        }
    }

    namespace BuildTowns
    {
        static constexpr Ui::Size kWindowSize = { 220, 87 };

        enum widx
        {
            current_size = 8,
            select_size,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << current_size) | (1 << select_size);

        Widget widgets[] = {
            commonWidgets(220, 87, StringIds::title_build_new_towns),
            makeDropdownWidgets({ 100, 45 }, { 117, 12 }, WidgetType::combobox, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_select_town_size),
            widgetEnd(),
        };

        static WindowEventList events;

        constexpr StringId townSizeNames[9] = {
            StringIds::town_size_1,
            StringIds::town_size_2,
            StringIds::town_size_3,
            StringIds::town_size_4,
            StringIds::town_size_5,
            StringIds::town_size_6,
            StringIds::town_size_7,
            StringIds::town_size_8,
        };

        // 0x0049A59A
        static void prepareDraw(Ui::Window& self)
        {
            Common::prepareDraw(self);

            Widget::leftAlignTabs(self, Common::widx::tab_town_list, Common::widx::tab_build_misc_buildings);

            self.widgets[widx::current_size].text = townSizeNames[_townSize - 1];
        }

        // 0x0049A627
        static void draw(Ui::Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            drawingCtx.drawStringLeft(*rt, self.x + 3, self.y + self.widgets[widx::current_size].top + 1, Colour::black, StringIds::town_size_label);

            drawingCtx.drawStringLeft(*rt, self.x + 3, self.y + self.height - 13, Colour::black, StringIds::select_town_size);
        }

        // 0x0049A675
        static void onMouseUp(Ui::Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_town_list:
                case Common::widx::tab_build_town:
                case Common::widx::tab_build_buildings:
                case Common::widx::tab_build_misc_buildings:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x0049A7F2
        static void onUpdate(Window& self)
        {
            self.frameNo++;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self.number, self.currentTab + Common::widx::tab_town_list);
            if ((!Input::hasFlag(Input::Flags::toolActive)) || self.type != ToolManager::getToolWindowType() || self.number != ToolManager::getToolWindowNumber())
            {
                WindowManager::close(&self);
            }
        }

        // 0x0049A697
        static void onDropdown(Window& self, Ui::WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::select_size)
                return;
            if (itemIndex != -1)
            {
                itemIndex++;
                _townSize = itemIndex;
                self.invalidate();
            }
        }

        // 0x0049A7C1
        static void onToolAbort([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex)
        {
            Ui::Windows::Main::hideGridlines();
        }

        // 0x0049A710
        static void onToolUpdate([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            World::mapInvalidateSelectionRect();
            World::resetMapSelectionFlag(World::MapSelectionFlags::enable);

            auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
            if (mapPos)
            {
                World::setMapSelectionFlags(World::MapSelectionFlags::enable);
                World::setMapSelectionCorner(MapSelectionType::full);
                World::setMapSelectionArea(*mapPos, *mapPos);
                World::mapInvalidateSelectionRect();
            }
        }

        // 0x0049A75E
        static void onToolDown([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
            if (mapPos)
            {
                GameCommands::setErrorTitle(StringIds::error_cant_build_this_here);
                GameCommands::TownPlacementArgs placementArgs;
                placementArgs.pos = *mapPos;
                placementArgs.size = _townSize;
                if (GameCommands::doCommand(placementArgs, GameCommands::Flags::apply) != GameCommands::FAILURE)
                {
                    Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
                }
            }
        }

        // 0x0049A69E
        static void populateTownSizeSelect(Window* self, [[maybe_unused]] Widget* widget)
        {
            auto& currentSizeWidget = self->widgets[widx::current_size];

            Dropdown::show(self->x + currentSizeWidget.left, self->y + currentSizeWidget.top, currentSizeWidget.width() - 2, currentSizeWidget.height(), self->getColour(WindowColour::secondary), 8, (1 << 7));

            for (size_t i = 0; i < std::size(townSizeNames); ++i)
            {
                Dropdown::add(i, townSizeNames[i]);
            }

            Dropdown::setHighlightedItem(_townSize - 1);
        }

        // 0x0049A690
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == widx::select_size)
                populateTownSizeSelect(&self, &self.widgets[widgetIndex]);
        }

        // 0x0049A844
        static void onResize(Window& self)
        {
            self.setSize(kWindowSize, kWindowSize);
        }

        // 0x0049A7C7
        static void onClose(Window& self)
        {
            if (ToolManager::isToolActive(self.type, self.number))
                ToolManager::toolCancel();
        }

        // 0x0049A3BE
        static void tabReset(Window* self)
        {
            self->minWidth = kWindowSize.width;
            self->minHeight = kWindowSize.height;
            self->maxWidth = kWindowSize.width;
            self->maxWidth = kWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
            ToolManager::toolSet(self, Common::widx::tab_build_town, CursorId::placeTown);
            Input::setFlag(Input::Flags::flag6);
            Ui::Windows::Main::showGridlines();
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onClose = onClose;
            events.onDropdown = onDropdown;
            events.onResize = onResize;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.onMouseUp = onMouseUp;
            events.onMouseDown = onMouseDown;
            events.onUpdate = onUpdate;
            events.prepareDraw = prepareDraw;
            events.onToolAbort = onToolAbort;
        }
    }

    namespace BuildBuildings
    {
        static constexpr Ui::Size kWindowSize = { 600, 172 };

        static constexpr uint8_t kRowHeight = 112;

        enum widx
        {
            scrollview = 8,
            rotate_object,
            object_colour,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << scrollview) | (1 << rotate_object) | (1 << object_colour);

        Widget widgets[] = {
            commonWidgets(640, 172, StringIds::title_build_new_buildings),
            makeWidget({ 2, 45 }, { 573, 112 }, WidgetType::scrollview, WindowColour::secondary, 2),
            makeWidget({ 575, 46 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_object_90),
            makeWidget({ 579, 91 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_object_colour),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0049A8A6
        static void prepareDraw(Ui::Window& self)
        {
            self.widgets[widx::object_colour].image = Widget::kImageIdColourSet | Gfx::recolour(ImageIds::colour_swatch_recolourable, *_buildingColour);
            self.widgets[widx::object_colour].type = WidgetType::none;

            if (self.rowHover != -1)
            {
                auto buildingObj = ObjectManager::get<BuildingObject>(self.rowHover);
                if (buildingObj->colours != 0)
                    self.widgets[widx::object_colour].type = WidgetType::buttonWithColour;
            }

            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 26;
            self.widgets[widx::scrollview].bottom = self.height - 14;

            self.widgets[widx::rotate_object].left = self.width - 25;
            self.widgets[widx::rotate_object].right = self.width - 2;

            self.widgets[widx::object_colour].left = self.width - 21;
            self.widgets[widx::object_colour].right = self.width - 6;

            self.widgets[Common::widx::caption].text = StringIds::title_build_new_buildings;

            if (self.currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                self.widgets[Common::widx::caption].text = StringIds::title_build_new_misc_buildings;

            Widget::leftAlignTabs(self, Common::widx::tab_town_list, Common::widx::tab_build_misc_buildings);
        }

        // 0x0049A9C2
        static void draw(Ui::Window& self, Gfx::RenderTarget* rt)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            self.draw(rt);
            Common::drawTabs(&self, rt);

            auto buildingId = self.var_846;

            if (buildingId == 0xFFFF)
            {
                buildingId = self.rowHover;

                if (buildingId == 0xFFFF)
                    return;
            }

            auto buildingObj = ObjectManager::get<BuildingObject>(buildingId);

            drawingCtx.drawStringLeftClipped(*rt, self.x + 3, self.y + self.height - 13, self.width - 19, Colour::black, StringIds::black_stringid, &buildingObj->name);
        }

        // 0x0049AB31
        static void onMouseUp(Ui::Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_town_list:
                case Common::widx::tab_build_town:
                case Common::widx::tab_build_buildings:
                case Common::widx::tab_build_misc_buildings:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::rotate_object:
                    if (_buildingRotation < 3)
                        _buildingRotation++;
                    else
                        _buildingRotation = 0;
                    self.invalidate();
                    break;
            }
        }

        // 0x0049AD51
        static void onUpdate(Window& self)
        {
            if (!Input::hasFlag(Input::Flags::rightMousePressed))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                Window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == &self)
                {
                    xPos -= self.x;
                    xPos += 26;
                    yPos -= self.y;

                    if ((yPos < 42) || (xPos <= self.width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        WidgetIndex_t activeWidget = self.findWidgetAt(xPos, yPos);
                        if (activeWidget > Common::widx::panel)
                        {
                            self.savedView.mapX += 1;
                            if (self.savedView.mapX >= 8)
                            {
                                auto y = std::min(self.scrollAreas[0].contentHeight - 1 + 60, 500);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 276);
                                }
                                self.minWidth = kWindowSize.width;
                                self.minHeight = y;
                                self.maxWidth = kWindowSize.width;
                                self.maxHeight = y;
                            }
                            else
                            {
                                if (Input::state() != Input::State::scrollLeft)
                                {
                                    self.minWidth = kWindowSize.width;
                                    self.minHeight = kWindowSize.height;
                                    self.maxWidth = kWindowSize.width;
                                    self.maxHeight = kWindowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self.savedView.mapX = 0;
                    if (Input::state() != Input::State::scrollLeft)
                    {
                        self.minWidth = kWindowSize.width;
                        self.minHeight = kWindowSize.height;
                        self.maxWidth = kWindowSize.width;
                        self.maxHeight = kWindowSize.height;
                    }
                }
            }
            self.frameNo++;

            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self.number, self.currentTab + Common::widx::tab_town_list);
            if (!ToolManager::isToolActive(self.type, self.number))
                WindowManager::close(&self);
        }

        // 0x0049AB59
        static void onDropdown(Window& self, Ui::WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::object_colour)
                return;
            if (itemIndex == -1)
                return;

            _buildingColour = static_cast<Colour>(Dropdown::getItemArgument(itemIndex, 2));
            self.invalidate();
        }

        // 0x0049B37F
        static void removeBuildingGhost()
        {
            if (_buildingGhostPlaced)
            {
                GameCommands::BuildingRemovalArgs args;
                args.pos = _buildingGhostPos;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
                _buildingGhostPlaced = false;
            }
        }

        // 0x0049AD46
        static void onToolAbort([[maybe_unused]] Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex)
        {
            removeBuildingGhost();
            Ui::Windows::Main::hideGridlines();
        }

        // 0x0049B32A
        static currency32_t placeBuildingGhost(const GameCommands::BuildingPlacementArgs& placementArgs)
        {
            removeBuildingGhost();
            auto res = GameCommands::doCommand(placementArgs, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            if (res != GameCommands::FAILURE)
            {
                _buildingGhostPos = placementArgs.pos;
                _buildingGhostType = placementArgs.type;
                _buildingGhostRotation = placementArgs.rotation;
                _buildingGhostPlaced = true;
            }
            return res;
        }

        // 0x0049B3B2
        static std::optional<GameCommands::BuildingPlacementArgs> getBuildingPlacementArgsFromCursor(const int16_t x, const int16_t y)
        {
            auto* townListWnd = WindowManager::find(WindowType::townList);
            if (townListWnd == nullptr)
            {
                return {};
            }

            if (townListWnd->currentTab != (Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list) && townListWnd->currentTab != (Common::widx::tab_build_buildings - Common::widx::tab_town_list))
            {
                return {};
            }

            if (townListWnd->rowHover == -1)
            {
                return {};
            }

            const auto pos = ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y }); // ax,cx
            if (!pos)
            {
                return {};
            }

            GameCommands::BuildingPlacementArgs args;
            args.rotation = (_buildingRotation - WindowManager::getCurrentRotation()) & 0x3; // bh
            args.colour = *_buildingColour;
            auto tile = World::TileManager::get(*pos);
            const auto* surface = tile.surface();
            if (surface == nullptr)
            {
                return {};
            }

            auto z = surface->baseHeight(); // di
            if (surface->slope())
            {
                z += 16;
            }
            args.pos = World::Pos3(pos->x, pos->y, z);
            args.type = townListWnd->rowHover;   // dl
            args.variation = _buildingVariation; // dh
            if (isEditorMode())
            {
                args.buildImmediately = true; // bh
            }
            return { args };
        }

        // 0x0049ABF0
        static void onToolUpdate(Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            World::mapInvalidateSelectionRect();
            World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
            auto placementArgs = getBuildingPlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeBuildingGhost();
                return;
            }

            // Always show buildings, not scaffolding, for ghost placements.
            placementArgs->buildImmediately = true;

            World::setMapSelectionFlags(World::MapSelectionFlags::enable);
            World::setMapSelectionCorner(MapSelectionType::full);
            auto* building = ObjectManager::get<BuildingObject>(placementArgs->type);
            auto posB = World::Pos2(placementArgs->pos) + (building->hasFlags(BuildingObjectFlags::largeTile) ? World::Pos2(32, 32) : World::Pos2(0, 0));
            World::setMapSelectionArea(placementArgs->pos, posB);
            World::mapInvalidateSelectionRect();

            if (_buildingGhostPlaced)
            {
                if (*_buildingGhostPos == placementArgs->pos && _buildingGhostRotation == placementArgs->rotation && _buildingGhostType == placementArgs->type)
                {
                    return;
                }
            }

            removeBuildingGhost();
            auto cost = placeBuildingGhost(*placementArgs);
            if (cost != _dword_1135C34)
            {
                _dword_1135C34 = cost;
                self.invalidate();
            }
        }

        // 0x0049ACBD
        static void onToolDown(Window& self, [[maybe_unused]] const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            removeBuildingGhost();
            auto placementArgs = getBuildingPlacementArgsFromCursor(x, y);
            if (placementArgs)
            {
                GameCommands::setErrorTitle(StringIds::error_cant_build_this_here);

                if (GameCommands::doCommand(*placementArgs, GameCommands::Flags::apply | GameCommands::Flags::flag_1) != GameCommands::FAILURE)
                {
                    Audio::playSound(Audio::SoundId::construct, GameCommands::getPosition());
                }
            }

            uint8_t variation = 0;
            if (self.rowHover != -1)
            {
                auto* buildingObj = ObjectManager::get<BuildingObject>(self.rowHover);
                variation = (_buildingVariation + 1) % buildingObj->numVariations;
            }
            _buildingVariation = variation;
        }

        // 0x0049AB52
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == widx::object_colour)
            {
                auto obj = ObjectManager::get<BuildingObject>(self.rowHover);
                Dropdown::showColour(&self, &self.widgets[widgetIndex], obj->colours, _buildingColour, self.getColour(WindowColour::secondary));
            }
        }

        // 0x0049B2B5
        static void updateActiveThumb(Window* self)
        {
            uint16_t scrollHeight = 0;
            self->callGetScrollSize(0, 0, &scrollHeight);
            self->scrollAreas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self->var_83C; i++)
            {
                if (self->rowInfo[i] == self->rowHover)
                    break;
            }

            if (i >= self->var_83C)
                i = 0;

            i = (i / 5) * kRowHeight;

            self->scrollAreas[0].contentOffsetY = i;

            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x0049AF98
        static void onResize(Window& self)
        {
            self.invalidate();
            Ui::Size kMinWindowSize = { self.minWidth, self.minHeight };
            Ui::Size kMaxWindowSize = { self.maxWidth, self.maxHeight };
            bool hasResized = self.setSize(kMinWindowSize, kMaxWindowSize);
            if (hasResized)
                updateActiveThumb(&self);
        }

        // 0x0049AE83
        static void getScrollSize(Ui::Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (4 + self.var_83C) / 5;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= kRowHeight;
        }

        // 0x0049ABBB
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_building_list);
            return args;
        }

        // 0x0049AA1C
        static void drawScroll(Ui::Window& self, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            drawingCtx.clearSingle(rt, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                if (yPos + kRowHeight < rt.y)
                {
                    xPos += kRowHeight;
                    if (xPos >= kRowHeight * 5) // full row
                    {
                        xPos = 0;
                        yPos += kRowHeight;
                    }
                    continue;
                }
                else if (yPos > rt.y + rt.height)
                {
                    break;
                }

                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        drawingCtx.drawRectInset(rt, xPos, yPos, kRowHeight, kRowHeight, self.getColour(WindowColour::secondary), Drawing::RectInsetFlags::colourLight);
                    }
                }
                else
                {
                    drawingCtx.drawRectInset(rt, xPos, yPos, kRowHeight, kRowHeight, self.getColour(WindowColour::secondary), (Drawing::RectInsetFlags::colourLight | Drawing::RectInsetFlags::borderInset));
                }

                auto buildingObj = ObjectManager::get<BuildingObject>(self.rowInfo[i]);

                auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(xPos + 1, yPos + 1, 110, 110));
                if (clipped)
                {
                    auto colour = *_buildingColour;
                    if (self.rowHover != self.rowInfo[i])
                    {
                        auto bit = Numerics::bitScanReverse(buildingObj->colours);
                        colour = bit == -1 ? Colour::black : static_cast<Colour>(bit);
                    }

                    buildingObj->drawBuilding(&*clipped, _buildingRotation, 56, 96, colour);
                }

                xPos += kRowHeight;

                if (xPos >= kRowHeight * 5) // full row
                {
                    xPos = 0;
                    yPos += kRowHeight;
                }
            }
        }

        // 0x0049B304
        static void updateBuildingColours(Window* self)
        {
            if (self->rowHover != -1)
            {
                auto buildingObj = ObjectManager::get<BuildingObject>(self->rowHover);
                if (buildingObj->colours != 0)
                {
                    auto bit = Numerics::bitScanReverse(buildingObj->colours);
                    auto colour = bit == -1 ? Colour::black : static_cast<Colour>(bit);
                    _buildingColour = colour;
                }
            }
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / 112) + (y / 112) * 5;
        }

        // 0x0049AEFD
        static void onScrollMouseDown(Ui::Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self.var_83C; i++)
            {
                auto rowInfo = self.rowInfo[i];
                index--;
                if (index < 0)
                {
                    self.rowHover = rowInfo;

                    if (self.currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                        LastGameOptionManager::setLastMiscBuildingOption(static_cast<uint8_t>(rowInfo));
                    else
                        LastGameOptionManager::setLastBuildingOption(static_cast<uint8_t>(rowInfo));

                    updateBuildingColours(&self);

                    int32_t pan = (self.width >> 1) + self.x;
                    Audio::playSound(Audio::SoundId::clickDown, pan);
                    self.savedView.mapX = -16;
                    _dword_1135C34 = GameCommands::FAILURE;
                    _buildingVariation = 0;
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x0049AEBA
        static void onScrollMouseOver(Ui::Window& self, int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = y;
            auto i = 0;
            for (; i < self.var_83C; i++)
            {
                rowInfo = self.rowInfo[i];
                index--;
                if (index < 0)
                {
                    self.var_846 = rowInfo;
                    self.invalidate();
                    break;
                }
            }
        }

        // 0x0049ABC5
        static void onClose(Window& self)
        {
            if (ToolManager::isToolActive(self.type, self.number))
                ToolManager::toolCancel();
        }

        // 0x0049AEA1
        static void event_08(Window& self)
        {
            if (self.var_846 != 0xFFFF)
            {
                self.var_846 = 0xFFFF;
                self.invalidate();
            }
        }

        // 0x0049B206
        static void updateBuildingList(Window* self)
        {
            auto buildingCount = 0;
            for (auto i = 0; i < 128; i++)
            {
                auto buildingObj = ObjectManager::get<BuildingObject>(i);
                if (buildingObj == nullptr)
                    continue;
                if (self->currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                {
                    if (!buildingObj->hasFlags(BuildingObjectFlags::miscBuilding))
                        continue;
                    if (buildingObj->hasFlags(BuildingObjectFlags::isHeadquarters))
                        continue;
                }
                else
                {
                    if (buildingObj->hasFlags(BuildingObjectFlags::miscBuilding))
                        continue;
                }
                self->rowInfo[buildingCount] = i;
                buildingCount++;
            }

            self->var_83C = buildingCount;
            auto rowHover = -1;

            auto lastSelectedBuilding = LastGameOptionManager::getLastBuildingOption();
            if (self->currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                lastSelectedBuilding = LastGameOptionManager::getLastMiscBuildingOption();

            if (lastSelectedBuilding != LastGameOptionManager::kNoLastOption)
            {
                for (auto i = 0; i <= self->var_83C; i++)
                {
                    if (lastSelectedBuilding == self->rowInfo[i])
                    {
                        rowHover = lastSelectedBuilding;
                        break;
                    }
                }
            }

            if (rowHover == -1 && self->var_83C != 0)
            {
                rowHover = self->rowInfo[0];
            }

            self->rowHover = rowHover;
            updateActiveThumb(self);
            updateBuildingColours(self);
        }

        // 0x0049A3FF
        static void tabReset(Window* self)
        {
            self->minWidth = kWindowSize.width;
            self->minHeight = kWindowSize.height;
            self->maxWidth = kWindowSize.width;
            self->maxWidth = kWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;

            auto tab = Common::widx::tab_build_buildings;
            if (self->currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                tab = Common::widx::tab_build_misc_buildings;

            ToolManager::toolSet(self, tab, CursorId::placeBuilding);
            Input::setFlag(Input::Flags::flag6);
            Ui::Windows::Main::showGridlines();

            static loco_global<uint8_t, 0x01135C60> _byte_1135C60;
            _byte_1135C60 = 0;
            _dword_1135C34 = GameCommands::FAILURE;
            self->var_83C = 0;
            self->rowHover = -1;
            self->var_846 = 0xFFFFU;

            updateBuildingList(self);
            updateBuildingColours(self);

            _buildingVariation = 0;
        }

        static void initEvents()
        {
            events.draw = draw;
            events.onClose = onClose;
            events.onResize = onResize;
            events.drawScroll = drawScroll;
            events.event_08 = event_08;
            events.onDropdown = onDropdown;
            events.onToolUpdate = onToolUpdate;
            events.onToolDown = onToolDown;
            events.onMouseDown = onMouseDown;
            events.getScrollSize = getScrollSize;
            events.onMouseUp = onMouseUp;
            events.onUpdate = onUpdate;
            events.scrollMouseDown = onScrollMouseDown;
            events.scrollMouseOver = onScrollMouseOver;
            events.prepareDraw = prepareDraw;
            events.tooltip = tooltip;
            events.onToolAbort = onToolAbort;
        }
    }

    bool rotate(Window* self)
    {
        if (self->currentTab >= Common::widx::tab_build_buildings - Common::widx::tab_town_list)
        {
            if (!self->isDisabled(BuildBuildings::widx::rotate_object))
            {
                if (self->widgets[BuildBuildings::widx::rotate_object].type != WidgetType::none)
                {
                    self->callOnMouseUp(BuildBuildings::widx::rotate_object);
                    return true;
                }
            }
        }

        return false;
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            const widx widgetIndex;
            WindowEventList* events;
            const uint64_t enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { TownList::widgets, widx::tab_town_list, &TownList::events, TownList::enabledWidgets },
            { BuildTowns::widgets, widx::tab_build_town, &BuildTowns::events, BuildTowns::enabledWidgets },
            { BuildBuildings::widgets, widx::tab_build_buildings, &BuildBuildings::events, BuildBuildings::enabledWidgets },
            { BuildBuildings::widgets, widx::tab_build_misc_buildings, &BuildBuildings::events, BuildBuildings::enabledWidgets },
        };

        static void prepareDraw(Window& self)
        {
            // Reset tab widgets if needed
            const auto& tabWidgets = tabInformationByTabOffset[self.currentTab].widgets;
            if (self.widgets != tabWidgets)
            {
                self.widgets = tabWidgets;
                self.initScrollWidgets();
            }

            // Activate the current tab
            self.activatedWidgets &= ~((1ULL << tab_town_list) | (1ULL << tab_build_town) | (1ULL << tab_build_buildings) | (1ULL << tab_build_misc_buildings));
            self.activatedWidgets |= (1ULL << Common::tabInformationByTabOffset[self.currentTab].widgetIndex);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;
        }

        // 0x0049B054
        static void drawTabs(Window* self, Gfx::RenderTarget* rt)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Town List Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_towns;

                Widget::drawTab(self, rt, imageId, widx::tab_town_list);
            }

            // Build New Towns Tab
            {
                static const uint32_t buildNewTownsImageIds[] = {
                    InterfaceSkin::ImageIds::build_town_frame_0,
                    InterfaceSkin::ImageIds::build_town_frame_1,
                    InterfaceSkin::ImageIds::build_town_frame_2,
                    InterfaceSkin::ImageIds::build_town_frame_3,
                    InterfaceSkin::ImageIds::build_town_frame_4,
                    InterfaceSkin::ImageIds::build_town_frame_5,
                    InterfaceSkin::ImageIds::build_town_frame_6,
                    InterfaceSkin::ImageIds::build_town_frame_7,
                    InterfaceSkin::ImageIds::build_town_frame_8,
                    InterfaceSkin::ImageIds::build_town_frame_9,
                    InterfaceSkin::ImageIds::build_town_frame_10,
                    InterfaceSkin::ImageIds::build_town_frame_11,
                    InterfaceSkin::ImageIds::build_town_frame_12,
                    InterfaceSkin::ImageIds::build_town_frame_13,
                    InterfaceSkin::ImageIds::build_town_frame_14,
                    InterfaceSkin::ImageIds::build_town_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_build_town - widx::tab_town_list)
                    imageId += buildNewTownsImageIds[(self->frameNo / 2) % std::size(buildNewTownsImageIds)];
                else
                    imageId += buildNewTownsImageIds[0];

                Widget::drawTab(self, rt, imageId, widx::tab_build_town);
            }

            // Build New Buildings Tab
            {
                static const uint32_t buildBuildingsImageIds[] = {
                    InterfaceSkin::ImageIds::build_buildings_frame_0,
                    InterfaceSkin::ImageIds::build_buildings_frame_1,
                    InterfaceSkin::ImageIds::build_buildings_frame_2,
                    InterfaceSkin::ImageIds::build_buildings_frame_3,
                    InterfaceSkin::ImageIds::build_buildings_frame_4,
                    InterfaceSkin::ImageIds::build_buildings_frame_5,
                    InterfaceSkin::ImageIds::build_buildings_frame_6,
                    InterfaceSkin::ImageIds::build_buildings_frame_7,
                    InterfaceSkin::ImageIds::build_buildings_frame_8,
                    InterfaceSkin::ImageIds::build_buildings_frame_9,
                    InterfaceSkin::ImageIds::build_buildings_frame_10,
                    InterfaceSkin::ImageIds::build_buildings_frame_11,
                    InterfaceSkin::ImageIds::build_buildings_frame_12,
                    InterfaceSkin::ImageIds::build_buildings_frame_13,
                    InterfaceSkin::ImageIds::build_buildings_frame_14,
                    InterfaceSkin::ImageIds::build_buildings_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_build_buildings - widx::tab_town_list)
                    imageId += buildBuildingsImageIds[(self->frameNo / 2) % std::size(buildBuildingsImageIds)];
                else
                    imageId += buildBuildingsImageIds[0];

                Widget::drawTab(self, rt, imageId, widx::tab_build_buildings);
            }

            // Build New Misc Buildings Tab
            {
                static const uint32_t buildMiscBuildingsImageIds[] = {
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_0,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_1,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_2,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_3,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_4,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_5,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_6,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_7,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_8,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_9,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_10,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_11,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_12,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_13,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_14,
                    InterfaceSkin::ImageIds::build_misc_buildings_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self->currentTab == widx::tab_build_misc_buildings - widx::tab_town_list)
                    imageId += buildMiscBuildingsImageIds[(self->frameNo / 2) % std::size(buildMiscBuildingsImageIds)];
                else
                    imageId += buildMiscBuildingsImageIds[0];

                Widget::drawTab(self, rt, imageId, widx::tab_build_misc_buildings);
            }
        }

        // 0x0049A2E2
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (ToolManager::isToolActive(self->type, self->number))
                ToolManager::toolCancel();

            self->currentTab = widgetIndex - widx::tab_town_list;
            self->frameNo = 0;
            self->flags &= ~(WindowFlags::flag_16);

            self->viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_town_list];

            self->enabledWidgets = tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = tabInfo.events;
            self->activatedWidgets = 0;
            self->widgets = tabInfo.widgets;

            if (isEditorMode() || isSandboxMode())
                self->disabledWidgets = 0;
            else
                self->disabledWidgets |= (1 << Common::widx::tab_build_town) | (1 << Common::widx::tab_build_buildings) | (1 << Common::widx::tab_build_misc_buildings);

            self->invalidate();

            if (self->currentTab == widx::tab_town_list - widx::tab_town_list)
                TownList::tabReset(self);
            if (self->currentTab == widx::tab_build_town - widx::tab_town_list)
                BuildTowns::tabReset(self);
            if (self->currentTab == widx::tab_build_buildings - widx::tab_town_list || self->currentTab == widx::tab_build_misc_buildings - widx::tab_town_list)
                BuildBuildings::tabReset(self);

            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x00499DDE
        static void refreshTownList(Window* self)
        {
            self->rowCount = 0;

            for (auto& town : TownManager::towns())
            {
                town.flags &= ~TownFlags::sorted;
            }
        }

        static void initEvents()
        {
            TownList::initEvents();
            BuildTowns::initEvents();
            BuildBuildings::initEvents();
        }
    }
}
