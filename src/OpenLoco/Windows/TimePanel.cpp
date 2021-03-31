#include "../CompanyManager.h"
#include "../Date.h"
#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::TimePanel
{
    static const Gfx::ui_size_t window_size = { 140, 27 };

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
    static void processChatMessage(const char* str);
    static void togglePaused();
    static void changeGameSpeed(window* w, uint8_t speed);

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 140, 29 }, widget_type::wt_3, 0),                                                                                        // 0,
        makeWidget({ 2, 2 }, { 136, 25 }, widget_type::wt_3, 0),                                                                                        // 1,
        makeWidget({ 113, 1 }, { 26, 26 }, widget_type::wt_9, 0),                                                                                       // 2,
        makeWidget({ 2, 2 }, { 111, 12 }, widget_type::wt_9, 0, ImageIds::null, StringIds::tooltip_daymonthyear_challenge),                             // 3,
        makeRemapWidget({ 18, 15 }, { 20, 12 }, widget_type::wt_9, 0, ImageIds::speed_pause, StringIds::tooltip_speed_pause),                           // 4,
        makeRemapWidget({ 38, 15 }, { 20, 12 }, widget_type::wt_9, 0, ImageIds::speed_normal, StringIds::tooltip_speed_normal),                         // 5,
        makeRemapWidget({ 58, 15 }, { 20, 12 }, widget_type::wt_9, 0, ImageIds::speed_fast_forward, StringIds::tooltip_speed_fast_forward),             // 6,
        makeRemapWidget({ 78, 15 }, { 20, 12 }, widget_type::wt_9, 0, ImageIds::speed_extra_fast_forward, StringIds::tooltip_speed_extra_fast_forward), // 7,
        widgetEnd(),
    };

    static window_event_list _events;

    static void prepareDraw(window* window);
    static void draw(Ui::window* self, Gfx::drawpixelinfo_t* dpi);
    static void onMouseUp(Ui::window* window, widget_index widgetIndex);
    static void onMouseDown(Ui::window* window, widget_index widgetIndex);
    static void textInput(window* w, widget_index widgetIndex, const char* str);
    static void onDropdown(window* w, widget_index widgetIndex, int16_t item_index);
    static Ui::cursor_id onCursor(window* w, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::cursor_id fallback);
    static std::optional<FormatArguments> tooltip(Ui::window* window, widget_index widgetIndex);
    static void onUpdate(window* w);

    static loco_global<uint16_t, 0x0050A004> _50A004;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;

    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;
    static loco_global<uint16_t, 0x00526243> objectiveMonthsInChallenge;

    loco_global<uint16_t[8], 0x112C826> _common_format_args;

    window* open()
    {
        _events.on_mouse_up = onMouseUp;
        _events.event_03 = onMouseDown;
        _events.on_mouse_down = onMouseDown;
        _events.on_dropdown = onDropdown;
        _events.on_update = onUpdate;
        _events.text_input = textInput;
        _events.tooltip = tooltip;
        _events.cursor = onCursor;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;

        auto window = WindowManager::createWindow(
            WindowType::timeToolbar,
            Gfx::point_t(Ui::width() - window_size.width, Ui::height() - window_size.height),
            Gfx::ui_size_t(window_size.width, window_size.height),
            Ui::WindowFlags::stick_to_front | Ui::WindowFlags::transparent | Ui::WindowFlags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << Widx::map_chat_menu) | (1 << Widx::date_btn) | (1 << Widx::pause_btn) | (1 << Widx::normal_speed_btn) | (1 << Widx::fast_forward_btn) | (1 << Widx::extra_fast_forward_btn);
        window->var_854 = 0;
        window->var_856 = 0;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->colours[0] = Colour::translucent(skin->colour_17);
            window->colours[1] = Colour::translucent(skin->colour_17);
        }

        return window;
    }

    // 0x004396A4
    static void prepareDraw(window* window)
    {
        _widgets[Widx::inner_frame].type = widget_type::none;
        _widgets[Widx::pause_btn].image = Gfx::recolour(ImageIds::speed_pause);
        _widgets[Widx::normal_speed_btn].image = Gfx::recolour(ImageIds::speed_normal);
        _widgets[Widx::fast_forward_btn].image = Gfx::recolour(ImageIds::speed_fast_forward);
        _widgets[Widx::extra_fast_forward_btn].image = Gfx::recolour(ImageIds::speed_extra_fast_forward);

        if (isPaused())
        {
            _widgets[Widx::pause_btn].image = Gfx::recolour(ImageIds::speed_pause_active);
        }
        else if (getGameSpeed() == 0)
        {
            _widgets[Widx::normal_speed_btn].image = Gfx::recolour(ImageIds::speed_normal_active);
        }
        else if (getGameSpeed() == 1)
        {
            _widgets[Widx::fast_forward_btn].image = Gfx::recolour(ImageIds::speed_fast_forward_active);
        }
        else if (getGameSpeed() == 2)
        {
            _widgets[Widx::extra_fast_forward_btn].image = Gfx::recolour(ImageIds::speed_extra_fast_forward_active);
        }

        if (isNetworked())
        {
            _widgets[Widx::fast_forward_btn].type = widget_type::none;
            _widgets[Widx::extra_fast_forward_btn].type = widget_type::none;

            _widgets[Widx::pause_btn].left = 38;
            _widgets[Widx::pause_btn].right = 57;
            _widgets[Widx::normal_speed_btn].left = 58;
            _widgets[Widx::normal_speed_btn].right = 77;
        }
        else
        {
            _widgets[Widx::fast_forward_btn].type = widget_type::wt_9;
            _widgets[Widx::extra_fast_forward_btn].type = widget_type::wt_9;

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
    static void draw(Ui::window* self, Gfx::drawpixelinfo_t* dpi)
    {
        widget_t& frame = _widgets[Widx::outer_frame];
        Gfx::drawRect(dpi, self->x + frame.left, self->y + frame.top, frame.width(), frame.height(), 0x2000000 | 52);

        // Draw widgets.
        self->draw(dpi);

        Gfx::drawRectInset(dpi, self->x + frame.left + 1, self->y + frame.top + 1, frame.width() - 2, frame.height() - 2, self->colours[1], 0x30);

        *(uint32_t*)&_common_format_args[0] = getCurrentDay();
        string_id format = StringIds::date_monthyear;

        if (isPaused() && (getPauseFlags() & (1 << 2)) == 0)
        {
            if (self->var_856 >= 30)
            {
                format = StringIds::toolbar_status_paused;
            }
        }

        colour_t c = Colour::opaque(self->colours[0]);
        if (Input::isHovering(WindowType::timeToolbar, 0, Widx::date_btn))
        {
            c = Colour::white;
        }
        Gfx::drawStringCentred(*dpi, self->x + _widgets[Widx::date_btn].mid_x(), self->y + _widgets[Widx::date_btn].top + 1, c, format, &*_common_format_args);

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        Gfx::drawImage(dpi, self->x + _widgets[Widx::map_chat_menu].left - 2, self->y + _widgets[Widx::map_chat_menu].top - 1, skin->img + map_sprites_by_rotation[gCurrentRotation]);
    }

    // 0x004398FB
    static void onMouseUp(Ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::date_btn:
                MessageWindow::open();
                break;
            case Widx::pause_btn:
                togglePaused();
                break;
            case Widx::normal_speed_btn:
                changeGameSpeed(window, 0);
                break;
            case Widx::fast_forward_btn:
                changeGameSpeed(window, 1);
                break;
            case Widx::extra_fast_forward_btn:
                changeGameSpeed(window, 2);
                break;
        }
    }

    // 0x0043A67F
    static void mapMouseDown(Ui::window* self, widget_index widgetIndex)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();

        if (isNetworked())
        {
            Dropdown::add(0, StringIds::menu_sprite_stringid, { (uint32_t)skin->img + InterfaceSkin::ImageIds::phone, StringIds::chat_send_message });
            Dropdown::add(1, StringIds::menu_sprite_stringid, { (uint32_t)skin->img + map_sprites_by_rotation[gCurrentRotation], StringIds::menu_map });
            Dropdown::showBelow(self, widgetIndex, 2, 25, (1 << 6));
            Dropdown::setHighlightedItem(1);
        }
        else
        {
            Dropdown::add(0, StringIds::menu_sprite_stringid, { (uint32_t)skin->img + map_sprites_by_rotation[gCurrentRotation], StringIds::menu_map });
            Dropdown::showBelow(self, widgetIndex, 1, 25, (1 << 6));
            Dropdown::setHighlightedItem(0);
        }
    }

    // 0x0043A72F
    static void mapDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = Dropdown::getHighlightedItem();

        if (isNetworked())
        {
            switch (itemIndex)
            {
                case 0:
                {
                    auto opponent = CompanyManager::getOpponent();
                    _common_format_args[4] = opponent->owner_name;
                    Ui::Windows::TextInput::openTextInput(self, StringIds::chat_title, StringIds::chat_instructions, StringIds::empty, widgetIndex, &*_common_format_args);
                    break;
                }
                case 1:
                    Windows::Map::open();
                    break;
            }
        }
        else
        {
            switch (itemIndex)
            {
                case 0:
                    Windows::Map::open();
                    break;
            }
        }
    }

    // 0x043992E
    static void onMouseDown(Ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::map_chat_menu:
                mapMouseDown(window, widgetIndex);
                break;
        }
    }

    // 0x439939
    static void onDropdown(window* w, widget_index widgetIndex, int16_t item_index)
    {
        switch (widgetIndex)
        {
            case Widx::map_chat_menu:
                mapDropdown(w, widgetIndex, item_index);
                break;
        }
    }

    // 0x00439944
    static Ui::cursor_id onCursor(Ui::window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::cursor_id fallback)
    {
        switch (widgetIdx)
        {
            case Widx::date_btn:
                _tooltipTimeout = 2000;
                break;
        }

        return fallback;
    }

    // 0x00439955
    static std::optional<FormatArguments> tooltip(Ui::window* window, widget_index widgetIndex)
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

        if ((playerCompany->challenge_flags & company_flags::challenge_completed) != 0)
        {
            args.push(StringIds::challenge_completed);
        }
        else if ((playerCompany->challenge_flags & company_flags::challenge_failed) != 0)
        {
            args.push(StringIds::challenge_failed);
        }
        else if ((playerCompany->challenge_flags & company_flags::challenge_beaten_by_opponent) != 0)
        {
            args.push(StringIds::empty);
        }
        else
        {
            args.push(StringIds::challenge_progress);
            args.push<uint16_t>(playerCompany->challengeProgress);

            if (objectiveFlags & 4)
            {
                uint16_t monthsLeft = (*objectiveTimeLimitYears * 12 - objectiveMonthsInChallenge);
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
    static void textInput(window* w, widget_index widgetIndex, const char* str)
    {
        switch (widgetIndex)
        {
            case Widx::map_chat_menu:
                processChatMessage(str);
                break;
        }
    }

    // 0x00439A1C
    static void processChatMessage(const char* string)
    {
        addr<0x009C68E8, string_id>() = StringIds::empty;

        for (uint8_t i = 0; i < 32; i++)
        {
            GameCommands::do_71(i, &string[i * 16]);
        }
    }

    static void togglePaused()
    {
        GameCommands::do_20();
    }

    // 0x00439A70 (speed: 0)
    // 0x00439A93 (speed: 1)
    // 0x00439AB6 (speed: 2)
    static void changeGameSpeed(window* w, uint8_t speed)
    {
        if (getPauseFlags() & 1)
        {
            GameCommands::do_20();
        }

        setGameSpeed(speed);
        w->invalidate();
    }

    void invalidateFrame()
    {
        _50A004 = _50A004 | (1 << 1);
    }

    // 0x00439AD9
    static void onUpdate(window* w)
    {
        w->var_854 += 1;
        if (w->var_854 >= 24)
        {
            w->var_854 = 0;
        }

        w->var_856 += 1;
        if (w->var_856 >= 60)
        {
            w->var_856 = 0;
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
}
