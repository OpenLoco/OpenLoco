#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/BuildingObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Town.h"
#include "../TownManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Utility/numeric.hpp"
#include "../Widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::town_list
{
    static loco_global<uint32_t, 0x01135C34> dword_1135C34;
    static loco_global<colour_t, 0x01135C61> _buildingColour;
    static loco_global<uint8_t, 0x01135C63> _buildingRotation;
    static loco_global<uint8_t, 0x01135C65> byte_1135C65;
    static loco_global<uint8_t, 0x01135C66> _townSize;
    static loco_global<uint8_t, 0x00525FC8> _lastSelectedBuilding;
    static loco_global<uint8_t, 0x00525FC9> _lastSelectedMiscBuilding;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;

    namespace common
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

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                          \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, 155 }, widget_type::panel, 1),                                                               \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_town_list),                     \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_town),                   \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_buildings),              \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_misc_buildings)

        static void prepareDraw(window* self);
        static void repositionTabs(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void switchTab(window* self, widget_index widgetIndex);
        static void initEvents();
        static void refreshTownList(window* self);

    }

    namespace town_list
    {
        static const gfx::ui_size_t windowSize = { 600, 197 };
        static const gfx::ui_size_t maxDimensions = { 600, 900 };
        static const gfx::ui_size_t minDimensions = { 192, 100 };

        static const uint8_t rowHeight = 10;

        enum widx
        {
            sort_town_name = 8,
            sort_town_type,
            sort_town_population,
            sort_town_stations,
            scrollview,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << sort_town_name) | (1 << sort_town_type) | (1 << sort_town_population) | (1 << sort_town_stations) | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(600, 197, string_ids::title_towns),
            makeWidget({ 4, 43 }, { 200, 12 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_by_name),
            makeWidget({ 204, 43 }, { 80, 12 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_town_type),
            makeWidget({ 284, 43 }, { 70, 12 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_population),
            makeWidget({ 354, 43 }, { 70, 12 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_stations),
            makeWidget({ 3, 56 }, { 594, 126 }, widget_type::scrollview, 1, 2),
            widgetEnd(),
        };

        static window_event_list events;

        enum SortMode : uint16_t
        {
            Name,
            Type,
            Population,
            Stations,
        };

        // 0x00499F53
        static void prepareDraw(ui::window* self)
        {
            common::prepareDraw(self);

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
            self->widgets[widx::sort_town_name].text = self->sort_mode == SortMode::Name ? string_ids::table_header_name_desc : string_ids::table_header_name;
            self->widgets[widx::sort_town_type].text = self->sort_mode == SortMode::Type ? string_ids::table_header_town_type_desc : string_ids::table_header_town_type;
            self->widgets[widx::sort_town_population].text = self->sort_mode == SortMode::Population ? string_ids::table_header_population_desc : string_ids::table_header_population;
            self->widgets[widx::sort_town_stations].text = self->sort_mode == SortMode::Stations ? string_ids::table_header_stations_desc : string_ids::table_header_stations;

            common::repositionTabs(self);
        }

        // 0x0049A0F8
        static void drawScroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto shade = colour::getShade(self->colours[1], 3);
            gfx::clearSingle(*dpi, shade);

            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                town_id_t townId = self->row_info[i];

                // Skip items outside of view, or irrelevant to the current filter.
                if (yPos + rowHeight < dpi->y || yPos >= yPos + rowHeight + dpi->height || townId == (uint16_t)-1)
                {
                    yPos += rowHeight;
                    continue;
                }

                string_id text_colour_id = string_ids::black_stringid;

                // Highlight selection.
                if (townId == self->row_hover)
                {
                    gfx::drawRect(dpi, 0, yPos, self->width, rowHeight, 0x2000030);
                    text_colour_id = string_ids::wcolour2_stringid;
                }

                if (townId == 0xFFFF)
                    continue;
                auto town = townmgr::get(townId);

                // Town Name
                {
                    auto args = FormatArguments();
                    args.push(town->name);

                    gfx::drawString_494BBF(*dpi, 0, yPos, 198, colour::black, text_colour_id, &args);
                }
                // Town Type
                {
                    auto args = FormatArguments();
                    args.push(town->getTownSizeString());

                    gfx::drawString_494BBF(*dpi, 200, yPos, 278, colour::black, text_colour_id, &args);
                }
                // Town Population
                {
                    auto args = FormatArguments();
                    args.push(string_ids::int_32);
                    args.push(town->population);

                    gfx::drawString_494BBF(*dpi, 280, yPos, 68, colour::black, text_colour_id, &args);
                }
                // Town Stations
                {
                    auto args = FormatArguments();
                    args.push(string_ids::int_32);
                    args.push(town->num_stations);

                    gfx::drawString_494BBF(*dpi, 350, yPos, 68, colour::black, text_colour_id, &args);
                }
                yPos += rowHeight;
            }
        }

        // 0x0049A0A7
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            auto args = FormatArguments();
            auto xPos = self->x + 4;
            auto yPos = self->y + self->height - 12;

            if (self->var_83C == 1)
                args.push(string_ids::status_towns_singular);
            else
                args.push(string_ids::status_towns_plural);
            args.push(self->var_83C);

            gfx::drawString_494B3F(*dpi, xPos, yPos, colour::black, string_ids::black_stringid, &args);
        }

        // 0x0049A27F
        static void onMouseUp(ui::window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_town_list:
                case common::widx::tab_build_town:
                case common::widx::tab_build_buildings:
                case common::widx::tab_build_misc_buildings:
                    common::switchTab(self, widgetIndex);
                    break;

                case widx::sort_town_name:
                case widx::sort_town_type:
                case widx::sort_town_population:
                case widx::sort_town_stations:
                {
                    auto sortMode = widgetIndex - widx::sort_town_name;
                    if (self->sort_mode == sortMode)
                        return;

                    self->sort_mode = sortMode;
                    self->invalidate();
                    self->var_83C = 0;
                    self->row_hover = -1;

                    common::refreshTownList(self);
                    break;
                }
            }
        }

        // 0x0049A56D
        static void onScrollMouseDown(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            uint16_t currentRow = y / rowHeight;
            if (currentRow > self->var_83C)
                return;

            int16_t currentTown = self->row_info[currentRow];
            if (currentTown == -1)
                return;

            windows::town::open(currentTown);
        }

        // 0x0049A532
        static void onScrollMouseOver(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            self->flags &= ~(window_flags::not_scroll_view);

            uint16_t currentRow = y / rowHeight;
            int16_t currentTown = -1;

            if (currentRow < self->var_83C)
                currentTown = self->row_info[currentRow];

            if (self->row_hover == currentTown)
                return;

            self->row_hover = currentTown;
            self->invalidate();
        }

        // 0x00499EC9
        static bool orderByName(const openloco::town& lhs, const openloco::town& rhs)
        {
            char lhsString[256] = { 0 };
            stringmgr::formatString(lhsString, lhs.name);

            char rhsString[256] = { 0 };
            stringmgr::formatString(rhsString, rhs.name);

            return strcmp(lhsString, rhsString) < 0;
        }

        // 0x00499F28
        static bool orderByPopulation(const openloco::town& lhs, const openloco::town& rhs)
        {
            auto lhsPopulation = lhs.population;

            auto rhsPopulation = rhs.population;

            return rhsPopulation < lhsPopulation;
        }

        // 0x00499F0A Left this in to match the x86 code. can be replaced with orderByPopulation
        static bool orderByType(const openloco::town& lhs, const openloco::town& rhs)
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
        static bool orderByStations(const openloco::town& lhs, const openloco::town& rhs)
        {
            auto lhsStations = lhs.num_stations;

            auto rhsStations = rhs.num_stations;

            return rhsStations < lhsStations;
        }

        // 0x00499EC9, 0x00499F0A, 0x00499F28, 0x00499F3B
        static bool getOrder(const SortMode mode, openloco::town& lhs, openloco::town& rhs)
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
        static void updateTownList(window* self)
        {
            auto chosenTown = -1;

            auto i = -1;

            for (auto& town : townmgr::towns())
            {
                i++;
                if (town.empty())
                    continue;

                if ((town.flags & town_flags::sorted) != 0)
                    continue;

                if (chosenTown == -1)
                {
                    chosenTown = i;
                    continue;
                }

                if (getOrder(SortMode(self->sort_mode), town, *townmgr::get(chosenTown)))
                {
                    chosenTown = i;
                }
            }

            if (chosenTown != -1)
            {
                bool shouldInvalidate = false;

                townmgr::get(chosenTown)->flags |= town_flags::sorted;

                if (chosenTown != self->row_info[self->row_count])
                {
                    self->row_info[self->row_count] = chosenTown;
                    shouldInvalidate = true;
                }

                self->row_count += 1;
                if (self->row_count > self->var_83C)
                {
                    self->var_83C = self->row_count;
                    shouldInvalidate = true;
                }

                if (shouldInvalidate)
                {
                    self->invalidate();
                }
            }
            else
            {
                if (self->var_83C != self->row_count)
                {
                    self->var_83C = self->row_count;
                    self->invalidate();
                }

                common::refreshTownList(self);
            }
        }

        // 0x0049A4A0
        static void onUpdate(window* self)
        {
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->current_tab + common::widx::tab_town_list);

            // Add three towns every tick.
            updateTownList(self);
            updateTownList(self);
            updateTownList(self);
        }

        // 0x0049A4D0
        static void event_08(window* self)
        {
            self->flags |= window_flags::not_scroll_view;
        }

        // 0x0049A4D8
        static void event_09(window* self)
        {
            if (!(self->flags & window_flags::not_scroll_view))
                return;

            if (self->row_hover == -1)
                return;

            self->row_hover = -1;
            self->invalidate();
        }

        // 0x0049A4FA
        static void getScrollSize(ui::window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = rowHeight * self->var_83C;
        }

        // 0x00491841
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_town_list);
        }

        // 0x004919A4
        static ui::cursor_id cursor(window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / rowHeight;
            if (currentIndex < self->var_83C && self->row_info[currentIndex] != -1)
                return cursor_id::hand_pointer;

            return fallback;
        }

        // 0x0049A37E
        static void tabReset(window* self)
        {
            self->min_width = minDimensions.width;
            self->min_height = minDimensions.height;
            self->max_width = maxDimensions.width;
            self->max_height = maxDimensions.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
            self->var_83C = 0;
            self->row_hover = -1;

            common::refreshTownList(self);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.cursor = cursor;
            events.draw_scroll = drawScroll;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.get_scroll_size = getScrollSize;
            events.on_mouse_up = onMouseUp;
            events.on_update = onUpdate;
            events.scroll_mouse_down = onScrollMouseDown;
            events.scroll_mouse_over = onScrollMouseOver;
            events.prepare_draw = prepareDraw;
            events.tooltip = tooltip;
        }
    }

    // 0x00499C83
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::townList, 0);
        if (window != nullptr)
        {
            window->callOnMouseUp(common::widx::tab_town_list);
        }
        else
        {
            // 0x00499CFC
            auto origin = gfx::point_t(ui::width() - town_list::windowSize.width, 30);

            window = WindowManager::createWindow(
                WindowType::townList,
                origin,
                town_list::windowSize,
                window_flags::resizable,
                &town_list::events);

            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;
            window->sort_mode = 0;
            window->var_83C = 0;
            window->row_hover = -1;

            common::refreshTownList(window);

            WindowManager::sub_4CEE0B(window);

            window->min_width = town_list::minDimensions.width;
            window->min_height = town_list::minDimensions.height;
            window->max_width = town_list::maxDimensions.width;
            window->max_height = town_list::maxDimensions.height;
            window->flags |= window_flags::resizable;

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[0] = skin->colour_0B;
            window->colours[1] = skin->colour_0C;

            // 0x00499CFC end

            // TODO: only needs to be called once.
            window->width = town_list::windowSize.width;
            window->height = town_list::windowSize.height;

            common::initEvents();

            window->invalidate();

            window->widgets = town_list::widgets;
            window->enabled_widgets = town_list::enabledWidgets;

            if (isEditorMode())
                window->disabled_widgets = 0;
            else
                window->disabled_widgets |= (1 << common::widx::tab_build_town) | (1 << common::widx::tab_build_buildings) | (1 << common::widx::tab_build_misc_buildings);

            window->activated_widgets = 0;
            window->holdable_widgets = 0;

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();
        }
        return window;
    }

    namespace build_towns
    {
        static const gfx::ui_size_t windowSize = { 220, 87 };

        enum widx
        {
            current_size = 8,
            select_size,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << current_size) | (1 << select_size);

        widget_t widgets[] = {
            commonWidgets(220, 87, string_ids::title_build_new_towns),
            makeWidget({ 100, 45 }, { 117, 12 }, widget_type::wt_18, 1, image_ids::null, string_ids::tooltip_select_town_size),
            makeWidget({ 205, 46 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x0049A59A
        static void prepareDraw(ui::window* self)
        {
            common::prepareDraw(self);

            common::repositionTabs(self);

            static string_id townSizeNames[9] = {
                string_ids::tooltip_select_town_size,
                string_ids::town_size_1,
                string_ids::town_size_2,
                string_ids::town_size_3,
                string_ids::town_size_4,
                string_ids::town_size_5,
                string_ids::town_size_6,
                string_ids::town_size_7,
                string_ids::town_size_8,
            };

            self->widgets[widx::current_size].text = townSizeNames[_townSize];
        }

        // 0x0049A627
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            gfx::drawString_494B3F(*dpi, self->x + 3, self->y + self->widgets[widx::current_size].top + 1, colour::black, string_ids::town_size_label);

            gfx::drawString_494B3F(*dpi, self->x + 3, self->y + self->height - 13, colour::black, string_ids::select_town_size);
        }

        // 0x0049A675
        static void onMouseUp(ui::window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_town_list:
                case common::widx::tab_build_town:
                case common::widx::tab_build_buildings:
                case common::widx::tab_build_misc_buildings:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0049A7F2
        static void onUpdate(window* self)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->current_tab + common::widx::tab_town_list);
            if ((!input::hasFlag(input::input_flags::tool_active)) || self->type != _toolWindowType || self->number != _toolWindowNumber)
            {
                WindowManager::close(self);
            }
        }

        // 0x0049A697
        static void onDropdown(window* self, ui::widget_index widgetIndex, int16_t itemIndex)
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
        static void onToolAbort(window& self, const widget_index widgetIndex)
        {
            ui::windows::hideGridlines();
        }

        // 0x0049A710
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049A710, regs);
        }

        // 0x0049A75E
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049A75E, regs);
        }

        // 0x0049A69E
        static void populateTownSizeSelect(window* self, widget_t* widget)
        {
            registers regs;
            regs.edi = (int32_t)widget;
            regs.esi = (int32_t)self;

            call(0x0049A69E, regs);
        }

        // 0x0049A690
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == widx::select_size)
                populateTownSizeSelect(self, &self->widgets[widgetIndex]);
        }

        // 0x0049A844
        static void onResize(window* self)
        {
            self->setSize(windowSize, windowSize);
        }

        // 0x0049A7C7
        static void onClose(window* self)
        {
            if (input::isToolActive(self->type, self->number))
                input::toolCancel();
        }

        // 0x0049A3BE
        static void tabReset(window* self)
        {
            self->min_width = windowSize.width;
            self->min_height = windowSize.height;
            self->max_width = windowSize.width;
            self->max_width = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;
            input::toolSet(self, common::widx::tab_build_town, 38);
            input::setFlag(input::input_flags::flag6);
            ui::windows::showGridlines();
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_close = onClose;
            events.on_dropdown = onDropdown;
            events.on_resize = onResize;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.on_mouse_up = onMouseUp;
            events.on_mouse_down = onMouseDown;
            events.on_update = onUpdate;
            events.prepare_draw = prepareDraw;
            events.on_tool_abort = onToolAbort;
        }
    }

    namespace build_buildings
    {
        static const gfx::ui_size_t windowSize = { 600, 172 };

        static const uint8_t rowHeight = 112;

        enum widx
        {
            scrollview = 8,
            rotate_object,
            object_colour,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview) | (1 << rotate_object) | (1 << object_colour);

        widget_t widgets[] = {
            commonWidgets(640, 172, string_ids::title_build_new_buildings),
            makeWidget({ 2, 45 }, { 573, 112 }, widget_type::scrollview, 1, 2),
            makeWidget({ 575, 46 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rotate_object, string_ids::rotate_object_90),
            makeWidget({ 579, 91 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_object_colour),
            widgetEnd(),
        };

        static window_event_list events;

        // 0x0049A8A6
        static void prepareDraw(ui::window* self)
        {
            self->widgets[widx::object_colour].image = (1 << 30) | gfx::recolour(image_ids::colour_swatch_recolourable, _buildingColour);
            self->widgets[widx::object_colour].type = widget_type::none;

            if (self->row_hover != -1)
            {
                auto buildingObj = objectmgr::get<building_object>(self->row_hover);
                if (buildingObj->colours != 0)
                    self->widgets[widx::object_colour].type = widget_type::wt_10;
            }

            common::prepareDraw(self);

            self->widgets[widx::scrollview].right = self->width - 26;
            self->widgets[widx::scrollview].bottom = self->height - 14;

            self->widgets[widx::rotate_object].left = self->width - 25;
            self->widgets[widx::rotate_object].right = self->width - 2;

            self->widgets[widx::object_colour].left = self->width - 21;
            self->widgets[widx::object_colour].right = self->width - 6;

            self->widgets[common::widx::caption].text = string_ids::title_build_new_buildings;

            if (self->current_tab == common::widx::tab_build_misc_buildings - common::widx::tab_town_list)
                self->widgets[common::widx::caption].text = string_ids::title_build_new_misc_buildings;

            common::repositionTabs(self);
        }

        // 0x0049A9C2
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto buildingId = self->var_846;

            if (buildingId == 0xFFFF)
            {
                buildingId = self->row_hover;

                if (buildingId == 0xFFFF)
                    return;
            }

            auto buildingObj = objectmgr::get<building_object>(buildingId);

            gfx::drawString_494BBF(*dpi, self->x + 3, self->y + self->height - 13, self->width - 19, colour::black, string_ids::black_stringid, &buildingObj->name);
        }

        // 0x0049AB31
        static void onMouseUp(ui::window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_town_list:
                case common::widx::tab_build_town:
                case common::widx::tab_build_buildings:
                case common::widx::tab_build_misc_buildings:
                    common::switchTab(self, widgetIndex);
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
        static void onUpdate(window* self)
        {
            if (!input::hasFlag(input::input_flags::flag5))
            {
                auto cursor = input::getMouseLocation();
                auto xPos = cursor.x;
                auto yPos = cursor.y;
                window* activeWindow = WindowManager::findAt(xPos, yPos);
                if (activeWindow == self)
                {
                    xPos -= self->x;
                    xPos += 26;
                    yPos -= self->y;

                    if ((yPos < 42) || (xPos <= self->width))
                    {
                        xPos = cursor.x;
                        yPos = cursor.y;
                        widget_index activeWidget = self->findWidgetAt(xPos, yPos);
                        if (activeWidget > common::widx::panel)
                        {
                            self->saved_view.mapX += 1;
                            if (self->saved_view.mapX >= 8)
                            {
                                auto y = std::min(self->scroll_areas[0].contentHeight - 1 + 60, 500);
                                if (ui::height() < 600)
                                {
                                    y = std::min(y, 276);
                                }
                                self->min_width = windowSize.width;
                                self->min_height = y;
                                self->max_width = windowSize.width;
                                self->max_height = y;
                            }
                            else
                            {
                                if (input::state() != input::input_state::scroll_left)
                                {
                                    self->min_width = windowSize.width;
                                    self->min_height = windowSize.height;
                                    self->max_width = windowSize.width;
                                    self->max_height = windowSize.height;
                                }
                            }
                        }
                    }
                }
                else
                {
                    self->saved_view.mapX = 0;
                    if (input::state() != input::input_state::scroll_left)
                    {
                        self->min_width = windowSize.width;
                        self->min_height = windowSize.height;
                        self->max_width = windowSize.width;
                        self->max_height = windowSize.height;
                    }
                }
            }
            self->frame_no++;

            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->current_tab + common::widx::tab_town_list);
            if (!input::isToolActive(self->type, self->number))
                WindowManager::close(self);
        }

        // 0x0049AB59
        static void onDropdown(window* self, ui::widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex != widx::object_colour)
                return;
            if (itemIndex == -1)
                return;

            _buildingColour = dropdown::getItemArgument(itemIndex, 4);
            self->invalidate();
        }

        // 0x0049B37F
        static void sub_49B37F()
        {
            registers regs;
            call(0x0049B37F, regs);
        }

        // 0x0049AD46
        static void onToolAbort(window& self, const widget_index widgetIndex)
        {
            sub_49B37F();
            ui::windows::hideGridlines();
        }

        // 0x0049ABF0
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049ABF0, regs);
        }

        // 0x0049ACBD
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049ACBD, regs);
        }

        // 0x0049AB52
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == widx::object_colour)
            {
                registers regs;
                regs.edx = widgetIndex;
                regs.esi = (int32_t)self;
                regs.edi = (int32_t)&self->widgets[widgetIndex];
                call(0x0049AB72, regs);
            }
        }

        // 0x0049B2B5
        static void updateActiveThumb(window* self)
        {
            uint16_t scrollHeight = 0;
            self->callGetScrollSize(0, 0, &scrollHeight);
            self->scroll_areas[0].contentHeight = scrollHeight;

            auto i = 0;
            for (; i <= self->var_83C; i++)
            {
                if (self->row_info[i] == self->row_hover)
                    break;
            }

            if (i >= self->var_83C)
                i = 0;

            i = (i / 5) * rowHeight;

            self->scroll_areas[0].contentOffsetY = i;

            ui::scrollview::updateThumbs(self, widx::scrollview);
        }

        // 0x0049AF98
        static void onResize(window* self)
        {
            self->invalidate();
            gfx::ui_size_t minWindowSize = { self->min_width, self->min_height };
            gfx::ui_size_t maxWindowSize = { self->max_width, self->max_height };
            bool hasResized = self->setSize(minWindowSize, maxWindowSize);
            if (hasResized)
                updateActiveThumb(self);
        }

        // 0x0049AE83
        static void getScrollSize(ui::window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (4 + self->var_83C) / 5;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= rowHeight;
        }

        // 0x0049ABBB
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_building_list);
        }

        // 0x0042DB95
        static void drawBuildingThumb(gfx::drawpixelinfo_t* clipped, const openloco::building_object* buildingObj, uint8_t buildingRotation, int16_t x, int16_t y, colour_t colour)
        {
            registers regs;
            regs.cx = x;
            regs.dx = y;
            regs.esi = colour;
            regs.eax = buildingRotation;
            regs.edi = (uint32_t)clipped;
            regs.ebp = (uint32_t)buildingObj;
            call(0x0042DB95, regs);
        }

        // 0x0049AA1C
        static void drawScroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto shade = colour::getShade(self->colours[1], 3);
            gfx::clearSingle(*dpi, shade);

            uint16_t xPos = 0;
            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                if (self->row_info[i] != self->row_hover)
                {
                    if (self->row_info[i] == self->var_846)
                    {
                        gfx::drawRectInset(dpi, xPos, yPos, 112, 112, self->colours[1], colour::translucent_flag);
                    }
                }
                else
                {
                    gfx::drawRectInset(dpi, xPos, yPos, 112, 112, self->colours[1], (colour::translucent_flag | colour::outline_flag));
                }

                auto buildingObj = objectmgr::get<building_object>(self->row_info[i]);

                gfx::drawpixelinfo_t* clipped = nullptr;

                if (gfx::clipDrawpixelinfo(&clipped, dpi, xPos + 1, yPos + 1, 110, 110))
                {
                    colour_t colour = _buildingColour;
                    if (self->row_hover != self->row_info[i])
                    {
                        colour = utility::bitScanReverse(buildingObj->colours);
                        if (colour == 0xFF)
                            colour = 0;
                    }

                    drawBuildingThumb(clipped, buildingObj, _buildingRotation, 56, 96, colour);
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
        static void updateBuildingColours(window* self)
        {
            if (self->row_hover != -1)
            {
                auto buildingObj = objectmgr::get<building_object>(self->row_hover);
                if (buildingObj->colours != 0)
                {
                    colour_t colour = utility::bitScanReverse(buildingObj->colours);
                    if (colour == 0xFF)
                        colour = 0;
                    _buildingColour = colour;
                }
            }
        }

        static int getRowIndex(int16_t x, int16_t y)
        {
            return (x / 112) + (y / 112) * 5;
        }

        // 0x0049AEFD
        static void onScrollMouseDown(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            int16_t xPos = (x / 112);
            int16_t yPos = (y / 112) * 5;
            auto index = getRowIndex(x, y);

            for (auto i = 0; i < self->var_83C; i++)
            {
                auto rowInfo = self->row_info[i];
                index--;
                if (index < 0)
                {
                    self->row_hover = rowInfo;

                    if (self->current_tab == common::widx::tab_build_misc_buildings - common::widx::tab_town_list)
                        _lastSelectedMiscBuilding = static_cast<uint8_t>(rowInfo);
                    else
                        _lastSelectedBuilding = static_cast<uint8_t>(rowInfo);

                    updateBuildingColours(self);

                    int32_t pan = (self->width >> 1) + self->x;
                    loc16 loc = { xPos, yPos, static_cast<int16_t>(pan) };

                    audio::playSound(audio::sound_id::click_down, loc, pan);
                    self->saved_view.mapX = -16;
                    dword_1135C34 = 0x80000000;
                    byte_1135C65 = 0;
                    self->invalidate();
                    break;
                }
            }
        }

        // 0x0049AEBA
        static void onScrollMouseOver(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            auto index = getRowIndex(x, y);
            uint16_t rowInfo = y;
            auto i = 0;
            for (; i < self->var_83C; i++)
            {
                rowInfo = self->row_info[i];
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
        static void onClose(window* self)
        {
            if (input::isToolActive(self->type, self->number))
                input::toolCancel();
        }

        // 0x0049AEA1
        static void event_08(window* self)
        {
            if (self->var_846 != 0xFFFF)
            {
                self->var_846 = 0xFFFF;
                self->invalidate();
            }
        }

        // 0x0049B206
        static void updateBuildingList(window* self)
        {
            auto buildingCount = 0;
            for (auto i = 0; i < 128; i++)
            {
                auto buildingObj = objectmgr::get<building_object>(i);
                if (buildingObj == nullptr)
                    continue;
                if (self->current_tab == common::widx::tab_build_misc_buildings - common::widx::tab_town_list)
                {
                    if (!(buildingObj->flags & building_object_flags::misc_building))
                        continue;
                    if ((buildingObj->flags & building_object_flags::is_headquarters) != 0)
                        continue;
                }
                else
                {
                    if ((buildingObj->flags & building_object_flags::misc_building) != 0)
                        continue;
                }
                self->row_info[buildingCount] = i;
                buildingCount++;
            }

            self->var_83C = buildingCount;
            auto rowHover = -1;

            auto lastSelectedBuilding = _lastSelectedBuilding;
            if (self->current_tab == common::widx::tab_build_misc_buildings - common::widx::tab_town_list)
                lastSelectedBuilding = _lastSelectedMiscBuilding;

            if (lastSelectedBuilding != 0xFF)
            {
                for (auto i = 0; i <= self->var_83C; i++)
                {
                    if (lastSelectedBuilding == self->row_info[i])
                    {
                        rowHover = lastSelectedBuilding;
                        break;
                    }
                }
            }

            if (rowHover == -1 && self->var_83C != 0)
            {
                rowHover = self->row_info[0];
            }

            self->row_hover = rowHover;
            updateActiveThumb(self);
            updateBuildingColours(self);
        }

        // 0x0049A3FF
        static void tabReset(window* self)
        {
            self->min_width = windowSize.width;
            self->min_height = windowSize.height;
            self->max_width = windowSize.width;
            self->max_width = windowSize.height;
            self->width = windowSize.width;
            self->height = windowSize.height;

            auto tab = common::widx::tab_build_buildings;
            if (self->current_tab == common::widx::tab_build_misc_buildings - common::widx::tab_town_list)
                tab = common::widx::tab_build_misc_buildings;

            input::toolSet(self, tab, 39);
            input::setFlag(input::input_flags::flag6);
            ui::windows::showGridlines();

            static loco_global<uint8_t, 0x01135C60> byte_1135C60;
            byte_1135C60 = 0;
            dword_1135C34 = 0x80000000;
            self->var_83C = 0;
            self->row_hover = -1;
            self->var_846 = -1;

            updateBuildingList(self);
            updateBuildingColours(self);

            byte_1135C65 = 0;
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_close = onClose;
            events.on_resize = onResize;
            events.draw_scroll = drawScroll;
            events.event_08 = event_08;
            events.on_dropdown = onDropdown;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.on_mouse_down = onMouseDown;
            events.get_scroll_size = getScrollSize;
            events.on_mouse_up = onMouseUp;
            events.on_update = onUpdate;
            events.scroll_mouse_down = onScrollMouseDown;
            events.scroll_mouse_over = onScrollMouseOver;
            events.prepare_draw = prepareDraw;
            events.tooltip = tooltip;
            events.on_tool_abort = onToolAbort;
        }
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { town_list::widgets, widx::tab_town_list, &town_list::events, town_list::enabledWidgets },
            { build_towns::widgets, widx::tab_build_town, &build_towns::events, build_towns::enabledWidgets },
            { build_buildings::widgets, widx::tab_build_buildings, &build_buildings::events, build_buildings::enabledWidgets },
            { build_buildings::widgets, widx::tab_build_misc_buildings, &build_buildings::events, build_buildings::enabledWidgets },
        };

        static void prepareDraw(window* self)
        {
            // Reset tab widgets if needed
            const auto& tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
            }

            // Activate the current tab
            self->activated_widgets &= ~((1ULL << tab_town_list) | (1ULL << tab_build_town) | (1ULL << tab_build_buildings) | (1ULL << tab_build_misc_buildings));
            self->activated_widgets |= (1ULL << common::tabInformationByTabOffset[self->current_tab].widgetIndex);

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;
        }

        // 0x0049B004 and 0x0049B00A
        static void repositionTabs(window* self)
        {
            int16_t new_tab_x = self->widgets[widx::tab_town_list].left;
            int16_t tab_width = self->widgets[widx::tab_town_list].right - new_tab_x;

            for (auto& tabInfo : tabInformationByTabOffset)
            {
                if (self->isDisabled(tabInfo.widgetIndex))
                    continue;

                widget_t& tab = self->widgets[tabInfo.widgetIndex];

                tab.left = new_tab_x;
                new_tab_x += tab_width;
                tab.right = new_tab_x++;
            }
        }

        // 0x0049B054
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Town List Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::toolbar_menu_towns;

                widget::draw_tab(self, dpi, imageId, widx::tab_town_list);
            }

            // Build New Towns Tab
            {
                static const uint32_t buildNewTownsImageIds[] = {
                    interface_skin::image_ids::build_town_frame_0,
                    interface_skin::image_ids::build_town_frame_1,
                    interface_skin::image_ids::build_town_frame_2,
                    interface_skin::image_ids::build_town_frame_3,
                    interface_skin::image_ids::build_town_frame_4,
                    interface_skin::image_ids::build_town_frame_5,
                    interface_skin::image_ids::build_town_frame_6,
                    interface_skin::image_ids::build_town_frame_7,
                    interface_skin::image_ids::build_town_frame_8,
                    interface_skin::image_ids::build_town_frame_9,
                    interface_skin::image_ids::build_town_frame_10,
                    interface_skin::image_ids::build_town_frame_11,
                    interface_skin::image_ids::build_town_frame_12,
                    interface_skin::image_ids::build_town_frame_13,
                    interface_skin::image_ids::build_town_frame_14,
                    interface_skin::image_ids::build_town_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_build_town - widx::tab_town_list)
                    imageId += buildNewTownsImageIds[(self->frame_no / 2) % std::size(buildNewTownsImageIds)];
                else
                    imageId += buildNewTownsImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_build_town);
            }

            // Build New Buildings Tab
            {
                static const uint32_t buildBuildingsImageIds[] = {
                    interface_skin::image_ids::build_buildings_frame_0,
                    interface_skin::image_ids::build_buildings_frame_1,
                    interface_skin::image_ids::build_buildings_frame_2,
                    interface_skin::image_ids::build_buildings_frame_3,
                    interface_skin::image_ids::build_buildings_frame_4,
                    interface_skin::image_ids::build_buildings_frame_5,
                    interface_skin::image_ids::build_buildings_frame_6,
                    interface_skin::image_ids::build_buildings_frame_7,
                    interface_skin::image_ids::build_buildings_frame_8,
                    interface_skin::image_ids::build_buildings_frame_9,
                    interface_skin::image_ids::build_buildings_frame_10,
                    interface_skin::image_ids::build_buildings_frame_11,
                    interface_skin::image_ids::build_buildings_frame_12,
                    interface_skin::image_ids::build_buildings_frame_13,
                    interface_skin::image_ids::build_buildings_frame_14,
                    interface_skin::image_ids::build_buildings_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_build_buildings - widx::tab_town_list)
                    imageId += buildBuildingsImageIds[(self->frame_no / 2) % std::size(buildBuildingsImageIds)];
                else
                    imageId += buildBuildingsImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_build_buildings);
            }

            // Build New Misc Buildings Tab
            {
                static const uint32_t buildMiscBuildingsImageIds[] = {
                    interface_skin::image_ids::build_misc_buildings_frame_0,
                    interface_skin::image_ids::build_misc_buildings_frame_1,
                    interface_skin::image_ids::build_misc_buildings_frame_2,
                    interface_skin::image_ids::build_misc_buildings_frame_3,
                    interface_skin::image_ids::build_misc_buildings_frame_4,
                    interface_skin::image_ids::build_misc_buildings_frame_5,
                    interface_skin::image_ids::build_misc_buildings_frame_6,
                    interface_skin::image_ids::build_misc_buildings_frame_7,
                    interface_skin::image_ids::build_misc_buildings_frame_8,
                    interface_skin::image_ids::build_misc_buildings_frame_9,
                    interface_skin::image_ids::build_misc_buildings_frame_10,
                    interface_skin::image_ids::build_misc_buildings_frame_11,
                    interface_skin::image_ids::build_misc_buildings_frame_12,
                    interface_skin::image_ids::build_misc_buildings_frame_13,
                    interface_skin::image_ids::build_misc_buildings_frame_14,
                    interface_skin::image_ids::build_misc_buildings_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_build_misc_buildings - widx::tab_town_list)
                    imageId += buildMiscBuildingsImageIds[(self->frame_no / 2) % std::size(buildMiscBuildingsImageIds)];
                else
                    imageId += buildMiscBuildingsImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_build_misc_buildings);
            }
        }

        //0x0049A2E2
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::isToolActive(self->type, self->number))
                input::toolCancel();

            self->current_tab = widgetIndex - widx::tab_town_list;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_town_list];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            if (isEditorMode())
                self->disabled_widgets = 0;
            else
                self->disabled_widgets |= (1 << common::widx::tab_build_town) | (1 << common::widx::tab_build_buildings) | (1 << common::widx::tab_build_misc_buildings);

            self->invalidate();

            if (self->current_tab == widx::tab_town_list - widx::tab_town_list)
                town_list::tabReset(self);
            if (self->current_tab == widx::tab_build_town - widx::tab_town_list)
                build_towns::tabReset(self);
            if (self->current_tab == widx::tab_build_buildings - widx::tab_town_list || self->current_tab == widx::tab_build_misc_buildings - widx::tab_town_list)
                build_buildings::tabReset(self);

            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x00499DDE
        static void refreshTownList(window* self)
        {
            self->row_count = 0;

            for (auto& town : townmgr::towns())
            {
                if (town.empty())
                    continue;

                town.flags &= ~town_flags::sorted;
            }
        }

        static void initEvents()
        {
            town_list::initEvents();
            build_towns::initEvents();
            build_buildings::initEvents();
        }
    }
}
