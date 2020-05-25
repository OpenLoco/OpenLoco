#include "../companymgr.h"
#include "../date.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../map/tilemgr.h"
#include "../objects/airport_object.h"
#include "../objects/bridge_object.h"
#include "../objects/dock_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/road_station_object.h"
#include "../objects/track_object.h"
#include "../objects/train_signal_object.h"
#include "../objects/train_station_object.h"
#include "../objects/vehicle_object.h"
#include "../ui/WindowManager.h"
#include "../utility/numeric.hpp"

using namespace openloco::interop;
using namespace openloco::map;

namespace openloco::ui::windows::construction
{
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;
    static loco_global<uint8_t[8], 0x0525F72> _byte_525F72;
    static loco_global<uint8_t[8], 0x0525F7A> _byte_525F7A;
    static loco_global<uint8_t[8], 0x0525F82> _byte_525F82;
    static loco_global<uint8_t[8], 0x0525F8A> _byte_525F8A;
    static loco_global<uint8_t[8], 0x0525F9A> _byte_525F9A;
    static loco_global<uint8_t[8], 0x0525FA2> _byte_525FA2;
    static loco_global<uint8_t, 0x00525FAA> _lastRailroadOption;
    static loco_global<uint8_t, 0x00525FAB> _lastRoadOption;
    static loco_global<uint8_t, 0x00525FAC> _haveAirports;
    static loco_global<uint8_t, 0x00525FAD> _haveShipPorts;
    static loco_global<company_id_t, 0x009C68EB> _updatingCompanyId;
    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    static loco_global<uint8_t, 0x0112C2E1> _byte_112C2E1;
    static loco_global<uint32_t, 0x01135F3E> _dword_1135F3E;
    static loco_global<uint16_t, 0x01135F86> _word_1135F86;
    static loco_global<uint16_t, 0x01135FB4> _word_1135FB4;
    static loco_global<uint16_t, 0x01135FB6> _word_1135FB6;
    static loco_global<uint16_t, 0x01135FB8> _word_1135FB8;
    static loco_global<uint16_t, 0x01135FE4> _word_1135FE4;
    static loco_global<uint8_t[17], 0x0113601D> _signalList;
    static loco_global<uint8_t, 0x0113602E> _byte_113602E;
    static loco_global<uint8_t[9], 0x01136030> _bridgeList;
    static loco_global<uint8_t, 0x01136039> _byte_1136039;
    static loco_global<uint8_t, 0x0113603A> _byte_113603A;
    static loco_global<uint8_t[17], 0x0113603B> _stationList;
    static loco_global<uint8_t, 0x0113604C> _byte_113604C;
    static loco_global<uint32_t, 0x01136054> _dword_1136054;
    static loco_global<uint8_t, 0x01136061> _byte_1136061;
    static loco_global<uint8_t, 0x01136062> _trackType;
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
            tab_construction = 4,
            tab_station,
            tab_signal,
            tab_overhead,
            construct = 28,
            remove,
            place,
        };
    }

    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_construction,
            tab_station,
            tab_signal,
            tab_overhead,
        };

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 235 }, widget_type::wt_3, 1),                                                                \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_track_road_construction),          \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_station_construction),            \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_signal_construction),             \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tab_electrification_construction)

        constexpr uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_construction) | (1 << widx::tab_station) | (1 << widx::tab_signal) | (1 << widx::tab_overhead);
    
        static void init_events();
    }

    namespace construction
    {
        static const gfx::ui_size_t windowSize = { 138, 276 };

        enum widx
        {
            left_hand_curve_very_small = 8,
            left_hand_curve_small,
            left_hand_curve,
            left_hand_curve_large,
            right_hand_curve_large,
            right_hand_curve,
            right_hand_curve_small,
            right_hand_curve_very_small,
            s_bend_dual_track_left,
            s_bend_left,
            straight,
            s_bend_right,
            s_bend_dual_track_right,
            steep_slope_down,
            slope_down,
            level,
            slope_up,
            steep_slope_up,
            bridge,
            bridge_2,
            construct,
            remove,
            rotate_90,
        };

        // clang-format off
        constexpr uint64_t allConstruction = {
            (1ULL << widx::left_hand_curve_very_small) |
            (1ULL << widx::left_hand_curve_small) |
            (1ULL << widx::left_hand_curve) |
            (1ULL << widx::left_hand_curve_large) |
            (1ULL << widx::right_hand_curve_large) |
            (1ULL << widx::right_hand_curve) |
            (1ULL << widx::right_hand_curve_small) |
            (1ULL << widx::right_hand_curve_very_small ) |
            (1ULL << widx::s_bend_dual_track_left) |
            (1ULL << widx::s_bend_left) |
            (1ULL << widx::straight) |
            (1ULL << widx::s_bend_right) |
            (1ULL << widx::s_bend_dual_track_right) |
            (1ULL << widx::steep_slope_down) |
            (1ULL << widx::slope_down) |
            (1ULL << widx::level) |
            (1ULL << widx::slope_up) |
            (1ULL << widx::steep_slope_up) |
            (1ULL << widx::bridge) |
            (1ULL << widx::bridge_2) |
            (1ULL << widx::construct) |
            (1ULL << widx::remove) |
            (1ULL << widx::rotate_90)
        };
        // clang-format on

        widget_t widgets[] = {
            commonWidgets(138, 276, string_ids::stringid_2),
            make_widget({ 3, 45 }, { 22, 24 }, widget_type::wt_9, 1, image_ids::construction_left_hand_curve_very_small, string_ids::tooltip_left_hand_curve_very_small),
            make_widget({ 3, 45 }, { 22, 24 }, widget_type::wt_9, 1, image_ids::construction_left_hand_curve_small, string_ids::tooltip_left_hand_curve_small),
            make_widget({ 25, 45 }, { 22, 24 }, widget_type::wt_9, 1, image_ids::construction_left_hand_curve, string_ids::tooltip_left_hand_curve),
            make_widget({ 47, 45 }, { 22, 24 }, widget_type::wt_9, 1, image_ids::construction_left_hand_curve_large, string_ids::tooltip_left_hand_curve_large),
            make_widget({ 69, 45 }, { 22, 24 }, widget_type::wt_9, 1, image_ids::construction_right_hand_curve_large, string_ids::tooltip_right_hand_curve_large),
            make_widget({ 91, 45 }, { 22, 24 }, widget_type::wt_9, 1, image_ids::construction_right_hand_curve, string_ids::tooltip_right_hand_curve),
            make_widget({ 113, 45 }, { 22, 24 }, widget_type::wt_9, 1, image_ids::construction_right_hand_curve_small, string_ids::tooltip_right_hand_curve_small),
            make_widget({ 113, 45 }, { 22, 24 }, widget_type::wt_9, 1, image_ids::construction_right_hand_curve_very_small, string_ids::tooltip_right_hand_curve_very_small),
            make_widget({ 9, 69 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_s_bend_dual_track_left, string_ids::tooltip_s_bend_left_dual_track),
            make_widget({ 33, 69 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_s_bend_left, string_ids::tooltip_s_bend_left),
            make_widget({ 57, 69 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_straight, string_ids::tooltip_straight),
            make_widget({ 81, 69 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_s_bend_right, string_ids::tooltip_s_bend_right),
            make_widget({ 105, 69 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_s_bend_dual_track_right, string_ids::tooltip_s_bend_right_dual_track),
            make_widget({ 9, 96 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_steep_slope_down, string_ids::tooltip_steep_slope_down),
            make_widget({ 33, 96 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_slope_down, string_ids::tooltip_slope_down),
            make_widget({ 57, 96 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_level, string_ids::tooltip_level),
            make_widget({ 81, 96 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_slope_up, string_ids::tooltip_slope_up),
            make_widget({ 105, 96 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::construction_steep_slope_up, string_ids::tooltip_steep_slope_up),
            make_widget({ 40, 123 }, { 58, 20 }, widget_type::wt_18, 1, string_ids::empty, string_ids::tooltip_bridge_stats),
            make_widget({ 86, 124 }, { 11, 18 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::tooltip_bridge_stats),
            make_widget({ 3, 145 }, { 132, 100 }, widget_type::wt_5, 1, 0xFFFFFFFF, string_ids::tooltip_construct),
            make_widget({ 6, 248 }, { 46, 24 }, widget_type::wt_9, 1, image_ids::construction_remove, string_ids::tooltip_remove),
            make_widget({ 57, 248 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rotate_object, string_ids::rotate_90),
            widget_end(),
        };

        constexpr uint64_t enabledWidgets = common::enabledWidgets | allConstruction;

        static window_event_list events;

        // 0x0049DD14
        static void on_close(window* self)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            call(0x0049DD14, regs);
        }

        // 0x0049D3F6
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edx = widgetIndex;
            call(0x0049D3F6, regs);
        }

        // 0x0049DAA5
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            call(0x0049DAA5, regs);
        }

        // 0x0049D42F
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edx = widgetIndex;
            call(0x0049D42F, regs);
        }

        // 0x0049D4EA
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edx = widgetIndex;
            regs.ax = itemIndex;
            call(0x0049D4EA, regs);
        }

        // 0x0049DCA2
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            call(0x0049DCA2, regs);
        }

        // 0x0049DC8C
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (uint32_t)&self;
            regs.edx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049DC8C, regs);
        }

        // 0x0049DC97
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (uint32_t)&self;
            regs.edx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049DC97, regs);
        }

        // 0x0049D4F5
        static ui::cursor_id cursor(window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.ax = xPos;
            regs.cx = yPos;
            regs.edi = (int32_t)fallback;
            call(0x0049D4F5, regs);
            return (cursor_id)regs.edi;
        }

        // 0x0049CE79
        static void prepare_draw(window*self)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            call(0x0049CE79, regs);
        }

        // 0x0049CF36
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x0049CF36, regs);
        }

        static void init_events()
        {
            events.on_close = on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.on_update = on_update;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.cursor = cursor;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
        }
    }

    // 0x0049FEC7
    static void sub_49FEC7()
    {
        registers regs;
        call(0x0049FEC7, regs);
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
    static void setTrackOptions(const uint32_t trackType)
    {
        auto newTrackType = trackType;
        if (trackType & (1 << 7))
        {
            newTrackType &= ~(1 << 7);
            auto roadObj = objectmgr::get<road_object>(newTrackType);
            if (roadObj->flags & flags_12::unk_01)
                _lastRoadOption = trackType;
            else
                _lastRailroadOption = trackType;
        }
        else
        {
            auto trackObj = objectmgr::get<track_object>(newTrackType);
            if (trackObj->flags & flags_22::unk_02)
                _lastRailroadOption = trackType;
            else
                _lastRoadOption = trackType;
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
            disabledWidgets |= (1 << 7);
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
                if (!(mainWindow->viewports[0]->flags & viewport_flags::one_way_direction_arrows))
                {
                    mainWindow->viewports[0]->flags |= viewport_flags::one_way_direction_arrows;
                    mainWindow->invalidate();
                }
            }
        }
        _byte_112C2E1++;
    }

    // 0x004A0963
    static void createConstructionWindow()
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

    // 0x004723BD
    static void formatList(uint8_t* list, size_t size)
    {
        size_t count = 0;
        for (; count < size; count++)
        {
            if (list[count] == 0xFF)
                break;
        }
        for (; count > 1; count--)
        {
            size_t i = 1;
            for (; i < count; i++)
            {
                uint16_t ax = list[count + i - 1];
                uint8_t al = ax;
                uint8_t ah = ax >> 8;
                if (al > ah)
                {
                    ax = (al << 8) | ah;
                    list[count + i - 1] = ax;
                }
            }
        }
    }

    // 0x0048D70C
    static void refreshAirportList()
    {
        auto currentYear = current_year();
        auto airportCount = 0;
        for (uint8_t i = 0; i <= objectmgr::get_max_objects(object_type::airport); i++)
        {
            auto airportObj = objectmgr::get<airport_object>(i);
            if (airportObj == nullptr)
                continue;
            if (currentYear < airportObj->designed_year)
                continue;
            if (currentYear > airportObj->obsolete_year)
                continue;
            _stationList[airportCount] = i;
            airportCount++;
        }
        _stationList[airportCount] = 0xFF;

        formatList(_stationList, std::size(_stationList));
    }

    // 0x0048D753
    static void refreshDockList()
    {
        auto currentYear = current_year();
        auto dockCount = 0;
        for (uint8_t i = 0; i <= objectmgr::get_max_objects(object_type::dock); i++)
        {
            auto dockObj = objectmgr::get<dock_object>(i);
            if (dockObj == nullptr)
                continue;
            if (currentYear < dockObj->designed_year)
                continue;
            if (currentYear > dockObj->obsolete_year)
                continue;
            _stationList[dockCount] = i;
            dockCount++;
        }
        _stationList[dockCount] = 0xFF;

        formatList(_stationList, std::size(_stationList));
    }

    // 0x0048D678
    static void refreshRoadStationList()
    {
        auto trackType = _trackType & ~(1 << 7);
        auto stationListCopy = _stationList;
        auto currentYear = current_year();
        auto roadObj = objectmgr::get<road_object>(trackType);
        auto roadStationCount = 0;

        for (auto i = 0; i <= roadObj->num_stations; i++)
        {
            if (roadObj->stations[i] == 0xFF)
                continue;
            auto roadStationObj = objectmgr::get<road_station_object>(i);
            if (currentYear < roadStationObj->designed_year)
                continue;
            if (currentYear > roadStationObj->obsolete_year)
                continue;
            _stationList[roadStationCount] = i;
            roadStationCount++;
        }

        for (uint8_t i = 0; i <= objectmgr::get_max_objects(object_type::road_station); i++)
        {
            auto roadStationObj = objectmgr::get<road_station_object>(i);
            if (roadStationObj == nullptr)
                continue;
            for (auto j = 0; j < roadStationObj->var_20; j++)
            {
                if (trackType != roadStationObj->num_compatible)
                    continue;
                if (currentYear < roadStationObj->designed_year)
                    continue;
                if (currentYear > roadStationObj->obsolete_year)
                    continue;
                for (size_t k = 0; k < std::size(_stationList); k++)
                {
                    auto station = stationListCopy[k];
                    if (stationListCopy = _stationList)
                    {
                        _stationList[roadStationCount] = i;
                        roadStationCount++;
                        break;
                    }
                    if (i == station)
                        break;
                }
            }
        }

        _stationList[roadStationCount] = 0xFF;

        formatList(_stationList, std::size(_stationList));
    }

    // 0x0042C518
    static void refreshRoadBridgeList()
    {
        auto trackType = _trackType & ~(1 << 7);
        auto bridgeListCopy = _bridgeList;
        auto currentYear = current_year();
        auto roadObj = objectmgr::get<road_object>(trackType);
        auto bridgeCount = 0;
        for (auto i = 0; i <= roadObj->num_bridges; i++)
        {
            if (roadObj->bridges[i] == 0xFF)
                continue;
            auto bridgeObj = objectmgr::get<bridge_object>(i);
            if (currentYear < bridgeObj->designed_year)
                continue;
            _bridgeList[bridgeCount] = i;
            bridgeCount++;
        }

        for (uint8_t i = 0; i <= objectmgr::get_max_objects(object_type::bridge); i++)
        {
            auto bridgeObj = objectmgr::get<bridge_object>(i);
            if (bridgeObj == nullptr)
                continue;
            for (auto j = 0; j < bridgeObj->var_22; j++)
            {
                if (trackType != bridgeObj->var_22)
                    continue;
                if (currentYear < bridgeObj->designed_year)
                    continue;
                for (size_t k = 0; k < std::size(_bridgeList); k++)
                {
                    auto bridge = bridgeListCopy[k];
                    if (bridgeListCopy = _bridgeList)
                    {
                        _bridgeList[bridgeCount] = i;
                        bridgeCount++;
                        break;
                    }
                    if (i == bridge)
                        break;
                }
            }
        }

        _bridgeList[bridgeCount] = 0xFF;

        formatList(_bridgeList, std::size(_bridgeList));
    }

    // 0x004781C5
    static void refreshRoadOption()
    {
        auto trackType = _trackType & ~(1 << 7);
        auto companyId = _updatingCompanyId;
        _dword_1136054 = 0xFFFFFFFF;
        auto flags = 0;

        for (uint8_t vehicle = 0; vehicle < objectmgr::get_max_objects(object_type::vehicle); vehicle++)
        {
            auto vehicleObj = objectmgr::get<vehicle_object>(vehicle);

            if (vehicleObj == nullptr)
                continue;

            if (vehicleObj->mode != TransportMode::road)
                continue;

            if (trackType != vehicleObj->track_type)
                continue;

            auto edi = vehicle;
            auto edx = vehicle & 0x1F;
            edi = edi >> 5;

            auto company = companymgr::get(companyId);

            if (!(company->unlocked_vehicles[edi] & 1 << edx))
                continue;

            for (auto i = 0; i < vehicleObj->num_mods; i++)
            {
                flags |= 1ULL << vehicleObj->required_track_extras[i];
            }

            if (!(vehicleObj->flags & flags_E0::rack_rail))
                continue;

            flags |= 1ULL << vehicleObj->rack_rail_type;
        }

        auto roadObj = objectmgr::get<road_object>(trackType);

        for (auto i = 0; i < roadObj->num_mods; i++)
        {
            if (flags & roadObj->mods[i])
                _dword_1136054 = roadObj->mods[i];
            _dword_1136054++;
        }
    } 
    
    // 0x0049F1B5
    static void sub_49F1B5()
    {
        registers regs;
        call(0x0049F1B5, regs);

        //auto window = WindowManager::find(WindowType::construction);

        //if (window != nullptr)
        //{
        //    return;
        //}

        //if (_trackType & (1 << 7))
        //{
        //    tilemgr::map_invalidate_map_selection_tiles();
        //    _mapSelectionFlags = _mapSelectionFlags | 0x0A;
        //    auto carryFlag = sub_4A0832();
        //}
        //else
        //{
        //    tilemgr::map_invalidate_map_selection_tiles();
        //    _mapSelectionFlags = _mapSelectionFlags | 0x0A;
        //}
    }

    // 0x004A3A50
    static void sub_4A3A50()
    {
        sub_49FEC7();
        setTrackOptions(_trackType);
        refreshRoadStationList();

        auto al = _byte_525F9A[(_trackType & ~(1ULL << 7))];
        if (al == 0xFF)
            al = _stationList[0];
        _byte_113604C = al;

        refreshRoadBridgeList();

        al = _byte_525F7A[(_trackType & ~(1ULL << 7))];
        if (al == 0xFF)
            al = _bridgeList[0];
        _byte_1136039 = al;

        refreshRoadOption();

        al = _byte_525F7A[(_trackType & ~(1ULL << 7))];
        if (al == 0xFF)
            al = 0;
        _word_1135FE4 = al;

        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            setDisabledWidgets(window);
        }
        sub_49F1B5();
    }

    //static bool sub_4A0832()
    //{
    //    auto eax = _byte_1136067;
    //    auto bl = _byte_1136068;
    //    auto bh = _byte_1136064;
    //    if (eax == 0xFF)
    //        return;
    //    switch (eax)
    //    {
    //        case 0: // 0x004A0856
    //        {
    //            if (bh >= 0x0C)
    //                return true;
    //            if (bh >= 8)
    //                return true;
    //            if (bh >= 4)
    //                return true;
    //            auto dh = 0;
    //            if (bl != 0)
    //            {
    //                dh = 5;
    //                if (bl != 2)
    //                {
    //                    dh = 7;
    //                    if (bl != 4)
    //                    {
    //                        dh = 6;
    //                        if (bl != 6)
    //                        {
    //                            dh = 8;
    //                            if (bl != 8)
    //                                return true;
    //                        }
    //                    }
    //                }
    //            }
    //        }
    //        case 1: // 0x004A08A5
    //        {
    //            if (bh != 0)
    //                return true;
    //            if (bh >= 0xC)
    //                return true;
    //            if (bh >= 8)
    //                return true;
    //            if (bh >= 4)
    //                return true;
    //            auto dh = 1;
    //        }
    //        case 2: // 0x004A08CD
    //        {
    //            if (bh != 0)
    //                return true;
    //            if (bh >= 0xC)
    //                return true;
    //            if (bh >= 8)
    //                return true;
    //            if (bh >= 4)
    //                return true;
    //            auto dh = 2;
    //        }
    //        case 3: // 0x004A08ED
    //        {
    //            if (bh >= 4)
    //                return true;
    //            auto dh = 3;
    //            if (bl != 0)
    //                return true;
    //        }
    //        case 4: // 0x004A08FB
    //        {
    //            if (bh >= 4)
    //                return true;
    //            auto dh = 4;
    //            if (bl != 0)
    //                return true;
    //        }
    //        case 5: // 0x004A095F
    //        case 6:
    //        case 7:
    //        case 8:
    //        case 9:
    //        case 10:
    //        case 11:
    //        case 12:
    //        {
    //            return true;
    //        }
    //        case 13: // 0x004A0909
    //        {
    //            if (bl != 0)
    //                return true;
    //            if (bh >= 0x0C)
    //                return true;
    //        }
    //    }
    //    auto dh = 9;
    //    if (bh < 0x0C)
    //        bh &= uint8_t(3);
    //    uint32_t edx = (_trackType & 0x7F) | (dh << 8);
    //    edx |= (_byte_1136039 << 24);
    //    auto edi = _word_1135FB8 << 16 | _word_1135FE4;
    //    if (_word_1135FB4)
    //        return true;
    //    else
    //        return false;
    //}

    // 0x00488B4D
    static void refreshSignalList()
    {
        auto trackType = _trackType;
        auto signalListCopy = _signalList;
        auto currentYear = current_year();
        auto trackObj = objectmgr::get<track_object>(trackType);
        auto signalCount = 0;
        auto var_0E = trackObj->var_0E;
        for (auto i = 0; i <= 32; i++)
        {
            auto ecx = utility::bitscanforward(var_0E);
            if (ecx == 0)
                break;
            var_0E &= ~(1 << ecx);
            auto signalObj = objectmgr::get<train_signal_object>(i);
            if (currentYear > signalObj->obsolete_year)
                continue;
            if (currentYear < signalObj->designed_year)
                continue;
            _signalList[signalCount] = ecx;
            signalCount++;
        }

        for (uint8_t i = 0; i <= objectmgr::get_max_objects(object_type::track_signal); i++)
        {
            auto signalObj = objectmgr::get<train_signal_object>(i);
            if (signalObj == nullptr)
                continue;
            for (auto j = 0; j < signalObj->var_12; j++)
            {
                if (trackType != signalObj->var_13)
                    continue;
                if (currentYear < signalObj->designed_year)
                    continue;
                for (size_t k = 0; k < std::size(_signalList); k++)
                {
                    auto signal = signalListCopy[k];
                    if (signalListCopy = _signalList)
                    {
                        _signalList[signalCount] = i;
                        signalCount++;
                        break;
                    }
                    if (i == signal)
                        break;
                }
            }
        }

        _signalList[signalCount] = 0xFF;

        formatList(_signalList, std::size(_signalList));
    }

    // 0x0048D5E4
    static void refreshtrainStationList()
    {
        auto trackType = _trackType;
        auto stationListCopy = _stationList;
        auto currentYear = current_year();
        auto trackObj = objectmgr::get<track_object>(trackType);
        auto trainStationCount = 0;

        for (auto i = 0; i <= trackObj->num_stations; i++)
        {
            if (trackObj->stations[i] == 0xFF)
                continue;
            auto trainStationObj = objectmgr::get<train_station_object>(i);
            if (currentYear < trainStationObj->designed_year)
                continue;
            if (currentYear > trainStationObj->obsolete_year)
                continue;
            _stationList[trainStationCount] = i;
            trainStationCount++;
        }

        for (uint8_t i = 0; i <= objectmgr::get_max_objects(object_type::track_station); i++)
        {
            auto trainStationObj = objectmgr::get<train_station_object>(i);
            if (trainStationObj == nullptr)
                continue;
            for (auto j = 0; j < trainStationObj->var_22; j++)
            {
                if (trackType != trainStationObj->num_compatible)
                    continue;
                if (currentYear < trainStationObj->designed_year)
                    continue;
                if (currentYear > trainStationObj->obsolete_year)
                    continue;
                for (size_t k = 0; k < std::size(_stationList); k++)
                {
                    auto station = stationListCopy[k];
                    if (stationListCopy = _stationList)
                    {
                        _stationList[trainStationCount] = i;
                        trainStationCount++;
                        break;
                    }
                    if (i == station)
                        break;
                }
            }
        }

        _stationList[trainStationCount] = 0xFF;

        formatList(_stationList, std::size(_stationList));
    }

    // 0x0042C490
    static void refreshTrackBridgeList()
    {
        auto trackType = _trackType;
        auto bridgeListCopy = _bridgeList;
        auto currentYear = current_year();
        auto trackObj = objectmgr::get<track_object>(trackType);
        auto bridgeCount = 0;
        for (auto i = 0; i <= trackObj->num_bridges; i++)
        {
            if (trackObj->bridges[i] == 0xFF)
                continue;
            auto bridgeObj = objectmgr::get<bridge_object>(i);
            if (currentYear < bridgeObj->designed_year)
                continue;
            _bridgeList[bridgeCount] = i;
            bridgeCount++;
        }

        for (uint8_t i = 0; i <= objectmgr::get_max_objects(object_type::bridge); i++)
        {
            auto bridgeObj = objectmgr::get<bridge_object>(i);
            if (bridgeObj == nullptr)
                continue;
            for (auto j = 0; j < bridgeObj->var_22; j++)
            {
                if (trackType != bridgeObj->var_22)
                    continue;
                if (currentYear < bridgeObj->designed_year)
                    continue;
                for (size_t k = 0; k < std::size(_bridgeList); k++)
                {
                    auto bridge = bridgeListCopy[k];
                    if (bridgeListCopy = _bridgeList)
                    {
                        _bridgeList[bridgeCount] = i;
                        bridgeCount++;
                        break;
                    }
                    if (i == bridge)
                        break;
                }
            }
        }

        _bridgeList[bridgeCount] = 0xFF;

        formatList(_bridgeList, std::size(_bridgeList));
    }

    // 0x004A693D
    static void refreshTrackOption()
    {
        auto trackType = _trackType;
        auto companyId = _updatingCompanyId;
        _dword_1136054 = 0xFFFFFFFF;
        auto flags = 0;

        for (uint8_t vehicle = 0; vehicle < objectmgr::get_max_objects(object_type::vehicle); vehicle++)
        {
            auto vehicleObj = objectmgr::get<vehicle_object>(vehicle);

            if (vehicleObj == nullptr)
                continue;

            if (vehicleObj->mode != TransportMode::rail)
                continue;

            if (trackType != vehicleObj->track_type)
                continue;

            auto edi = vehicle;
            auto edx = vehicle & 0x1F;
            edi = edi >> 5;

            auto company = companymgr::get(companyId);

            if (!(company->unlocked_vehicles[edi] & 1 << edx))
                continue;

            for (auto i = 0; i < vehicleObj->num_mods; i++)
            {
                flags |= 1ULL << vehicleObj->required_track_extras[i];
            }

            if (!(vehicleObj->flags & flags_E0::rack_rail))
                continue;

            flags |= 1ULL << vehicleObj->rack_rail_type;
        }

        auto trackObj = objectmgr::get<track_object>(trackType);

        for (auto i = 0; i < trackObj->num_mods; i++)
        {
            if (flags & trackObj->mods[i])
                _dword_1136054 = trackObj->mods[i];
            _dword_1136054++;
        }
    }

    window* check(uint8_t al)
    {
        if (al == 0xFF)
        {
            al = _stationList[0];
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
            window->call_on_mouse_up(widx::tab_construction);
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

        sub_49F1B5();
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

        common::init_events();

        if (window != nullptr)
        {
            if (flags & (1 << 7))
            {
                auto al = flags & 0x7F;
                auto roadObj = objectmgr::get<road_object>(al);

                if (roadObj->flags & 8)
                {
                    if (_trackType & (1 << 7))
                    {
                        al = flags & 0x7F;
                        roadObj = objectmgr::get<road_object>(al);

                        if (roadObj->flags & 8)
                        {
                            _trackType = flags;

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

        _trackType = flags;
        _byte_1136063 = flags >> 24;
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

        setTrackOptions(flags);

        if (flags & 0x80000000)
        {
            createConstructionWindow();

            _byte_113602E = 0x0FF;
            _dword_1136054 = 0x0FFFFFFFF;
            _word_1135FE4 = 0;
            _byte_1136039 = 0x0FF;

            refreshAirportList();

            auto al = _haveAirports;

            return check(al);
        }
        else
        {
            if (flags & 0x40000000)
            {
                createConstructionWindow();

                _byte_113602E = 0xFF;
                _dword_1136054 = 0xFFFFFFFF;
                _word_1135FE4 = 0;
                _byte_1136039 = 0xFF;

                refreshDockList();

                auto al = _haveShipPorts;

                return check(al);
            }
            else
            {
                if (flags & 80)
                {
                    createConstructionWindow();

                    _byte_113602E = 0xFF;

                    refreshRoadStationList();

                    auto al = _byte_525F9A[(_trackType & ~(1ULL << 7))];

                    if (al == 0xFF)
                        al = _stationList[0];

                    _byte_113604C = al;

                    refreshRoadBridgeList();

                    al = _byte_525F7A[(_trackType & ~(1ULL << 7))];

                    if (al == 0xFF)
                        al = _bridgeList[0];

                    _byte_1136039 = al;

                    refreshRoadOption();

                    al = _byte_525FA2[(_trackType & ~(1ULL << 7))];

                    if (al == 0xff)
                        al = 0;

                    return checkB(al);
                }
            }
        }
        createConstructionWindow();

        auto al = _trackType;

        refreshSignalList();

        al = _byte_525F72[_trackType];

        if (al == 0xFF)
            al = _signalList[0];

        _byte_113602E = al;

        refreshtrainStationList();

        al = _byte_525F82[_trackType];

        if (al == 0xFF)
            al = _stationList[0];

        _byte_113604C = al;

        refreshTrackBridgeList();

        al = _byte_525F7A[_trackType];

        if (al == 0xFF)
            al = _bridgeList[0];

        _byte_1136039 = al;

        refreshTrackOption();

        al = _byte_525F8A[_trackType];

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
            case widx::tab_construction:
            case widx::tab_station:
            case widx::tab_signal:
            case widx::tab_overhead:
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

    namespace common
    {
        static void init_events()
        {
            construction::init_events();
        }
    }
}
