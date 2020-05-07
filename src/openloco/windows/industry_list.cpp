#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../industrymgr.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::industry_list
{

    namespace common
    {
        static const gfx::ui_size_t window_size = { 600, 197 };
        static const gfx::ui_size_t max_dimensions = { 640, 1200 };
        static const gfx::ui_size_t min_dimensions = { 192, 100 };

        static const uint8_t rowHeight = 10;

        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_industry_list,
            tab_new_industry,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_industry_list) | (1 << widx::tab_new_industry);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 154 }, widget_type::panel, 1),                                                               \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_industries_list),              \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_fund_new_industires)

        static window_event_list _events;

        static void init_events();
        static void refreshIndustryList(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
    }

    namespace industry_list
    {
        enum widx
        {
            sort_industry_name = 6,
            sort_industry_status,
            sort_industry_production_transported,
            scrollview,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << sort_industry_name) | (1 << sort_industry_status) | (1 << sort_industry_production_transported) | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(600, 197, string_ids::title_industries),
            make_widget({ 4, 44 }, { 199, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_industry_name),
            make_widget({ 204, 44 }, { 204, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_industry_status),
            make_widget({ 444, 44 }, { 159, 11 }, widget_type::wt_14, 1, image_ids::null, string_ids::sort_industry_production_transported),
            make_widget({ 3, 56 }, { 593, 125 }, widget_type::scrollview, 1, 2),
            widget_end(),
        };

        static window_event_list events;

        // 0x00457B94
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            call(0x00457B94, regs);
        }

        // 0x00457CD9
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            auto args = FormatArguments();
            auto xPos = self->x + 4;
            auto yPos = self->y + self->height - 12;

            if (self->var_83C == 1)
                args.push(string_ids::status_num_industries_singular);
            else
                args.push(string_ids::status_num_industries_plural);
            args.push(self->var_83C);

            gfx::draw_string_494B3F(*dpi, xPos, yPos, colour::black, string_ids::white_stringid2, &args);
        }

        // 0x00457EC4
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edx = widgetIndex;
            call(0x00457EC4, regs);
        }

        //0x00458172
        static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            uint16_t currentRow = y / common::rowHeight;
            if (currentRow > self->var_83C)
                return;

            int16_t currentIndustry = self->row_info[currentRow];
            if (currentIndustry == -1)
                return;

            //windows::industry::open(currentIndustry);
        }

        // 0x00458140
        static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            self->flags &= ~(window_flags::flag_14);

            uint16_t currentRow = y / common::rowHeight;
            int16_t currentIndustry = -1;

            if (currentRow < self->var_83C)
                currentIndustry = self->row_info[currentRow];

            self->row_hover = currentIndustry;
            self->invalidate();
        }

        // 0x00457991
        static void updateIndustryList(window* self)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            call(0x00457991, regs);
        }

        // 0x004580AE
        static void on_update(window* self)
        {
            self->frame_no++;

            self->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::stationList, self->number, self->current_tab + 4);

            // Add three stations every tick.
            updateIndustryList(self);
            updateIndustryList(self);
            updateIndustryList(self);
        }

        // 0x00457EE8
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_industry_list);
        }

        // 0x00458108
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = common::rowHeight * self->var_83C;
        }

        // 0x00457D2A
        static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edi = (uint32_t)dpi;
            regs.eax = scrollIndex;
            call(0x00457D2A, regs);
        }

        // 0x00458113
        static ui::cursor_id cursor(window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / common::rowHeight;
            if (currentIndex < self->var_83C && self->row_info[currentIndex] != -1)
                return cursor_id::hand_pointer;

            return fallback;
        }

        // 0x004580DE
        static void event_08(window* self)
        {
            self->flags |= window_flags::flag_14;
        }

        // 0x004580E6
        static void event_09(window* self)
        {
            if ((self->flags & window_flags::flag_14) == 0)
                return;

            if (self->row_hover == -1)
                return;

            self->row_hover = -1;
            self->invalidate();
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

    // 0x004577FF
    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::industryList, 0);
        if (window != nullptr)
        {
            window->call_on_mouse_up(4);
        }
        else
        {
            // 0x00457878
            window = WindowManager::createWindow(
                WindowType::industryList,
                common::window_size,
                window_flags::flag_11,
                &industry_list::events);

            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;
            window->sort_mode = 0;
            window->var_83C = 0;
            window->row_hover = -1;

            common::refreshIndustryList(window);

            WindowManager::sub_4CEE0B(window);

            window->min_width = common::min_dimensions.width;
            window->min_height = common::min_dimensions.height;
            window->max_width = common::max_dimensions.width;
            window->max_height = common::max_dimensions.height;
            window->flags |= window_flags::resizable;

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[0] = skin->colour_0B;
            window->colours[1] = skin->colour_0C;

            // 0x00457878 end

            // TODO: only needs to be called once.
            common::init_events();

            window->current_tab = 0;
            window->invalidate();

            window->widgets = industry_list::widgets;
            window->enabled_widgets = industry_list::enabledWidgets;

            window->activated_widgets = 0;
            window->holdable_widgets = 0;

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();
        }
        return window;
    }

    namespace new_industries
    {
        enum widx
        {
            scrollview = 6,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(577, 171, string_ids::title_fund_new_industries),
            make_widget({ 3, 45 }, { 549, 111 }, widget_type::scrollview, 1, 2),
            widget_end(),
        };

        static window_event_list events;

        // 0x0045819F
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            call(0x0045819F, regs);
        }

        // 0x0045826C
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edi = (uint32_t)dpi;
            call(0x0045826C, regs);
        }

        // 0x0045843A
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edx = widgetIndex;
            call(0x0045843A, regs);
        }

        //0x00458966
        static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.cx = x;
            regs.dx = y;
            call(0x00458966, regs);
        }

        // 0x00458721
        static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.cx = x;
            regs.dx = y;
            call(0x00458721, regs);
        }

        // 0x004585B8
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            call(0x004585B8, regs);
        }

        // 0x00458455
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_new_industry_list);
        }

        // 0x004586EA
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = (4 * self->var_83C) / 5;
            if (*scrollHeight == 0)
                *scrollHeight += 1;
            *scrollHeight *= 112;
        }

        // 0x00458352
        static void draw_scroll(ui::window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edi = (uint32_t)dpi;
            regs.eax = scrollIndex;
            call(0x00458352, regs);
        }

        // 0x00458708
        static void event_08(window* self)
        {
            if (self->var_846 != -1)
                self->var_846 = -1;
            self->invalidate();
        }

        // 0x0045848A
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0045848A, regs);
        }

        // 0x0045851F
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0045851F, regs);
        }

        // 0x00458C09
        static void sub_458C09()
        {
            registers regs;
            call(0x00458C09, regs);
        }

        // 0x00468FFE hide_gridlines
        static void sub_468FFE()
        {
            registers regs;
            call(0x00468FFE, regs);
        }

        // 0x004585AD
        static void on_tool_abort(window& self, const widget_index widgetIndex)
        {
            sub_458C09();
            sub_468FFE();
        }

        static void init_events()
        {
            events.draw = draw;
            events.draw_scroll = draw_scroll;
            events.event_08 = event_08;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
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

    namespace common
    {
        // 0x00458A57
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Industry List Tab
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::toolbar_menu_industries;

                widget::draw_tab(self, dpi, imageId, widx::tab_industry_list);
            }

            // Fund New Industries Tab
            {
                static const uint32_t fundNewIndustriesImageIds[] = {
                    interface_skin::image_ids::build_industry_frame_0,
                    interface_skin::image_ids::build_industry_frame_1,
                    interface_skin::image_ids::build_industry_frame_2,
                    interface_skin::image_ids::build_industry_frame_3,
                    interface_skin::image_ids::build_industry_frame_4,
                    interface_skin::image_ids::build_industry_frame_5,
                    interface_skin::image_ids::build_industry_frame_6,
                    interface_skin::image_ids::build_industry_frame_7,
                    interface_skin::image_ids::build_industry_frame_8,
                    interface_skin::image_ids::build_industry_frame_9,
                    interface_skin::image_ids::build_industry_frame_10,
                    interface_skin::image_ids::build_industry_frame_11,
                    interface_skin::image_ids::build_industry_frame_12,
                    interface_skin::image_ids::build_industry_frame_13,
                    interface_skin::image_ids::build_industry_frame_14,
                    interface_skin::image_ids::build_industry_frame_15,
                };
                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_new_industry - widx::tab_industry_list)
                    imageId += fundNewIndustriesImageIds[(self->frame_no / 4) % std::size(fundNewIndustriesImageIds)];
                else
                    imageId += fundNewIndustriesImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_new_industry);
            }
        }

        // 0x004910E8
        static void refreshIndustryList(window* window)
        {
            window->row_count = 0;

            for (auto& industry : industrymgr::industries())
            {
                if (industry.empty())
                    continue;

                industry.flags &= ~industry_flags::flag_01;
            }
        }

        static void init_events()
        {
            industry_list::init_events();
            new_industries::init_events();
        }
    }
}
