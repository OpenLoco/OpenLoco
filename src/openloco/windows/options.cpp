#include "../config.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui.h"
#include "../ui/dropdown.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::options
{
    static void sub_4C141D(window* w, gfx::drawpixelinfo_t* dpi)
    {
        registers regs;
        regs.esi = (uint32_t)w;
        regs.edi = (int32_t)dpi;
        call(0x004C141D, regs);
    }

    namespace common
    {
        namespace widx
        {
            enum
            {
                frame,
                caption,
                close_button,
                panel,
                tab_display,
                tab_sound,
                tab_music,
                tab_regional,
                tab_control,
                tab_miscellaneous,
            };
        }

#define common_options_widgets(window_size, window_caption_id)                                                                                              \
    make_widget({ 0, 0 }, window_size, widget_type::frame, 0),                                                                                              \
        make_widget({ 1, 1 }, { (uint16_t)(window_size.width - 2), 13 }, widget_type::caption_25, 0, window_caption_id),                                    \
        make_widget({ (int16_t)(window_size.width - 15), 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { window_size.width, 102 }, widget_type::panel, 1),                                                                          \
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::display_options),                         \
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::sound_options),                          \
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::music_options),                          \
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::regional_options),                       \
        make_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::control_options),                       \
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, colour::remap_flag | image_ids::tab, string_ids::miscellaneous_options)
    }

    namespace display
    {
        static const gfx::ui_size_t _window_size = { 366, 144 };

        namespace widx
        {
            enum
            {
                display_resolution = 10,
                display_resolution_btn,
                landscape_smoothing,
                gridlines_on_landscape,
                vehicles_min_scale,
                vehicles_min_scale_btn,
                station_names_min_scale,
                station_names_min_scale_btn,
                construction_marker,
                construction_marker_btn,
            };
        }

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_display),
            make_widget({ 183, 49 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::display_resolution_format),
            make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 65 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::landscape_smoothing, string_ids::landscape_smoothing_tip),
            make_widget({ 10, 80 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::gridlines_on_landscape, string_ids::gridlines_on_landscape_tip),
            make_widget({ 183, 94 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::empty, string_ids::vehicles_min_scale_tip),
            make_widget({ 344, 95 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::vehicles_min_scale_tip),
            make_widget({ 183, 109 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::empty, string_ids::station_names_min_scale_tip),
            make_widget({ 344, 110 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::station_names_min_scale_tip),
            make_widget({ 183, 124 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::empty),
            make_widget({ 344, 125 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            widget_end(),
        };

        static window_event_list _events;

        // 0x004BFB8C
        static void on_mouse_up(window* w, widget_index wi)
        {
            registers regs;
            regs.edx = wi;
            regs.esi = (uint32_t)w;
            call(0x004BFB8C, regs);

            switch (wi)
            {
                case common::widx::close_button:
                    windowmgr::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_control:
                case common::widx::tab_miscellaneous:
                    return;

                case widx::landscape_smoothing:
                {
                    // Does this actually create a reference?
                    auto cfg = &openloco::config::get();
                    if (cfg->flags & 4)
                    {
                        cfg->flags &= ~4;
                    }
                    else
                    {
                        cfg->flags |= 4;
                    }
                    openloco::config::write();
                    gfx::invalidate_screen();
                    return;
                }

                case widx::gridlines_on_landscape:
                {
                    // Does this actually create a reference?
                    auto cfg = &openloco::config::get();
                    if (cfg->flags & 1)
                    {
                        cfg->flags &= ~1;
                    }
                    else
                    {
                        cfg->flags |= 1;
                    }
                    openloco::config::write();
                    gfx::invalidate_screen();

                    auto main = windowmgr::get_main();
                    if (main != nullptr)
                    {
                        main->viewports[0]->var_12 &= ~(1 << 5);

                        if ((cfg->flags & 1) != 0)
                        {
                            main->viewports[0]->var_12 |= (1 << 5);
                        }
                    }

                    return;
                }
            }
        }

#pragma mark - Widget 19

        // dropdown
        static void sub_4BFE2E(window* w)
        {
            // w->widgets[widx - 1].left;   // -E
            // w->widgets[widx - 1].right;  // -C
            // w->widgets[widx - 1].top;    // -A
            // w->widgets[widx - 1].bottom; // -8

            // TODO: allow to pass object with format args
            dropdown::add(0, string_ids::str_421, string_ids::white);
            dropdown::add(1, string_ids::str_421, string_ids::translucent);
            dropdown::set_selection(config::get().construction_marker);
        }

        // dropdown
        static void sub_4BFE98(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().construction_marker)
                return;

            // Does this actually create a reference?
            auto cfg = &openloco::config::get();
            cfg->construction_marker = ax;
            openloco::config::write();
            gfx::invalidate_screen();
        }

#pragma mark - Widget 15
        // mouse down
        static void sub_4BFEBE()
        {
        }

        // dropdown
        static void sub_4BFF4C(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().thing_zoom_max)
                return;

            // Does this actually create a reference?
            auto cfg = &openloco::config::get();
            cfg->thing_zoom_max = ax;
            openloco::config::write();
            gfx::invalidate_screen();
        }

#pragma mark - Widget 17
        // mouse down
        static void sub_4BFF72()
        {
        }

        // dropdown
        static void sub_4C0000(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().var_114)
                return;

            // Does this actually create a reference?
            auto cfg = &openloco::config::get();
            cfg->var_114 = ax;
            openloco::config::write();
            gfx::invalidate_screen();
        }

#pragma mark - Widget 11
        // mouse down
        static void sub_4C0026()
        {
        }

        // dropdown
        static void sub_4C00F4(int16_t ax)
        {
        }

#pragma mark -

        // 0x004BFBB7
        static void on_mouse_down(window* w, widget_index wi)
        {
            registers regs;
            regs.edx = wi;
            regs.esi = (uint32_t)w;
            call(0x004BFBB7, regs);
        }

        // 0x004BFBE8
        static void on_dropdown(window* w, widget_index wi, int16_t item_index)
        {
            registers regs;
            regs.ax = item_index;
            regs.edx = wi;
            regs.esi = (uint32_t)w;
            call(0x004BFBE8, regs);
        }

        // 0x004C01F5
        static void on_update(window* w)
        {
            w->var_872 += 1;
            w->call_prepare_draw();
            w->invalidate();
        }

        // 0x004BFA04
        static void prepare_draw(window* w)
        {
            registers regs;
            regs.esi = (uint32_t)w;
            call(0x004BFA04, regs);
        }

        // 0x004BFAF9
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            sub_4C141D(w, dpi);

            int16_t x = w->x + 10;
            int16_t y = w->y + display::_widgets[display::widx::display_resolution].top + 1;
            draw_string_494B3F(*dpi, x, y, colour::black, string_ids::display_resolution, nullptr);

            y = w->y + display::_widgets[display::widx::construction_marker].top + 1;
            draw_string_494B3F(*dpi, x, y, colour::black, string_ids::construction_marker, nullptr);

            y = w->y + display::_widgets[display::widx::vehicles_min_scale].top + 1;
            draw_string_494B3F(*dpi, x, y, colour::black, string_ids::vehicles_min_scale, nullptr);

            y = w->y + display::_widgets[display::widx::station_names_min_scale].top + 1;
            draw_string_494B3F(*dpi, x, y, colour::black, string_ids::station_names_min_scale, nullptr);
        }

        static const window_event_list init_events()
        {
            _events.on_mouse_up = on_mouse_up;
            _events.on_mouse_down = on_mouse_down;
            _events.on_dropdown = on_dropdown;
            _events.on_update = on_update;
            _events.prepare_draw = prepare_draw;
            _events.draw = draw;
            return _events;
        }
    }

    namespace sound
    {
        static const gfx::ui_size_t _window_size = { 366, 97 };

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_sound),
            make_widget({ 10, 49 }, { 346, 12 }, widget_type::wt_18, 1, string_ids::STR_0085),
            make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 64 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0090),
            make_widget({ 344, 65 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 79 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1010, string_ids::STR_1011),
            widget_end(),
        };

        static loco_global<window_event_list, 0x00503FB4> _events;
    }

    namespace music
    {
        static const gfx::ui_size_t _window_size = { 366, 129 };

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_music),
            make_widget({ 160, 49 }, { 196, 12 }, widget_type::wt_18, 1, string_ids::STR_0085),
            make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_stop, string_ids::STR_1536),
            make_widget({ 34, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_play, string_ids::STR_1537),
            make_widget({ 58, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_next, string_ids::STR_1538),
            make_widget({ 256, 64 }, { 109, 24 }, widget_type::wt_5, 1, -1, string_ids::STR_1548),
            make_widget({ 10, 93 }, { 346, 12 }, widget_type::wt_18, 1, string_ids::STR_0086),
            make_widget({ 344, 94 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 108 }, { 173, 12 }, widget_type::wt_11, 1, string_ids::STR_1542, string_ids::STR_1543),
            widget_end(),
        };

        static loco_global<window_event_list, 0x00504028> _events;
    }

    namespace regional
    {
        static const gfx::ui_size_t _window_size = { 366, 147 };

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_regional),
            make_widget({ 183, 49 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0091),
            make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 64 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0088),
            make_widget({ 344, 65 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 84 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_0090, string_ids::STR_1502),
            make_widget({ 344, 85 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::STR_1502),
            make_widget({ 183, 99 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::STR_1506, string_ids::STR_1503),
            make_widget({ 344, 100 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::STR_1503),
            make_widget({ 10, 114 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1500, string_ids::STR_1501),
            make_widget({ 10, 129 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1498, string_ids::STR_1499),
            widget_end(),
        };

        static loco_global<window_event_list, 0x0050409C> _events;
    }

    namespace controls
    {
        static const gfx::ui_size_t _window_size = { 366, 87 };

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_controls),
            make_widget({ 10, 50 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1002, string_ids::STR_1003),
            make_widget({ 26, 64 }, { 160, 12 }, widget_type::wt_11, 1, string_ids::STR_0701, string_ids::STR_1004),
            widget_end(),
        };

        static loco_global<window_event_list, 0x00504110> _events;
    }

    namespace misc
    {
        static const gfx::ui_size_t _window_size = { 420, 102 };

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_miscellaneous),
            make_widget({ 10, 49 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::STR_1919, string_ids::STR_1920),
            make_widget({ 335, 64 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::STR_1700),
            make_widget({ 10, 79 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::STR_2089, string_ids::STR_2090),
            widget_end(),
        };

        static loco_global<window_event_list, 0x00504184> _events;
    }

    static void sub_4BF8CD()
    {
        call(0x004BF8CD);
    }

    static void sub_4C1519()
    {
        call(0x004C1519);
    }

    // 0x004BF7B9
    void open()
    {
        if (windowmgr::bring_to_front(window_type::options, 0) != nullptr)
            return;

        display::init_events();

        // 0x004BF833 (create_options_window)
        auto window = windowmgr::create_window_centred(
            window_type::options,
            display::_window_size.width,
            display::_window_size.height,
            0,
            &display::_events);

        window->widgets = display::_widgets;
        window->number = 0;
        window->var_870 = 0;
        window->var_872 = 0;

        auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;

        sub_4BF8CD();
        sub_4C1519();

        // Returning to 0x004BF7CB (in windowmgr__open_options)
        window->enabled_widgets = -1;  // !!! TODO
        window->holdable_widgets = -1; // !!! TODO
        window->event_handlers = &display::_events;
        window->activated_widgets = 0;

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();
    }
}
