#include "../config.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../town.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::town_list
{
    loco_global<int8_t, 0x00F2533F> _gridlines_state;

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

        static void init_events();
        static void refreshTownList(window* self);
        static void hideGridlines();
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
            make_widget({ 204, 44 }, { 79, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_town_type),
            make_widget({ 284, 44 }, { 69, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_population),
            make_widget({ 354, 44 }, { 69, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_stations),
            make_widget({ 3, 56 }, { 593, 125 }, widget_type::scrollview, 1, 2),
            widget_end(),
        };

        static window_event_list events;

        // 0x00499F53
        static void prepare_draw(ui::window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00499F53, regs);
        }

        // 0x0049A0F8
        static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            registers regs;
            regs.ax = scrollIndex;
            regs.esi = (int32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x0049A0F8, regs);
        }

        // 0x0049A0A7
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x0049A0A7, regs);
        }

        // 0x0049A27F
        static void on_mouse_up(ui::window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x0049A27F, regs);
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
        auto window = WindowManager::bringToFront(WindowType::industryList, 0);
        if (window != nullptr)
        {
            window->call_on_mouse_up(4);
        }
        else
        {
            // 0x00499CFC
            window = WindowManager::createWindow(
                WindowType::industryList,
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
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0049A59A, regs);
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
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0049A8A6, regs);
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
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0049AD51, regs);
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

        // 0x0049A4D0
        static void event_08(window* self)
        {
            if (self->var_846 != -1)
            {
                self->var_846 = -1;
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

        // 0x00468FFE hide_gridlines
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
