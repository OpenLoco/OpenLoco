#include "../Audio/Audio.h"
#include "../Config.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/TileManager.h"
#include "../Objects/BuildingObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Town.h"
#include "../TownManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Utility/Numeric.hpp"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TownList
{
    static loco_global<currency32_t, 0x01135C34> dword_1135C34;
    static loco_global<bool, 0x01135C60> _buildingGhostPlaced;
    static loco_global<Map::Pos3, 0x01135C50> _buildingGhostPos;
    static loco_global<Colour, 0x01135C61> _buildingColour;
    static loco_global<uint8_t, 0x01135C62> _buildingGhostType;
    static loco_global<uint8_t, 0x01135C63> _buildingRotation;
    static loco_global<uint8_t, 0x01135C64> _buildingGhostRotation;
    static loco_global<uint8_t, 0x01135C65> _buildingVariation;
    static loco_global<uint8_t, 0x01135C66> _townSize;
    static loco_global<uint8_t, 0x00525FC8> _lastSelectedBuilding;
    static loco_global<uint8_t, 0x00525FC9> _lastSelectedMiscBuilding;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;

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

        static void prepareDraw(Window* self);
        static void repositionTabs(Window* self);
        static void drawTabs(Window* self, Gfx::Context* context);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void initEvents();
        static void refreshTownList(Window* self);

    }

    namespace TownList
    {
        static const Ui::Size windowSize = { 600, 197 };
        static const Ui::Size maxDimensions = { 600, 900 };
        static const Ui::Size minDimensions = { 192, 100 };

        static const uint8_t rowHeight = 10;

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
            makeWidget({ 4, 43 }, { 200, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_by_name),
            makeWidget({ 204, 43 }, { 80, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_town_type),
            makeWidget({ 284, 43 }, { 70, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_population),
            makeWidget({ 354, 43 }, { 70, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, ImageIds::null, StringIds::tooltip_sort_stations),
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
        static void prepareDraw(Ui::Window* self)
        {
            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 4;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            // Reposition header buttons
            self->widgets[widx::sort_town_name].right = std::min(203, self->width - 8);

            self->widgets[widx::sort_town_type].left = std::min(204, self->width - 8);
            self->widgets[widx::sort_town_type].right = std::min(283, self->width - 8);

            self->widgets[widx::sort_town_population].left = std::min(284, self->width - 8);
            self->widgets[widx::sort_town_population].right = std::min(353, self->width - 8);

            self->widgets[widx::sort_town_stations].left = std::min(354, self->width - 8);
            self->widgets[widx::sort_town_stations].right = std::min(423, self->width - 8);

            // Set header button captions
            self->widgets[widx::sort_town_name].text = self->sortMode == SortMode::Name ? StringIds::table_header_name_desc : StringIds::table_header_name;
            self->widgets[widx::sort_town_type].text = self->sortMode == SortMode::Type ? StringIds::table_header_town_type_desc : StringIds::table_header_town_type;
            self->widgets[widx::sort_town_population].text = self->sortMode == SortMode::Population ? StringIds::table_header_population_desc : StringIds::table_header_population;
            self->widgets[widx::sort_town_stations].text = self->sortMode == SortMode::Stations ? StringIds::table_header_stations_desc : StringIds::table_header_stations;

            Common::repositionTabs(self);
        }

        // 0x0049A0F8
        static void drawScroll(Ui::Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            Gfx::clearSingle(context, shade);

            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                const auto townId = TownId(self.rowInfo[i]);

                // Skip items outside of view, or irrelevant to the current filter.
                if (yPos + rowHeight < context.y || yPos >= yPos + rowHeight + context.height || townId == TownId::null)
                {
                    yPos += rowHeight;
                    continue;
                }

                string_id text_colour_id = StringIds::black_stringid;

                // Highlight selection.
                if (townId == TownId(self.rowHover))
                {
                    Gfx::drawRect(context, 0, yPos, self.width, rowHeight, 0x2000030);
                    text_colour_id = StringIds::wcolour2_stringid;
                }

                if (townId == TownId::null)
                    continue;
                auto town = TownManager::get(townId);

                // Town Name
                {
                    auto args = FormatArguments();
                    args.push(town->name);

                    Gfx::drawString_494BBF(context, 0, yPos, 198, Colour::black, text_colour_id, &args);
                }
                // Town Type
                {
                    auto args = FormatArguments();
                    args.push(town->getTownSizeString());

                    Gfx::drawString_494BBF(context, 200, yPos, 278, Colour::black, text_colour_id, &args);
                }
                // Town Population
                {
                    auto args = FormatArguments();
                    args.push(StringIds::int_32);
                    args.push(town->population);

                    Gfx::drawString_494BBF(context, 280, yPos, 68, Colour::black, text_colour_id, &args);
                }
                // Town Stations
                {
                    auto args = FormatArguments();
                    args.push(StringIds::int_32);
                    args.push<int32_t>(town->num_stations);

                    Gfx::drawString_494BBF(context, 350, yPos, 68, Colour::black, text_colour_id, &args);
                }
                yPos += rowHeight;
            }
        }

        // 0x0049A0A7
        static void draw(Ui::Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);
            auto args = FormatArguments();
            auto xPos = self->x + 4;
            auto yPos = self->y + self->height - 12;

            if (self->var_83C == 1)
                args.push(StringIds::status_towns_singular);
            else
                args.push(StringIds::status_towns_plural);
            args.push(self->var_83C);

            Gfx::drawString_494B3F(*context, xPos, yPos, Colour::black, StringIds::black_stringid, &args);
        }

        // 0x0049A27F
        static void onMouseUp(Ui::Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_town_list:
                case Common::widx::tab_build_town:
                case Common::widx::tab_build_buildings:
                case Common::widx::tab_build_misc_buildings:
                    Common::switchTab(self, widgetIndex);
                    break;

                case widx::sort_town_name:
                case widx::sort_town_type:
                case widx::sort_town_population:
                case widx::sort_town_stations:
                {
                    auto sortMode = widgetIndex - widx::sort_town_name;
                    if (self->sortMode == sortMode)
                        return;

                    self->sortMode = sortMode;
                    self->invalidate();
                    self->var_83C = 0;
                    self->rowHover = -1;

                    Common::refreshTownList(self);
                    break;
                }
            }
        }

        // 0x0049A56D
        static void onScrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            uint16_t currentRow = y / rowHeight;
            if (currentRow > self->var_83C)
                return;

            int16_t currentTown = self->rowInfo[currentRow];
            if (currentTown == -1)
                return;

            Town::open(currentTown);
        }

        // 0x0049A532
        static void onScrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            self->flags &= ~(WindowFlags::notScrollView);

            uint16_t currentRow = y / rowHeight;
            int16_t currentTown = -1;

            if (currentRow < self->var_83C)
                currentTown = self->rowInfo[currentRow];

            if (self->rowHover == currentTown)
                return;

            self->rowHover = currentTown;
            self->invalidate();
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
            auto lhsStations = lhs.num_stations;

            auto rhsStations = rhs.num_stations;

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
                if ((town.flags & TownFlags::sorted) != 0)
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
        static void onUpdate(Window* self)
        {
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->currentTab + Common::widx::tab_town_list);

            // Add three towns every tick.
            updateTownList(self);
            updateTownList(self);
            updateTownList(self);
        }

        // 0x0049A4D0
        static void event_08(Window* self)
        {
            self->flags |= WindowFlags::notScrollView;
        }

        // 0x0049A4D8
        static void event_09(Window* self)
        {
            if (!(self->flags & WindowFlags::notScrollView))
                return;

            if (self->rowHover == -1)
                return;

            self->rowHover = -1;
            self->invalidate();
        }

        // 0x0049A4FA
        static void getScrollSize(Ui::Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = rowHeight * self->var_83C;
        }

        // 0x00491841
        static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_town_list);
            return args;
        }

        // 0x004919A4
        static Ui::CursorId cursor(Window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / rowHeight;
            if (currentIndex < self->var_83C && self->rowInfo[currentIndex] != -1)
                return CursorId::handPointer;

            return fallback;
        }

        // 0x0049A37E
        static void tabReset(Window* self)
        {
            self->minWidth = minDimensions.width;
            self->minHeight = minDimensions.height;
            self->maxWidth = maxDimensions.width;
            self->maxHeight = maxDimensions.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
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
            auto origin = Ui::Point(Ui::width() - TownList::windowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::townList,
                origin,
                TownList::windowSize,
                WindowFlags::resizable,
                &TownList::events);

            window->number = 0;
            window->currentTab = 0;
            window->frame_no = 0;
            window->sortMode = 0;
            window->var_83C = 0;
            window->rowHover = -1;

            Common::refreshTownList(window);

            WindowManager::sub_4CEE0B(window);

            window->minWidth = TownList::minDimensions.width;
            window->minHeight = TownList::minDimensions.height;
            window->maxWidth = TownList::maxDimensions.width;
            window->maxHeight = TownList::maxDimensions.height;
            window->flags |= WindowFlags::resizable;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::primary, skin->colour_0B);
            window->setColour(WindowColour::secondary, skin->colour_0C);

            // 0x00499CFC end

            // TODO: only needs to be called once.
            window->width = TownList::windowSize.width;
            window->height = TownList::windowSize.height;

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
        _lastSelectedBuilding = 0xFF;
        _lastSelectedMiscBuilding = 0xFF;
        _buildingRotation = 2;
        _townSize = 3;
    }

    namespace BuildTowns
    {
        static const Ui::Size windowSize = { 220, 87 };

        enum widx
        {
            current_size = 8,
            select_size,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << current_size) | (1 << select_size);

        Widget widgets[] = {
            commonWidgets(220, 87, StringIds::title_build_new_towns),
            makeDropdownWidgets({ 100, 45 }, { 117, 12 }, WidgetType::combobox, WindowColour::secondary, ImageIds::null, StringIds::tooltip_select_town_size),
            widgetEnd(),
        };

        static WindowEventList events;

        constexpr string_id townSizeNames[9] = {
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
        static void prepareDraw(Ui::Window* self)
        {
            Common::prepareDraw(self);

            Common::repositionTabs(self);

            self->widgets[widx::current_size].text = townSizeNames[_townSize - 1];
        }

        // 0x0049A627
        static void draw(Ui::Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            Gfx::drawString_494B3F(*context, self->x + 3, self->y + self->widgets[widx::current_size].top + 1, Colour::black, StringIds::town_size_label);

            Gfx::drawString_494B3F(*context, self->x + 3, self->y + self->height - 13, Colour::black, StringIds::select_town_size);
        }

        // 0x0049A675
        static void onMouseUp(Ui::Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_town_list:
                case Common::widx::tab_build_town:
                case Common::widx::tab_build_buildings:
                case Common::widx::tab_build_misc_buildings:
                    Common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0049A7F2
        static void onUpdate(Window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->currentTab + Common::widx::tab_town_list);
            if ((!Input::hasFlag(Input::Flags::toolActive)) || self->type != _toolWindowType || self->number != _toolWindowNumber)
            {
                WindowManager::close(self);
            }
        }

        // 0x0049A697
        static void onDropdown(Window* self, Ui::WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::select_size)
                return;
            if (itemIndex != -1)
            {
                itemIndex++;
                _townSize = itemIndex;
                self->invalidate();
            }
        }

        // 0x0049A7C1
        static void onToolAbort(Window& self, const WidgetIndex_t widgetIndex)
        {
            Ui::Windows::hideGridlines();
        }

        // 0x0049A710
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            Map::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);

            auto mapPos = Ui::ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });
            if (mapPos)
            {
                Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
                Map::TileManager::setMapSelectionCorner(4);
                Map::TileManager::setMapSelectionArea(*mapPos, *mapPos);
                Map::TileManager::mapInvalidateSelectionRect();
            }
        }

        // 0x0049A75E
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
        static void populateTownSizeSelect(Window* self, Widget* widget)
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
        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == widx::select_size)
                populateTownSizeSelect(self, &self->widgets[widgetIndex]);
        }

        // 0x0049A844
        static void onResize(Window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x0049A7C7
        static void onClose(Window* self)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();
        }

        // 0x0049A3BE
        static void tabReset(Window* self)
        {
            self->minWidth = windowSize.width;
            self->minHeight = windowSize.height;
            self->maxWidth = windowSize.width;
            self->maxWidth = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
            Input::toolSet(self, Common::widx::tab_build_town, CursorId::placeTown);
            Input::setFlag(Input::Flags::flag6);
            Ui::Windows::showGridlines();
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
        static const Ui::Size windowSize = { 600, 172 };

        static const uint8_t rowHeight = 112;

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
            makeWidget({ 579, 91 }, { 16, 16 }, WidgetType::buttonWithColour, WindowColour::secondary, ImageIds::null, StringIds::tooltip_object_colour),
            widgetEnd(),
        };

        static WindowEventList events;

        // 0x0049A8A6
        static void prepareDraw(Ui::Window* self)
        {
            self->widgets[widx::object_colour].image = Widget::imageIdColourSet | Gfx::recolour(ImageIds::colour_swatch_recolourable, *_buildingColour);
            self->widgets[widx::object_colour].type = WidgetType::none;

            if (self->rowHover != -1)
            {
                auto buildingObj = ObjectManager::get<BuildingObject>(self->rowHover);
                if (buildingObj->colours != 0)
                    self->widgets[widx::object_colour].type = WidgetType::buttonWithColour;
            }

            Common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 26;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            self->widgets[widx::rotate_object].left = self->width - 25;
            self->widgets[widx::rotate_object].right = self->width - 2;

            self->widgets[widx::object_colour].left = self->width - 21;
            self->widgets[widx::object_colour].right = self->width - 6;

            self->widgets[Common::widx::caption].text = StringIds::title_build_new_buildings;

            if (self->currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                self->widgets[Common::widx::caption].text = StringIds::title_build_new_misc_buildings;

            Common::repositionTabs(self);
        }

        // 0x0049A9C2
        static void draw(Ui::Window* self, Gfx::Context* context)
        {
            self->draw(context);
            Common::drawTabs(self, context);

            auto buildingId = self->var_846;

            if (buildingId == 0xFFFF)
            {
                buildingId = self->rowHover;

                if (buildingId == 0xFFFF)
                    return;
            }

            auto buildingObj = ObjectManager::get<BuildingObject>(buildingId);

            Gfx::drawString_494BBF(*context, self->x + 3, self->y + self->height - 13, self->width - 19, Colour::black, StringIds::black_stringid, &buildingObj->name);
        }

        // 0x0049AB31
        static void onMouseUp(Ui::Window* self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case Common::widx::tab_town_list:
                case Common::widx::tab_build_town:
                case Common::widx::tab_build_buildings:
                case Common::widx::tab_build_misc_buildings:
                    Common::switchTab(self, widgetIndex);
                    break;

                case widx::rotate_object:
                    if (_buildingRotation < 3)
                        _buildingRotation++;
                    else
                        _buildingRotation = 0;
                    self->invalidate();
                    break;
            }
        }

        // 0x0049AD51
        static void onUpdate(Window* self)
        {
            if (!Input::hasFlag(Input::Flags::flag5))
            {
                auto cursor = Input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                Window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == self)
                {
                    xPos -= self->x;
                    xPos += 26;
                    yPos -= self->y;

                    if ((yPos < 42) || (xPos <= self->width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        WidgetIndex_t activeWidget = self->findWidgetAt(xPos, yPos);
                        if (activeWidget > Common::widx::panel)
                        {
                            self->savedView.mapX += 1;
                            if (self->savedView.mapX >= 8)
                            {
                                auto y = std::min(self->scrollAreas[0].contentHeight - 1 + 60, 500);
                                if (Ui::height() < 600)
                                {
                                    y = std::min(y, 276);
                                }
                                self->minWidth = windowSize.width;
                                self->minHeight = y;
                                self->maxWidth = windowSize.width;
                                self->maxHeight = y;
                            }
                            else
                            {
                                if (Input::state() != Input::State::scrollLeft)
                                {
                                    self->minWidth = windowSize.width;
                                    self->minHeight = windowSize.height;
                                    self->maxWidth = windowSize.width;
                                    self->maxHeight = windowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self->savedView.mapX = 0;
                    if (Input::state() != Input::State::scrollLeft)
                    {
                        self->minWidth = windowSize.width;
                        self->minHeight = windowSize.height;
                        self->maxWidth = windowSize.width;
                        self->maxHeight = windowSize.height;
                    }
                }
            }
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->currentTab + Common::widx::tab_town_list);
            if (!Input::isToolActive(self->type, self->number))
                WindowManager::close(self);
        }

        // 0x0049AB59
        static void onDropdown(Window* self, Ui::WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::object_colour)
                return;
            if (itemIndex == -1)
                return;

            _buildingColour = static_cast<Colour>(Dropdown::getItemArgument(itemIndex, 2));
            self->invalidate();
        }

        // 0x0049B37F
        static void removeBuildingGhost()
        {
            if (_buildingGhostPlaced)
            {
                GameCommands::BuildingRemovalArgs args;
                args.pos = _buildingGhostPos;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
                _buildingGhostPlaced = false;
            }
        }

        // 0x0049AD46
        static void onToolAbort(Window& self, const WidgetIndex_t widgetIndex)
        {
            removeBuildingGhost();
            Ui::Windows::hideGridlines();
        }

        // 0x0049B32A
        static currency32_t placeBuildingGhost(const GameCommands::BuildingPlacementArgs& placementArgs)
        {
            removeBuildingGhost();
            auto res = GameCommands::doCommand(placementArgs, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
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
            auto tile = Map::TileManager::get(*pos);
            const auto* surface = tile.surface();
            if (surface == nullptr)
            {
                return {};
            }

            auto z = surface->baseZ() * 4; // di
            if (surface->slope())
            {
                z += 16;
            }
            args.pos = Map::Pos3(pos->x, pos->y, z);
            args.type = townListWnd->rowHover;   // dl
            args.variation = _buildingVariation; // dh
            if (isEditorMode())
            {
                args.buildImmediately = true; // bh
            }
            return { args };
        }

        // 0x0049ABF0
        static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
        {
            Map::TileManager::mapInvalidateSelectionRect();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
            auto placementArgs = getBuildingPlacementArgsFromCursor(x, y);
            if (!placementArgs)
            {
                removeBuildingGhost();
                return;
            }

            Input::setMapSelectionFlags(Input::MapSelectionFlags::enable);
            Map::TileManager::setMapSelectionCorner(4);
            auto* building = ObjectManager::get<BuildingObject>(placementArgs->type);
            auto posB = Map::Pos2(placementArgs->pos) + (building->flags & BuildingObjectFlags::large_tile ? Map::Pos2(32, 32) : Map::Pos2(0, 0));
            Map::TileManager::setMapSelectionArea(placementArgs->pos, posB);
            Map::TileManager::mapInvalidateSelectionRect();

            if (_buildingGhostPlaced)
            {
                if (*_buildingGhostPos == placementArgs->pos && _buildingGhostRotation == placementArgs->rotation && _buildingGhostType == placementArgs->type)
                {
                    return;
                }
            }

            removeBuildingGhost();
            auto cost = placeBuildingGhost(*placementArgs);
            if (cost != dword_1135C34)
            {
                dword_1135C34 = cost;
                self.invalidate();
            }
        }

        // 0x0049ACBD
        static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
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
        static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
        {
            if (widgetIndex == widx::object_colour)
            {
                auto obj = ObjectManager::get<BuildingObject>(self->rowHover);
                Dropdown::showColour(self, &self->widgets[widgetIndex], obj->colours, _buildingColour, self->getColour(WindowColour::secondary));
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

            i = (i / 5) * rowHeight;

            self->scrollAreas[0].contentOffsetY = i;

            Ui::ScrollView::updateThumbs(self, widx::scrollview);
        }

        // 0x0049AF98
        static void onResize(Window* self)
        {
            self->invalidate();
            Ui::Size minWindowSize = { self->minWidth, self->minHeight };
            Ui::Size maxWindowSize = { self->maxWidth, self->maxHeight };
            bool hasResized = self->setSize(minWindowSize, maxWindowSize);
            if (hasResized)
                updateActiveThumb(self);
        }

        // 0x0049AE83
        static void getScrollSize(Ui::Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (4 + self->var_83C) / 5;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= rowHeight;
        }

        // 0x0049ABBB
        static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_building_list);
            return args;
        }

        // 0x0049AA1C
        static void drawScroll(Ui::Window& self, Gfx::Context& context, const uint32_t scrollIndex)
        {
            auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 3);
            Gfx::clearSingle(context, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self.var_83C; i++)
            {
                if (self.rowInfo[i] != self.rowHover)
                {
                    if (self.rowInfo[i] == self.var_846)
                    {
                        Gfx::drawRectInset(context, xPos, yPos, 112, 112, self.getColour(WindowColour::secondary).u8(), AdvancedColour::translucent_flag);
                    }
                }
                else
                {
                    Gfx::drawRectInset(context, xPos, yPos, 112, 112, self.getColour(WindowColour::secondary).u8(), (AdvancedColour::translucent_flag | AdvancedColour::outline_flag));
                }

                auto buildingObj = ObjectManager::get<BuildingObject>(self.rowInfo[i]);

                auto clipped = Gfx::clipContext(context, Ui::Rect(xPos + 1, yPos + 1, 110, 110));
                if (clipped)
                {
                    auto colour = *_buildingColour;
                    if (self.rowHover != self.rowInfo[i])
                    {
                        auto bit = Utility::bitScanReverse(buildingObj->colours);
                        colour = bit == -1 ? Colour::black : static_cast<Colour>(bit);
                    }

                    buildingObj->drawBuilding(&*clipped, _buildingRotation, 56, 96, colour);
                }

                xPos += 112;

                if (xPos >= 112 * 5) // full row
                {
                    xPos = 0;
                    yPos += 112;
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
                    auto bit = Utility::bitScanReverse(buildingObj->colours);
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
        static void onScrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            int16_t xPos = (x / 112);
            int16_t yPos = (y / 112) * 5;
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self->var_83C; i++)
            {
                auto rowInfo = self->rowInfo[i];
                index--;
                if (index < 0)
                {
                    self->rowHover = rowInfo;

                    if (self->currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                        _lastSelectedMiscBuilding = static_cast<uint8_t>(rowInfo);
                    else
                        _lastSelectedBuilding = static_cast<uint8_t>(rowInfo);

                    updateBuildingColours(self);

                    int32_t pan = (self->width >> 1) + self->x;
                    Map::Pos3 loc = { xPos, yPos, static_cast<int16_t>(pan) };

                    Audio::playSound(Audio::SoundId::clickDown, loc, pan);
                    self->savedView.mapX = -16;
                    dword_1135C34 = GameCommands::FAILURE;
                    _buildingVariation = 0;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x0049AEBA
        static void onScrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = y;
            auto i = 0;
            for (; i < self->var_83C; i++)
            {
                rowInfo = self->rowInfo[i];
                index--;
                if (index < 0)
                {
                    self->var_846 = rowInfo;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x0049ABC5
        static void onClose(Window* self)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();
        }

        // 0x0049AEA1
        static void event_08(Window* self)
        {
            if (self->var_846 != 0xFFFF)
            {
                self->var_846 = 0xFFFF;
                self->invalidate();
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
                    if (!(buildingObj->flags & BuildingObjectFlags::misc_building))
                        continue;
                    if ((buildingObj->flags & BuildingObjectFlags::is_headquarters) != 0)
                        continue;
                }
                else
                {
                    if ((buildingObj->flags & BuildingObjectFlags::misc_building) != 0)
                        continue;
                }
                self->rowInfo[buildingCount] = i;
                buildingCount++;
            }

            self->var_83C = buildingCount;
            auto rowHover = -1;

            auto lastSelectedBuilding = _lastSelectedBuilding;
            if (self->currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                lastSelectedBuilding = _lastSelectedMiscBuilding;

            if (lastSelectedBuilding != 0xFF)
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
            self->minWidth = windowSize.width;
            self->minHeight = windowSize.height;
            self->maxWidth = windowSize.width;
            self->maxWidth = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;

            auto tab = Common::widx::tab_build_buildings;
            if (self->currentTab == Common::widx::tab_build_misc_buildings - Common::widx::tab_town_list)
                tab = Common::widx::tab_build_misc_buildings;

            Input::toolSet(self, tab, CursorId::placeBuilding);
            Input::setFlag(Input::Flags::flag6);
            Ui::Windows::showGridlines();

            static loco_global<uint8_t, 0x01135C60> byte_1135C60;
            byte_1135C60 = 0;
            dword_1135C34 = GameCommands::FAILURE;
            self->var_83C = 0;
            self->rowHover = -1;
            self->var_846 = -1;

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

        static void prepareDraw(Window* self)
        {
            // Reset tab widgets if needed
            const auto& tabWidgets = tabInformationByTabOffset[self->currentTab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab
            self->activatedWidgets &= ~((1ULL << tab_town_list) | (1ULL << tab_build_town) | (1ULL << tab_build_buildings) | (1ULL << tab_build_misc_buildings));
            self->activatedWidgets |= (1ULL << Common::tabInformationByTabOffset[self->currentTab].widgetIndex);

            self->widgets[Common::widx::frame].right = self->width - 1;
            self->widgets[Common::widx::frame].bottom = self->height - 1;

            self->widgets[Common::widx::panel].right = self->width - 1;
            self->widgets[Common::widx::panel].bottom = self->height - 1;

            self->widgets[Common::widx::caption].right = self->width - 2;

            self->widgets[Common::widx::close_button].left = self->width - 15;
            self->widgets[Common::widx::close_button].right = self->width - 3;
        }

        // 0x0049B004 and 0x0049B00A
        static void repositionTabs(Window* self)
        {
            int16_t new_tab_x = self->widgets[widx::tab_town_list].left;
            int16_t tab_width = self->widgets[widx::tab_town_list].right - new_tab_x;

            for (auto& tabInfo : tabInformationByTabOffset)
            {
                if (self->isDisabled(tabInfo.widgetIndex))
                    continue;

                Widget& tab = self->widgets[tabInfo.widgetIndex];

                tab.left = new_tab_x;
                new_tab_x += tab_width;
                tab.right = new_tab_x++;
            }
        }

        // 0x0049B054
        static void drawTabs(Window* self, Gfx::Context* context)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Town List Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::toolbar_menu_towns;

                Widget::drawTab(self, context, imageId, widx::tab_town_list);
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
                    imageId += buildNewTownsImageIds[(self->frame_no / 2) % std::size(buildNewTownsImageIds)];
                else
                    imageId += buildNewTownsImageIds[0];

                Widget::drawTab(self, context, imageId, widx::tab_build_town);
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
                    imageId += buildBuildingsImageIds[(self->frame_no / 2) % std::size(buildBuildingsImageIds)];
                else
                    imageId += buildBuildingsImageIds[0];

                Widget::drawTab(self, context, imageId, widx::tab_build_buildings);
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
                    imageId += buildMiscBuildingsImageIds[(self->frame_no / 2) % std::size(buildMiscBuildingsImageIds)];
                else
                    imageId += buildMiscBuildingsImageIds[0];

                Widget::drawTab(self, context, imageId, widx::tab_build_misc_buildings);
            }
        }

        // 0x0049A2E2
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            self->currentTab = widgetIndex - widx::tab_town_list;
            self->frame_no = 0;
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
