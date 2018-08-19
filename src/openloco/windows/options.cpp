#include "../audio/audio.h"
#include "../config.h"
#include "../date.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/languagefiles.h"
#include "../localisation/languages.h"
#include "../localisation/string_ids.h"
#include "../objects/currency_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui.h"
#include "../ui/dropdown.h"
#include "../windowmgr.h"
#include <cassert>

using namespace openloco::interop;

namespace openloco::ui::options
{
    static void tab_on_mouse_up(window* w, widget_index wi);
    static void sub_4C13BE(window* w);
    static void sub_4C1519();
    static void sub_4BF935();

    static loco_global<uint8_t, 0x009C8714> _9C8714;

    static loco_global<uint32_t, 0x0050D430> _50D430;
    static loco_global<int8_t, 0x0050D434> _currentSong;
    static loco_global<uint8_t, 0x0050D435> _50D435;
    static loco_global<uint8_t[3], 0x0050D5B5> _50D5B5;
    static loco_global<uint8_t[3], 0x0050D5B8> _50D5B8;

    // Should be a pointer to an array of u8's
    static loco_global<void*, 0x011364A0> __11364A0;
    static loco_global<uint16_t, 0x0112C185> _112C185;
    static loco_global<char[20], 0x0112C826> _commonFormatArgs;

#define set_format_arg(a, b, c) *((b*)(&_commonFormatArgs[a])) = (c)

    static void on_close(window* w)
    {
        free(__11364A0);
    }

    // 0x004CF194
    static void draw_tab(window* w, gfx::drawpixelinfo_t* ctx, int32_t imageId, widget_index index)
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
                frame = 0,
                caption = 1,
                close_button = 2,
                panel = 3,
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
            draw_tab(w, ctx, image_ids::tab_display, widx::tab_display);
            draw_tab(w, ctx, image_ids::tab_sound, widx::tab_sound);

            static const uint32_t music_tab_ids[] = {
                image_ids::tab_music_0,
                image_ids::tab_music_1,
                image_ids::tab_music_2,
                image_ids::tab_music_3,
                image_ids::tab_music_4,
                image_ids::tab_music_5,
                image_ids::tab_music_6,
                image_ids::tab_music_7,
                image_ids::tab_music_8,
                image_ids::tab_music_9,
                image_ids::tab_music_10,
                image_ids::tab_music_11,
                image_ids::tab_music_12,
                image_ids::tab_music_13,
                image_ids::tab_music_14,
                image_ids::tab_music_15,
            };
            uint32_t imageId = music_tab_ids[0];
            if (w->current_tab == tab::music)
            {
                imageId = music_tab_ids[(w->frame_no / 4) % 16];
            }
            draw_tab(w, ctx, imageId, widx::tab_music);

            static const uint32_t globe_tab_ids[] = {
                image_ids::tab_globe_0,
                image_ids::tab_globe_1,
                image_ids::tab_globe_2,
                image_ids::tab_globe_3,
                image_ids::tab_globe_4,
                image_ids::tab_globe_5,
                image_ids::tab_globe_6,
                image_ids::tab_globe_7,
                image_ids::tab_globe_8,
                image_ids::tab_globe_9,
                image_ids::tab_globe_10,
                image_ids::tab_globe_11,
                image_ids::tab_globe_12,
                image_ids::tab_globe_13,
                image_ids::tab_globe_14,
                image_ids::tab_globe_15,
                image_ids::tab_globe_16,
                image_ids::tab_globe_17,
                image_ids::tab_globe_18,
                image_ids::tab_globe_19,
                image_ids::tab_globe_20,
                image_ids::tab_globe_21,
                image_ids::tab_globe_22,
                image_ids::tab_globe_23,
                image_ids::tab_globe_24,
                image_ids::tab_globe_25,
                image_ids::tab_globe_26,
                image_ids::tab_globe_27,
                image_ids::tab_globe_28,
                image_ids::tab_globe_29,
                image_ids::tab_globe_30,
                image_ids::tab_globe_31,
            };
            imageId = image_ids::tab_globe_0;
            if (w->current_tab == tab::regional)
            {
                imageId = globe_tab_ids[(w->frame_no / 2) % 32];
            }
            draw_tab(w, ctx, imageId, widx::tab_regional);

            draw_tab(w, ctx, image_ids::tab_control, widx::tab_controls);
            draw_tab(w, ctx, image_ids::tab_miscellaneous, widx::tab_miscellaneous);
        }

#define common_options_widgets(window_size, window_caption_id)                                                                                              \
    make_widget({ 0, 0 }, window_size, widget_type::frame, 0),                                                                                              \
        make_widget({ 1, 1 }, { (uint16_t)(window_size.width - 2), 13 }, widget_type::caption_25, 0, window_caption_id),                                    \
        make_widget({ (int16_t)(window_size.width - 15), 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { window_size.width, 102 }, widget_type::panel, 1),                                                                          \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::display_options),                                        \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::sound_options),                                         \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::music_options),                                         \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::regional_options),                                      \
        make_remap_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::control_options),                                      \
        make_remap_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::miscellaneous_options)

        static constexpr int tabWidgets = (1 << widx::tab_display) | (1 << widx::tab_sound) | (1 << widx::tab_music) | (1 << widx::tab_regional) | (1 << widx::tab_controls) | (1 << widx::tab_miscellaneous);
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
            make_widget({ 183, 49 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::display_resolution_label_format),
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
                    options::tab_on_mouse_up(w, wi);
                    return;

                case widx::landscape_smoothing:
                {
                    auto& cfg = openloco::config::get();
                    // TODO: is there a better way to toggle a flag?
                    if (cfg.flags & config::flags::landscape_smoothing)
                    {
                        cfg.flags &= ~config::flags::landscape_smoothing;
                    }
                    else
                    {
                        cfg.flags |= config::flags::landscape_smoothing;
                    }
                    openloco::config::write();
                    gfx::invalidate_screen();
                    return;
                }

                case widx::gridlines_on_landscape:
                {
                    auto& cfg = openloco::config::get();
                    if (cfg.flags & config::flags::gridlines_on_landscape)
                    {
                        cfg.flags &= ~config::flags::gridlines_on_landscape;
                    }
                    else
                    {
                        cfg.flags |= config::flags::gridlines_on_landscape;
                    }
                    openloco::config::write();
                    gfx::invalidate_screen();

                    auto main = windowmgr::get_main();
                    if (main != nullptr)
                    {
                        main->viewports[0]->var_12 &= ~(1 << 5);

                        if ((cfg.flags & config::flags::gridlines_on_landscape) != 0)
                        {
                            main->viewports[0]->var_12 |= (1 << 5);
                        }
                    }

                    return;
                }
            }
        }

#pragma mark - Construction Marker (Widget 19)

        // 0x004BFE2E
        static void construction_marker_mouse_down(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[widx::construction_marker];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::white);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::translucent);
            dropdown::set_selection(config::get().construction_marker);
        }

        // 0x004BFE98
        static void construction_marker_dropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().construction_marker)
                return;

            auto& cfg = openloco::config::get();
            cfg.construction_marker = ax;
            openloco::config::write();
            gfx::invalidate_screen();
        }

#pragma mark - Vehicle zoom (Widget 15)

        // 0x004BFEBE
        static void vehicle_zoom_mouse_down(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[widx::vehicles_min_scale];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 4, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::full_scale);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::half_scale);
            dropdown::add(2, string_ids::dropdown_stringid, string_ids::quarter_scale);
            dropdown::add(3, string_ids::dropdown_stringid, string_ids::eighth_scale);
            dropdown::set_selection(config::get().vehicles_min_scale);
        }

        // 0x004BFF4C
        static void vehicle_zoom_dropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().vehicles_min_scale)
                return;

            auto& cfg = openloco::config::get();
            cfg.vehicles_min_scale = ax;
            openloco::config::write();
            gfx::invalidate_screen();
        }

#pragma mark - Station names minimum scale (Widget 17)

        // 0x004BFF72
        static void station_names_scale_mouse_down(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[widx::station_names_min_scale];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 4, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::full_scale);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::half_scale);
            dropdown::add(2, string_ids::dropdown_stringid, string_ids::quarter_scale);
            dropdown::add(3, string_ids::dropdown_stringid, string_ids::eighth_scale);
            dropdown::set_selection(config::get().station_names_min_scale);
        }

        // 0x004C0000
        static void station_names_scale_dropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().station_names_min_scale)
                return;

            auto& cfg = openloco::config::get();
            cfg.station_names_min_scale = ax;
            openloco::config::write();
            gfx::invalidate_screen();
        }

#pragma mark - Resolution dropdown (Widget 11)

        // 0x004C0026
        static void resolution_mouse_down(window* w, widget_index wi)
        {
            std::vector<Resolution> resolutions = getFullscreenResolutions();

            widget_t dropdown = w->widgets[widx::display_resolution];
            dropdown::show_text_2(w->x + dropdown.left, w->y + dropdown.top, dropdown.width(), dropdown.height(), w->colours[1], (int8_t)resolutions.size(), 0x80);

            auto& cfg = config::get();
            for (size_t i = 0; i < resolutions.size(); i++)
            {
                dropdown::add((int16_t)i, string_ids::dropdown_stringid, { string_ids::display_resolution_dropdown_format, (uint16_t)resolutions[i].width, (uint16_t)resolutions[i].height });
                if (cfg.resolution_width == resolutions[i].width && cfg.resolution_height == resolutions[i].height)
                    dropdown::set_selection((int16_t)i);
            }
        }

        // 0x004C00F4
        static void resolution_dropdown(window* w, int16_t index)
        {
            if (index == -1)
                return;

            auto& config = config::get();

            std::vector<Resolution> resolutions = getFullscreenResolutions();
            if (config.resolution_width == resolutions[index].width && config.resolution_height == resolutions[index].height)
                return;

            config.resolution_width = resolutions[index].width;
            config.resolution_height = resolutions[index].height;
            openloco::config::write();
            windowmgr::invalidate_widget(w->type, w->number, widx::display_resolution);
        }

#pragma mark -

        // 0x004BFBB7
        static void on_mouse_down(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::display_resolution_btn:
                    resolution_mouse_down(w, wi);
                    break;
                case widx::construction_marker_btn:
                    construction_marker_mouse_down(w, wi);
                    break;
                case widx::vehicles_min_scale_btn:
                    vehicle_zoom_mouse_down(w, wi);
                    break;
                case widx::station_names_min_scale_btn:
                    station_names_scale_mouse_down(w, wi);
                    break;
            }
        }

        // 0x004BFBE8
        static void on_dropdown(window* w, widget_index wi, int16_t item_index)
        {
            switch (wi)
            {
                case widx::display_resolution_btn:
                    resolution_dropdown(w, item_index);
                    break;
                case widx::construction_marker_btn:
                    construction_marker_dropdown(item_index);
                    break;
                case widx::vehicles_min_scale_btn:
                    vehicle_zoom_dropdown(item_index);
                    break;
                case widx::station_names_min_scale_btn:
                    station_names_scale_dropdown(item_index);
                    break;
            }
        }

        // 0x004C01F5
        static void on_update(window* w)
        {
            w->frame_no += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->current_tab + 4);
        }

        // 0x004BFA04
        static void prepare_draw(window* w)
        {
            assert(w->current_tab == common::tab::display);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            set_format_arg(0x10, uint16_t, config::get().resolution_width);
            set_format_arg(0x12, uint16_t, config::get().resolution_height);

            if (config::get().construction_marker)
                w->widgets[widx::construction_marker].text = string_ids::translucent;
            else
                w->widgets[widx::construction_marker].text = string_ids::white;

            static const string_id scale_string_ids[] = {
                string_ids::full_scale,
                string_ids::half_scale,
                string_ids::quarter_scale,
                string_ids::eighth_scale,
            };

            w->widgets[widx::vehicles_min_scale].text = scale_string_ids[config::get().vehicles_min_scale];
            w->widgets[widx::station_names_min_scale].text = scale_string_ids[config::get().station_names_min_scale];

            w->activated_widgets &= ~(1 << widx::landscape_smoothing);
            if ((config::get().flags & config::flags::landscape_smoothing) == 0)
            {
                w->activated_widgets |= (1 << widx::landscape_smoothing);
            }

            w->activated_widgets &= ~(1 << widx::gridlines_on_landscape);
            if (config::get().flags & config::flags::gridlines_on_landscape)
            {
                w->activated_widgets |= (1 << widx::gridlines_on_landscape);
            }

            sub_4C13BE(w);
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
            _events.on_close = on_close;
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

        namespace widx
        {
            enum
            {
                audio_device = 10,
                audio_device_btn,
                sound_quality,
                sound_quality_btn,
                force_software_audio_mixer
            };
        }

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_sound),
            make_widget({ 10, 49 }, { 346, 12 }, widget_type::wt_18, 1, string_ids::stringid),
            make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 64 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::arg10_stringid),
            make_widget({ 344, 65 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 79 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::forced_software_buffer_mixing, string_ids::forced_software_buffer_mixing_tip),
            widget_end(),
        };

        static window_event_list _events;
        static loco_global<int32_t, 0x0050D1E8> _currentSoundDevice;
        static loco_global<int32_t, 0x005251F0> _numSoundDevices;
        static loco_global<uintptr_t, 0x005251F4> _soundDevices;

        static void force_software_audio_mixer_mouse_up(window* w);
        static void sound_quality_mouse_down(ui::window* window);
        static void sound_quality_dropdown(ui::window* window, int16_t itemIndex);
        static void audio_device_mouse_down(ui::window* window);
        static void audio_device_dropdown(ui::window* window, int16_t itemIndex);

        // 0x004C0217
        static void prepare_draw(window* w)
        {
            assert(w->current_tab == common::tab::sound);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            set_format_arg(0x0, string_id, string_ids::audio_device_none);

#ifdef _WIN32
            if (_currentSoundDevice != -1 && _numSoundDevices != 0)
            {
                set_format_arg(0x0, string_id, string_ids::stringptr);
                uintptr_t soundDevicePtr = _soundDevices + 0x10 + 0x210 * _currentSoundDevice;
                set_format_arg(0x2, char*, (char*)soundDevicePtr);
            }
#endif

            static const string_id sound_quality_strings[] = {
                string_ids::sound_quality_low,
                string_ids::sound_quality_medium,
                string_ids::sound_quality_high,
            };
            set_format_arg(0xA, string_id, sound_quality_strings[openloco::config::get().sound_quality]);

            w->activated_widgets &= ~(1 << widx::force_software_audio_mixer);
            if (config::get().force_software_audio_mixer)
            {
                w->activated_widgets |= (1 << widx::force_software_audio_mixer);
            }

            sub_4C13BE(w);
        }

        // 0x004C02F5
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            common::draw_tabs(w, dpi);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::sound_quality].top + 1, 0, string_ids::sound_quality, nullptr);
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
                    options::tab_on_mouse_up(w, wi);
                    return;

                case widx::force_software_audio_mixer:
                    force_software_audio_mixer_mouse_up(w);
                    return;
            }
        }

        static void on_mouse_down(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::audio_device_btn:
                    audio_device_mouse_down(w);
                    break;

                case widx::sound_quality_btn:
                    sound_quality_mouse_down(w);
                    break;
            }
        }

        static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::audio_device_btn:
                    audio_device_dropdown(window, itemIndex);
                    break;

                case widx::sound_quality_btn:
                    sound_quality_dropdown(window, itemIndex);
                    break;
            }
        }

#pragma mark - Force software audio mixer (Widget 14)

        // 0x004C0371
        static void force_software_audio_mixer_mouse_up(window* w)
        {
            audio::pause_sound();

            auto& cfg = openloco::config::get();
            cfg.force_software_audio_mixer = !cfg.force_software_audio_mixer;
            openloco::config::write();

            audio::unpause_sound();
            w->invalidate();
        }

#pragma mark - Widget 13

        // 0x004C03A4
        static void sound_quality_mouse_down(ui::window* w)
        {
            widget_t dropdown = w->widgets[widx::sound_quality];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 3, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::sound_quality_low);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::sound_quality_medium);
            dropdown::add(2, string_ids::dropdown_stringid, string_ids::sound_quality_high);
            dropdown::set_selection(config::get().sound_quality);
        }

        // 0x004C040A
        static void sound_quality_dropdown(ui::window* w, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

            auto& cfg = openloco::config::get();
            cfg.sound_quality = itemIndex;
            cfg.var_25 = _50D5B5[itemIndex];
            cfg.var_26 = _50D5B8[itemIndex];
            openloco::config::write();

            w->invalidate();
        }

#pragma mark - Widget 11

        // 0x004C043D
        static void audio_device_mouse_down(ui::window* w)
        {
#ifdef _WIN32
            if (_numSoundDevices == 0)
                return;

            widget_t dropdown = w->widgets[widx::audio_device];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], _numSoundDevices, 0x80);

            uintptr_t dsoundPtr = _soundDevices + 0x10;
            for (int i = 0; i < _numSoundDevices; i++)
            {
                dropdown::add(i, string_ids::dropdown_stringid, { string_ids::stringptr, (char*)dsoundPtr });
                dsoundPtr += 0x210;
            }

            dropdown::set_selection(_currentSoundDevice);
#endif
        }

        // 0x004C04CA
        static void audio_device_dropdown(ui::window* window, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

#ifdef _WIN32
            // Updates config, switches audio devices.
            registers regs;
            regs.eax = itemIndex;
            call(0x00489A37, regs);
#endif
        }

#pragma mark -

        // 0x004C04E0
        static void on_update(window* w)
        {
            w->frame_no += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->current_tab + 4);
        }

        static void init_events()
        {
            _events.on_close = on_close;
            _events.on_mouse_up = on_mouse_up;
            _events.on_mouse_down = on_mouse_down;
            _events.on_dropdown = on_dropdown;
            _events.on_update = on_update;
            _events.prepare_draw = prepare_draw;
            _events.draw = draw;
        }
    }

    namespace music
    {
        static const gfx::ui_size_t _window_size = { 366, 129 };

        namespace widx
        {
            enum
            {
                audio_device = 10,
                audio_device_btn,
                music_controls_stop,
                music_controls_play,
                music_controls_next,
                volume,
                music_playlist,
                music_playlist_btn,
                edit_selection
            };
        }

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_music),
            make_widget({ 160, 49 }, { 196, 12 }, widget_type::wt_18, 1, string_ids::stringid),
            make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 10, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_stop, string_ids::music_controls_stop_tip),
            make_widget({ 34, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_play, string_ids::music_controls_stop_tip),
            make_widget({ 58, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_next, string_ids::music_controls_stop_tip),
            make_widget({ 256, 64 }, { 109, 24 }, widget_type::wt_5, 1, -1, string_ids::set_volume_tip),
            make_widget({ 10, 93 }, { 346, 12 }, widget_type::wt_18, 1, string_ids::arg2_stringid),
            make_widget({ 344, 94 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 108 }, { 173, 12 }, widget_type::wt_11, 1, string_ids::edit_music_selection, string_ids::edit_music_selection_tip),
            widget_end(),
        };

        static void volume_mouse_down(window* w);
        static void open_music_selection();
        static void stop_music(window* w);
        static void play_music(window* w);
        static void play_next_song(window* w);
        static void music_playlist_mouse_down(window* w);
        static void music_playlist_dropdown(window* w, int16_t ax);
        static void audio_device_mouse_down(window* w);
        static void audio_device_dropdown(window* w, int16_t ax);

        static window_event_list _events;

        static const string_id music_ids_to_string_id[] = {
            string_ids::music_chuggin_along,
            string_ids::music_long_dusty_road,
            string_ids::music_flying_high,
            string_ids::music_gettin_on_the_gas,
            string_ids::music_jumpin_the_rails,
            string_ids::music_smooth_running,
            string_ids::music_traffic_jam,
            string_ids::music_never_stop_til_you_get_there,
            string_ids::music_soaring_away,
            string_ids::music_techno_torture,
            string_ids::music_everlasting_high_rise,
            string_ids::music_solace,
            string_ids::music_chrysanthemum,
            string_ids::music_eugenia,
            string_ids::music_the_ragtime_dance,
            string_ids::music_easy_winners,
            string_ids::music_setting_off,
            string_ids::music_a_travellers_seranade,
            string_ids::music_latino_trip,
            string_ids::music_a_good_head_of_steam,
            string_ids::music_hop_to_the_bop,
            string_ids::music_the_city_lights,
            string_ids::music_steamin_down_town,
            string_ids::music_bright_expectations,
            string_ids::music_mo_station,
            string_ids::music_far_out,
            string_ids::music_running_on_time,
            string_ids::music_get_me_to_gladstone_bay,
            string_ids::music_sandy_track_blues,
        };

        static void prepare_draw(window* w)
        {
            assert(w->current_tab == common::tab::music);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            string_id songName = string_ids::music_none;
            if (_currentSong != -1)
            {
                songName = music_ids_to_string_id[_currentSong];
            }
            set_format_arg(0, string_id, songName);

            static const string_id playlist_string_ids[] = {
                string_ids::play_only_music_from_current_era,
                string_ids::play_all_music,
                string_ids::play_custom_music_selection,
            };

            set_format_arg(2, string_id, playlist_string_ids[config::get().music_playlist]);

            w->activated_widgets &= ~((1 << widx::music_controls_stop) | (1 << widx::music_controls_play));
            w->activated_widgets |= (1 << widx::music_controls_stop);
            if (_currentSong != -1)
            {
                if (config::get().music_playing)
                {
                    w->activated_widgets &= ~((1 << widx::music_controls_stop) | (1 << widx::music_controls_play));
                    w->activated_widgets |= (1 << widx::music_controls_play);
                }
            }

            w->disabled_widgets |= (1 << widx::edit_selection);
            if (config::get().music_playlist == 2)
            {
                w->disabled_widgets &= ~(1 << widx::edit_selection);
            }

            sub_4C13BE(w);
        }

        // 0x004C05F9
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            common::draw_tabs(w, dpi);

            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::audio_device_btn].top, 0, string_ids::currently_playing, nullptr);

            gfx::draw_string_494B3F(*dpi, w->x + 183, w->y + w->widgets[widx::volume].top + 7, 0, string_ids::volume, nullptr);

            gfx::draw_image(dpi, w->x + w->widgets[widx::volume].left, w->y + w->widgets[widx::volume].top, 0x20000000 | (w->colours[1] << 19) | image_ids::volume_slider_track);

            int16_t x = 90 + (config::get().volume / 32);
            gfx::draw_image(dpi, w->x + w->widgets[widx::volume].left + x, w->y + w->widgets[widx::volume].top, 0x20000000 | (w->colours[1] << 19) | image_ids::volume_slider_thumb);
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
                    options::tab_on_mouse_up(w, wi);
                    return;

                case widx::music_controls_stop:
                    stop_music(w);
                    return;

                case widx::music_controls_play:
                    play_music(w);
                    return;

                case widx::music_controls_next:
                    play_next_song(w);
                    return;

                case widx::edit_selection:
                    open_music_selection();
                    return;
            }
        }

        // 0x004C06F2
        static void on_mouse_down(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::music_playlist_btn:
                    music_playlist_mouse_down(w);
                    break;
                case widx::audio_device_btn:
                    audio_device_mouse_down(w);
                    break;
                case widx::volume:
                    volume_mouse_down(w);
                    break;
            }
        }

        // 0x004C070D
        static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::music_playlist_btn:
                    music_playlist_dropdown(window, itemIndex);
                    break;
                case widx::audio_device_btn:
                    audio_device_dropdown(window, itemIndex);
                    break;
            }
        }

        static loco_global<int16_t, 0x005233A4> _5233A4;
        static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;

        // 0x004C072A
        static void volume_mouse_down(window* w)
        {
            _clickRepeatTicks = 31;

            int x = _5233A4 - w->x - w->widgets[widx::volume].left - 10;
            x = std::clamp(x, 0, 80);

            registers regs;
            regs.ecx = (x * 32) - 2560;
            call(0x0048AA67, regs);

            w->invalidate();
        }

        // 0x004C0770
        static void open_music_selection()
        {
            call(0x004C1602); // Open music selection
        }

        // 0x004C0778
        static void stop_music(window* w)
        {
            if (config::get().music_playing == 0)
                return;

            auto& cfg = config::get();
            cfg.music_playing = 0;
            config::write();

            call(0x0048AAE8);

            _currentSong = -1;

            w->invalidate();
        }

        // 0x004C07A4
        static void play_music(window* w)
        {
            if (config::get().music_playing != 0)
                return;

            auto& cfg = config::get();
            cfg.music_playing = 1;
            config::write();

            w->invalidate();
        }

        // 0x004C07C4
        static void play_next_song(window* w)
        {
            if (config::get().music_playing == 0)
                return;

            call(0x0048AAE8);

            _currentSong = -1;

            w->invalidate();
        }

#pragma mark - Widget 17

        // 0x004C07E4
        static void music_playlist_mouse_down(window* w)
        {
            widget_t dropdown = w->widgets[widx::music_playlist];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 3, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::play_only_music_from_current_era);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::play_all_music);
            dropdown::add(2, string_ids::dropdown_stringid, string_ids::play_custom_music_selection);

            dropdown::set_selection(config::get().music_playlist);
        }

        // 0x004C084A
        static void music_playlist_dropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = openloco::config::get();
            cfg.music_playlist = ax;
            config::write();

            w->invalidate();

            call(0x048AA0C);

            windowmgr::close(window_type::music_selection);
        }

#pragma mark - Widget 11

        struct unk1_t
        {
            uint8_t pad_0[0x8];
            uint16_t var_8;
            uint16_t var_A;
            uint16_t var_C;
        };

        static loco_global<unk1_t[29], 0x004FE910> _4FE910;

        static std::vector<int> get_available_tracks()
        {
            auto vector = std::vector<int>();

            if (config::get().music_playlist == 0)
            {
                uint16_t year = current_year();
                for (int i = 0; i < 29; i++)
                {
                    if (year >= _4FE910[i].var_8 && year <= _4FE910[i].var_A)
                    {
                        vector.push_back(i);
                    }
                }
            }
            else if (config::get().music_playlist == 1)
            {
                for (int i = 0; i < 29; i++)
                {
                    vector.push_back(i);
                }
            }
            else if (config::get().music_playlist == 2)
            {
                for (int i = 0; i < 29; i++)
                {
                    if (config::get().enabled_music[i] & 1)
                    {
                        vector.push_back(i);
                    }
                }

                if (vector.size() == 0)
                {
                    for (int i = 0; i < 29; i++)
                    {
                        vector.push_back(i);
                    }
                }
            }

            return vector;
        }

        // 0x004C0875
        static void audio_device_mouse_down(window* w)
        {
            auto tracks = get_available_tracks();

            widget_t dropdown = w->widgets[widx::audio_device];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], (int8_t)tracks.size(), 0x80);

            int index = -1;
            for (auto track : tracks)
            {
                index++;
                dropdown::add(index, string_ids::dropdown_stringid, music_ids_to_string_id[track]);
                if (track == _currentSong)
                {
                    dropdown::set_selection(index);
                }
            }
        }

        // 0x004C09F8
        static void audio_device_dropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto tracks = get_available_tracks();
            int track = tracks.at(ax);
            if (track == _currentSong)
                return;

            call(0x0048AAE8);

            _currentSong = track;
            _50D435 = track;
            _50D430 = 0;

            w->invalidate();
        }

        // 0x004C0A37
        static void on_update(window* w)
        {
            w->frame_no += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->current_tab + 4);
        }

        static void init_events()
        {
            _events.on_close = on_close;
            _events.on_mouse_up = on_mouse_up;
            _events.on_mouse_down = on_mouse_down;
            _events.on_dropdown = on_dropdown;
            _events.on_update = on_update;
            _events.prepare_draw = prepare_draw;
            _events.draw = draw;
        }
    }

    namespace regional
    {
        static const gfx::ui_size_t _window_size = { 366, 167 };

        namespace widx
        {
            enum
            {
                language = 10,
                language_btn,
                distance_speed,
                distance_speed_btn,
                heights,
                heights_btn,
                currency,
                currency_btn,
                preferred_currency,
                preferred_currency_btn,
                preferred_currency_for_new_games,
                preferred_currency_always
            };
        }

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_regional),
            make_widget({ 183, 49 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::stringptr),
            make_widget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 69 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::arg12_stringid),
            make_widget({ 344, 70 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 84 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::arg6_stringid),
            make_widget({ 344, 85 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            make_widget({ 183, 104 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::arg10_stringid, string_ids::current_game_currency_tip),
            make_widget({ 344, 105 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::current_game_currency_tip),
            make_widget({ 183, 119 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::preferred_currency_buffer, string_ids::new_game_currency_tip),
            make_widget({ 344, 120 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::new_game_currency_tip),
            make_widget({ 10, 134 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::use_preferred_currency_new_game, string_ids::use_preferred_currency_new_game_tip),
            make_widget({ 10, 148 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::use_preferred_currency_always, string_ids::use_preferred_currency_always_tip),
            widget_end(),
        };

        static void language_mouse_down(window* w);
        static void language_dropdown(window* w, int16_t ax);
        static void currency_mouse_down(window* w);
        static void currency_dropdown(window* w, int16_t ax);
        static void preferred_currency_mouse_down(window* w);
        static void preferred_currency_dropdown(window* w, int16_t ax);
        static void preferred_currency_new_game_mouse_up(window* w);
        static void preferred_currency_always_mouse_up(window* w);
        static void distance_speed_mouse_down(window* w);
        static void distance_speed_dropdown(window* w, int16_t ax);
        static void heights_labels_mouse_down(window* w);
        static void heights_labels_dropdown(window* w, int16_t ax);

        static window_event_list _events;

        // 0x004C0A59
        static void prepare_draw(window* w)
        {
            assert(w->current_tab == common::tab::regional);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            auto& language = localisation::getDescriptorForLanguage(config::get_new().language);
            set_format_arg(0, const char*, language.native_name.c_str());

            string_id current_height_units = string_ids::height_units;
            if ((openloco::config::get().flags & config::flags::show_height_as_units) == 0)
            {
                current_height_units = string_ids::height_real_values;
            }

            set_format_arg(0x6, string_id, current_height_units);

            string_id current_measurement_format = string_ids::imperial;
            if (openloco::config::get().measurement_format)
            {
                current_measurement_format = string_ids::metric;
            }

            set_format_arg(0xC, string_id, current_measurement_format);
            set_format_arg(0xA, string_id, objectmgr::get<currency_object>()->name);

            w->activated_widgets &= ~(1 << widx::preferred_currency_for_new_games);
            if (config::get().flags & config::flags::preferred_currency_for_new_games)
            {
                w->activated_widgets |= (1 << widx::preferred_currency_for_new_games);
            }

            w->activated_widgets &= ~(1 << widx::preferred_currency_always);
            if (config::get().flags & config::flags::preferred_currency_always)
            {
                w->activated_widgets |= (1 << widx::preferred_currency_always);
            }

            w->disabled_widgets &= ~(1 << widx::currency);
            w->disabled_widgets &= ~(1 << widx::currency_btn);
            if (config::get().flags & config::flags::preferred_currency_always)
            {
                w->disabled_widgets |= (1 << widx::currency);
                w->disabled_widgets |= (1 << widx::currency_btn);
            }

            sub_4C13BE(w);
        }

        // 0x004C0B5B
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);
            common::draw_tabs(w, dpi);

            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::distance_speed].top + 1, 0, string_ids::distance_and_speed, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::heights].top + 1, 0, string_ids::heights, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::currency].top + 1, 0, string_ids::current_game_currency, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::preferred_currency].top + 1, 0, string_ids::new_game_currency, nullptr);
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
                    options::tab_on_mouse_up(w, wi);
                    return;

                case widx::preferred_currency_for_new_games:
                    preferred_currency_new_game_mouse_up(w);
                    return;

                case widx::preferred_currency_always:
                    preferred_currency_always_mouse_up(w);
                    return;
            }
        }

        // 0x004BFBB7
        static void on_mouse_down(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::language_btn:
                    language_mouse_down(w);
                    break;
                case widx::heights_btn:
                    heights_labels_mouse_down(w);
                    break;
                case widx::distance_speed_btn:
                    distance_speed_mouse_down(w);
                    break;
                case widx::currency_btn:
                    currency_mouse_down(w);
                    break;
                case widx::preferred_currency_btn:
                    preferred_currency_mouse_down(w);
                    break;
            }
        }

        // 0x004C0C4A
        static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::language_btn:
                    language_dropdown(window, itemIndex);
                    break;

                case widx::heights_btn:
                    heights_labels_dropdown(window, itemIndex);
                    break;

                case widx::distance_speed_btn:
                    distance_speed_dropdown(window, itemIndex);
                    break;

                case widx::currency_btn:
                    currency_dropdown(window, itemIndex);
                    break;

                case widx::preferred_currency_btn:
                    preferred_currency_dropdown(window, itemIndex);
                    break;
            }
        }

        static loco_global<std::byte*, 0x0050D13C> _installedObjectList;

        static void language_mouse_down(window* w)
        {
            widget_t dropdown = w->widgets[widx::language];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], localisation::languages::count - 1, 0x80);

            std::string& current_language = config::get_new().language;

            for (int index = 1; index < localisation::languages::count; index++)
            {
                auto& ld = localisation::language_descriptors[index];
                dropdown::add(index - 1, string_ids::dropdown_stringptr, (char*)ld.native_name.c_str());

                if (ld.locale == current_language)
                    dropdown::set_selection(index - 1);
            }
        }

        static void language_dropdown(window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            auto& ld = localisation::language_descriptors[ax + 1];
            config::get_new().language = ld.locale;
            config::write();
            localisation::loadLanguageFile();
            gfx::invalidate_screen();
        }

        // 0x004C0C73
        static void currency_mouse_down(window* w)
        {
            uint8_t* _11364A0 = (uint8_t*)*__11364A0;

            widget_t dropdown = w->widgets[widx::currency];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], _112C185, 0x80);
            int index = -1;
            for (auto object : objectmgr::getAvailableObjects(object_type::currency))
            {
                index++;
                dropdown::add(index, string_ids::dropdown_stringptr, object.second._name);

                if (_11364A0[object.first] & 1)
                {
                    dropdown::set_selection(index);
                }
            }
        }

        // 0x004C0D33
        static void currency_dropdown(window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            uint8_t* _11364A0 = (uint8_t*)*__11364A0;

            int index = -1;
            for (auto object : objectmgr::getAvailableObjects(object_type::currency))
            {
                index++;
                if (index == ax)
                {
                    registers regs;
                    regs.dl = (uint8_t)object_type::currency;
                    regs.edi = (uintptr_t)_11364A0;
                    call(0x00472AFE, regs); // get active object?

                    if (regs.ebp != -1)
                    {
                        registers regs2;
                        regs2.ebp = regs.ebp;
                        call(0x00471FF8, regs2); // unload object
                    }

                    registers regs3;
                    regs3.ebp = (uintptr_t)object.second._header;

                    call(0x00471BCE, regs3);
                    call(0x0047237D); // reset_loaded_objects
                    call(0x0046E07B); // load currency gfx
                    sub_4BF935();

                    break;
                }
            }

            w->invalidate();
        }

        // 0x004C0DCF
        static void preferred_currency_mouse_down(window* w)
        {
            widget_t dropdown = w->widgets[widx::preferred_currency];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], _112C185, 0x80);

            int index = -1;
            for (auto object : objectmgr::getAvailableObjects(object_type::currency))
            {
                index++;
                dropdown::add(index, string_ids::dropdown_stringptr, object.second._name);

                // TODO: Mark current value as selected
            }
        }

        // 0x004C0E82
        static void preferred_currency_dropdown(window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            int index = -1;
            for (auto object : objectmgr::getAvailableObjects(object_type::currency))
            {
                index++;

                if (index == ax)
                {
                    auto& cfg = openloco::config::get();
                    memcpy(cfg.preferred_currency, object.second._header, 0x10);

                    sub_4C1519();
                    config::write();
                    call(0x004C153B);
                    sub_4BF935();

                    break;
                }
            }

            w->invalidate();
        }

        // 0x004C0F14
        static void preferred_currency_new_game_mouse_up(window* w)
        {
            auto& cfg = openloco::config::get();
            if (cfg.flags & config::flags::preferred_currency_for_new_games)
            {
                cfg.flags &= ~config::flags::preferred_currency_for_new_games;
            }
            else
            {
                cfg.flags |= config::flags::preferred_currency_for_new_games;
            }
            config::write();

            w->invalidate();
        }

        // 0x004C0F27
        static void preferred_currency_always_mouse_up(window* w)
        {
            auto& cfg = openloco::config::get();
            if (cfg.flags & config::flags::preferred_currency_always)
            {
                cfg.flags &= ~config::flags::preferred_currency_always;
            }
            else
            {
                cfg.flags |= config::flags::preferred_currency_always;
            }
            config::write();

            call(0x004C153B);
            sub_4BF935();

            w->invalidate();
        }

        // 0x004C0F49
        static void distance_speed_mouse_down(window* w)
        {
            widget_t dropdown = w->widgets[widx::distance_speed];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::imperial);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::metric);
            dropdown::set_selection(config::get().measurement_format);
        }

        // 0x004C0FB3
        static void distance_speed_dropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = config::get();
            cfg.measurement_format = ax;

            // 0x004C0FC2
            cfg.height_marker_offset = 0;
            if ((cfg.flags & config::flags::show_height_as_units) == 0)
            {
                cfg.height_marker_offset = 0x100 + cfg.measurement_format * 0x100;
            }

            config::write();
            gfx::invalidate_screen();
        }

        // 0x004C0FFA
        static void heights_labels_mouse_down(window* w)
        {
            widget_t dropdown = w->widgets[widx::heights];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::height_units);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::height_real_values);

            int selectedItem = 0;
            if ((config::get().flags & config::flags::show_height_as_units) == 0)
            {
                selectedItem = 1;
            }
            dropdown::set_selection(selectedItem);
        }

        // 0x004C106C
        static void heights_labels_dropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = config::get();
            cfg.flags &= ~config::flags::show_height_as_units;

            if (ax == 0)
            {
                cfg.flags |= config::flags::show_height_as_units;
            }

            // 0x004C0FC2
            cfg.height_marker_offset = 0;
            if ((cfg.flags & config::flags::show_height_as_units) == 0)
            {
                cfg.height_marker_offset = 0x100 + cfg.measurement_format * 0x100;
            }

            config::write();
            gfx::invalidate_screen();
        }

        // 0x004C1195
        static void on_update(window* w)
        {
            w->frame_no += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->current_tab + 4);
        }

        static void init_events()
        {
            _events.on_close = on_close;
            _events.on_mouse_up = on_mouse_up;
            _events.on_mouse_down = on_mouse_down;
            _events.on_dropdown = on_dropdown;
            _events.on_update = on_update;
            _events.prepare_draw = prepare_draw;
            _events.draw = draw;
        }
    }

    namespace controls
    {
        namespace widx
        {
            enum
            {
                edge_scrolling = 10,
                customize_keys
            };
        }

        static const gfx::ui_size_t _window_size = { 366, 87 };

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_controls),
            make_widget({ 10, 50 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::scroll_screen_edge, string_ids::scroll_screen_edge_tip),
            make_widget({ 26, 64 }, { 160, 12 }, widget_type::wt_11, 1, string_ids::customise_keys, string_ids::customise_keys_tip),
            widget_end(),
        };

        static void edge_scrolling_mouse_up(window* w);
        static void open_keyboard_shortcuts();

        static window_event_list _events;

        static void prepare_draw(window* w)
        {
            assert(w->current_tab == common::tab::controls);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            w->activated_widgets &= ~(1 << widx::edge_scrolling);
            if (config::get().edge_scrolling)
            {
                w->activated_widgets |= (1 << widx::edge_scrolling);
            }

            sub_4C13BE(w);
        }

        // 0x004C113F
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            w->draw(dpi);
            common::draw_tabs(w, dpi);
        }

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
                    options::tab_on_mouse_up(w, wi);
                    return;

                case widx::customize_keys:
                    open_keyboard_shortcuts();
                    break;

                case widx::edge_scrolling:
                    edge_scrolling_mouse_up(w);
                    break;
            }
        }

        // 0x004C117A
        static void edge_scrolling_mouse_up(window* w)
        {
            auto& cfg = openloco::config::get();
            cfg.edge_scrolling = !cfg.edge_scrolling;
            config::write();

            w->invalidate();
        }

        // 0x004C118D
        static void open_keyboard_shortcuts()
        {
            call(0x004BE6C7);
        }

        // 0x004C1195
        static void on_update(window* w)
        {
            w->frame_no += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->current_tab + 4);
        }

        static void init_events()
        {
            _events.on_close = on_close;
            _events.on_mouse_up = on_mouse_up;
            _events.on_update = on_update;
            _events.prepare_draw = prepare_draw;
            _events.draw = draw;
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
            make_widget({ 10, 49 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::use_preferred_owner_name, string_ids::use_preferred_owner_name_tip),
            make_widget({ 335, 64 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::change),
            make_widget({ 10, 79 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::export_plugin_objects, string_ids::export_plugin_objects_tip),
            widget_end(),
        };

        static window_event_list _events;

        static loco_global<uint8_t, 0x0112A17E> _112A17E;

        static void change_preferred_name(window* w);
        static void set_preferred_name(window* w, char* str);
        static void use_preferred_owner_name_mouse_up(window* w);
        static void export_plugin_objects_mouse_up(window* w);

        // 0x004C11B7
        static void prepare_draw(window* w)
        {
            assert(w->current_tab == common::tab::miscellaneous);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            w->activated_widgets &= ~(1 << widx::export_plugin_objects);
            if (config::get().flags & config::flags::export_objects_with_saves)
            {
                w->activated_widgets |= (1 << widx::export_plugin_objects);
            }

            w->activated_widgets &= ~(1 << widx::use_preferred_owner_name);
            w->disabled_widgets |= (1 << widx::change_btn);
            if (config::get().flags & config::flags::use_preferred_owner_name)
            {
                w->activated_widgets |= (1 << widx::use_preferred_owner_name);
                w->disabled_widgets &= ~(1 << widx::change_btn);
            }

            w->widgets[widx::export_plugin_objects].type = widget_type::none;
            if (_112A17E)
            {
                w->widgets[widx::export_plugin_objects].type = widget_type::checkbox;
            }

            sub_4C13BE(w);
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

            set_format_arg(0, string_id, string_ids::buffer_2039);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::change_btn].top + 1, 0, string_ids::wcolour2_preferred_owner_name, _commonFormatArgs);
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
                    options::tab_on_mouse_up(w, wi);
                    return;

                case widx::export_plugin_objects:
                    export_plugin_objects_mouse_up(w);
                    break;

                case widx::use_preferred_owner_name:
                    use_preferred_owner_name_mouse_up(w);
                    break;

                case widx::change_btn:
                    change_preferred_name(w);
                    break;
            }
        }

        // 0x004C1304
        static void text_input(window* w, widget_index i, char* str)
        {
            switch (i)
            {
                case widx::change_btn:
                    set_preferred_name(w, str);
                    break;
            }
        }

        // 0x004C1319
        static void change_preferred_name(window* w)
        {
            auto buffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
            char* playerName = config::get().preferred_name;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            textinput::open_textinput(w, string_ids::preferred_owner_name, string_ids::enter_preferred_owner_name, string_ids::buffer_2039, 11, nullptr);
        }

        // 0x004C1342
        static void set_preferred_name(window* w, char* str)
        {
            if (strlen(str) == 0)
                return;

            auto& cfg = openloco::config::get();
            strcpy(cfg.preferred_name, str);
            cfg.preferred_name[strlen(str)] = '\0';

            config::write();

            w->invalidate();
        }

        // 0x004C135F
        static void use_preferred_owner_name_mouse_up(window* w)
        {
            auto& cfg = openloco::config::get();
            if (cfg.flags & config::flags::use_preferred_owner_name)
            {
                cfg.flags &= ~config::flags::use_preferred_owner_name;
            }
            else
            {
                cfg.flags |= config::flags::use_preferred_owner_name;
            }
            config::write();

            w->invalidate();

            if (cfg.flags & config::flags::use_preferred_owner_name)
            {
                if (strlen(cfg.preferred_name) == 0)
                {
                    change_preferred_name(w);
                }
            }
        }

        static void export_plugin_objects_mouse_up(window* w)
        {
            auto& cfg = openloco::config::get();
            if (cfg.flags & config::flags::export_objects_with_saves)
            {
                cfg.flags &= ~config::flags::export_objects_with_saves;
            }
            else
            {
                cfg.flags |= config::flags::export_objects_with_saves;
            }
            config::write();

            w->invalidate();
        }

        // 0x004C139C
        static void on_update(window* w)
        {
            w->frame_no += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->current_tab + 4);
        }

        static void init_events()
        {
            _events.on_close = on_close;
            _events.on_mouse_up = on_mouse_up;
            _events.on_update = on_update;
            _events.text_input = text_input;
            _events.prepare_draw = prepare_draw;
            _events.draw = draw;
        }
    }

    static void sub_4BF8CD()
    {
        auto ptr = malloc(objectmgr::getNumInstalledObjects());
        // TODO: reimplement nullptr check?

        __11364A0 = ptr;
        sub_4BF935();
    }

    static void sub_4BF935()
    {
        // TODO: implement
        call(0x004BF935);
    }

    static void sub_4C13BE(window* w)
    {
        w->disabled_widgets &= ~((1 << common::widx::tab_music) | (1 << common::widx::tab_regional));
        if (is_editor_mode() || is_title_mode())
        {
            w->disabled_widgets |= 1 << common::widx::tab_music;
        }

        if (is_editor_mode() && _9C8714 == 0)
        {
            w->disabled_widgets |= 1 << common::widx::tab_regional;
        }

        int x = w->widgets[common::widx::tab_display].left;
        for (int i = common::widx::tab_display; i <= common::widx::tab_miscellaneous; i++)
        {
            if (!w->is_disabled(i))
            {
                w->widgets[i].left = x;
                w->widgets[i].right = x + 30;
                x += 31;
            }
        }
    }

    static void sub_4C1519()
    {
        call(0x004C1519);
    }

    // 0x004BF7B9
    window* open()
    {
        window* window;

        window = windowmgr::bring_to_front(window_type::options, 0);
        if (window != nullptr)
            return window;

        // TODO: only needs to be called once
        display::init_events();
        sound::init_events();
        music::init_events();
        regional::init_events();
        controls::init_events();
        misc::init_events();

        // 0x004BF833 (create_options_window)
        window = windowmgr::create_window_centred(
            window_type::options,
            display::_window_size.width,
            display::_window_size.height,
            0,
            &display::_events);

        window->widgets = display::_widgets;
        window->number = 0;
        window->current_tab = 0;
        window->frame_no = 0;
        window->var_840 = 0xFFFF;

        auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;

        sub_4BF8CD();
        sub_4C1519();

        // Returning to 0x004BF7CB (in windowmgr__open_options)
        window->disabled_widgets = (1 << display::widx::display_resolution) | (1 << display::widx::display_resolution_btn);
        window->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << display::widx::landscape_smoothing) | (1 << display::widx::gridlines_on_landscape) | (1 << display::widx::vehicles_min_scale) | (1 << display::widx::vehicles_min_scale_btn) | (1 << display::widx::station_names_min_scale) | (1 << display::widx::station_names_min_scale_btn) | (1 << display::widx::construction_marker) | (1 << display::widx::construction_marker_btn);
        window->holdable_widgets = 0;
        window->event_handlers = &display::_events;
        window->activated_widgets = 0;

        window->call_on_resize();
        window->call_prepare_draw();
        window->init_scroll_widgets();

        return window;
    }

    // 0x004BF823
    window* open_music_settings()
    {
        auto window = open();

        window->call_on_mouse_up(common::widx::tab_music);

        return window;
    }

    // 0x004BFC11
    static void tab_on_mouse_up(window* w, widget_index wi)
    {
        input::cancel_tool(w->type, w->number);

        textinput::sub_4CE6C9(w->type, w->number);
        w->current_tab = wi - common::widx::tab_display;
        w->frame_no = 0;
        w->flags &= ~(window_flags::flag_16);
        w->disabled_widgets = 0;
        w->holdable_widgets = 0;
        w->activated_widgets = 0;
        if (w->viewports[0])
        {
            w->viewports[0]->width = 0;
            w->viewports[0] = nullptr;
        }

        switch ((common::tab)w->current_tab)
        {
            case common::tab::display:
                w->disabled_widgets = (1 << display::widx::display_resolution) | (1 << display::widx::display_resolution_btn);
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << display::widx::landscape_smoothing) | (1 << display::widx::gridlines_on_landscape) | (1 << display::widx::vehicles_min_scale) | (1 << display::widx::vehicles_min_scale_btn) | (1 << display::widx::station_names_min_scale) | (1 << display::widx::station_names_min_scale_btn) | (1 << display::widx::construction_marker) | (1 << display::widx::construction_marker_btn);
                w->event_handlers = &display::_events;
                w->widgets = display::_widgets;
                w->invalidate();
                w->set_size(display::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::sound:
                w->disabled_widgets = 0;
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << sound::widx::audio_device) | (1 << sound::widx::audio_device_btn) | (1 << sound::widx::sound_quality) | (1 << sound::widx::sound_quality_btn) | (1 << sound::widx::force_software_audio_mixer);
#ifdef _WIN32
                w->disabled_widgets = 0;
#else
                w->disabled_widgets = (1 << music::widx::audio_device) | (1 << music::widx::audio_device_btn);
                w->enabled_widgets ^= (1 << music::widx::audio_device) | (1 << music::widx::audio_device_btn);
#endif
                w->event_handlers = &sound::_events;
                w->widgets = sound::_widgets;
                w->invalidate();
                w->set_size(sound::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::music:
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << music::widx::music_controls_stop) | (1 << music::widx::music_controls_play) | (1 << music::widx::music_controls_next) | (1 << music::widx::volume) | (1 << music::widx::music_playlist) | (1 << music::widx::music_playlist_btn) | (1 << music::widx::edit_selection);
                w->holdable_widgets = (1 << music::widx::volume);
                w->event_handlers = &music::_events;
                w->widgets = music::_widgets;
                w->invalidate();
                w->set_size(music::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::regional:
                w->disabled_widgets = 0;
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << regional::widx::language) | (1 << regional::widx::language_btn) | (1 << regional::widx::distance_speed) | (1 << regional::widx::distance_speed_btn) | (1 << regional::widx::heights) | (1 << regional::widx::heights_btn) | (1 << regional::widx::currency) | (1 << regional::widx::currency_btn) | (1 << regional::widx::preferred_currency) | (1 << regional::widx::preferred_currency_btn) | (1 << regional::widx::preferred_currency_for_new_games) | (1 << regional::widx::preferred_currency_always);
                w->holdable_widgets = 0;
                w->event_handlers = &regional::_events;
                w->widgets = regional::_widgets;
                w->invalidate();
                w->set_size(regional::_window_size);
                break;

            case common::tab::controls:
                w->disabled_widgets = 0;
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << controls::widx::edge_scrolling) | (1 << controls::widx::customize_keys);
                w->event_handlers = &controls::_events;
                w->widgets = controls::_widgets;
                w->invalidate();
                w->set_size(controls::_window_size);
                break;

            case common::tab::miscellaneous:
                w->disabled_widgets = 0;
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << misc::widx::use_preferred_owner_name) | (1 << misc::widx::change_btn) | (1 << misc::widx::export_plugin_objects);
                w->event_handlers = &misc::_events;
                w->widgets = misc::_widgets;
                w->invalidate();
                w->set_size(misc::_window_size);
                break;
        }

        w->call_on_resize();
        w->call_prepare_draw();
        w->init_scroll_widgets();
        w->invalidate();
    }
}
