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

namespace OpenLoco::Ui::Windows::Options
{
    static void tabOnMouseUp(Window* w, WidgetIndex_t wi);
    static void sub_4C13BE(Window* w);
    static void sub_4C1519();
    static void sub_4BF935();

    static loco_global<uint32_t, 0x0050D430> _songProgress;
    static loco_global<int8_t, 0x0050D434> _currentSong;
    static loco_global<uint8_t, 0x0050D435> _lastSong;

    // Should be a pointer to an array of u8's
    static loco_global<void*, 0x011364A0> __11364A0;
    static loco_global<uint16_t, 0x0112C185> _112C185;

    static void onClose(Window* w)
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

        static void drawTabs(Window* w, Gfx::Context* ctx)
        {
            Widget::drawTab(w, ctx, ImageIds::tab_display, Widx::tab_display);
            Widget::drawTab(w, ctx, ImageIds::tab_sound, Widx::tab_sound);

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
            if (w->currentTab == tab::music)
            {
                imageId = music_tab_ids[(w->frame_no / 4) % 16];
            }
            Widget::drawTab(w, ctx, imageId, Widx::tab_music);

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
            if (w->currentTab == tab::regional)
            {
                imageId = globe_tab_ids[(w->frame_no / 2) % 32];
            }
            Widget::drawTab(w, ctx, imageId, Widx::tab_regional);

            Widget::drawTab(w, ctx, ImageIds::tab_control, Widx::tab_controls);
            Widget::drawTab(w, ctx, ImageIds::tab_miscellaneous, Widx::tab_miscellaneous);
        }

#define common_options_widgets(window_size, window_caption_id)                                                                                                                         \
    makeWidget({ 0, 0 }, window_size, WidgetType::frame, WindowColour::primary),                                                                                                       \
        makeWidget({ 1, 1 }, { (uint16_t)(window_size.width - 2), 13 }, WidgetType::caption_25, WindowColour::primary, window_caption_id),                                             \
        makeWidget({ (int16_t)(window_size.width - 15), 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { window_size.width, 102 }, WidgetType::panel, WindowColour::secondary),                                                                                 \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::display_options),                                                   \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::sound_options),                                                    \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::music_options),                                                    \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::regional_options),                                                 \
        makeRemapWidget({ 127, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::control_options),                                                 \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::miscellaneous_options)

        static constexpr int tabWidgets = (1 << Widx::tab_display) | (1 << Widx::tab_sound) | (1 << Widx::tab_music) | (1 << Widx::tab_regional) | (1 << Widx::tab_controls) | (1 << Widx::tab_miscellaneous);

        static constexpr int enabledWidgets = (1 << Widx::close_button) | tabWidgets;
    }

    namespace Display
    {
        static const Ui::Size _window_size = { 400, 250 };

        namespace Widx
        {
            enum
            {
                frame_hardware = 10,
                screen_mode,
                screen_mode_btn,
                display_resolution,
                display_resolution_btn,
                display_scale,
                display_scale_down_btn,
                display_scale_up_btn,
                uncap_fps,
                show_fps,
                frame_map_rendering,
                vehicles_min_scale,
                vehicles_min_scale_btn,
                station_names_min_scale,
                station_names_min_scale_btn,
                construction_marker,
                construction_marker_btn,
                landscape_smoothing,
                gridlines_on_landscape,
            };
        }

        static Widget _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_display),
            makeWidget({ 4, 49 }, { 392, 97 }, WidgetType::groupbox, WindowColour::secondary, StringIds::frame_hardware),
            makeDropdownWidgets({ 235, 63 }, { 154, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty),
            makeDropdownWidgets({ 235, 79 }, { 154, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::display_resolution_label_format),
            makeStepperWidgets({ 235, 95 }, { 154, 12 }, WidgetType::textbox, WindowColour::secondary, StringIds::empty),
            makeWidget({ 10, 111 }, { 174, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::option_uncap_fps, StringIds::option_uncap_fps_tooltip),
            makeWidget({ 10, 127 }, { 174, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::option_show_fps_counter, StringIds::option_show_fps_counter_tooltip),

            makeWidget({ 4, 150 }, { 392, 96 }, WidgetType::groupbox, WindowColour::secondary, StringIds::frame_map_rendering),
            makeDropdownWidgets({ 235, 164 }, { 154, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty, StringIds::vehicles_min_scale_tip),
            makeDropdownWidgets({ 235, 180 }, { 154, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty, StringIds::station_names_min_scale_tip),
            makeDropdownWidgets({ 235, 196 }, { 154, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty),
            makeWidget({ 10, 211 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::landscape_smoothing, StringIds::landscape_smoothing_tip),
            makeWidget({ 10, 227 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::gridlines_on_landscape, StringIds::gridlines_on_landscape_tip),
            widgetEnd(),
        };

        static WindowEventList _events;

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Widx::show_fps) | (1 << Widx::uncap_fps) | (1 << Display::Widx::landscape_smoothing) | (1 << Display::Widx::gridlines_on_landscape) | (1 << Display::Widx::vehicles_min_scale) | (1 << Display::Widx::vehicles_min_scale_btn) | (1 << Display::Widx::station_names_min_scale) | (1 << Display::Widx::station_names_min_scale_btn) | (1 << Display::Widx::construction_marker) | (1 << Display::Widx::construction_marker_btn) | (1 << Display::Widx::display_scale_up_btn) | (1 << Display::Widx::display_scale_down_btn);

        // 0x004BFB8C
        static void onMouseUp(Window* w, WidgetIndex_t wi)
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

                case Widx::show_fps:
                {
                    auto& cfg = OpenLoco::Config::getNew();
                    cfg.showFPS ^= 1;
                    OpenLoco::Config::writeNewConfig();
                    Gfx::invalidateScreen();
                    return;
                }
                case Widx::uncap_fps:
                {
                    auto& cfg = OpenLoco::Config::getNew();
                    cfg.uncapFPS ^= 1;
                    OpenLoco::Config::writeNewConfig();
                    Gfx::invalidateScreen();
                    return;
                }

                case Widx::landscape_smoothing:
                {
                    auto& cfg = OpenLoco::Config::get();
                    // TODO: is there a better way to toggle a flag?
                    if (cfg.flags & Config::Flags::landscapeSmoothing)
                    {
                        cfg.flags &= ~Config::Flags::landscapeSmoothing;
                    }
                    else
                    {
                        cfg.flags |= Config::Flags::landscapeSmoothing;
                    }
                    OpenLoco::Config::write();
                    Gfx::invalidateScreen();
                    return;
                }

                case Widx::gridlines_on_landscape:
                {
                    auto& cfg = OpenLoco::Config::get();
                    if (cfg.flags & Config::Flags::gridlinesOnLandscape)
                    {
                        cfg.flags &= ~Config::Flags::gridlinesOnLandscape;
                    }
                    else
                    {
                        cfg.flags |= Config::Flags::gridlinesOnLandscape;
                    }
                    OpenLoco::Config::write();
                    Gfx::invalidateScreen();

                    auto main = WindowManager::getMainWindow();
                    if (main != nullptr)
                    {
                        main->viewports[0]->flags &= ~ViewportFlags::gridlines_on_landscape;

                        if ((cfg.flags & Config::Flags::gridlinesOnLandscape) != 0)
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
        static void constructionMarkerMouseDown(Window* w, WidgetIndex_t wi)
        {
            Widget dropdown = w->widgets[Widx::construction_marker];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::white);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::translucent);
            Dropdown::setItemSelected(Config::get().constructionMarker);
        }

        // 0x004BFE98
        static void constructionMarkerDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == Config::get().constructionMarker)
                return;

            auto& cfg = OpenLoco::Config::get();
            cfg.constructionMarker = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

#pragma mark - Vehicle zoom (Widget 15)

        // 0x004BFEBE
        static void vehicleZoomMouseDown(Window* w, WidgetIndex_t wi)
        {
            Widget dropdown = w->widgets[Widx::vehicles_min_scale];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), 4, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::full_scale);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::half_scale);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::quarter_scale);
            Dropdown::add(3, StringIds::dropdown_stringid, StringIds::eighth_scale);
            Dropdown::setItemSelected(Config::get().vehiclesMinScale);
        }

        // 0x004BFF4C
        static void vehicleZoomDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == Config::get().vehiclesMinScale)
                return;

            auto& cfg = OpenLoco::Config::get();
            cfg.vehiclesMinScale = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

#pragma mark - Station names minimum scale (Widget 17)

        // 0x004BFF72
        static void stationNamesScaleMouseDown(Window* w, WidgetIndex_t wi)
        {
            Widget dropdown = w->widgets[Widx::station_names_min_scale];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), 4, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::full_scale);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::half_scale);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::quarter_scale);
            Dropdown::add(3, StringIds::dropdown_stringid, StringIds::eighth_scale);
            Dropdown::setItemSelected(Config::get().stationNamesMinScale);
        }

        // 0x004C0000
        static void stationNamesScaleDropdown(int16_t ax)
        {
            if (ax == -1)
                return;

            if (ax == Config::get().stationNamesMinScale)
                return;

            auto& cfg = OpenLoco::Config::get();
            cfg.stationNamesMinScale = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

#if !(defined(__APPLE__) && defined(__MACH__))
        static void screenModeToggleEnabled(Window* w)
        {
            if (Config::getNew().display.mode == Config::ScreenMode::fullscreen)
            {
                w->enabledWidgets |= (1 << Widx::display_resolution) | (1 << Widx::display_resolution_btn);
                w->disabledWidgets &= ~((1 << Widx::display_resolution) | (1 << Widx::display_resolution_btn));
            }
            else
            {
                w->enabledWidgets &= ~((1 << Widx::display_resolution) | (1 << Widx::display_resolution_btn));
                w->disabledWidgets |= (1 << Widx::display_resolution) | (1 << Widx::display_resolution_btn);
            }
        }
#endif

        static void screenModeMouseDown(Window* w, WidgetIndex_t wi)
        {
            Widget dropdown = w->widgets[Widx::screen_mode];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::options_mode_windowed);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::options_mode_fullscreen);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::options_mode_fullscreen_window);

            auto selection = static_cast<uint16_t>(Config::getNew().display.mode);
            Dropdown::setItemSelected(selection);
        }

        static void screenModeDropdown(Window* w, int16_t selection)
        {
            if (selection == -1)
                return;

            auto new_mode = static_cast<Config::ScreenMode>(selection);
            if (new_mode == Config::getNew().display.mode)
                return;

#if !(defined(__APPLE__) && defined(__MACH__))
            Ui::setDisplayMode(new_mode);
#endif
        }

#pragma mark - Resolution dropdown (Widget 11)

        // 0x004C0026
        static void resolutionMouseDown(Window* w, WidgetIndex_t wi)
        {
            std::vector<Resolution> resolutions = getFullscreenResolutions();

            Widget dropdown = w->widgets[Widx::display_resolution];
            Dropdown::showText2(w->x + dropdown.left, w->y + dropdown.top, dropdown.width(), dropdown.height(), w->getColour(WindowColour::secondary), resolutions.size(), 0x80);

            auto& cfg = Config::getNew();
            for (size_t i = 0; i < resolutions.size(); i++)
            {
                Dropdown::add(i, StringIds::dropdown_stringid, { StringIds::display_resolution_dropdown_format, (uint16_t)resolutions[i].width, (uint16_t)resolutions[i].height });
                if (cfg.display.fullscreenResolution.width == resolutions[i].width && cfg.display.fullscreenResolution.height == resolutions[i].height)
                    Dropdown::setItemSelected((int16_t)i);
            }
        }

        // 0x004C00F4
        static void resolutionDropdown(Window* w, int16_t index)
        {
            if (index == -1)
                return;
            std::vector<Resolution> resolutions = getFullscreenResolutions();
            Ui::setDisplayMode(Config::ScreenMode::fullscreen, { resolutions[index].width, resolutions[index].height });
        }

#pragma mark -

        static void displayScaleMouseDown(Window* w, WidgetIndex_t wi, float adjust_by)
        {
            OpenLoco::Ui::adjustWindowScale(adjust_by);
        }

        // 0x004BFBB7
        static void onMouseDown(Window* w, WidgetIndex_t wi)
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
        static void onDropdown(Window* w, WidgetIndex_t wi, int16_t item_index)
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
        static void onUpdate(Window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->currentTab + 4);
        }

        // 0x004BFA04
        static void prepareDraw(Window* w)
        {
            assert(w->currentTab == Common::tab::display);
            assert(w->widgets == _widgets);

            w->activatedWidgets &= ~Common::tabWidgets;
            w->activatedWidgets |= 1ULL << (w->currentTab + 4);

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
                case Config::ScreenMode::window:
                    screen_mode_string_id = StringIds::options_mode_windowed;
                    break;
                case Config::ScreenMode::fullscreen:
                    screen_mode_string_id = StringIds::options_mode_fullscreen;
                    break;
                case Config::ScreenMode::fullscreenBorderless:
                    screen_mode_string_id = StringIds::options_mode_fullscreen_window;
                    break;
            }
            w->widgets[Widx::screen_mode].text = screen_mode_string_id;

            FormatArguments args = {};
            args.skip(0x10);
            auto& resolution = Config::getNew().display.fullscreenResolution;
            args.push<uint16_t>(resolution.width);
            args.push<uint16_t>(resolution.height);

            if (Config::get().constructionMarker)
                w->widgets[Widx::construction_marker].text = StringIds::translucent;
            else
                w->widgets[Widx::construction_marker].text = StringIds::white;

            static const string_id scale_string_ids[] = {
                StringIds::full_scale,
                StringIds::half_scale,
                StringIds::quarter_scale,
                StringIds::eighth_scale,
            };

            w->widgets[Widx::vehicles_min_scale].text = scale_string_ids[Config::get().vehiclesMinScale];
            w->widgets[Widx::station_names_min_scale].text = scale_string_ids[Config::get().stationNamesMinScale];

            w->activatedWidgets &= ~(1 << Widx::show_fps);
            if (Config::getNew().showFPS)
            {
                w->activatedWidgets |= (1 << Widx::show_fps);
            }

            w->activatedWidgets &= ~(1 << Widx::uncap_fps);
            if (Config::getNew().uncapFPS)
            {
                w->activatedWidgets |= (1 << Widx::uncap_fps);
            }

            w->activatedWidgets &= ~(1 << Widx::landscape_smoothing);
            if ((Config::get().flags & Config::Flags::landscapeSmoothing) == 0)
            {
                w->activatedWidgets |= (1 << Widx::landscape_smoothing);
            }

            w->activatedWidgets &= ~(1 << Widx::gridlines_on_landscape);
            if (Config::get().flags & Config::Flags::gridlinesOnLandscape)
            {
                w->activatedWidgets |= (1 << Widx::gridlines_on_landscape);
            }

            if (Config::getNew().scaleFactor <= OpenLoco::Ui::ScaleFactor::min)
                w->disabledWidgets |= (1 << Widx::display_scale_down_btn);
            else
                w->disabledWidgets &= ~(1 << Widx::display_scale_down_btn);

            if (Config::getNew().scaleFactor >= OpenLoco::Ui::ScaleFactor::max)
                w->disabledWidgets |= (1 << Widx::display_scale_up_btn);
            else
                w->disabledWidgets &= ~(1 << Widx::display_scale_up_btn);

#if !(defined(__APPLE__) && defined(__MACH__))
            screenModeToggleEnabled(w);
#endif

            sub_4C13BE(w);
        }

        // 0x004BFAF9
        static void draw(Window* w, Gfx::Context* context)
        {
            // Draw widgets.
            w->draw(context);

            Common::drawTabs(w, context);

            int16_t x = w->x + 10;
            int16_t y = w->y + Display::_widgets[Display::Widx::screen_mode].top + 1;
            drawString_494B3F(*context, x, y, Colour::black, StringIds::options_screen_mode, nullptr);

            y = w->y + Display::_widgets[Display::Widx::display_resolution].top + 1;
            drawString_494B3F(*context, x, y, Colour::black, StringIds::display_resolution, nullptr);

            y = w->y + Display::_widgets[Display::Widx::construction_marker].top + 1;
            drawString_494B3F(*context, x, y, Colour::black, StringIds::construction_marker, nullptr);

            y = w->y + Display::_widgets[Display::Widx::vehicles_min_scale].top + 1;
            drawString_494B3F(*context, x, y, Colour::black, StringIds::vehicles_min_scale, nullptr);

            y = w->y + Display::_widgets[Display::Widx::station_names_min_scale].top + 1;
            drawString_494B3F(*context, x, y, Colour::black, StringIds::station_names_min_scale, nullptr);

            y = w->y + Display::_widgets[Display::Widx::display_scale].top + 1;
            drawString_494B3F(*context, x, y, Colour::black, StringIds::window_scale_factor, nullptr);

            int scale = (int)(Config::getNew().scaleFactor * 100);
            auto& scale_widget = w->widgets[Widx::display_scale];
            drawString_494B3F(*context, w->x + scale_widget.left + 1, w->y + scale_widget.top + 1, Colour::black, StringIds::scale_formatted, &scale);
        }

        static void applyScreenModeRestrictions(Window* w)
        {
            if (Config::getNew().display.mode != Config::ScreenMode::fullscreen)
                w->disabledWidgets = (1 << Display::Widx::display_resolution) | (1 << Display::Widx::display_resolution_btn);

#if !(defined(__APPLE__) && defined(__MACH__))
            w->enabledWidgets |= (1 << Display::Widx::screen_mode) | (1 << Display::Widx::screen_mode_btn);
            Display::screenModeToggleEnabled(w);
#else
            w->disabledWidgets |= (1 << Display::Widx::screen_mode) | (1 << Display::Widx::screen_mode_btn) | (1 << Display::Widx::display_resolution) | (1 << Display::Widx::display_resolution_btn);
#endif
        }

        static const WindowEventList initEvents()
        {
            _events.onClose = onClose;
            _events.onMouseUp = onMouseUp;
            _events.onMouseDown = onMouseDown;
            _events.onDropdown = onDropdown;
            _events.onUpdate = onUpdate;
            _events.prepareDraw = prepareDraw;
            _events.draw = draw;
            return _events;
        }
    }

    namespace Sound
    {
        static const Ui::Size _window_size = { 366, 84 };

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

        static Widget _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_sound),
            makeDropdownWidgets({ 10, 49 }, { 346, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::stringid),
            makeWidget({ 10, 65 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::play_title_music),
            widgetEnd(),
        };

        static WindowEventList _events;

        static void audioDeviceMouseDown(Ui::Window* window);
        static void audioDeviceDropdown(Ui::Window* window, int16_t itemIndex);
        static void playTitleMusicOnMouseUp(Ui::Window* window);

        // 0x004C0217
        static void prepareDraw(Window* w)
        {
            assert(w->currentTab == Common::tab::sound);
            assert(w->widgets == _widgets);

            w->activatedWidgets &= ~Common::tabWidgets;
            w->activatedWidgets |= 1ULL << (w->currentTab + 4);

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

            if (Config::getNew().audio.playTitleMusic)
                w->activatedWidgets |= (1 << Widx::play_title_music);
            else
                w->activatedWidgets &= ~(1 << Widx::play_title_music);

            sub_4C13BE(w);
        }

        // 0x004C02F5
        static void draw(Window* w, Gfx::Context* context)
        {
            // Draw widgets.
            w->draw(context);

            Common::drawTabs(w, context);
        }

        static void onMouseUp(Window* w, WidgetIndex_t wi)
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

        static void onMouseDown(Window* w, WidgetIndex_t wi)
        {
            switch (wi)
            {
                case Widx::audio_device_btn:
                    audioDeviceMouseDown(w);
                    break;
            }
        }

        static void onDropdown(Ui::Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
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
        static void audioDeviceMouseDown(Ui::Window* w)
        {
            const auto& devices = Audio::getDevices();
            if (devices.size() != 0)
            {
                Widget dropdown = w->widgets[Widx::audio_device];
                Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), devices.size(), 0x80);
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
        static void audioDeviceDropdown(Ui::Window* w, int16_t itemIndex)
        {
            if (itemIndex != -1)
            {
                Audio::setDevice(itemIndex);
                WindowManager::invalidateWidget(w->type, w->number, Widx::audio_device);
            }
        }

#pragma mark -

        static void playTitleMusicOnMouseUp(Window* w)
        {
            auto& cfg = Config::getNew();
            cfg.audio.playTitleMusic = !cfg.audio.playTitleMusic;
            Config::write();

            Audio::playTitleScreenMusic();

            w->invalidate();
        }

        // 0x004C04E0
        static void onUpdate(Window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->currentTab + 4);
        }

        static void initEvents()
        {
            _events.onClose = onClose;
            _events.onMouseUp = onMouseUp;
            _events.onMouseDown = onMouseDown;
            _events.onDropdown = onDropdown;
            _events.onUpdate = onUpdate;
            _events.prepareDraw = prepareDraw;
            _events.draw = draw;
        }
    }

    namespace Music
    {
        static const Ui::Size _window_size = { 366, 129 };

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

        static Widget _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_music),
            makeDropdownWidgets({ 160, 49 }, { 196, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::stringid),
            makeWidget({ 10, 64 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::music_controls_stop, StringIds::music_controls_stop_tip),
            makeWidget({ 34, 64 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::music_controls_play, StringIds::music_controls_play_tip),
            makeWidget({ 58, 64 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::music_controls_next, StringIds::music_controls_next_tip),
            makeWidget({ 256, 64 }, { 109, 24 }, WidgetType::wt_5, WindowColour::secondary, -1, StringIds::set_volume_tip),
            makeDropdownWidgets({ 10, 93 }, { 346, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::arg2_stringid),
            makeWidget({ 183, 108 }, { 173, 12 }, WidgetType::button, WindowColour::secondary, StringIds::edit_music_selection, StringIds::edit_music_selection_tip),
            widgetEnd(),
        };

        static void volumeMouseDown(Window* w);
        static void stopMusic(Window* w);
        static void playMusic(Window* w);
        static void playNextSong(Window* w);
        static void musicPlaylistMouseDown(Window* w);
        static void musicPlaylistDropdown(Window* w, int16_t ax);
        static void currentlyPlayingMouseDown(Window* w);
        static void currentlyPlayingDropdown(Window* w, int16_t ax);

        static WindowEventList _events;

        static void prepareDraw(Window* w)
        {
            assert(w->currentTab == Common::tab::music);
            assert(w->widgets == _widgets);

            w->activatedWidgets &= ~Common::tabWidgets;
            w->activatedWidgets |= 1ULL << (w->currentTab + 4);

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
                songName = Audio::getMusicInfo(_currentSong)->titleId;
            }

            FormatArguments args = {};
            args.push(songName);

            static const string_id playlist_string_ids[] = {
                StringIds::play_only_music_from_current_era,
                StringIds::play_all_music,
                StringIds::play_custom_music_selection,
            };

            string_id currentSongStringId = playlist_string_ids[(uint8_t)Config::get().musicPlaylist];
            args.push(currentSongStringId);

            w->activatedWidgets &= ~((1 << Widx::music_controls_stop) | (1 << Widx::music_controls_play));
            w->activatedWidgets |= (1 << Widx::music_controls_stop);
            if (_currentSong != -1)
            {
                if (Config::get().musicPlaying)
                {
                    w->activatedWidgets &= ~((1 << Widx::music_controls_stop) | (1 << Widx::music_controls_play));
                    w->activatedWidgets |= (1 << Widx::music_controls_play);
                }
            }

            w->disabledWidgets |= (1 << Widx::edit_selection);
            if (Config::get().musicPlaylist == Config::MusicPlaylistType::custom)
            {
                w->disabledWidgets &= ~(1 << Widx::edit_selection);
            }

            sub_4C13BE(w);
        }

        // 0x004C05F9
        static void draw(Window* w, Gfx::Context* context)
        {
            // Draw widgets.
            w->draw(context);

            Common::drawTabs(w, context);

            Gfx::drawString_494B3F(*context, w->x + 10, w->y + w->widgets[Widx::currently_playing_btn].top, Colour::black, StringIds::currently_playing, nullptr);

            Gfx::drawString_494B3F(*context, w->x + 183, w->y + w->widgets[Widx::volume].top + 7, Colour::black, StringIds::volume, nullptr);

            Gfx::drawImage(context, w->x + w->widgets[Widx::volume].left, w->y + w->widgets[Widx::volume].top, Gfx::recolour(ImageIds::volume_slider_track, w->getColour(WindowColour::secondary).c()));

            int16_t x = 90 + (Config::get().volume / 32);
            Gfx::drawImage(context, w->x + w->widgets[Widx::volume].left + x, w->y + w->widgets[Widx::volume].top, Gfx::recolour(ImageIds::volume_slider_thumb, w->getColour(WindowColour::secondary).c()));
        }

        static void onMouseUp(Window* w, WidgetIndex_t wi)
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
                    MusicSelection::open();
                    return;
            }
        }

        // 0x004C06F2
        static void onMouseDown(Window* w, WidgetIndex_t wi)
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
        static void onDropdown(Ui::Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
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
        static void volumeMouseDown(Window* w)
        {
            _clickRepeatTicks = 31;

            int x = _5233A4 - w->x - w->widgets[Widx::volume].left - 10;
            x = std::clamp(x, 0, 80);

            Audio::setBgmVolume((x * 32) - 2560);

            w->invalidate();
        }

        // 0x004C0778
        static void stopMusic(Window* w)
        {
            if (Config::get().musicPlaying == 0)
                return;

            auto& cfg = Config::get();
            cfg.musicPlaying = 0;
            Config::write();

            Audio::stopBackgroundMusic();

            _currentSong = -1;

            w->invalidate();
        }

        // 0x004C07A4
        static void playMusic(Window* w)
        {
            if (Config::get().musicPlaying != 0)
                return;

            auto& cfg = Config::get();
            cfg.musicPlaying = 1;
            Config::write();

            w->invalidate();
        }

        // 0x004C07C4
        static void playNextSong(Window* w)
        {
            if (Config::get().musicPlaying == 0)
                return;

            Audio::stopBackgroundMusic();

            _currentSong = -1;

            w->invalidate();
        }

#pragma mark - Widget 17

        // 0x004C07E4
        static void musicPlaylistMouseDown(Window* w)
        {
            Widget dropdown = w->widgets[Widx::music_playlist];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::play_only_music_from_current_era);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::play_all_music);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::play_custom_music_selection);

            Dropdown::setItemSelected((uint8_t)Config::get().musicPlaylist);
        }

        // 0x004C084A
        static void musicPlaylistDropdown(Window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = OpenLoco::Config::get();
            cfg.musicPlaylist = (Config::MusicPlaylistType)ax;
            Config::write();

            w->invalidate();

            Audio::revalidateCurrentTrack();

            WindowManager::close(WindowType::musicSelection);
        }

#pragma mark - Widget 11

        static std::vector<int> get_available_tracks()
        {
            auto vector = std::vector<int>();

            if (Config::get().musicPlaylist == Config::MusicPlaylistType::currentEra)
            {
                uint16_t year = getCurrentYear();
                for (int i = 0; i < Audio::kNumMusicTracks; i++)
                {
                    auto info = Audio::getMusicInfo(i);
                    if (year >= info->startYear && year <= info->endYear)
                    {
                        vector.push_back(i);
                    }
                }
            }
            else if (Config::get().musicPlaylist == Config::MusicPlaylistType::all)
            {
                for (int i = 0; i < Audio::kNumMusicTracks; i++)
                {
                    vector.push_back(i);
                }
            }
            else if (Config::get().musicPlaylist == Config::MusicPlaylistType::custom)
            {
                for (int i = 0; i < Audio::kNumMusicTracks; i++)
                {
                    if (Config::get().enabledMusic[i] & 1)
                    {
                        vector.push_back(i);
                    }
                }

                if (vector.size() == 0)
                {
                    for (int i = 0; i < Audio::kNumMusicTracks; i++)
                    {
                        vector.push_back(i);
                    }
                }
            }

            return vector;
        }

        // 0x004C0875
        static void currentlyPlayingMouseDown(Window* w)
        {
            auto tracks = get_available_tracks();

            Widget dropdown = w->widgets[Widx::currently_playing];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), tracks.size(), 0x80);

            int index = -1;
            for (auto track : tracks)
            {
                index++;
                Dropdown::add(index, StringIds::dropdown_stringid, Audio::getMusicInfo(track)->titleId);
                if (track == _currentSong)
                {
                    Dropdown::setItemSelected(index);
                }
            }
        }

        // 0x004C09F8
        static void currentlyPlayingDropdown(Window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto tracks = get_available_tracks();
            int track = tracks.at(ax);
            if (track == _currentSong)
                return;

            Audio::stopBackgroundMusic();

            _currentSong = track;
            _lastSong = track;
            _songProgress = 0;

            w->invalidate();
        }

        // 0x004C0A37
        static void onUpdate(Window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->currentTab + 4);
        }

        static void initEvents()
        {
            _events.onClose = onClose;
            _events.onMouseUp = onMouseUp;
            _events.onMouseDown = onMouseDown;
            _events.onDropdown = onDropdown;
            _events.onUpdate = onUpdate;
            _events.prepareDraw = prepareDraw;
            _events.draw = draw;
        }
    }

    namespace Regional
    {
        static const Ui::Size _window_size = { 366, 167 };

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

        static Widget _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_regional),
            makeDropdownWidgets({ 183, 49 }, { 173, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::stringptr),
            makeDropdownWidgets({ 183, 69 }, { 173, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::arg12_stringid),
            makeDropdownWidgets({ 183, 84 }, { 173, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::arg6_stringid),
            makeDropdownWidgets({ 183, 104 }, { 173, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::arg10_stringid, StringIds::current_game_currency_tip),
            makeDropdownWidgets({ 183, 119 }, { 173, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::preferred_currency_buffer, StringIds::new_game_currency_tip),
            makeWidget({ 10, 134 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::use_preferred_currency_new_game, StringIds::use_preferred_currency_new_game_tip),
            makeWidget({ 10, 148 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::use_preferred_currency_always, StringIds::use_preferred_currency_always_tip),
            widgetEnd(),
        };

        static void languageMouseDown(Window* w);
        static void languageDropdown(Window* w, int16_t ax);
        static void currencyMouseDown(Window* w);
        static void currencyDropdown(Window* w, int16_t ax);
        static void preferredCurrencyMouseDown(Window* w);
        static void preferredCurrencyDropdown(Window* w, int16_t ax);
        static void preferredCurrencyNewGameMouseUp(Window* w);
        static void preferredCurrencyAlwaysMouseUp(Window* w);
        static void distanceSpeedMouseDown(Window* w);
        static void distanceSpeedDropdown(Window* w, int16_t ax);
        static void heightsLabelsMouseDown(Window* w);
        static void heightsLabelsDropdown(Window* w, int16_t ax);

        static WindowEventList _events;

        // 0x004C0A59
        static void prepareDraw(Window* w)
        {
            assert(w->currentTab == Common::tab::regional);
            assert(w->widgets == _widgets);

            w->activatedWidgets &= ~Common::tabWidgets;
            w->activatedWidgets |= 1ULL << (w->currentTab + 4);

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
            if ((OpenLoco::Config::get().flags & Config::Flags::showHeightAsUnits) == 0)
            {
                current_height_units = StringIds::height_real_values;
            }

            args.skip(0x2);
            args.push(current_height_units);

            string_id current_measurement_format = StringIds::imperial;
            if (OpenLoco::Config::get().measurementFormat == Config::MeasurementFormat::metric)
            {
                current_measurement_format = StringIds::metric;
            }

            args.skip(0x2);
            args.push(ObjectManager::get<CurrencyObject>()->name);
            args.push(current_measurement_format);

            w->activatedWidgets &= ~(1 << Widx::preferred_currency_for_new_games);
            if (Config::get().flags & Config::Flags::preferredCurrencyForNewGames)
            {
                w->activatedWidgets |= (1 << Widx::preferred_currency_for_new_games);
            }

            w->activatedWidgets &= ~(1 << Widx::preferred_currency_always);
            if (Config::get().flags & Config::Flags::preferredCurrencyAlways)
            {
                w->activatedWidgets |= (1 << Widx::preferred_currency_always);
            }

            w->disabledWidgets &= ~(1 << Widx::currency);
            w->disabledWidgets &= ~(1 << Widx::currency_btn);
            if (Config::get().flags & Config::Flags::preferredCurrencyAlways)
            {
                w->disabledWidgets |= (1 << Widx::currency);
                w->disabledWidgets |= (1 << Widx::currency_btn);
            }

            sub_4C13BE(w);
        }

        // 0x004C0B5B
        static void draw(Window* w, Gfx::Context* context)
        {
            // Draw widgets.
            w->draw(context);
            Common::drawTabs(w, context);

            Gfx::drawString_494B3F(*context, w->x + 10, w->y + w->widgets[Widx::language].top + 1, Colour::black, StringIds::options_language, nullptr);
            Gfx::drawString_494B3F(*context, w->x + 10, w->y + w->widgets[Widx::distance_speed].top + 1, Colour::black, StringIds::distance_and_speed, nullptr);
            Gfx::drawString_494B3F(*context, w->x + 10, w->y + w->widgets[Widx::heights].top + 1, Colour::black, StringIds::heights, nullptr);
            Gfx::drawString_494B3F(*context, w->x + 10, w->y + w->widgets[Widx::currency].top + 1, Colour::black, StringIds::current_game_currency, nullptr);
            Gfx::drawString_494B3F(*context, w->x + 10, w->y + w->widgets[Widx::preferred_currency].top + 1, Colour::black, StringIds::new_game_currency, nullptr);
        }

        static void onMouseUp(Window* w, WidgetIndex_t wi)
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
        static void onMouseDown(Window* w, WidgetIndex_t wi)
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
        static void onDropdown(Ui::Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
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

        static void languageMouseDown(Window* w)
        {
            auto& lds = Localisation::getLanguageDescriptors();
            uint8_t num_languages = static_cast<uint8_t>(lds.size());

            Widget dropdown = w->widgets[Widx::language];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), num_languages - 1, 0x80);

            std::string& current_language = Config::getNew().language;

            for (uint8_t index = 1; index < num_languages; index++)
            {
                auto& ld = lds[index];
                Dropdown::add(index - 1, StringIds::dropdown_stringptr, (char*)ld.native_name.c_str());

                if (ld.locale == current_language)
                    Dropdown::setItemSelected(index - 1);
            }
        }

        static void languageDropdown(Window* w, int16_t ax)
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
        static void currencyMouseDown(Window* w)
        {
            uint8_t* _11364A0 = (uint8_t*)*__11364A0;

            Widget dropdown = w->widgets[Widx::currency];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), _112C185, 0x80);
            int index = -1;
            for (auto object : ObjectManager::getAvailableObjects(ObjectType::currency))
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
        static void currencyDropdown(Window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            uint8_t* _11364A0 = (uint8_t*)*__11364A0;

            int index = -1;
            for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::currency))
            {
                index++;
                if (index == ax)
                {
                    auto ebp = ObjectManager::getActiveObject(ObjectType::currency, _11364A0);

                    if (ebp.index != -1)
                    {
                        ObjectManager::unload(*ebp.object._header);
                    }

                    ObjectManager::load(*object.second._header);
                    ObjectManager::reloadAll();
                    call(0x0046E07B); // load currency gfx
                    sub_4BF935();

                    break;
                }
            }

            w->invalidate();
        }

        // 0x004C0DCF
        static void preferredCurrencyMouseDown(Window* w)
        {
            Widget dropdown = w->widgets[Widx::preferred_currency];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), _112C185, 0x80);

            int index = -1;
            for (auto object : ObjectManager::getAvailableObjects(ObjectType::currency))
            {
                index++;
                Dropdown::add(index, StringIds::dropdown_stringptr, object.second._name);

                // TODO: Mark current value as selected
            }
        }

        // 0x004C0E82
        static void preferredCurrencyDropdown(Window* w, int16_t ax)
        {
            if (ax == -1)
            {
                w->invalidate();
                return;
            }

            int index = -1;
            for (const auto& object : ObjectManager::getAvailableObjects(ObjectType::currency))
            {
                index++;

                if (index == ax)
                {
                    auto& cfg = OpenLoco::Config::get();
                    cfg.preferredCurrency = *object.second._header;

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
        static void preferredCurrencyNewGameMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            if (cfg.flags & Config::Flags::preferredCurrencyForNewGames)
            {
                cfg.flags &= ~Config::Flags::preferredCurrencyForNewGames;
            }
            else
            {
                cfg.flags |= Config::Flags::preferredCurrencyForNewGames;
            }
            Config::write();

            w->invalidate();
        }

        // 0x004C0F27
        static void preferredCurrencyAlwaysMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            if (cfg.flags & Config::Flags::preferredCurrencyAlways)
            {
                cfg.flags &= ~Config::Flags::preferredCurrencyAlways;
            }
            else
            {
                cfg.flags |= Config::Flags::preferredCurrencyAlways;
            }
            Config::write();

            call(0x004C153B);
            sub_4BF935();

            w->invalidate();
        }

        // 0x004C0F49
        static void distanceSpeedMouseDown(Window* w)
        {
            Widget dropdown = w->widgets[Widx::distance_speed];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::imperial);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::metric);
            Dropdown::setItemSelected(static_cast<uint8_t>(Config::get().measurementFormat));
        }

        // 0x004C0FB3
        static void distanceSpeedDropdown(Window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = Config::get();
            cfg.measurementFormat = Config::MeasurementFormat(ax);

            // 0x004C0FC2
            cfg.heightMarkerOffset = 0;
            if ((cfg.flags & Config::Flags::showHeightAsUnits) == 0)
            {
                cfg.heightMarkerOffset = cfg.measurementFormat == Config::MeasurementFormat::imperial ? 0x100 : 0x200;
            }

            Config::write();
            Gfx::invalidateScreen();
        }

        // 0x004C0FFA
        static void heightsLabelsMouseDown(Window* w)
        {
            Widget dropdown = w->widgets[Widx::heights];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::height_units);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::height_real_values);

            int selectedItem = 0;
            if ((Config::get().flags & Config::Flags::showHeightAsUnits) == 0)
            {
                selectedItem = 1;
            }
            Dropdown::setItemSelected(selectedItem);
        }

        // 0x004C106C
        static void heightsLabelsDropdown(Window* w, int16_t ax)
        {
            if (ax == -1)
                return;

            auto& cfg = Config::get();
            cfg.flags &= ~Config::Flags::showHeightAsUnits;

            if (ax == 0)
            {
                cfg.flags |= Config::Flags::showHeightAsUnits;
            }

            // 0x004C0FC2
            cfg.heightMarkerOffset = 0;
            if ((cfg.flags & Config::Flags::showHeightAsUnits) == 0)
            {
                cfg.heightMarkerOffset = cfg.measurementFormat == Config::MeasurementFormat::imperial ? 0x100 : 0x200;
            }

            Config::write();
            Gfx::invalidateScreen();
        }

        // 0x004C1195
        static void onUpdate(Window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->currentTab + 4);
        }

        static void initEvents()
        {
            _events.onClose = onClose;
            _events.onMouseUp = onMouseUp;
            _events.onMouseDown = onMouseDown;
            _events.onDropdown = onDropdown;
            _events.onUpdate = onUpdate;
            _events.prepareDraw = prepareDraw;
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

        static const Ui::Size _window_size = { 366, 99 };

        static Widget _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_controls),
            makeWidget({ 10, 49 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::scroll_screen_edge, StringIds::scroll_screen_edge_tip),
            makeWidget({ 10, 64 }, { 346, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::zoom_to_cursor, StringIds::zoom_to_cursor_tip),
            makeWidget({ 26, 79 }, { 160, 12 }, WidgetType::button, WindowColour::secondary, StringIds::customise_keys, StringIds::customise_keys_tip),
            widgetEnd(),
        };

        static void edgeScrollingMouseUp(Window* w);
        static void zoomToCursorMouseUp(Window* w);
        static void openKeyboardShortcuts();

        static WindowEventList _events;

        static void prepareDraw(Window* w)
        {
            assert(w->currentTab == Common::tab::controls);
            assert(w->widgets == _widgets);

            w->activatedWidgets &= ~Common::tabWidgets;
            w->activatedWidgets |= 1ULL << (w->currentTab + 4);

            w->widgets[Common::Widx::frame].right = w->width - 1;
            w->widgets[Common::Widx::frame].bottom = w->height - 1;
            w->widgets[Common::Widx::panel].right = w->width - 1;
            w->widgets[Common::Widx::panel].bottom = w->height - 1;
            w->widgets[Common::Widx::caption].right = w->width - 2;
            w->widgets[Common::Widx::close_button].left = w->width - 15;
            w->widgets[Common::Widx::close_button].right = w->width - 15 + 12;

            w->activatedWidgets &= ~(1 << Widx::edge_scrolling | 1 << Widx::zoom_to_cursor);
            if (Config::get().edgeScrolling)
            {
                w->activatedWidgets |= (1 << Widx::edge_scrolling);
            }
            if (Config::getNew().zoomToCursor)
            {
                w->activatedWidgets |= (1 << Widx::zoom_to_cursor);
            }

            sub_4C13BE(w);
        }

        // 0x004C113F
        static void draw(Window* w, Gfx::Context* context)
        {
            w->draw(context);
            Common::drawTabs(w, context);
        }

        // 0x004C114A
        static void onMouseUp(Window* w, WidgetIndex_t wi)
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
        static void edgeScrollingMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.edgeScrolling = !cfg.edgeScrolling;
            Config::write();

            w->invalidate();
        }

        static void zoomToCursorMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::getNew();
            cfg.zoomToCursor = !cfg.zoomToCursor;
            Config::write();

            w->invalidate();
        }

        // 0x004C118D
        static void openKeyboardShortcuts()
        {
            KeyboardShortcuts::open();
        }

        // 0x004C1195
        static void onUpdate(Window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->currentTab + 4);
        }

        static void initEvents()
        {
            _events.onClose = onClose;
            _events.onMouseUp = onMouseUp;
            _events.onUpdate = onUpdate;
            _events.prepareDraw = prepareDraw;
            _events.draw = draw;
        }
    }

    namespace Misc
    {
        static const Ui::Size _window_size = { 420, 189 };

        namespace Widx
        {
            enum
            {
                enableCheatsToolbarButton = 10,
                disable_vehicle_breakdowns,
                trainsReverseAtSignals,
                disableAICompanies,
                use_preferred_owner_name,
                change_btn,
                export_plugin_objects,
                autosave_frequency,
                autosave_frequency_btn,
                autosave_amount,
                autosave_amount_down_btn,
                autosave_amount_up_btn,
            };
        }

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << Misc::Widx::enableCheatsToolbarButton) | (1 << Misc::Widx::disable_vehicle_breakdowns) | (1 << Widx::trainsReverseAtSignals) | (1 << Widx::disableAICompanies) | (1 << Misc::Widx::use_preferred_owner_name) | (1 << Misc::Widx::change_btn) | (1 << Misc::Widx::export_plugin_objects) | (1 << Misc::Widx::autosave_frequency_btn) | (1 << Misc::Widx::autosave_amount) | (1 << Misc::Widx::autosave_amount_down_btn) | (1 << Misc::Widx::autosave_amount_up_btn);

        static Widget _widgets[] = {
            common_options_widgets(_window_size, StringIds::options_title_miscellaneous),
            makeWidget({ 10, 49 }, { 400, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::option_cheat_menu_enable, StringIds::tooltip_option_cheat_menu_enable),
            makeWidget({ 10, 64 }, { 400, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::disable_vehicle_breakdowns),
            makeWidget({ 10, 79 }, { 400, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::trainsReverseAtSignals),
            makeWidget({ 10, 94 }, { 400, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::disableAICompanies, StringIds::disableAICompanies_tip),
            makeWidget({ 10, 109 }, { 400, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::use_preferred_owner_name, StringIds::use_preferred_owner_name_tip),
            makeWidget({ 335, 124 }, { 75, 12 }, WidgetType::button, WindowColour::secondary, StringIds::change),
            makeWidget({ 10, 139 }, { 400, 12 }, WidgetType::checkbox, WindowColour::secondary, StringIds::export_plugin_objects, StringIds::export_plugin_objects_tip),
            makeDropdownWidgets({ 250, 154 }, { 156, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty),
            makeStepperWidgets({ 250, 169 }, { 156, 12 }, WidgetType::textbox, WindowColour::secondary, StringIds::empty),
            widgetEnd(),
        };

        static WindowEventList _events;

        static loco_global<uint8_t, 0x0112A17E> _112A17E;

        static void changePreferredName(Window* w);
        static void setPreferredName(Window* w, const char* str);
        static void usePreferredOwnerNameMouseUp(Window* w);
        static void enableCheatsToolbarButtonMouseUp(Window* w);
        static void disableVehicleBreakdownsMouseUp(Window* w);
        static void trainsReverseAtSignalsMouseUp(Window* w);
        static void disableAICompaniesMouseUp(Window* w);
        static void exportPluginObjectsMouseUp(Window* w);

        // 0x004C11B7
        static void prepareDraw(Window* w)
        {
            assert(w->currentTab == Common::tab::miscellaneous);
            assert(w->widgets == _widgets);

            w->activatedWidgets &= ~Common::tabWidgets;
            w->activatedWidgets |= 1ULL << (w->currentTab + 4);

            w->widgets[Common::Widx::frame].right = w->width - 1;
            w->widgets[Common::Widx::frame].bottom = w->height - 1;
            w->widgets[Common::Widx::panel].right = w->width - 1;
            w->widgets[Common::Widx::panel].bottom = w->height - 1;
            w->widgets[Common::Widx::caption].right = w->width - 2;
            w->widgets[Common::Widx::close_button].left = w->width - 15;
            w->widgets[Common::Widx::close_button].right = w->width - 15 + 12;

            if (Config::getNew().cheatsMenuEnabled)
                w->activatedWidgets |= (1 << Widx::enableCheatsToolbarButton);
            else
                w->activatedWidgets &= ~(1 << Widx::enableCheatsToolbarButton);

            if (Config::getNew().breakdownsDisabled)
                w->activatedWidgets |= (1 << Widx::disable_vehicle_breakdowns);
            else
                w->activatedWidgets &= ~(1 << Widx::disable_vehicle_breakdowns);

            if (Config::getNew().trainsReverseAtSignals)
                w->activatedWidgets |= (1 << Widx::trainsReverseAtSignals);
            else
                w->activatedWidgets &= ~(1 << Widx::trainsReverseAtSignals);

            if (Config::getNew().companyAIDisabled)
                w->activatedWidgets |= (1 << Widx::disableAICompanies);
            else
                w->activatedWidgets &= ~(1 << Widx::disableAICompanies);

            w->activatedWidgets &= ~(1 << Widx::export_plugin_objects);
            if (Config::get().flags & Config::Flags::exportObjectsWithSaves)
            {
                w->activatedWidgets |= (1 << Widx::export_plugin_objects);
            }

            w->activatedWidgets &= ~(1 << Widx::use_preferred_owner_name);
            w->disabledWidgets |= (1 << Widx::change_btn);
            if (Config::get().flags & Config::Flags::usePreferredOwnerName)
            {
                w->activatedWidgets |= (1 << Widx::use_preferred_owner_name);
                w->disabledWidgets &= ~(1 << Widx::change_btn);
            }

            w->widgets[Widx::export_plugin_objects].type = WidgetType::none;
            if (_112A17E)
            {
                w->widgets[Widx::export_plugin_objects].type = WidgetType::checkbox;
            }

            sub_4C13BE(w);
        }

        static void drawDropdownContent(Window* w, Gfx::Context* context, WidgetIndex_t widgetIndex, string_id stringId, int32_t value)
        {
            auto& widget = w->widgets[widgetIndex];
            FormatArguments args = {};
            args.push(stringId);
            args.push(value);
            drawString_494B3F(*context, w->x + widget.left + 1, w->y + widget.top + 1, Colour::black, StringIds::black_stringid, &args);
        }

        // 0x004C1282
        static void draw(Window* w, Gfx::Context* context)
        {
            w->draw(context);
            Common::drawTabs(w, context);

            auto buffer = (char*)StringManager::getString(StringIds::buffer_2039);
            char* playerName = Config::get().preferredName;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            FormatArguments args = {};
            args.push(StringIds::buffer_2039);
            Gfx::drawString_494B3F(*context, w->x + 10, w->y + w->widgets[Widx::change_btn].top + 1, Colour::black, StringIds::wcolour2_preferred_owner_name, &args);

            auto y = w->y + w->widgets[Widx::autosave_frequency].top + 1;
            drawString_494B3F(*context, w->x + 10, y, Colour::black, StringIds::autosave_frequency, nullptr);

            auto freq = Config::getNew().autosaveFrequency;
            string_id stringId;
            switch (freq)
            {
                case 0:
                    stringId = StringIds::autosave_never;
                    break;
                case 1:
                    stringId = StringIds::autosave_every_month;
                    break;
                default:
                    stringId = StringIds::autosave_every_x_months;
                    break;
            }
            drawDropdownContent(w, context, Widx::autosave_frequency, stringId, freq);

            y = w->y + w->widgets[Widx::autosave_amount].top + 1;
            drawString_494B3F(*context, w->x + 10, y, Colour::black, StringIds::autosave_amount, nullptr);

            auto scale = Config::getNew().autosaveAmount;
            drawDropdownContent(w, context, Widx::autosave_amount, StringIds::int_32, scale);
        }

        static void changeAutosaveAmount(Window* w, int32_t delta)
        {
            auto& cfg = Config::getNew();
            auto newValue = std::clamp(cfg.autosaveAmount + delta, 1, 24);
            if (cfg.autosaveAmount != newValue)
            {
                cfg.autosaveAmount = newValue;
                Config::writeNewConfig();
                w->invalidate();
            }
        }

        static void changeAutosaveFrequency(Window* w, int32_t value)
        {
            auto& cfg = Config::getNew();
            if (cfg.autosaveFrequency != value)
            {
                cfg.autosaveFrequency = value;
                Config::writeNewConfig();
                w->invalidate();
            }
        }

        static void showAutosaveFrequencyDropdown(Window* w, WidgetIndex_t wi)
        {
            auto dropdown = w->widgets[wi];
            Dropdown::show(w->x + dropdown.left, w->y + dropdown.top, dropdown.width() - 4, dropdown.height(), w->getColour(WindowColour::secondary), 5, 0x80);

            // Add pre-defined entries
            Dropdown::add(0, StringIds::dropdown_stringid, { StringIds::autosave_never });
            Dropdown::add(1, StringIds::dropdown_stringid, { StringIds::autosave_every_month });
            Dropdown::add(2, StringIds::dropdown_stringid, { StringIds::autosave_every_x_months, static_cast<uint32_t>(3) });
            Dropdown::add(3, StringIds::dropdown_stringid, { StringIds::autosave_every_x_months, static_cast<uint32_t>(6) });
            Dropdown::add(4, StringIds::dropdown_stringid, { StringIds::autosave_every_x_months, static_cast<uint32_t>(12) });

            // Set current selection
            auto freq = Config::getNew().autosaveFrequency;
            std::optional<size_t> selected;
            switch (freq)
            {
                case 0:
                    selected = 0;
                    break;
                case 1:
                    selected = 1;
                    break;
                case 3:
                    selected = 2;
                    break;
                case 6:
                    selected = 3;
                    break;
                case 12:
                    selected = 4;
                    break;
            }
            if (selected)
            {
                Dropdown::setItemSelected(*selected);
            }
        }

        static void handleAutosaveFrequencyDropdown(Window* w, int32_t index)
        {
            switch (index)
            {
                case 0:
                    changeAutosaveFrequency(w, 0);
                    break;
                case 1:
                    changeAutosaveFrequency(w, 1);
                    break;
                case 2:
                    changeAutosaveFrequency(w, 3);
                    break;
                case 3:
                    changeAutosaveFrequency(w, 6);
                    break;
                case 4:
                    changeAutosaveFrequency(w, 12);
                    break;
            }
        }

        // 0x004C12D2
        static void onMouseUp(Window* w, WidgetIndex_t wi)
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

                case Widx::enableCheatsToolbarButton:
                    enableCheatsToolbarButtonMouseUp(w);
                    break;

                case Widx::disable_vehicle_breakdowns:
                    disableVehicleBreakdownsMouseUp(w);
                    break;

                case Widx::trainsReverseAtSignals:
                    trainsReverseAtSignalsMouseUp(w);
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

        static void onMouseDown(Window* w, WidgetIndex_t wi)
        {
            switch (wi)
            {
                case Widx::autosave_frequency_btn:
                    showAutosaveFrequencyDropdown(w, Widx::autosave_frequency);
                    break;
                case Widx::autosave_amount_down_btn:
                    changeAutosaveAmount(w, -1);
                    break;
                case Widx::autosave_amount_up_btn:
                    changeAutosaveAmount(w, 1);
                    break;
            }
        }

        static void onDropdown(Window* w, WidgetIndex_t wi, int16_t item_index)
        {
            switch (wi)
            {
                case Widx::autosave_frequency_btn:
                    handleAutosaveFrequencyDropdown(w, item_index);
                    break;
            }
        }

        // 0x004C1304
        static void textInput(Window* w, WidgetIndex_t i, const char* str)
        {
            switch (i)
            {
                case Widx::use_preferred_owner_name:
                    setPreferredName(w, str);
                    break;
            }
        }

        // 0x004C1319
        static void changePreferredName(Window* w)
        {
            auto buffer = (char*)StringManager::getString(StringIds::buffer_2039);
            char* playerName = Config::get().preferredName;
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            TextInput::openTextInput(w, StringIds::preferred_owner_name, StringIds::enter_preferred_owner_name, StringIds::buffer_2039, Widx::use_preferred_owner_name, nullptr);
        }

        // 0x004C1342
        static void setPreferredName(Window* w, const char* str)
        {
            auto& cfg = OpenLoco::Config::get();

            if (strlen(str) == 0)
                cfg.flags &= ~Config::Flags::usePreferredOwnerName;

            strcpy(cfg.preferredName, str);
            cfg.preferredName[strlen(str)] = '\0';

            Config::write();

            w->invalidate();
        }

        // 0x004C135F
        static void usePreferredOwnerNameMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            if (cfg.flags & Config::Flags::usePreferredOwnerName)
            {
                cfg.flags &= ~Config::Flags::usePreferredOwnerName;
            }
            else
            {
                cfg.flags |= Config::Flags::usePreferredOwnerName;
            }
            Config::write();

            w->invalidate();

            if (cfg.flags & Config::Flags::usePreferredOwnerName)
            {
                if (strlen(cfg.preferredName) == 0)
                {
                    changePreferredName(w);
                }
            }
        }

        static void enableCheatsToolbarButtonMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::getNew();
            cfg.cheatsMenuEnabled = !cfg.cheatsMenuEnabled;
            Config::write();
            w->invalidate();
            WindowManager::invalidate(WindowType::topToolbar);
        }

        static void disableVehicleBreakdownsMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::getNew();
            cfg.breakdownsDisabled = !cfg.breakdownsDisabled;
            Config::write();
            w->invalidate();
        }

        static void trainsReverseAtSignalsMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::getNew();
            cfg.trainsReverseAtSignals = !cfg.trainsReverseAtSignals;
            Config::write();
            w->invalidate();
        }

        static void disableAICompaniesMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::getNew();
            cfg.companyAIDisabled = !cfg.companyAIDisabled;
            Config::write();
            w->invalidate();
        }

        static void exportPluginObjectsMouseUp(Window* w)
        {
            auto& cfg = OpenLoco::Config::get();
            if (cfg.flags & Config::Flags::exportObjectsWithSaves)
            {
                cfg.flags &= ~Config::Flags::exportObjectsWithSaves;
            }
            else
            {
                cfg.flags |= Config::Flags::exportObjectsWithSaves;
            }
            Config::write();

            w->invalidate();
        }

        // 0x004C139C
        static void onUpdate(Window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, w->currentTab + 4);
        }

        static void initEvents()
        {
            _events.onClose = onClose;
            _events.onMouseUp = onMouseUp;
            _events.onMouseDown = onMouseDown;
            _events.onDropdown = onDropdown;
            _events.onUpdate = onUpdate;
            _events.textInput = textInput;
            _events.prepareDraw = prepareDraw;
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

    static void sub_4C13BE(Window* w)
    {
        w->disabledWidgets &= ~((1 << Common::Widx::tab_music) | (1 << Common::Widx::tab_regional));
        if (isEditorMode() || isTitleMode())
        {
            w->disabledWidgets |= 1 << Common::Widx::tab_music;
        }

        if (isEditorMode() && S5::getOptions().editorStep == EditorController::Step::objectSelection)
        {
            w->disabledWidgets |= 1 << Common::Widx::tab_regional;
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
    Window* open()
    {
        Window* window;

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
        window->currentTab = 0;
        window->frame_no = 0;
        window->rowHover = -1;

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->colour_0B);
        window->setColour(WindowColour::secondary, interface->colour_10);

        sub_4BF8CD();
        sub_4C1519();

        window->enabledWidgets = Display::enabledWidgets;
        Display::applyScreenModeRestrictions(window);

        window->holdableWidgets = 0;
        window->eventHandlers = &Display::_events;
        window->activatedWidgets = 0;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        return window;
    }

    // 0x004BF823
    Window* openMusicSettings()
    {
        auto window = open();

        window->callOnMouseUp(Common::Widx::tab_music);

        return window;
    }

    struct TabInformation
    {
        Widget* widgets;
        WindowEventList* events;
        Ui::Size windowSize;
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
    static void tabOnMouseUp(Window* w, WidgetIndex_t wi)
    {
        Input::toolCancel(w->type, w->number);

        TextInput::sub_4CE6C9(w->type, w->number);
        w->currentTab = wi - Common::Widx::tab_display;
        w->frame_no = 0;
        w->flags &= ~(WindowFlags::flag_16);
        w->disabledWidgets = 0;
        w->holdableWidgets = 0;
        w->activatedWidgets = 0;
        w->rowHover = -1;
        w->viewportRemove(0);

        auto& tabInfo = tabInformationByTabOffset[w->currentTab];
        w->enabledWidgets = *tabInfo.enabledWidgets;
        w->eventHandlers = tabInfo.events;
        w->widgets = tabInfo.widgets;
        w->invalidate();
        w->setSize(tabInfo.windowSize);

        if ((Common::tab)w->currentTab == Common::tab::display)
            Display::applyScreenModeRestrictions(w);

        else if ((Common::tab)w->currentTab == Common::tab::music)
            w->holdableWidgets = (1 << Music::Widx::volume);

        w->callOnResize();
        w->callPrepareDraw();
        w->initScrollWidgets();
        w->invalidate();
    }
}
