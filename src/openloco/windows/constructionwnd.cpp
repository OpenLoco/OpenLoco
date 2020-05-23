#include "../date.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../objects/airport_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/track_object.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::construction
{
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;
    static loco_global<std::uint8_t[8], 0x0525F72> _byte_525F72;
    static loco_global<std::uint8_t[8], 0x0525F7A> _byte_525F7A;
    static loco_global<std::uint8_t[8], 0x0525F82> _byte_525F82;
    static loco_global<std::uint8_t[8], 0x0525F8A> _byte_525F8A;
    static loco_global<std::uint8_t[8], 0x0525F9A> _byte_525F9A;
    static loco_global<std::uint8_t[8], 0x0525FA2> _byte_525FA2;
    static loco_global<uint8_t, 0x00525FAA> _lastRailroadOption;
    static loco_global<uint8_t, 0x00525FAB> _lastRoadOption;
    static loco_global<uint8_t, 0x00525FAC> _haveAirports;
    static loco_global<uint8_t, 0x00525FAD> _haveShipPorts;
    static loco_global<uint8_t, 0x0112C2E1> _byte_112C2E1;
    static loco_global<uint32_t, 0x01135F3E> _dword_1135F3E;
    static loco_global<uint16_t, 0x01135F86> _word_1135F86;
    static loco_global<uint16_t, 0x01135FB4> _word_1135FB4;
    static loco_global<uint16_t, 0x01135FB6> _word_1135FB6;
    static loco_global<uint16_t, 0x01135FB8> _word_1135FB8;
    static loco_global<uint16_t, 0x01135FE4> _word_1135FE4;
    static loco_global<uint8_t, 0x0113601D> _byte_113601D;
    static loco_global<uint8_t, 0x0113602E> _byte_113602E;
    static loco_global<uint8_t, 0x01136030> _byte_1136030;
    static loco_global<uint8_t, 0x01136039> _byte_1136039;
    static loco_global<uint8_t, 0x0113603A> _byte_113603A;
    static loco_global<uint8_t, 0x0113603B> _byte_113603B;
    static loco_global<uint8_t, 0x0113604C> _byte_113604C;
    static loco_global<uint32_t, 0x01136054> _dword_1136054;
    static loco_global<uint8_t, 0x01136061> _byte_1136061;
    static loco_global<uint8_t, 0x01136062> _byte_1136062;
    static loco_global<uint8_t, 0x01136063> _byte_1136063;
    static loco_global<uint8_t, 0x01136064> _byte_1136064;
    static loco_global<uint8_t, 0x01136067> _byte_1136067;
    static loco_global<uint8_t, 0x01136068> _byte_1136068;
    static loco_global<uint8_t, 0x0113606E> _byte_113606E;
    static loco_global<uint8_t, 0x01136076> _byte_1136076;
    static loco_global<uint8_t, 0x0113607E> _byte_113607E;

    namespace widx
    {
        enum
        {
            close = 2,
            tab_0 = 4,
            tab_1,
            tab_2,
            tab_3,
            construct = 28,
            remove,
            place,
        };
    }

    namespace construction
    {
    }

    static void sub_4A3A50()
    {
    }

    // 0x004CD454
    static void sub_4CD454()
    {
        if (is_unknown_3_mode())
        {
            auto window = WindowManager::find(_toolWindowType, _toolWindowNumber);
            if (window != nullptr)
                input::cancel_tool();
        }
    }

    // 0x004A3A06
    static void sub_4A3A06(const uint32_t flags)
    {
        auto ebx = flags;
        if (flags & 0x80)
        {
            ebx &= 0xFFFFFF7F;
            auto roadObj = objectmgr::get<road_object>(ebx);
            if (roadObj->flags & flags_12::unk_01)
                _lastRoadOption = flags;
            else
                _lastRailroadOption = flags;
        }
        else
        {
            auto trackObj = objectmgr::get<track_object>(ebx);
            if (trackObj->flags & flags_22::unk_02)
                _lastRailroadOption = flags;
            else
                _lastRoadOption = flags;
        }
        WindowManager::invalidate(WindowType::topToolbar, 0);
    }

    // 0x0049CE33
    static void setDisabledWidgets(window* self)
    {
        auto disabledWidgets = 0;
        if (is_editor_mode())
            disabledWidgets |= 0x20;
        if (_byte_1136063 & 0xC0)
            disabledWidgets |= 0x10;
        if (_byte_113602E == 0xFF)
            disabledWidgets |= 0x40;
        if (_dword_1136054 == 0xFFFFFFFF)
            disabledWidgets |= 0x80;
        if (_byte_113604C == 0xFF)
            disabledWidgets |= 0x20;
        self->disabled_widgets = disabledWidgets;
    }

    // 0x004793C4
    static void setDirectionArrows()
    {
        if (_byte_112C2E1 == 0)
        {
            auto mainWindow = WindowManager::getMainWindow();
            if (mainWindow != nullptr)
            {
                if (!mainWindow->viewports[0]->flags & viewport_flags::one_way_direction_arrows)
                {
                    mainWindow->viewports[0]->flags |= viewport_flags::one_way_direction_arrows;
                    mainWindow->invalidate();
                }
            }
        }
        _byte_112C2E1++;
    }

    static void sub_4A0963()
    {
        auto window = WindowManager::createWindow(
            WindowType::construction,
            construction::windowSize,
            window_flags::flag_11 | window_flags::no_auto_close,
            &construction::events);

        window->widgets = construction::widgets;
        window->current_tab = 0;
        window->enabled_widgets = construction::enabledWidgets;
        window->activated_widgets = 0;

        setDisabledWidgets(window);

        window->init_scroll_widgets();
        window->owner = _playerCompany;

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[1] = skin->colour_0D;

        WindowManager::sub_4CEE0B(window);
        setDirectionArrows();
        ui::windows::showGridlines();
    }

    static void sub_4723BD()
    {

    }

    // 0x0048D70C
    static void sub_48D70C(uint32_t* edi)
    {
        auto currentYear = current_year();
        auto airportCount = 0;
        for (auto i = 0; 1 <= 8; i++)
        {
            auto airportObj = objectmgr::get<airport_object>(i);
            if (airportObj == nullptr)
                continue;
            if (currentYear < airportObj->designedYear)
                continue;
            if (currentYear > airportObj->obsoleteYear)
                continue;
            edi[airportCount] = i;
            airportCount++;
        }
        edi[airportCount] = 0xFF;

        sub_4723BD();
    }

    static void sub_48D753()
    {
    }

    static void sub_48D678()
    {
    }

    static void sub_42C518()
    {
    }

    static void sub_4781C5()
    {
    }

    static void sub_49F185()
    {
    }

    static void sub_488B4D()
    {
    }

    static void sub_48D5E4()
    {
    }

    static void sub_42C490()
    {
    }

    static void sub_4A693D()
    {
    }

    window* check(uint8_t al)
    {
        if (al = 0x0FF)
        {
            al = _byte_113603B;
        }

        _byte_113604C = al;
        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            setDisabledWidgets(window);
        }

        window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            window->call_on_mouse_up(widx::tab_1);
        }
        return window;
    }

    window* checkB(uint8_t al)
    {
        _word_1135FE4 = al;
        _byte_113603A = 0;

        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            setDisabledWidgets(window);
        }

        sub_49F185();
        window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            window->call_on_mouse_up(widx::place);
        }
        return window;
    }

    // 0x004A3B0D
    window* openWithFlags(const uint32_t flags)
    {
        //registers regs;
        //regs.ecx = flags;
        //call(0x004A3B0D, regs);
        //return (window*)regs.esi;
        auto mainWindow = WindowManager::getMainWindow();
        if (mainWindow)
        {
            auto viewport = mainWindow->viewports[0];
            _word_1135F86 = viewport->flags;
        }

        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            if (flags & 0x80)
            {
                auto al = flags & 0x7F;
                auto roadObj = objectmgr::get<road_object>(al);

                if (roadObj->flags & 8)
                {
                    if (_byte_1136062 & 0x80)
                    {
                        al = flags & 0x7F;
                        roadObj = objectmgr::get<road_object>(al);

                        if (roadObj->flags & 8)
                        {
                            _byte_1136062 = flags;

                            sub_4A3A50();

                            _byte_1136067 = 0;
                            _byte_1136068 = 0;

                            return window;
                        }
                    }
                }
            }
        }

        WindowManager::closeConstructionWindows();
        sub_4CD454();

        mainWindow = WindowManager::getMainWindow();

        if (mainWindow)
        {
            auto viewport = mainWindow->viewports[0];
            _word_1135F86 = viewport->flags;
        }

        _byte_1136062 = flags;
        _byte_1136063, flags >> 24;
        _word_1135FB4 = 0x1800;
        _word_1135FB6 = 0x1800;
        _word_1135FB8 = 0x100;
        _byte_1136064 = 0;
        _byte_1136061 = 0;
        _byte_113607E = 1;
        _dword_1135F3E = 0x80000000;
        _byte_1136076 = 0;
        _byte_1136067 = 0;
        _byte_1136068 = 0;
        _byte_113606E = 0;

        sub_4A3A06(flags);

        if (flags & 0x80000000)
        {
            sub_4A0963();

            _byte_113602E = 0x0FF;
            _dword_1136054 = 0x0FFFFFFFF;
            _word_1135FE4 = 0;
            _byte_1136039 = 0x0FF;
            uint32_t edi = _byte_113603B;

            sub_48D70C(&edi);

            auto al = _haveAirports;

            return check(al);
        }
        else
        {
            if (flags & 0x40000000)
            {
                sub_4A0963();

                _byte_113602E = 0xFF;
                _dword_1136054 = 0xFFFFFFFF;
                _word_1135FE4 = 0;
                _byte_1136039 = 0xFF;
                auto edi = _byte_113603B;

                sub_48D753();

                auto al = _haveShipPorts;

                return check(al);
            }
            else
            {
                if (flags & 80)
                {
                    sub_4A0963();

                    _byte_113602E = 0xFF;
                    auto al = _byte_1136062 & 0x7F;
                    auto edi = _byte_113603B;

                    sub_48D678();

                    edi = _byte_1136062 & 0xFFFFFF7F;
                    al = _byte_525F9A[edi];

                    if (al == 0xFF)
                        al = _byte_113603B;

                    _byte_113604C = al;
                    al = _byte_1136062 & 0x7F;
                    edi = _byte_1136030;

                    sub_42C518();

                    edi = _byte_1136062 & 0xFFFFFF7F;
                    al = _byte_525F7A[edi];

                    if (al == 0xFF)
                        al = _byte_1136030;

                    _byte_1136039 = al;
                    al = _byte_1136062 & 0x7F;
                    edi = _dword_1136054;

                    sub_4781C5();

                    edi = _byte_1136062 & 0xFFFFFF7F;
                    al = _byte_525FA2[edi];

                    if (al == 0xff)
                        al = 0;

                    return checkB(al);
                }
            }
        }
        sub_4A0963();

        auto al = _byte_1136062;
        auto edi = _byte_113601D;

        sub_488B4D();

        edi = _byte_1136062;
        al = _byte_525F72[edi];

        if (al == 0xFF)
            al = _byte_113601D;

        _byte_113602E = al;
        al = _byte_1136062;
        edi = _byte_113603B;

        sub_48D5E4();

        edi = _byte_1136062;
        al = _byte_525F82[edi];

        if (al == 0xFF)
            al = _byte_113603B;

        _byte_113604C = al;
        al = _byte_1136062;
        edi = _byte_1136030;

        sub_42C490();

        edi = _byte_1136062;
        al = _byte_525F7A[edi];

        if (al == 0xFF)
            al = _byte_1136030;

        _byte_1136039 = al;
        al = _byte_1136062;
        edi = _dword_1136054;

        sub_4A693D();

        edi = _byte_1136062;
        al = _byte_525F8A[edi];

        if (al == 0xFF)
            al = 0;

        return checkB(al);
    }

    // 0x0049D3F6
    void on_mouse_up(window& w, const uint16_t widgetIndex)
    {
        // Allow shift key to repeat the action multiple times
        // This is useful for building very long tracks.
        int multiplier = 1;
        if (input::has_key_modifier(input::key_modifier::shift))
        {
            multiplier = 10;
        }

        registers regs;
        regs.edx = widgetIndex;
        regs.esi = (int32_t)&w;
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(&w);
                break;
            case widx::tab_0:
            case widx::tab_1:
            case widx::tab_2:
            case widx::tab_3:
                call(0x0049D93A, regs);
                break;
            case widx::construct:
                for (int i = 0; i < multiplier; i++)
                {
                    call(0x0049F92D, regs);
                }
                break;
            case widx::remove:
                for (int i = 0; i < multiplier; i++)
                {
                    call(0x004A0121, regs);
                }
                break;
            case widx::place:
                call(0x0049D7DC, regs);
                break;
        }
    }
}
