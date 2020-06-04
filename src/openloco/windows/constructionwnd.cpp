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
#include "../objects/cargo_object.h"
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
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
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

    static loco_global<uint8_t[16], 0x004FFB08> _signalFrames2State;
    static loco_global<uint8_t[32], 0x004FFB19> _signalFrames3State;
    static loco_global<uint8_t[64], 0x004FFB3A> _signalFrames4State;

    static loco_global<uint8_t[31], 0x005045FA> _byte_5045FA;
    static loco_global<uint8_t, 0x00522090> _byte_522090;
    static loco_global<uint8_t, 0x00522091> _byte_522091;
    static loco_global<uint8_t, 0x00522092> _byte_522092;
    static loco_global<uint8_t, 0x00522095> _byte_522095;
    static loco_global<uint8_t, 0x00522096> _byte_522096;
    static loco_global<uint16_t, 0x0052338A> _tooltipTimeout;
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;
    static loco_global<ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<uint32_t, 0x00523394> _toolWidgetIndex;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;
    static loco_global<uint8_t[8], 0x0525F72> _scenarioSignals;
    static loco_global<uint8_t[8], 0x0525F7A> _scenarioBridges;
    static loco_global<uint8_t[8], 0x0525F82> _scenarioTrainStations;
    static loco_global<uint8_t[8], 0x0525F8A> _scenarioTrackMods;
    static loco_global<uint8_t[8], 0x0525F9A> _scenarioRoadStations;
    static loco_global<uint8_t[8], 0x0525FA2> _scenarioRoadMods;
    static loco_global<uint8_t, 0x00525FAA> _lastRailroadOption;
    static loco_global<uint8_t, 0x00525FAB> _lastRoadOption;
    static loco_global<uint8_t, 0x00525FAC> _lastAirport;
    static loco_global<uint8_t, 0x00525FAD> _lastShipPort;
    static loco_global<uint8_t, 0x00525FAE> _byte_525FAE;
    static loco_global<company_id_t, 0x009C68EB> _updatingCompanyId;
    static loco_global<uint32_t, 0x00E0C3E0> _dword_E0C3E0;
    static loco_global<tile_element* [0x30004], 0x00E40134> _tiles;
    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<map_pos[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;
    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> _stringFormatBuffer;
    static loco_global<uint32_t, 0x01135F3E> _trackCost;
    static loco_global<uint32_t, 0x01135F4E> _signalCost;
    static loco_global<uint32_t, 0x01135F6C> _stationCost;
    static loco_global<uint32_t, 0x01135F70> _dword_1135F70;
    static loco_global<uint32_t, 0x01135F74> _dword_1135F74;
    static loco_global<uint32_t, 0x01135F78> _dword_1135F78;
    static loco_global<uint32_t, 0x01135F46> _dword_1135F46;
    static loco_global<uint16_t, 0x01135F86> _word_1135F86;
    static loco_global<uint16_t, 0x01135FB4> _x;
    static loco_global<uint16_t, 0x01135FB6> _y;
    static loco_global<uint16_t, 0x01135FB8> _word_1135FB8;
    static loco_global<uint16_t, 0x01135FD6> _word_1135FD6;
    static loco_global<uint16_t, 0x01135FD8> _word_1135FD8;
    static loco_global<uint16_t, 0x01135FE4> _lastSelectedMod;
    static loco_global<uint16_t, 0x01136000> _word_1136000;
    static loco_global<uint8_t[17], 0x0113601D> _signalList;
    static loco_global<uint8_t, 0x0113602E> _lastSelectedSignal;
    static loco_global<uint8_t, 0x0113602F> _isSignalBothDirections;
    static loco_global<uint8_t[9], 0x01136030> _bridgeList;
    static loco_global<uint8_t, 0x01136039> _lastSelectedBridge;
    static loco_global<uint8_t, 0x0113603A> _byte_113603A;
    static loco_global<uint8_t[17], 0x0113603B> _stationList;
    static loco_global<uint8_t, 0x0113604C> _lastSelectedStationType;
    static loco_global<uint8_t[4], 0x01136054> _modList;
    static loco_global<uint8_t, 0x01136061> _constructionHover;
    static loco_global<uint8_t, 0x01136062> _trackType;
    static loco_global<uint8_t, 0x01136063> _byte_1136063;
    static loco_global<uint8_t, 0x01136064> _constructionRotation;
    static loco_global<uint8_t, 0x01136065> _byte_1136065;
    static loco_global<uint8_t, 0x01136067> _lastSelectedTrackPiece;
    static loco_global<uint8_t, 0x01136068> _lastSelectedTrackGradient;
    static loco_global<uint8_t, 0x0113606E> _byte_113606E;
    static loco_global<uint8_t, 0x01136076> _byte_1136076;
    static loco_global<uint8_t, 0x01136077> _byte_1136077;
    static loco_global<uint8_t, 0x01136078> _byte_1136078;
    static loco_global<uint8_t, 0x01136079> _lastSelectedTrackPieceId;
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

        enum trackPiece
        {
            straight,
            left_hand_curve_very_small,
            right_hand_curve_very_small,
            left_hand_curve_small,
            right_hand_curve_small,
            left_hand_curve,
            right_hand_curve,
            left_hand_curve_large,
            right_hand_curve_large,
            s_bend_left,
            s_bend_right,
            s_bend_to_dual_track,
            s_bend_to_single_track,
            turnaround,
        };

        enum trackGradient
        {
            level = 0,
            slope_up = 2,
            steep_slope_up = 4,
            slope_down = 6,
            steep_slope_down = 8,
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
        static bool getRoadPieceId(registers& regs);
        static bool getTrackPieceId(registers& regs);
        static void sub_49F1B5();
        static void sub_49FEC7();
        static void on_close(window* self);
        static void on_update(window* self, uint8_t flag);

        static const uint8_t* roadPieces[] = {
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

        static const uint8_t* trackPieces[] = {
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
        constexpr uint64_t allTrack = {
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
            (1ULL << widx::steep_slope_up)
            };

        constexpr uint64_t allConstruction = {
            allTrack |
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
                        common::sub_49F1B5();
                        break;
                    }
                    common::sub_49FEC7();
                    WindowManager::viewportSetVisibility(3);
                    input::toolSet(self, widx::construct, 12);
                    input::set_flag(input::input_flags::flag6);
                    _constructionHover = 1;
                    _byte_113607E = 0;
                    _constructionRotation = _constructionRotation & 3;
                    common::sub_49F1B5();
                    break;
                }
            }
        }

        static void sub_49DBEC(window* self, uint64_t disabledWidgets)
        {
            if (_lastSelectedTrackGradient == 2 || _lastSelectedTrackGradient == 6)
            {
                disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
                disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);
                disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
            }

            if (_lastSelectedTrackGradient == 4 || _lastSelectedTrackGradient == 8)
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
                auto carryFlag = common::getRoadPieceId(regs);
                if (carryFlag)
                    disabledWidgets |= (1 << widx::construct);
            }
            self->set_disabled_widgets_and_invalidate(disabledWidgets);
        }

        static void sub_49DB1F(window* self, track_object trackObj, uint64_t disabledWidgets)
        {
            if (_lastSelectedTrackGradient == 2 || _lastSelectedTrackGradient == 6)
            {
                disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
                disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);

                if (!(trackObj.track_pieces & (1 << 8)))
                    disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
            }

            if (_lastSelectedTrackGradient == 4 || _lastSelectedTrackGradient == 8)
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
                auto invalidTrack = common::getTrackPieceId(regs);

                if (invalidTrack)
                    disabledWidgets |= (1 << widx::construct);
            }
            self->set_disabled_widgets_and_invalidate(disabledWidgets);
        }

        static void sub_49DAF3(window* self, track_object trackObj, uint64_t disabledWidgets)
        {
            auto trackPieces = trackObj.track_pieces & ((1 << 5) | (1 << 8));

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
                if (_lastSelectedTrackPiece == 0xFF)
                {
                    sub_49DBEC(self, disabledWidgets);
                    return;
                }
                switch (_lastSelectedTrackPiece)
                {
                    case common::trackPiece::straight:
                    case common::trackPiece::left_hand_curve:
                    case common::trackPiece::right_hand_curve:
                    case common::trackPiece::left_hand_curve_large:
                    case common::trackPiece::right_hand_curve_large:
                    case common::trackPiece::s_bend_left:
                    case common::trackPiece::s_bend_right:
                    case common::trackPiece::s_bend_to_dual_track:
                    case common::trackPiece::s_bend_to_single_track:
                    {
                        sub_49DBEC(self, disabledWidgets);
                        break;
                    }

                    case common::trackPiece::left_hand_curve_very_small:
                    case common::trackPiece::right_hand_curve_very_small:
                    case common::trackPiece::left_hand_curve_small:
                    case common::trackPiece::right_hand_curve_small:
                    case common::trackPiece::turnaround:
                    {
                        disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
                        break;
                    }
                }
            }
            else
            {
                auto trackObj = objectmgr::get<track_object>(trackType);
                if (_lastSelectedTrackPiece == 0xFF)
                {
                    sub_49DB1F(self, *trackObj, disabledWidgets);
                    return;
                }
                switch (_lastSelectedTrackPiece)
                {
                    case common::trackPiece::straight:
                        sub_49DB1F(self, *trackObj, disabledWidgets);
                        break;

                    case common::trackPiece::left_hand_curve_very_small:
                    case common::trackPiece::right_hand_curve_very_small:
                    case common::trackPiece::left_hand_curve:
                    case common::trackPiece::right_hand_curve:
                    case common::trackPiece::left_hand_curve_large:
                    case common::trackPiece::right_hand_curve_large:
                    case common::trackPiece::s_bend_left:
                    case common::trackPiece::s_bend_right:
                    case common::trackPiece::s_bend_to_dual_track:
                    case common::trackPiece::s_bend_to_single_track:
                    case common::trackPiece::turnaround:
                    {
                        disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
                        sub_49DB1F(self, *trackObj, disabledWidgets);
                        break;
                    }

                    case common::trackPiece::left_hand_curve_small:
                    case common::trackPiece::right_hand_curve_small:
                    {
                        sub_49DAF3(self, *trackObj, disabledWidgets);
                        break;
                    }
                }
            }
        }

        static void changeTrackPiece(uint8_t trackPiece, bool slope)
        {
            _byte_113603A = 0xFF;
            common::sub_49FEC7();
            if (slope)
                _lastSelectedTrackGradient = trackPiece;
            else
                _lastSelectedTrackPiece = trackPiece;
            _trackCost = 0x80000000;
            common::sub_49F1B5();
        }

        static void bridgeDropdown(window* self)
        {
            auto bridgeCount = 0;
            for (; bridgeCount < 9; bridgeCount++)
            {
                if (_bridgeList[bridgeCount] == 0xFF)
                    break;
            }

            uint8_t flags = (1 << 7) | (1 << 6);
            auto widget = self->widgets[widx::bridge];
            auto x = widget.left + self->x;
            auto y = widget.top + self->y;
            auto width = 155;
            auto height = widget.height();

            dropdown::show(x, y, width, height, self->colours[1], bridgeCount, 22, flags);
            for (auto i = 0; i < 9; i++)
            {
                auto bridge = _bridgeList[i];

                if (bridge == 0xFF)
                    return;

                if (bridge == _lastSelectedBridge)
                    dropdown::set_highlighted_item(i);

                auto bridgeObj = objectmgr::get<bridge_object>(bridge);
                auto company = companymgr::get(_playerCompany);
                auto companyColour = company->mainColours.primary;
                auto imageId = gfx::recolour(bridgeObj->var_16, companyColour);
                auto args = FormatArguments();
                args.push(imageId);
                if (bridgeObj->max_speed == 0xFFFF)
                {
                    args.push(string_ids::unlimited_speed);
                    args.push<uint16_t>(0);
                }
                else
                {
                    args.push(string_ids::velocity);
                    args.push(bridgeObj->max_speed);
                }
                args.push<uint16_t>(bridgeObj->max_height);

                dropdown::add(i, string_ids::dropdown_bridge_stats, args);
            }
        }

        // 0x0049D42F
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::left_hand_curve:
                    changeTrackPiece(common::trackPiece::left_hand_curve, false);
                    break;

                case widx::right_hand_curve:
                    changeTrackPiece(common::trackPiece::right_hand_curve, false);
                    break;

                case widx::left_hand_curve_small:
                    changeTrackPiece(common::trackPiece::left_hand_curve_small, false);
                    break;

                case widx::right_hand_curve_small:
                    changeTrackPiece(common::trackPiece::right_hand_curve_small, false);
                    break;

                case widx::left_hand_curve_very_small:
                    changeTrackPiece(common::trackPiece::left_hand_curve_very_small, false);
                    break;

                case widx::right_hand_curve_very_small:
                    changeTrackPiece(common::trackPiece::right_hand_curve_very_small, false);
                    break;

                case widx::left_hand_curve_large:
                    changeTrackPiece(common::trackPiece::left_hand_curve_large, false);
                    break;

                case widx::right_hand_curve_large:
                    changeTrackPiece(common::trackPiece::right_hand_curve_large, false);
                    break;

                case widx::straight:
                    changeTrackPiece(common::trackPiece::straight, false);
                    break;

                case widx::s_bend_left:
                    changeTrackPiece(common::trackPiece::s_bend_left, false);
                    break;

                case widx::s_bend_right:
                    changeTrackPiece(common::trackPiece::s_bend_right, false);
                    break;

                case widx::s_bend_dual_track_left:
                {
                    _byte_113603A = 0xFF;
                    common::sub_49FEC7();
                    _lastSelectedTrackPiece = common::trackPiece::s_bend_to_dual_track;
                    _trackCost = 0x80000000;
                    if (self->widgets[widx::s_bend_dual_track_left].image != image_ids::construction_s_bend_dual_track_left)
                    {
                        _lastSelectedTrackPiece = common::trackPiece::turnaround;
                        if (self->widgets[widx::s_bend_dual_track_left].image != image_ids::construction_right_turnaround)
                        {
                            if (self->widgets[widx::s_bend_dual_track_left].image != image_ids::construction_left_turnaround)
                                _lastSelectedTrackPiece = common::trackPiece::s_bend_to_single_track;
                        }
                    }
                    common::sub_49F1B5();
                    break;
                }

                case widx::s_bend_dual_track_right:
                {
                    _byte_113603A = 0xFF;
                    common::sub_49FEC7();
                    _lastSelectedTrackPiece = common::trackPiece::s_bend_to_single_track;
                    _trackCost = 0x80000000;
                    if (self->widgets[widx::s_bend_dual_track_right].image != image_ids::construction_s_bend_dual_track_right)
                    {
                        _lastSelectedTrackPiece = common::trackPiece::turnaround;
                        if (self->widgets[widx::s_bend_dual_track_left].image != image_ids::construction_left_turnaround)
                            _lastSelectedTrackPiece = common::trackPiece::s_bend_to_dual_track;
                    }
                    common::sub_49F1B5();
                    break;
                }

                case widx::steep_slope_down:
                    changeTrackPiece(common::trackGradient::steep_slope_down, true);
                    break;

                case widx::slope_down:
                    changeTrackPiece(common::trackGradient::slope_down, true);
                    break;

                case widx::level:
                    changeTrackPiece(common::trackGradient::level, true);
                    break;

                case widx::slope_up:
                    changeTrackPiece(common::trackGradient::slope_up, true);
                    break;

                case widx::steep_slope_up:
                    changeTrackPiece(common::trackGradient::steep_slope_up, true);
                    break;

                case widx::bridge_dropdown:
                {
                    bridgeDropdown(self);
                    break;
                }
            }
            std::printf("WidgetIndex: %d\n", int(widgetIndex));
            std::printf("TrackPiece: %d\n", int(_lastSelectedTrackPiece));
            std::printf("TrackGradient: %d\n", int(_lastSelectedTrackGradient));
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
                    _scenarioBridges[_trackType] = bridge;
                    common::sub_49FEC7();
                    _trackCost = 0x80000000;
                    common::sub_49F1B5();
                }
            }
        }

        static void sub_49FD66()
        {
            registers regs;
            call(0x0049FD66, regs);
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
            sub_49FD66();
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
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049DC97, regs);
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
                    if (bridgeObj->max_speed == 0xFFFF)
                    {
                        args.push(string_ids::unlimited_speed);
                        args.push<uint16_t>(0);
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

        // 0x004A0AE5
        static void drawTrack(uint16_t ax, uint16_t cx, uint32_t edi, uint8_t bh, uint32_t edx)
        {
            registers regs;
            regs.ax = ax;
            regs.cx = cx;
            regs.edi = edi;
            regs.bh = bh;
            regs.edx = edx;
            call(0x004A0AE5, regs);
        }

        // 0x00478F1F
        static void drawRoad(uint16_t ax, uint16_t cx, uint32_t edi, uint8_t bh, uint32_t edx)
        {
            registers regs;
            regs.ax = ax;
            regs.cx = cx;
            regs.edi = edi;
            regs.bh = bh;
            regs.edx = edx;
            call(0x00478F1F, regs);
        }

        // 0x0049D38A and 0x0049D16B
        static void drawCostString(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto x = self->widgets[widx::construct].width() + 1;
            x >>= 1;
            x += self->x;
            auto y = self->widgets[widx::construct].bottom + self->y - 23;

            if (_constructionHover != 1)
                gfx::draw_string_centred(*dpi, x, y, colour::black, string_ids::build_this);

            y += 11;

            if (_trackCost != 0x80000000)
            {
                if (_trackCost != 0)
                {
                    auto args = FormatArguments();
                    args.push<uint32_t>(_trackCost);
                    gfx::draw_string_centred(*dpi, x, y, colour::black, string_ids::build_cost, &args);
                }
            }
        }

        static void loc_49D106(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, uint16_t bp, uint16_t si)
        {
            bp >>= 1;
            si >>= 1;
            si += 16;
            ax -= bp;
            cx -= si;
            clipped->x += ax;
            clipped->y += cx;
            _dword_E0C3E0 = (uint32_t)clipped;
            uint16_t x = 0x2000;
            uint16_t y = 0x2000;
            uint32_t edi = _word_1135FD8 << 16 | 0x1E0;
            uint32_t edx = _word_1135FD6 << 16 | _lastSelectedTrackPieceId << 8 | _byte_1136077;
            _byte_522095 = _byte_522095 | (1 << 1);

            std::printf("trackPieceId: %d\n", int(_lastSelectedTrackPieceId));

            drawTrack(x, y, edi, _byte_1136078, edx);

            _byte_522095 = _byte_522095 & ~(1 << 1);

            drawCostString(self, dpi);
        }

        static void loc_49D325(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, uint16_t bp, uint16_t si)
        {
            bp >>= 1;
            si >>= 1;
            si += 16;
            ax -= bp;
            cx -= si;
            clipped->x += ax;
            clipped->y += cx;
            _dword_E0C3E0 = (uint32_t)clipped;
            uint16_t x = 0x2000;
            uint16_t y = 0x2000;
            uint32_t edi = _word_1135FD8 << 16 | 0x1E0;
            uint32_t edx = _word_1135FD6 << 16 | _lastSelectedTrackPieceId << 8 | _byte_1136077;
            _byte_522095 = _byte_522095 | (1 << 1);

            drawRoad(x, y, edi, _byte_1136078, edx);

            _byte_522095 = _byte_522095 & ~(1 << 1);

            drawCostString(self, dpi);
        }

        // 0x0049D0B8
        static void trackRotation0(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax = -ax + cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D106(self, clipped, dpi, ax, cx, bp, si);
        }

        // 0x0049D0CC
        static void trackRotation1(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            ax = -ax;
            auto bx = ax;
            ax -= cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D106(self, clipped, dpi, ax, cx, bp, si);
        }

        // 0x0049D0E0
        static void trackRotation2(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax -= cx;
            cx = -cx;
            cx -= bx;
            cx >>= 1;
            cx -= dx;
            loc_49D106(self, clipped, dpi, ax, cx, bp, si);
        }

        // 0x0049D0F4
        static void trackRotation3(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax += cx;
            cx = -cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D106(self, clipped, dpi, ax, cx, bp, si);
        }

        // 0x0049D2D7
        static void roadRotation0(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax = -ax + cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D325(self, clipped, dpi, ax, cx, bp, si);
        }

        // 0x0049D2EB
        static void roadRotation1(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            ax = -ax;
            auto bx = ax;
            ax -= cx;
            cx += bx;
            cx >>= 1;
            cx -= dx;
            loc_49D325(self, clipped, dpi, ax, cx, bp, si);
        }

        // 0x0049D2FF
        static void roadRotation2(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
        {
            auto bx = ax;
            ax -= cx;
            cx = -cx;
            cx -= bx;
            cx >>= 1;
            cx -= dx;
            loc_49D325(self, clipped, dpi, ax, cx, bp, si);
        }

        // 0x0049D313
        static void roadRotation3(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, int16_t ax, int16_t cx, int16_t dx, uint16_t bp, uint16_t si)
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
                auto carryFlag = common::getRoadPieceId(regs);

                _word_1135FD8 = regs.edi >> 16;

                if (carryFlag)
                    return;
                _byte_1136077 = regs.dl;
                _byte_1136078 = regs.bh;
                _lastSelectedTrackPieceId = regs.dh;
                _word_1135FD6 = (regs.edx >> 16) & 0x1F;

                auto x = self->x + self->widgets[widx::construct].left + 1;
                auto y = self->y + self->widgets[widx::construct].top + 1;
                auto width = self->widgets[widx::construct].width();
                auto height = self->widgets[widx::construct].width();

                gfx::drawpixelinfo_t* clipped = nullptr;

                if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                {
                    auto ecx = common::roadPieces[_lastSelectedTrackPieceId];
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
                            roadRotation0(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 1:
                            roadRotation1(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 2:
                            roadRotation2(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 3:
                            roadRotation3(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                    }
                }
                else
                {
                    drawCostString(self, dpi);
                }
            }
            else
            {
                registers regs;
                auto invalidTrack = common::getTrackPieceId(regs);

                _word_1135FD8 = regs.edi >> 16;

                if (invalidTrack)
                    return;

                _byte_1136077 = regs.dl;
                _byte_1136078 = regs.bh;
                _lastSelectedTrackPieceId = regs.dh;
                _word_1135FD6 = (regs.edx >> 16) & 0x1F;

                auto x = self->x + self->widgets[widx::construct].left + 1;
                auto y = self->y + self->widgets[widx::construct].top + 1;
                auto width = self->widgets[widx::construct].width();
                auto height = self->widgets[widx::construct].width();

                gfx::drawpixelinfo_t* clipped = nullptr;

                if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                {
                    auto ecx = common::trackPieces[_lastSelectedTrackPieceId];
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
                            trackRotation0(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 1:
                            trackRotation1(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 2:
                            trackRotation2(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                        case 3:
                            trackRotation3(self, clipped, dpi, ax, cx, dx, width, height);
                            break;
                    }
                }
                else
                {
                    drawCostString(self, dpi);
                }
            }
        }

        static void init_events()
        {
            events.on_close = common::on_close;
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

        if (_modList[0] == 0xFF && _modList[1] == 0xFF && _modList[2] == 0xFF && _modList[3] == 0xFF)
            disabledWidgets |= (1ULL << common::widx::tab_overhead);

        if (_lastSelectedStationType == 0xFF)
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

        common::init_events();
    }

    // 0x004723BD
    static void sortList(uint8_t* list, size_t size)
    {
        size_t count = 0;
        while (list[count] != 0xFF)
        {
            count++;
        }
        while (count > 1)
        {
            size_t i = 1;
            for (; i < count; i++)
            {
                uint8_t ah = list[i];
                uint8_t al = list[i - 1];
                if (al > ah)
                {
                    list[i - 1] = ah;
                    list[i] = al;
                }
            }
            count--;
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

        sortList(_stationList, std::size(_stationList));
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

        sortList(_stationList, std::size(_stationList));
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
            _stationList[roadStationCount] = station;
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
                    if (stationListCopy == _stationList)
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

        sortList(_stationList, std::size(_stationList));
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
            _bridgeList[bridgeCount] = bridge;
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
                    if (bridgeListCopy == _bridgeList)
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

        sortList(_bridgeList, std::size(_bridgeList));
    }

    // 0x004781C5
    static void refreshRoadModList()
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
        common::sub_49FEC7();
        setTrackOptions(_trackType);
        refreshRoadStationList();

        auto al = _scenarioRoadStations[(_trackType & ~(1ULL << 7))];
        if (al == 0xFF)
            al = _stationList[0];
        _lastSelectedStationType = al;

        refreshRoadBridgeList();

        al = _scenarioBridges[(_trackType & ~(1ULL << 7))];
        if (al == 0xFF)
            al = _bridgeList[0];
        _lastSelectedBridge = al;

        refreshRoadModList();

        al = _scenarioRoadMods[(_trackType & ~(1ULL << 7))];
        if (al == 0xFF)
            al = 0;
        _lastSelectedMod = al;

        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            setDisabledWidgets(window);
        }
        common::sub_49F1B5();
    }

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

        sortList(_signalList, std::size(_signalList));
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
            _stationList[trainStationCount] = station;
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
                    if (stationListCopy == _stationList)
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

        sortList(_stationList, std::size(_stationList));
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
            _bridgeList[bridgeCount] = bridge;
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
                    if (bridgeListCopy == _bridgeList)
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

        sortList(_bridgeList, std::size(_bridgeList));
    }

    // 0x004A693D
    static void refreshTrackModList()
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

    static window* nonTrackWindow()
    {
        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            setDisabledWidgets(window);
        }

        window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            window->call_on_mouse_up(common::widx::tab_station);
        }
        return window;
    }

    static window* trackWindow()
    {
        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            setDisabledWidgets(window);
        }

        common::sub_49F1B5();
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

        if (window != nullptr)
        {
            if (flags & (1 << 7))
            {
                auto trackType = flags & ~(1 << 7);
                auto roadObj = objectmgr::get<road_object>(trackType);

                if (roadObj->flags & 8)
                {
                    if (_trackType & (1 << 7))
                    {
                        trackType = _trackType & ~(1 << 7);
                        roadObj = objectmgr::get<road_object>(trackType);

                        if (roadObj->flags & 8)
                        {
                            _trackType = flags;

                            sub_4A3A50();

                            _lastSelectedTrackPiece = 0;
                            _lastSelectedTrackGradient = 0;

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
            viewport->flags = _word_1135F86;
        }

        _trackType = flags;
        _byte_1136063 = flags >> 24;
        _x = 0x1800;
        _y = 0x1800;
        _word_1135FB8 = 0x100;
        _constructionRotation = 0;
        _constructionHover = 0;
        _byte_113607E = 1;
        _trackCost = 0x80000000;
        _byte_1136076 = 0;
        _lastSelectedTrackPiece = 0;
        _lastSelectedTrackGradient = 0;
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
            _lastSelectedMod = 0;
            _lastSelectedBridge = 0xFF;

            refreshAirportList();

            auto al = _lastAirport;

            if (al == 0xFF)
            {
                al = _stationList[0];
            }

            _lastSelectedStationType = al;

            return nonTrackWindow();
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

                _lastSelectedMod = 0;
                _lastSelectedBridge = 0xFF;

                refreshDockList();

                auto al = _lastShipPort;

                if (al == 0xFF)
                {
                    al = _stationList[0];
                }

                _lastSelectedStationType = al;

                return nonTrackWindow();
            }
            else
            {
                if (flags & (1 << 7))
                {
                    createConstructionWindow();

                    _lastSelectedSignal = 0xFF;

                    refreshRoadStationList();

                    auto al = _scenarioRoadStations[(_trackType & ~(1ULL << 7))];

                    if (al == 0xFF)
                        al = _stationList[0];

                    _lastSelectedStationType = al;

                    refreshRoadBridgeList();

                    al = _scenarioBridges[(_trackType & ~(1ULL << 7))];

                    if (al == 0xFF)
                        al = _bridgeList[0];

                    _lastSelectedBridge = al;

                    refreshRoadModList();

                    al = _scenarioRoadMods[(_trackType & ~(1ULL << 7))];

                    if (al == 0xff)
                        al = 0;

                    _lastSelectedMod = al;
                    _byte_113603A = 0;

                    return trackWindow();
                }
            }
        }
        createConstructionWindow();

        refreshSignalList();

        auto al = _scenarioSignals[_trackType];

        if (al == 0xFF)
            al = _signalList[0];

        _lastSelectedSignal = al;

        refreshtrainStationList();

        al = _scenarioTrainStations[_trackType];

        if (al == 0xFF)
            al = _stationList[0];

        _lastSelectedStationType = al;

        refreshTrackBridgeList();

        al = _scenarioBridges[_trackType];

        if (al == 0xFF)
            al = _bridgeList[0];

        _lastSelectedBridge = al;

        refreshTrackModList();

        al = _scenarioTrackMods[_trackType];

        if (al == 0xFF)
            al = 0;

        _lastSelectedMod = al;
        _byte_113603A = 0;

        return trackWindow();
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

        // 0x0049E228
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
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

                case widx::rotate:
                    _constructionRotation++;
                    _constructionRotation = _constructionRotation & 3;
                    _stationCost = 0x80000000;
                    self->invalidate();
                    break;
            }
        }

        // 0x0049E249
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //regs.dx = widgetIndex;
            //call(0x0049E249, regs);

            switch (widgetIndex)
            {
                case widx::station_dropdown:
                {
                    uint8_t stationCount = 0;
                    while (_stationList[stationCount] != 0xFF)
                        stationCount++;

                    auto widget = self->widgets[widx::station];
                    auto xPos = widget.left + self->x;
                    auto yPos = widget.top + self->y;
                    auto width = widget.width() + 2;
                    auto height = widget.height();
                    dropdown::show(xPos, yPos, width, height, self->colours[1], stationCount, 0x80);

                    if (_byte_1136063 & (1 << 7))
                    {
                        stationCount = 0;
                        while (_stationList[stationCount] != 0xFF)
                        {
                            auto station = _stationList[stationCount];
                            if (station == _lastSelectedStationType)
                                dropdown::set_highlighted_item(stationCount);

                            auto airportObj = objectmgr::get<airport_object>(station);

                            dropdown::add(stationCount, airportObj->name);

                            stationCount++;
                        }
                    }
                    else if (_byte_1136063 & (1 << 6))
                    {
                        stationCount = 0;
                        while (_stationList[stationCount] != 0xFF)
                        {
                            auto station = _stationList[stationCount];
                            if (station == _lastSelectedStationType)
                                dropdown::set_highlighted_item(stationCount);

                            auto dockObj = objectmgr::get<dock_object>(station);

                            dropdown::add(stationCount, dockObj->name);

                            stationCount++;
                        }
                    }
                    else if (_trackType & (1 << 7))
                    {
                        stationCount = 0;
                        while (_stationList[stationCount] != 0xFF)
                        {
                            auto station = _stationList[stationCount];
                            if (station == _lastSelectedStationType)
                                dropdown::set_highlighted_item(stationCount);

                            auto roadStationObj = objectmgr::get<road_station_object>(station);

                            dropdown::add(stationCount, roadStationObj->name);

                            stationCount++;
                        }
                    }
                    else
                    {
                        stationCount = 0;
                        while (_stationList[stationCount] != 0xFF)
                        {
                            auto station = _stationList[stationCount];
                            if (station == _lastSelectedStationType)
                                dropdown::set_highlighted_item(stationCount);

                            auto trainStationObj = objectmgr::get<train_station_object>(station);

                            dropdown::add(stationCount, trainStationObj->name);

                            stationCount++;
                        }
                    }
                    break;
                }
                case widx::image:
                {
                    input::cancel_tool();
                    input::toolSet(self, widgetIndex, 44);
                    break;
                }
            }
        }

        // 0x0049E256
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            //registers regs;
            //regs.esi = (int32_t)&self;
            //regs.dx = widgetIndex;
            //regs.ax = itemIndex;
            //call(0x0049E256, regs);

            if (widgetIndex == widx::station_dropdown)
            {
                if (itemIndex == -1)
                    return;

                auto selectedStation = _stationList[itemIndex];
                _lastSelectedStationType = selectedStation;

                if (_byte_1136063 & (1 << 7))
                {
                    _lastAirport = selectedStation;
                }
                else if (_byte_1136063 & (1 << 6))
                {
                    _lastShipPort = selectedStation;
                }
                else if (_trackType & (1 << 7))
                {
                    auto trackType = _trackType & ~(1 << 7);
                    _scenarioRoadStations[trackType] = selectedStation;
                }
                else
                {
                    _scenarioTrainStations[_trackType] = selectedStation;
                }

                self->invalidate();
            }
        }

        // 0x0049E437
        static void on_update(window* self)
        {
            common::on_update(self, 8);
        }

        // 0x0049E421
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049E421, regs);
        }

        // 0x0049E42C
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049E42C, regs);
        }

        // 0x0049DD39
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            self->widgets[widx::rotate].type = widget_type::none;

            auto args = FormatArguments();

            if (_byte_1136063 & (1 << 7))
            {
                self->widgets[widx::rotate].type = widget_type::wt_9;

                auto airportObj = objectmgr::get<airport_object>(_lastSelectedStationType);

                self->widgets[widx::station].text = airportObj->name;

                args.push(string_ids::title_airport);
            }
            else if (_byte_1136063 & (1 << 6))
            {
                auto dockObj = objectmgr::get<dock_object>(_lastSelectedStationType);

                self->widgets[widx::station].text = dockObj->name;

                args.push(string_ids::title_ship_port);
            }
            else if (_trackType & (1 << 7))
            {
                auto trackType = _trackType & ~(1 << 7);

                auto roadObj = objectmgr::get<road_object>(trackType);

                args.push(roadObj->name);

                auto roadStationObject = objectmgr::get<road_station_object>(_lastSelectedStationType);

                self->widgets[widx::station].text = roadStationObject->name;
            }
            else
            {
                auto trackObj = objectmgr::get<track_object>(_trackType);

                args.push(trackObj->name);

                auto trainStationObject = objectmgr::get<train_station_object>(_lastSelectedStationType);

                self->widgets[widx::station].text = trainStationObject->name;
            }

            common::repositionTabs(self);
        }

        // 0x0049DE40
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //regs.edi = (int32_t)dpi;
            //call(0x0049DE40, regs);

            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto company = companymgr::get(_playerCompany);
            auto companyColour = company->mainColours.primary;
            int16_t xPos = self->widgets[widx::image].left + self->x;
            int16_t yPos = self->widgets[widx::image].top + self->y;

            if (_byte_1136063 & (1 << 7))
            {
                auto airportObj = objectmgr::get<airport_object>(_lastSelectedStationType);

                auto imageId = gfx::recolour(airportObj->var_08, companyColour);

                gfx::draw_image(dpi, xPos, yPos, imageId);
            }
            else if (_byte_1136063 & (1 << 6))
            {
                auto dockObj = objectmgr::get<dock_object>(_lastSelectedStationType);

                auto imageId = gfx::recolour(dockObj->var_08, companyColour);

                gfx::draw_image(dpi, xPos, yPos, imageId);
            }
            else if (_trackType & (1 << 7))
            {
                auto roadStationObj = objectmgr::get<road_station_object>(_lastSelectedStationType);

                auto imageId = gfx::recolour(roadStationObj->var_0C, companyColour);

                gfx::draw_image(dpi, xPos, yPos, imageId);

                auto colour = _byte_5045FA[companyColour];

                if (!(roadStationObj->flags & road_station_flags::recolourable))
                {
                    colour = 46;
                }

                imageId = gfx::recolour(imageId, colour) + 1;

                gfx::draw_image(dpi, xPos, yPos, imageId);
            }
            else
            {
                auto trainStationObj = objectmgr::get<train_station_object>(_lastSelectedStationType);

                auto imageId = gfx::recolour(trainStationObj->var_0E, companyColour);

                gfx::draw_image(dpi, xPos, yPos, imageId);

                auto colour = _byte_5045FA[companyColour];

                if (!(trainStationObj->flags & train_station_flags::recolourable))
                {
                    colour = 46;
                }

                imageId = gfx::recolour(imageId, colour) + 1;

                gfx::draw_image(dpi, xPos, yPos, imageId);
            }

            if (_stationCost != 0x80000000 && _stationCost != 0)
            {
                xPos = self->x + 69;
                yPos = self->widgets[widx::image].bottom + self->y + 4;

                auto args = FormatArguments();
                args.push<uint32_t>(_stationCost);

                gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::build_cost, &args);
            }

            xPos = self->x + 3;
            yPos = self->widgets[widx::image].bottom + self->y + 16;
            auto width = self->width - 4;
            gfx::draw_rect_inset(dpi, xPos, yPos, width, 1, self->colours[1], (1 << 5));

            if (!(_byte_522096 & (1 << 3)))
                return;

            auto args = FormatArguments();

            if (_dword_1135F70 == 0xFFFFFFFF)
            {
                args.push(string_ids::new_station);
            }
            else
            {
                auto station = stationmgr::get(_dword_1135F70);
                args.push(station->name);
                args.push(station->town);
            }

            stringmgr::format_string(&_stringFormatBuffer[0], string_ids::new_station_buffer, &args);

            _currentFontSpriteBase = font::medium_bold;
            width = self->width - 4;
            int16_t stringWidth = gfx::clip_string(width, _stringFormatBuffer);
            xPos = self->x + 69 - ((stringWidth - 1) / 2);
            yPos = self->widgets[widx::image].bottom + self->y + 18;

            gfx::draw_string(dpi, xPos, yPos, colour::black, _stringFormatBuffer);

            xPos = self->x + 2;
            yPos = self->widgets[widx::image].bottom + self->y + 29;
            gfx::point_t origin = { xPos, yPos };
            gfx::draw_string_494B3F(*dpi, &origin, colour::black, string_ids::catchment_area_accepts);

            if (_dword_1135F74 == 0)
            {
                gfx::draw_string_494B3F(*dpi, origin.x, origin.y, colour::black, string_ids::catchment_area_nothing);
            }
            else
            {
                yPos--;
                for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::cargo); i++)
                {
                    if (_dword_1135F74 & (1 << i))
                    {
                        auto xPosMax = self->x + self->width - 12;
                        if (origin.x <= xPosMax)
                        {
                            auto cargoObj = objectmgr::get<cargo_object>(i);

                            gfx::draw_image(dpi, origin.x, origin.y, cargoObj->unit_inline_sprite);
                            origin.x += 10;
                        }
                    }
                }
            }

            xPos = self->x + 2;
            yPos = self->widgets[widx::image].bottom + self->y + 49;
            origin = { xPos, yPos };
            gfx::draw_string_494B3F(*dpi, &origin, colour::black, string_ids::catchment_area_produces);

            if (_dword_1135F78 == 0)
            {
                gfx::draw_string_494B3F(*dpi, origin.x, origin.y, colour::black, string_ids::catchment_area_nothing);
            }
            else
            {
                yPos--;
                for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::cargo); i++)
                {
                    if (_dword_1135F78 & (1 << i))
                    {
                        auto xPosMax = self->x + self->width - 12;
                        if (origin.x <= xPosMax)
                        {
                            auto cargoObj = objectmgr::get<cargo_object>(i);

                            gfx::draw_image(dpi, origin.x, origin.y, cargoObj->unit_inline_sprite);
                            origin.x += 10;
                        }
                    }
                }
            }
        }

        static void init_events()
        {
            events.on_close = common::on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.on_update = on_update;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
        }
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

        // 0x0049E64E
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //regs.dx = widgetIndex;
            //call(0x0049E64E, regs);

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
            }
        }

        // 0x0049E669
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //regs.dx = widgetIndex;
            //call(0x0049E669, regs);

            switch (widgetIndex)
            {
                case widx::signal_dropdown:
                {
                    uint8_t signalCount = 0;
                    while (_signalList[signalCount] != 0xFF)
                        signalCount++;

                    auto widget = self->widgets[widx::signal];
                    auto xPos = widget.left + self->x;
                    auto yPos = widget.top + self->y;
                    auto width = widget.width() + 2;
                    auto height = widget.height();

                    dropdown::show(xPos, yPos, width, height, self->colours[1], signalCount, 0x80);

                    signalCount = 0;
                    while (_signalList[signalCount] != 0xFF)
                    {
                        auto signal = _signalList[signalCount];
                        if (signal == _lastSelectedSignal)
                            dropdown::set_highlighted_item(signalCount);

                        auto trainSignalObj = objectmgr::get<train_signal_object>(signal);

                        dropdown::add(signalCount, trainSignalObj->name);

                        signalCount++;
                    }
                }

                case widx::both_directions:
                {
                    _isSignalBothDirections = 1;
                    input::cancel_tool();
                    input::toolSet(self, widgetIndex, 42);
                    break;
                }

                case widx::single_direction:
                {
                    _isSignalBothDirections = 0;
                    input::cancel_tool();
                    input::toolSet(self, widgetIndex, 42);
                    break;
                }
            }
        }

        // 0x0049E67C
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            //registers regs;
            //regs.esi = (int32_t)&self;
            //regs.dx = widgetIndex;
            //regs.ax = itemIndex;
            //call(0x0049E67C, regs);

            if (widgetIndex != widx::signal_dropdown)
                return;

            if (itemIndex != -1)
            {
                _lastSelectedSignal = _signalList[itemIndex];
                _scenarioSignals[_trackType] = _signalList[itemIndex];
                self->invalidate();
            }
        }

        // 0x0049E76F
        static void on_update(window* self)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //call(0x0049E76F, regs);

            common::on_update(self, 4);
        }

        // 0x0049E745
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049E745, regs);
        }

        // 0x0049E75A
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049E75A, regs);
        }

        // 0x0049E499
        static void prepare_draw(window* self)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //call(0x0049E499, regs);

            common::prepare_draw(self);

            auto trackObj = objectmgr::get<track_object>(_trackType);

            auto args = FormatArguments();
            args.push(trackObj->name);

            auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);

            self->widgets[widx::signal].text = trainSignalObject->name;

            common::repositionTabs(self);
        }

        // 0x0049E501
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //regs.edi = (int32_t)dpi;
            //call(0x0049E501, regs);

            self->draw(dpi);
            common::drawTabs(self, dpi);

            auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);

            auto xPos = self->x + 3;
            auto yPos = self->y + 63;
            auto width = 130;

            {
                auto args = FormatArguments();
                args.push(trainSignalObject->var_0C);

                gfx::draw_string_495224(*dpi, xPos, yPos, width, colour::black, string_ids::signal_black, &args);
            }

            auto signalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);
            auto imageId = signalObject->var_0E;

            xPos = (self->widgets[widx::both_directions].left + self->widgets[widx::both_directions].right) / 2 + self->x;
            yPos = self->widgets[widx::both_directions].bottom + self->y - 4;

            gfx::draw_image(dpi, xPos - 8, yPos, imageId);

            gfx::draw_image(dpi, xPos + 8, yPos, imageId + 4);

            xPos = (self->widgets[widx::single_direction].left + self->widgets[widx::single_direction].right) / 2 + self->x;
            yPos = self->widgets[widx::single_direction].bottom + self->y - 4;

            gfx::draw_image(dpi, xPos, yPos, imageId);

            if (_signalCost != 0x80000000 && _signalCost != 0)
            {
                auto args = FormatArguments();
                args.push<uint32_t>(_signalCost);

                xPos = self->x + 69;
                yPos = self->widgets[widx::single_direction].bottom + self->y + 5;

                gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::build_cost, &args);
            }
        }

        static void init_events()
        {
            events.on_close = common::on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.on_update = on_update;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
        }
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

        // 0x0049EBD1
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.dx = widgetIndex;
            call(0x0049EBD1, regs);
        }

        // 0x0049EBFC
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.dx = widgetIndex;
            call(0x0049EBFC, regs);
        }

        // 0x0049EC09
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = itemIndex;
            call(0x0049EC09, regs);
        }

        // 0x0049ECD1
        static void on_update(window* self)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //call(0x0049ECD1, regs);

            common::on_update(self, 16);
        }

        // 0x0049EC15
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049EC15, regs);
        }

        // 0x0049EC20
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049EC20, regs);
        }

        // 0x0049E7D3
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0049E7D3, regs);
        }

        // 0x0049EA3E
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x0049EA3E, regs);
        }
        static void init_events()
        {
            events.on_close = common::on_close;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.on_update = on_update;
            events.on_tool_update = on_tool_update;
            events.on_tool_down = on_tool_down;
            events.prepare_draw = prepare_draw;
            events.draw = draw;
        }
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
            common::sub_49FEC7();
            tilemgr::map_invalidate_map_selection_tiles();
            _mapSelectionFlags = _mapSelectionFlags & ~MapSelectFlag::enableConstruct;
            _trackCost = 0x80000000;
            _signalCost = 0x80000000;
            _stationCost = 0x80000000;
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
                self->call_on_mouse_down(signal::widx::both_directions);
            }

            if (self->current_tab == widx::tab_station - widx::tab_construction)
            {
                self->call_on_mouse_down(station::widx::image);
            }

            if (self->current_tab == widx::tab_overhead - widx::tab_construction)
            {
                self->call_on_mouse_down(overhead::widx::image);
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
                            auto roadStationObj = objectmgr::get<road_station_object>(_lastSelectedStationType);
                            auto imageId = gfx::recolour(roadStationObj->var_0C, companyColour);
                            gfx::draw_image(clipped, -4, -9, imageId);
                            auto colour = _byte_5045FA[companyColour];
                            if (!(roadStationObj->flags & road_station_flags::recolourable))
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
                                    auto trainStationObj = objectmgr::get<train_station_object>(_lastSelectedStationType);
                                    auto imageId = gfx::recolour(trainStationObj->var_0E, companyColour);
                                    gfx::draw_image(clipped, -4, -9, imageId);
                                    auto colour = _byte_5045FA[companyColour];
                                    if (!(trainStationObj->flags & train_station_flags::recolourable))
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

                        static const uint8_t* signalFrames[] = {
                            _signalFrames2State,
                            _signalFrames3State,
                            _signalFrames4State,
                        };

                        if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                        {
                            auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);
                            auto imageId = trainSignalObject->var_0E;
                            if (self->current_tab == widx::tab_signal - widx::tab_construction)
                            {
                                auto frames = signalFrames[(((trainSignalObject->num_frames + 2) / 3) - 2)];
                                auto frameCount = frames[0];
                                frameCount &= (self->frame_no >> trainSignalObject->var_04);
                                auto frameIndex = frames[frameCount + 1];
                                frameIndex <<= 3;
                                imageId += frameIndex;
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

        static void on_update(window* self, uint8_t flag)
        {
            self->frame_no++;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::construction, self->number);

            if (!input::is_tool_active(WindowType::construction, self->number))
                return;

            if (_byte_522096 & flag)
                return;

            sub_49FEC7();
        }

        static void init_events()
        {
            construction::init_events();
            station::init_events();
            signal::init_events();
            overhead::init_events();
        }

        // 0x004A0832
        static bool getRoadPieceId(registers& regs)
        {
            if (_lastSelectedTrackPiece == 0xFF)
                return true;

            switch (_lastSelectedTrackPiece)
            {
                case common::trackPiece::straight: // 0x004A0856
                {
                    if (_constructionRotation >= 12)
                        return true;
                    if (_constructionRotation >= 8)
                        return true;
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 0;
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                    {
                        regs.dh = 5;
                        if (_lastSelectedTrackGradient != common::trackGradient::slope_up)
                        {
                            regs.dh = 7;
                            if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_up)
                            {
                                regs.dh = 6;
                                if (_lastSelectedTrackGradient != common::trackGradient::slope_down)
                                {
                                    regs.dh = 8;
                                    if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_down)
                                        return true;
                                }
                            }
                        }
                    }
                    break;
                }
                case common::trackPiece::left_hand_curve_very_small: // 0x004A08A5
                {
                    if (_constructionRotation != 0)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    if (_constructionRotation >= 8)
                        return true;
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 1;
                    break;
                }
                case common::trackPiece::right_hand_curve_very_small: // 0x004A08CD
                {
                    if (_constructionRotation != 0)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    if (_constructionRotation >= 8)
                        return true;
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 2;
                    break;
                }
                case common::trackPiece::left_hand_curve_small: // 0x004A08ED
                {
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 3;
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    break;
                }
                case common::trackPiece::right_hand_curve_small: // 0x004A08FB
                {
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 4;
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    break;
                }
                case common::trackPiece::left_hand_curve: // 0x004A095F
                case common::trackPiece::right_hand_curve:
                case common::trackPiece::left_hand_curve_large:
                case common::trackPiece::right_hand_curve_large:
                case common::trackPiece::s_bend_left:
                case common::trackPiece::s_bend_right:
                case common::trackPiece::s_bend_to_dual_track:
                case common::trackPiece::s_bend_to_single_track:
                {
                    return true;
                }
                case common::trackPiece::turnaround: // 0x004A0909
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    regs.dh = 9;
                    break;
                }
            }

            regs.bh = _constructionRotation;
            if (regs.bh < 0x0C)
                regs.bh &= 3;

            regs.ax = _x;
            regs.cx = _y;
            regs.edx = (_trackType & ~(1 << 7)) | (regs.dh << 8);
            regs.dl = _trackType & ~(1 << 7);
            regs.edx |= (_lastSelectedBridge << 24);
            regs.edi = _word_1135FB8 | _lastSelectedMod << 16;

            regs.ax &= regs.ax;
            return false;
        }

        // 0x004A04F8
        static bool getTrackPieceId(registers& regs)
        {
            if (_lastSelectedTrackPiece == 0xFF)
                return true;

            switch (_lastSelectedTrackPiece)
            {
                case common::trackPiece::straight: // loc_4A051C
                {
                    if (_constructionRotation >= 12)
                    {
                        regs.dh = 1;
                        if (_lastSelectedTrackGradient != common::trackGradient::level)
                            return true;
                    }
                    else
                    {
                        if (_constructionRotation >= 8)
                        {
                            regs.dh = 27;
                            if (_lastSelectedTrackGradient != common::trackGradient::level)
                            {
                                regs.dh = 35;
                                if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_up)
                                {
                                    regs.dh = 37;
                                    if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_down)
                                        return true;
                                }
                            }
                        }
                        else
                        {
                            if (_constructionRotation >= 4)
                            {
                                regs.dh = 26;
                                if (_lastSelectedTrackGradient != common::trackGradient::level)
                                {
                                    regs.dh = 34;
                                    if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_up)
                                    {
                                        regs.dh = 36;
                                        if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_down)
                                            return true;
                                    }
                                }
                            }
                            else
                            {
                                regs.dh = 0;
                                if (_lastSelectedTrackGradient != common::trackGradient::level)
                                {
                                    regs.dh = 14;
                                    if (_lastSelectedTrackGradient != common::trackGradient::slope_up)
                                    {
                                        regs.dh = 16;
                                        if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_up)
                                        {
                                            regs.dh = 15;
                                            if (_lastSelectedTrackGradient != common::trackGradient::slope_down)
                                            {
                                                regs.dh = 17;
                                                if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_down)
                                                    return true;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }

                case common::trackPiece::left_hand_curve_very_small: // loc_4A05C3
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    if (_constructionRotation >= 8)
                    {
                        regs.dh = 29;
                        break;
                    }
                    if (_constructionRotation >= 4)
                    {
                        regs.dh = 28;
                        break;
                    }
                    regs.dh = 2;
                    break;
                }

                case common::trackPiece::right_hand_curve_very_small: // loc_4A05F4
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    if (_constructionRotation >= 8)
                    {
                        regs.dh = 31;
                        break;
                    }
                    if (_constructionRotation >= 4)
                    {
                        regs.dh = 30;
                        break;
                    }
                    regs.dh = 3;
                    break;
                }

                case common::trackPiece::left_hand_curve_small: // loc_4A0625
                {
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 4;
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                    {
                        regs.dh = 18;
                        if (_lastSelectedTrackGradient != common::trackGradient::slope_up)
                        {
                            regs.dh = 22;
                            if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_up)
                            {
                                regs.dh = 20;
                                if (_lastSelectedTrackGradient != common::trackGradient::slope_down)
                                {
                                    regs.dh = 24;
                                    if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_down)
                                        return true;
                                }
                            }
                        }
                    }
                    break;
                }

                case common::trackPiece::right_hand_curve_small: // loc_4A066A
                {
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 5;
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                    {
                        regs.dh = 19;
                        if (_lastSelectedTrackGradient != common::trackGradient::slope_up)
                        {
                            regs.dh = 23;
                            if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_up)
                            {
                                regs.dh = 21;
                                if (_lastSelectedTrackGradient != common::trackGradient::slope_down)
                                {
                                    regs.dh = 25;
                                    if (_lastSelectedTrackGradient != common::trackGradient::steep_slope_down)
                                        return true;
                                }
                            }
                        }
                    }
                    break;
                }

                case common::trackPiece::left_hand_curve: // loc_4A06AF
                {
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 6;
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    break;
                }

                case common::trackPiece::right_hand_curve: // loc_4A06C8
                {
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 7;
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    break;
                }

                case common::trackPiece::left_hand_curve_large: // loc_4A06E1
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    regs.dh = 10;
                    if (_constructionRotation >= 12)
                        break;
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 8;
                    break;
                }

                case common::trackPiece::right_hand_curve_large: // loc_4A0705
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    regs.dh = 11;
                    if (_constructionRotation >= 12)
                        break;
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 9;
                    break;
                }

                case common::trackPiece::s_bend_left: // loc_4A0729
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    regs.dh = 33;
                    if (_constructionRotation >= 8)
                        break;
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 12;
                    break;
                }

                case common::trackPiece::s_bend_right: // loc_4A0756
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    if (_constructionRotation >= 8)
                        return true;
                    regs.dh = 32;
                    if (_constructionRotation >= 4)
                        break;
                    regs.dh = 13;
                    break;
                }

                case common::trackPiece::s_bend_to_dual_track: // loc_4A077C
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    if (_constructionRotation >= 8)
                        return true;
                    regs.dh = 40;
                    if (_constructionRotation >= 4)
                        break;
                    regs.dh = 38;
                    break;
                }

                case common::trackPiece::s_bend_to_single_track: // loc_4A07A2
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    regs.dh = 41;
                    if (_constructionRotation >= 8)
                        break;
                    if (_constructionRotation >= 4)
                        return true;
                    regs.dh = 39;
                    break;
                }

                case common::trackPiece::turnaround: // loc_4A07C0
                {
                    if (_lastSelectedTrackGradient != common::trackGradient::level)
                        return true;
                    if (_constructionRotation >= 12)
                        return true;
                    regs.dh = 43;
                    if (_constructionRotation >= 8)
                        break;
                    regs.dh = 42;
                    if (_constructionRotation >= 4)
                        return true;
                    break;
                }
            }

            regs.bh = _constructionRotation;
            if (regs.bh < 12)
                regs.bh &= 3;

            regs.ax = _x;
            regs.cx = _y;
            regs.edx = _trackType | (regs.dh << 8);
            regs.dl = _trackType;
            regs.edx |= (_lastSelectedBridge << 24);
            regs.edi = _word_1135FB8 | _lastSelectedMod << 16;

            if (_byte_113607E & (1 << 1))
            {
                auto thing = thingmgr::get<Thing>(2492);
                regs.edi |= thing->type;
            }

            regs.ax &= regs.ax;
            return false;
        }

        // 0x0049F1B5
        static void sub_49F1B5()
        {
            //registers regs;
            //call(0x0049F1B5, regs);

            static const uint8_t trackPieceWidgets[] = {
                construction::widx::straight,
                construction::widx::left_hand_curve_very_small,
                construction::widx::right_hand_curve_very_small,
                construction::widx::left_hand_curve_small,
                construction::widx::right_hand_curve_small,
                construction::widx::left_hand_curve,
                construction::widx::right_hand_curve,
                construction::widx::left_hand_curve_large,
                construction::widx::right_hand_curve_large,
                construction::widx::s_bend_left,
                construction::widx::s_bend_right,
            };

            auto window = WindowManager::find(WindowType::construction);

            if (window == nullptr)
            {
                return;
            }

            if (_trackType & (1 << 7))
            {
                tilemgr::map_invalidate_map_selection_tiles();
                _mapSelectionFlags = _mapSelectionFlags | 0x0A;

                registers regs;
                auto invalidRoad = getRoadPieceId(regs);

                auto rotation = regs.bh;
                auto roadId = regs.dh;
                auto x = _x;
                auto y = _y;

                if (invalidRoad)
                {
                    rotation = _constructionRotation;
                    roadId = 0;
                }

                auto roadPiece = roadPieces[roadId];
                auto i = 0;
                auto posId = 0;
                while (roadPiece[i] != 0xFF)
                {
                    if (roadPiece[i + 9] & (1 << 7))
                    {
                        i += 10;
                        continue;
                    }
                    auto xPos = roadPiece[i + 1];
                    auto yPos = roadPiece[i + 3];

                    switch (rotation)
                    {
                        case 0:
                            break;
                        case 1:
                            std::swap(xPos, yPos);
                            yPos = -yPos;
                            break;
                        case 2:
                            xPos = -xPos;
                            yPos = -yPos;
                            break;
                        case 3:
                            std::swap(xPos, yPos);
                            xPos = -xPos;
                            break;
                    }
                    xPos += x;
                    yPos += y;
                    _mapSelectedTiles[posId].x = xPos;
                    _mapSelectedTiles[posId].y = yPos;
                    posId++;
                    i += 10;
                }
                _mapSelectedTiles[posId].x = -1;
                map_invalidate_map_selection_tiles();
                window->holdable_widgets = 0;
                auto trackType = _trackType & ~(1 << 7);
                auto roadObj = objectmgr::get<road_object>(trackType);

                window->widgets[construction::widx::s_bend_left].type = widget_type::none;
                window->widgets[construction::widx::s_bend_right].type = widget_type::none;
                window->widgets[construction::widx::left_hand_curve_large].type = widget_type::none;
                window->widgets[construction::widx::right_hand_curve_large].type = widget_type::none;
                window->widgets[construction::widx::left_hand_curve].type = widget_type::none;
                window->widgets[construction::widx::right_hand_curve].type = widget_type::none;
                window->widgets[construction::widx::left_hand_curve_small].type = widget_type::none;
                window->widgets[construction::widx::right_hand_curve_small].type = widget_type::none;
                window->widgets[construction::widx::left_hand_curve_very_small].type = widget_type::none;
                window->widgets[construction::widx::right_hand_curve_very_small].type = widget_type::none;

                window->widgets[construction::widx::left_hand_curve_small].left = 3;
                window->widgets[construction::widx::left_hand_curve_small].right = 24;
                window->widgets[construction::widx::right_hand_curve_small].left = 113;
                window->widgets[construction::widx::right_hand_curve_small].right = 134;
                window->widgets[construction::widx::left_hand_curve].left = 25;
                window->widgets[construction::widx::left_hand_curve].right = 46;
                window->widgets[construction::widx::right_hand_curve].left = 91;
                window->widgets[construction::widx::right_hand_curve].right = 112;

                if (roadObj->road_pieces & road_piece_flags::track)
                {
                    window->widgets[construction::widx::left_hand_curve_small].left = 25;
                    window->widgets[construction::widx::left_hand_curve_small].right = 46;
                    window->widgets[construction::widx::right_hand_curve_small].left = 91;
                    window->widgets[construction::widx::right_hand_curve_small].right = 112;
                    window->widgets[construction::widx::left_hand_curve].left = 47;
                    window->widgets[construction::widx::left_hand_curve].right = 68;
                    window->widgets[construction::widx::right_hand_curve].left = 69;
                    window->widgets[construction::widx::right_hand_curve].right = 90;

                    window->widgets[construction::widx::left_hand_curve_very_small].type = widget_type::wt_9;
                    window->widgets[construction::widx::right_hand_curve_very_small].type = widget_type::wt_9;
                }

                if (roadObj->road_pieces & road_piece_flags::one_way)
                {
                    window->widgets[construction::widx::left_hand_curve_small].type = widget_type::wt_9;
                    window->widgets[construction::widx::right_hand_curve_small].type = widget_type::wt_9;
                }

                window->widgets[construction::widx::s_bend_dual_track_left].type = widget_type::none;
                window->widgets[construction::widx::s_bend_dual_track_right].type = widget_type::none;

                if (roadObj->road_pieces & road_piece_flags::one_sided)
                {
                    window->widgets[construction::widx::s_bend_dual_track_left].type = widget_type::wt_9;
                    window->widgets[construction::widx::s_bend_dual_track_left].image = image_ids::construction_right_turnaround;
                    window->widgets[construction::widx::s_bend_dual_track_left].tooltip = string_ids::tooltip_turnaround;
                    if (_byte_525FAE == 0)
                        window->widgets[construction::widx::s_bend_dual_track_left].image = image_ids::construction_left_turnaround;
                }

                window->widgets[construction::widx::steep_slope_down].type = widget_type::none;
                window->widgets[construction::widx::slope_down].type = widget_type::none;
                window->widgets[construction::widx::slope_up].type = widget_type::none;
                window->widgets[construction::widx::steep_slope_up].type = widget_type::none;

                if (roadObj->road_pieces & road_piece_flags::slope)
                {
                    window->widgets[construction::widx::slope_down].type = widget_type::wt_9;
                    window->widgets[construction::widx::slope_up].type = widget_type::wt_9;
                }

                if (roadObj->road_pieces & road_piece_flags::steep_slope)
                {
                    window->widgets[construction::widx::steep_slope_down].type = widget_type::wt_9;
                    window->widgets[construction::widx::steep_slope_up].type = widget_type::wt_9;
                }

                window->widgets[construction::widx::bridge].type = widget_type::wt_18;
                window->widgets[construction::widx::bridge_dropdown].type = widget_type::wt_11;

                if (_lastSelectedBridge == 0xFF || (_constructionHover != 1 && !(_byte_1136076 & 1)))
                {
                    window->widgets[construction::widx::bridge].type = widget_type::none;
                    window->widgets[construction::widx::bridge_dropdown].type = widget_type::none;
                }

                auto activatedWidgets = window->activated_widgets;
                activatedWidgets &= ~(construction::allTrack);

                window->widgets[construction::widx::construct].type = widget_type::none;
                window->widgets[construction::widx::remove].type = widget_type::wt_9;
                window->widgets[construction::widx::rotate_90].type = widget_type::none;

                if (_constructionHover == 1)
                {
                    window->widgets[construction::widx::construct].type = widget_type::wt_5;
                    window->widgets[construction::widx::construct].tooltip = string_ids::tooltip_start_construction;
                    window->widgets[construction::widx::remove].type = widget_type::none;
                    window->widgets[construction::widx::rotate_90].type = widget_type::wt_9;
                    window->widgets[construction::widx::rotate_90].image = image_ids::rotate_object;
                    window->widgets[construction::widx::rotate_90].tooltip = string_ids::rotate_90;
                }
                else if (_constructionHover == 0)
                {
                    window->widgets[construction::widx::construct].type = widget_type::wt_3;
                    window->widgets[construction::widx::construct].tooltip = string_ids::tooltip_construct;
                    window->widgets[construction::widx::rotate_90].type = widget_type::wt_9;
                    window->widgets[construction::widx::rotate_90].image = image_ids::construction_new_position;
                    window->widgets[construction::widx::rotate_90].tooltip = string_ids::new_construction_position;
                }
                if (_constructionHover == 0 || _constructionHover == 1)
                {
                    if (_lastSelectedTrackPiece != 0xFF)
                    {
                        auto trackPieceWidget = trackPieceWidgets[_lastSelectedTrackPiece];
                        activatedWidgets |= 1ULL << trackPieceWidget;
                    }

                    uint8_t trackGradient = construction::widx::level;

                    switch (_lastSelectedTrackGradient)
                    {
                        case common::trackGradient::level:
                            trackGradient = construction::widx::level;
                            break;

                        case common::trackGradient::slope_up:
                            trackGradient = construction::widx::slope_up;
                            break;

                        case common::trackGradient::slope_down:
                            trackGradient = construction::widx::slope_down;
                            break;

                        case common::trackGradient::steep_slope_up:
                            trackGradient = construction::widx::steep_slope_up;
                            break;

                        case common::trackGradient::steep_slope_down:
                            trackGradient = construction::widx::steep_slope_down;
                            break;
                    }

                    activatedWidgets |= 1ULL << trackGradient;
                }
                window->activated_widgets = activatedWidgets;
                window->invalidate();
            }
            else
            {
                tilemgr::map_invalidate_map_selection_tiles();
                _mapSelectionFlags = _mapSelectionFlags | 0x0A;

                registers regs;
                auto invalidRoad = getTrackPieceId(regs);

                auto rotation = regs.bh;
                auto roadId = regs.dh;
                auto x = _x;
                auto y = _y;

                if (invalidRoad)
                {
                    rotation = _constructionRotation;
                    roadId = 0;
                }

                auto trackPiece = trackPieces[roadId];
                auto i = 0;
                auto posId = 0;
                while (trackPiece[i] != 0xFF)
                {
                    if (trackPiece[i + 9] & (1 << 7))
                    {
                        i += 10;
                        continue;
                    }
                    auto xPos = trackPiece[i + 1];
                    auto yPos = trackPiece[i + 3];

                    switch (rotation)
                    {
                        case 0:
                            break;
                        case 1:
                            std::swap(xPos, yPos);
                            yPos = -yPos;
                            break;
                        case 2:
                            xPos = -xPos;
                            yPos = -yPos;
                            break;
                        case 3:
                            std::swap(xPos, yPos);
                            xPos = -xPos;
                            break;
                    }
                    xPos += x;
                    yPos += y;
                    _mapSelectedTiles[posId].x = xPos;
                    _mapSelectedTiles[posId].y = yPos;
                    posId++;
                    i += 10;
                }
                _mapSelectedTiles[posId].x = -1;
                map_invalidate_map_selection_tiles();
                window->holdable_widgets = 0;

                auto trackObj = objectmgr::get<track_object>(_trackType);

                window->widgets[construction::widx::s_bend_left].type = widget_type::wt_9;
                window->widgets[construction::widx::s_bend_right].type = widget_type::wt_9;
                window->widgets[construction::widx::left_hand_curve_large].type = widget_type::none;
                window->widgets[construction::widx::right_hand_curve_large].type = widget_type::none;
                window->widgets[construction::widx::left_hand_curve].type = widget_type::none;
                window->widgets[construction::widx::right_hand_curve].type = widget_type::none;
                window->widgets[construction::widx::left_hand_curve_small].type = widget_type::none;
                window->widgets[construction::widx::right_hand_curve_small].type = widget_type::none;
                window->widgets[construction::widx::left_hand_curve_very_small].type = widget_type::none;
                window->widgets[construction::widx::right_hand_curve_very_small].type = widget_type::none;

                window->widgets[construction::widx::left_hand_curve_small].left = 3;
                window->widgets[construction::widx::left_hand_curve_small].right = 24;
                window->widgets[construction::widx::right_hand_curve_small].left = 113;
                window->widgets[construction::widx::right_hand_curve_small].right = 134;
                window->widgets[construction::widx::left_hand_curve].left = 25;
                window->widgets[construction::widx::left_hand_curve].right = 46;
                window->widgets[construction::widx::right_hand_curve].left = 91;
                window->widgets[construction::widx::right_hand_curve].right = 112;

                if (trackObj->track_pieces & track_piece_flags::very_small_curve)
                {
                    window->widgets[construction::widx::left_hand_curve_small].left = 25;
                    window->widgets[construction::widx::left_hand_curve_small].right = 46;
                    window->widgets[construction::widx::right_hand_curve_small].left = 91;
                    window->widgets[construction::widx::right_hand_curve_small].right = 112;
                    window->widgets[construction::widx::left_hand_curve].left = 47;
                    window->widgets[construction::widx::left_hand_curve].right = 68;
                    window->widgets[construction::widx::right_hand_curve].left = 69;
                    window->widgets[construction::widx::right_hand_curve].right = 90;

                    window->widgets[construction::widx::left_hand_curve_very_small].type = widget_type::wt_9;
                    window->widgets[construction::widx::right_hand_curve_very_small].type = widget_type::wt_9;
                }

                if (trackObj->track_pieces & track_piece_flags::large_curve)
                {
                    window->widgets[construction::widx::left_hand_curve_large].type = widget_type::wt_9;
                    window->widgets[construction::widx::right_hand_curve_large].type = widget_type::wt_9;
                }

                if (trackObj->track_pieces & track_piece_flags::normal_curve)
                {
                    window->widgets[construction::widx::left_hand_curve].type = widget_type::wt_9;
                    window->widgets[construction::widx::right_hand_curve].type = widget_type::wt_9;
                }

                if (trackObj->track_pieces & track_piece_flags::small_curve)
                {
                    window->widgets[construction::widx::left_hand_curve_small].type = widget_type::wt_9;
                    window->widgets[construction::widx::right_hand_curve_small].type = widget_type::wt_9;
                }

                window->widgets[construction::widx::s_bend_dual_track_left].type = widget_type::none;
                window->widgets[construction::widx::s_bend_dual_track_right].type = widget_type::none;

                if (trackObj->track_pieces & track_piece_flags::one_sided)
                {
                    window->widgets[construction::widx::s_bend_dual_track_left].type = widget_type::wt_9;
                    window->widgets[construction::widx::s_bend_dual_track_right].type = widget_type::wt_9;
                    window->widgets[construction::widx::s_bend_dual_track_left].image = image_ids::construction_s_bend_dual_track_left;
                    window->widgets[construction::widx::s_bend_dual_track_right].image = image_ids::construction_s_bend_dual_track_right;
                    window->widgets[construction::widx::s_bend_dual_track_left].tooltip = string_ids::tooltip_s_bend_left_dual_track;
                    window->widgets[construction::widx::s_bend_dual_track_right].tooltip = string_ids::tooltip_s_bend_right_dual_track;

                    _byte_522090 = 16;
                    _byte_522091 = 20;

                    if (_constructionRotation >= 4 && _constructionRotation < 12)
                    {
                        window->widgets[construction::widx::s_bend_dual_track_left].image = image_ids::construction_right_turnaround;
                        window->widgets[construction::widx::s_bend_dual_track_right].image = image_ids::construction_s_bend_to_single_track_left;
                        window->widgets[construction::widx::s_bend_dual_track_left].tooltip = string_ids::tooltip_turnaround;
                        window->widgets[construction::widx::s_bend_dual_track_right].tooltip = string_ids::tooltip_s_bend_to_single_track;
                        _byte_522090 = 20;
                        _byte_522092 = 16;
                        if (_constructionRotation >= 8)
                        {
                            window->widgets[construction::widx::s_bend_dual_track_left].image = image_ids::construction_s_bend_to_single_track_right;
                            window->widgets[construction::widx::s_bend_dual_track_right].image = image_ids::construction_left_turnaround;
                            window->widgets[construction::widx::s_bend_dual_track_left].tooltip = string_ids::tooltip_s_bend_to_single_track;
                            window->widgets[construction::widx::s_bend_dual_track_right].tooltip = string_ids::tooltip_turnaround;
                            _byte_522091 = 16;
                            _byte_522092 = 20;
                        }
                    }
                }
                window->widgets[construction::widx::steep_slope_down].type = widget_type::none;
                window->widgets[construction::widx::slope_down].type = widget_type::none;
                window->widgets[construction::widx::slope_up].type = widget_type::none;
                window->widgets[construction::widx::steep_slope_up].type = widget_type::none;

                if (trackObj->track_pieces & track_piece_flags::slope)
                {
                    window->widgets[construction::widx::slope_down].type = widget_type::wt_9;
                    window->widgets[construction::widx::slope_up].type = widget_type::wt_9;
                }

                if (trackObj->track_pieces & track_piece_flags::steep_slope)
                {
                    window->widgets[construction::widx::steep_slope_down].type = widget_type::wt_9;
                    window->widgets[construction::widx::steep_slope_up].type = widget_type::wt_9;
                }

                window->widgets[construction::widx::bridge].type = widget_type::wt_18;
                window->widgets[construction::widx::bridge_dropdown].type = widget_type::wt_11;

                if (_lastSelectedBridge == 0xFF || (_constructionHover != 1 && !(_byte_1136076 & 1)))
                {
                    window->widgets[construction::widx::bridge].type = widget_type::none;
                    window->widgets[construction::widx::bridge_dropdown].type = widget_type::none;
                }

                auto activatedWidgets = window->activated_widgets;
                activatedWidgets &= ~(construction::allTrack);

                window->widgets[construction::widx::construct].type = widget_type::none;
                window->widgets[construction::widx::remove].type = widget_type::wt_9;
                window->widgets[construction::widx::rotate_90].type = widget_type::none;

                if (_constructionHover == 1)
                {
                    window->widgets[construction::widx::construct].type = widget_type::wt_5;
                    window->widgets[construction::widx::construct].tooltip = string_ids::tooltip_start_construction;
                    window->widgets[construction::widx::remove].type = widget_type::none;
                    window->widgets[construction::widx::rotate_90].type = widget_type::wt_9;
                    window->widgets[construction::widx::rotate_90].image = image_ids::rotate_object;
                    window->widgets[construction::widx::rotate_90].tooltip = string_ids::rotate_90;
                }
                else if (_constructionHover == 0)
                {
                    window->widgets[construction::widx::construct].type = widget_type::wt_3;
                    window->widgets[construction::widx::construct].tooltip = string_ids::tooltip_construct;
                    window->widgets[construction::widx::rotate_90].type = widget_type::wt_9;
                    window->widgets[construction::widx::rotate_90].image = image_ids::construction_new_position;
                    window->widgets[construction::widx::rotate_90].tooltip = string_ids::new_construction_position;
                }
                if (_constructionHover == 0 || _constructionHover == 1)
                {
                    if (_lastSelectedTrackPiece != 0xFF)
                    {
                        auto trackPieceWidget = trackPieceWidgets[_lastSelectedTrackPiece];
                        activatedWidgets |= 1ULL << trackPieceWidget;
                    }

                    uint8_t trackGradient = construction::widx::level;

                    switch (_lastSelectedTrackGradient)
                    {
                        case common::trackGradient::level:
                            trackGradient = construction::widx::level;
                            break;

                        case common::trackGradient::slope_up:
                            trackGradient = construction::widx::slope_up;
                            break;

                        case common::trackGradient::slope_down:
                            trackGradient = construction::widx::slope_down;
                            break;

                        case common::trackGradient::steep_slope_up:
                            trackGradient = construction::widx::steep_slope_up;
                            break;

                        case common::trackGradient::steep_slope_down:
                            trackGradient = construction::widx::steep_slope_down;
                            break;
                    }

                    activatedWidgets |= 1ULL << trackGradient;
                }
                window->activated_widgets = activatedWidgets;
                window->invalidate();
            }
        }
    }
}
