#include "../config.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/building_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../town.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/scrollview.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::town_list
{
    loco_global<int8_t, 0x00F2533F> _gridlines_state;
    static loco_global<uint32_t, 0x01135C34> dword_1135C34;
    static loco_global<uint8_t, 0x01135C60> byte_1135C60;
    static loco_global<uint8_t, 0x01135C61> byte_1135C61;
    static loco_global<uint8_t, 0x01135C65> byte_1135C65;
    static loco_global<uint8_t, 0x01135C66> byte_1135C66;
    static loco_global<uint8_t, 0x00525FC8> byte_525FC8;
    static loco_global<uint8_t, 0x00525FC9> byte_525FC9;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint16_t, 0x00523390> _toolWindowNumber;

    namespace common
    {
        static const gfx::ui_size_t window_size = { 600, 197 };
        static const gfx::ui_size_t max_dimensions = { 600, 900 };
        static const gfx::ui_size_t min_dimensions = { 192, 100 };

        static const uint8_t rowHeight = 10;

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

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 154 }, widget_type::panel, 1),                                                               \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_town_list),                    \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_town),                  \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_buildings),             \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_misc_buildings)

        static void prepare_draw(window* self);
        static void repositionTabs(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void switchTab(window* self, widget_index widgetIndex);
        static void init_events();
        static void refreshTownList(window* self);
        static void hideGridlines();
        static void showGridlines();

    }

    namespace town_list
    {
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
            make_widget({ 4, 44 }, { 199, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_by_name),
            make_widget({ 204, 44 }, { 79, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_town_type),
            make_widget({ 284, 44 }, { 69, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_population),
            make_widget({ 354, 44 }, { 69, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_stations),
            make_widget({ 3, 56 }, { 593, 125 }, widget_type::scrollview, 1, 2),
            widget_end(),
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
        static void prepare_draw(ui::window* self)
        {
            common::prepare_draw(self);

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
        static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            auto shade = colour::get_shade(self->colours[1], 3);
            gfx::clear_single(*dpi, shade);

            uint16_t yPos = 0;
            for (uint16_t i = 0; i < self->var_83C; i++)
            {
                town_id_t townId = self->row_info[i];

                // Skip items outside of view, or irrelevant to the current filter.
                if (yPos + common::rowHeight < dpi->y || yPos >= yPos + common::rowHeight + dpi->height || townId == (uint16_t)-1)
                {
                    yPos += common::rowHeight;
                    continue;
                }

                string_id text_colour_id = string_ids::white_stringid2;

                // Highlight selection.
                if (townId == self->row_hover)
                {
                    gfx::draw_rect(dpi, 0, yPos, self->width, common::rowHeight, 0x2000030);
                    text_colour_id = string_ids::wcolour2_stringid2;
                }

                if (townId == 0xFFFF)
                    continue;
                auto town = townmgr::get(townId);

                // Town Name
                {
                    auto args = FormatArguments();
                    args.push(town->name);

                    gfx::draw_string_494BBF(*dpi, 0, yPos, 198, colour::black, text_colour_id, &args);
                }
                // Town Type
                {
                    auto args = FormatArguments();
                    args.push(town->getTownSizeString());

                    gfx::draw_string_494BBF(*dpi, 200, yPos, 278, colour::black, text_colour_id, &args);
                }
                // Town Population
                {
                    auto args = FormatArguments();
                    args.push(string_ids::population_int);
                    args.push(town->population);

                    gfx::draw_string_494BBF(*dpi, 280, yPos, 68, colour::black, text_colour_id, &args);
                }
                // Town Stations
                {
                    auto args = FormatArguments();
                    args.push(string_ids::population_int);
                    args.push(town->num_stations);

                    gfx::draw_string_494BBF(*dpi, 350, yPos, 68, colour::black, text_colour_id, &args);
                }
                yPos += common::rowHeight;
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

            gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::white_stringid2, &args);
        }

        // 0x0049A27F
        static void on_mouse_up(ui::window* self, widget_index widgetIndex)
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
        static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = (int32_t)self;
            regs.cx = x;
            regs.dx = y;
            call(0x0049A56D, regs);
        }

        // 0x0049A532
        static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = (int32_t)self;
            regs.cx = x;
            regs.dx = y;
            call(0x0049A532, regs);
        }

        // 0x00499E0B
        static void updateTownList(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00499E0B, regs);
        }

        // 0x0049A4A0
        static void on_update(window* self)
        {
            self->frame_no++;

            self->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->current_tab + common::widx::tab_town_list);

            // Add three towns every tick.
            updateTownList(self);
            updateTownList(self);
            updateTownList(self);
        }

        // 0x0049A4D0
        static void event_08(window* self)
        {
            self->flags |= window_flags::flag_14;
        }

        // 0x0049A4D8
        static void event_09(window* self)
        {
            if ((self->flags & window_flags::flag_14) == 0)
                return;

            if (self->row_hover == -1)
                return;

            self->row_hover = -1;
            self->invalidate();
        }

        // 0x0049A4FA
        static void get_scroll_size(ui::window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = common::rowHeight * self->var_83C;
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

            uint16_t currentIndex = yPos / common::rowHeight;
            if (currentIndex < self->var_83C && self->row_info[currentIndex] != -1)
                return cursor_id::hand_pointer;

            return fallback;
        }

        static void init_events()
        {
            events.draw = draw;
            events.cursor = cursor;
            events.draw_scroll = draw_scroll;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.get_scroll_size = get_scroll_size;
            events.on_mouse_up = on_mouse_up;
            events.on_update = on_update;
            events.scroll_mouse_down = on_scroll_mouse_down;
            events.scroll_mouse_over = on_scroll_mouse_over;
            events.prepare_draw = prepare_draw;
            events.tooltip = tooltip;
        }
    }

    // 0x00499C83
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::townList, 0);
        if (window != nullptr)
        {
            window->call_on_mouse_up(4);
        }
        else
        {
            // 0x00499CFC
            window = WindowManager::createWindow(
                WindowType::townList,
                common::window_size,
                window_flags::flag_8,
                &town_list::events);

            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;
            window->sort_mode = 0;
            window->var_83C = 0;
            window->row_hover = -1;

            common::refreshTownList(window);

            WindowManager::sub_4CEE0B(window);

            window->min_width = common::min_dimensions.width;
            window->min_height = common::min_dimensions.height;
            window->max_width = common::max_dimensions.width;
            window->max_height = common::max_dimensions.height;
            window->flags |= window_flags::resizable;

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[0] = skin->colour_0B;
            window->colours[1] = skin->colour_0C;

            // 0x00499CFC end

            // TODO: only needs to be called once.
            common::init_events();

            window->current_tab = 0;
            window->invalidate();

            window->widgets = town_list::widgets;
            window->enabled_widgets = town_list::enabledWidgets;

            if (is_editor_mode())
                window->disabled_widgets = 0;
            else
                window->disabled_widgets |= (1 << common::widx::tab_build_town) | (1 << common::widx::tab_build_buildings) | (1 << common::widx::tab_build_misc_buildings);

            window->activated_widgets = 0;
            window->holdable_widgets = 0;

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();
        }
        return window;
    }

    namespace build_towns
    {
        enum widx
        {
            current_size = 8,
            select_size,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << current_size) | (1 << select_size);

        widget_t widgets[] = {
            commonWidgets(219, 86, string_ids::title_build_new_towns),
            make_widget({ 100, 45 }, { 116, 11 }, widget_type::wt_18, 1, image_ids::null, string_ids::tooltip_select_town_size),
            make_widget({ 204, 46 }, { 10, 9 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::tooltip_select_town_size),
            widget_end(),
        };

        static window_event_list events;

        // 0x0049A59A
        static void prepare_draw(ui::window* self)
        {
            common::prepare_draw(self);

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

            self->widgets[widx::current_size].text = townSizeNames[byte_1135C66];
        }

        // 0x0049A0A7
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x0049A0A7, regs);
        }

        // 0x0049A675
        static void on_mouse_up(ui::window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x0049A675, regs);
        }

        // 0x0049A4A0
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00499E0B, regs);
        }

        // 0x0049A697
        static void on_dropdown(window* self, ui::widget_index widget_index, int16_t item_index)
        {
            registers regs;
            regs.ax = item_index;
            regs.edx = widget_index;
            regs.esi = (uint32_t)self;
            call(0x0049A697, regs);
        }

        static void on_tool_abort(window& self, const widget_index widgetIndex)
        {
            common::hideGridlines();
        }

        // 0x0049A710
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049A710, regs);
        }

        // 0x0049A75E
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049A75E, regs);
        }

        // 0x0049A690
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (uint32_t)self;
            regs.edi = (uint32_t)&self->widgets[widgetIndex];
            call(0x0049A690, regs);
        }

        // 0x0049A844
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            call(0x0049A844, regs);
        }

        // 0x0049A7C7
        static void on_close(window* self)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            call(0x0049A7C7, regs);
        }

        static void init_events()
        {
            events.draw = draw;
            events.on_close = on_close;
            events.on_dropdown = on_dropdown;
            events.on_resize = on_resize;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.on_update = on_update;
            events.prepare_draw = prepare_draw;
            events.on_tool_abort = on_tool_abort;
        }
    }

    namespace build_misc_buildings
    {
        enum widx
        {
            scrollview = 8,
            rotate_object,
            object_colour,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview) | (1 << rotate_object) | (1 << object_colour);

        widget_t widgets[] = {
            commonWidgets(639, 171, string_ids::title_build_new_buildings),
            make_widget({ 2, 45 }, { 572, 111 }, widget_type::scrollview, 1, 2),
            make_widget({ 575, 46 }, { 23, 23 }, widget_type::wt_9, 1, image_ids::rotate_object, string_ids::rotate_object_90),
            make_widget({ 579, 91 }, { 15, 15 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_object_colour),
            widget_end(),
        };

        static window_event_list events;

        // 0x0049A8A6
        static void prepare_draw(ui::window* self)
        {
            if (self->widgets != widgets)
            {
                self->widgets = widgets;
                self->init_scroll_widgets();
            }

            self->activated_widgets &= ~((1ULL << common::widx::tab_town_list) | (1ULL << common::widx::tab_build_town) | (1ULL << common::widx::tab_build_buildings) | (1ULL << common::widx::tab_build_misc_buildings));
            self->activated_widgets |= 1ULL << (self->current_tab - common::widx::tab_town_list);

            self->widgets[widx::object_colour].text = ((byte_1135C61 << 19) & 0x60000902);
            self->widgets[widx::object_colour].type = widget_type::none;

            if (self->row_hover != -1)
            {
                auto buildingObj = objectmgr::get<building_object>(self->row_hover);
                if (buildingObj->colours != 0)
                    self->widgets[widx::object_colour].type = widget_type::wt_10;
            }

            common::prepare_draw(self);

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
            registers regs;
            regs.esi = (int32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x0049A9C2, regs);
        }

        // 0x0049AB31
        static void on_mouse_up(ui::window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x0049AB31, regs);
        }

        // 0x0049AD51
        static void on_update(window* self)
        {
            if (!input::has_flag(input::input_flags::flag5))
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
                        widget_index activeWidget = self->find_widget_at(xPos, yPos);
                        if (activeWidget == common::widx::panel)
                        {
                            self->saved_view.mapX += 1;
                            if (self->saved_view.mapX >= 8)
                            {
                                auto y = std::min(self->scroll_areas[0].v_bottom - 1 + 60, 500);
                                if (ui::height() < 600)
                                {
                                    y = std::min(y, 276);
                                }
                                self->min_width = 578;
                                self->min_height = y;
                                self->max_width = 578;
                                self->max_height = y;
                            }
                            else
                            {
                                if (input::state() != input::input_state::scroll_left)
                                {
                                    self->min_width = 578;
                                    self->min_height = 172;
                                    self->max_width = 578;
                                    self->max_height = 172;
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
                        self->min_width = 578;
                        self->min_height = 172;
                        self->max_width = 578;
                        self->max_height = 172;
                    }
                }
            }
            self->frame_no++;

            self->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::townList, self->number, self->current_tab + common::widx::tab_town_list);
            if ((!input::has_flag(input::input_flags::tool_active)) || self->type != _toolWindowType || self->number != _toolWindowNumber)
            {
                WindowManager::close(self);
            }
        }

        // 0x0049AB59
        static void on_dropdown(window* self, ui::widget_index widget_index, int16_t item_index)
        {
            registers regs;
            regs.ax = item_index;
            regs.edx = widget_index;
            regs.esi = (uint32_t)self;
            call(0x0049AB59, regs);
        }

        // 0x0049B37F
        static void sub_49B37F()
        {
            registers regs;
            call(0x0049B37F, regs);
        }

        // 0x0049AD46
        static void on_tool_abort(window& self, const widget_index widgetIndex)
        {
            sub_49B37F();
            common::hideGridlines();
        }

        // 0x0049ABF0
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049ABF0, regs);
        }

        // 0x0049ACBD
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049ACBD, regs);
        }

        // 0x0049AB52
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (uint32_t)self;
            regs.edi = (uint32_t)&self->widgets[widgetIndex];
            call(0x0049AB52, regs);
        }

        // 0x0049AF98
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            call(0x0049AF98, regs);
        }

        // 0x0049AE83
        static void get_scroll_size(ui::window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (4 + self->var_83C) / 5;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= 112;
        }

        // 0x0049ABBB
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_building_list);
        }

        // 0x0049AA1C
        static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            registers regs;
            regs.ax = scrollIndex;
            regs.esi = (int32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x0049AA1C, regs);
        }

        // 0x0049AEFD
        static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = (int32_t)self;
            regs.cx = x;
            regs.dx = y;
            call(0x0049AEFD, regs);
        }

        // 0x0049AEBA
        static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.ax = scroll_index;
            regs.esi = (int32_t)self;
            regs.cx = x;
            regs.dx = y;
            call(0x0049AEBA, regs);
        }

        // 0x0049ABC5
        static void on_close(window* self)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            call(0x0049ABC5, regs);
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

        static void init_events()
        {
            events.draw = draw;
            events.on_close = on_close;
            events.on_resize = on_resize;
            events.draw_scroll = draw_scroll;
            events.event_08 = event_08;
            events.on_dropdown = on_dropdown;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.on_mouse_down = on_mouse_down;
            events.get_scroll_size = get_scroll_size;
            events.on_mouse_up = on_mouse_up;
            events.on_update = on_update;
            events.scroll_mouse_down = on_scroll_mouse_down;
            events.scroll_mouse_over = on_scroll_mouse_over;
            events.prepare_draw = prepare_draw;
            events.tooltip = tooltip;
            events.on_tool_abort = on_tool_abort;
        }
    }

    namespace build_buildings
    {
        enum widx
        {
            scrollview = 8,
            rotate_object,
            object_colour,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview) | (1 << rotate_object) | (1 << object_colour);

        widget_t widgets[] = {
            commonWidgets(639, 171, string_ids::title_build_new_buildings),
            make_widget({ 2, 45 }, { 572, 111 }, widget_type::scrollview, 1, 2),
            make_widget({ 575, 46 }, { 23, 23 }, widget_type::wt_9, 1, image_ids::rotate_object, string_ids::rotate_object_90),
            make_widget({ 579, 91 }, { 15, 15 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_object_colour),
            widget_end(),
        };

        static window_event_list events;

        static void init_events()
        {
            events.draw = build_misc_buildings::draw;
            events.on_close = build_misc_buildings::on_close;
            events.on_resize = build_misc_buildings::on_resize;
            events.draw_scroll = build_misc_buildings::draw_scroll;
            events.event_08 = build_misc_buildings::event_08;
            events.on_dropdown = build_misc_buildings::on_dropdown;
            events.on_tool_update = build_misc_buildings::on_tool_update;
            events.on_tool_down = build_misc_buildings::on_tool_down;
            events.on_mouse_down = build_misc_buildings::on_mouse_down;
            events.get_scroll_size = build_misc_buildings::get_scroll_size;
            events.on_mouse_up = build_misc_buildings::on_mouse_up;
            events.on_update = build_misc_buildings::on_update;
            events.scroll_mouse_down = build_misc_buildings::on_scroll_mouse_down;
            events.scroll_mouse_over = build_misc_buildings::on_scroll_mouse_over;
            events.prepare_draw = build_misc_buildings::prepare_draw;
            events.tooltip = build_misc_buildings::tooltip;
            events.on_tool_abort = build_misc_buildings::on_tool_abort;
        }
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t* enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { town_list::widgets, widx::tab_town_list, &town_list::events, &town_list::enabledWidgets },
            { build_towns::widgets, widx::tab_build_town, &build_towns::events, &build_towns::enabledWidgets },
            { build_misc_buildings::widgets, widx::tab_build_buildings, &build_misc_buildings::events, &build_misc_buildings::enabledWidgets },
            { build_misc_buildings::widgets, widx::tab_build_misc_buildings, &build_misc_buildings::events, &build_misc_buildings::enabledWidgets },
        };

        static void prepare_draw(window* self)
        {
            // Reset tab widgets if needed
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->init_scroll_widgets();
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
                if (self->is_disabled(tabInfo.widgetIndex))
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
                    imageId += buildNewTownsImageIds[(self->frame_no / 4) % std::size(buildNewTownsImageIds)];
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
                    imageId += buildBuildingsImageIds[(self->frame_no / 4) % std::size(buildBuildingsImageIds)];
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
                    imageId += buildMiscBuildingsImageIds[(self->frame_no / 4) % std::size(buildMiscBuildingsImageIds)];
                else
                    imageId += buildMiscBuildingsImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_build_misc_buildings);
            }
        }

        // 0x0049A37E
        static void sub_49A37E(window* self)
        {
            self->min_width = 192;
            self->min_height = 200;
            self->max_width = 600;
            self->max_width = 900;
            self->width = 600;
            self->height = 197;
            self->var_83C = 0;
            self->row_hover = -1;

            refreshTownList(self);
        }

        // 0x0049A3BE
        static void sub_49A3BE(window* self)
        {
            self->min_width = 220;
            self->min_height = 87;
            self->max_width = 220;
            self->max_width = 87;
            self->width = 220;
            self->height = 87;
            input::toolSet(self, tab_build_town, 38);
            input::set_flag(input::input_flags::flag6);
        }

        // 0x0049B2B5
        static void townList_49B2B5(window* self)
        {
            uint16_t scrollHeight = 0;
            self->call_get_scroll_size(0, 0, &scrollHeight);
            self->scroll_areas[0].v_bottom = scrollHeight;

            auto i = 0;
            if (i >= self->var_83C)
            {
                for (; i <= self->var_83C; i++)
                {
                    if (self->row_info[i] == self->row_hover)
                        break;
                }
            }
            self->scroll_areas[0].v_top = i;
            ui::scrollview::update_thumbs(self, build_misc_buildings::widx::scrollview);
        }

        static void sub_49B304(window* self)
        {
            if (self->row_hover != -1)
            {
                auto buildingObj = objectmgr::get<building_object>(self->row_hover);

                unsigned long* byte = 0;
                _BitScanReverse(byte, buildingObj->colours);
                byte_1135C61 = *byte;
            }
        }

        // 0x0049B206
        static void townList_49B206(window* self)
        {
            auto buildingCount = 0;
            for (auto i = 0; i < 0x80; i++)
            {
                auto buildingObj = objectmgr::get<building_object>(i);
                if (buildingObj == nullptr)
                    break;
                if (self->current_tab == widx::tab_build_misc_buildings - widx::tab_town_list)
                {
                    if ((buildingObj->flags & building_object_flags::flag_2) != 0)
                    {
                        if ((buildingObj->flags & building_object_flags::flag_4) != 0)
                            continue;
                    }
                }
                else
                {
                    if ((buildingObj->flags & building_object_flags::flag_2) != 0)
                        continue;
                }
                self->row_info[buildingCount] = i;
                buildingCount++;
            }

            self->var_83C = buildingCount;
            auto rowHover = -1;

            auto byte = byte_525FC8;
            if (self->current_tab == widx::tab_build_misc_buildings - widx::tab_town_list)
                byte = byte_525FC9;

            if (byte != 0xFF)
            {
                for (auto i = 0; i <= self->var_83C; i++)
                {
                    if (i >= self->var_83C)
                    {
                        if (byte == self->row_info[i])
                        {
                            rowHover = byte;
                            break;
                        }
                    }
                    else
                    {
                        if (self->var_83C != 0)
                        {
                            rowHover = self->row_info[0];
                        }
                    }
                }
            }
            else
            {
                if (self->var_83C != 0)
                {
                    rowHover = self->row_info[0];
                }
            }
            townList_49B2B5(self);
            sub_49B304(self);
        }

        // 0x0049A3FF
        static void sub_49A3FF(window* self)
        {
            self->min_width = 600;
            self->min_height = 172;
            self->max_width = 600;
            self->max_width = 172;
            self->width = 600;
            self->height = 172;

            auto tab = tab_build_buildings;
            if (self->current_tab == widx::tab_build_misc_buildings - widx::tab_town_list)
                tab = tab_build_misc_buildings;

            input::toolSet(self, tab, 39);
            input::set_flag(input::input_flags::flag6);
            showGridlines();

            byte_1135C60 = 0;
            dword_1135C34 = 0x80000000;
            self->var_83C = 0;
            self->row_hover = -1;
            self->var_846 = -1;

            townList_49B206(self);
            sub_49B304(self);

            byte_1135C65 = 0;
        }

        //0x00457F27
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();

            self->current_tab = widgetIndex - widx::tab_town_list;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_town_list];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            if (self->current_tab == widx::tab_town_list - widx::tab_town_list)
                sub_49A37E(self);
            if (self->current_tab == widx::tab_build_town - widx::tab_town_list)
                sub_49A3BE(self);
            if (self->current_tab == widx::tab_build_buildings - widx::tab_town_list || self->current_tab == widx::tab_build_misc_buildings - widx::tab_town_list)
                sub_49A3FF(self);

            self->call_on_resize();
            self->call_prepare_draw();
            self->init_scroll_widgets();
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

                town.flags &= ~town_flags::rating_adjusted;
            }
        }

        // 0x00468FD3
        static void showGridlines()
        {
            if (_gridlines_state == 0)
            {
                auto window = WindowManager::getMainWindow();
                if ((window->viewports[0]->flags & viewport_flags::gridlines_on_landscape) != 0)
                {
                    window->invalidate();
                }
                window->viewports[0]->flags |= viewport_flags::gridlines_on_landscape;
            }
            _gridlines_state++;
        }

        // 0x00468FFE
        static void hideGridlines()
        {
            _gridlines_state--;
            if (_gridlines_state == 0)
            {
                auto window = WindowManager::getMainWindow();
                if (window != nullptr)
                {
                    if ((config::get().flags & config::flags::gridlines_on_landscape) == 0)
                    {
                        if ((window->viewports[0]->flags & viewport_flags::gridlines_on_landscape) == 0)
                        {
                            window->invalidate();
                        }
                        window->viewports[0]->flags ^= viewport_flags::gridlines_on_landscape;
                    }
                }
            }
        }

        static void init_events()
        {
            town_list::init_events();
            build_towns::init_events();
            build_buildings::init_events();
            build_misc_buildings::init_events();
        }
    }
}
