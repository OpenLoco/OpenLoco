#include "../companymgr.h"
#include "../date.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"

using namespace openloco::interop;

namespace openloco::ui::MessageWindow
{
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;

    namespace common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_messages,
            tab_settings,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_messages) | (1 << widx::tab_settings);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { 366, 175 }, widget_type::panel, 1),                                                                      \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_recent_messages),              \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_message_options)
    }

    namespace messages
    {
        static const gfx::ui_size_t minWindowSize = { 366, 217 };
        static const gfx::ui_size_t maxWindowSize = { 366, 1200 };

        enum widx
        {
            scrollview = 6,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << scrollview);

        widget_t widgets[] = {
            commonWidgets(366, 217, string_ids::title_messages),
            make_widget({ 3, 45 }, { 360, 146 }, widget_type::scrollview, 1, scrollbars::vertical),
            widget_end(),
        };

        static window_event_list events;

    }
    // 0x0042A3FF
    void open()
    {
        //call(0x0042A3FF);
        auto window = WindowManager::bringToFront(WindowType::messages);

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
            gfx::point_t origin = { ui::width() - 366, 29 };

            window = WindowManager::createWindow(
                WindowType::messages,
                origin,
                { 366, 217 },
                window_flags::flag_11,
                &messages::events);

            window->enabled_widgets = messages::enabledWidgets;
            window->number = 0;
            window->current_tab = 0;
            window->frame_no = 0;
            window->row_hover = -1;
            window->disabled_widgets = 0;

            WindowManager::sub_4CEE0B(window);

            window->min_width = messages::minWindowSize.width;
            window->min_height = messages::minWindowSize.height;
            window->max_width = messages::maxWindowSize.width;
            window->max_height = messages::maxWindowSize.height;
            window->flags != window_flags::resizable;

            window->owner = _playerCompany;
            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[1] = skin->colour_0A;

            window->width = messages::minWindowSize.width;
            window->height = messages::minWindowSize.height;

            window->invalidate();

            window->widgets = messages::widgets;
            window->holdable_widgets = 0;
            window->event_handlers = &messages::events;
            window->disabled_widgets = 0;

            window->call_on_resize();
            window->call_prepare_draw();
            window->init_scroll_widgets();
            window->call_get_scroll_size(0,0,0);


        }
    }
}
