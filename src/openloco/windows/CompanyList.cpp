#include "../company.h"
#include "../companymgr.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyList
{
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;

    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_company_list,
            tab_performance,
            tab_cargo_units,
            tab_cargo_distance,
            tab_values,
            tab_payment_rates,
            tab_speed_records,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_company_list) | (1 << widx::tab_performance) | (1 << widx::tab_cargo_units) | (1 << widx::tab_cargo_distance) | (1 << widx::tab_values) | (1 << widx::tab_payment_rates) | (1 << widx::tab_speed_records);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 231 }, widget_type::panel, 1),                                                               \
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_compare_companies),                      \
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_company_performance),                   \
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_cargo_graphs),                          \
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_cargo_distance_graphs),                 \
        make_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_company_values),                       \
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_cargo_payment_rates),                  \
        make_widget({ 189, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_speed_records)

        static void initEvents();
    }

    namespace company_list
    {
        static const gfx::ui_size_t maxWindowSize = { 640, 470 };
        static const gfx::ui_size_t minWindowSize = { 300, 272 };

        static const uint8_t rowHeight = 25;

        enum widx
        {
            sort_name = 11,
            sort_status,
            sort_performance,
            sort_value,
            scrollview,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << sort_name) | (1 << sort_status) | (1 << sort_performance) | (1 << sort_value) | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(640, 272, string_ids::title_company_list),
            make_widget({ 4, 43 }, { 175, 12 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_company_name),
            make_widget({ 179, 43 }, { 210, 12 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_company_status),
            make_widget({ 389, 43 }, { 145, 12 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_company_performance),
            make_widget({ 534, 43 }, { 100, 12 }, widget_type::wt_14, 1, image_ids::null, string_ids::tooltip_sort_company_value),
            make_widget({ 3, 56 }, { 634, 201 }, widget_type::scrollview, 1, vertical),
            widget_end(),
        };

        static window_event_list events;

        enum SortMode : uint16_t
        {
            Name,
            Status,
            Performance,
            Value,
        };

        // 0x004360A2
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
        }

        // 0x004363CB
        static void on_resize(window* self)
        {
        }

        // 0x004362C0
        static void on_update(window* self)
        {
        }

        // 0x004362F7
        static void event_08(window* self)
        {
            self->flags |= window_flags::not_scroll_view;
        }

        // 0x004362FF
        static void event_09(window* self)
        {
            if (!(self->flags & window_flags::not_scroll_view))
                return;

            if (self->row_hover == -1)
                return;

            self->row_hover = -1;
            self->invalidate();
        }

        // 0x00436321
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = self->var_83C * rowHeight;
        }

        // 0x004363A0
        static void on_scroll_mouse_down(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
        }

        // 0x00436361
        static void on_scroll_mouse_over(window* self, int16_t x, int16_t y, uint8_t scroll_index)
        {
        }

        // 0x004362B6
        static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_company_list);
        }

        // 0x0043632C
        static ui::cursor_id cursor(window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
        {
            if (widgetIdx != widx::scrollview)
                return fallback;

            uint16_t currentIndex = yPos / rowHeight;
            if (currentIndex < self->var_83C && self->row_info[currentIndex] != -1)
                return cursor_id::hand_pointer;

            return fallback;
        }

        // 0x00435D07
        static void prepare_draw(window* self)
        {
        }

        // 0x00435E56
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
        }

        // 0x00435EA7
        static void draw_scroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
        {
        }

        // 0x00437AB6
        static void sub_437AB6(window* self)
        {
            self->row_count = 0;

            for (auto i = 0; i < companymgr::max_companies; i++)
            {
                auto company = companymgr::get(i);
                company->challenge_flags &= ~(1 << 3);
            }
        }

        static void initEvents()
        {
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = on_update;
            events.event_08 = event_08;
            events.event_09 = event_09;
            events.get_scroll_size = get_scroll_size;
            events.scroll_mouse_down = on_scroll_mouse_down;
            events.scroll_mouse_over = on_scroll_mouse_over;
            events.tooltip = tooltip;
            events.cursor = cursor;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.draw_scroll = draw_scroll;
        }
    }

    void openPerformanceIndexes()
    {
        call(0x00435C69);
    }

    void openUnk()
    {
        auto window = WindowManager::bringToFront(WindowType::companyList);

        if (window != nullptr)
        {
            if (input::is_tool_active(_toolWindowType, _toolWindowNumber))
            {
                input::cancel_tool();
                window = WindowManager::bringToFront(WindowType::messages);
            }
        }

        if (window == nullptr)
        {
            gfx::ui_size_t windowSize = { 640, 272 };

            WindowManager::createWindow(WindowType::companyList, windowSize, 0, &company_list::events);
        }

        //window->enabled_widgets = company_list::enabledWidgets;
        //window->current_tab = 0;
        window->frame_no = 0;
        window->saved_view.clear();
        window->flags &= window_flags::resizable;
        window->var_83C = 0;
        window->row_hover = -1;

        company_list::sub_437AB6(window);

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0C;

        window->var_854 = 0;
        window->current_tab = 0;
        window->min_width = company_list::minWindowSize.width;
        window->min_height = company_list::minWindowSize.height;
        window->max_width = company_list::maxWindowSize.width;
        window->max_height = company_list::maxWindowSize.height;

        window->invalidate();

        common::initEvents();

        window->widgets = company_list::widgets;
        window->enabled_widgets = company_list::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &company_list::events;
        window->activated_widgets = 0;
        window->init_scroll_widgets();
    }
    namespace common
    {
        static void initEvents()
        {
            company_list::initEvents();
        }
    }
}
