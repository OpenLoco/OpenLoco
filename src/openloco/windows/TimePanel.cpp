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
#include "../OpenLoco.h"
#include "../Ui.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"

using namespace openloco::interop;

namespace openloco::ui::TimePanel
{
    static const gfx::ui_size_t window_size = { 140, 27 };

    namespace widx
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
    static void processChatMessage(char* str);
    static void togglePaused();
    static void changeGameSpeed(window* w, uint8_t speed);

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 140, 29 }, widget_type::wt_3, 0),                                                                                          // 0,
        makeWidget({ 2, 2 }, { 136, 25 }, widget_type::wt_3, 0),                                                                                          // 1,
        makeWidget({ 113, 1 }, { 26, 26 }, widget_type::wt_9, 0),                                                                                         // 2,
        makeWidget({ 2, 2 }, { 111, 12 }, widget_type::wt_9, 0, image_ids::null, string_ids::tooltip_daymonthyear_challenge),                             // 3,
        makeRemapWidget({ 18, 15 }, { 20, 12 }, widget_type::wt_9, 0, image_ids::speed_pause, string_ids::tooltip_speed_pause),                           // 4,
        makeRemapWidget({ 38, 15 }, { 20, 12 }, widget_type::wt_9, 0, image_ids::speed_normal, string_ids::tooltip_speed_normal),                         // 5,
        makeRemapWidget({ 58, 15 }, { 20, 12 }, widget_type::wt_9, 0, image_ids::speed_fast_forward, string_ids::tooltip_speed_fast_forward),             // 6,
        makeRemapWidget({ 78, 15 }, { 20, 12 }, widget_type::wt_9, 0, image_ids::speed_extra_fast_forward, string_ids::tooltip_speed_extra_fast_forward), // 7,
        widgetEnd(),
    };

    static window_event_list _events;

    static void prepareDraw(window* window);
    static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi);
    static void onMouseUp(ui::window* window, widget_index widgetIndex);
    static void onMouseDown(ui::window* window, widget_index widgetIndex);
    static void textInput(window* w, widget_index widgetIndex, char* str);
    static void onDropdown(window* w, widget_index widgetIndex, int16_t item_index);
    static ui::cursor_id onCursor(window* w, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex);
    static void textInput(window* w, widget_index widgetIndex, char* str);
    static void onUpdate(window* w);

    static loco_global<uint16_t, 0x0050A004> _50A004;
    static loco_global<uint8_t, 0x00526231> objectiveFlags;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<int32_t, 0x00e3f0b8> gCurrentRotation;
    static loco_global<uint8_t, 0x00508F1A> game_speed;

    static loco_global<uint8_t, 0x00526240> objectiveTimeLimitYears;
    static loco_global<uint16_t, 0x00526243> _526243;

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
            gfx::point_t(ui::width() - window_size.width, ui::height() - window_size.height),
            gfx::ui_size_t(window_size.width, window_size.height),
            ui::window_flags::stick_to_front | ui::window_flags::transparent | ui::window_flags::no_background,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::map_chat_menu) | (1 << widx::date_btn) | (1 << widx::pause_btn) | (1 << widx::normal_speed_btn) | (1 << widx::fast_forward_btn) | (1 << widx::extra_fast_forward_btn);
        window->var_854 = 0;
        window->var_856 = 0;
        window->initScrollWidgets();

        auto skin = objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[0] = colour::translucent(skin->colour_17);
            window->colours[1] = colour::translucent(skin->colour_17);
        }

        return window;
    }

    // 0x004396A4
    static void prepareDraw(window* window)
    {
        _widgets[widx::inner_frame].type = widget_type::none;
        _widgets[widx::pause_btn].image = gfx::recolour(image_ids::speed_pause);
        _widgets[widx::normal_speed_btn].image = gfx::recolour(image_ids::speed_normal);
        _widgets[widx::fast_forward_btn].image = gfx::recolour(image_ids::speed_fast_forward);
        _widgets[widx::extra_fast_forward_btn].image = gfx::recolour(image_ids::speed_extra_fast_forward);

        if (isPaused())
        {
            _widgets[widx::pause_btn].image = gfx::recolour(image_ids::speed_pause_active);
        }
        else if (game_speed == 0)
        {
            _widgets[widx::normal_speed_btn].image = gfx::recolour(image_ids::speed_normal_active);
        }
        else if (game_speed == 1)
        {
            _widgets[widx::fast_forward_btn].image = gfx::recolour(image_ids::speed_fast_forward_active);
        }
        else if (game_speed == 2)
        {
            _widgets[widx::extra_fast_forward_btn].image = gfx::recolour(image_ids::speed_extra_fast_forward_active);
        }

        if (isNetworked())
        {
            _widgets[widx::fast_forward_btn].type = widget_type::none;
            _widgets[widx::extra_fast_forward_btn].type = widget_type::none;

            _widgets[widx::pause_btn].left = 38;
            _widgets[widx::pause_btn].right = 57;
            _widgets[widx::normal_speed_btn].left = 58;
            _widgets[widx::normal_speed_btn].right = 77;
        }
        else
        {
            _widgets[widx::fast_forward_btn].type = widget_type::wt_9;
            _widgets[widx::extra_fast_forward_btn].type = widget_type::wt_9;

            _widgets[widx::pause_btn].left = 18;
            _widgets[widx::pause_btn].right = 37;
            _widgets[widx::normal_speed_btn].left = 38;
            _widgets[widx::normal_speed_btn].right = 57;
            _widgets[widx::fast_forward_btn].left = 58;
            _widgets[widx::fast_forward_btn].right = 77;
            _widgets[widx::extra_fast_forward_btn].left = 78;
            _widgets[widx::extra_fast_forward_btn].right = 97;
        }
    }

    // TODO: use same list as top toolbar
    static const uint32_t map_sprites_by_rotation[] = {
        interface_skin::image_ids::toolbar_menu_map_north,
        interface_skin::image_ids::toolbar_menu_map_west,
        interface_skin::image_ids::toolbar_menu_map_south,
        interface_skin::image_ids::toolbar_menu_map_east,
    };

    // 0x004397BE
    static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
    {
        widget_t& frame = _widgets[widx::outer_frame];
        gfx::drawRect(dpi, self->x + frame.left, self->y + frame.top, frame.width(), frame.height(), 0x2000000 | 52);

        // Draw widgets.
        self->draw(dpi);

        gfx::drawRectInset(dpi, self->x + frame.left + 1, self->y + frame.top + 1, frame.width() - 2, frame.height() - 2, self->colours[1], 0x30);

        *(uint32_t*)&_common_format_args[0] = getCurrentDay();
        string_id format = string_ids::date_monthyear;

        if (isPaused() && (getPauseFlags() & (1 << 2)) == 0)
        {
            if (self->var_856 >= 30)
            {
                format = string_ids::toolbar_status_paused;
            }
        }

        colour_t c = colour::opaque(self->colours[0]);
        if (input::isHovering(WindowType::timeToolbar, 0, widx::date_btn))
        {
            c = colour::white;
        }
        gfx::drawStringCentred(*dpi, self->x + _widgets[widx::date_btn].mid_x(), self->y + _widgets[widx::date_btn].top + 1, c, format, &*_common_format_args);

        auto skin = objectmgr::get<interface_skin_object>();
        gfx::drawImage(dpi, self->x + _widgets[widx::map_chat_menu].left - 2, self->y + _widgets[widx::map_chat_menu].top - 1, skin->img + map_sprites_by_rotation[gCurrentRotation]);
    }

    // 0x004398FB
    static void onMouseUp(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::date_btn:
                MessageWindow::open();
                break;
            case widx::pause_btn:
                togglePaused();
                break;
            case widx::normal_speed_btn:
                changeGameSpeed(window, 0);
                break;
            case widx::fast_forward_btn:
                changeGameSpeed(window, 1);
                break;
            case widx::extra_fast_forward_btn:
                changeGameSpeed(window, 2);
                break;
        }
    }

    // 0x0043A67F
    static void mapMouseDown(ui::window* self, widget_index widgetIndex)
    {
        auto skin = objectmgr::get<interface_skin_object>();

        if (isNetworked())
        {
            dropdown::add(0, string_ids::menu_sprite_stringid, { (uint32_t)skin->img + interface_skin::image_ids::phone, string_ids::chat_send_message });
            dropdown::add(1, string_ids::menu_sprite_stringid, { (uint32_t)skin->img + map_sprites_by_rotation[gCurrentRotation], string_ids::menu_map });
            dropdown::showBelow(self, widgetIndex, 2, 25, (1 << 6));
            dropdown::setHighlightedItem(1);
        }
        else
        {
            dropdown::add(0, string_ids::menu_sprite_stringid, { (uint32_t)skin->img + map_sprites_by_rotation[gCurrentRotation], string_ids::menu_map });
            dropdown::showBelow(self, widgetIndex, 1, 25, (1 << 6));
            dropdown::setHighlightedItem(0);
        }
    }

    // 0x0043A72F
    static void mapDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
    {
        if (itemIndex == -1)
            itemIndex = dropdown::getHighlightedItem();

        if (isNetworked())
        {
            switch (itemIndex)
            {
                case 0:
                {
                    auto opponent = companymgr::getOpponent();
                    _common_format_args[4] = opponent->owner_name;
                    ui::textinput::openTextinput(self, string_ids::chat_title, string_ids::chat_instructions, string_ids::empty, widgetIndex, &*_common_format_args);
                    break;
                }
                case 1:
                    windows::map::open();
                    break;
            }
        }
        else
        {
            switch (itemIndex)
            {
                case 0:
                    windows::map::open();
                    break;
            }
        }
    }

    // 0x043992E
    static void onMouseDown(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::map_chat_menu:
                mapMouseDown(window, widgetIndex);
                break;
        }
    }

    // 0x439939
    static void onDropdown(window* w, widget_index widgetIndex, int16_t item_index)
    {
        switch (widgetIndex)
        {
            case widx::map_chat_menu:
                mapDropdown(w, widgetIndex, item_index);
                break;
        }
    }

    // 0x00439944
    static ui::cursor_id onCursor(ui::window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        switch (widgetIdx)
        {
            case widx::date_btn:
                _tooltipTimeout = 2000;
                break;
        }

        return fallback;
    }

    // 0x00439955
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::date_btn:
                formatChallenge(args);
                break;
        }
    }

    // 0x0043995C
    static void formatChallenge(FormatArguments& args)
    {
        args.push(getCurrentDay());

        auto playerCompany = companymgr::get(companymgr::getControllingId());

        if ((playerCompany->challenge_flags & company_flags::challenge_completed) != 0)
        {
            args.push(string_ids::challenge_completed);
        }
        else if ((playerCompany->challenge_flags & company_flags::challenge_failed) != 0)
        {
            args.push(string_ids::challenge_failed);
        }
        else if ((playerCompany->challenge_flags & company_flags::challenge_beaten_by_opponent) != 0)
        {
            args.push(string_ids::empty);
        }
        else
        {
            args.push(string_ids::challenge_progress);
            args.push<uint16_t>(playerCompany->var_8C4E);

            if (objectiveFlags & 4)
            {
                uint16_t monthsLeft = (*objectiveTimeLimitYears * 12 - _526243);
                uint16_t yearsLeft = monthsLeft / 12;
                monthsLeft = monthsLeft % 12;
                args.push(string_ids::challenge_time_left);
                args.push(yearsLeft);
                args.push(monthsLeft);
            }
            else
            {
                args.push(string_ids::empty);
            }
        }
    }

    // 0x00439A15
    static void textInput(window* w, widget_index widgetIndex, char* str)
    {
        switch (widgetIndex)
        {
            case widx::map_chat_menu:
                processChatMessage(str);
                break;
        }
    }

    // 0x00439A1C
    static void processChatMessage(char string[512])
    {
        addr<0x009C68E8, string_id>() = string_ids::empty;

        for (uint8_t i = 0; i < 32; i++)
        {
            game_commands::do_71(i, &string[i * 16]);
        }
    }

    static void togglePaused()
    {
        game_commands::do_20();
    }

    // 0x00439A70 (speed: 0)
    // 0x00439A93 (speed: 1)
    // 0x00439AB6 (speed: 2)
    static void changeGameSpeed(window* w, uint8_t speed)
    {
        if (getPauseFlags() & 1)
        {
            game_commands::do_20();
        }

        game_speed = speed;
        w->invalidate();
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
            WindowManager::invalidateWidget(WindowType::timeToolbar, 0, widx::inner_frame);
        }

        if (isPaused())
        {
            WindowManager::invalidateWidget(WindowType::timeToolbar, 0, widx::inner_frame);
        }
    }
}
