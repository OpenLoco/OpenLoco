#include "Date.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/SetGameSpeed.h"
#include "GameCommands/General/TogglePause.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Intro.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Network/Network.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Scenario.h"
#include "ScenarioObjective.h"
#include "SceneManager.h"
#include "Ui.h"
#include "Ui/Dropdown.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TimePanel
{
    static constexpr Ui::Size kWindowSize = { 145, 27 };

    namespace Widx
    {
        enum
        {
            outer_frame,
            inner_frame,
            map_chat_menu,
            date_btn,
            pause_btn,
            normal_speed_btn,
            fast_forward_btn,
            extra_fast_forward_btn,
        };
    }

    static void formatChallenge(FormatArguments& args);
    static void sendChatMessage(const char* str);

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 140, 29 }, WidgetType::wt_3, WindowColour::primary),                                                                                                   // 0,
        makeWidget({ 2, 2 }, { 136, 25 }, WidgetType::wt_3, WindowColour::primary),                                                                                                   // 1,
        makeWidget({ 113, 1 }, { 26, 26 }, WidgetType::buttonWithImage, WindowColour::primary),                                                                                       // 2,
        makeWidget({ 2, 2 }, { 111, 12 }, WidgetType::buttonWithImage, WindowColour::primary, Widget::kContentNull, StringIds::tooltip_daymonthyear_challenge),                       // 3,
        makeRemapWidget({ 18, 15 }, { 20, 12 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::speed_pause, StringIds::tooltip_speed_pause),                           // 4,
        makeRemapWidget({ 38, 15 }, { 20, 12 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::speed_normal, StringIds::tooltip_speed_normal),                         // 5,
        makeRemapWidget({ 58, 15 }, { 20, 12 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::speed_fast_forward, StringIds::tooltip_speed_fast_forward),             // 6,
        makeRemapWidget({ 78, 15 }, { 20, 12 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::speed_extra_fast_forward, StringIds::tooltip_speed_extra_fast_forward), // 7,
        widgetEnd(),
    };

    static loco_global<uint16_t, 0x0050A004> _50A004;

    static loco_global<uint16_t[8], 0x112C826> _commonFormatArgs;

    static const WindowEventList& getEvents();

    Window* open()
    {
        auto window = WindowManager::createWindow(
            WindowType::timeToolbar,
            Ui::Point(Ui::width() - kWindowSize.width, Ui::height() - kWindowSize.height),
            Ui::Size(kWindowSize.width, kWindowSize.height),
            Ui::WindowFlags::stickToFront | Ui::WindowFlags::transparent | Ui::WindowFlags::noBackground,
            getEvents());
        window->widgets = _widgets;
        window->enabledWidgets = (1 << Widx::map_chat_menu) | (1 << Widx::date_btn) | (1 << Widx::pause_btn) | (1 << Widx::normal_speed_btn) | (1 << Widx::fast_forward_btn) | (1 << Widx::extra_fast_forward_btn);
        window->var_854 = 0;
        window->var_856 = 0;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::primary, AdvancedColour(skin->colour_17).translucent());
            window->setColour(WindowColour::secondary, AdvancedColour(skin->colour_17).translucent());
        }

        return window;
    }

    // 0x004396A4
    static void prepareDraw([[maybe_unused]] Window& window)
    {
        _widgets[Widx::inner_frame].type = WidgetType::none;
        _widgets[Widx::pause_btn].image = Gfx::recolour(ImageIds::speed_pause);
        _widgets[Widx::normal_speed_btn].image = Gfx::recolour(ImageIds::speed_normal);
        _widgets[Widx::fast_forward_btn].image = Gfx::recolour(ImageIds::speed_fast_forward);
        _widgets[Widx::extra_fast_forward_btn].image = Gfx::recolour(ImageIds::speed_extra_fast_forward);

        if (isPaused())
        {
            _widgets[Widx::pause_btn].image = Gfx::recolour(ImageIds::speed_pause_active);
        }
        else if (getGameSpeed() == GameSpeed::Normal)
        {
            _widgets[Widx::normal_speed_btn].image = Gfx::recolour(ImageIds::speed_normal_active);
        }
        else if (getGameSpeed() == GameSpeed::FastForward)
        {
            _widgets[Widx::fast_forward_btn].image = Gfx::recolour(ImageIds::speed_fast_forward_active);
        }
        else if (getGameSpeed() == GameSpeed::ExtraFastForward)
        {
            _widgets[Widx::extra_fast_forward_btn].image = Gfx::recolour(ImageIds::speed_extra_fast_forward_active);
        }

        if (isNetworked())
        {
            _widgets[Widx::fast_forward_btn].type = WidgetType::none;
            _widgets[Widx::extra_fast_forward_btn].type = WidgetType::none;

            _widgets[Widx::pause_btn].left = 38;
            _widgets[Widx::pause_btn].right = 57;
            _widgets[Widx::normal_speed_btn].left = 58;
            _widgets[Widx::normal_speed_btn].right = 77;
        }
        else
        {
            _widgets[Widx::fast_forward_btn].type = WidgetType::buttonWithImage;
            _widgets[Widx::extra_fast_forward_btn].type = WidgetType::buttonWithImage;

            _widgets[Widx::pause_btn].left = 18;
            _widgets[Widx::pause_btn].right = 37;
            _widgets[Widx::normal_speed_btn].left = 38;
            _widgets[Widx::normal_speed_btn].right = 57;
            _widgets[Widx::fast_forward_btn].left = 58;
            _widgets[Widx::fast_forward_btn].right = 77;
            _widgets[Widx::extra_fast_forward_btn].left = 78;
            _widgets[Widx::extra_fast_forward_btn].right = 97;
        }
    }

    // TODO: use same list as top toolbar
    static const uint32_t map_sprites_by_rotation[] = {
        InterfaceSkin::ImageIds::toolbar_menu_map_north,
        InterfaceSkin::ImageIds::toolbar_menu_map_west,
        InterfaceSkin::ImageIds::toolbar_menu_map_south,
        InterfaceSkin::ImageIds::toolbar_menu_map_east,
    };

    // 0x004397BE
    static void draw(Ui::Window& self, Gfx::RenderTarget* rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        Widget& frame = _widgets[Widx::outer_frame];
        drawingCtx.drawRect(*rt, self.x + frame.left, self.y + frame.top, frame.width(), frame.height(), enumValue(ExtColour::unk34), Drawing::RectFlags::transparent);

        // Draw widgets.
        self.draw(rt);

        drawingCtx.drawRectInset(*rt, self.x + frame.left + 1, self.y + frame.top + 1, frame.width() - 2, frame.height() - 2, self.getColour(WindowColour::secondary), Drawing::RectInsetFlags::borderInset | Drawing::RectInsetFlags::fillNone);

        *(uint32_t*)&_commonFormatArgs[0] = getCurrentDay();
        StringId format = StringIds::date_daymonthyear;

        if (isPaused() && (getPauseFlags() & (1 << 2)) == 0)
        {
            if (self.var_856 >= 30)
            {
                format = StringIds::toolbar_status_paused;
            }
        }

        auto c = self.getColour(WindowColour::primary).opaque();
        if (Input::isHovering(WindowType::timeToolbar, 0, Widx::date_btn))
        {
            c = Colour::white;
        }
        drawingCtx.drawStringCentred(*rt, self.x + _widgets[Widx::date_btn].midX(), self.y + _widgets[Widx::date_btn].top + 1, c, format, &*_commonFormatArgs);

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        drawingCtx.drawImage(rt, self.x + _widgets[Widx::map_chat_menu].left - 2, self.y + _widgets[Widx::map_chat_menu].top - 1, skin->img + map_sprites_by_rotation[WindowManager::getCurrentRotation()]);
    }

    // 0x004398FB
    static void onMouseUp([[maybe_unused]] Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::date_btn:
                MessageWindow::open();
                break;
            case Widx::pause_btn:
                GameCommands::doCommand(GameCommands::PauseGameArgs{}, GameCommands::Flags::apply);
                break;
            case Widx::normal_speed_btn:
                GameCommands::doCommand(GameCommands::SetGameSpeedArgs{ GameSpeed::Normal }, GameCommands::Flags::apply);
                break;
            case Widx::fast_forward_btn:
                GameCommands::doCommand(GameCommands::SetGameSpeedArgs{ GameSpeed::FastForward }, GameCommands::Flags::apply);
                break;
            case Widx::extra_fast_forward_btn:
                GameCommands::doCommand(GameCommands::SetGameSpeedArgs{ GameSpeed::ExtraFastForward }, GameCommands::Flags::apply);
                break;
        }
    }

    // 0x0043A67F
    static void mapMouseDown(Ui::Window* self, WidgetIndex_t widgetIndex)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();

        if (isNetworked())
        {
            Dropdown::add(0, StringIds::menu_sprite_stringid, { (uint32_t)skin->img + InterfaceSkin::ImageIds::phone, StringIds::chat_send_message });
            Dropdown::add(1, StringIds::menu_sprite_stringid, { (uint32_t)skin->img + map_sprites_by_rotation[WindowManager::getCurrentRotation()], StringIds::menu_map });
            Dropdown::showBelow(self, widgetIndex, 2, 25, (1 << 6));
            Dropdown::setHighlightedItem(1);
        }
        else
        {
            Dropdown::add(0, StringIds::menu_sprite_stringid, { (uint32_t)skin->img + map_sprites_by_rotation[WindowManager::getCurrentRotation()], StringIds::menu_map });
            Dropdown::showBelow(self, widgetIndex, 1, 25, (1 << 6));
            Dropdown::setHighlightedItem(0);
        }
    }

    void beginSendChatMessage(Window& self)
    {
        const auto* opponent = CompanyManager::getOpponent();
        FormatArguments args{};
        args.push(opponent->name);

        // TODO: convert this to a builder pattern, with chainable functions to set the different string ids and arguments
        TextInput::openTextInput(&self, StringIds::chat_title, StringIds::chat_instructions, StringIds::empty, Widx::map_chat_menu, &args);
    }

    // 0x0043A72F
    static void mapDropdown(Window* self, [[maybe_unused]] WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (isNetworked())
        {
            switch (itemIndex)
            {
                case 0:
                    beginSendChatMessage(*self);
                    break;
                case 1:
                    MapWindow::open();
                    break;
            }
        }
        else
        {
            switch (itemIndex)
            {
                case 0:
                    MapWindow::open();
                    break;
            }
        }
    }

    // 0x043992E
    static void onMouseDown(Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::map_chat_menu:
                mapMouseDown(&window, widgetIndex);
                break;
        }
    }

    // 0x439939
    static void onDropdown(Window& w, WidgetIndex_t widgetIndex, int16_t item_index)
    {
        switch (widgetIndex)
        {
            case Widx::map_chat_menu:
                mapDropdown(&w, widgetIndex, item_index);
                break;
        }
    }

    // 0x00439944
    static Ui::CursorId onCursor([[maybe_unused]] Ui::Window& self, int16_t widgetIdx, [[maybe_unused]] int16_t xPos, [[maybe_unused]] int16_t yPos, Ui::CursorId fallback)
    {
        switch (widgetIdx)
        {
            case Widx::date_btn:
                Input::setTooltipTimeout(2000);
                break;
        }

        return fallback;
    }

    // 0x00439955
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        switch (widgetIndex)
        {
            case Widx::date_btn:
                formatChallenge(args);
                break;
        }
        return args;
    }

    // 0x0043995C
    static void formatChallenge(FormatArguments& args)
    {
        args.push(getCurrentDay());

        auto playerCompany = CompanyManager::get(CompanyManager::getControllingId());

        if ((playerCompany->challengeFlags & CompanyFlags::challengeCompleted) != CompanyFlags::none)
        {
            args.push(StringIds::challenge_completed);
        }
        else if ((playerCompany->challengeFlags & CompanyFlags::challengeFailed) != CompanyFlags::none)
        {
            args.push(StringIds::challenge_failed);
        }
        else if ((playerCompany->challengeFlags & CompanyFlags::challengeBeatenByOpponent) != CompanyFlags::none)
        {
            args.push(StringIds::empty);
        }
        else
        {
            args.push(StringIds::challenge_progress);
            args.push<uint16_t>(playerCompany->challengeProgress);

            if ((Scenario::getObjective().flags & Scenario::ObjectiveFlags::withinTimeLimit) != Scenario::ObjectiveFlags::none)
            {
                uint16_t monthsLeft = (Scenario::getObjective().timeLimitYears * 12 - Scenario::getObjectiveProgress().monthsInChallenge);
                uint16_t yearsLeft = monthsLeft / 12;
                monthsLeft = monthsLeft % 12;
                args.push(StringIds::challenge_time_left);
                args.push(yearsLeft);
                args.push(monthsLeft);
            }
            else
            {
                args.push(StringIds::empty);
            }
        }
    }

    // 0x00439A15
    static void textInput([[maybe_unused]] Window& w, WidgetIndex_t widgetIndex, const char* str)
    {
        switch (widgetIndex)
        {
            case Widx::map_chat_menu:
                sendChatMessage(str);
                break;
        }
    }

    static void sendChatMessage(const char* string)
    {
        Network::sendChatMessage(string);
    }

    void invalidateFrame()
    {
        _50A004 = _50A004 | (1 << 1);
    }

    // 0x00439AD9
    static void onUpdate(Window& w)
    {
        w.var_854 += 1;
        if (w.var_854 >= 24)
        {
            w.var_854 = 0;
        }

        w.var_856 += 1;
        if (w.var_856 >= 60)
        {
            w.var_856 = 0;
        }

        if (_50A004 & (1 << 1))
        {
            _50A004 = _50A004 & ~(1 << 1);
            WindowManager::invalidateWidget(WindowType::timeToolbar, 0, Widx::inner_frame);
        }

        if (isPaused())
        {
            WindowManager::invalidateWidget(WindowType::timeToolbar, 0, Widx::inner_frame);
        }
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .event_03 = onMouseDown,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .textInput = textInput,
        .tooltip = tooltip,
        .cursor = onCursor,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
