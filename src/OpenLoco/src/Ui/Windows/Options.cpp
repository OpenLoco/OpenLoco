#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "Environment.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Jukebox.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/LanguageFiles.h"
#include "Localisation/Languages.h"
#include "Localisation/StringIds.h"
#include "Objects/CompetitorObject.h"
#include "Objects/CurrencyObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "Scenario/Scenario.h"
#include "Scenario/ScenarioManager.h"
#include "Scenario/ScenarioOptions.h"
#include "SceneManager.h"
#include "Ui.h"
#include "Ui/Dropdown.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/CheckboxWidget.h"
#include "Ui/Widgets/DropdownWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/GroupBoxWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/SliderWidget.h"
#include "Ui/Widgets/StepperWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/WindowManager.h"

#include <cassert>

namespace OpenLoco::Ui::Windows::Options
{
    static void tabOnMouseUp(Window& self, WidgetIndex_t wi);
    static void disableTabsByCurrentScene(Window& self);
    static void setPreferredCurrencyNameBuffer();

    std::vector<ObjectManager::SelectedObjectsFlags> _objectListSelection; // 0x011364A0

    // TODO: This shouldn't be required but its due to how the lifetime
    // of the string needs to exist beyond a prepare draw function and
    // into a draw function. Rework when custom formatter can store full
    // strings or widgets can store dynamically allocated strings.
    static std::string _chosenLanguage;

    struct AvailableCurrency
    {
        std::string name;
        ObjectHeader header;
        ObjectManager::ObjectIndexId index;
    };
    // We need to keep a copy due to lifetimes
    static sfl::small_vector<AvailableCurrency, 10> _availableCurrencies;

    static void populateAvailableCurrencies()
    {
        _availableCurrencies.clear();
        for (auto& object : ObjectManager::getAvailableObjects(ObjectType::currency))
        {
            _availableCurrencies.push_back(AvailableCurrency{ object.object._name, object.object._header, object.index });
        }
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
                tab_rendering,
                tab_audio,
                tab_regional,
                tab_controls,
                tab_company,
                tab_miscellaneous,
            };
        }

        static_assert(Widx::tab_audio == Widx::tab_display + kTabOffsetMusic);

        enum tab
        {
            display,
            rendering,
            audio,
            regional,
            controls,
            company,
            miscellaneous,
        };

        static void prepareDraw(Window& self)
        {
            self.activatedWidgets = 1ULL << (self.currentTab + 4);
            self.disabledWidgets = 0;

            self.widgets[Widx::frame].right = self.width - 1;
            self.widgets[Widx::frame].bottom = self.height - 1;
            self.widgets[Widx::panel].right = self.width - 1;
            self.widgets[Widx::panel].bottom = self.height - 1;
            self.widgets[Widx::caption].right = self.width - 2;
            self.widgets[Widx::close_button].left = self.width - 15;
            self.widgets[Widx::close_button].right = self.width - 15 + 12;

            disableTabsByCurrentScene(self);

            const auto* skin = ObjectManager::get<InterfaceSkinObject>();

            // Rendering tab
            {
                static constexpr uint32_t kRedenderingTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame0,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame1,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame2,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame3,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame4,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame5,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame6,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame7,
                };

                uint32_t imageId = skin->img;
                if (self.currentTab == tab::rendering)
                {
                    imageId += kRedenderingTabImageIds[(self.frameNo / 4) % std::size(kRedenderingTabImageIds)];
                }
                else
                {
                    imageId += kRedenderingTabImageIds[0];
                }

                self.widgets[Widx::tab_rendering].image = imageId;
            }

            static constexpr uint32_t globe_tab_ids[] = {
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

            // Regional tab
            {
                auto imageId = ImageIds::tab_globe_0;
                if (self.currentTab == tab::regional)
                {
                    imageId = globe_tab_ids[(self.frameNo / 2) % 32];
                }
                self.widgets[Widx::tab_regional].image = imageId;
            }

            // Company tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_company;
                self.widgets[Widx::tab_company].image = imageId;
            }
        }

        static void onClose([[maybe_unused]] Window& self)
        {
            ObjectManager::freeTemporaryObject();
        }

        static bool onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            switch (wi)
            {
                case Widx::close_button:
                    WindowManager::close(&self);
                    return true;

                case Widx::tab_display:
                case Widx::tab_rendering:
                case Widx::tab_audio:
                case Widx::tab_regional:
                case Widx::tab_controls:
                case Widx::tab_company:
                case Widx::tab_miscellaneous:
                    Options::tabOnMouseUp(self, wi);
                    return true;

                default:
                    return false;
            }
        }

        static constexpr auto makeCommonWidgets(Ui::Size windowSize, StringId windowCaptionId)
        {
            constexpr auto kTabWidth = 31;

            return makeWidgets(
                Widgets::Frame({ 0, 0 }, windowSize, WindowColour::primary),
                Widgets::Caption({ 1, 1 }, { (uint16_t)(windowSize.width - 2), 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ (int16_t)(windowSize.width - 15), 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { windowSize.width, 102 }, WindowColour::secondary),
                Widgets::Tab({ 3 + kTabWidth * 0, 15 }, { kTabWidth, 27 }, WindowColour::secondary, ImageIds::tab_display, StringIds::tooltip_display_options),
                Widgets::Tab({ 3 + kTabWidth * 1, 15 }, { kTabWidth, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_options_rendering),
                Widgets::Tab({ 3 + kTabWidth * 2, 15 }, { kTabWidth, 27 }, WindowColour::secondary, ImageIds::tab_sound, StringIds::tooltip_audio_options),
                Widgets::Tab({ 3 + kTabWidth * 3, 15 }, { kTabWidth, 27 }, WindowColour::secondary, ImageIds::tab_globe_0, StringIds::tooltip_regional_options),
                Widgets::Tab({ 3 + kTabWidth * 4, 15 }, { kTabWidth, 27 }, WindowColour::secondary, ImageIds::tab_control, StringIds::tooltip_control_options),
                Widgets::Tab({ 3 + kTabWidth * 5, 15 }, { kTabWidth, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_company_options),
                Widgets::Tab({ 3 + kTabWidth * 6, 15 }, { kTabWidth, 27 }, WindowColour::secondary, ImageIds::tab_miscellaneous, StringIds::tooltip_miscellaneous_options));
        }
    }

    namespace Display
    {
        static constexpr Ui::Size kWindowSize = { 400, 151 };

        namespace Widx
        {
            enum
            {
                frame_hardware = Common::Widx::tab_miscellaneous + 1,
                screen_mode_label,
                screen_mode,
                screen_mode_btn,
                display_resolution_label,
                display_resolution,
                display_resolution_btn,
                display_scale_label,
                display_scale,
                display_scale_down_btn,
                display_scale_up_btn,
                frame_limit_label,
                frame_limit,
                frame_limit_btn,
                show_fps,
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize, StringIds::options_title_display),
            Widgets::GroupBox({ 4, 49 }, { 392, 97 }, WindowColour::secondary, StringIds::frame_hardware),

            Widgets::Label({ 10, 63 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::options_screen_mode),
            Widgets::dropdownWidgets({ 235, 63 }, { 154, 12 }, WindowColour::secondary, StringIds::empty),

            Widgets::Label({ 10, 79 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::display_resolution),
            Widgets::dropdownWidgets({ 235, 79 }, { 154, 12 }, WindowColour::secondary, StringIds::display_resolution_dropdown_format),

            Widgets::Label({ 10, 95 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::window_scale_factor),
            Widgets::stepperWidgets({ 235, 95 }, { 154, 12 }, WindowColour::secondary, StringIds::scale_formatted),

            Widgets::Label({ 10, 111 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::frameRateLimitLabel),
            Widgets::dropdownWidgets({ 235, 111 }, { 154, 12 }, WindowColour::secondary, StringIds::empty),

            Widgets::Checkbox({ 10, 127 }, { 174, 12 }, WindowColour::secondary, StringIds::option_show_fps_counter, StringIds::option_show_fps_counter_tooltip)

        );

        // 0x004BFB8C
        static void onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            if (Common::onMouseUp(self, wi, id))
            {
                return;
            }

            switch (wi)
            {
                case Widx::show_fps:
                {
                    auto& cfg = OpenLoco::Config::get();
                    cfg.showFPS ^= 1;
                    OpenLoco::Config::write();
                    Gfx::invalidateScreen();
                    return;
                }
            }
        }

#if !(defined(__APPLE__) && defined(__MACH__))
        static void screenModeToggleEnabled(Window& self)
        {
            if (Config::get().display.mode == Config::ScreenMode::fullscreen)
            {
                self.disabledWidgets &= ~(1ULL << Widx::display_resolution) | (1ULL << Widx::display_resolution_btn);
                self.disabledWidgets &= ~((1ULL << Widx::display_resolution) | (1ULL << Widx::display_resolution_btn));
            }
            else
            {
                self.disabledWidgets |= ((1ULL << Widx::display_resolution) | (1ULL << Widx::display_resolution_btn));
                self.disabledWidgets |= (1ULL << Widx::display_resolution) | (1ULL << Widx::display_resolution_btn);
            }
        }
#endif

        static void screenModeMouseDown(const Window& self, [[maybe_unused]] WidgetIndex_t wi)
        {
            auto& dropdown = self.widgets[Widx::screen_mode];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::options_mode_windowed);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::options_mode_fullscreen);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::options_mode_fullscreen_window);

            auto selection = static_cast<uint16_t>(Config::get().display.mode);
            Dropdown::setItemSelected(selection);
        }

        static void screenModeDropdown([[maybe_unused]] const Window& self, int16_t selection)
        {
            if (selection == -1)
            {
                return;
            }

            auto new_mode = static_cast<Config::ScreenMode>(selection);
            if (new_mode == Config::get().display.mode)
            {
                return;
            }

#if !(defined(__APPLE__) && defined(__MACH__))
            Ui::setDisplayMode(new_mode);
#endif
        }

#pragma mark - Resolution dropdown

        // 0x004C0026
        static void resolutionMouseDown(const Window& self, [[maybe_unused]] WidgetIndex_t wi)
        {
            std::vector<Resolution> resolutions = getFullscreenResolutions();

            auto& dropdown = self.widgets[Widx::display_resolution];
            Dropdown::showText2(self.x + dropdown.left, self.y + dropdown.top, dropdown.width(), dropdown.height(), self.getColour(WindowColour::secondary), resolutions.size(), 0x80);

            auto& cfg = Config::get();
            for (size_t i = 0; i < resolutions.size(); i++)
            {
                Dropdown::add(i, StringIds::dropdown_stringid, { StringIds::display_resolution_dropdown_format, (uint16_t)resolutions[i].width, (uint16_t)resolutions[i].height });
                if (cfg.display.fullscreenResolution.width == resolutions[i].width && cfg.display.fullscreenResolution.height == resolutions[i].height)
                {
                    Dropdown::setItemSelected((int16_t)i);
                }
            }
        }

        // 0x004C00F4
        static void resolutionDropdown([[maybe_unused]] const Window& self, int16_t index)
        {
            if (index == -1)
            {
                return;
            }
            std::vector<Resolution> resolutions = getFullscreenResolutions();
            Ui::setDisplayMode(Config::ScreenMode::fullscreen, { resolutions[index].width, resolutions[index].height });
        }

#pragma mark - Frame limit dropdown

        static void frameLimitMouseDown(const Window& self, [[maybe_unused]] WidgetIndex_t wi)
        {
            auto& dropdown = self.widgets[Widx::frame_limit];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width(), dropdown.height(), self.getColour(WindowColour::secondary), 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::frameRateLimitInternal);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::frameRateLimitVsync);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::frameRateLimitUnrestricted);

            auto activeItem = 0U;
            if (Config::get().uncapFPS)
            {
                activeItem = Config::get().display.vsync ? 1 : 2;
            }

            Dropdown::setItemSelected(activeItem);
        }

        static void frameLimitDropdown(const Window& self, int16_t itemIndex)
        {
            if (itemIndex == -1)
            {
                return;
            }

            auto& config = Config::get();
            switch (itemIndex)
            {
                case 0: // vanilla
                    config.uncapFPS = 0;
                    config.display.vsync = 0;
                    break;
                case 1: // vsync
                    config.uncapFPS = 1;
                    config.display.vsync = 1;
                    break;
                case 2: // uncapped
                    config.uncapFPS = 1;
                    config.display.vsync = 0;
                    break;
            }

            auto& drawingEngine = Gfx::getDrawingEngine();
            if (drawingEngine.setVSync(config.display.vsync))
            {
                Config::write();
                WindowManager::invalidateWidget(self.type, self.number, Widx::frame_limit);
            }
        }

#pragma mark -

        static void displayScaleMouseDown([[maybe_unused]] const Window& self, [[maybe_unused]] WidgetIndex_t wi, float adjust_by)
        {
            OpenLoco::Ui::adjustWindowScale(adjust_by);
        }

        // 0x004BFBB7
        static void onMouseDown(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            switch (wi)
            {
                case Widx::screen_mode_btn:
                    screenModeMouseDown(self, wi);
                    break;
                case Widx::display_resolution_btn:
                    resolutionMouseDown(self, wi);
                    break;
                case Widx::display_scale_down_btn:
                    displayScaleMouseDown(self, wi, -OpenLoco::Ui::ScaleFactor::step);
                    break;
                case Widx::display_scale_up_btn:
                    displayScaleMouseDown(self, wi, OpenLoco::Ui::ScaleFactor::step);
                    break;
                case Widx::frame_limit_btn:
                    frameLimitMouseDown(self, wi);
                    break;
            }
        }

        // 0x004BFBE8
        static void onDropdown(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id, int16_t item_index)
        {
            switch (wi)
            {
                case Widx::screen_mode_btn:
                    screenModeDropdown(self, item_index);
                    break;
                case Widx::display_resolution_btn:
                    resolutionDropdown(self, item_index);
                    break;
                case Widx::frame_limit_btn:
                    frameLimitDropdown(self, item_index);
                    break;
            }
        }

        // 0x004C01F5
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, self.currentTab + 4);
        }

        // 0x004BFA04
        static void prepareDraw(Window& self)
        {
            assert(self.currentTab == Common::tab::display);

            Common::prepareDraw(self);

            StringId screenModeStringId = StringIds::empty;
            switch (Config::get().display.mode)
            {
                case Config::ScreenMode::window:
                    screenModeStringId = StringIds::options_mode_windowed;
                    break;
                case Config::ScreenMode::fullscreen:
                    screenModeStringId = StringIds::options_mode_fullscreen;
                    break;
                case Config::ScreenMode::fullscreenBorderless:
                    screenModeStringId = StringIds::options_mode_fullscreen_window;
                    break;
            }
            self.widgets[Widx::screen_mode].text = screenModeStringId;

            // Resolution.
            {
                auto args = FormatArguments(self.widgets[Widx::display_resolution].textArgs);
                auto& resolution = Config::get().display.fullscreenResolution;
                args.push<uint16_t>(resolution.width);
                args.push<uint16_t>(resolution.height);
            }

            // Scale.
            {
                auto args = FormatArguments(self.widgets[Widx::display_scale].textArgs);
                args.push<int32_t>(Config::get().scaleFactor * 100);
            }

            StringId frameLimitStringId = StringIds::frameRateLimitInternal;
            if (Config::get().uncapFPS)
            {
                if (Config::get().display.vsync)
                {
                    frameLimitStringId = StringIds::frameRateLimitVsync;
                }
                else
                {
                    frameLimitStringId = StringIds::frameRateLimitUnrestricted;
                }
            }
            self.widgets[Widx::frame_limit].text = frameLimitStringId;

            if (Config::get().showFPS)
            {
                self.activatedWidgets |= (1ULL << Widx::show_fps);
            }

            if (Config::get().scaleFactor <= OpenLoco::Ui::ScaleFactor::min)
            {
                self.disabledWidgets |= (1ULL << Widx::display_scale_down_btn);
            }

            if (Config::get().scaleFactor >= OpenLoco::Ui::ScaleFactor::max)
            {
                self.disabledWidgets |= (1ULL << Widx::display_scale_up_btn);
            }

#if !(defined(__APPLE__) && defined(__MACH__))
            screenModeToggleEnabled(self);
#endif
        }

        // 0x004BFAF9
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            // Draw widgets.
            self.draw(drawingCtx);
        }

        static void applyScreenModeRestrictions(Window& self)
        {
            if (Config::get().display.mode != Config::ScreenMode::fullscreen)
            {
                self.disabledWidgets = (1ULL << Display::Widx::display_resolution) | (1ULL << Display::Widx::display_resolution_btn);
            }

#if !(defined(__APPLE__) && defined(__MACH__))
            Display::screenModeToggleEnabled(self);
#else
            self.disabledWidgets |= (1ULL << Display::Widx::screen_mode)
                | (1ULL << Display::Widx::screen_mode_btn)
                | (1ULL << Display::Widx::display_resolution)
                | (1ULL << Display::Widx::display_resolution_btn);
#endif
        }

        static constexpr WindowEventList kEvents = {
            .onClose = Common::onClose,
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Rendering
    {
        static constexpr Ui::Size kWindowSize = { 400, 218 };

        namespace Widx
        {
            enum
            {
                frame_map_rendering = Common::Widx::tab_miscellaneous + 1,
                vehicles_min_scale_label,
                vehicles_min_scale,
                vehicles_min_scale_btn,
                station_names_min_scale_label,
                station_names_min_scale,
                station_names_min_scale_btn,
                construction_marker_label,
                construction_marker,
                construction_marker_btn,
                landscape_smoothing,
                gridlines_on_landscape,
                cash_popup_rendering,
                show_company_ai_planning,

                frame_user_interface,
                window_frame_style_label,
                window_frame_style,
                window_frame_style_btn,
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize, StringIds::title_options_rendering),
            Widgets::GroupBox({ 4, 49 }, { 392, 128 }, WindowColour::secondary, StringIds::frame_map_rendering),

            Widgets::Label({ 10, 63 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::vehicles_min_scale),
            Widgets::dropdownWidgets({ 235, 63 }, { 154, 12 }, WindowColour::secondary, StringIds::empty, StringIds::vehicles_min_scale_tip),

            Widgets::Label({ 10, 79 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::station_names_min_scale),
            Widgets::dropdownWidgets({ 235, 79 }, { 154, 12 }, WindowColour::secondary, StringIds::empty, StringIds::station_names_min_scale_tip),

            Widgets::Label({ 10, 95 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::construction_marker),
            Widgets::dropdownWidgets({ 235, 95 }, { 154, 12 }, WindowColour::secondary, StringIds::empty),

            Widgets::Checkbox({ 10, 110 }, { 346, 12 }, WindowColour::secondary, StringIds::landscape_smoothing, StringIds::landscape_smoothing_tip),
            Widgets::Checkbox({ 10, 126 }, { 346, 12 }, WindowColour::secondary, StringIds::gridlines_on_landscape, StringIds::gridlines_on_landscape_tip),
            Widgets::Checkbox({ 10, 142 }, { 346, 12 }, WindowColour::secondary, StringIds::cash_popup_rendering, StringIds::tooltip_cash_popup_rendering),
            Widgets::Checkbox({ 10, 158 }, { 346, 12 }, WindowColour::secondary, StringIds::show_company_ai_planning, StringIds::show_company_ai_planning_tip),

            Widgets::GroupBox({ 4, 180 }, { 392, 32 }, WindowColour::secondary, StringIds::userInterfaceGroup),
            Widgets::Label({ 10, 195 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::windowFrameStyle),
            Widgets::dropdownWidgets({ 235, 194 }, { 154, 12 }, WindowColour::secondary, StringIds::empty, StringIds::windowFrameStyleTip)

        );

        // 0x004BFB8C
        static void onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            if (Common::onMouseUp(self, wi, id))
            {
                return;
            }

            switch (wi)
            {
                case Widx::landscape_smoothing:
                {
                    auto& cfg = OpenLoco::Config::get();
                    cfg.landscapeSmoothing ^= true;
                    Config::write();
                    Gfx::invalidateScreen();
                    return;
                }

                case Widx::gridlines_on_landscape:
                {
                    auto& cfg = OpenLoco::Config::get();
                    cfg.gridlinesOnLandscape ^= true;
                    Config::write();
                    Gfx::invalidateScreen();

                    auto main = WindowManager::getMainWindow();
                    if (main != nullptr)
                    {
                        main->viewports[0]->flags &= ~ViewportFlags::gridlines_on_landscape;

                        if (cfg.gridlinesOnLandscape)
                        {
                            main->viewports[0]->flags |= ViewportFlags::gridlines_on_landscape;
                        }
                    }

                    return;
                }

                case Widx::cash_popup_rendering:
                {
                    auto& cfg = OpenLoco::Config::get();
                    cfg.cashPopupRendering = !cfg.cashPopupRendering;
                    Config::write();
                    self.invalidate();
                    return;
                }

                case Widx::show_company_ai_planning:
                {
                    auto& cfg = OpenLoco::Config::get();
                    cfg.showAiPlanningAsGhosts = !cfg.showAiPlanningAsGhosts;
                    Config::write();
                    Gfx::invalidateScreen();
                }
            }
        }

#pragma mark - Construction Marker (Widget 19)

        // 0x004BFE2E
        static void constructionMarkerMouseDown(const Window& self, [[maybe_unused]] WidgetIndex_t wi)
        {
            auto& dropdown = self.widgets[Widx::construction_marker];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::white);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::translucent);
            Dropdown::setItemSelected(Config::get().constructionMarker);
        }

        // 0x004BFE98
        static void constructionMarkerDropdown(int16_t ax)
        {
            if (ax == -1)
            {
                return;
            }

            if (ax == Config::get().constructionMarker)
            {
                return;
            }

            auto& cfg = OpenLoco::Config::get();
            cfg.constructionMarker = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

#pragma mark - Vehicle zoom (Widget 15)

        // 0x004BFEBE
        static void vehicleZoomMouseDown(const Window& self, [[maybe_unused]] WidgetIndex_t wi)
        {
            auto& dropdown = self.widgets[Widx::vehicles_min_scale];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 4, 0x80);

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
            {
                return;
            }

            if (ax == Config::get().vehiclesMinScale)
            {
                return;
            }

            auto& cfg = OpenLoco::Config::get();
            cfg.vehiclesMinScale = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

#pragma mark - Station names minimum scale (Widget 17)

        // 0x004BFF72
        static void stationNamesScaleMouseDown(const Window& self, [[maybe_unused]] WidgetIndex_t wi)
        {
            auto& dropdown = self.widgets[Widx::station_names_min_scale];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 4, 0x80);

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
            {
                return;
            }

            if (ax == Config::get().stationNamesMinScale)
            {
                return;
            }

            auto& cfg = OpenLoco::Config::get();
            cfg.stationNamesMinScale = ax;
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

        static void windowFrameStyleMouseDown(const Window& self, [[maybe_unused]] WidgetIndex_t wi)
        {
            auto& dropdown = self.widgets[Widx::window_frame_style];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::windowFrameStyleGradient);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::windowFrameStyleSolid);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::windowFrameStyleTranslucent);
            Dropdown::setItemSelected(enumValue(Config::get().windowFrameStyle));
        }

        static void windowFrameStyleDropdown(int16_t selectedItem)
        {
            if (selectedItem == -1)
            {
                return;
            }

            if (selectedItem == enumValue(Config::get().windowFrameStyle))
            {
                return;
            }

            auto& cfg = OpenLoco::Config::get();
            cfg.windowFrameStyle = OpenLoco::Config::WindowFrameStyle(selectedItem);
            OpenLoco::Config::write();
            Gfx::invalidateScreen();
        }

        // 0x004BFBB7
        static void onMouseDown(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            switch (wi)
            {
                case Widx::construction_marker_btn:
                    constructionMarkerMouseDown(self, wi);
                    break;
                case Widx::vehicles_min_scale_btn:
                    vehicleZoomMouseDown(self, wi);
                    break;
                case Widx::station_names_min_scale_btn:
                    stationNamesScaleMouseDown(self, wi);
                    break;
                case Widx::window_frame_style_btn:
                    windowFrameStyleMouseDown(self, wi);
                    break;
            }
        }

        // 0x004BFBE8
        static void onDropdown([[maybe_unused]] Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id, int16_t item_index)
        {
            switch (wi)
            {
                case Widx::construction_marker_btn:
                    constructionMarkerDropdown(item_index);
                    break;
                case Widx::vehicles_min_scale_btn:
                    vehicleZoomDropdown(item_index);
                    break;
                case Widx::station_names_min_scale_btn:
                    stationNamesScaleDropdown(item_index);
                    break;
                case Widx::window_frame_style_btn:
                    windowFrameStyleDropdown(item_index);
                    break;
            }
        }

        // 0x004C01F5
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, self.currentTab + 4);
        }

        // 0x004BFA04
        static void prepareDraw(Window& self)
        {
            assert(self.currentTab == Common::tab::rendering);

            Common::prepareDraw(self);

            if (Config::get().constructionMarker)
            {
                self.widgets[Widx::construction_marker].text = StringIds::translucent;
            }
            else
            {
                self.widgets[Widx::construction_marker].text = StringIds::white;
            }

            static constexpr StringId kScaleStringIds[] = {
                StringIds::full_scale,
                StringIds::half_scale,
                StringIds::quarter_scale,
                StringIds::eighth_scale,
            };

            self.widgets[Widx::vehicles_min_scale].text = kScaleStringIds[Config::get().vehiclesMinScale];
            self.widgets[Widx::station_names_min_scale].text = kScaleStringIds[Config::get().stationNamesMinScale];

            static constexpr StringId kWindowStyleStringIds[] = {
                StringIds::windowFrameStyleGradient,
                StringIds::windowFrameStyleSolid,
                StringIds::windowFrameStyleTranslucent,
            };

            self.widgets[Widx::window_frame_style].text = kWindowStyleStringIds[enumValue(Config::get().windowFrameStyle)];

            if (Config::get().landscapeSmoothing)
            {
                self.activatedWidgets |= (1ULL << Widx::landscape_smoothing);
            }

            if (Config::get().gridlinesOnLandscape)
            {
                self.activatedWidgets |= (1ULL << Widx::gridlines_on_landscape);
            }

            if (Config::get().cashPopupRendering)
            {
                self.activatedWidgets |= (1ULL << Widx::cash_popup_rendering);
            }

            if (Config::get().showAiPlanningAsGhosts)
            {
                self.activatedWidgets |= (1ULL << Widx::show_company_ai_planning);
            }
        }

        // 0x004BFAF9
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            // Draw widgets.
            self.draw(drawingCtx);
        }

        static constexpr WindowEventList kEvents = {
            .onClose = Common::onClose,
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace AudioTab
    {
        // Some widget positions in pixels
        constexpr auto kSoundGroupOffset = 49;
        constexpr auto kSoundGroupLastItemOffset = 30;
        constexpr auto kSoundGroupHeight = kSoundGroupLastItemOffset + 19;

        constexpr auto kMusicGroupOffset = kSoundGroupOffset + kSoundGroupHeight + 4;
        constexpr auto kMusicGroupLastItemOffset = 73;
        constexpr auto kMusicGroupHeight = kMusicGroupLastItemOffset + 19;

        static constexpr Ui::Size kWindowSize = { 366, 49 + kSoundGroupHeight + 4 + kMusicGroupHeight + 4 };

        namespace Widx
        {
            enum
            {
                frame_sound = Common::Widx::tab_miscellaneous + 1,
                audio_device,
                audio_device_btn,
                play_title_music,

                frame_jukebox,
                currently_playing_label,
                currently_playing,
                currently_playing_btn,
                music_controls_stop,
                music_controls_play,
                music_controls_next,
                music_volume_label,
                music_volume,
                music_playlist,
                music_playlist_btn,
                edit_selection
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize, StringIds::options_title_audio),

            Widgets::GroupBox({ 4, kSoundGroupOffset }, { kWindowSize.width - 8, kSoundGroupHeight }, WindowColour::secondary, StringIds::frame_sound),
            Widgets::dropdownWidgets({ 10, kSoundGroupOffset + 14 }, { 346, 12 }, WindowColour::secondary, StringIds::stringid),
            Widgets::Checkbox({ 10, kSoundGroupOffset + kSoundGroupLastItemOffset }, { 346, 12 }, WindowColour::secondary, StringIds::play_title_music),

            Widgets::GroupBox({ 4, kMusicGroupOffset }, { kWindowSize.width - 8, kMusicGroupHeight }, WindowColour::secondary, StringIds::frame_jukebox),
            Widgets::Label({ 10, kMusicGroupOffset + 14 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::currently_playing),
            Widgets::dropdownWidgets({ 160, kMusicGroupOffset + 14 }, { 196, 12 }, WindowColour::secondary, StringIds::stringid),
            Widgets::ImageButton({ 10, kMusicGroupOffset + 29 }, { 24, 24 }, WindowColour::secondary, ImageIds::music_controls_stop, StringIds::music_controls_stop_tip),
            Widgets::ImageButton({ 34, kMusicGroupOffset + 29 }, { 24, 24 }, WindowColour::secondary, ImageIds::music_controls_play, StringIds::music_controls_play_tip),
            Widgets::ImageButton({ 58, kMusicGroupOffset + 29 }, { 24, 24 }, WindowColour::secondary, ImageIds::music_controls_next, StringIds::music_controls_next_tip),
            Widgets::Label({ 160, kMusicGroupOffset + 35 }, { 215, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::music_volume),
            Widgets::Slider({ 256, kMusicGroupOffset + 35 - 6 }, { 109, 24 }, WindowColour::secondary, Widget::kContentNull, StringIds::set_music_volume_tip),
            Widgets::dropdownWidgets({ 10, kMusicGroupOffset + 58 }, { 346, 12 }, WindowColour::secondary, StringIds::stringid),
            Widgets::Button({ 183, kMusicGroupOffset + kMusicGroupLastItemOffset }, { 173, 12 }, WindowColour::secondary, StringIds::edit_music_selection, StringIds::edit_music_selection_tip));

        static void audioDeviceMouseDown(const Window& self);
        static void audioDeviceDropdown(const Window& self, int16_t itemIndex);
        static void playTitleMusicOnMouseUp(Window& self);
        static void currentlyPlayingMouseDown(const Window& self);
        static void currentlyPlayingDropdown(Window& self, int16_t itemIndex);
        static void stopMusic(Window& self);
        static void playMusic(Window& self);
        static void playNextSong(Window& self);
        static void volumeMouseDown(Window& self);
        static void musicPlaylistMouseDown(const Window& self);
        static void musicPlaylistDropdown(Window& self, int16_t itemIndex);

        // 0x004C0217, 0x004C0217
        static void prepareDraw(Window& self)
        {
            assert(self.currentTab == Common::tab::audio);

            Common::prepareDraw(self);

            // Audio device
            {
                auto args = FormatArguments(self.widgets[Widx::audio_device].textArgs);
                auto audioDeviceName = Audio::getCurrentDeviceName();
                if (audioDeviceName != nullptr)
                {
                    args.push(StringIds::stringptr);
                    args.push(audioDeviceName);
                }
                else
                {
                    args.push(StringIds::audio_device_none);
                }
            }

            // Currently playing music track
            {
                StringId songName = StringIds::music_none;
                if (SceneManager::isPlayMode())
                {
                    songName = Jukebox::getSelectedTrackTitleId();
                }
                else if (SceneManager::isTitleMode())
                {
                    auto& cfg = Config::get();
                    if (cfg.audio.playTitleMusic)
                    {
                        songName = StringIds::locomotion_title; // Name of the title screen music
                    }
                }
                auto args = FormatArguments(self.widgets[Widx::currently_playing].textArgs);
                args.push(songName);
            }

            // Jukebox controls (stop/play/skip)
            if (!SceneManager::isPlayMode())
            {
                self.disabledWidgets |= (1ULL << Widx::currently_playing) | (1ULL << Widx::currently_playing_btn) | (1ULL << Widx::music_controls_play) | (1ULL << Widx::music_controls_stop) | (1ULL << Widx::music_controls_next);
            }
            else if (Jukebox::isMusicPlaying())
            {
                // Play button appears pressed
                self.activatedWidgets |= (1ULL << Widx::music_controls_play);
            }
            else
            {
                // Stop button appears pressed
                self.activatedWidgets |= (1ULL << Widx::music_controls_stop);
            }

            // Selected playlist
            {
                static constexpr StringId playlist_string_ids[] = {
                    StringIds::play_only_music_from_current_era,
                    StringIds::play_all_music,
                    StringIds::play_custom_music_selection,
                };

                auto args = FormatArguments(self.widgets[Widx::music_playlist].textArgs);

                StringId selectedPlaylistStringId = playlist_string_ids[enumValue(Config::get().audio.playlist)];
                args.push(selectedPlaylistStringId);
            }

            // Edit custom playlist music selection button
            if (Config::get().audio.playlist != Config::MusicPlaylistType::custom)
            {
                self.disabledWidgets |= (1ULL << Widx::edit_selection);
            }

            // Play title music checkbox
            if (Config::get().audio.playTitleMusic)
            {
                self.activatedWidgets |= (1ULL << Widx::play_title_music);
            }
        }

        // 0x004C02F5, 0x004C05F9
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            // Draw widgets.
            self.draw(drawingCtx);

            // TODO: Move this in Slider widget.
            drawingCtx.drawImage(self.x + self.widgets[Widx::music_volume].left, self.y + self.widgets[Widx::music_volume].top, Gfx::recolour(ImageIds::volume_slider_track, self.getColour(WindowColour::secondary).c()));

            int16_t x = 90 + (Config::get().audio.mainVolume / 32);
            drawingCtx.drawImage(self.x + self.widgets[Widx::music_volume].left + x, self.y + self.widgets[Widx::music_volume].top, Gfx::recolour(ImageIds::volume_slider_thumb, self.getColour(WindowColour::secondary).c()));
        }

        static void onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            if (Common::onMouseUp(self, wi, id))
            {
                return;
            }

            switch (wi)
            {
                case Widx::play_title_music:
                    playTitleMusicOnMouseUp(self);
                    return;

                case Widx::music_controls_stop:
                    stopMusic(self);
                    return;

                case Widx::music_controls_play:
                    playMusic(self);
                    return;

                case Widx::music_controls_next:
                    playNextSong(self);
                    return;

                case Widx::edit_selection:
                    MusicSelection::open();
                    return;
            }
        }

        static void onMouseDown(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            switch (wi)
            {
                case Widx::audio_device_btn:
                    audioDeviceMouseDown(self);
                    break;
                case Widx::music_playlist_btn:
                    musicPlaylistMouseDown(self);
                    break;
                case Widx::currently_playing_btn:
                    currentlyPlayingMouseDown(self);
                    break;
                case Widx::music_volume:
                    volumeMouseDown(self);
                    break;
            }
        }

        static void onDropdown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case Widx::audio_device_btn:
                    audioDeviceDropdown(self, itemIndex);
                    break;
                case Widx::music_playlist_btn:
                    musicPlaylistDropdown(self, itemIndex);
                    break;
                case Widx::currently_playing_btn:
                    currentlyPlayingDropdown(self, itemIndex);
                    break;
            }
        }

        // 0x004C043D
        static void audioDeviceMouseDown(const Window& self)
        {
            const auto& devices = Audio::getDevices();
            if (devices.size() != 0)
            {
                auto& dropdown = self.widgets[Widx::audio_device];
                Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), devices.size(), 0x80);
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
        static void audioDeviceDropdown(const Window& self, int16_t itemIndex)
        {
            if (itemIndex != -1)
            {
                Audio::setDevice(itemIndex);

                // Start playing the title screen music again if applicable (Fix for #2689)
                auto& config = Config::get();
                if (SceneManager::isTitleMode() && config.audio.playTitleMusic)
                {
                    Audio::playMusic(Environment::PathId::css5, config.audio.mainVolume, true);
                }

                WindowManager::invalidateWidget(self.type, self.number, Widx::audio_device);
            }
        }

        static void playTitleMusicOnMouseUp(Window& self)
        {
            auto& cfg = Config::get();
            cfg.audio.playTitleMusic = !cfg.audio.playTitleMusic;
            Config::write();
            self.invalidate();

            if (!SceneManager::isTitleMode())
            {
                return;
            }

            if (cfg.audio.playTitleMusic)
            {
                Audio::playMusic(Environment::PathId::css5, Config::get().audio.mainVolume, true);
            }
            else
            {
                Audio::stopMusic();
            }
        }

        // 0x004C0875
        static void currentlyPlayingMouseDown(const Window& self)
        {
            auto tracks = Jukebox::makeSelectedPlaylist();

            auto& dropdown = self.widgets[Widx::currently_playing];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), tracks.size(), 0x80);

            int index = -1;
            for (auto track : tracks)
            {
                index++;
                Dropdown::add(index, StringIds::dropdown_stringid, Jukebox::getMusicInfo(track).titleId);
                if (track == Jukebox::getCurrentTrack())
                {
                    Dropdown::setItemSelected(index);
                }
            }
        }

        // 0x004C09F8
        static void currentlyPlayingDropdown(Window& self, int16_t ax)
        {
            if (ax == -1)
            {
                return;
            }

            auto track = Jukebox::makeSelectedPlaylist().at(ax);
            if (Jukebox::requestTrack(track))
            {
                self.invalidate();
            }
        }

        // 0x004C0778
        static void stopMusic(Window& self)
        {
            if (Jukebox::disableMusic())
            {
                self.invalidate();
            }
        }

        // 0x004C07A4
        static void playMusic(Window& self)
        {
            if (Jukebox::enableMusic())
            {
                self.invalidate();
            }
        }

        // 0x004C07C4
        static void playNextSong(Window& self)
        {
            if (Jukebox::skipCurrentTrack())
            {
                self.invalidate();
            }
        }

        // 0x004C072A
        static void volumeMouseDown(Window& self)
        {
            Input::setClickRepeatTicks(31);

            auto mousePos = Input::getScrollLastLocation();
            int x = mousePos.x - self.x - self.widgets[Widx::music_volume].left - 10;
            x = std::clamp(x, 0, 80);

            Audio::setBgmVolume((x * 32) - 2560);

            self.invalidate();
        }

        // 0x004C07E4
        static void musicPlaylistMouseDown(const Window& self)
        {
            auto& dropdown = self.widgets[Widx::music_playlist];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::play_only_music_from_current_era);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::play_all_music);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::play_custom_music_selection);

            Dropdown::setItemSelected(enumValue(Config::get().audio.playlist));
        }

        // 0x004C084A
        static void musicPlaylistDropdown(Window& self, int16_t index)
        {
            if (index == -1)
            {
                return;
            }

            auto& cfg = Config::get().audio;
            cfg.playlist = Config::MusicPlaylistType(index);
            Config::write();

            self.invalidate();

            if (!SceneManager::isTitleMode()) // Prevents title music from stopping
            {
                Audio::revalidateCurrentTrack();
            }

            WindowManager::close(WindowType::musicSelection);
        }

        // 0x004C04E0, 0x004C0A37
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, self.currentTab + 4);
        }

        static constexpr WindowEventList kEvents = {
            .onClose = Common::onClose,
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Regional
    {
        static constexpr Ui::Size kWindowSize = { 366, 167 };

        namespace Widx
        {
            enum
            {
                language_label = Common::Widx::tab_miscellaneous + 1,
                language,
                language_btn,
                distance_label,
                distance_speed,
                distance_speed_btn,
                heights_label,
                heights,
                heights_btn,
                currency_label,
                currency,
                currency_btn,
                preferred_currency_label,
                preferred_currency,
                preferred_currency_btn,
                preferred_currency_for_new_games,
                preferred_currency_always
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize, StringIds::options_title_regional),

            Widgets::Label({ 10, 49 }, { 173, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::options_language),
            Widgets::dropdownWidgets({ 183, 49 }, { 173, 12 }, WindowColour::secondary, StringIds::stringptr),

            Widgets::Label({ 10, 69 }, { 173, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::distance_and_speed),
            Widgets::dropdownWidgets({ 183, 69 }, { 173, 12 }, WindowColour::secondary, StringIds::stringid),

            Widgets::Label({ 10, 84 }, { 173, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::heights),
            Widgets::dropdownWidgets({ 183, 84 }, { 173, 12 }, WindowColour::secondary, StringIds::stringid),

            Widgets::Label({ 10, 104 }, { 173, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::current_game_currency),
            Widgets::dropdownWidgets({ 183, 104 }, { 173, 12 }, WindowColour::secondary, StringIds::stringid, StringIds::current_game_currency_tip),

            Widgets::Label({ 10, 119 }, { 173, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::new_game_currency),
            Widgets::dropdownWidgets({ 183, 119 }, { 173, 12 }, WindowColour::secondary, StringIds::preferred_currency_buffer, StringIds::new_game_currency_tip),

            Widgets::Checkbox({ 10, 134 }, { 346, 12 }, WindowColour::secondary, StringIds::use_preferred_currency_new_game, StringIds::use_preferred_currency_new_game_tip),
            Widgets::Checkbox({ 10, 148 }, { 346, 12 }, WindowColour::secondary, StringIds::use_preferred_currency_always, StringIds::use_preferred_currency_always_tip)

        );

        static void languageMouseDown(const Window& self);
        static void languageDropdown(Window& self, int16_t itemIndex);
        static void currencyMouseDown(const Window& self);
        static void currencyDropdown(Window& self, int16_t itemIndex);
        static void preferredCurrencyMouseDown(const Window& self);
        static void preferredCurrencyDropdown(Window& self, int16_t itemIndex);
        static void preferredCurrencyNewGameMouseUp(Window& self);
        static void preferredCurrencyAlwaysMouseUp(Window& self);
        static void distanceSpeedMouseDown(const Window& self);
        static void distanceSpeedDropdown(const Window& self, int16_t itemIndex);
        static void heightsLabelsMouseDown(const Window& self);
        static void heightsLabelsDropdown(const Window& self, int16_t itemIndex);

        // 0x004C0A59
        static void prepareDraw(Window& self)
        {
            assert(self.currentTab == Common::tab::regional);

            Common::prepareDraw(self);

            {
                auto args = FormatArguments(self.widgets[Widx::language].textArgs);

                auto& language = Localisation::getDescriptorForLanguage(Config::get().language);
                _chosenLanguage = language.nativeName;
                args.push(_chosenLanguage.c_str());
            }

            {
                auto args = FormatArguments(self.widgets[Widx::distance_speed].textArgs);

                StringId current_measurement_format = StringIds::imperial;
                if (OpenLoco::Config::get().measurementFormat == Config::MeasurementFormat::metric)
                {
                    current_measurement_format = StringIds::metric;
                }

                args.push(current_measurement_format);
            }

            {
                auto args = FormatArguments(self.widgets[Widx::currency].textArgs);
                args.push(ObjectManager::get<CurrencyObject>()->name);
            }

            {
                auto args = FormatArguments(self.widgets[Widx::heights].textArgs);

                StringId current_height_units = StringIds::height_units;
                if (!OpenLoco::Config::get().showHeightAsUnits)
                {
                    current_height_units = StringIds::height_real_values;
                }

                args.push(current_height_units);
            }

            if (Config::get().usePreferredCurrencyForNewGames)
            {
                self.activatedWidgets |= (1ULL << Widx::preferred_currency_for_new_games);
            }

            if (Config::get().usePreferredCurrencyAlways)
            {
                self.activatedWidgets |= (1ULL << Widx::preferred_currency_always);
            }

            if (Config::get().usePreferredCurrencyAlways || SceneManager::isTitleMode())
            {
                self.disabledWidgets |= (1ULL << Widx::currency);
                self.disabledWidgets |= (1ULL << Widx::currency_btn);
            }
        }

        // 0x004C0B5B
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            // Draw widgets.
            self.draw(drawingCtx);
        }

        static void onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            if (Common::onMouseUp(self, wi, id))
            {
                return;
            }

            switch (wi)
            {
                case Widx::preferred_currency_for_new_games:
                    preferredCurrencyNewGameMouseUp(self);
                    return;

                case Widx::preferred_currency_always:
                    preferredCurrencyAlwaysMouseUp(self);
                    return;
            }
        }

        // 0x004BFBB7
        static void onMouseDown(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            switch (wi)
            {
                case Widx::language_btn:
                    languageMouseDown(self);
                    break;
                case Widx::heights_btn:
                    heightsLabelsMouseDown(self);
                    break;
                case Widx::distance_speed_btn:
                    distanceSpeedMouseDown(self);
                    break;
                case Widx::currency_btn:
                    currencyMouseDown(self);
                    break;
                case Widx::preferred_currency_btn:
                    preferredCurrencyMouseDown(self);
                    break;
            }
        }

        // 0x004C0C4A
        static void onDropdown(Ui::Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case Widx::language_btn:
                    languageDropdown(self, itemIndex);
                    break;

                case Widx::heights_btn:
                    heightsLabelsDropdown(self, itemIndex);
                    break;

                case Widx::distance_speed_btn:
                    distanceSpeedDropdown(self, itemIndex);
                    break;

                case Widx::currency_btn:
                    currencyDropdown(self, itemIndex);
                    break;

                case Widx::preferred_currency_btn:
                    preferredCurrencyDropdown(self, itemIndex);
                    break;
            }
        }

        static void languageMouseDown(const Window& self)
        {
            const auto lds = Localisation::getLanguageDescriptors();
            uint8_t numLanguages = static_cast<uint8_t>(lds.size());

            auto& dropdown = self.widgets[Widx::language];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), numLanguages - 1, 0x80);

            std::string& current_language = Config::get().language;

            for (uint8_t index = 1; index < numLanguages; index++)
            {
                auto& ld = lds[index];
                Dropdown::add(index - 1, StringIds::dropdown_stringptr, (char*)ld.nativeName.c_str());

                if (ld.locale == current_language)
                {
                    Dropdown::setItemSelected(index - 1);
                }
            }
        }

        static void languageDropdown(Window& self, int16_t itemIndex)
        {
            if (itemIndex == -1)
            {
                self.invalidate();
                return;
            }

            const auto lds = Localisation::getLanguageDescriptors();
            const auto& ld = lds[itemIndex + 1];
            Config::get().language = ld.locale;
            Config::write();
            Localisation::loadLanguageFile();
            // Reloading the objects will force objects to load the new language
            ObjectManager::reloadAll();
            Gfx::invalidateScreen();

            // Rebuild the scenario index to use the new language.
            ScenarioManager::loadIndex(true);
        }

        // 0x004C0C73
        static void currencyMouseDown(const Window& self)
        {
            auto& dropdown = self.widgets[Widx::currency];
            auto numItems = ObjectManager::getNumAvailableObjectsByType(ObjectType::currency);
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), numItems, 0x80);

            int index = -1;
            for (auto& object : _availableCurrencies)
            {
                index++;
                Dropdown::add(index, StringIds::dropdown_stringptr, object.name.c_str());

                if ((_objectListSelection[object.index] & ObjectManager::SelectedObjectsFlags::selected) != ObjectManager::SelectedObjectsFlags::none)
                {
                    Dropdown::setItemSelected(index);
                }
            }
        }

        // 0x004C0D33
        static void currencyDropdown(Window& self, int16_t itemIndex)
        {
            if (itemIndex == -1)
            {
                self.invalidate();
                return;
            }

            int index = -1;
            for (const auto& object : _availableCurrencies)
            {
                index++;
                if (index == itemIndex)
                {
                    auto ebp = ObjectManager::getActiveObject(ObjectType::currency, _objectListSelection);

                    if (ebp.index != ObjectManager::kNullObjectIndex)
                    {
                        ObjectManager::unload(ebp.object._header);
                    }

                    ObjectManager::load(object.header);
                    ObjectManager::reloadAll();
                    Gfx::loadCurrency();
                    ObjectManager::markOnlyLoadedObjects(_objectListSelection);

                    break;
                }
            }

            self.invalidate();
        }

        // 0x004C0DCF
        static void preferredCurrencyMouseDown(const Window& self)
        {
            auto& dropdown = self.widgets[Widx::preferred_currency];
            auto numItems = ObjectManager::getNumAvailableObjectsByType(ObjectType::currency);
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), numItems, 0x80);

            int index = -1;
            for (auto& object : _availableCurrencies)
            {
                index++;
                Dropdown::add(index, StringIds::dropdown_stringptr, object.name.c_str());

                if (OpenLoco::Config::get().preferredCurrency == object.header)
                {
                    Dropdown::setItemSelected(index);
                }
            }
        }

        // 0x004C0E82
        static void preferredCurrencyDropdown(Window& self, int16_t itemIndex)
        {
            if (itemIndex == -1)
            {
                self.invalidate();
                return;
            }

            int index = -1;
            for (const auto& object : _availableCurrencies)
            {
                index++;

                if (index == itemIndex)
                {
                    auto& cfg = OpenLoco::Config::get();
                    cfg.preferredCurrency = object.header;

                    setPreferredCurrencyNameBuffer();
                    Config::write();
                    Scenario::loadPreferredCurrencyAlways();
                    ObjectManager::markOnlyLoadedObjects(_objectListSelection);

                    break;
                }
            }

            self.invalidate();
        }

        // 0x004C0F14
        static void preferredCurrencyNewGameMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.usePreferredCurrencyForNewGames ^= true;
            Config::write();

            self.invalidate();
        }

        // 0x004C0F27
        static void preferredCurrencyAlwaysMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.usePreferredCurrencyAlways ^= true;
            Config::write();

            Scenario::loadPreferredCurrencyAlways();
            ObjectManager::markOnlyLoadedObjects(_objectListSelection);

            self.invalidate();
        }

        // 0x004C0F49
        static void distanceSpeedMouseDown(const Window& self)
        {
            auto& dropdown = self.widgets[Widx::distance_speed];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::imperial);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::metric);
            Dropdown::setItemSelected(static_cast<uint8_t>(Config::get().measurementFormat));
        }

        // 0x004C0FB3
        static void distanceSpeedDropdown([[maybe_unused]] const Window& self, int16_t itemIndex)
        {
            if (itemIndex == -1)
            {
                return;
            }

            auto& cfg = Config::get();
            cfg.measurementFormat = Config::MeasurementFormat(itemIndex);

            // 0x004C0FC2
            cfg.heightMarkerOffset = 0;
            if (!cfg.showHeightAsUnits)
            {
                cfg.heightMarkerOffset = cfg.measurementFormat == Config::MeasurementFormat::imperial ? 1 : 2;
            }

            Config::write();
            Gfx::invalidateScreen();
        }

        // 0x004C0FFA
        static void heightsLabelsMouseDown(const Window& self)
        {
            auto& dropdown = self.widgets[Widx::heights];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 2, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::height_units);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::height_real_values);

            int selectedItem = 0;
            if (!Config::get().showHeightAsUnits)
            {
                selectedItem = 1;
            }
            Dropdown::setItemSelected(selectedItem);
        }

        // 0x004C106C
        static void heightsLabelsDropdown([[maybe_unused]] const Window& self, int16_t index)
        {
            if (index == -1)
            {
                return;
            }

            auto& cfg = Config::get();
            cfg.showHeightAsUnits = index == 0;

            // 0x004C0FC2
            cfg.heightMarkerOffset = 0;
            if (!cfg.showHeightAsUnits)
            {
                cfg.heightMarkerOffset = cfg.measurementFormat == Config::MeasurementFormat::imperial ? 1 : 2;
            }

            Config::write();
            Gfx::invalidateScreen();
        }

        // 0x004C1195
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, self.currentTab + 4);
        }

        static constexpr WindowEventList kEvents = {
            .onClose = Common::onClose,
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Controls
    {
        namespace Widx
        {
            enum
            {
                edge_scrolling = Common::Widx::tab_miscellaneous + 1,
                zoom_to_cursor,
                invertRightMouseViewPan,
                customize_keys
            };
        }

        static constexpr Ui::Size kWindowSize = { 366, 114 };

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize, StringIds::options_title_controls),
            Widgets::Checkbox({ 10, 49 }, { 346, 12 }, WindowColour::secondary, StringIds::scroll_screen_edge, StringIds::scroll_screen_edge_tip),
            Widgets::Checkbox({ 10, 64 }, { 346, 12 }, WindowColour::secondary, StringIds::zoom_to_cursor, StringIds::zoom_to_cursor_tip),
            Widgets::Checkbox({ 10, 79 }, { 346, 12 }, WindowColour::secondary, StringIds::invert_right_mouse_dragging, StringIds::tooltip_invert_right_mouse_dragging),
            Widgets::Button({ 26, 94 }, { 160, 12 }, WindowColour::secondary, StringIds::customise_keys, StringIds::customise_keys_tip)

        );

        static void edgeScrollingMouseUp(Window& self);
        static void zoomToCursorMouseUp(Window& self);
        static void invertRightMouseViewPan(Window& self);
        static void openKeyboardShortcuts();

        static void prepareDraw(Window& self)
        {
            assert(self.currentTab == Common::tab::controls);

            Common::prepareDraw(self);

            self.activatedWidgets &= ~(1ULL << Widx::edge_scrolling | 1ULL << Widx::zoom_to_cursor | 1ULL << Widx::invertRightMouseViewPan);
            if (Config::get().edgeScrolling)
            {
                self.activatedWidgets |= (1ULL << Widx::edge_scrolling);
            }
            if (Config::get().zoomToCursor)
            {
                self.activatedWidgets |= (1ULL << Widx::zoom_to_cursor);
            }
            if (Config::get().invertRightMouseViewPan)
            {
                self.activatedWidgets |= (1ULL << Widx::invertRightMouseViewPan);
            }
        }

        // 0x004C113F
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
        }

        // 0x004C114A
        static void onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            if (Common::onMouseUp(self, wi, id))
            {
                return;
            }

            switch (wi)
            {
                case Widx::customize_keys:
                    openKeyboardShortcuts();
                    break;

                case Widx::edge_scrolling:
                    edgeScrollingMouseUp(self);
                    break;

                case Widx::zoom_to_cursor:
                    zoomToCursorMouseUp(self);
                    break;

                case Widx::invertRightMouseViewPan:
                    invertRightMouseViewPan(self);
                    break;
            }
        }

        // 0x004C117A
        static void edgeScrollingMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.edgeScrolling = !cfg.edgeScrolling;
            Config::write();

            self.invalidate();
        }

        static void zoomToCursorMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.zoomToCursor = !cfg.zoomToCursor;
            Config::write();

            self.invalidate();
        }

        static void invertRightMouseViewPan(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.invertRightMouseViewPan = !cfg.invertRightMouseViewPan;
            Config::write();

            self.invalidate();
        }

        // 0x004C118D
        static void openKeyboardShortcuts()
        {
            KeyboardShortcuts::open();
        }

        // 0x004C1195
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, self.currentTab + 4);
        }

        static constexpr WindowEventList kEvents = {
            .onClose = Common::onClose,
            .onMouseUp = onMouseUp,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Company
    {
        static constexpr Ui::Size kWindowSize = { 420, 134 };

        namespace Widx
        {
            enum
            {
                groupPreferredOwner = Common::Widx::tab_miscellaneous + 1,

                usePreferredOwnerFace,
                changeOwnerFaceBtn,
                labelOwnerFace,

                usePreferredOwnerName,
                changeOwnerNameBtn,
                labelPreferredOwnerName,

                ownerFacePreview,
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize, StringIds::options_title_company),

            // Preferred owner group
            Widgets::GroupBox({ 4, 50 }, { 412, 80 }, WindowColour::secondary, StringIds::preferred_owner_name),

            // Preferred owner face
            Widgets::Checkbox({ 10, 64 }, { 400, 12 }, WindowColour::secondary, StringIds::usePreferredCompanyFace, StringIds::usePreferredCompanyFaceTip),
            Widgets::Button({ 265, 79 }, { 75, 12 }, WindowColour::secondary, StringIds::change),
            Widgets::Label({ 24, 79 }, { 240, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::currentPreferredFace),

            // Preferred owner name
            Widgets::Checkbox({ 10, 97 }, { 400, 12 }, WindowColour::secondary, StringIds::use_preferred_owner_name, StringIds::use_preferred_owner_name_tip),
            Widgets::Button({ 265, 112 }, { 75, 12 }, WindowColour::secondary, StringIds::change),
            Widgets::Label({ 24, 112 }, { 240, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::wcolour2_preferred_owner_name),

            // Preferred owner preview
            Widgets::ImageButton({ 345, 59 }, { 66, 66 }, WindowColour::secondary, Widget::kContentNull)

        );

        static void loadPreferredFace(Window& self)
        {
            if (WindowManager::getCurrentModalType() == WindowType::companyFaceSelection)
            {
                self.object = nullptr;
                return;
            }

            auto& preferredOwnerFace = Config::get().preferredOwnerFace;

            if (self.object != nullptr)
            {
                // Ensure our temporary object is still loaded
                auto* object = ObjectManager::getTemporaryObject();
                if (object == reinterpret_cast<Object*>(self.object))
                {
                    return;
                }
            }

            ObjectManager::freeTemporaryObject();
            if (ObjectManager::loadTemporaryObject(preferredOwnerFace))
            {
                self.object = reinterpret_cast<std::byte*>(ObjectManager::getTemporaryObject());
            }
            else
            {
                // Can't be loaded? Disable the option
                auto& cfg = Config::get();
                cfg.usePreferredOwnerFace = false;
                cfg.preferredOwnerFace = kEmptyObjectHeader;
                Config::write();

                self.object = nullptr;
            }
        }

        static void prepareDraw(Window& self)
        {
            assert(self.currentTab == Common::tab::company);

            Common::prepareDraw(self);

            if (Config::get().usePreferredOwnerName)
            {
                self.activatedWidgets |= (1ULL << Widx::usePreferredOwnerName);
            }
            else
            {
                self.disabledWidgets |= (1ULL << Widx::changeOwnerNameBtn);
            }

            if (Config::get().usePreferredOwnerFace)
            {
                self.activatedWidgets |= (1ULL << Widx::usePreferredOwnerFace);
            }
            else
            {
                self.disabledWidgets |= ((1ULL << Widx::changeOwnerFaceBtn) | (1ULL << Widx::ownerFacePreview));

                self.widgets[Widx::labelOwnerFace].text = StringIds::empty;
                self.widgets[Widx::ownerFacePreview].content = Widget::kContentNull;

                self.object = nullptr;
            }

            // Set preferred owner name.
            {
                // TODO: Do not share this buffer, also unsafe, we should change the localisation to use a string pointer.
                auto buffer = (char*)StringManager::getString(StringIds::buffer_2039);
                const char* playerName = Config::get().preferredOwnerName.c_str();
                strcpy(buffer, playerName);
                buffer[strlen(playerName)] = '\0';

                FormatArguments args{ self.widgets[Widx::labelPreferredOwnerName].textArgs };
                args.push(StringIds::buffer_2039);
            }

            // Set preferred owner face.
            if (self.object != nullptr)
            {
                const CompetitorObject* competitor = reinterpret_cast<CompetitorObject*>(self.object);

                self.widgets[Widx::labelOwnerFace].text = StringIds::currentPreferredFace;

                FormatArguments args{ self.widgets[Widx::labelOwnerFace].textArgs };
                args.push(competitor->name);

                self.widgets[Widx::ownerFacePreview].image = ImageId(competitor->images[0]).withIndexOffset(1).withPrimary(Colour::black).toUInt32();
            }

            loadPreferredFace(self);
        }

        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
        }

        // 0x004C1319
        static void changePreferredName(Window& self)
        {
            auto buffer = (char*)StringManager::getString(StringIds::buffer_2039);
            const char* playerName = Config::get().preferredOwnerName.c_str();
            strcpy(buffer, playerName);
            buffer[strlen(playerName)] = '\0';

            TextInput::openTextInput(&self, StringIds::preferred_owner_name, StringIds::enter_preferred_owner_name, StringIds::buffer_2039, Widx::usePreferredOwnerName, {});
        }

        // 0x004C135F
        static void usePreferredOwnerNameMouseUp(Window& self)
        {
            auto& cfg = Config::get();
            cfg.usePreferredOwnerName ^= true;
            Config::write();

            self.invalidate();

            if (cfg.usePreferredOwnerName && cfg.preferredOwnerName.empty())
            {
                changePreferredName(self);
            }
        }

        static void changePreferredFace(Window& self)
        {
            CompanyFaceSelection::open(CompanyId::neutral, self.type);
        }

        static void usePreferredOwnerFaceMouseUp(Window& self)
        {
            auto& cfg = Config::get();
            cfg.usePreferredOwnerFace ^= true;
            Config::write();

            self.invalidate();

            if (cfg.usePreferredOwnerFace && cfg.preferredOwnerFace == kEmptyObjectHeader)
            {
                changePreferredFace(self);
            }
        }

        static void onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            if (Common::onMouseUp(self, wi, id))
            {
                return;
            }

            switch (wi)
            {
                case Widx::usePreferredOwnerName:
                    usePreferredOwnerNameMouseUp(self);
                    break;

                case Widx::changeOwnerNameBtn:
                    changePreferredName(self);
                    break;

                case Widx::usePreferredOwnerFace:
                    usePreferredOwnerFaceMouseUp(self);
                    break;

                case Widx::changeOwnerFaceBtn:
                case Widx::ownerFacePreview:
                    changePreferredFace(self);
                    break;
            }
        }

        // 0x004C1342
        static void setPreferredName(Window& self, const char* str)
        {
            auto& cfg = Config::get();
            cfg.preferredOwnerName = str;
            if (cfg.preferredOwnerName.empty())
            {
                cfg.usePreferredOwnerName = false;
            }

            Config::write();
            self.invalidate();
        }

        // 0x004C1304
        static void textInput(Window& self, WidgetIndex_t i, [[maybe_unused]] const WidgetId id, const char* str)
        {
            switch (i)
            {
                case Widx::usePreferredOwnerName:
                    setPreferredName(self, str);
                    break;
            }
        }

        // 0x004C139C
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, self.currentTab + 4);
        }

        static constexpr WindowEventList kEvents = {
            .onClose = Common::onClose,
            .onMouseUp = onMouseUp,
            .onUpdate = onUpdate,
            .textInput = textInput,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Misc
    {
        static constexpr Ui::Size kWindowSize = { 420, 266 };

        namespace Widx
        {
            enum
            {
                groupCheats = Common::Widx::tab_miscellaneous + 1,
                enableCheatsToolbarButton,
                disableAICompanies,
                disableTownExpansion,

                groupVehicleBehaviour,
                disable_vehicle_breakdowns,
                disable_vehicle_load_penalty,
                disableStationSizeLimit,
                trainsReverseAtSignals,

                groupSaveOptions,
                autosave_frequency_label,
                autosave_frequency,
                autosave_frequency_btn,
                autosave_amount_label,
                autosave_amount,
                autosave_amount_down_btn,
                autosave_amount_up_btn,
                export_plugin_objects,
            };
        }

        static constexpr auto _widgets = makeWidgets(
            Common::makeCommonWidgets(kWindowSize, StringIds::options_title_miscellaneous),

            // Gameplay tweaks group
            Widgets::GroupBox({ 4, 49 }, { 412, 62 }, WindowColour::secondary, StringIds::gameplay_tweaks),
            Widgets::Checkbox({ 10, 64 }, { 400, 12 }, WindowColour::secondary, StringIds::option_cheat_menu_enable, StringIds::tooltip_option_cheat_menu_enable),
            Widgets::Checkbox({ 10, 79 }, { 400, 12 }, WindowColour::secondary, StringIds::disableAICompanies, StringIds::disableAICompanies_tip),
            Widgets::Checkbox({ 10, 94 }, { 400, 12 }, WindowColour::secondary, StringIds::disableTownExpansion, StringIds::disableTownExpansion_tip),

            // Vehicle behaviour
            Widgets::GroupBox({ 4, 115 }, { 412, 77 }, WindowColour::secondary, StringIds::vehicleTrackBehaviour),
            Widgets::Checkbox({ 10, 130 }, { 400, 12 }, WindowColour::secondary, StringIds::disable_vehicle_breakdowns),
            Widgets::Checkbox({ 10, 145 }, { 200, 12 }, WindowColour::secondary, StringIds::disableVehicleLoadingPenalty, StringIds::disableVehicleLoadingPenaltyTip),
            Widgets::Checkbox({ 10, 160 }, { 200, 12 }, WindowColour::secondary, StringIds::disableStationSizeLimitLabel, StringIds::disableStationSizeLimitTooltip),
            Widgets::Checkbox({ 10, 175 }, { 400, 12 }, WindowColour::secondary, StringIds::trainsReverseAtSignals),

            // Save options group
            Widgets::GroupBox({ 4, 196 }, { 412, 65 }, WindowColour::secondary, StringIds::autosave_preferences),

            Widgets::Label({ 10, 211 }, { 200, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::autosave_frequency),
            Widgets::dropdownWidgets({ 250, 211 }, { 156, 12 }, WindowColour::secondary, StringIds::empty),

            Widgets::Label({ 10, 226 }, { 200, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::autosave_amount),
            Widgets::stepperWidgets({ 250, 226 }, { 156, 12 }, WindowColour::secondary, StringIds::empty),

            Widgets::Checkbox({ 10, 241 }, { 400, 12 }, WindowColour::secondary, StringIds::export_plugin_objects, StringIds::export_plugin_objects_tip)

        );

        static void enableCheatsToolbarButtonMouseUp(Window& self);
        static void disableVehicleBreakdownsMouseUp(Window& self);
        static void trainsReverseAtSignalsMouseUp(Window& self);
        static void disableAICompaniesMouseUp(Window& self);
        static void disableTownExpansionMouseUp(Window& self);
        static void exportPluginObjectsMouseUp(Window& self);

        // 0x004C11B7
        static void prepareDraw(Window& self)
        {
            assert(self.currentTab == Common::tab::miscellaneous);

            Common::prepareDraw(self);

            if (Config::get().cheatsMenuEnabled)
            {
                self.activatedWidgets |= (1ULL << Widx::enableCheatsToolbarButton);
            }

            if (Config::get().breakdownsDisabled)
            {
                self.activatedWidgets |= (1ULL << Widx::disable_vehicle_breakdowns);
            }

            if (Config::get().trainsReverseAtSignals)
            {
                self.activatedWidgets |= (1ULL << Widx::trainsReverseAtSignals);
            }

            if (Config::get().disableVehicleLoadPenaltyCheat)
            {
                self.activatedWidgets |= (1ULL << Widx::disable_vehicle_load_penalty);
            }

            if (Config::get().disableStationSizeLimit)
            {
                self.activatedWidgets |= (1ULL << Widx::disableStationSizeLimit);
            }

            if (Config::get().companyAIDisabled)
            {
                self.activatedWidgets |= (1ULL << Widx::disableAICompanies);
            }

            if (Config::get().townGrowthDisabled)
            {
                self.activatedWidgets |= (1ULL << Widx::disableTownExpansion);
            }

            if (Config::get().exportObjectsWithSaves)
            {
                self.activatedWidgets |= (1ULL << Widx::export_plugin_objects);
            }

            self.widgets[Widx::export_plugin_objects].hidden = !ObjectManager::getCustomObjectsInIndexStatus();
        }

        static void drawDropdownContent(const Window& self, Gfx::DrawingContext& drawingCtx, WidgetIndex_t widgetIndex, StringId stringId, int32_t value)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            auto& widget = self.widgets[widgetIndex];
            FormatArguments args{};
            args.push(stringId);
            args.push(value);

            auto point = Point(self.x + widget.left + 1, self.y + widget.top + 1);
            tr.drawStringLeft(point, Colour::black, StringIds::black_stringid, args);
        }

        // 0x004C1282
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);

            // Value for autosave frequency
            auto freq = Config::get().autosaveFrequency;
            StringId stringId;
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
            drawDropdownContent(self, drawingCtx, Widx::autosave_frequency, stringId, freq);

            // Value for autosave amount
            auto scale = Config::get().autosaveAmount;
            drawDropdownContent(self, drawingCtx, Widx::autosave_amount, StringIds::int_32, scale);
        }

        static void changeAutosaveAmount(Window& self, int32_t delta)
        {
            auto& cfg = Config::get();
            auto newValue = std::clamp(cfg.autosaveAmount + delta, 1, 24);
            if (cfg.autosaveAmount != newValue)
            {
                cfg.autosaveAmount = newValue;
                Config::write();
                self.invalidate();
            }
        }

        static void changeAutosaveFrequency(Window& self, int32_t value)
        {
            auto& cfg = Config::get();
            if (cfg.autosaveFrequency != value)
            {
                cfg.autosaveFrequency = value;
                Config::write();
                self.invalidate();
            }
        }

        static void showAutosaveFrequencyDropdown(const Window& self, WidgetIndex_t wi)
        {
            auto dropdown = self.widgets[wi];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 5, 0x80);

            // Add pre-defined entries
            Dropdown::add(0, StringIds::dropdown_stringid, { StringIds::autosave_never });
            Dropdown::add(1, StringIds::dropdown_stringid, { StringIds::autosave_every_month });
            Dropdown::add(2, StringIds::dropdown_stringid, { StringIds::autosave_every_x_months, static_cast<uint32_t>(3) });
            Dropdown::add(3, StringIds::dropdown_stringid, { StringIds::autosave_every_x_months, static_cast<uint32_t>(6) });
            Dropdown::add(4, StringIds::dropdown_stringid, { StringIds::autosave_every_x_months, static_cast<uint32_t>(12) });

            // Set current selection
            auto freq = Config::get().autosaveFrequency;
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

        static void handleAutosaveFrequencyDropdown(Window& self, int32_t index)
        {
            switch (index)
            {
                case 0:
                    changeAutosaveFrequency(self, 0);
                    break;
                case 1:
                    changeAutosaveFrequency(self, 1);
                    break;
                case 2:
                    changeAutosaveFrequency(self, 3);
                    break;
                case 3:
                    changeAutosaveFrequency(self, 6);
                    break;
                case 4:
                    changeAutosaveFrequency(self, 12);
                    break;
            }
        }

        // 0x004C12D2
        static void onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            if (Common::onMouseUp(self, wi, id))
            {
                return;
            }

            switch (wi)
            {
                case Widx::enableCheatsToolbarButton:
                    enableCheatsToolbarButtonMouseUp(self);
                    break;

                case Widx::disable_vehicle_breakdowns:
                    disableVehicleBreakdownsMouseUp(self);
                    break;

                case Widx::trainsReverseAtSignals:
                    trainsReverseAtSignalsMouseUp(self);
                    break;

                case Widx::disable_vehicle_load_penalty:
                    Config::get().disableVehicleLoadPenaltyCheat = !Config::get().disableVehicleLoadPenaltyCheat;
                    WindowManager::invalidateWidget(self.type, self.number, Widx::disable_vehicle_load_penalty);
                    break;

                case Widx::disableStationSizeLimit:
                    Config::get().disableStationSizeLimit ^= true;
                    WindowManager::invalidateWidget(self.type, self.number, Widx::disableStationSizeLimit);
                    break;

                case Widx::disableAICompanies:
                    disableAICompaniesMouseUp(self);
                    break;

                case Widx::disableTownExpansion:
                    disableTownExpansionMouseUp(self);
                    break;

                case Widx::export_plugin_objects:
                    exportPluginObjectsMouseUp(self);
                    break;
            }
        }

        static void onMouseDown(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
        {
            switch (wi)
            {
                case Widx::autosave_frequency_btn:
                    showAutosaveFrequencyDropdown(self, Widx::autosave_frequency);
                    break;
                case Widx::autosave_amount_down_btn:
                    changeAutosaveAmount(self, -1);
                    break;
                case Widx::autosave_amount_up_btn:
                    changeAutosaveAmount(self, 1);
                    break;
            }
        }

        static void onDropdown(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id, int16_t item_index)
        {
            switch (wi)
            {
                case Widx::autosave_frequency_btn:
                    handleAutosaveFrequencyDropdown(self, item_index);
                    break;
            }
        }

        static void enableCheatsToolbarButtonMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.cheatsMenuEnabled = !cfg.cheatsMenuEnabled;
            Config::write();
            self.invalidate();
            WindowManager::invalidate(WindowType::topToolbar);
        }

        static void disableVehicleBreakdownsMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.breakdownsDisabled = !cfg.breakdownsDisabled;
            Config::write();
            self.invalidate();
        }

        static void trainsReverseAtSignalsMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.trainsReverseAtSignals = !cfg.trainsReverseAtSignals;
            Config::write();
            self.invalidate();
        }

        static void disableAICompaniesMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.companyAIDisabled = !cfg.companyAIDisabled;
            Config::write();
            self.invalidate();
        }

        static void disableTownExpansionMouseUp(Window& self)
        {
            auto& cfg = OpenLoco::Config::get();
            cfg.townGrowthDisabled = !cfg.townGrowthDisabled;
            Config::write();
            self.invalidate();
        }

        static void exportPluginObjectsMouseUp(Window& self)
        {
            auto& cfg = Config::get();
            cfg.exportObjectsWithSaves ^= true;
            Config::write();

            self.invalidate();
        }

        // 0x004C139C
        static void onUpdate(Window& self)
        {
            self.frameNo += 1;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(self.type, self.number, self.currentTab + 4);
        }

        static constexpr WindowEventList kEvents = {
            .onClose = Common::onClose,
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    static void prepareObjectSelectionList()
    {
        _objectListSelection.resize(ObjectManager::getNumInstalledObjects());
        ObjectManager::markOnlyLoadedObjects(_objectListSelection);
    }

    static void disableTabsByCurrentScene(Window& self)
    {
        self.disabledWidgets &= ~(1ULL << Common::Widx::tab_regional);

        if (SceneManager::isEditorMode() && Scenario::getOptions().editorStep == EditorController::Step::objectSelection)
        {
            self.disabledWidgets |= 1ULL << Common::Widx::tab_regional;
        }

        Widget::leftAlignTabs(self, Common::Widx::tab_display, Common::Widx::tab_miscellaneous);
    }

    // 0x004C1519 & 0x00474911
    static void setPreferredCurrencyNameBuffer()
    {
        const auto res = ObjectManager::findObjectInIndex(Config::get().preferredCurrency);
        if (res.has_value())
        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::preferred_currency_buffer));
            strcpy(buffer, res->_name.c_str());
        }
    }

    // 0x004BF7B9
    Window* open()
    {
        Window* window = WindowManager::bringToFront(WindowType::options);
        if (window != nullptr)
        {
            return window;
        }

        // 0x004BF833 (create_options_window)
        window = WindowManager::createWindowCentred(
            WindowType::options,
            Display::kWindowSize,
            WindowFlags::none,
            Display::getEvents());

        window->setWidgets(Display::_widgets);
        window->number = 0;
        window->currentTab = 0;
        window->frameNo = 0;
        window->rowHover = -1;
        window->object = nullptr;

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->windowTitlebarColour);
        window->setColour(WindowColour::secondary, interface->windowOptionsColour);

        prepareObjectSelectionList();
        populateAvailableCurrencies();
        setPreferredCurrencyNameBuffer();

        Display::applyScreenModeRestrictions(*window);

        window->holdableWidgets = 0;
        window->eventHandlers = &Display::getEvents();
        window->activatedWidgets = 0;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        return window;
    }

    // 0x004BF823
    Window* openMusicSettings()
    {
        auto* window = open();

        window->callOnMouseUp(Common::Widx::tab_audio, window->widgets[Common::Widx::tab_audio].id);

        return window;
    }

    struct TabInformation
    {
        std::span<const Widget> widgets;
        const WindowEventList& events;
        Ui::Size kWindowSize;
    };

    // clang-format off
    static TabInformation kTabInformationByTabOffset[] = {
        { Display::_widgets,   Display::getEvents(),   Display::kWindowSize  },
        { Rendering::_widgets, Rendering::getEvents(), Rendering::kWindowSize  },
        { AudioTab::_widgets,  AudioTab::getEvents(),  AudioTab::kWindowSize },
        { Regional::_widgets,  Regional::getEvents(),  Regional::kWindowSize },
        { Controls::_widgets,  Controls::getEvents(),  Controls::kWindowSize },
        { Company::_widgets,   Company::getEvents(),   Company::kWindowSize  },
        { Misc::_widgets,      Misc::getEvents(),      Misc::kWindowSize     },
    };
    // clang-format on

    // 0x004BFC11
    static void tabOnMouseUp(Window& self, WidgetIndex_t wi)
    {
        ToolManager::toolCancel(self.type, self.number);

        TextInput::sub_4CE6C9(self.type, self.number);
        self.currentTab = wi - Common::Widx::tab_display;
        self.frameNo = 0;
        self.flags &= ~(WindowFlags::beingResized);
        self.disabledWidgets = 0;
        self.holdableWidgets = 0;
        self.activatedWidgets = 0;
        self.rowHover = -1;
        self.viewportRemove(0);

        auto& tabInfo = kTabInformationByTabOffset[self.currentTab];
        self.eventHandlers = &tabInfo.events;
        self.setWidgets(tabInfo.widgets);
        self.invalidate();
        self.setSize(tabInfo.kWindowSize);

        if ((Common::tab)self.currentTab == Common::tab::display)
        {
            Display::applyScreenModeRestrictions(self);
        }

        else if ((Common::tab)self.currentTab == Common::tab::audio)
        {
            self.holdableWidgets = (1ULL << AudioTab::Widx::music_volume);
        }

        self.callOnResize();
        self.callPrepareDraw();
        self.initScrollWidgets();
        self.invalidate();
    }
}
