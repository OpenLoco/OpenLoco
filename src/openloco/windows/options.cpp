#include "../config.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/currency_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui.h"
#include "../ui/dropdown.h"
#include "../windowmgr.h"

using namespace openloco::interop;

namespace openloco::ui::options
{
    static void on_mouse_up(window* w, widget_index wi);

    static loco_global<char[16], 0x0112C826> _commonFormatArgs;

#define set_format_arg(a, b, c) *((b*)(&_commonFormatArgs[a])) = (c)

    static void sub_4CF194(window* w, gfx::drawpixelinfo_t* ctx, uint32_t imageId, widget_index index)
    {
        auto widget = &w->widgets[index];

        gfx::point_t pos = {};
        pos.x = widget->left + w->x;
        pos.y = widget->top + w->y;

        if (w->is_disabled(index))
        {
            return; // 0x8000
        }

        bool isActivated = false;
        if (w->is_activated(index))
        {
            isActivated = true;
        }
        else if (input::state() == input::input_state::widget_pressed)
        {
            isActivated = input::is_pressed(w->type, w->number, index);
        }

        if (imageId == -1)
        {
            return;
        }

        if (isActivated)
        {
            if (imageId != -2)
            {
                gfx::draw_image(ctx, pos.x, pos.y, imageId);
            }
        }
        else
        {
            if (imageId != -2)
            {
                gfx::draw_image(ctx, pos.x, pos.y + 1, imageId);
            }
            gfx::draw_image(ctx, pos.x, pos.y, 0x40000000 | (51 << 19) | 2387);
            gfx::draw_rect(ctx, pos.x, pos.y + 26, 31, 1, colour::get_shade(w->colours[1], 7));
        }
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
                tab_controls,
                tab_miscellaneous,
            };
        }

        enum tab
        {
            display,
            sound,
            music,
            regional,
            controls,
            miscellaneous,
        };

        static void draw_tabs(window* w, gfx::drawpixelinfo_t* ctx)
        {
            sub_4CF194(w, ctx, image_ids::tab_display, widx::tab_display);
            sub_4CF194(w, ctx, image_ids::tab_sound, widx::tab_sound);

            if (w->var_870 == 2)
            {
                //     w->var_872 >> 2 % 16;
            }
            sub_4CF194(w, ctx, image_ids::tab_music_0, widx::tab_music);

            if (w->var_870 == 3)
            {
                //      w->var_872 >> 1 % 32;
            }
            sub_4CF194(w, ctx, image_ids::tab_globe_0, widx::tab_regional);

            sub_4CF194(w, ctx, image_ids::tab_control, widx::tab_controls);
            sub_4CF194(w, ctx, image_ids::tab_miscellaneous, widx::tab_miscellaneous);
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
            switch (wi)
            {
                case common::widx::close_button:
                    windowmgr::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::on_mouse_up(w, wi);
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

        // mouse down
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
            switch (wi)
            {
                case 11:
                    sub_4C0026();
                    break;
                case 19:
                    sub_4BFE2E(w);
                    break;
                case 15:
                    sub_4BFEBE();
                    break;
                case 17:
                    sub_4BFF72();
                    break;
            }
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

            common::draw_tabs(w, dpi);

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

        // 0x004C0217
        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::sound);
            assert(w->widgets == _widgets);

            w->activated_widgets &= 0xFFFFFC0F;
            w->activated_widgets |= 1 << (w->var_870 + 4);

            w->widgets[0].right = w->width - 1;
            w->widgets[0].bottom = w->height - 1;
            w->widgets[3].right = w->width - 1;
            w->widgets[3].bottom = w->height - 1;
            w->widgets[1].right = w->width - 2;
            w->widgets[2].left = w->width - 15;
            w->widgets[2].right = w->width - 15 + 12;

            set_format_arg(0x0, string_id, string_ids::str_479);

            /*if(_currentSoundDevice != -1 && _numSoundDevices != 0) {
                set_format_arg(0x2, uint32_t, soundDevices[_currentSoundDevice].name);
                set_format_arg(0x0, string_id, string_ids::str_480);
            }*/

            set_format_arg(0xA, string_id, string_ids::str_651 + openloco::config::get().sound_quality);

            w->activated_widgets &= ~(1 << 14);
            if (config::get().force_software_audio_mixer)
            {
                w->activated_widgets |= (1 << 14);
            }

            registers regs;
            regs.esi = (uintptr_t)w;
            call(0x4C13BE, regs);
        }

        // 0x004C02F5
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            common::draw_tabs(w, dpi);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[0xC].top + 1, 0, string_ids::str_650, nullptr);
        }

        static void on_mouse_up(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    windowmgr::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::on_mouse_up(w, wi);
                    return;
            }
        }

        static void init_events()
        {
            _events->on_mouse_up = on_mouse_up;
            _events->on_mouse_down = nullptr;
            _events->on_dropdown = nullptr;
            _events->on_update = nullptr;
            _events->prepare_draw = prepare_draw;
            _events->draw = draw;
        }
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

        // 0x004C05F9
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            common::draw_tabs(w, dpi);

            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[0xB].top, 0, string_ids::str_1535, nullptr);

            gfx::draw_string_494B3F(*dpi, w->x + 183, w->y + w->widgets[0xF].top + 7, 0, string_ids::str_1547, nullptr);

            // TODO: images
        }

        static void on_mouse_up(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    windowmgr::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::on_mouse_up(w, wi);
                    return;
            }
        }

        static void init_events()
        {
            _events->on_mouse_up = on_mouse_up;
            _events->on_mouse_down = nullptr;
            _events->on_dropdown = nullptr;
            _events->on_update = nullptr;
            _events->prepare_draw = nullptr;
            _events->draw = draw;
        }
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

        // 0x004C0A59
        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::regional);
            assert(w->widgets == _widgets);

            w->activated_widgets &= 0xFFFFFC0F;
            w->activated_widgets |= 1 << (w->var_870 + 4);

            w->widgets[0].right = w->width - 1;
            w->widgets[0].bottom = w->height - 1;
            w->widgets[3].right = w->width - 1;
            w->widgets[3].bottom = w->height - 1;
            w->widgets[1].right = w->width - 2;
            w->widgets[2].left = w->width - 15;
            w->widgets[2].right = w->width - 15 + 12;

            string_id ax = string_ids::str_660;
            if (openloco::config::get().flags & 2)
            {
                ax = string_ids::str_661;
            }

            set_format_arg(0x6, string_id, ax);
            set_format_arg(0xC, string_id, string_ids::str_658 + openloco::config::get().measurement_format);
            set_format_arg(0xA, string_id, objectmgr::get<currency_object>()->name);

            w->activated_widgets &= ~(1 << 18);
            if (config::get().flags & 0x40)
            {
                w->activated_widgets |= (1 << 18);
            }

            w->activated_widgets &= ~(1 << 19);
            if (config::get().flags & 0x80)
            {
                w->activated_widgets |= (1 << 19);
            }

            w->disabled_widgets &= ~(1 << 14);
            w->disabled_widgets &= ~(1 << 15);
            if (config::get().flags & 0x80)
            {
                w->disabled_widgets |= (1 << 14);
                w->disabled_widgets |= (1 << 15);
            }

            registers regs;
            regs.esi = (uintptr_t)w;
            call(0x4C13BE, regs);
        }

        // 0x004C0B5B
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);
            common::draw_tabs(w, dpi);

            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[0xA].top + 1, 0, string_ids::str_656, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[0xC].top + 1, 0, string_ids::str_657, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[0xE].top + 1, 0, string_ids::str_1504, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[0x10].top + 1, 0, string_ids::str_1505, nullptr);
        }

        static void on_mouse_up(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    windowmgr::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::on_mouse_up(w, wi);
                    return;
            }
        }

        static void init_events()
        {
            _events->on_mouse_up = on_mouse_up;
            _events->on_mouse_down = nullptr;
            _events->on_dropdown = nullptr;
            _events->on_update = nullptr;
            _events->prepare_draw = prepare_draw;
            _events->draw = draw;
        }
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

        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::controls);
            assert(w->widgets == _widgets);

            w->activated_widgets &= 0xFFFFFC0F;
            w->activated_widgets |= 1 << (w->var_870 + 4);

            w->widgets[0].right = w->width - 1;
            w->widgets[0].bottom = w->height - 1;
            w->widgets[3].right = w->width - 1;
            w->widgets[3].bottom = w->height - 1;
            w->widgets[1].right = w->width - 2;
            w->widgets[2].left = w->width - 15;
            w->widgets[2].right = w->width - 15 + 12;

            w->activated_widgets &= ~(1 << 10);
            if (config::get().edge_scrolling)
            {
                w->activated_widgets |= (1 << 10);
            }

            registers regs;
            regs.esi = (uintptr_t)w;
            call(0x4C13BE, regs);
        }

        // 0x004C113F
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            w->draw(dpi);
            common::draw_tabs(w, dpi);
        }

        static void sub_4C117A(window* w);
        static void sub_4C118D();

        // 0x004C114A
        static void on_mouse_up(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    windowmgr::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::on_mouse_up(w, wi);
                    return;

                case 11:
                    sub_4C118D();
                    break;

                case 10:
                    sub_4C117A(w);
                    break;
            }
        }

        static void sub_4C117A(window* w)
        {
            auto cfg = &openloco::config::get();
            cfg->edge_scrolling = !cfg->edge_scrolling;
            config::write();

            w->invalidate();
        }

        static void sub_4C118D()
        {
            call(0x004BE6C7);
        }

        // 0x004C1195
        static void on_update(window* w)
        {
            w->var_872 += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->var_870 + 4);
        }

        static void init_events()
        {
            _events->on_mouse_up = on_mouse_up;
            _events->on_update = on_update;
            _events->prepare_draw = prepare_draw;
            _events->draw = draw;
        }
    }

    namespace misc
    {
        static const gfx::ui_size_t _window_size = { 420, 102 };

        namespace widx
        {
            enum
            {
                use_preferred_owner_name = 10,
                change_btn,
                export_plugin_objects,
            };
        }

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_miscellaneous),
            make_widget({ 10, 49 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::STR_1919, string_ids::STR_1920),
            make_widget({ 335, 64 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::STR_1700),
            make_widget({ 10, 79 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::STR_2089, string_ids::STR_2090),
            widget_end(),
        };

        static loco_global<window_event_list, 0x00504184> _events;

        static loco_global<uint8_t, 0x0112A17E> _112A17E;

        static void sub_4C1319(window* w);
        static void sub_4C1342(window* w, char* str);
        static void sub_4C135F(window* w);
        static void sub_4C1389(window* w);

        // 0x004C11B7
        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::miscellaneous);
            //  assert(w->widgets == _widgets);

            w->activated_widgets &= 0x0FFFFFC0F;
            w->activated_widgets |= 1 << (w->var_870 + 4);

            w->widgets[0].right = w->width - 1;
            w->widgets[0].bottom = w->height - 1;
            w->widgets[3].right = w->width - 1;
            w->widgets[3].bottom = w->height - 1;
            w->widgets[1].right = w->width - 2;
            w->widgets[2].left = w->width - 15;
            w->widgets[2].right = w->width - 15 + 12;

            w->activated_widgets &= ~(1 << widx::export_plugin_objects);
            if (config::get().flags & 8)
            {
                w->activated_widgets |= (1 << widx::export_plugin_objects);
            }

            w->activated_widgets &= ~(1 << widx::use_preferred_owner_name);
            w->disabled_widgets |= (1 << widx::change_btn);
            if (config::get().flags & 0x200)
            {
                w->activated_widgets |= (1 << widx::use_preferred_owner_name);
                w->disabled_widgets &= ~(1 << widx::change_btn);
            }

            w->widgets[widx::export_plugin_objects].type = widget_type::none;
            if (_112A17E)
            {
                w->widgets[widx::export_plugin_objects].type = widget_type::checkbox;
            }

            registers regs;
            regs.esi = (uint32_t)w;
            call(0x004C13BE, regs);
        }

        // 0x004C1282
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            w->draw(dpi);
            common::draw_tabs(w, dpi);

            auto buffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
            char* playerName = config::get().preferred_name;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            *((string_id*)(&_commonFormatArgs[0])) = string_ids::buffer_2039;
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[0xB].top + 1, 0, string_ids::str_1921, _commonFormatArgs);
        }

        // 0x004C12D2
        static void on_mouse_up(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    windowmgr::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::on_mouse_up(w, wi);
                    return;

                case 12:
                    sub_4C1389(w);
                    break;

                case 10:
                    sub_4C135F(w);
                    break;

                case 11:
                    sub_4C1319(w);
                    break;
            }
        }

        // 0x004C1304
        static void text_input(window* w, widget_index i, char* str)
        {
            switch (i)
            {
                case 11:
                    sub_4C1342(w, str);
                    break;
            }
        }

        static void sub_4C1319(window* w)
        {
            auto buffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
            char* playerName = config::get().preferred_name;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            textinput::open_textinput(w, string_ids::str_1922, string_ids::str_1923, string_ids::buffer_2039, 11, nullptr);
        }

        static void sub_4C1342(window* w, char* str)
        {
            if (strlen(str) == 0)
                return;

            auto cfg = &openloco::config::get();
            strcpy(cfg->preferred_name, str);
            cfg->preferred_name[strlen(str)] = '\0';

            config::write();

            w->invalidate();
        }

        static void sub_4C135F(window* w)
        {
            auto cfg = &openloco::config::get();
            if (cfg->flags & 0x200)
            {
                cfg->flags &= ~0x200;
            }
            else
            {
                cfg->flags |= 0x200;
            }
            config::write();

            w->invalidate();

            if (cfg->flags & 0x200)
            {
                if (strlen(cfg->preferred_name) == 0)
                {
                    sub_4C1319(w);
                }
            }
        }

        static void sub_4C1389(window* w)
        {
            auto cfg = &openloco::config::get();
            if (cfg->flags & 8)
            {
                cfg->flags &= ~8;
            }
            else
            {
                cfg->flags |= 8;
            }
            config::write();

            w->invalidate();
        }

        // 0x004C139C
        static void on_update(window* w)
        {
            w->var_872 += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->var_870 + 4);
        }

        static void init_events()
        {
            _events->on_mouse_up = on_mouse_up;
            _events->on_update = on_update;
            _events->text_input = text_input;
            _events->prepare_draw = prepare_draw;
            _events->draw = draw;
        }
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
        sound::init_events();
        music::init_events();
        regional::init_events();
        controls::init_events();
        misc::init_events();

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

    // 0x004BFC11
    static void on_mouse_up(window* w, widget_index wi)
    {
        input::cancel_tool(w->type, w->number);

        textinput::sub_4CE6C9(w->type, w->number);
        w->var_870 = wi - common::widx::tab_display;
        w->var_872 = 0;
        w->flags &= ~(window_flags::flag_16);
        w->disabled_widgets = 0;
        w->holdable_widgets = 0;
        w->activated_widgets = 0;
        if (w->viewports[0])
        {
            w->viewports[0]->width = 0;
            w->viewports[0] = nullptr;
        }

        switch ((common::tab)w->var_870)
        {
            case common::tab::display:
                w->enabled_widgets = 0x0FFFF4;
                w->event_handlers = &display::_events;
                w->widgets = display::_widgets;
                w->invalidate();
                //
                w->set_size(display::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::sound:
                w->enabled_widgets = 0x7FF4;
                w->event_handlers = &(*sound::_events);
                w->widgets = sound::_widgets;
                w->invalidate();
                //
                w->set_size(sound::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::music:
                w->enabled_widgets = 0x7FFF4;
                w->holdable_widgets = 0x8000;
                w->event_handlers = &(*music::_events);
                w->widgets = music::_widgets;
                w->invalidate();
                //
                w->set_size(music::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::regional:
                w->enabled_widgets = 0x0FFFF4;
                w->holdable_widgets = 0;
                w->event_handlers = &(*regional::_events);
                w->widgets = regional::_widgets;
                w->invalidate();
                //
                w->set_size(regional::_window_size);
                break;

            case common::tab::controls:
                w->enabled_widgets = 0x0FF4;
                w->event_handlers = &(*controls::_events);
                w->widgets = controls::_widgets;
                w->invalidate();
                //
                w->set_size(controls::_window_size);
                break;

            case common::tab::miscellaneous:
                w->enabled_widgets = 0x1FF4;
                w->event_handlers = &(*misc::_events);
                w->widgets = misc::_widgets;
                w->invalidate();
                //
                w->set_size(misc::_window_size);
                break;
        }

        w->call_on_resize();
        w->call_prepare_draw();
        w->init_scroll_widgets();
        w->invalidate();
    }
}
