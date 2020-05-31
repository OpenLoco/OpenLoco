#include "../companymgr.h"
#include "../date.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../map/tilemgr.h"
#include "../objects/airport_object.h"
#include "../objects/bridge_object.h"
#include "../objects/dock_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_extra_object.h"
#include "../objects/road_object.h"
#include "../objects/road_station_object.h"
#include "../objects/track_extra_object.h"
#include "../objects/track_object.h"
#include "../objects/train_signal_object.h"
#include "../objects/train_station_object.h"
#include "../objects/vehicle_object.h"
#include "../ui/WindowManager.h"
#include "../utility/numeric.hpp"
#include "../widget.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction
{
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;
    static loco_global<uint8_t[10], 0x004F6D44> _unk_4F6D44;
    static loco_global<uint8_t[10], 0x004F6D4F> _unk_4F6D4F;
    static loco_global<uint8_t[10], 0x004F6D5A> _unk_4F6D5A;
    static loco_global<uint8_t[40], 0x004F6D65> _unk_4F6D65;
    static loco_global<uint8_t[40], 0x004F6D8E> _unk_4F6D8E;
    static loco_global<uint8_t[20], 0x004F6DB7> _unk_4F6DB7;
    static loco_global<uint8_t[20], 0x004F6DCC> _unk_4F6DCC;
    static loco_global<uint8_t[10], 0x004F6DE1> _unk_4F6DE1;
    static loco_global<uint8_t[10], 0x004F6DEC> _unk_4F6DEC;
    static loco_global<uint8_t[10], 0x004F6DF7> _unk_4F6DF7;

    static loco_global<uint8_t[10], 0x004F7488> _unk_4F7488;
    static loco_global<uint8_t[40], 0x004F7493> _unk_4F7493;
    static loco_global<uint8_t[10], 0x004F74BC> _unk_4F74BC;
    static loco_global<uint8_t[10], 0x004F74C7> _unk_4F74C7;
    static loco_global<uint8_t[40], 0x004F74D2> _unk_4F74D2;
    static loco_global<uint8_t[40], 0x004F74FB> _unk_4F74FB;
    static loco_global<uint8_t[50], 0x004F7524> _unk_4F7524;
    static loco_global<uint8_t[50], 0x004F7557> _unk_4F7557;
    static loco_global<uint8_t[50], 0x004F758A> _unk_4F758A;
    static loco_global<uint8_t[50], 0x004F75BD> _unk_4F75BD;
    static loco_global<uint8_t[50], 0x004F75F0> _unk_4F75F0;
    static loco_global<uint8_t[50], 0x004F7623> _unk_4F7623;
    static loco_global<uint8_t[40], 0x004F7656> _unk_4F7656;
    static loco_global<uint8_t[40], 0x004F767F> _unk_4F767F;
    static loco_global<uint8_t[20], 0x004F76A8> _unk_4F76A8;
    static loco_global<uint8_t[20], 0x004F76BD> _unk_4F76BD;
    static loco_global<uint8_t[10], 0x004F76D2> _unk_4F76D2;
    static loco_global<uint8_t[10], 0x004F76DD> _unk_4F76DD;
    static loco_global<uint8_t[40], 0x004F76E8> _unk_4F76E8;
    static loco_global<uint8_t[40], 0x004F7711> _unk_4F7711;
    static loco_global<uint8_t[40], 0x004F773A> _unk_4F773A;
    static loco_global<uint8_t[40], 0x004F7763> _unk_4F7763;
    static loco_global<uint8_t[40], 0x004F778C> _unk_4F778C;
    static loco_global<uint8_t[40], 0x004F77B5> _unk_4F77B5;
    static loco_global<uint8_t[40], 0x004F77DE> _unk_4F77DE;
    static loco_global<uint8_t[40], 0x004F7807> _unk_4F7807;
    static loco_global<uint8_t[10], 0x004F7830> _unk_4F7830;
    static loco_global<uint8_t[10], 0x004F783B> _unk_4F783B;
    static loco_global<uint8_t[10], 0x004F7846> _unk_4F7846;
    static loco_global<uint8_t[10], 0x004F7851> _unk_4F7851;
    static loco_global<uint8_t[10], 0x004F785C> _unk_4F785C;
    static loco_global<uint8_t[10], 0x004F7867> _unk_4F7867;
    static loco_global<uint8_t[10], 0x004F7872> _unk_4F7872;
    static loco_global<uint8_t[10], 0x004F787D> _unk_4F787D;
    static loco_global<uint8_t[10], 0x004F7888> _unk_4F7888;
    static loco_global<uint8_t[10], 0x004F7893> _unk_4F7893;
    static loco_global<uint8_t[10], 0x004F789E> _unk_4F789E;
    static loco_global<uint8_t[10], 0x004F78A9> _unk_4F78A9;
    static loco_global<uint8_t[10], 0x004F78B4> _unk_4F78B4;
    static loco_global<uint8_t[10], 0x004F78BF> _unk_4F78BF;
    static loco_global<uint8_t[10], 0x004F78CA> _unk_4F78CA;
    static loco_global<uint8_t[10], 0x004F78D5> _unk_4F78D5;
    static loco_global<uint8_t[10], 0x004F78E0> _unk_4F78E0;
    static loco_global<uint8_t[10], 0x004F78EB> _unk_4F78EB;

    static loco_global<uint8_t[40][8], 0x004F7B62> _word_4F7B62;

    static loco_global<uint16_t[2], 0x004FFAF0> _dword_4FFAF0;
    static loco_global<uint8_t[31], 0x005045FA> _byte_5045FA;
    static loco_global<uint8_t, 0x00522095> _byte_522095;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint32_t, 0x00523394> _toolWidgetIndex;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;
    static loco_global<uint8_t[8], 0x0525F72> _byte_525F72;
    static loco_global<uint8_t[8], 0x0525F7A> _selectedBridges;
    static loco_global<uint8_t[8], 0x0525F82> _byte_525F82;
    static loco_global<uint8_t[8], 0x0525F8A> _byte_525F8A;
    static loco_global<uint8_t[8], 0x0525F9A> _selectedStations;
    static loco_global<uint8_t[8], 0x0525FA2> _selectedMods;
    static loco_global<uint8_t, 0x00525FAA> _lastRailroadOption;
    static loco_global<uint8_t, 0x00525FAB> _lastRoadOption;
    static loco_global<uint8_t, 0x00525FAC> _haveAirports;
    static loco_global<uint8_t, 0x00525FAD> _haveShipPorts;
    static loco_global<company_id_t, 0x009C68EB> _updatingCompanyId;
    static loco_global<uint32_t, 0x00E0C3E0> _dword_E0C3E0;
    static loco_global<tile_element* [0x30004], 0x00E40134> _tiles;
    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<map_pos[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;
    static loco_global<uint32_t, 0x01135F3E> _constructionCost;
    static loco_global<uint16_t, 0x01135F86> _word_1135F86;
    static loco_global<uint16_t, 0x01135FB4> _x;
    static loco_global<uint16_t, 0x01135FB6> _y;
    static loco_global<uint32_t, 0x01135F3E> _trackCost;
    static loco_global<uint32_t, 0x01135F4E> _dword_1135F4E;
    static loco_global<uint32_t, 0x01135F6C> _dword_1135F6C;
    static loco_global<uint32_t, 0x01135F46> _dword_1135F46;
    static loco_global<uint16_t, 0x01135FB8> _word_1135FB8;
    static loco_global<uint16_t, 0x01135FD6> _word_1135FD6;
    static loco_global<uint16_t, 0x01135FD8> _word_1135FD8;
    static loco_global<uint16_t, 0x01135FE4> _word_1135FE4;
    static loco_global<uint16_t, 0x01136000> _word_1136000;
    static loco_global<uint8_t[17], 0x0113601D> _signalList;
    static loco_global<uint8_t, 0x0113602E> _lastSelectedSignal;
    static loco_global<uint8_t[9], 0x01136030> _bridgeList;
    static loco_global<uint8_t, 0x01136039> _lastSelectedBridge;
    static loco_global<uint8_t, 0x0113603A> _byte_113603A;
    static loco_global<uint8_t[17], 0x0113603B> _stationList;
    static loco_global<uint8_t, 0x0113604C> _lastSelectedStationBuilding;
    static loco_global<uint8_t[4], 0x01136054> _modList;
    static loco_global<uint8_t, 0x01136061> _constructionHover;
    static loco_global<uint8_t, 0x01136062> _trackType;
    static loco_global<uint8_t, 0x01136063> _byte_1136063;
    static loco_global<uint8_t, 0x01136064> _constructionRotation;
    static loco_global<uint8_t, 0x01136064> _byte_1136065;
    static loco_global<uint8_t, 0x01136067> _byte_1136067;
    static loco_global<uint8_t, 0x01136068> _byte_1136068;
    static loco_global<uint8_t, 0x0113606E> _byte_113606E;
    static loco_global<uint8_t, 0x01136076> _byte_1136076;
    static loco_global<uint8_t, 0x01136077> _byte_1136077;
    static loco_global<uint8_t, 0x01136078> _byte_1136078;
    static loco_global<uint8_t, 0x01136079> _byte_1136079;
    static loco_global<uint8_t, 0x0113607E> _byte_113607E;

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

        static void prepare_draw(window* self);
        static void switchTab(window* self, widget_index widgetIndex);
        static void repositionTabs(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void init_events();
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
            bridge_dropdown,
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
            (1ULL << widx::bridge_dropdown) |
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

        // 0x0049FEC7
        static void sub_49FEC7()
        {
            registers regs;
            call(0x0049FEC7, regs);
        }

        // 0x0049DD14
        static void on_close(window* self)
        {
            sub_49FEC7();
            WindowManager::viewportSetVisibility(0);
            tilemgr::map_invalidate_map_selection_tiles();
            _mapSelectionFlags = _mapSelectionFlags & ~MapSelectFlag::enableConstruct;
            windows::hideDirectionArrows();
            windows::hideGridlines();
        }

        // 0x0049D3F6
        static void on_mouse_up(window* self, widget_index widgetIndex)
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
            regs.esi = (int32_t)&self;
            switch (widgetIndex)
            {
                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_construction:
                case common::widx::tab_overhead:
                case common::widx::tab_signal:
                case common::widx::tab_station:
                    common::switchTab(self, widgetIndex);
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
                case widx::rotate_90:
                {
                    if (_constructionHover == 1)
                    {
                        _constructionRotation++;
                        _constructionRotation = _constructionRotation & 3;
                        _trackCost = 0x80000000;
                        sub_49F1B5();
                        break;
                    }
                    sub_49FEC7();
                    WindowManager::viewportSetVisibility(3);
                    input::toolSet(self, widx::construct, 12);
                    input::set_flag(input::input_flags::flag6);
                    _constructionHover = 1;
                    _byte_113607E = 0;
                    _constructionRotation = _constructionRotation & 3;
                    sub_49F1B5();
                    break;
                }
            }
        }

        static void sub_49DBEC(window* self, uint64_t disabledWidgets)
        {
            if (_byte_1136068 == 2 || _byte_1136068 == 6)
            {
                disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
                disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);
                disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
            }

            if (_byte_1136068 == 4 || _byte_1136068 == 8)
            {
                disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
                disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);
                disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
            }

            if (_constructionRotation <= 12)
            {
                if (_constructionRotation > 8)
                {
                    disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_small);
                    disabledWidgets |= (1 << widx::s_bend_right) | (1 << widx::slope_down) | (1 << widx::slope_up);
                }
                else
                {
                    if (_constructionRotation >= 4)
                    {
                        disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_small);
                        disabledWidgets |= (1 << widx::s_bend_left) | (1 << widx::slope_down) | (1 << widx::slope_up);
                    }
                }
            }
            else
            {
                disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small) | (1 << widx::right_hand_curve_small);
                disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right) | (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
            }

            if (_constructionHover == 0)
            {
                registers regs;
                auto flags = call(0x004A0832, regs);
                auto carryFlag = (flags >> 16) & (1 << 1);
                if (carryFlag)
                    disabledWidgets |= (1 << widx::construct);
            }
            self->set_disabled_widgets_and_invalidate(disabledWidgets);
        }

        static void sub_49DB1F(window* self, track_object trackObj, uint64_t disabledWidgets)
        {
            if (_byte_1136068 == 2 || _byte_1136068 == 6)
            {
                disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
                disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);
                if (!(trackObj.track_pieces & (1 << 8)))
                    disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
            }
            if (_byte_1136068 == 4 || _byte_1136068 == 8)
            {
                disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
                disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);
                if (!(trackObj.track_pieces & (1 << 8)))
                    disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
            }

            if (_constructionRotation <= 12)
            {
                if (_constructionRotation > 8)
                {
                    disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_small);
                    disabledWidgets |= (1 << widx::s_bend_right) | (1 << widx::slope_down) | (1 << widx::slope_up);
                }
                else
                {
                    if (_constructionRotation >= 4)
                    {
                        disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_small);
                        disabledWidgets |= (1 << widx::s_bend_left) | (1 << widx::slope_down) | (1 << widx::slope_up);
                    }
                }
            }
            else
            {
                disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small) | (1 << widx::right_hand_curve_small);
                disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right) | (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
            }

            if (_constructionHover == 0)
            {
                registers regs;
                auto flags = call(0x004A04F8, regs);
                auto carryFlag = (flags >> 16) & (1 << 1);
                if (carryFlag)
                    disabledWidgets |= (1 << widx::construct);
            }
            self->set_disabled_widgets_and_invalidate(disabledWidgets);
        }

        static void sub_49DAF3(window* self, track_object trackObj, uint64_t disabledWidgets)
        {
            auto trackPieces = trackObj.track_pieces & ((1 << 5) |(1<<8));

            if (trackPieces != ((1 << 5) | (1 << 8)))
                disabledWidgets |= (1 << widx::slope_down) | (1 << widx::slope_up);

            trackPieces = trackObj.track_pieces & ((1 << 6) | (1 << 8));

            if (trackPieces != ((1 << 6) | (1 << 8)))
                disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::steep_slope_up);

            sub_49DB1F(self, trackObj, disabledWidgets);
        }

        // 0x0049DAA5
        static void on_resize(window* self)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //call(0x0049DAA5, regs);

            self->enabled_widgets &= ~(1 << widx::construct);
            if (_constructionHover != 1)
                self->enabled_widgets |= (1 << widx::construct);
            auto disabledWidgets = self->disabled_widgets;
            disabledWidgets &= (1 << common::widx::tab_construction | 1 << common::widx::tab_overhead | 1 << common::widx::tab_signal | 1 << common::widx::tab_station);
            uint8_t trackType = _trackType;
            if (trackType & (1 << 7))
            {
                trackType &= ~(1 << 7);
                // auto roadObj = objectmgr::get<road_object>(trackType);
                if (_byte_1136067 == 0xFF)
                {
                    sub_49DBEC(self, disabledWidgets);
                    return;
                }
                switch (_byte_1136067)
                {
                    case 0:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    {
                        sub_49DBEC(self, disabledWidgets);
                        break;
                    }

                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 13:
                    {
                        disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
                        break;
                    }
                }
            }
            else
            {
                auto trackObj = objectmgr::get<track_object>(trackType);
                if (_byte_1136067 == 0xFF)
                {
                    sub_49DB1F(self, *trackObj, disabledWidgets);
                    return;
                }
                switch (_byte_1136067)
                {
                    case 0:
                        sub_49DB1F(self, *trackObj, disabledWidgets);
                        break;

                    case 1:
                    case 2:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    {
                        disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
                        sub_49DB1F(self, *trackObj, disabledWidgets);
                        break;
                    }

                    case 3:
                    case 4:
                    {
                        sub_49DAF3(self, *trackObj, disabledWidgets);
                        break;
                    }
                }
            }
        }

        // 0x0049D42F
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            std::printf("on_mouse_down\n");
            std::printf("%d\n", widgetIndex);
            registers regs;
            regs.esi = (int32_t)self;
            regs.edx = widgetIndex;
            call(0x0049D42F, regs);
        }

        // 0x0049D4EA
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == widx::bridge_dropdown)
            {
                if (itemIndex != -1)
                {
                    auto bridge = _bridgeList[itemIndex];
                    _lastSelectedBridge = bridge;
                    _selectedBridges[_trackType] = bridge;
                    sub_49FEC7();
                    _trackCost = 0x80000000;
                    sub_49F1B5();
                }
            }
        }

        // 0x0049DCA2
        static void on_update(window* self)
        {
            self->frame_no++;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::construction, self->number);

            if (_constructionHover == 1)
            {
                if (!input::is_tool_active(WindowType::construction, self->number) || _toolWidgetIndex != construction::widx::construct)
                    WindowManager::close(self);
            }
            if (_constructionHover == 0)
            {
                if (input::has_flag(input::input_flags::tool_active) && _toolWindowType == WindowType::construction)
                    input::cancel_tool();
            }
        }

        // 0x0049DC8C
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049DC8C, regs);
        }

        // 0x0049DC97
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            std::printf("on_tool_down\n");
            std::printf("%d\n", widgetIndex);
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049DC97, regs);

            //if (widgetIndex != widx::construct)
            //    return;

            //tilemgr::map_invalidate_map_selection_tiles();
            //sub_49FEC7();

            //registers regs;
            //uint32_t flags;

            //if (_trackType & (1 << 7))
            //{
            //    flags = call(0x004A0832, regs);
            //}
            //else
            //{
            //    flags = call(0x004A04F8, regs);
            //}
            //auto edi = regs.edi;
            //auto carryFlag = (flags >> 16) & (1 << 1);

            //if (carryFlag)
            //    return;

            //auto i = 0;
            //_byte_1136065 = regs.dh;

            //if (input::getMapSelectionFlags() & MapSelectFlag::enableConstruct)
            //{
            //    auto count = 0;
            //    auto pos = _mapSelectedTiles[count];
            //    uint16_t dx = 0;

            //    while (pos.x != -1)
            //    {
            //        std::printf("%d\n", count);
            //        pos = _mapSelectedTiles[count];

            //        if (pos.x == -1)
            //        {
            //            count++;
            //            continue;
            //        }

            //        if (pos.y == -1)
            //        {
            //            count++;
            //            continue;
            //        }

            //        auto tileId = ((pos.y << 9) | pos.x) >> 3;
            //        auto tile = _tiles[tileId]->as_track();

            //        if (tile == nullptr)
            //        {
            //            count++;
            //            continue;
            //        }

            //        if (!tile->has_type_3C())
            //        {
            //            while (tile->has_type_3C())
            //            {
            //                tileId++;
            //                tile = _tiles[tileId]->as_track();
            //            }
            //        }
            //        dx = tile->base_z() << 2;

            //        if (tile->has_4_F())
            //            dx += 16;

            //        if (tile->has_4_10())
            //            dx += 16;

            //        if (dx > i)
            //            i = dx;

            //        dx &= ~(0xFF);
            //        dx |= tile->track_object_id();
            //        dx &= 0x1F;

            //        if (dx > i)
            //            i = dx;

            //        count++;
            //    }
            //    edi = pos.x << 16 | pos.y;
            //}
            //registers regs2;
            //regs2.esi = (int32_t)&self;
            //regs2.edi = edi;
            //if (_trackType & (1 << 7))
            //{
            //    call(0x004A23F8, regs2);
            //}
            //else
            //{
            //    call(0x004A212D, regs2);
            //}
            //_word_1136000 = i;
            //_mapSelectionFlags = _mapSelectionFlags & ~(1 << 2 | 1 << 1 | 1 << 0);
            //registers regs;
            //if (_trackType & (1 << 7))
            //{
            //    call(0x00478361, regs);
            //}
            //else
            //{
            //    call(0x004A4011, regs);
            //}
            //if (!regs.eax)
            //{
            //    uint32_t ebp = regs.dl << 3;
            //    if (_word_4F7B62[ebp][0] == 0)
            //    {

            //    }
            //}
        }

        // 0x0049D4F5
        static ui::cursor_id cursor(window* self, int16_t widgetIndex, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
        {
            if (widgetIndex == widx::bridge || widgetIndex == widx::bridge_dropdown)
                _tooltipTimeout = 2000;
            return fallback;
        }

        // 0x0049CE79
        static void prepare_draw(window* self)
        {
            //registers regs;
            //regs.esi = (uint32_t)self;
            //call(0x0049CE79, regs);

            common::prepare_draw(self);
            auto args = FormatArguments();
            if (_trackType & (1 << 7))
            {
                auto roadObj = objectmgr::get<road_object>(_trackType & ~(1 << 7));
                args.push(roadObj->name);
            }
            else
            {
                auto trackObj = objectmgr::get<track_object>(_trackType);
                args.push(trackObj->name);
            }
            if (_lastSelectedBridge != 0xFF)
            {
                auto bridgeObj = objectmgr::get<bridge_object>(_lastSelectedBridge);
                if (bridgeObj != nullptr)
                {
                    args.push(bridgeObj->name);
                    if (bridgeObj->max_speed == 0xFF)
                    {
                        args.push(string_ids::unlimited_speed);
                        args.push(bridgeObj->max_speed);
                    }
                    else
                    {
                        args.push(string_ids::velocity);
                        args.push(bridgeObj->max_speed);
                    }
                    args.push<uint16_t>(bridgeObj->max_height);
                }
            }
            common::repositionTabs(self);
        }

        static registers sub_4A0AE5(uint16_t ax, uint16_t cx, uint32_t edi, uint16_t bh, uint32_t edx)
        {
            registers regs;
            regs.ax = ax;
            regs.cx = cx;
            regs.edi = edi;
            regs.bh = bh;
            regs.edx = edx;
            call(0x004A0AE5, regs);
            return regs;
        }

        static registers sub_478F1F(uint16_t ax, uint16_t cx, uint32_t edi, uint16_t bh, uint32_t edx)
        {
            registers regs;
            regs.ax = ax;
            regs.cx = cx;
            regs.edi = edi;
            regs.bh = bh;
            regs.edx = edx;
            call(0x00478F1F, regs);
            return regs;
        }

        // 0x0049D38A and 0x0049D16B
        static void loc_49D38A(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto x = self->widgets[widx::construct].width() + 1;
            x >>= 1;
            x += self->x;
            auto y = self->widgets[widx::construct].bottom + self->y - 23;

            if (_constructionHover != 1)
                gfx::draw_string_centred(*dpi, x, y, colour::black, string_ids::build_this);

            y += 11;

            if (_constructionCost != 0x80000000)
            {
                if (_constructionCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_constructionCost);
                    gfx::draw_string_centred(*dpi, x, y, colour::black, string_ids::build_cost, &args);
                }
            }
        }

        static void loc_49D106(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, uint16_t bp, uint16_t si)
        {
            bp >>= 1;
            si >>= 1;
            si += 0x10;
            ax -= bp;
            cx -= si;
            clipped->x += ax;
            clipped->y += cx;
            _dword_E0C3E0 = (uint32_t)clipped;
            auto x = 0x2000;
            auto y = 0x2000;
            auto edi = _word_1135FD8 << 16 | 0x1E0;
            auto edx = _word_1135FD6 << 16 | _byte_1136079 << 8 | _byte_1136077;
            _byte_522095 = _byte_522095 | (1 << 1);

            sub_4A0AE5(x, y, edi, _byte_1136078, edx);

            _byte_522095 = _byte_522095 & ~(1 << 1);

            loc_49D38A(self, dpi);
        }

        static void loc_49D325(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, uint16_t bp, uint16_t si)
        {
            bp >>= 1;
            si >>= 1;
            si += 0x10;
            ax -= bp;
            cx -= si;
            clipped->x += ax;
            clipped->y += cx;
            _dword_E0C3E0 = (uint32_t)clipped;
            auto x = 0x2000;
            auto y = 0x2000;
            auto edi = _word_1135FD8 << 16 | 0x1E0;
            auto edx = _word_1135FD6 << 16 | _byte_1136079 << 8 | _byte_1136077;
            _byte_522095 = _byte_522095 | (1 << 1);

            sub_478F1F(x, y, edi, _byte_1136078, edx);

            _byte_522095 = _byte_522095 & ~(1 << 1);

            loc_49D38A(self, dpi);
        }

        static void sub_49D0B8(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax = -ax + cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D106(self, clipped, dpi, ax, cx, bp, si);
        }

        static void loc_49D0CC(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            ax = -ax;
            auto bx = ax;
            ax -= cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D106(self, clipped, dpi, ax, cx, bp, si);
        }

        static void loc_49D0E0(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax -= cx;
            cx = -cx;
            cx -= bx;
            cx >>= 1;
            cx -= dx;
            loc_49D106(self, clipped, dpi, ax, cx, bp, si);
        }

        static void loc_49D0F4(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax += cx;
            cx = -cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D106(self, clipped, dpi, ax, cx, bp, si);
        }

        static void sub_49D2D7(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax = -ax + cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D325(self, clipped, dpi, ax, cx, bp, si);
        }

        static void loc_49D2EB(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            ax = -ax;
            auto bx = ax;
            ax -= cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D325(self, clipped, dpi, ax, cx, bp, si);
        }

        static void loc_49D2FF(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax -= cx;
            cx = -cx;
            cx -= bx;
            cx >>= 1;
            cx -= dx;
            loc_49D325(self, clipped, dpi, ax, cx, bp, si);
        }

        static void loc_49D313(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax += cx;
            cx = -cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D325(self, clipped, dpi, ax, cx, bp, si);
        }

        // 0x0049CF36
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //regs.edi = (int32_t)dpi;
            //call(0x0049CF36, regs);

            self->draw(dpi);
            common::drawTabs(self, dpi);

            if (self->widgets[widx::bridge].type != widget_type::none)
            {
                if (_lastSelectedBridge != 0xFF)
                {
                    auto bridgeObj = objectmgr::get<bridge_object>(_lastSelectedBridge);
                    if (bridgeObj != nullptr)
                    {
                        auto company = companymgr::get(_playerCompany);
                        auto imageId = gfx::recolour(bridgeObj->var_16, company->mainColours.primary);
                        auto x = self->x + self->widgets[widx::bridge].left + 2;
                        auto y = self->y + self->widgets[widx::bridge].top + 1;

                        gfx::draw_image(dpi, x, y, imageId);
                    }
                }
            }

            if (self->widgets[widx::construct].type == widget_type::none)
                return;

            if (_trackType & (1 << 7))
            {
                registers regs;
                auto flags = call(0x004A0832, regs);
                auto carryFlag = (flags >> 16) & (1 << 1);
                _word_1135FD8 = regs.edi >> 16;
                if (carryFlag)
                    return;
                _byte_1136077 = regs.dl;
                _byte_1136078 = regs.bh;
                _byte_1136079 = regs.dh;
                _word_1135FD6 = (regs.edx >> 16) & 0x1F;

                auto x = self->x + self->widgets[widx::construct].left + 1;
                auto y = self->y + self->widgets[widx::construct].top + 1;
                auto width = self->widgets[widx::construct].width();
                auto height = self->widgets[widx::construct].width();

                gfx::drawpixelinfo_t* clipped = nullptr;

                if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                {
                    static const uint8_t* offset_4F6D1C[] = {
                        _unk_4F6D44,
                        _unk_4F6D4F,
                        _unk_4F6D5A,
                        _unk_4F6D65,
                        _unk_4F6D8E,
                        _unk_4F6DB7,
                        _unk_4F6DCC,
                        _unk_4F6DE1,
                        _unk_4F6DEC,
                        _unk_4F6DF7,
                    };

                    auto ecx = offset_4F6D1C[_byte_1136079];
                    auto i = 0;

                    while (ecx[i + 10] != 0xFF)
                    {
                        i += 10;
                    }

                    int16_t ax = ecx[i + 1];
                    int16_t dx = ecx[i + 5];
                    int16_t cx = ecx[i + 3];

                    if (ecx[i + 9] & (1 << 6))
                    {
                        ax = 0;
                        cx = 0;
                    }

                    switch (_byte_1136078 & 3)
                    {
                        case 0:
                            break;
                        case 1:
                        {
                            std::swap(ax, cx);
                            cx = -cx;
                            break;
                        }
                        case 2:
                        {
                            ax = -ax;
                            cx = -cx;
                            break;
                        }
                        case 3:
                        {
                            std::swap(ax, cx);
                            ax = -ax;
                            break;
                        }
                    }
                    ax >>= 1;
                    cx >>= 1;
                    ax += 0x2010;
                    cx += 0x2010;
                    dx += 0x1CC;
                    switch (gCurrentRotation)
                    {
                        case 0:
                            sub_49D2D7(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 1:
                            loc_49D2EB(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 2:
                            loc_49D2FF(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 3:
                            loc_49D313(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                    }
                }
                else
                {
                    loc_49D38A(self, dpi);
                }
            }
            else
            {
                registers regs;
                auto flags = call(0x004A04F8, regs);
                auto carryFlag = (flags >> 16) & (1 << 1);
                _word_1135FD8 = regs.edi >> 16;
                if (carryFlag)
                    return;
                _byte_1136077 = regs.dl;
                _byte_1136078 = regs.bh;
                _byte_1136079 = regs.dh;
                _word_1135FD6 = (regs.edx >> 16) & 0x1F;

                auto x = self->x + self->widgets[widx::construct].left + 1;
                auto y = self->y + self->widgets[widx::construct].top + 1;
                auto width = self->widgets[widx::construct].width();
                auto height = self->widgets[widx::construct].width();

                gfx::drawpixelinfo_t* clipped = nullptr;

                if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                {
                    static const uint8_t* offset_4F6D1C[] = {
                        _unk_4F7488,
                        _unk_4F7493,
                        _unk_4F74BC,
                        _unk_4F74C7,
                        _unk_4F74D2,
                        _unk_4F74FB,
                        _unk_4F7524,
                        _unk_4F7557,
                        _unk_4F758A,
                        _unk_4F75BD,
                        _unk_4F75F0,
                        _unk_4F7623,
                        _unk_4F7656,
                        _unk_4F767F,
                        _unk_4F76A8,
                        _unk_4F76BD,
                        _unk_4F76D2,
                        _unk_4F76DD,
                        _unk_4F76E8,
                        _unk_4F7711,
                        _unk_4F773A,
                        _unk_4F7763,
                        _unk_4F778C,
                        _unk_4F77B5,
                        _unk_4F77DE,
                        _unk_4F7807,
                        _unk_4F7830,
                        _unk_4F783B,
                        _unk_4F7846,
                        _unk_4F7851,
                        _unk_4F785C,
                        _unk_4F7867,
                        _unk_4F7872,
                        _unk_4F787D,
                        _unk_4F7888,
                        _unk_4F7893,
                        _unk_4F789E,
                        _unk_4F78A9,
                        _unk_4F78B4,
                        _unk_4F78BF,
                        _unk_4F78CA,
                        _unk_4F78D5,
                        _unk_4F78E0,
                        _unk_4F78EB,
                    };

                    auto ecx = offset_4F6D1C[_byte_1136079];
                    auto i = 0;

                    while (ecx[i + 10] != 0xFF)
                    {
                        i += 10;
                    }

                    int16_t ax = ecx[i + 1];
                    int16_t dx = ecx[i + 5];
                    int16_t cx = ecx[i + 3];

                    if (ecx[i + 9] & (1 << 6))
                    {
                        ax = 0;
                        cx = 0;
                    }

                    switch (_byte_1136078 & 3)
                    {
                        case 0:
                            break;
                        case 1:
                        {
                            std::swap(ax, cx);
                            cx = -cx;
                            break;
                        }
                        case 2:
                        {
                            ax = -ax;
                            cx = -cx;
                            break;
                        }
                        case 3:
                        {
                            std::swap(ax, cx);
                            ax = -ax;
                            break;
                        }
                    }
                    ax >>= 1;
                    cx >>= 1;
                    ax += 0x2010;
                    cx += 0x2010;
                    dx += 0x1CC;
                    switch (gCurrentRotation)
                    {
                        case 0:
                            sub_49D0B8(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 1:
                            loc_49D0CC(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 2:
                            loc_49D0E0(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 3:
                            loc_49D0F4(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                    }
                }
                else
                {
                    loc_49D38A(self, dpi);
                }
            }
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
    static void setTrackOptions(const uint8_t trackType)
    {
        auto newTrackType = trackType;
        if (trackType & (1 << 7))
        {
            newTrackType &= ~(1 << 7);
            auto roadObj = objectmgr::get<road_object>(newTrackType);
            if (!(roadObj->flags & flags_12::unk_01))
                _lastRoadOption = trackType;
            else
                _lastRailroadOption = trackType;
        }
        else
        {
            auto trackObj = objectmgr::get<track_object>(newTrackType);
            if (!(trackObj->flags & flags_22::unk_02))
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
            disabledWidgets |= (1ULL << common::widx::tab_station);

        if (_byte_1136063 & 0xC0)
            disabledWidgets |= (1ULL << common::widx::tab_construction);

        if (_lastSelectedSignal == 0xFF)
            disabledWidgets |= (1ULL << common::widx::tab_signal);

        if (_modList[0] == 0xFFFFFFFF)
            disabledWidgets |= (1ULL << common::widx::tab_overhead);

        if (_lastSelectedStationBuilding == 0xFF)
            disabledWidgets |= (1ULL << common::widx::tab_station);

        self->disabled_widgets = disabledWidgets;
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
        ui::windows::showDirectionArrows();
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
        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::airport); i++)
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
        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::dock); i++)
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

        for (auto i = 0; i < roadObj->num_stations; i++)
        {
            auto station = roadObj->stations[i];
            if (station == 0xFF)
                continue;
            auto roadStationObj = objectmgr::get<road_station_object>(station);
            if (currentYear < roadStationObj->designed_year)
                continue;
            if (currentYear > roadStationObj->obsolete_year)
                continue;
            _stationList[roadStationCount] = i;
            roadStationCount++;
        }

        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::road_station); i++)
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
        for (auto i = 0; i < roadObj->num_bridges; i++)
        {
            auto bridge = roadObj->bridges[i];
            if (bridge == 0xFF)
                continue;
            auto bridgeObj = objectmgr::get<bridge_object>(bridge);
            if (currentYear < bridgeObj->designed_year)
                continue;
            _bridgeList[bridgeCount] = i;
            bridgeCount++;
        }

        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::bridge); i++)
        {
            auto bridgeObj = objectmgr::get<bridge_object>(i);
            if (bridgeObj == nullptr)
                continue;
            for (auto j = 0; j < bridgeObj->var_22; j++)
            {
                if (trackType != bridgeObj->var_23)
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
        _modList[0] = 0xFF;
        _modList[1] = 0xFF;
        _modList[2] = 0xFF;
        _modList[3] = 0xFF;
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
                _modList[i] = roadObj->mods[i];
        }
    }

    // 0x004A3A50
    static void sub_4A3A50()
    {
        construction::sub_49FEC7();
        setTrackOptions(_trackType);
        refreshRoadStationList();

        auto al = _selectedStations[(_trackType & ~(1ULL << 7))];
        if (al == 0xFF)
            al = _stationList[0];
        _lastSelectedStationBuilding = al;

        refreshRoadBridgeList();

        al = _selectedBridges[(_trackType & ~(1ULL << 7))];
        if (al == 0xFF)
            al = _bridgeList[0];
        _lastSelectedBridge = al;

        refreshRoadOption();

        al = _selectedMods[(_trackType & ~(1ULL << 7))];
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
    //    auto bh = _constructionRotation;
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
    //    edx |= (_lastSelectedBridge << 24);
    //    auto edi = _word_1135FB8 | _word_1135FE4 << 16;
    //    if (_x)
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
        while (var_0E > 0)
        {
            auto ecx = utility::bitscanforward(var_0E);
            if (ecx == -1)
                break;
            var_0E &= ~(1 << ecx);
            auto signalObj = objectmgr::get<train_signal_object>(ecx);
            std::printf("start: %d end: %d\n", signalObj->designed_year, signalObj->obsolete_year);
            if (currentYear > signalObj->obsolete_year)
                continue;
            if (currentYear < signalObj->designed_year)
                continue;
            _signalList[signalCount] = ecx;
            signalCount++;
        }

        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::track_signal); i++)
        {
            auto signalObj = objectmgr::get<train_signal_object>(i);
            if (signalObj == nullptr)
                continue;
            for (auto j = 0; j < signalObj->var_12; j++)
            {
                if (trackType != objectmgr::get<train_signal_object>(i + j)->var_13)
                    continue;
                if (currentYear < signalObj->designed_year)
                    continue;
                if (currentYear > signalObj->obsolete_year)
                    continue;
                for (size_t k = 0; k < std::size(_signalList); k++)
                {
                    auto signal = signalListCopy[k];
                    if (signalListCopy == _signalList)
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

        for (auto i = 0; i < trackObj->num_stations; i++)
        {
            auto station = trackObj->stations[i];
            if (station == 0xFF)
                continue;
            auto trainStationObj = objectmgr::get<train_station_object>(station);
            if (currentYear < trainStationObj->designed_year)
                continue;
            if (currentYear > trainStationObj->obsolete_year)
                continue;
            _stationList[trainStationCount] = i;
            trainStationCount++;
        }

        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::track_station); i++)
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
        for (auto i = 0; i < trackObj->num_bridges; i++)
        {
            auto bridge = trackObj->bridges[i];
            if (bridge == 0xFF)
                continue;
            auto bridgeObj = objectmgr::get<bridge_object>(bridge);
            if (currentYear < bridgeObj->designed_year)
                continue;
            _bridgeList[bridgeCount] = i;
            bridgeCount++;
        }

        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::bridge); i++)
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
        _modList[0] = 0xFF;
        _modList[1] = 0xFF;
        _modList[2] = 0xFF;
        _modList[3] = 0xFF;
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
                _modList[i] = trackObj->mods[i];
        }
    }

    window* check(uint8_t al)
    {
        if (al == 0xFF)
        {
            al = _stationList[0];
        }

        _lastSelectedStationBuilding = al;
        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            setDisabledWidgets(window);
        }

        window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            window->call_on_mouse_up(common::widx::tab_construction);
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
            window->call_on_mouse_up(construction::widx::rotate_90);
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
        _x = 0x1800;
        _y = 0x1800;
        _word_1135FB8 = 0x100;
        _constructionRotation = 0;
        _constructionHover = 0;
        _byte_113607E = 1;
        _constructionCost = 0x80000000;
        _byte_1136076 = 0;
        _byte_1136067 = 0;
        _byte_1136068 = 0;
        _byte_113606E = 0;

        setTrackOptions(flags);

        if (flags & 0x80000000)
        {
            createConstructionWindow();

            _lastSelectedSignal = 0xFF;
            _modList[0] = 0xFF;
            _modList[1] = 0xFF;
            _modList[2] = 0xFF;
            _modList[3] = 0xFF;
            _word_1135FE4 = 0;
            _lastSelectedBridge = 0xFF;

            refreshAirportList();

            auto al = _haveAirports;

            return check(al);
        }
        else
        {
            if (flags & 0x40000000)
            {
                createConstructionWindow();

                _lastSelectedSignal = 0xFF;

                _modList[0] = 0xFF;
                _modList[1] = 0xFF;
                _modList[2] = 0xFF;
                _modList[3] = 0xFF;

                _word_1135FE4 = 0;
                _lastSelectedBridge = 0xFF;

                refreshDockList();

                auto al = _haveShipPorts;

                return check(al);
            }
            else
            {
                if (flags & (1 << 7))
                {
                    createConstructionWindow();

                    _lastSelectedSignal = 0xFF;

                    refreshRoadStationList();

                    auto al = _selectedStations[(_trackType & ~(1ULL << 7))];

                    if (al == 0xFF)
                        al = _stationList[0];

                    _lastSelectedStationBuilding = al;

                    refreshRoadBridgeList();

                    al = _selectedBridges[(_trackType & ~(1ULL << 7))];

                    if (al == 0xFF)
                        al = _bridgeList[0];

                    _lastSelectedBridge = al;

                    refreshRoadOption();

                    al = _selectedMods[(_trackType & ~(1ULL << 7))];

                    if (al == 0xff)
                        al = 0;

                    return checkB(al);
                }
            }
        }
        createConstructionWindow();

        refreshSignalList();

        auto al = _byte_525F72[_trackType];

        if (al == 0xFF)
            al = _signalList[0];

        _lastSelectedSignal = al;

        refreshtrainStationList();

        al = _byte_525F82[_trackType];

        if (al == 0xFF)
            al = _stationList[0];

        _lastSelectedStationBuilding = al;

        refreshTrackBridgeList();

        al = _selectedBridges[_trackType];

        if (al == 0xFF)
            al = _bridgeList[0];

        _lastSelectedBridge = al;

        refreshTrackOption();

        al = _byte_525F8A[_trackType];

        if (al == 0xFF)
            al = 0;

        return checkB(al);
    }

    // 0x004A6FAC
    void sub_4A6FAC()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window == nullptr)
            return;
        if (window->current_tab == common::widx::tab_station - common::widx::tab_construction)
        {
            if (_byte_1136063 & 0xC0)
                WindowManager::close(window);
            else
                window->call_on_mouse_up(common::widx::tab_construction);
        }
    }

    namespace station
    {
        enum widx
        {
            station = 8,
            station_dropdown,
            image,
            rotate,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << station) | (1 << station_dropdown) | (1 << image) | (1 << rotate);

        widget_t widgets[] = {
            commonWidgets(138, 190, string_ids::stringid_2),
            make_widget({ 3, 45 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, string_ids::tooltip_select_station_type),
            make_widget({ 123, 46 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::tooltip_select_station_type),
            make_widget({ 35, 60 }, { 68, 68 }, widget_type::wt_3, 1),
            make_widget({ 112, 104 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rotate_object, string_ids::rotate_90),
            widget_end(),
        };

        static window_event_list events;
    }

    namespace signal
    {
        enum widx
        {
            signal = 8,
            signal_dropdown,
            both_directions,
            single_direction,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << signal) | (1 << signal_dropdown) | (1 << both_directions) | (1 << single_direction);

        widget_t widgets[] = {
            commonWidgets(138, 190, string_ids::stringid_2),
            make_widget({ 3, 45 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, string_ids::tooltip_select_signal_type),
            make_widget({ 123, 46 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::tooltip_select_signal_type),
            make_widget({ 27, 110 }, { 40, 40 }, widget_type::wt_9, 1, 0xFFFFFFFF, string_ids::tooltip_signal_both_directions),
            make_widget({ 71, 110 }, { 40, 40 }, widget_type::wt_9, 1, 0xFFFFFFFF, string_ids::tooltip_signal_single_direction),
            widget_end(),
        };

        static window_event_list events;
    }

    namespace overhead
    {
        enum widx
        {
            checkbox_1 = 8,
            checkbox_2,
            checkbox_3,
            checkbox_4,
            image,
            track,
            track_dropdown,
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << checkbox_1) | (1 << checkbox_2) | (1 << checkbox_3) | (1 << checkbox_4) | (1 << image) | (1 << track) | (1 << track_dropdown);

        widget_t widgets[] = {
            commonWidgets(138, 167, string_ids::stringid_2),
            make_widget({ 3, 45 }, { 132, 12 }, widget_type::checkbox, 1, string_ids::empty, string_ids::tooltip_select_track_mod),
            make_widget({ 3, 57 }, { 132, 12 }, widget_type::checkbox, 1, string_ids::empty, string_ids::tooltip_select_track_mod),
            make_widget({ 3, 69 }, { 132, 12 }, widget_type::checkbox, 1, string_ids::empty, string_ids::tooltip_select_track_mod),
            make_widget({ 3, 81 }, { 132, 12 }, widget_type::checkbox, 1, string_ids::empty, string_ids::tooltip_select_track_mod),
            make_widget({ 35, 110 }, { 66, 66 }, widget_type::wt_3, 1),
            make_widget({ 3, 95 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, string_ids::tooltip_select_track_to_upgrade),
            make_widget({ 123, 96 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::tooltip_select_track_to_upgrade),
            widget_end(),
        };

        static window_event_list events;
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { construction::widgets, widx::tab_construction, &construction::events, construction::enabledWidgets },
            { station::widgets, widx::tab_station, &station::events, station::enabledWidgets },
            { signal::widgets, widx::tab_signal, &signal::events, signal::enabledWidgets },
            { overhead::widgets, widx::tab_overhead, &overhead::events, overhead::enabledWidgets },
        };

        static void prepare_draw(window* self)
        {
            // Reset tab widgets if needed
            const auto& tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->init_scroll_widgets();
            }

            // Activate the current tab
            self->activated_widgets &= ~((1ULL << tab_construction) | (1ULL << tab_overhead) | (1ULL << tab_signal) | (1ULL << tab_station));
            self->activated_widgets |= (1ULL << common::tabInformationByTabOffset[self->current_tab].widgetIndex);
        }

        // 0x0049D93A
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (self->current_tab == widgetIndex - widx::tab_construction)
                return;
            if (widgetIndex == widx::tab_station)
            {
                ui::windows::station::showStationCatchment(-1);
            }
            construction::sub_49FEC7();
            tilemgr::map_invalidate_map_selection_tiles();
            _mapSelectionFlags = _mapSelectionFlags & ~MapSelectFlag::enableConstruct;
            _trackCost = 0x80000000;
            _dword_1135F4E = 0x80000000;
            _dword_1135F6C = 0x80000000;
            _dword_1135F46 = 0x80000000;
            _byte_1136076 = 0;

            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();

            self->current_tab = widgetIndex - widx::tab_construction;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_construction];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            setDisabledWidgets(self);

            self->invalidate();

            self->width = self->widgets[widx::frame].right + 1;
            self->height = self->widgets[widx::frame].bottom + 1;

            self->call_on_resize();
            self->call_prepare_draw();
            self->init_scroll_widgets();
            self->invalidate();

            if (self->current_tab == widx::tab_construction - widx::tab_construction)
            {
                if (_constructionHover != 0)
                {
                    _constructionHover = 0;
                    _byte_113607E = 1;
                    self->call_on_mouse_up(construction::widx::rotate_90);
                }
            }

            if (self->current_tab == widx::tab_signal - widx::tab_construction)
            {
                // TODO: replace with signal tab widx
                self->call_on_mouse_down(0x0A);
            }

            if (self->current_tab == widx::tab_overhead - widx::tab_construction)
            {
                // TODO: replace with overhead tab widx
                self->call_on_mouse_down(0x0C);
            }

            self->moveInsideScreenEdges();
        }

        // 0x0049ED33
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto company = companymgr::get(_playerCompany);
            auto companyColour = company->mainColours.primary;
            if (_trackType & (1 << 7))
            {
                auto roadObj = objectmgr::get<road_object>(_trackType & ~(1 << 7));
                // Construction Tab
                {
                    auto imageId = roadObj->var_0E;
                    if (self->current_tab == widx::tab_construction - widx::tab_construction)
                        imageId += (self->frame_no / 4) % 32;
                    gfx::recolour(imageId, companyColour);

                    widget::draw_tab(self, dpi, imageId, widx::tab_construction);
                }
                // Station Tab
                {
                    widget::draw_tab(self, dpi, image_ids::null, widx::tab_station);
                    if (!self->is_disabled(widx::tab_station))
                    {
                        auto x = self->widgets[widx::tab_station].left + self->x + 1;
                        auto y = self->widgets[widx::tab_station].top + self->y + 1;
                        auto width = 29;
                        auto height = 25;
                        if (self->current_tab == widx::tab_station - widx::tab_construction)
                            height++;

                        gfx::drawpixelinfo_t* clipped = nullptr;

                        if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                        {
                            clipped->zoom_level = 1;
                            clipped->width <<= 1;
                            clipped->height <<= 1;
                            clipped->x <<= 1;
                            clipped->y <<= 1;
                            auto roadStationObj = objectmgr::get<road_station_object>(_lastSelectedStationBuilding);
                            auto imageId = gfx::recolour(roadStationObj->var_0C, companyColour);
                            gfx::draw_image(clipped, -4, -9, imageId);
                            auto colour = _byte_5045FA[companyColour];
                            if (!(roadStationObj->colour_flags & 1))
                            {
                                colour = 46;
                            }
                            imageId = gfx::recolour(imageId, colour) + 1;
                            gfx::draw_image(clipped, -4, -9, imageId);
                        }

                        widget::draw_tab(self, dpi, -2, widx::tab_station);
                    }
                }
                // Overhead tab
                {
                    widget::draw_tab(self, dpi, image_ids::null, widx::tab_overhead);
                    if (!self->is_disabled(widx::tab_station))
                    {
                        auto x = self->widgets[widx::tab_overhead].left + self->x + 2;
                        auto y = self->widgets[widx::tab_overhead].top + self->y + 2;

                        for (auto i = 0; i < 2; i++)
                        {
                            if (_modList[i] != 0xFF)
                            {
                                auto roadExtraObj = objectmgr::get<road_extra_object>(_modList[i]);
                                auto imageId = roadExtraObj->var_0E;
                                if (self->current_tab == widx::tab_overhead - widx::tab_construction)
                                    imageId += (self->frame_no / 2) % 8;
                                gfx::draw_image(dpi, x, y, imageId);
                            }
                        }

                        widget::draw_tab(self, dpi, -2, widx::tab_overhead);
                    }
                }
            }
            else
            {
                auto trackObj = objectmgr::get<track_object>(_trackType);
                // Construction Tab
                {
                    auto imageId = trackObj->var_1E;
                    if (self->current_tab == widx::tab_construction - widx::tab_construction)
                        imageId += (self->frame_no / 4) % 15;
                    gfx::recolour(imageId, companyColour);

                    widget::draw_tab(self, dpi, imageId, widx::tab_construction);
                }
                // Station Tab
                {
                    if (_byte_1136063 & (1 << 7))
                    {
                        auto imageId = objectmgr::get<interface_skin_object>()->img + 46;

                        widget::draw_tab(self, dpi, imageId, widx::tab_station);
                    }
                    else
                    {
                        if (_byte_1136063 & (1 << 6))
                        {
                            auto imageId = objectmgr::get<interface_skin_object>()->img + 47;

                            widget::draw_tab(self, dpi, imageId, widx::tab_station);
                        }
                        else
                        {
                            widget::draw_tab(self, dpi, image_ids::null, widx::tab_station);
                            if (!self->is_disabled(widx::tab_station))
                            {
                                auto x = self->widgets[widx::tab_station].left + self->x + 1;
                                auto y = self->widgets[widx::tab_station].top + self->y + 1;
                                auto width = 29;
                                auto height = 25;
                                if (self->current_tab == widx::tab_station - widx::tab_construction)
                                    height++;

                                gfx::drawpixelinfo_t* clipped = nullptr;

                                if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                                {
                                    clipped->zoom_level = 1;
                                    clipped->width <<= 1;
                                    clipped->height <<= 1;
                                    clipped->x <<= 1;
                                    clipped->y <<= 1;
                                    auto trainStationObj = objectmgr::get<train_station_object>(_lastSelectedStationBuilding);
                                    auto imageId = gfx::recolour(trainStationObj->var_0E, companyColour);
                                    gfx::draw_image(clipped, -4, -9, imageId);
                                    auto colour = _byte_5045FA[companyColour];
                                    if (!(trainStationObj->colour_flags & 1))
                                    {
                                        colour = 46;
                                    }
                                    imageId = gfx::recolour(imageId, colour) + 1;
                                    gfx::draw_image(clipped, -4, -9, imageId);
                                }

                                widget::draw_tab(self, dpi, -2, widx::tab_station);
                            }
                        }
                    }
                }
                // Signal Tab
                {
                    widget::draw_tab(self, dpi, image_ids::null, widx::tab_signal);
                    if (!self->is_disabled(widx::tab_signal))
                    {
                        auto x = self->widgets[widx::tab_signal].left + self->x + 1;
                        auto y = self->widgets[widx::tab_signal].top + self->y + 1;
                        auto width = 29;
                        auto height = 25;
                        if (self->current_tab == widx::tab_station - widx::tab_construction)
                            height++;

                        gfx::drawpixelinfo_t* clipped = nullptr;

                        if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                        {
                            auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);
                            auto imageId = trainSignalObject->var_0E;
                            if (self->current_tab == widx::tab_signal - widx::tab_construction)
                            {
                                auto eax = _dword_4FFAF0[trainSignalObject->var_05];
                                auto si = self->current_tab >> trainSignalObject->var_04;
                                eax &= si;
                                eax = _dword_4FFAF0[eax + 1];
                                eax >>= 3;
                                imageId += eax;
                            }
                            gfx::draw_image(clipped, 15, 31, imageId);
                        }

                        widget::draw_tab(self, dpi, -2, widx::tab_signal);
                    }
                }
                // Overhead Tab
                {
                    widget::draw_tab(self, dpi, image_ids::null, widx::tab_overhead);
                    if (!self->is_disabled(widx::tab_station))
                    {
                        auto x = self->widgets[widx::tab_overhead].left + self->x + 2;
                        auto y = self->widgets[widx::tab_overhead].top + self->y + 2;

                        for (auto i = 0; i < 4; i++)
                        {
                            if (_modList[i] != 0xFF)
                            {
                                auto trackExtraObj = objectmgr::get<track_extra_object>(_modList[i]);
                                auto imageId = trackExtraObj->var_0E;
                                if (self->current_tab == widx::tab_overhead - widx::tab_construction)
                                    imageId += (self->frame_no / 2) % 8;
                                gfx::draw_image(dpi, x, y, imageId);
                            }
                        }

                        widget::draw_tab(self, dpi, -2, widx::tab_overhead);
                    }
                }
            }
        }

        // 0x004A09DE
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_construction].left;
            const int16_t tabWidth = self->widgets[widx::tab_construction].right - xPos;

            for (uint8_t i = widx::tab_construction; i <= widx::tab_overhead; i++)
            {
                if (self->is_disabled(i))
                {
                    self->widgets[i].type = widget_type::none;
                    continue;
                }

                self->widgets[i].type = widget_type::wt_8;
                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        static void init_events()
        {
            construction::init_events();
        }
    }
}
