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

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Options
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

    namespace Common
    {
        namespace Widx
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

        static_assert(Widx::tab_music == Widx::tab_display + tab_offset_music);

        enum tab
        {
            display,
            sound,
            music,
            regional,
            controls,
            miscellaneous,
        };

        static void drawTabs(window* w, Gfx::drawpixelinfo_t* ctx)
        {
            Widget::draw_tab(w, ctx, ImageIds::tab_display, Widx::tab_display);
            Widget::draw_tab(w, ctx, ImageIds::tab_sound, Widx::tab_sound);

            static const uint32_t music_tab_ids[] = {
                ImageIds::tab_music_0,
                ImageIds::tab_music_1,
                ImageIds::tab_music_2,
                ImageIds::tab_music_3,
                ImageIds::tab_music_4,
                ImageIds::tab_music_5,
                ImageIds::tab_music_6,
                ImageIds::tab_music_7,
                ImageIds::tab_music_8,
                ImageIds::tab_music_9,
                ImageIds::tab_music_10,
                ImageIds::tab_music_11,
                ImageIds::tab_music_12,
                ImageIds::tab_music_13,
                ImageIds::tab_music_14,
                ImageIds::tab_music_15,
            };
            uint32_t imageId = music_tab_ids[0];
            if (w->current_tab == tab::music)
            {
                imageId = music_tab_ids[(w->frame_no / 4) % 16];
            }
            Widget::draw_tab(w, ctx, imageId, Widx::tab_music);

            static const uint32_t globe_tab_ids[] = {
                ImageIds::tab_globe_0,
                ImageIds::tab_globe_1,
                ImageIds::tab_globe_2,
                ImageIds::tab_globe_3,
                ImageIds::tab_globe_4,
                ImageIds::tab_globe_5,
                ImageIds::tab_globe_6,
                ImageIds::tab_globe_7,
                ImageIds::tab_globe_8,
                ImageIds::tab_globe_9,
                ImageIds::tab_globe_10,
                ImageIds::tab_globe_11,
                ImageIds::tab_globe_12,
                ImageIds::tab_globe_13,
                ImageIds::tab_globe_14,
                ImageIds::tab_globe_15,
                ImageIds::tab_globe_16,
                ImageIds::tab_globe_17,
                ImageIds::tab_globe_18,
                ImageIds::tab_globe_19,
                ImageIds::tab_globe_20,
                ImageIds::tab_globe_21,
                ImageIds::tab_globe_22,
                ImageIds::tab_globe_23,
                ImageIds::tab_globe_24,
                ImageIds::tab_globe_25,
                ImageIds::tab_globe_26,
                ImageIds::tab_globe_27,
                ImageIds::tab_globe_28,
                ImageIds::tab_globe_29,
                ImageIds::tab_globe_30,
                ImageIds::tab_globe_31,
            };
            imageId = ImageIds::tab_globe_0;
            if (w->current_tab == tab::regional)
            {
                imageId = globe_tab_ids[(w->frame_no / 2) % 32];
            }
            Widget::draw_tab(w, ctx, imageId, Widx::tab_regional);

            Widget::draw_tab(w, ctx, ImageIds::tab_control, Widx::tab_controls);
            Widget::draw_tab(w, ctx, ImageIds::tab_miscellaneous, Widx::tab_miscellaneous);
        }

#define common_options_widgets(window_size, window_caption_id)                                                                                           \
    makeWidget({ 0, 0 }, window_size, widget_type::frame, 0),                                                                                            \
        makeWidget({ 1, 1 }, { (uint16_t)(window_size.width - 2), 13 }, widget_type::caption_25, 0, window_caption_id),                                  \
        makeWidget({ (int16_t)(window_size.width - 15), 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { window_size.width, 102 }, widget_type::panel, 1),                                                                        \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::display_options),                                         \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::sound_options),                                          \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::music_options),                                          \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::regional_options),                                       \
        makeRemapWidget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::control_options),                                       \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::miscellaneous_options)

        static constexpr int tabWidgets = (1 << Widx::tab_display) | (1 << Widx::tab_sound) | (1 << Widx::tab_music) | (1 << Widx::tab_regional) | (1 << Widx::tab_controls) | (1 << Widx::tab_miscellaneous);

        static constexpr int enabledWidgets = (1 << Widx::close_button) | tabWidgets;
    }

    namespace Display
    {
        static const Gfx::ui_size_t _window_size = { 400, 184 };

        namespace Widx
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
            common_options_widgets(_window_size, StringIds::options_title_display),
            makeWidget({ 235, 49 }, { 156, 12 }, widget_type::wt_18, 1, StringIds::empty),
            makeWidget({ 379, 50 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 235, 64 }, { 156, 12 }, widget_type::wt_18, 1, StringIds::display_resolution_label_format),
            makeWidget({ 379, 65 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeStepperWidgets({ 235, 79 }, { 156, 12 }, widget_type::wt_17, 1, StringIds::empty),
            makeWidget({ 10, 99 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::landscape_smoothing, StringIds::landscape_smoothing_tip),
            makeWidget({ 10, 114 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::gridlines_on_landscape, StringIds::gridlines_on_landscape_tip),
            makeWidget({ 235, 133 }, { 156, 12 }, widget_type::wt_18, 1, StringIds::empty, StringIds::vehicles_min_scale_tip),
            makeWidget({ 379, 134 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown, StringIds::vehicles_min_scale_tip),
            makeWidget({ 235, 148 }, { 156, 12 }, widget_type::wt_18, 1, StringIds::empty, StringIds::station_names_min_scale_tip),
            makeWidget({ 379, 149 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown, StringIds::station_names_min_scale_tip),
            makeWidget({ 235, 163 }, { 156, 12 }, widget_type::wt_18, 1, StringIds::empty),
            makeWidget({ 379, 164 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            widgetEnd(),
        };

        static window_event_list _events;

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Display::Widx::landscape_smoothing) | (1 << Display::Widx::gridlines_on_landscape) | (1 << Display::Widx::vehicles_min_scale) | (1 << Display::Widx::vehicles_min_scale_btn) | (1 << Display::Widx::station_names_min_scale) | (1 << Display::Widx::station_names_min_scale_btn) | (1 << Display::Widx::construction_marker) | (1 << Display::Widx::construction_marker_btn) | (1 << Display::Widx::display_scale_up_btn) | (1 << Display::Widx::display_scale_down_btn);

        // 0x004BFB8C
        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Common::Widx::close_button:
                    WindowManager::close(w);
                    return;

                case Common::Widx::tab_display:
                case Common::Widx::tab_sound:
                case Common::Widx::tab_music:
                case Common::Widx::tab_regional:
                case Common::Widx::tab_controls:
                case Common::Widx::tab_miscellaneous:
                    Options::tabOnMouseUp(w, wi);
                    return;

                case Widx::landscape_smoothing:
                {
                    auto& cfg = OpenLoco::Config::get();
                    // TODO: is there a better way to toggle a flag?
                    if (cfg.flags & Config::flags::landscape_smoothing)
                    {
                        cfg.flags &= ~Config::flags::landscape_smoothing;
                    }
                    else
                    {
                        cfg.flags |= Config::flags::landscape_smoothing;
                    }
                    OpenLoco::Config::write();
                    Gfx::invalidateScreen();
                    return;
                }

                case Widx::gridlines_on_landscape:
                {
                    auto& cfg = OpenLoco::Config::get();
                    if (cfg.flags & Config::flags::gridlines_on_landscape)
                    {
                        cfg.flags &= ~Config::flags::gridlines_on_landscape;
                    }
                    else
                    {
                        cfg.flags |= Config::flags::gridlines_on_landscape;
                    }
                    OpenLoco::Config::write();
                    Gfx::invalidateScreen();

                    auto main = WindowManager::getMainWindow();
                    if (main != nullptr)
                    {
                        main->viewports[0]->flags &= ~ViewportFlags::gridlines_on_landscape;

                        if ((cfg.flags & Config::flags::gridlines_on_landscape) != 0)
                        {
                            main->viewports[0]->flags |= ViewportFlags::gridlines_on_landscape;
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
            widget_t dropdown = w->widgets[Widx::construction_marker];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::white);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::translucent);
            Dropdown::setItemSelected(Config::get().construction_marker);
        }

        // 0x004BFE98
        static void constructionMarkerDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == Config::get().construction_marker)
                return;

            auto& cfg = OpenLoco::Config::get();
            cfg.construction_marker = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

#pragma mark - Vehicle zoom (Widget 15)

        // 0x004BFEBE
        static void vehicleZoomMouseDown(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[Widx::vehicles_min_scale];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 4, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::full_scale);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::half_scale);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::quarter_scale);
            Dropdown::add(3, StringIds::dropdown_stringid, StringIds::eighth_scale);
            Dropdown::setItemSelected(Config::get().vehicles_min_scale);
        }

        // 0x004BFF4C
        static void vehicleZoomDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == Config::get().vehicles_min_scale)
                return;

            auto& cfg = OpenLoco::Config::get();
            cfg.vehicles_min_scale = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

#pragma mark - Station names minimum scale (Widget 17)

        // 0x004BFF72
        static void stationNamesScaleMouseDown(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[Widx::station_names_min_scale];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 4, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::full_scale);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::half_scale);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::quarter_scale);
            Dropdown::add(3, StringIds::dropdown_stringid, StringIds::eighth_scale);
            Dropdown::setItemSelected(Config::get().station_names_min_scale);
        }

        // 0x004C0000
        static void stationNamesScaleDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == Config::get().station_names_min_scale)
                return;

            auto& cfg = OpenLoco::Config::get();
            cfg.station_names_min_scale = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

#if !(defined(__APPLE__) && defined(__MACH__))
        static void screenModeToggleEnabled(window* w)
        {
            if (Config::getNew().display.mode == Config::screen_mode::fullscreen)
            {
                w->enabled_widgets |= (1 << Widx::display_resolution) | (1 << Widx::display_resolution_btn);
                w->disabled_widgets &= ~((1 << Widx::display_resolution) | (1 << Widx::display_resolution_btn));
            }
            else
            {
                w->enabled_widgets &= ~((1 << Widx::display_resolution) | (1 << Widx::display_resolution_btn));
                w->disabled_widgets |= (1 << Widx::display_resolution) | (1 << Widx::display_resolution_btn);
            }
        }
#endif

        static void screenModeMouseDown(window* w, widget_index wi)
        {
            widget_t dropdown = w->widgets[Widx::screen_mode];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::options_mode_windowed);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::options_mode_fullscreen);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::options_mode_fullscreen_window);

            auto selection = static_cast<uint16_t>(Config::getNew().display.mode);
            Dropdown::setItemSelected(selection);
        }

        static void screenModeDropdown(window* w, int16_t selection)
        {
            if (selection == -1)
                return;

            auto new_mode = static_cast<Config::screen_mode>(selection);
            if (new_mode == Config::getNew().display.mode)
                return;

#if !(defined(__APPLE__) && defined(__MACH__))
            Ui::setDisplayMode(new_mode);
#endif
        }

#pragma mark - Resolution dropdown (Widget 11)

        // 0x004C0026
        static void resolutionMouseDown(window* w, widget_index wi)
        {
            std::vector<Resolution> resolutions = getFullscreenResolutions();

            widget_t dropdown = w->widgets[Widx::display_resolution];
            Dropdown::showText2(w->x + dropdown.left, w->y + dropdown.top, dropdown.width(), dropdown.height(), w->colours[1], resolutions.size(), 0x80);

            auto& cfg = Config::getNew();
            for (size_t i = 0; i < resolutions.size(); i++)
            {
                Dropdown::add(i, StringIds::dropdown_stringid, { StringIds::display_resolution_dropdown_format, (uint16_t)resolutions[i].width, (uint16_t)resolutions[i].height });
                if (cfg.display.fullscreen_resolution.width == resolutions[i].width && cfg.display.fullscreen_resolution.height == resolutions[i].height)
                    Dropdown::setItemSelected((int16_t)i);
            }
        }

        // 0x004C00F4
        static void resolutionDropdown(window* w, int16_t index)
        {
            if (index == -1)
                return;
            std::vector<Resolution> resolutions = getFullscreenResolutions();
            Ui::setDisplayMode(Config::screen_mode::fullscreen, { resolutions[index].width, resolutions[index].height });
        }

#pragma mark -

        static void displayScaleMouseDown(window* w, widget_index wi, float adjust_by)
        {
            OpenLoco::Ui::adjustWindowScale(adjust_by);
        }

        // 0x004BFBB7
        static void onMouseDown(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Widx::screen_mode_btn:
                    screenModeMouseDown(w, wi);
                    break;
                case Widx::display_resolution_btn:
                    resolutionMouseDown(w, wi);
                    break;
                case Widx::construction_marker_btn:
                    constructionMarkerMouseDown(w, wi);
                    break;
                case Widx::vehicles_min_scale_btn:
                    vehicleZoomMouseDown(w, wi);
                    break;
                case Widx::station_names_min_scale_btn:
                    stationNamesScaleMouseDown(w, wi);
                    break;
                case Widx::display_scale_down_btn:
                    displayScaleMouseDown(w, wi, -OpenLoco::Ui::ScaleFactor::step);
                    break;
                case Widx::display_scale_up_btn:
                    displayScaleMouseDown(w, wi, OpenLoco::Ui::ScaleFactor::step);
                    break;
            }
        }

        // 0x004BFBE8
        static void onDropdown(window* w, widget_index wi, int16_t item_index)
        {
            switch (wi)
            {
                case Widx::screen_mode_btn:
                    screenModeDropdown(w, item_index);
                    break;
                case Widx::display_resolution_btn:
                    resolutionDropdown(w, item_index);
                    break;
                case Widx::construction_marker_btn:
                    constructionMarkerDropdown(item_index);
                    break;
                case Widx::vehicles_min_scale_btn:
                    vehicleZoomDropdown(item_index);
                    break;
                case Widx::station_names_min_scale_btn:
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
            assert(w->current_tab == Common::tab::display);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~Common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[Common::Widx::frame].right = w->width - 1;
            w->widgets[Common::Widx::frame].bottom = w->height - 1;
            w->widgets[Common::Widx::panel].right = w->width - 1;
            w->widgets[Common::Widx::panel].bottom = w->height - 1;
            w->widgets[Common::Widx::caption].right = w->width - 2;
            w->widgets[Common::Widx::close_button].left = w->width - 15;
            w->widgets[Common::Widx::close_button].right = w->width - 15 + 12;

            string_id screen_mode_string_id = StringIds::empty;
            switch (Config::getNew().display.mode)
            {
                case Config::screen_mode::window:
                    screen_mode_string_id = StringIds::options_mode_windowed;
                    break;
                case Config::screen_mode::fullscreen:
                    screen_mode_string_id = StringIds::options_mode_fullscreen;
                    break;
                case Config::screen_mode::fullscreen_borderless:
                    screen_mode_string_id = StringIds::options_mode_fullscreen_window;
                    break;
            }
            w->widgets[Widx::screen_mode].text = screen_mode_string_id;

            FormatArguments args = {};
            args.skip(0x10);
            auto& resolution = Config::getNew().display.fullscreen_resolution;
            args.push<uint16_t>(resolution.width);
            args.push<uint16_t>(resolution.height);

            if (Config::get().construction_marker)
                w->widgets[Widx::construction_marker].text = StringIds::translucent;
            else
                w->widgets[Widx::construction_marker].text = StringIds::white;

            static const string_id scale_string_ids[] = {
                StringIds::full_scale,
                StringIds::half_scale,
                StringIds::quarter_scale,
                StringIds::eighth_scale,
            };

            w->widgets[Widx::vehicles_min_scale].text = scale_string_ids[Config::get().vehicles_min_scale];
            w->widgets[Widx::station_names_min_scale].text = scale_string_ids[Config::get().station_names_min_scale];

            w->activated_widgets &= ~(1 << Widx::landscape_smoothing);
            if ((Config::get().flags & Config::flags::landscape_smoothing) == 0)
            {
                w->activated_widgets |= (1 << Widx::landscape_smoothing);
            }

            w->activated_widgets &= ~(1 << Widx::gridlines_on_landscape);
            if (Config::get().flags & Config::flags::gridlines_on_landscape)
            {
                w->activated_widgets |= (1 << Widx::gridlines_on_landscape);
            }

            if (Config::getNew().scale_factor <= OpenLoco::Ui::ScaleFactor::min)
                w->disabled_widgets |= (1 << Widx::display_scale_down_btn);
            else
                w->disabled_widgets &= ~(1 << Widx::display_scale_down_btn);

            if (Config::getNew().scale_factor >= OpenLoco::Ui::ScaleFactor::max)
                w->disabled_widgets |= (1 << Widx::display_scale_up_btn);
            else
                w->disabled_widgets &= ~(1 << Widx::display_scale_up_btn);

#if !(defined(__APPLE__) && defined(__MACH__))
            screenModeToggleEnabled(w);
#endif

            sub_4C13BE(w);
        }

        // 0x004BFAF9
        static void draw(window* w, Gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            Common::drawTabs(w, dpi);

            int16_t x = w->x + 10;
            int16_t y = w->y + Display::_widgets[Display::Widx::screen_mode].top + 1;
            drawString_494B3F(*dpi, x, y, Colour::black, StringIds::options_screen_mode, nullptr);

            y = w->y + Display::_widgets[Display::Widx::display_resolution].top + 1;
            drawString_494B3F(*dpi, x + 14, y, Colour::black, StringIds::display_resolution, nullptr);

            y = w->y + Display::_widgets[Display::Widx::construction_marker].top + 1;
            drawString_494B3F(*dpi, x, y, Colour::black, StringIds::construction_marker, nullptr);

            y = w->y + Display::_widgets[Display::Widx::vehicles_min_scale].top + 1;
            drawString_494B3F(*dpi, x, y, Colour::black, StringIds::vehicles_min_scale, nullptr);

            y = w->y + Display::_widgets[Display::Widx::station_names_min_scale].top + 1;
            drawString_494B3F(*dpi, x, y, Colour::black, StringIds::station_names_min_scale, nullptr);

            y = w->y + Display::_widgets[Display::Widx::display_scale].top + 1;
            drawString_494B3F(*dpi, x + 14, y, Colour::black, StringIds::window_scale_factor, nullptr);

            int scale = (int)(Config::getNew().scale_factor * 100);
            auto& scale_widget = w->widgets[Widx::display_scale];
            drawString_494B3F(*dpi, w->x + scale_widget.left + 1, w->y + scale_widget.top + 1, Colour::black, StringIds::scale_formatted, &scale);
        }

        static void applyScreenModeRestrictions(window* w)
        {
            if (Config::getNew().display.mode != Config::screen_mode::fullscreen)
                w->disabled_widgets = (1 << Display::Widx::display_resolution) | (1 << Display::Widx::display_resolution_btn);

#if !(defined(__APPLE__) && defined(__MACH__))
            w->enabled_widgets |= (1 << Display::Widx::screen_mode) | (1 << Display::Widx::screen_mode_btn);
            Display::screenModeToggleEnabled(w);
#else
            w->disabled_widgets |= (1 << Display::Widx::screen_mode) | (1 << Display::Widx::screen_mode_btn) | (1 << Display::Widx::display_resolution) | (1 << Display::Widx::display_resolution_btn);
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

    namespace Sound
    {
        static const Gfx::ui_size_t _window_size = { 366, 84 };

        namespace Widx
        {
            enum
            {
                audio_device = 10,
                audio_device_btn,
                play_title_music,
            };
        }

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Sound::Widx::audio_device) | (1 << Sound::Widx::audio_device_btn) | (1 << Sound::Widx::play_title_music);

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_sound),
            makeWidget({ 10, 49 }, { 346, 12 }, widget_type::wt_18, 1, StringIds::stringid),
            makeWidget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 10, 65 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::play_title_music),
            widgetEnd(),
        };

        static window_event_list _events;

        static void audioDeviceMouseDown(Ui::window* window);
        static void audioDeviceDropdown(Ui::window* window, int16_t itemIndex);
        static void playTitleMusicOnMouseUp(Ui::window* window);

        // 0x004C0217
        static void prepareDraw(window* w)
        {
            assert(w->current_tab == Common::tab::sound);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~Common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[Common::Widx::frame].right = w->width - 1;
            w->widgets[Common::Widx::frame].bottom = w->height - 1;
            w->widgets[Common::Widx::panel].right = w->width - 1;
            w->widgets[Common::Widx::panel].bottom = w->height - 1;
            w->widgets[Common::Widx::caption].right = w->width - 2;
            w->widgets[Common::Widx::close_button].left = w->width - 15;
            w->widgets[Common::Widx::close_button].right = w->width - 15 + 12;

            FormatArguments args = {};

            auto audioDeviceName = Audio::getCurrentDeviceName();
            if (audioDeviceName != nullptr)
            {
                args.push(StringIds::stringptr);
                args.push(audioDeviceName);
            }
            else
                args.push(StringIds::audio_device_none);

            if (Config::getNew().audio.play_title_music)
                w->activated_widgets |= (1 << Widx::play_title_music);
            else
                w->activated_widgets &= ~(1 << Widx::play_title_music);

            sub_4C13BE(w);
        }

        // 0x004C02F5
        static void draw(window* w, Gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            Common::drawTabs(w, dpi);
        }

        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Common::Widx::close_button:
                    WindowManager::close(w);
                    return;

                case Common::Widx::tab_display:
                case Common::Widx::tab_sound:
                case Common::Widx::tab_music:
                case Common::Widx::tab_regional:
                case Common::Widx::tab_controls:
                case Common::Widx::tab_miscellaneous:
                    Options::tabOnMouseUp(w, wi);
                    return;

                case Widx::play_title_music:
                    playTitleMusicOnMouseUp(w);
                    return;
            }
        }

        static void onMouseDown(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Widx::audio_device_btn:
                    audioDeviceMouseDown(w);
                    break;
            }
        }

        static void onDropdown(Ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case Widx::audio_device_btn:
                    audioDeviceDropdown(window, itemIndex);
                    break;
            }
        }

#pragma mark - Widget 11

        // 0x004C043D
        static void audioDeviceMouseDown(Ui::window* w)
        {
            const auto& devices = Audio::getDevices();
            if (devices.size() != 0)
            {
                widget_t dropdown = w->widgets[Widx::audio_device];
                Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], devices.size(), 0x80);
                for (size_t i = 0; i < devices.size(); i++)
                {
                    auto name = devices[i].c_str();
                    Dropdown::add(i, StringIds::dropdown_stringid, { StringIds::stringptr, name });
                }

                auto currentDevice = Audio::getCurrentDevice();
                if (currentDevice != std::numeric_limits<size_t>().max())
                {
                    Dropdown::setItemSelected((int16_t)currentDevice);
                }
            }
        }

        // 0x004C04CA
        static void audioDeviceDropdown(Ui::window* w, int16_t itemIndex)
        {
            if (itemIndex != -1)
            {
                Audio::setDevice(itemIndex);
                WindowManager::invalidateWidget(w->type, w->number, Widx::audio_device);
            }
        }

#pragma mark -

        static void playTitleMusicOnMouseUp(window* w)
        {
            auto& cfg = Config::getNew();
            cfg.audio.play_title_music = !cfg.audio.play_title_music;
            Config::write();

            Audio::playTitleScreenMusic();

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

    namespace Music
    {
        static const Gfx::ui_size_t _window_size = { 366, 129 };

        namespace Widx
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

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Music::Widx::currently_playing) | (1 << Music::Widx::currently_playing_btn) | (1 << Music::Widx::music_controls_stop) | (1 << Music::Widx::music_controls_play) | (1 << Music::Widx::music_controls_next) | (1 << Music::Widx::volume) | (1 << Music::Widx::music_playlist) | (1 << Music::Widx::music_playlist_btn) | (1 << Music::Widx::edit_selection);

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_music),
            makeWidget({ 160, 49 }, { 196, 12 }, widget_type::wt_18, 1, StringIds::stringid),
            makeWidget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 10, 64 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::music_controls_stop, StringIds::music_controls_stop_tip),
            makeWidget({ 34, 64 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::music_controls_play, StringIds::music_controls_play_tip),
            makeWidget({ 58, 64 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::music_controls_next, StringIds::music_controls_next_tip),
            makeWidget({ 256, 64 }, { 109, 24 }, widget_type::wt_5, 1, -1, StringIds::set_volume_tip),
            makeWidget({ 10, 93 }, { 346, 12 }, widget_type::wt_18, 1, StringIds::arg2_stringid),
            makeWidget({ 344, 94 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 183, 108 }, { 173, 12 }, widget_type::wt_11, 1, StringIds::edit_music_selection, StringIds::edit_music_selection_tip),
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
            assert(w->current_tab == Common::tab::music);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~Common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[Common::Widx::frame].right = w->width - 1;
            w->widgets[Common::Widx::frame].bottom = w->height - 1;
            w->widgets[Common::Widx::panel].right = w->width - 1;
            w->widgets[Common::Widx::panel].bottom = w->height - 1;
            w->widgets[Common::Widx::caption].right = w->width - 2;
            w->widgets[Common::Widx::close_button].left = w->width - 15;
            w->widgets[Common::Widx::close_button].right = w->width - 15 + 12;

            string_id songName = StringIds::music_none;
            if (_currentSong != -1)
            {
                songName = Audio::getMusicInfo(_currentSong)->title_id;
            }

            FormatArguments args = {};
            args.push(songName);

            static const string_id playlist_string_ids[] = {
                StringIds::play_only_music_from_current_era,
                StringIds::play_all_music,
                StringIds::play_custom_music_selection,
            };

            string_id currentSongStringId = playlist_string_ids[(uint8_t)Config::get().music_playlist];
            args.push(currentSongStringId);

            w->activated_widgets &= ~((1 << Widx::music_controls_stop) | (1 << Widx::music_controls_play));
            w->activated_widgets |= (1 << Widx::music_controls_stop);
            if (_currentSong != -1)
            {
                if (Config::get().music_playing)
                {
                    w->activated_widgets &= ~((1 << Widx::music_controls_stop) | (1 << Widx::music_controls_play));
                    w->activated_widgets |= (1 << Widx::music_controls_play);
                }
            }

            w->disabled_widgets |= (1 << Widx::edit_selection);
            if (Config::get().music_playlist == Config::music_playlist_type::custom)
            {
                w->disabled_widgets &= ~(1 << Widx::edit_selection);
            }

            sub_4C13BE(w);
        }

        // 0x004C05F9
        static void draw(window* w, Gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);

            Common::drawTabs(w, dpi);

            Gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[Widx::currently_playing_btn].top, 0, StringIds::currently_playing, nullptr);

            Gfx::drawString_494B3F(*dpi, w->x + 183, w->y + w->widgets[Widx::volume].top + 7, 0, StringIds::volume, nullptr);

            Gfx::drawImage(dpi, w->x + w->widgets[Widx::volume].left, w->y + w->widgets[Widx::volume].top, 0x20000000 | (w->colours[1] << 19) | ImageIds::volume_slider_track);

            int16_t x = 90 + (Config::get().volume / 32);
            Gfx::drawImage(dpi, w->x + w->widgets[Widx::volume].left + x, w->y + w->widgets[Widx::volume].top, 0x20000000 | (w->colours[1] << 19) | ImageIds::volume_slider_thumb);
        }

        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Common::Widx::close_button:
                    WindowManager::close(w);
                    return;

                case Common::Widx::tab_display:
                case Common::Widx::tab_sound:
                case Common::Widx::tab_music:
                case Common::Widx::tab_regional:
                case Common::Widx::tab_controls:
                case Common::Widx::tab_miscellaneous:
                    Options::tabOnMouseUp(w, wi);
                    return;

                case Widx::music_controls_stop:
                    stopMusic(w);
                    return;

                case Widx::music_controls_play:
                    playMusic(w);
                    return;

                case Widx::music_controls_next:
                    playNextSong(w);
                    return;

                case Widx::edit_selection:
                    Windows::MusicSelection::open();
                    return;
            }
        }

        // 0x004C06F2
        static void onMouseDown(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Widx::music_playlist_btn:
                    musicPlaylistMouseDown(w);
                    break;
                case Widx::currently_playing_btn:
                    currentlyPlayingMouseDown(w);
                    break;
                case Widx::volume:
                    volumeMouseDown(w);
                    break;
            }
        }

        // 0x004C070D
        static void onDropdown(Ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case Widx::music_playlist_btn:
                    musicPlaylistDropdown(window, itemIndex);
                    break;
                case Widx::currently_playing_btn:
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

            int x = _5233A4 - w->x - w->widgets[Widx::volume].left - 10;
            x = std::clamp(x, 0, 80);

            registers regs;
            regs.ecx = (x * 32) - 2560;
            call(0x0048AA67, regs);

            w->invalidate();
        }

        // 0x004C0778
        static void stopMusic(window* w)
        {
            if (Config::get().music_playing == 0)
                return;

            auto& cfg = Config::get();
            cfg.music_playing = 0;
            Config::write();

            Audio::stopBackgroundMusic();

            _currentSong = -1;

            w->invalidate();
        }

        // 0x004C07A4
        static void playMusic(window* w)
        {
            if (Config::get().music_playing != 0)
                return;

            auto& cfg = Config::get();
            cfg.music_playing = 1;
            Config::write();

            w->invalidate();
        }

        // 0x004C07C4
        static void playNextSong(window* w)
        {
            if (Config::get().music_playing == 0)
                return;

            Audio::stopBackgroundMusic();

            _currentSong = -1;

            w->invalidate();
        }

#pragma mark - Widget 17

        // 0x004C07E4
        static void musicPlaylistMouseDown(window* w)
        {
            widget_t dropdown = w->widgets[Widx::music_playlist];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::play_only_music_from_current_era);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::play_all_music);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::play_custom_music_selection);

            Dropdown::setItemSelected((uint8_t)Config::get().music_playlist);
        }

        // 0x004C084A
        static void musicPlaylistDropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = OpenLoco::Config::get();
            cfg.music_playlist = (Config::music_playlist_type)ax;
            Config::write();

            w->invalidate();

            Audio::revalidateCurrentTrack();

            WindowManager::close(WindowType::musicSelection);
        }

#pragma mark - Widget 11

        static std::vector<int> get_available_tracks()
        {
            auto vector = std::vector<int>();

            if (Config::get().music_playlist == Config::music_playlist_type::current_era)
            {
                uint16_t year = getCurrentYear();
                for (int i = 0; i < Audio::num_music_tracks; i++)
                {
                    auto info = Audio::getMusicInfo(i);
                    if (year >= info->start_year && year <= info->end_year)
                    {
                        vector.push_back(i);
                    }
                }
            }
            else if (Config::get().music_playlist == Config::music_playlist_type::all)
            {
                for (int i = 0; i < Audio::num_music_tracks; i++)
                {
                    vector.push_back(i);
                }
            }
            else if (Config::get().music_playlist == Config::music_playlist_type::custom)
            {
                for (int i = 0; i < Audio::num_music_tracks; i++)
                {
                    if (Config::get().enabled_music[i] & 1)
                    {
                        vector.push_back(i);
                    }
                }

                if (vector.size() == 0)
                {
                    for (int i = 0; i < Audio::num_music_tracks; i++)
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

            widget_t dropdown = w->widgets[Widx::currently_playing];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], tracks.size(), 0x80);

            int index = -1;
            for (auto track : tracks)
            {
                index++;
                Dropdown::add(index, StringIds::dropdown_stringid, Audio::getMusicInfo(track)->title_id);
                if (track == _currentSong)
                {
                    Dropdown::setItemSelected(index);
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

            Audio::stopBackgroundMusic();

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

    namespace Regional
    {
        static const Gfx::ui_size_t _window_size = { 366, 167 };

        namespace Widx
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

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Regional::Widx::language) | (1 << Regional::Widx::language_btn) | (1 << Regional::Widx::distance_speed) | (1 << Regional::Widx::distance_speed_btn) | (1 << Regional::Widx::heights) | (1 << Regional::Widx::heights_btn) | (1 << Regional::Widx::currency) | (1 << Regional::Widx::currency_btn) | (1 << Regional::Widx::preferred_currency) | (1 << Regional::Widx::preferred_currency_btn) | (1 << Regional::Widx::preferred_currency_for_new_games) | (1 << Regional::Widx::preferred_currency_always);

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_regional),
            makeWidget({ 183, 49 }, { 173, 12 }, widget_type::wt_18, 1, StringIds::stringptr),
            makeWidget({ 344, 50 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 183, 69 }, { 173, 12 }, widget_type::wt_18, 1, StringIds::arg12_stringid),
            makeWidget({ 344, 70 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 183, 84 }, { 173, 12 }, widget_type::wt_18, 1, StringIds::arg6_stringid),
            makeWidget({ 344, 85 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown),
            makeWidget({ 183, 104 }, { 173, 12 }, widget_type::wt_18, 1, StringIds::arg10_stringid, StringIds::current_game_currency_tip),
            makeWidget({ 344, 105 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown, StringIds::current_game_currency_tip),
            makeWidget({ 183, 119 }, { 173, 12 }, widget_type::wt_18, 1, StringIds::preferred_currency_buffer, StringIds::new_game_currency_tip),
            makeWidget({ 344, 120 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown, StringIds::new_game_currency_tip),
            makeWidget({ 10, 134 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::use_preferred_currency_new_game, StringIds::use_preferred_currency_new_game_tip),
            makeWidget({ 10, 148 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::use_preferred_currency_always, StringIds::use_preferred_currency_always_tip),
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
            assert(w->current_tab == Common::tab::regional);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~Common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[Common::Widx::frame].right = w->width - 1;
            w->widgets[Common::Widx::frame].bottom = w->height - 1;
            w->widgets[Common::Widx::panel].right = w->width - 1;
            w->widgets[Common::Widx::panel].bottom = w->height - 1;
            w->widgets[Common::Widx::caption].right = w->width - 2;
            w->widgets[Common::Widx::close_button].left = w->width - 15;
            w->widgets[Common::Widx::close_button].right = w->width - 15 + 12;

            FormatArguments args = {};

            auto& language = Localisation::getDescriptorForLanguage(Config::getNew().language);
            args.push(language.native_name.c_str());

            string_id current_height_units = StringIds::height_units;
            if ((OpenLoco::Config::get().flags & Config::flags::show_height_as_units) == 0)
            {
                current_height_units = StringIds::height_real_values;
            }

            args.skip(0x2);
            args.push(current_height_units);

            string_id current_measurement_format = StringIds::imperial;
            if (OpenLoco::Config::get().measurement_format)
            {
                current_measurement_format = StringIds::metric;
            }

            args.skip(0x2);
            args.push(ObjectManager::get<currency_object>()->name);
            args.push(current_measurement_format);

            w->activated_widgets &= ~(1 << Widx::preferred_currency_for_new_games);
            if (Config::get().flags & Config::flags::preferred_currency_for_new_games)
            {
                w->activated_widgets |= (1 << Widx::preferred_currency_for_new_games);
            }

            w->activated_widgets &= ~(1 << Widx::preferred_currency_always);
            if (Config::get().flags & Config::flags::preferred_currency_always)
            {
                w->activated_widgets |= (1 << Widx::preferred_currency_always);
            }

            w->disabled_widgets &= ~(1 << Widx::currency);
            w->disabled_widgets &= ~(1 << Widx::currency_btn);
            if (Config::get().flags & Config::flags::preferred_currency_always)
            {
                w->disabled_widgets |= (1 << Widx::currency);
                w->disabled_widgets |= (1 << Widx::currency_btn);
            }

            sub_4C13BE(w);
        }

        // 0x004C0B5B
        static void draw(window* w, Gfx::drawpixelinfo_t* dpi)
        {
            // Draw widgets.
            w->draw(dpi);
            Common::drawTabs(w, dpi);

            Gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[Widx::language].top + 1, 0, StringIds::options_language, nullptr);
            Gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[Widx::distance_speed].top + 1, 0, StringIds::distance_and_speed, nullptr);
            Gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[Widx::heights].top + 1, 0, StringIds::heights, nullptr);
            Gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[Widx::currency].top + 1, 0, StringIds::current_game_currency, nullptr);
            Gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[Widx::preferred_currency].top + 1, 0, StringIds::new_game_currency, nullptr);
        }

        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Common::Widx::close_button:
                    WindowManager::close(w);
                    return;

                case Common::Widx::tab_display:
                case Common::Widx::tab_sound:
                case Common::Widx::tab_music:
                case Common::Widx::tab_regional:
                case Common::Widx::tab_controls:
                case Common::Widx::tab_miscellaneous:
                    Options::tabOnMouseUp(w, wi);
                    return;

                case Widx::preferred_currency_for_new_games:
                    preferredCurrencyNewGameMouseUp(w);
                    return;

                case Widx::preferred_currency_always:
                    preferredCurrencyAlwaysMouseUp(w);
                    return;
            }
        }

        // 0x004BFBB7
        static void onMouseDown(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Widx::language_btn:
                    languageMouseDown(w);
                    break;
                case Widx::heights_btn:
                    heightsLabelsMouseDown(w);
                    break;
                case Widx::distance_speed_btn:
                    distanceSpeedMouseDown(w);
                    break;
                case Widx::currency_btn:
                    currencyMouseDown(w);
                    break;
                case Widx::preferred_currency_btn:
                    preferredCurrencyMouseDown(w);
                    break;
            }
        }

        // 0x004C0C4A
        static void onDropdown(Ui::window* window, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case Widx::language_btn:
                    languageDropdown(window, itemIndex);
                    break;

                case Widx::heights_btn:
                    heightsLabelsDropdown(window, itemIndex);
                    break;

                case Widx::distance_speed_btn:
                    distanceSpeedDropdown(window, itemIndex);
                    break;

                case Widx::currency_btn:
                    currencyDropdown(window, itemIndex);
                    break;

                case Widx::preferred_currency_btn:
                    preferredCurrencyDropdown(window, itemIndex);
                    break;
            }
        }

        static loco_global<std::byte*, 0x0050D13C> _installedObjectList;

        static void languageMouseDown(window* w)
        {
            auto& lds = Localisation::getLanguageDescriptors();
            uint8_t num_languages = static_cast<uint8_t>(lds.size());

            widget_t dropdown = w->widgets[Widx::language];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], num_languages - 1, 0x80);

            std::string& current_language = Config::getNew().language;

            for (uint8_t index = 1; index < num_languages; index++)
            {
                auto& ld = lds[index];
                Dropdown::add(index - 1, StringIds::dropdown_stringptr, (char*)ld.native_name.c_str());

                if (ld.locale == current_language)
                    Dropdown::setItemSelected(index - 1);
            }
        }

        static void languageDropdown(window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            auto& lds = Localisation::getLanguageDescriptors();
            auto& ld = lds[ax + 1];
            Config::getNew().language = ld.locale;
            Config::write();
            Localisation::loadLanguageFile();
            Gfx::invalidateScreen();
        }

        // 0x004C0C73
        static void currencyMouseDown(window* w)
        {
            uint8_t* _11364A0 = (uint8_t*)*__11364A0;

            widget_t dropdown = w->widgets[Widx::currency];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], _112C185, 0x80);
            int index = -1;
            for (auto object : ObjectManager::getAvailableObjects(object_type::currency))
            {
                index++;
                Dropdown::add(index, StringIds::dropdown_stringptr, object.second._name);

                if (_11364A0[object.first] & 1)
                {
                    Dropdown::setItemSelected(index);
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
            for (auto object : ObjectManager::getAvailableObjects(object_type::currency))
            {
                index++;
                if (index == ax)
                {
                    auto ebp = ObjectManager::getActiveObject(object_type::currency, _11364A0);

                    if (ebp.index != -1)
                    {
                        registers regs2;
                        regs2.ebp = (uintptr_t)ebp.object._header;
                        call(0x00471FF8, regs2); // unload object
                    }

                    registers regs3;
                    regs3.ebp = (uintptr_t)object.second._header;

                    call(0x00471BCE, regs3);
                    ObjectManager::resetLoadedObjects();
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
            widget_t dropdown = w->widgets[Widx::preferred_currency];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], _112C185, 0x80);

            int index = -1;
            for (auto object : ObjectManager::getAvailableObjects(object_type::currency))
            {
                index++;
                Dropdown::add(index, StringIds::dropdown_stringptr, object.second._name);

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
            for (auto object : ObjectManager::getAvailableObjects(object_type::currency))
            {
                index++;

                if (index == ax)
                {
                    auto& cfg = OpenLoco::Config::get();
                    memcpy(cfg.preferred_currency, object.second._header, 0x10);

                    sub_4C1519();
                    Config::write();
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
            auto& cfg = OpenLoco::Config::get();
            if (cfg.flags & Config::flags::preferred_currency_for_new_games)
            {
                cfg.flags &= ~Config::flags::preferred_currency_for_new_games;
            }
            else
            {
                cfg.flags |= Config::flags::preferred_currency_for_new_games;
            }
            Config::write();

            w->invalidate();
        }

        // 0x004C0F27
        static void preferredCurrencyAlwaysMouseUp(window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            if (cfg.flags & Config::flags::preferred_currency_always)
            {
                cfg.flags &= ~Config::flags::preferred_currency_always;
            }
            else
            {
                cfg.flags |= Config::flags::preferred_currency_always;
            }
            Config::write();

            call(0x004C153B);
            sub_4BF935();

            w->invalidate();
        }

        // 0x004C0F49
        static void distanceSpeedMouseDown(window* w)
        {
            widget_t dropdown = w->widgets[Widx::distance_speed];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::imperial);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::metric);
            Dropdown::setItemSelected(Config::get().measurement_format);
        }

        // 0x004C0FB3
        static void distanceSpeedDropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = Config::get();
            cfg.measurement_format = ax;

            // 0x004C0FC2
            cfg.height_marker_offset = 0;
            if ((cfg.flags & Config::flags::show_height_as_units) == 0)
            {
                cfg.height_marker_offset = 0x100 + cfg.measurement_format * 0x100;
            }

            Config::write();
            Gfx::invalidateScreen();
        }

        // 0x004C0FFA
        static void heightsLabelsMouseDown(window* w)
        {
            widget_t dropdown = w->widgets[Widx::heights];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->colours[1], 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::height_units);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::height_real_values);

            int selectedItem = 0;
            if ((Config::get().flags & Config::flags::show_height_as_units) == 0)
            {
                selectedItem = 1;
            }
            Dropdown::setItemSelected(selectedItem);
        }

        // 0x004C106C
        static void heightsLabelsDropdown(window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = Config::get();
            cfg.flags &= ~Config::flags::show_height_as_units;

            if (ax == 0)
            {
                cfg.flags |= Config::flags::show_height_as_units;
            }

            // 0x004C0FC2
            cfg.height_marker_offset = 0;
            if ((cfg.flags & Config::flags::show_height_as_units) == 0)
            {
                cfg.height_marker_offset = 0x100 + cfg.measurement_format * 0x100;
            }

            Config::write();
            Gfx::invalidateScreen();
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

    namespace Controls
    {
        namespace Widx
        {
            enum
            {
                edge_scrolling = 10,
                zoom_to_cursor,
                customize_keys
            };
        }

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Controls::Widx::edge_scrolling) | (1 << Controls::Widx::customize_keys) | (1 << Controls::Widx::zoom_to_cursor);

        static const Gfx::ui_size_t _window_size = { 366, 99 };

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_controls),
            makeWidget({ 10, 49 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::scroll_screen_edge, StringIds::scroll_screen_edge_tip),
            makeWidget({ 10, 64 }, { 346, 12 }, widget_type::checkbox, 1, StringIds::zoom_to_cursor, StringIds::zoom_to_cursor_tip),
            makeWidget({ 26, 79 }, { 160, 12 }, widget_type::wt_11, 1, StringIds::customise_keys, StringIds::customise_keys_tip),
            widgetEnd(),
        };

        static void edgeScrollingMouseUp(window* w);
        static void zoomToCursorMouseUp(window* w);
        static void openKeyboardShortcuts();

        static window_event_list _events;

        static void prepareDraw(window* w)
        {
            assert(w->current_tab == Common::tab::controls);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~Common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[Common::Widx::frame].right = w->width - 1;
            w->widgets[Common::Widx::frame].bottom = w->height - 1;
            w->widgets[Common::Widx::panel].right = w->width - 1;
            w->widgets[Common::Widx::panel].bottom = w->height - 1;
            w->widgets[Common::Widx::caption].right = w->width - 2;
            w->widgets[Common::Widx::close_button].left = w->width - 15;
            w->widgets[Common::Widx::close_button].right = w->width - 15 + 12;

            w->activated_widgets &= ~(1 << Widx::edge_scrolling | 1 << Widx::zoom_to_cursor);
            if (Config::get().edge_scrolling)
            {
                w->activated_widgets |= (1 << Widx::edge_scrolling);
            }
            if (Config::getNew().zoom_to_cursor)
            {
                w->activated_widgets |= (1 << Widx::zoom_to_cursor);
            }

            sub_4C13BE(w);
        }

        // 0x004C113F
        static void draw(window* w, Gfx::drawpixelinfo_t* dpi)
        {
            w->draw(dpi);
            Common::drawTabs(w, dpi);
        }

        // 0x004C114A
        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Common::Widx::close_button:
                    WindowManager::close(w);
                    return;

                case Common::Widx::tab_display:
                case Common::Widx::tab_sound:
                case Common::Widx::tab_music:
                case Common::Widx::tab_regional:
                case Common::Widx::tab_controls:
                case Common::Widx::tab_miscellaneous:
                    Options::tabOnMouseUp(w, wi);
                    return;

                case Widx::customize_keys:
                    openKeyboardShortcuts();
                    break;

                case Widx::edge_scrolling:
                    edgeScrollingMouseUp(w);
                    break;

                case Widx::zoom_to_cursor:
                    zoomToCursorMouseUp(w);
                    break;
            }
        }

        // 0x004C117A
        static void edgeScrollingMouseUp(window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.edge_scrolling = !cfg.edge_scrolling;
            Config::write();

            w->invalidate();
        }

        static void zoomToCursorMouseUp(window* w)
        {
            auto& cfg = OpenLoco::Config::getNew();
            cfg.zoom_to_cursor = !cfg.zoom_to_cursor;
            Config::write();

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

    namespace Misc
    {
        static const Gfx::ui_size_t _window_size = { 420, 139 };

        namespace Widx
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

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Misc::Widx::disable_vehicle_breakdowns) | (1 << Widx::disableAICompanies) | (1 << Misc::Widx::use_preferred_owner_name) | (1 << Misc::Widx::change_btn) | (1 << Misc::Widx::export_plugin_objects);

        static widget_t _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_miscellaneous),
            makeWidget({ 10, 49 }, { 400, 12 }, widget_type::checkbox, 1, StringIds::disable_vehicle_breakdowns, StringIds::null),
            makeWidget({ 10, 64 }, { 400, 12 }, widget_type::checkbox, 1, StringIds::disableAICompanies, StringIds::disableAICompanies_tip),
            makeWidget({ 10, 79 }, { 400, 12 }, widget_type::checkbox, 1, StringIds::use_preferred_owner_name, StringIds::use_preferred_owner_name_tip),
            makeWidget({ 335, 94 }, { 75, 12 }, widget_type::wt_11, 1, StringIds::change),
            makeWidget({ 10, 109 }, { 400, 12 }, widget_type::checkbox, 1, StringIds::export_plugin_objects, StringIds::export_plugin_objects_tip),
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
            assert(w->current_tab == Common::tab::miscellaneous);
            assert(w->widgets == _widgets);

            w->activated_widgets &= ~Common::tabWidgets;
            w->activated_widgets |= 1ULL << (w->current_tab + 4);

            w->widgets[Common::Widx::frame].right = w->width - 1;
            w->widgets[Common::Widx::frame].bottom = w->height - 1;
            w->widgets[Common::Widx::panel].right = w->width - 1;
            w->widgets[Common::Widx::panel].bottom = w->height - 1;
            w->widgets[Common::Widx::caption].right = w->width - 2;
            w->widgets[Common::Widx::close_button].left = w->width - 15;
            w->widgets[Common::Widx::close_button].right = w->width - 15 + 12;

            if (Config::getNew().breakdowns_disabled)
                w->activated_widgets |= (1 << Widx::disable_vehicle_breakdowns);
            else
                w->activated_widgets &= ~(1 << Widx::disable_vehicle_breakdowns);

            if (Config::getNew().companyAIDisabled)
                w->activated_widgets |= (1 << Widx::disableAICompanies);
            else
                w->activated_widgets &= ~(1 << Widx::disableAICompanies);

            w->activated_widgets &= ~(1 << Widx::export_plugin_objects);
            if (Config::get().flags & Config::flags::export_objects_with_saves)
            {
                w->activated_widgets |= (1 << Widx::export_plugin_objects);
            }

            w->activated_widgets &= ~(1 << Widx::use_preferred_owner_name);
            w->disabled_widgets |= (1 << Widx::change_btn);
            if (Config::get().flags & Config::flags::use_preferred_owner_name)
            {
                w->activated_widgets |= (1 << Widx::use_preferred_owner_name);
                w->disabled_widgets &= ~(1 << Widx::change_btn);
            }

            w->widgets[Widx::export_plugin_objects].type = widget_type::none;
            if (_112A17E)
            {
                w->widgets[Widx::export_plugin_objects].type = widget_type::checkbox;
            }

            sub_4C13BE(w);
        }

        // 0x004C1282
        static void draw(window* w, Gfx::drawpixelinfo_t* dpi)
        {
            w->draw(dpi);
            Common::drawTabs(w, dpi);

            auto buffer = (char*)StringManager::getString(StringIds::buffer_2039);
            char* playerName = Config::get().preferred_name;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            FormatArguments args = {};
            args.push(StringIds::buffer_2039);
            Gfx::drawString_494B3F(*dpi, w->x + 10, w->y + w->widgets[Widx::change_btn].top + 1, 0, StringIds::wcolour2_preferred_owner_name, &args);
        }

        // 0x004C12D2
        static void onMouseUp(window* w, widget_index wi)
        {
            switch (wi)
            {
                case Common::Widx::close_button:
                    WindowManager::close(w);
                    return;

                case Common::Widx::tab_display:
                case Common::Widx::tab_sound:
                case Common::Widx::tab_music:
                case Common::Widx::tab_regional:
                case Common::Widx::tab_controls:
                case Common::Widx::tab_miscellaneous:
                    Options::tabOnMouseUp(w, wi);
                    return;

                case Widx::disable_vehicle_breakdowns:
                    disableVehicleBreakdownsMouseUp(w);
                    break;

                case Widx::disableAICompanies:
                    disableAICompaniesMouseUp(w);
                    break;

                case Widx::export_plugin_objects:
                    exportPluginObjectsMouseUp(w);
                    break;

                case Widx::use_preferred_owner_name:
                    usePreferredOwnerNameMouseUp(w);
                    break;

                case Widx::change_btn:
                    changePreferredName(w);
                    break;
            }
        }

        // 0x004C1304
        static void textInput(window* w, widget_index i, char* str)
        {
            switch (i)
            {
                case Widx::use_preferred_owner_name:
                    setPreferredName(w, str);
                    break;
            }
        }

        // 0x004C1319
        static void changePreferredName(window* w)
        {
            auto buffer = (char*)StringManager::getString(StringIds::buffer_2039);
            char* playerName = Config::get().preferred_name;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            TextInput::openTextInput(w, StringIds::preferred_owner_name, StringIds::enter_preferred_owner_name, StringIds::buffer_2039, Widx::use_preferred_owner_name, nullptr);
        }

        // 0x004C1342
        static void setPreferredName(window* w, char* str)
        {
            auto& cfg = OpenLoco::Config::get();

            if (strlen(str) == 0)
                cfg.flags &= ~Config::flags::use_preferred_owner_name;

            strcpy(cfg.preferred_name, str);
            cfg.preferred_name[strlen(str)] = '\0';

            Config::write();

            w->invalidate();
        }

        // 0x004C135F
        static void usePreferredOwnerNameMouseUp(window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            if (cfg.flags & Config::flags::use_preferred_owner_name)
            {
                cfg.flags &= ~Config::flags::use_preferred_owner_name;
            }
            else
            {
                cfg.flags |= Config::flags::use_preferred_owner_name;
            }
            Config::write();

            w->invalidate();

            if (cfg.flags & Config::flags::use_preferred_owner_name)
            {
                if (strlen(cfg.preferred_name) == 0)
                {
                    changePreferredName(w);
                }
            }
        }

        static void disableVehicleBreakdownsMouseUp(window* w)
        {
            auto& cfg = OpenLoco::Config::getNew();
            cfg.breakdowns_disabled = !cfg.breakdowns_disabled;
            Config::write();
            w->invalidate();
        }

        static void disableAICompaniesMouseUp(window* w)
        {
            auto& cfg = OpenLoco::Config::getNew();
            cfg.companyAIDisabled = !cfg.companyAIDisabled;
            Config::write();
            w->invalidate();
        }

        static void exportPluginObjectsMouseUp(window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            if (cfg.flags & Config::flags::export_objects_with_saves)
            {
                cfg.flags &= ~Config::flags::export_objects_with_saves;
            }
            else
            {
                cfg.flags |= Config::flags::export_objects_with_saves;
            }
            Config::write();

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
        auto ptr = malloc(ObjectManager::getNumInstalledObjects());
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
        w->disabled_widgets &= ~((1 << Common::Widx::tab_music) | (1 << Common::Widx::tab_regional));
        if (isEditorMode() || isTitleMode())
        {
            w->disabled_widgets |= 1 << Common::Widx::tab_music;
        }

        if (isEditorMode() && S5::getOptions().editorStep == 0)
        {
            w->disabled_widgets |= 1 << Common::Widx::tab_regional;
        }

        int x = w->widgets[Common::Widx::tab_display].left;
        for (int i = Common::Widx::tab_display; i <= Common::Widx::tab_miscellaneous; i++)
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
        Display::initEvents();
        Sound::initEvents();
        Music::initEvents();
        Regional::initEvents();
        Controls::initEvents();
        Misc::initEvents();

        // 0x004BF833 (create_options_window)
        window = WindowManager::createWindowCentred(
            WindowType::options,
            Display::_window_size,
            0,
            &Display::_events);

        window->widgets = Display::_widgets;
        window->number = 0;
        window->current_tab = 0;
        window->frame_no = 0;
        window->row_hover = -1;

        auto interface = ObjectManager::get<interface_skin_object>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;

        sub_4BF8CD();
        sub_4C1519();

        window->enabled_widgets = Display::enabledWidgets;
        Display::applyScreenModeRestrictions(window);

        window->holdable_widgets = 0;
        window->event_handlers = &Display::_events;
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

        window->callOnMouseUp(Common::Widx::tab_music);

        return window;
    }

    struct TabInformation
    {
        widget_t* widgets;
        window_event_list* events;
        Gfx::ui_size_t windowSize;
        const uint64_t* enabledWidgets;
    };

    static TabInformation tabInformationByTabOffset[] = {
        { Display::_widgets, &Display::_events, Display::_window_size, &Display::enabledWidgets },
        { Sound::_widgets, &Sound::_events, Sound::_window_size, &Sound::enabledWidgets },
        { Music::_widgets, &Music::_events, Music::_window_size, &Music::enabledWidgets },
        { Regional::_widgets, &Regional::_events, Regional::_window_size, &Regional::enabledWidgets },
        { Controls::_widgets, &Controls::_events, Controls::_window_size, &Controls::enabledWidgets },
        { Misc::_widgets, &Misc::_events, Misc::_window_size, &Misc::enabledWidgets },
    };

    // 0x004BFC11
    static void tabOnMouseUp(window* w, widget_index wi)
    {
        Input::toolCancel(w->type, w->number);

        TextInput::sub_4CE6C9(w->type, w->number);
        w->current_tab = wi - Common::Widx::tab_display;
        w->frame_no = 0;
        w->flags &= ~(WindowFlags::flag_16);
        w->disabled_widgets = 0;
        w->holdable_widgets = 0;
        w->activated_widgets = 0;
        w->row_hover = -1;
        w->viewportRemove(0);

        auto& tabInfo = tabInformationByTabOffset[w->current_tab];
        w->enabled_widgets = *tabInfo.enabledWidgets;
        w->event_handlers = tabInfo.events;
        w->widgets = tabInfo.widgets;
        w->invalidate();
        w->setSize(tabInfo.windowSize);

        if ((Common::tab)w->current_tab == Common::tab::display)
            Display::applyScreenModeRestrictions(w);

        else if ((Common::tab)w->current_tab == Common::tab::music)
            w->holdable_widgets = (1 << Music::Widx::volume);

        w->callOnResize();
        w->callPrepareDraw();
        w->initScrollWidgets();
        w->invalidate();
    }
}
