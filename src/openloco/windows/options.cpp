#include "../audio/audio.h"
#include "../config.h"
#include "../date.h"
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
    static loco_global<uint32_t, 0x0112A110> _installedObjectCount;
    static loco_global<uint16_t, 0x0112C185> _112C185;
    static loco_global<char[20], 0x0112C826> _commonFormatArgs;

#define set_format_arg(a, b, c) *((b*)(&_commonFormatArgs[a])) = (c)

    static void sub_4CF194(window* w, gfx::drawpixelinfo_t* ctx, int32_t imageId, widget_index index)
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
            sub_4CF194(w, ctx, image_ids::tab_display, widx::tab_display);
            sub_4CF194(w, ctx, image_ids::tab_sound, widx::tab_sound);

            uint32_t imageId = image_ids::tab_music_0;
            if (w->var_870 == tab::music)
            {
                // TODO: fix image id addition
                imageId += (w->var_872 / 4) % 16;
            }
            sub_4CF194(w, ctx, imageId, widx::tab_music);

            imageId = image_ids::tab_globe_0;
            if (w->var_870 == tab::regional)
            {
                // TODO: fix image id addition
                imageId += (w->var_872 / 2) % 32;
            }
            sub_4CF194(w, ctx, imageId, widx::tab_regional);

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

            dropdown::add(0, string_ids::str_421, string_ids::white);
            dropdown::add(1, string_ids::str_421, string_ids::translucent);
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

            dropdown::add(0, string_ids::str_421, string_ids::full_scale);
            dropdown::add(1, string_ids::str_421, string_ids::half_scale);
            dropdown::add(2, string_ids::str_421, string_ids::quarter_scale);
            dropdown::add(3, string_ids::str_421, string_ids::eighth_scale);
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

            dropdown::add(0, string_ids::str_421, string_ids::full_scale);
            dropdown::add(1, string_ids::str_421, string_ids::half_scale);
            dropdown::add(2, string_ids::str_421, string_ids::quarter_scale);
            dropdown::add(3, string_ids::str_421, string_ids::eighth_scale);
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
            for (size_t i = 0; i < resolutions.size(); i++)
            {
                dropdown::add((int16_t)i, string_ids::str_421, { string_ids::display_resolution_dropdown_format, (uint16_t)resolutions[i].width, (uint16_t)resolutions[i].height });
            }
            // !!! TODO: set selection
            // dropdown::set_selection();
            widget_t dropdown = w->widgets[widx::display_resolution];
            dropdown::show_text_2(w->x + dropdown.left, w->y + dropdown.top, dropdown.width(), dropdown.height(), w->colours[1], (int8_t)resolutions.size(), 0x80);
        }

        // 0x004C00F4
        static void resolution_dropdown(int16_t index)
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
                    resolution_dropdown(item_index);
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
            w->var_872 += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->var_870 + 4);
        }

        // 0x004BFA04
        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::display);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->var_870 + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            set_format_arg(0x10, uint16_t, config::get().resolution_width);
            set_format_arg(0x12, uint16_t, config::get().resolution_height);

            // TODO: remove string addition
            w->widgets[widx::construction_marker].text = string_ids::white + config::get().construction_marker;
            w->widgets[widx::vehicles_min_scale].text = string_ids::full_scale + config::get().vehicles_min_scale;
            w->widgets[widx::station_names_min_scale].text = string_ids::full_scale + config::get().station_names_min_scale;

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
            // TODO: Implement
            // _events.on_close = nullptr;
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
                w10 = 10,
                w10_btn,
                w12,
                w12_btn,
                force_software_audio_mixer
            };
        }

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

        static void sub_4C0371(window* w);
        static void sub_4C03A4(ui::window* window);
        static void sub_4C040A(ui::window* window, int16_t itemIndex);
        static void sub_4C043D(ui::window* window);
        static void sub_4C04CA(ui::window* window, int16_t itemIndex);

        // 0x004C0217
        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::sound);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->var_870 + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            set_format_arg(0x0, string_id, string_ids::str_479);

            // TODO: implement device selection
            /*if(_currentSoundDevice != -1 && _numSoundDevices != 0) {
                set_format_arg(0x2, uint32_t, soundDevices[_currentSoundDevice].name);
                set_format_arg(0x0, string_id, string_ids::str_480);
            }*/

            // TODO: remove string addition
            set_format_arg(0xA, string_id, string_ids::str_651 + openloco::config::get().sound_quality);

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
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::w12].top + 1, 0, string_ids::str_650, nullptr);
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
                    sub_4C0371(w);
                    return;
            }
        }

        static void on_mouse_down(window* w, widget_index wi)
        {
            switch (wi)
            {
                case 11:
                    sub_4C043D(w);
                    break;

                case 13:
                    sub_4C03A4(w);
                    break;
            }
        }

        static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case 11:
                    sub_4C04CA(window, itemIndex);
                    break;

                case 13:
                    sub_4C040A(window, itemIndex);
                    break;
            }
        }

#pragma mark - Force software audio mixer (Widget 14)

        static void sub_4C0371(window* w)
        {
            audio::pause_sound();

            auto& cfg = openloco::config::get();
            cfg.force_software_audio_mixer = !cfg.force_software_audio_mixer;
            openloco::config::write();

            audio::unpause_sound();
            w->invalidate();
        }

#pragma mark - Widget 13

        static void sub_4C03A4(ui::window* w)
        {
            widget_t dropdown = w->widgets[widx::w12];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 3, 0x80);

            dropdown::add(0, string_ids::str_421, string_ids::str_651);
            dropdown::add(1, string_ids::str_421, string_ids::str_652);
            dropdown::add(2, string_ids::str_421, string_ids::str_653);
            dropdown::set_selection(config::get().sound_quality);
        }

        static void sub_4C040A(ui::window* w, int16_t itemIndex)
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

        static void sub_4C043D(ui::window* window)
        {
            // TODO: loop through audio devices
        }

        static void sub_4C04CA(ui::window* window, int16_t itemIndex)
        {
            // TODO: select audio device
        }

#pragma mark -

        // 0x004C04E0
        static void on_update(window* w)
        {
            w->var_872 += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->var_870 + 4);
        }

        static void init_events()
        {
            // TODO: Implement
            // _events.on_close = nullptr;
            _events->on_mouse_up = on_mouse_up;
            _events->on_mouse_down = on_mouse_down;
            _events->on_dropdown = on_dropdown;
            _events->on_update = on_update;
            _events->prepare_draw = prepare_draw;
            _events->draw = draw;
        }
    }

    namespace music
    {
        static const gfx::ui_size_t _window_size = { 366, 129 };

        namespace widx
        {
            enum
            {
                w10 = 10,
                w10_btn,
                music_controls_stop,
                music_controls_play,
                music_controls_next,
                volume,
                w16,
                w16_btn,
                edit_selection
            };
        }

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

        static void sub_4C072A(window* w);
        static void sub_4C0770();
        static void sub_4C0778(window* w);
        static void sub_4C07A4(window* w);
        static void sub_4C07C4(window* w);
        static void sub_4C07E4(window* w);
        static void sub_4C084A(window* w, int16_t ax);
        static void sub_4C0875(window* w);
        static void sub_4C09F8(window* w, int16_t ax);

        static loco_global<window_event_list, 0x00504028> _events;

        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::music);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->var_870 + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            string_id ax = string_ids::music_none;
            if (_currentSong != -1)
            {
                ax = string_ids::music_chuggin_along + _currentSong;
            }

            set_format_arg(0, string_id, ax);
            set_format_arg(2, string_id, string_ids::str_1539 + config::get().var_73);

            w->activated_widgets &= ~((1 << widx::music_controls_stop) | (1 << widx::music_controls_play));
            w->activated_widgets |= (1 << widx::music_controls_stop);
            if (_currentSong != -1)
            {
                if (config::get().var_23)
                {
                    w->activated_widgets &= ~((1 << widx::music_controls_stop) | (1 << widx::music_controls_play));
                    w->activated_widgets |= (1 << widx::music_controls_play);
                }
            }

            w->disabled_widgets |= (1 << widx::edit_selection);
            if (config::get().var_73 == 2)
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

            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::w10_btn].top, 0, string_ids::str_1535, nullptr);

            gfx::draw_string_494B3F(*dpi, w->x + 183, w->y + w->widgets[widx::volume].top + 7, 0, string_ids::str_1547, nullptr);

            gfx::draw_image(dpi, w->x + w->widgets[widx::volume].left, w->y + w->widgets[widx::volume].top, 0x20000000 | (w->colours[1] << 19) | image_ids::spr_3543);

            int16_t x = 90 + (config::get().volume / 32);
            gfx::draw_image(dpi, w->x + w->widgets[widx::volume].left + x, w->y + w->widgets[widx::volume].top, 0x20000000 | (w->colours[1] << 19) | image_ids::spr_3544);
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
                    sub_4C0778(w);
                    return;

                case widx::music_controls_play:
                    sub_4C07A4(w);
                    return;

                case widx::music_controls_next:
                    sub_4C07C4(w);
                    return;

                case widx::edit_selection:
                    sub_4C0770();
                    return;
            }
        }

        // 0x004C06F2
        static void on_mouse_down(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::w16_btn:
                    sub_4C07E4(w);
                    break;
                case widx::w10_btn:
                    sub_4C0875(w);
                    break;
                case widx::volume:
                    sub_4C072A(w);
                    break;
            }
        }

        // 0x004C070D
        static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::w16_btn:
                    sub_4C084A(window, itemIndex);
                    break;
                case widx::w10_btn:
                    sub_4C09F8(window, itemIndex);
                    break;
            }
        }

        static loco_global<int16_t, 0x005233A4> _5233A4;
        static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;

        static void sub_4C072A(window* w)
        {
            _clickRepeatTicks = 31;

            int x = _5233A4 - w->x - w->widgets[widx::volume].left - 10;
            x = std::clamp(x, 0, 80);

            registers regs;
            regs.ecx = (x * 32) - 2560;
            call(0x0048AA67, regs);

            w->invalidate();
        }

        static void sub_4C0770()
        {
            call(0x004C1602); // Open music selection
        }

        static void sub_4C0778(window* w)
        {
            if (config::get().var_23 == 0)
                return;

            auto& cfg = config::get();
            cfg.var_23 = 0;
            config::write();

            call(0x0048AAE8);

            _currentSong = -1;

            w->invalidate();
        }

        static void sub_4C07A4(window* w)
        {
            if (config::get().var_23 != 0)
                return;

            auto& cfg = config::get();
            cfg.var_23 = 1;
            config::write();

            w->invalidate();
        }

        static void sub_4C07C4(window* w)
        {
            if (config::get().var_23 == 0)
                return;

            call(0x0048AAE8);

            _currentSong = -1;

            w->invalidate();
        }

#pragma mark - Widget 17

        static void sub_4C07E4(window* w)
        {
            widget_t dropdown = w->widgets[widx::w16];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 3, 0x80);

            for (int i = 0; i < 3; i++)
            {
                dropdown::add(i, string_ids::str_421, string_ids::str_1539 + i);
            }

            dropdown::set_selection(config::get().var_73);
        }

        static void sub_4C084A(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = openloco::config::get();
            cfg.var_73 = ax;
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

            if (config::get().var_73 == 0)
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
            else if (config::get().var_73 == 1)
            {
                for (int i = 0; i < 29; i++)
                {
                    vector.push_back(i);
                }
            }
            else if (config::get().var_73 == 2)
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

        static void sub_4C0875(window* w)
        {
            auto tracks = get_available_tracks();

            widget_t dropdown = w->widgets[widx::w10];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], (int8_t)tracks.size(), 0x80);

            int index = -1;
            for (auto track : tracks)
            {
                index++;
                dropdown::add(index, string_ids::str_421, string_ids::music_chuggin_along + track);
                if (track == _currentSong)
                {
                    dropdown::set_selection(index);
                }
            }
        }

        static void sub_4C09F8(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto tracks = get_available_tracks();
            int track = tracks.at(ax);
            if (track == _currentSong)
                return;

            call(0x0048AAE8);

            _currentSong = ax;
            _50D435 = ax;
            _50D430 = 0;

            w->invalidate();
        }

        // 0x004C0A37
        static void on_update(window* w)
        {
            w->var_872 += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->var_870 + 4);
        }

        static void init_events()
        {
            _events->on_mouse_up = on_mouse_up;
            _events->on_mouse_down = on_mouse_down;
            _events->on_dropdown = on_dropdown;
            _events->on_update = on_update;
            _events->prepare_draw = prepare_draw;
            _events->draw = draw;
        }
    }

    namespace regional
    {
        static const gfx::ui_size_t _window_size = { 366, 147 };

        namespace widx
        {
            enum
            {
                distance_speed = 10,
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

        static void sub_4C0C73(window* w);
        static void sub_4C0D33(window* w, int16_t ax);
        static void sub_4C0DCF(window* w);
        static void sub_4C0E82(window* w, int16_t ax);
        static void preferred_currency_new_game_mouse_up(window* w);
        static void preferred_currency_always_mouse_up(window* w);
        static void sub_4C0F49(window* w);
        static void sub_4C0FB3(window* w, int16_t ax);
        static void sub_4C0FFA(window* w);
        static void sub_4C106C(window* w, int16_t ax);

        static loco_global<window_event_list, 0x0050409C> _events;

        // 0x004C0A59
        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::regional);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->var_870 + 4);

            w->widgets[common::widx::frame].right = w->width - 1;
            w->widgets[common::widx::frame].bottom = w->height - 1;
            w->widgets[common::widx::panel].right = w->width - 1;
            w->widgets[common::widx::panel].bottom = w->height - 1;
            w->widgets[common::widx::caption].right = w->width - 2;
            w->widgets[common::widx::close_button].left = w->width - 15;
            w->widgets[common::widx::close_button].right = w->width - 15 + 12;

            string_id ax = string_ids::str_660;
            if ((openloco::config::get().flags & config::flags::show_height_as_units) == 0)
            {
                ax = string_ids::str_661;
            }

            set_format_arg(0x6, string_id, ax);
            set_format_arg(0xC, string_id, string_ids::str_658 + openloco::config::get().measurement_format);
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

            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::distance_speed].top + 1, 0, string_ids::str_656, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::heights].top + 1, 0, string_ids::str_657, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::currency].top + 1, 0, string_ids::str_1504, nullptr);
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::preferred_currency].top + 1, 0, string_ids::str_1505, nullptr);
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
                case widx::heights_btn:
                    sub_4C0FFA(w);
                    break;
                case widx::distance_speed_btn:
                    sub_4C0F49(w);
                    break;
                case widx::currency_btn:
                    sub_4C0C73(w);
                    break;
                case widx::preferred_currency_btn:
                    sub_4C0DCF(w);
                    break;
            }
        }

        // 0x004C0C4A
        static void on_dropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::heights_btn:
                    sub_4C106C(window, itemIndex);
                    break;

                case widx::distance_speed_btn:
                    sub_4C0FB3(window, itemIndex);
                    break;

                case widx::currency_btn:
                    sub_4C0D33(window, itemIndex);
                    break;

                case widx::preferred_currency_btn:
                    sub_4C0E82(window, itemIndex);
                    break;
            }
        }

        struct header
        {
            uint8_t type;
            uint8_t pad_01[3];
            uint8_t var_04[8];
            uint32_t checksum;

            constexpr uint8_t get_type()
            {
                return type & 0x3F;
            }
        };

        struct header2
        {
            uint8_t pad_00[0x04 - 0x00];
        };

        struct header3
        {
            uint32_t var_00;      // image count?
            uint8_t pad_04[0x08]; // competitor stats?
        };

        static loco_global<std::byte*, 0x0050D13C> _installedObjectList;

        /*
        static void printHeader(header data)
        {
            printf("(%02X | %02X << 6) ", data.type & 0x3F, data.type >> 6);
            printf("%02X ", data.pad_01[0]);
            printf("%02X ", data.pad_01[1]);
            printf("%02X ", data.pad_01[2]);

            char name[8 + 1] = { 0 };
            memcpy(name, data.var_04, 8);
            printf("'%s', ", name);

            printf("%08X ", data.checksum);
        }
        */

        struct object_index_entry
        {
            header* _header;
            char* _filename;
            char* _name;

            static object_index_entry read(std::byte** ptr)
            {
                object_index_entry entry = { 0 };

                entry._header = (header*)*ptr;
                *ptr += sizeof(header);

                entry._filename = (char*)*ptr;
                *ptr += strlen(entry._filename) + 1;

                // decoded_chunk_size
                //header2* h2 = (header2*)ptr;
                *ptr += sizeof(header2);

                entry._name = (char*)*ptr;
                *ptr += strlen(entry._name) + 1;

                //header3* h3 = (header3*)ptr;
                *ptr += sizeof(header3);

                uint8_t* countA = (uint8_t*)*ptr;
                *ptr += sizeof(uint8_t);
                for (int n = 0; n < *countA; n++)
                {
                    //header* subh = (header*)ptr;
                    *ptr += sizeof(header);
                }

                uint8_t* countB = (uint8_t*)*ptr;
                *ptr += sizeof(uint8_t);
                for (int n = 0; n < *countB; n++)
                {
                    //header* subh = (header*)ptr;
                    *ptr += sizeof(header);
                }

                return entry;
            }
        };

        static void sub_4C0C73(window* w)
        {
            auto ptr = (std::byte*)_installedObjectList;

            uint8_t* _11364A0 = (uint8_t*)*__11364A0;

            widget_t dropdown = w->widgets[widx::currency];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], _112C185, 0x80);
            int index = -1;
            for (uint8_t i = 0; i < _installedObjectCount; i++)
            {
                auto entry = object_index_entry::read(&ptr);
                if (entry._header->get_type() == 2)
                {
                    index++;
                    dropdown::add(index, string_ids::str_424, entry._name);

                    if (_11364A0[i] & 1)
                    {
                        dropdown::set_selection(index);
                    }
                }
            }
        }

        static void sub_4C0D33(window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            auto ptr = (std::byte*)_installedObjectList;
            uint8_t* _11364A0 = (uint8_t*)*__11364A0;

            int index = -1;
            for (uint8_t i = 0; i < _installedObjectCount; i++)
            {
                auto entry = object_index_entry::read(&ptr);
                if (entry._header->get_type() != 2)
                    continue;

                index++;

                if (index == ax)
                {
                    registers regs;
                    regs.dl = 2; // object type
                    regs.edi = (uintptr_t)_11364A0;
                    call(0x00472AFE, regs); // get active object?

                    if (regs.ebp != -1)
                    {
                        registers regs2;
                        regs2.ebp = regs.ebp;
                        call(0x00471FF8, regs2); // unload object
                    }

                    registers regs3;
                    regs3.ebp = (uintptr_t)entry._header;

                    call(0x00471BCE, regs3);
                    call(0x0047237D); // reset_loaded_objects
                    call(0x0046E07B); // load currency gfx
                    sub_4BF935();

                    break;
                }
            }

            w->invalidate();
        }

        static void sub_4C0DCF(window* w)
        {
            auto ptr = (std::byte*)_installedObjectList;

            widget_t dropdown = w->widgets[widx::preferred_currency];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], _112C185, 0x80);

            int index = -1;
            for (uint8_t i = 0; i < _installedObjectCount; i++)
            {
                auto entry = object_index_entry::read(&ptr);
                if (entry._header->get_type() == 2)
                {
                    index++;
                    dropdown::add(index, string_ids::str_424, entry._name);

                    // TODO: Mark current value as selected
                }
            }
        }

        static void sub_4C0E82(window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            auto ptr = (std::byte*)_installedObjectList;

            int index = -1;
            for (uint8_t i = 0; i < _installedObjectCount; i++)
            {
                auto entry = object_index_entry::read(&ptr);
                if (entry._header->get_type() != 2)
                    continue;

                index++;

                if (index == ax)
                {
                    auto& cfg = openloco::config::get();
                    memcpy(cfg.preferred_currency, entry._header, 0x10);

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

        static void sub_4C0F49(window* w)
        {
            widget_t dropdown = w->widgets[widx::distance_speed];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            dropdown::add(0, string_ids::str_421, string_ids::str_658);
            dropdown::add(1, string_ids::str_421, string_ids::str_659);
            dropdown::set_selection(config::get().measurement_format);
        }

        static void sub_4C0FB3(window* w, int16_t ax)
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

        static void sub_4C0FFA(window* w)
        {
            widget_t dropdown = w->widgets[widx::heights];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            dropdown::add(0, string_ids::str_421, string_ids::str_660);
            dropdown::add(1, string_ids::str_421, string_ids::str_661);

            int selectedItem = 0;
            if ((config::get().flags & config::flags::show_height_as_units) == 0)
            {
                selectedItem = 1;
            }
            dropdown::set_selection(selectedItem);
        }

        static void sub_4C106C(window* w, int16_t ax)
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
            w->var_872 += 1;
            w->call_prepare_draw();
            windowmgr::invalidate_widget(w->type, w->number, w->var_870 + 4);
        }

        static void init_events()
        {
            _events->on_mouse_up = on_mouse_up;
            _events->on_mouse_down = on_mouse_down;
            _events->on_dropdown = on_dropdown;
            _events->on_update = on_update;
            _events->prepare_draw = prepare_draw;
            _events->draw = draw;
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
            make_widget({ 10, 50 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::STR_1002, string_ids::STR_1003),
            make_widget({ 26, 64 }, { 160, 12 }, widget_type::wt_11, 1, string_ids::STR_0701, string_ids::STR_1004),
            widget_end(),
        };

        static void sub_4C117A(window* w);
        static void sub_4C118D();

        static loco_global<window_event_list, 0x00504110> _events;

        static void prepare_draw(window* w)
        {
            assert(w->var_870 == common::tab::controls);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->var_870 + 4);

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
                    sub_4C118D();
                    break;

                case widx::edge_scrolling:
                    sub_4C117A(w);
                    break;
            }
        }

        static void sub_4C117A(window* w)
        {
            auto& cfg = openloco::config::get();
            cfg.edge_scrolling = !cfg.edge_scrolling;
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
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->var_870 + 4);

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

            *((string_id*)(&_commonFormatArgs[0])) = string_ids::buffer_2039;
            gfx::draw_string_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::change_btn].top + 1, 0, string_ids::str_1921, _commonFormatArgs);
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
                    sub_4C1389(w);
                    break;

                case widx::use_preferred_owner_name:
                    sub_4C135F(w);
                    break;

                case widx::change_btn:
                    sub_4C1319(w);
                    break;
            }
        }

        // 0x004C1304
        static void text_input(window* w, widget_index i, char* str)
        {
            switch (i)
            {
                case widx::change_btn:
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

            auto& cfg = openloco::config::get();
            strcpy(cfg.preferred_name, str);
            cfg.preferred_name[strlen(str)] = '\0';

            config::write();

            w->invalidate();
        }

        static void sub_4C135F(window* w)
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
                    sub_4C1319(w);
                }
            }
        }

        static void sub_4C1389(window* w)
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
        auto ptr = malloc(_installedObjectCount);
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
        window->var_870 = 0;
        window->var_872 = 0;
        window->var_840 = 0xFFFF;

        auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;

        sub_4BF8CD();
        sub_4C1519();

        // Returning to 0x004BF7CB (in windowmgr__open_options)
        window->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << 10) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 14) | (1 << 15) | (1 << 16) | (1 << 17) | (1 << 18) | (1 << 19);
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
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << 10) | (1 << 11) | (1 << 12) | (1 << 13) | (1 << 14) | (1 << 15) | (1 << 16) | (1 << 17) | (1 << 18) | (1 << 19);
                w->event_handlers = &display::_events;
                w->widgets = display::_widgets;
                w->invalidate();
                //
                w->set_size(display::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::sound:
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << sound::widx::w10) | (1 << sound::widx::w10_btn) | (1 << sound::widx::w12) | (1 << sound::widx::w12_btn) | (1 << sound::widx::force_software_audio_mixer);
                w->event_handlers = &(*sound::_events);
                w->widgets = sound::_widgets;
                w->invalidate();
                //
                w->set_size(sound::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::music:
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << music::widx::w10) | (1 << music::widx::w10_btn) | (1 << music::widx::music_controls_stop) | (1 << music::widx::music_controls_play) | (1 << music::widx::music_controls_next) | (1 << music::widx::volume) | (1 << music::widx::w16) | (1 << music::widx::w16_btn) | (1 << music::widx::edit_selection);
                w->holdable_widgets = (1 << music::widx::volume);
                w->event_handlers = &(*music::_events);
                w->widgets = music::_widgets;
                w->invalidate();
                //
                w->set_size(music::_window_size);
                w->var_840 = 0xFFFF;
                break;

            case common::tab::regional:
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << regional::widx::distance_speed) | (1 << regional::widx::distance_speed_btn) | (1 << regional::widx::heights) | (1 << regional::widx::heights_btn) | (1 << regional::widx::currency) | (1 << regional::widx::currency_btn) | (1 << regional::widx::preferred_currency) | (1 << regional::widx::preferred_currency_btn) | (1 << regional::widx::preferred_currency_for_new_games) | (1 << regional::widx::preferred_currency_always);
                w->holdable_widgets = 0;
                w->event_handlers = &(*regional::_events);
                w->widgets = regional::_widgets;
                w->invalidate();
                //
                w->set_size(regional::_window_size);
                break;

            case common::tab::controls:
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << controls::widx::edge_scrolling) | (1 << controls::widx::customize_keys);
                w->event_handlers = &(*controls::_events);
                w->widgets = controls::_widgets;
                w->invalidate();
                //
                w->set_size(controls::_window_size);
                break;

            case common::tab::miscellaneous:
                w->enabled_widgets = (1 << common::widx::close_button) | common::tabWidgets | (1 << misc::widx::use_preferred_owner_name) | (1 << misc::widx::change_btn) | (1 << misc::widx::export_plugin_objects);
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
