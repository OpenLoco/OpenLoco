#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Date.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/LanguageFiles.h"
#include "../Localisation/Languages.h"
#include "../Localisation/StringIds.h"
#include "../Objects/CurrencyObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../S5/S5.h"
#include "../Ui.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"
#include <cassert>

using namespace OpenLoco::interop;

namespace OpenLoco::ui::options
{
    static void tabOnMouseUp(window* w, widget_index wi);
    static void sub_4C13BE(window* w);
    static void sub_4C1519();
    static void sub_4BF935();

    static loco_global<uint32_t, 0x0050D430> _50D430;
    static loco_global<int8_t, 0x0050D434> _currentSong;
    static loco_global<uint8_t, 0x0050D435> _50D435;

    // Should be a pointer to an array of u8's
    static loco_global<void*, 0x011364A0> __11364A0;
    static loco_global<uint16_t, 0x0112C185> _112C185;

    static void onClose(window* w)
    {
        free(__11364A0);
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

        static_assert(widx::tab_music == widx::tab_display + tab_offset_music);

        enum tab
        {
            display,
            sound,
            music,
            regional,
            controls,
            miscellaneous,
        };

        static void drawTabs(window* w, gfx::drawpixelinfo_t* ctx)
        {
            widget::draw_tab(w, ctx, image_ids::tab_display, widx::tab_display);
            widget::draw_tab(w, ctx, image_ids::tab_sound, widx::tab_sound);

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
            widget::draw_tab(w, ctx, imageId, widx::tab_music);

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
            widget::draw_tab(w, ctx, imageId, widx::tab_regional);

            widget::draw_tab(w, ctx, image_ids::tab_control, widx::tab_controls);
            widget::draw_tab(w, ctx, image_ids::tab_miscellaneous, widx::tab_miscellaneous);
        }

#define common_options_widgets(window_size, window_caption_id)                                                                                             \
    makeWidget({ 0, 0 }, window_size, widget_type::frame, 0),                                                                                              \
        makeWidget({ 1, 1 }, { (uint16_t)(window_size.width - 2), 13 }, widget_type::caption_25, 0, window_caption_id),                                    \
        makeWidget({ (int16_t)(window_size.width - 15), 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        makeWidget({ 0, 41 }, { window_size.width, 102 }, widget_type::panel, 1),                                                                          \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::display_options),                                         \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::sound_options),                                          \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::music_options),                                          \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::regional_options),                                       \
        makeRemapWidget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::control_options),                                       \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::miscellaneous_options)

        static constexpr int tabWidgets = (1 << widx::tab_display) | (1 << widx::tab_sound) | (1 << widx::tab_music) | (1 << widx::tab_regional) | (1 << widx::tab_controls) | (1 << widx::tab_miscellaneous);

        static constexpr int enabledWidgets = (1 << widx::close_button) | tabWidgets;
    }

    namespace display
    {
        static const gfx::ui_size_t _window_size = { 400, 184 };

        namespace widx
        {
            enum
            {
                screen_mode = 10,
                screen_mode_btn,
                display_resolution,
                display_resolution_btn,
                display_scale,
                display_scale_down_btn,
                display_scale_up_btn,
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
            makeWidget({ 235, 49 }, { 156, 12 }, widget_type::wt_18, 1, string_ids::empty),
            makeWidget({ 379, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            makeWidget({ 235, 64 }, { 156, 12 }, widget_type::wt_18, 1, string_ids::display_resolution_label_format),
            makeWidget({ 379, 65 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            makeStepperWidgets({ 235, 79 }, { 156, 12 }, widget_type::wt_17, 1, string_ids::empty),
            makeWidget({ 10, 99 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::landscape_smoothing, string_ids::landscape_smoothing_tip),
            makeWidget({ 10, 114 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::gridlines_on_landscape, string_ids::gridlines_on_landscape_tip),
            makeWidget({ 235, 133 }, { 156, 12 }, widget_type::wt_18, 1, string_ids::empty, string_ids::vehicles_min_scale_tip),
            makeWidget({ 379, 134 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::vehicles_min_scale_tip),
            makeWidget({ 235, 148 }, { 156, 12 }, widget_type::wt_18, 1, string_ids::empty, string_ids::station_names_min_scale_tip),
            makeWidget({ 379, 149 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::station_names_min_scale_tip),
            makeWidget({ 235, 163 }, { 156, 12 }, widget_type::wt_18, 1, string_ids::empty),
            makeWidget({ 379, 164 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            widgetEnd(),
        };

        static window_event_list _events;

        static constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << display::widx::landscape_smoothing) | (1 << display::widx::gridlines_on_landscape) | (1 << display::widx::vehicles_min_scale) | (1 << display::widx::vehicles_min_scale_btn) | (1 << display::widx::station_names_min_scale) | (1 << display::widx::station_names_min_scale_btn) | (1 << display::widx::construction_marker) | (1 << display::widx::construction_marker_btn) | (1 << display::widx::display_scale_up_btn) | (1 << display::widx::display_scale_down_btn);

        // 0x004BFB8C
        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    WindowManager::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::tabOnMouseUp(w, wi);
                    return;

                case widx::landscape_smoothing:
                {
                    auto& cfg = OpenLoco::config::get();
                    // TODO: is there a better way to toggle a flag?
                    if (cfg.flags & config::flags::landscape_smoothing)
                    {
                        cfg.flags &= ~config::flags::landscape_smoothing;
                    }
                    else
                    {
                        cfg.flags |= config::flags::landscape_smoothing;
                    }
                    OpenLoco::config::write();
                    gfx::invalidateScreen();
                    return;
                }

                case widx::gridlines_on_landscape:
                {
                    auto& cfg = OpenLoco::config::get();
                    if (cfg.flags & config::flags::gridlines_on_landscape)
                    {
                        cfg.flags &= ~config::flags::gridlines_on_landscape;
                    }
                    else
                    {
                        cfg.flags |= config::flags::gridlines_on_landscape;
                    }
                    OpenLoco::config::write();
                    gfx::invalidateScreen();

                    auto main = WindowManager::getMainWindow();
                    if (main != nullptr)
                    {
                        main->viewports[0]->flags &= ~viewport_flags::gridlines_on_landscape;

                        if ((cfg.flags & config::flags::gridlines_on_landscape) != 0)
                        {
                            main->viewports[0]->flags |= viewport_flags::gridlines_on_landscape;
                        }
                    }

                    return;
                }
            }
        }

#pragma mark - Construction Marker (Widget 19)

        // 0x004BFE2E
        static void constructionMarkerMouseDown(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[widx::construction_marker];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::white);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::translucent);
            dropdown::setItemSelected(config::get().construction_marker);
        }

        // 0x004BFE98
        static void constructionMarkerDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().construction_marker)
                return;

            auto& cfg = OpenLoco::config::get();
            cfg.construction_marker = ax;
            OpenLoco::config::write();
            gfx::invalidateScreen();
        }

#pragma mark - Vehicle zoom (Widget 15)

        // 0x004BFEBE
        static void vehicleZoomMouseDown(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[widx::vehicles_min_scale];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 4, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::full_scale);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::half_scale);
            dropdown::add(2, string_ids::dropdown_stringid, string_ids::quarter_scale);
            dropdown::add(3, string_ids::dropdown_stringid, string_ids::eighth_scale);
            dropdown::setItemSelected(config::get().vehicles_min_scale);
        }

        // 0x004BFF4C
        static void vehicleZoomDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().vehicles_min_scale)
                return;

            auto& cfg = OpenLoco::config::get();
            cfg.vehicles_min_scale = ax;
            OpenLoco::config::write();
            gfx::invalidateScreen();
        }

#pragma mark - Station names minimum scale (Widget 17)

        // 0x004BFF72
        static void stationNamesScaleMouseDown(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[widx::station_names_min_scale];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 4, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::full_scale);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::half_scale);
            dropdown::add(2, string_ids::dropdown_stringid, string_ids::quarter_scale);
            dropdown::add(3, string_ids::dropdown_stringid, string_ids::eighth_scale);
            dropdown::setItemSelected(config::get().station_names_min_scale);
        }

        // 0x004C0000
        static void stationNamesScaleDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == config::get().station_names_min_scale)
                return;

            auto& cfg = OpenLoco::config::get();
            cfg.station_names_min_scale = ax;
            OpenLoco::config::write();
            gfx::invalidateScreen();
        }

#if !(defined(__APPLE__) && defined(__MACH__))
        static void screenModeToggleEnabled(window* w)
        {
            if (config::getNew().display.mode == config::screen_mode::fullscreen)
            {
                w->enabled_widgets |= (1 << widx::display_resolution) | (1 << widx::display_resolution_btn);
                w->disabled_widgets &= ~((1 << widx::display_resolution) | (1 << widx::display_resolution_btn));
            }
            else
            {
                w->enabled_widgets &= ~((1 << widx::display_resolution) | (1 << widx::display_resolution_btn));
                w->disabled_widgets |= (1 << widx::display_resolution) | (1 << widx::display_resolution_btn);
            }
        }
#endif

        static void screenModeMouseDown(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[widx::screen_mode];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 3, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::options_mode_windowed);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::options_mode_fullscreen);
            dropdown::add(2, string_ids::dropdown_stringid, string_ids::options_mode_fullscreen_window);

            auto selection = static_cast<uint16_t>(config::getNew().display.mode);
            dropdown::setItemSelected(selection);
        }

        static void screenModeDropdown(window* w, int16_t selection)
        {
            if (selection == -1)
                return;

            auto new_mode = static_cast<config::screen_mode>(selection);
            if (new_mode == config::getNew().display.mode)
                return;

#if !(defined(__APPLE__) && defined(__MACH__))
            ui::setDisplayMode(new_mode);
#endif
        }

#pragma mark - Resolution dropdown (Widget 11)

        // 0x004C0026
        static void resolutionMouseDown(window* w, widget_index wi)
        {
            std::vector<Resolution> resolutions = getFullscreenResolutions();

            widget_t dropdown = w->widgets[widx::display_resolution];
            dropdown::showText2(w->x + dropdown.left, w->y + dropdown.top, dropdown.width(), dropdown.height(), w->colours[1], resolutions.size(), 0x80);

            auto& cfg = config::getNew();
            for (size_t i = 0; i < resolutions.size(); i++)
            {
                dropdown::add(i, string_ids::dropdown_stringid, { string_ids::display_resolution_dropdown_format, (uint16_t)resolutions[i].width, (uint16_t)resolutions[i].height });
                if (cfg.display.fullscreen_resolution.width == resolutions[i].width && cfg.display.fullscreen_resolution.height == resolutions[i].height)
                    dropdown::setItemSelected((int16_t)i);
            }
        }

        // 0x004C00F4
        static void resolutionDropdown(window* w, int16_t index)
        {
            if (index == -1)
                return;
            std::vector<Resolution> resolutions = getFullscreenResolutions();
            ui::setDisplayMode(config::screen_mode::fullscreen, { resolutions[index].width, resolutions[index].height });
        }

#pragma mark -

        static void displayScaleMouseDown(window* w, widget_index wi, float adjust_by)
        {
            OpenLoco::ui::adjustWindowScale(adjust_by);
        }

        // 0x004BFBB7
        static void onMouseDown(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::screen_mode_btn:
                    screenModeMouseDown(w, wi);
                    break;
                case widx::display_resolution_btn:
                    resolutionMouseDown(w, wi);
                    break;
                case widx::construction_marker_btn:
                    constructionMarkerMouseDown(w, wi);
                    break;
                case widx::vehicles_min_scale_btn:
                    vehicleZoomMouseDown(w, wi);
                    break;
                case widx::station_names_min_scale_btn:
                    stationNamesScaleMouseDown(w, wi);
                    break;
                case widx::display_scale_down_btn:
                    displayScaleMouseDown(w, wi, -OpenLoco::ui::ScaleFactor::step);
                    break;
                case widx::display_scale_up_btn:
                    displayScaleMouseDown(w, wi, OpenLoco::ui::ScaleFactor::step);
                    break;
            }
        }

        // 0x004BFBE8
        static void onDropdown(window* w, widget_index wi, int16_t item_index)
        {
            switch (wi)
            {
                case widx::screen_mode_btn:
                    screenModeDropdown(w, item_index);
                    break;
                case widx::display_resolution_btn:
                    resolutionDropdown(w, item_index);
                    break;
                case widx::construction_marker_btn:
                    constructionMarkerDropdown(item_index);
                    break;
                case widx::vehicles_min_scale_btn:
                    vehicleZoomDropdown(item_index);
                    break;
                case widx::station_names_min_scale_btn:
                    stationNamesScaleDropdown(item_index);
                    break;
            }
        }

        // 0x004C01F5
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->current_tab + 4);
        }

        // 0x004BFA04
        static void prepareDraw(window* w)
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

            string_id screen_mode_string_id = string_ids::empty;
            switch (config::getNew().display.mode)
            {
                case config::screen_mode::window:
                    screen_mode_string_id = string_ids::options_mode_windowed;
                    break;
                case config::screen_mode::fullscreen:
                    screen_mode_string_id = string_ids::options_mode_fullscreen;
                    break;
                case config::screen_mode::fullscreen_borderless:
                    screen_mode_string_id = string_ids::options_mode_fullscreen_window;
                    break;
            }
            w->widgets[widx::screen_mode].text = screen_mode_string_id;

            FormatArguments args = {};
            args.skip(0x10);
            auto& resolution = config::getNew().display.fullscreen_resolution;
            args.push<uint16_t>(resolution.width);
            args.push<uint16_t>(resolution.height);

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

            if (config::getNew().scale_factor <= OpenLoco::ui::ScaleFactor::min)
                w->disabled_widgets |= (1 << widx::display_scale_down_btn);
            else
                w->disabled_widgets &= ~(1 << widx::display_scale_down_btn);

            if (config::getNew().scale_factor >= OpenLoco::ui::ScaleFactor::max)
                w->disabled_widgets |= (1 << widx::display_scale_up_btn);
            else
                w->disabled_widgets &= ~(1 << widx::display_scale_up_btn);

#if !(defined(__APPLE__) && defined(__MACH__))
            screenModeToggleEnabled(w);
#endif

            sub_4C13BE(w);
        }

        // 0x004BFAF9
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            common::drawTabs(w, dpi);

            int16_t x = w->x + 10;
            int16_t y = w->y + display::_widgets[display::widx::screen_mode].top + 1;
            drawString_494B3F(*dpi, x, y, colour::black, string_ids::options_screen_mode, nullptr);

            y = w->y + display::_widgets[display::widx::display_resolution].top + 1;
            drawString_494B3F(*dpi, x + 14, y, colour::black, string_ids::display_resolution, nullptr);

            y = w->y + display::_widgets[display::widx::construction_marker].top + 1;
            drawString_494B3F(*dpi, x, y, colour::black, string_ids::construction_marker, nullptr);

            y = w->y + display::_widgets[display::widx::vehicles_min_scale].top + 1;
            drawString_494B3F(*dpi, x, y, colour::black, string_ids::vehicles_min_scale, nullptr);

            y = w->y + display::_widgets[display::widx::station_names_min_scale].top + 1;
            drawString_494B3F(*dpi, x, y, colour::black, string_ids::station_names_min_scale, nullptr);

            y = w->y + display::_widgets[display::widx::display_scale].top + 1;
            drawString_494B3F(*dpi, x + 14, y, colour::black, string_ids::window_scale_factor, nullptr);

            int scale = (int)(config::getNew().scale_factor * 100);
            auto& scale_widget = w->widgets[widx::display_scale];
            drawString_494B3F(*dpi, w->x + scale_widget.left + 1, w->y + scale_widget.top + 1, colour::black, string_ids::scale_formatted, &scale);
        }

        static void applyScreenModeRestrictions(window* w)
        {
            if (config::getNew().display.mode != config::screen_mode::fullscreen)
                w->disabled_widgets = (1 << display::widx::display_resolution) | (1 << display::widx::display_resolution_btn);

#if !(defined(__APPLE__) && defined(__MACH__))
            w->enabled_widgets |= (1 << display::widx::screen_mode) | (1 << display::widx::screen_mode_btn);
            display::screenModeToggleEnabled(w);
#else
            w->disabled_widgets |= (1 << display::widx::screen_mode) | (1 << display::widx::screen_mode_btn) | (1 << display::widx::display_resolution) | (1 << display::widx::display_resolution_btn);
#endif
        }

        static const window_event_list initEvents()
        {
            _events.on_close = onClose;
            _events.on_mouse_up = onMouseUp;
            _events.on_mouse_down = onMouseDown;
            _events.on_dropdown = onDropdown;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
            _events.draw = draw;
            return _events;
        }
    }

    namespace sound
    {
        static const gfx::ui_size_t _window_size = { 366, 84 };

        namespace widx
        {
            enum
            {
                audio_device = 10,
                audio_device_btn,
                play_title_music,
            };
        }

        static constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << sound::widx::audio_device) | (1 << sound::widx::audio_device_btn) | (1 << sound::widx::play_title_music);

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_sound),
            makeWidget({ 10, 49 }, { 346, 12 }, widget_type::wt_18, 1, string_ids::stringid),
            makeWidget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            makeWidget({ 10, 65 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::play_title_music),
            widgetEnd(),
        };

        static window_event_list _events;

        static void audioDeviceMouseDown(ui::window* window);
        static void audioDeviceDropdown(ui::window* window, int16_t itemIndex);
        static void playTitleMusicOnMouseUp(ui::window* window);

        // 0x004C0217
        static void prepareDraw(window* w)
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

            FormatArguments args = {};

            auto audioDeviceName = audio::getCurrentDeviceName();
            if (audioDeviceName != nullptr)
            {
                args.push(string_ids::stringptr);
                args.push(audioDeviceName);
            }
            else
                args.push(string_ids::audio_device_none);

            if (config::getNew().audio.play_title_music)
                w->activated_widgets |= (1 << widx::play_title_music);
            else
                w->activated_widgets &= ~(1 << widx::play_title_music);

            sub_4C13BE(w);
        }

        // 0x004C02F5
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            common::drawTabs(w, dpi);
        }

        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    WindowManager::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::tabOnMouseUp(w, wi);
                    return;

                case widx::play_title_music:
                    playTitleMusicOnMouseUp(w);
                    return;
            }
        }

        static void onMouseDown(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::audio_device_btn:
                    audioDeviceMouseDown(w);
                    break;
            }
        }

        static void onDropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::audio_device_btn:
                    audioDeviceDropdown(window, itemIndex);
                    break;
            }
        }

#pragma mark - Widget 11

        // 0x004C043D
        static void audioDeviceMouseDown(ui::window* w)
        {
            const auto& devices = audio::getDevices();
            if (devices.size() != 0)
            {
                widget_t dropdown = w->widgets[widx::audio_device];
                dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], devices.size(), 0x80);
                for (size_t i = 0; i < devices.size(); i++)
                {
                    auto name = devices[i].c_str();
                    dropdown::add(i, string_ids::dropdown_stringid, { string_ids::stringptr, name });
                }

                auto currentDevice = audio::getCurrentDevice();
                if (currentDevice != std::numeric_limits<size_t>().max())
                {
                    dropdown::setItemSelected((int16_t)currentDevice);
                }
            }
        }

        // 0x004C04CA
        static void audioDeviceDropdown(ui::window* w, int16_t itemIndex)
        {
            if (itemIndex != -1)
            {
                audio::set_device(itemIndex);
                WindowManager::invalidateWidget(w->type, w->number, widx::audio_device);
            }
        }

#pragma mark -

        static void playTitleMusicOnMouseUp(window* w)
        {
            auto& cfg = config::getNew();
            cfg.audio.play_title_music = !cfg.audio.play_title_music;
            config::write();

            audio::playTitleScreenMusic();

            w->invalidate();
        }

        // 0x004C04E0
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->current_tab + 4);
        }

        static void initEvents()
        {
            _events.on_close = onClose;
            _events.on_mouse_up = onMouseUp;
            _events.on_mouse_down = onMouseDown;
            _events.on_dropdown = onDropdown;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
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
                currently_playing = 10,
                currently_playing_btn,
                music_controls_stop,
                music_controls_play,
                music_controls_next,
                volume,
                music_playlist,
                music_playlist_btn,
                edit_selection
            };
        }

        static constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << music::widx::currently_playing) | (1 << music::widx::currently_playing_btn) | (1 << music::widx::music_controls_stop) | (1 << music::widx::music_controls_play) | (1 << music::widx::music_controls_next) | (1 << music::widx::volume) | (1 << music::widx::music_playlist) | (1 << music::widx::music_playlist_btn) | (1 << music::widx::edit_selection);

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_music),
            makeWidget({ 160, 49 }, { 196, 12 }, widget_type::wt_18, 1, string_ids::stringid),
            makeWidget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            makeWidget({ 10, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_stop, string_ids::music_controls_stop_tip),
            makeWidget({ 34, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_play, string_ids::music_controls_play_tip),
            makeWidget({ 58, 64 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::music_controls_next, string_ids::music_controls_next_tip),
            makeWidget({ 256, 64 }, { 109, 24 }, widget_type::wt_5, 1, -1, string_ids::set_volume_tip),
            makeWidget({ 10, 93 }, { 346, 12 }, widget_type::wt_18, 1, string_ids::arg2_stringid),
            makeWidget({ 344, 94 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            makeWidget({ 183, 108 }, { 173, 12 }, widget_type::wt_11, 1, string_ids::edit_music_selection, string_ids::edit_music_selection_tip),
            widgetEnd(),
        };

        static void volumeMouseDown(window* w);
        static void stopMusic(window* w);
        static void playMusic(window* w);
        static void playNextSong(window* w);
        static void musicPlaylistMouseDown(window* w);
        static void musicPlaylistDropdown(window* w, int16_t ax);
        static void currentlyPlayingMouseDown(window* w);
        static void currentlyPlayingDropdown(window* w, int16_t ax);

        static window_event_list _events;

        static void prepareDraw(window* w)
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
                songName = audio::getMusicInfo(_currentSong)->title_id;
            }

            FormatArguments args = {};
            args.push(songName);

            static const string_id playlist_string_ids[] = {
                string_ids::play_only_music_from_current_era,
                string_ids::play_all_music,
                string_ids::play_custom_music_selection,
            };

            string_id currentSongStringId = playlist_string_ids[(uint8_t)config::get().music_playlist];
            args.push(currentSongStringId);

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
            if (config::get().music_playlist == config::music_playlist_type::custom)
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

            common::drawTabs(w, dpi);

            gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::currently_playing_btn].top, 0, string_ids::currently_playing, nullptr);

            gfx::drawString_494B3F(*dpi, w->x + 183, w->y + w->widgets[widx::volume].top + 7, 0, string_ids::volume, nullptr);

            gfx::drawImage(dpi, w->x + w->widgets[widx::volume].left, w->y + w->widgets[widx::volume].top, 0x20000000 | (w->colours[1] << 19) | image_ids::volume_slider_track);

            int16_t x = 90 + (config::get().volume / 32);
            gfx::drawImage(dpi, w->x + w->widgets[widx::volume].left + x, w->y + w->widgets[widx::volume].top, 0x20000000 | (w->colours[1] << 19) | image_ids::volume_slider_thumb);
        }

        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    WindowManager::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::tabOnMouseUp(w, wi);
                    return;

                case widx::music_controls_stop:
                    stopMusic(w);
                    return;

                case widx::music_controls_play:
                    playMusic(w);
                    return;

                case widx::music_controls_next:
                    playNextSong(w);
                    return;

                case widx::edit_selection:
                    windows::music_selection::open();
                    return;
            }
        }

        // 0x004C06F2
        static void onMouseDown(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::music_playlist_btn:
                    musicPlaylistMouseDown(w);
                    break;
                case widx::currently_playing_btn:
                    currentlyPlayingMouseDown(w);
                    break;
                case widx::volume:
                    volumeMouseDown(w);
                    break;
            }
        }

        // 0x004C070D
        static void onDropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::music_playlist_btn:
                    musicPlaylistDropdown(window, itemIndex);
                    break;
                case widx::currently_playing_btn:
                    currentlyPlayingDropdown(window, itemIndex);
                    break;
            }
        }

        static loco_global<int16_t, 0x005233A4> _5233A4;
        static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;

        // 0x004C072A
        static void volumeMouseDown(window* w)
        {
            _clickRepeatTicks = 31;

            int x = _5233A4 - w->x - w->widgets[widx::volume].left - 10;
            x = std::clamp(x, 0, 80);

            registers regs;
            regs.ecx = (x * 32) - 2560;
            call(0x0048AA67, regs);

            w->invalidate();
        }

        // 0x004C0778
        static void stopMusic(window* w)
        {
            if (config::get().music_playing == 0)
                return;

            auto& cfg = config::get();
            cfg.music_playing = 0;
            config::write();

            audio::stopBackgroundMusic();

            _currentSong = -1;

            w->invalidate();
        }

        // 0x004C07A4
        static void playMusic(window* w)
        {
            if (config::get().music_playing != 0)
                return;

            auto& cfg = config::get();
            cfg.music_playing = 1;
            config::write();

            w->invalidate();
        }

        // 0x004C07C4
        static void playNextSong(window* w)
        {
            if (config::get().music_playing == 0)
                return;

            audio::stopBackgroundMusic();

            _currentSong = -1;

            w->invalidate();
        }

#pragma mark - Widget 17

        // 0x004C07E4
        static void musicPlaylistMouseDown(window* w)
        {
            widget_t dropdown = w->widgets[widx::music_playlist];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 3, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::play_only_music_from_current_era);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::play_all_music);
            dropdown::add(2, string_ids::dropdown_stringid, string_ids::play_custom_music_selection);

            dropdown::setItemSelected((uint8_t)config::get().music_playlist);
        }

        // 0x004C084A
        static void musicPlaylistDropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = OpenLoco::config::get();
            cfg.music_playlist = (config::music_playlist_type)ax;
            config::write();

            w->invalidate();

            audio::revalidateCurrentTrack();

            WindowManager::close(WindowType::musicSelection);
        }

#pragma mark - Widget 11

        static std::vector<int> get_available_tracks()
        {
            auto vector = std::vector<int>();

            if (config::get().music_playlist == config::music_playlist_type::current_era)
            {
                uint16_t year = getCurrentYear();
                for (int i = 0; i < audio::num_music_tracks; i++)
                {
                    auto info = audio::getMusicInfo(i);
                    if (year >= info->start_year && year <= info->end_year)
                    {
                        vector.push_back(i);
                    }
                }
            }
            else if (config::get().music_playlist == config::music_playlist_type::all)
            {
                for (int i = 0; i < audio::num_music_tracks; i++)
                {
                    vector.push_back(i);
                }
            }
            else if (config::get().music_playlist == config::music_playlist_type::custom)
            {
                for (int i = 0; i < audio::num_music_tracks; i++)
                {
                    if (config::get().enabled_music[i] & 1)
                    {
                        vector.push_back(i);
                    }
                }

                if (vector.size() == 0)
                {
                    for (int i = 0; i < audio::num_music_tracks; i++)
                    {
                        vector.push_back(i);
                    }
                }
            }

            return vector;
        }

        // 0x004C0875
        static void currentlyPlayingMouseDown(window* w)
        {
            auto tracks = get_available_tracks();

            widget_t dropdown = w->widgets[widx::currently_playing];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], tracks.size(), 0x80);

            int index = -1;
            for (auto track : tracks)
            {
                index++;
                dropdown::add(index, string_ids::dropdown_stringid, audio::getMusicInfo(track)->title_id);
                if (track == _currentSong)
                {
                    dropdown::setItemSelected(index);
                }
            }
        }

        // 0x004C09F8
        static void currentlyPlayingDropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto tracks = get_available_tracks();
            int track = tracks.at(ax);
            if (track == _currentSong)
                return;

            audio::stopBackgroundMusic();

            _currentSong = track;
            _50D435 = track;
            _50D430 = 0;

            w->invalidate();
        }

        // 0x004C0A37
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->current_tab + 4);
        }

        static void initEvents()
        {
            _events.on_close = onClose;
            _events.on_mouse_up = onMouseUp;
            _events.on_mouse_down = onMouseDown;
            _events.on_dropdown = onDropdown;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
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

        static constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << regional::widx::language) | (1 << regional::widx::language_btn) | (1 << regional::widx::distance_speed) | (1 << regional::widx::distance_speed_btn) | (1 << regional::widx::heights) | (1 << regional::widx::heights_btn) | (1 << regional::widx::currency) | (1 << regional::widx::currency_btn) | (1 << regional::widx::preferred_currency) | (1 << regional::widx::preferred_currency_btn) | (1 << regional::widx::preferred_currency_for_new_games) | (1 << regional::widx::preferred_currency_always);

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_regional),
            makeWidget({ 183, 49 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::stringptr),
            makeWidget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            makeWidget({ 183, 69 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::arg12_stringid),
            makeWidget({ 344, 70 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            makeWidget({ 183, 84 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::arg6_stringid),
            makeWidget({ 344, 85 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown),
            makeWidget({ 183, 104 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::arg10_stringid, string_ids::current_game_currency_tip),
            makeWidget({ 344, 105 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::current_game_currency_tip),
            makeWidget({ 183, 119 }, { 173, 12 }, widget_type::wt_18, 1, string_ids::preferred_currency_buffer, string_ids::new_game_currency_tip),
            makeWidget({ 344, 120 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::new_game_currency_tip),
            makeWidget({ 10, 134 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::use_preferred_currency_new_game, string_ids::use_preferred_currency_new_game_tip),
            makeWidget({ 10, 148 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::use_preferred_currency_always, string_ids::use_preferred_currency_always_tip),
            widgetEnd(),
        };

        static void languageMouseDown(window* w);
        static void languageDropdown(window* w, int16_t ax);
        static void currencyMouseDown(window* w);
        static void currencyDropdown(window* w, int16_t ax);
        static void preferredCurrencyMouseDown(window* w);
        static void preferredCurrencyDropdown(window* w, int16_t ax);
        static void preferredCurrencyNewGameMouseUp(window* w);
        static void preferredCurrencyAlwaysMouseUp(window* w);
        static void distanceSpeedMouseDown(window* w);
        static void distanceSpeedDropdown(window* w, int16_t ax);
        static void heightsLabelsMouseDown(window* w);
        static void heightsLabelsDropdown(window* w, int16_t ax);

        static window_event_list _events;

        // 0x004C0A59
        static void prepareDraw(window* w)
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

            FormatArguments args = {};

            auto& language = localisation::getDescriptorForLanguage(config::getNew().language);
            args.push(language.native_name.c_str());

            string_id current_height_units = string_ids::height_units;
            if ((OpenLoco::config::get().flags & config::flags::show_height_as_units) == 0)
            {
                current_height_units = string_ids::height_real_values;
            }

            args.skip(0x2);
            args.push(current_height_units);

            string_id current_measurement_format = string_ids::imperial;
            if (OpenLoco::config::get().measurement_format)
            {
                current_measurement_format = string_ids::metric;
            }

            args.skip(0x2);
            args.push(objectmgr::get<currency_object>()->name);
            args.push(current_measurement_format);

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
            common::drawTabs(w, dpi);

            gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::language].top + 1, 0, string_ids::options_language, nullptr);
            gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::distance_speed].top + 1, 0, string_ids::distance_and_speed, nullptr);
            gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::heights].top + 1, 0, string_ids::heights, nullptr);
            gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::currency].top + 1, 0, string_ids::current_game_currency, nullptr);
            gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::preferred_currency].top + 1, 0, string_ids::new_game_currency, nullptr);
        }

        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    WindowManager::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::tabOnMouseUp(w, wi);
                    return;

                case widx::preferred_currency_for_new_games:
                    preferredCurrencyNewGameMouseUp(w);
                    return;

                case widx::preferred_currency_always:
                    preferredCurrencyAlwaysMouseUp(w);
                    return;
            }
        }

        // 0x004BFBB7
        static void onMouseDown(window* w, widget_index wi)
        {
            switch (wi)
            {
                case widx::language_btn:
                    languageMouseDown(w);
                    break;
                case widx::heights_btn:
                    heightsLabelsMouseDown(w);
                    break;
                case widx::distance_speed_btn:
                    distanceSpeedMouseDown(w);
                    break;
                case widx::currency_btn:
                    currencyMouseDown(w);
                    break;
                case widx::preferred_currency_btn:
                    preferredCurrencyMouseDown(w);
                    break;
            }
        }

        // 0x004C0C4A
        static void onDropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::language_btn:
                    languageDropdown(window, itemIndex);
                    break;

                case widx::heights_btn:
                    heightsLabelsDropdown(window, itemIndex);
                    break;

                case widx::distance_speed_btn:
                    distanceSpeedDropdown(window, itemIndex);
                    break;

                case widx::currency_btn:
                    currencyDropdown(window, itemIndex);
                    break;

                case widx::preferred_currency_btn:
                    preferredCurrencyDropdown(window, itemIndex);
                    break;
            }
        }

        static loco_global<std::byte*, 0x0050D13C> _installedObjectList;

        static void languageMouseDown(window* w)
        {
            auto& lds = localisation::getLanguageDescriptors();
            uint8_t num_languages = static_cast<uint8_t>(lds.size());

            widget_t dropdown = w->widgets[widx::language];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], num_languages - 1, 0x80);

            std::string& current_language = config::getNew().language;

            for (uint8_t index = 1; index < num_languages; index++)
            {
                auto& ld = lds[index];
                dropdown::add(index - 1, string_ids::dropdown_stringptr, (char*)ld.native_name.c_str());

                if (ld.locale == current_language)
                    dropdown::setItemSelected(index - 1);
            }
        }

        static void languageDropdown(window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            auto& lds = localisation::getLanguageDescriptors();
            auto& ld = lds[ax + 1];
            config::getNew().language = ld.locale;
            config::write();
            localisation::loadLanguageFile();
            gfx::invalidateScreen();
        }

        // 0x004C0C73
        static void currencyMouseDown(window* w)
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
                    dropdown::setItemSelected(index);
                }
            }
        }

        // 0x004C0D33
        static void currencyDropdown(window* w, int16_t ax)
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
        static void preferredCurrencyMouseDown(window* w)
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
        static void preferredCurrencyDropdown(window* w, int16_t ax)
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
                    auto& cfg = OpenLoco::config::get();
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
        static void preferredCurrencyNewGameMouseUp(window* w)
        {
            auto& cfg = OpenLoco::config::get();
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
        static void preferredCurrencyAlwaysMouseUp(window* w)
        {
            auto& cfg = OpenLoco::config::get();
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
        static void distanceSpeedMouseDown(window* w)
        {
            widget_t dropdown = w->widgets[widx::distance_speed];
            dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            dropdown::add(0, string_ids::dropdown_stringid, string_ids::imperial);
            dropdown::add(1, string_ids::dropdown_stringid, string_ids::metric);
            dropdown::setItemSelected(config::get().measurement_format);
        }

        // 0x004C0FB3
        static void distanceSpeedDropdown(window* w, int16_t ax)
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
            gfx::invalidateScreen();
        }

        // 0x004C0FFA
        static void heightsLabelsMouseDown(window* w)
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
            dropdown::setItemSelected(selectedItem);
        }

        // 0x004C106C
        static void heightsLabelsDropdown(window* w, int16_t ax)
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
            gfx::invalidateScreen();
        }

        // 0x004C1195
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->current_tab + 4);
        }

        static void initEvents()
        {
            _events.on_close = onClose;
            _events.on_mouse_up = onMouseUp;
            _events.on_mouse_down = onMouseDown;
            _events.on_dropdown = onDropdown;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
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
                zoom_to_cursor,
                customize_keys
            };
        }

        static constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << controls::widx::edge_scrolling) | (1 << controls::widx::customize_keys) | (1 << controls::widx::zoom_to_cursor);

        static const gfx::ui_size_t _window_size = { 366, 99 };

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_controls),
            makeWidget({ 10, 49 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::scroll_screen_edge, string_ids::scroll_screen_edge_tip),
            makeWidget({ 10, 64 }, { 346, 12 }, widget_type::checkbox, 1, string_ids::zoom_to_cursor, string_ids::zoom_to_cursor_tip),
            makeWidget({ 26, 79 }, { 160, 12 }, widget_type::wt_11, 1, string_ids::customise_keys, string_ids::customise_keys_tip),
            widgetEnd(),
        };

        static void edgeScrollingMouseUp(window* w);
        static void zoomToCursorMouseUp(window* w);
        static void openKeyboardShortcuts();

        static window_event_list _events;

        static void prepareDraw(window* w)
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

            w->activated_widgets &= ~(1 << widx::edge_scrolling | 1 << widx::zoom_to_cursor);
            if (config::get().edge_scrolling)
            {
                w->activated_widgets |= (1 << widx::edge_scrolling);
            }
            if (config::getNew().zoom_to_cursor)
            {
                w->activated_widgets |= (1 << widx::zoom_to_cursor);
            }

            sub_4C13BE(w);
        }

        // 0x004C113F
        static void draw(window* w, gfx::drawpixelinfo_t* dpi)
        {
            w->draw(dpi);
            common::drawTabs(w, dpi);
        }

        // 0x004C114A
        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    WindowManager::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::tabOnMouseUp(w, wi);
                    return;

                case widx::customize_keys:
                    openKeyboardShortcuts();
                    break;

                case widx::edge_scrolling:
                    edgeScrollingMouseUp(w);
                    break;

                case widx::zoom_to_cursor:
                    zoomToCursorMouseUp(w);
                    break;
            }
        }

        // 0x004C117A
        static void edgeScrollingMouseUp(window* w)
        {
            auto& cfg = OpenLoco::config::get();
            cfg.edge_scrolling = !cfg.edge_scrolling;
            config::write();

            w->invalidate();
        }

        static void zoomToCursorMouseUp(window* w)
        {
            auto& cfg = OpenLoco::config::getNew();
            cfg.zoom_to_cursor = !cfg.zoom_to_cursor;
            config::write();

            w->invalidate();
        }

        // 0x004C118D
        static void openKeyboardShortcuts()
        {
            KeyboardShortcuts::open();
        }

        // 0x004C1195
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->current_tab + 4);
        }

        static void initEvents()
        {
            _events.on_close = onClose;
            _events.on_mouse_up = onMouseUp;
            _events.on_update = onUpdate;
            _events.prepare_draw = prepareDraw;
            _events.draw = draw;
        }
    }

    namespace misc
    {
        static const gfx::ui_size_t _window_size = { 420, 139 };

        namespace widx
        {
            enum
            {
                disable_vehicle_breakdowns = 10,
                disableAICompanies,
                use_preferred_owner_name,
                change_btn,
                export_plugin_objects,
            };
        }

        static constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << misc::widx::disable_vehicle_breakdowns) | (1 << widx::disableAICompanies) | (1 << misc::widx::use_preferred_owner_name) | (1 << misc::widx::change_btn) | (1 << misc::widx::export_plugin_objects);

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, string_ids::options_title_miscellaneous),
            makeWidget({ 10, 49 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::disable_vehicle_breakdowns, string_ids::null),
            makeWidget({ 10, 64 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::disableAICompanies, string_ids::disableAICompanies_tip),
            makeWidget({ 10, 79 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::use_preferred_owner_name, string_ids::use_preferred_owner_name_tip),
            makeWidget({ 335, 94 }, { 75, 12 }, widget_type::wt_11, 1, string_ids::change),
            makeWidget({ 10, 109 }, { 400, 12 }, widget_type::checkbox, 1, string_ids::export_plugin_objects, string_ids::export_plugin_objects_tip),
            widgetEnd(),
        };

        static window_event_list _events;

        static loco_global<uint8_t, 0x0112A17E> _112A17E;

        static void changePreferredName(window* w);
        static void setPreferredName(window* w, char* str);
        static void usePreferredOwnerNameMouseUp(window* w);
        static void disableVehicleBreakdownsMouseUp(window* w);
        static void disableAICompaniesMouseUp(window* w);
        static void exportPluginObjectsMouseUp(window* w);

        // 0x004C11B7
        static void prepareDraw(window* w)
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

            if (config::getNew().breakdowns_disabled)
                w->activated_widgets |= (1 << widx::disable_vehicle_breakdowns);
            else
                w->activated_widgets &= ~(1 << widx::disable_vehicle_breakdowns);

            if (config::getNew().companyAIDisabled)
                w->activated_widgets |= (1 << widx::disableAICompanies);
            else
                w->activated_widgets &= ~(1 << widx::disableAICompanies);

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
            common::drawTabs(w, dpi);

            auto buffer = (char*)stringmgr::getString(string_ids::buffer_2039);
            char* playerName = config::get().preferred_name;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            FormatArguments args = {};
            args.push(string_ids::buffer_2039);
            gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[widx::change_btn].top + 1, 0, string_ids::wcolour2_preferred_owner_name, &args);
        }

        // 0x004C12D2
        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case common::widx::close_button:
                    WindowManager::close(w);
                    return;

                case common::widx::tab_display:
                case common::widx::tab_sound:
                case common::widx::tab_music:
                case common::widx::tab_regional:
                case common::widx::tab_controls:
                case common::widx::tab_miscellaneous:
                    options::tabOnMouseUp(w, wi);
                    return;

                case widx::disable_vehicle_breakdowns:
                    disableVehicleBreakdownsMouseUp(w);
                    break;

                case widx::disableAICompanies:
                    disableAICompaniesMouseUp(w);
                    break;

                case widx::export_plugin_objects:
                    exportPluginObjectsMouseUp(w);
                    break;

                case widx::use_preferred_owner_name:
                    usePreferredOwnerNameMouseUp(w);
                    break;

                case widx::change_btn:
                    changePreferredName(w);
                    break;
            }
        }

        // 0x004C1304
        static void textInput(window* w, widget_index i, char* str)
        {
            switch (i)
            {
                case widx::use_preferred_owner_name:
                    setPreferredName(w, str);
                    break;
            }
        }

        // 0x004C1319
        static void changePreferredName(window* w)
        {
            auto buffer = (char*)stringmgr::getString(string_ids::buffer_2039);
            char* playerName = config::get().preferred_name;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            textinput::openTextinput(w, string_ids::preferred_owner_name, string_ids::enter_preferred_owner_name, string_ids::buffer_2039, widx::use_preferred_owner_name, nullptr);
        }

        // 0x004C1342
        static void setPreferredName(window* w, char* str)
        {
            auto& cfg = OpenLoco::config::get();

            if (strlen(str) == 0)
                cfg.flags &= ~config::flags::use_preferred_owner_name;

            strcpy(cfg.preferred_name, str);
            cfg.preferred_name[strlen(str)] = '\0';

            config::write();

            w->invalidate();
        }

        // 0x004C135F
        static void usePreferredOwnerNameMouseUp(window* w)
        {
            auto& cfg = OpenLoco::config::get();
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
                    changePreferredName(w);
                }
            }
        }

        static void disableVehicleBreakdownsMouseUp(window* w)
        {
            auto& cfg = OpenLoco::config::getNew();
            cfg.breakdowns_disabled = !cfg.breakdowns_disabled;
            config::write();
            w->invalidate();
        }

        static void disableAICompaniesMouseUp(window* w)
        {
            auto& cfg = OpenLoco::config::getNew();
            cfg.companyAIDisabled = !cfg.companyAIDisabled;
            config::write();
            w->invalidate();
        }

        static void exportPluginObjectsMouseUp(window* w)
        {
            auto& cfg = OpenLoco::config::get();
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
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->current_tab + 4);
        }

        static void initEvents()
        {
            _events.on_close = onClose;
            _events.on_mouse_up = onMouseUp;
            _events.on_update = onUpdate;
            _events.text_input = textInput;
            _events.prepare_draw = prepareDraw;
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
        if (isEditorMode() || isTitleMode())
        {
            w->disabled_widgets |= 1 << common::widx::tab_music;
        }

        if (isEditorMode() && s5::getOptions().editorStep == 0)
        {
            w->disabled_widgets |= 1 << common::widx::tab_regional;
        }

        int x = w->widgets[common::widx::tab_display].left;
        for (int i = common::widx::tab_display; i <= common::widx::tab_miscellaneous; i++)
        {
            if (!w->isDisabled(i))
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

        window = WindowManager::bringToFront(WindowType::options);
        if (window != nullptr)
            return window;

        // TODO: only needs to be called once
        display::initEvents();
        sound::initEvents();
        music::initEvents();
        regional::initEvents();
        controls::initEvents();
        misc::initEvents();

        // 0x004BF833 (create_options_window)
        window = WindowManager::createWindowCentred(
            WindowType::options,
            display::_window_size,
            0,
            &display::_events);

        window->widgets = display::_widgets;
        window->number = 0;
        window->current_tab = 0;
        window->frame_no = 0;
        window->row_hover = -1;

        auto interface = objectmgr::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;

        sub_4BF8CD();
        sub_4C1519();

        window->enabled_widgets = display::enabledWidgets;
        display::applyScreenModeRestrictions(window);

        window->holdable_widgets = 0;
        window->event_handlers = &display::_events;
        window->activated_widgets = 0;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        return window;
    }

    // 0x004BF823
    window* openMusicSettings()
    {
        auto window = open();

        window->callOnMouseUp(common::widx::tab_music);

        return window;
    }

    struct TabInformation
    {
        widget_t* widgets;
        window_event_list* events;
        gfx::ui_size_t windowSize;
        const uint64_t* enabledWidgets;
    };

    static TabInformation tabInformationByTabOffset[] = {
        { display::_widgets, &display::_events, display::_window_size, &display::enabledWidgets },
        { sound::_widgets, &sound::_events, sound::_window_size, &sound::enabledWidgets },
        { music::_widgets, &music::_events, music::_window_size, &music::enabledWidgets },
        { regional::_widgets, &regional::_events, regional::_window_size, &regional::enabledWidgets },
        { controls::_widgets, &controls::_events, controls::_window_size, &controls::enabledWidgets },
        { misc::_widgets, &misc::_events, misc::_window_size, &misc::enabledWidgets },
    };

    // 0x004BFC11
    static void tabOnMouseUp(window* w, widget_index wi)
    {
        input::toolCancel(w->type, w->number);

        textinput::sub_4CE6C9(w->type, w->number);
        w->current_tab = wi - common::widx::tab_display;
        w->frame_no = 0;
        w->flags &= ~(window_flags::flag_16);
        w->disabled_widgets = 0;
        w->holdable_widgets = 0;
        w->activated_widgets = 0;
        w->row_hover = -1;
        if (w->viewports[0])
        {
            w->viewports[0]->width = 0;
            w->viewports[0] = nullptr;
        }

        auto& tabInfo = tabInformationByTabOffset[w->current_tab];
        w->enabled_widgets = *tabInfo.enabledWidgets;
        w->event_handlers = tabInfo.events;
        w->widgets = tabInfo.widgets;
        w->invalidate();
        w->setSize(tabInfo.windowSize);

        if ((common::tab)w->current_tab == common::tab::display)
            display::applyScreenModeRestrictions(w);

        else if ((common::tab)w->current_tab == common::tab::music)
            w->holdable_widgets = (1 << music::widx::volume);

        w->callOnResize();
        w->callPrepareDraw();
        w->initScrollWidgets();
        w->invalidate();
    }
}
