#pragma once

#include "../../interop/interop.hpp"
#include "../../map/tilemgr.h"
#include "../../objects/vehicle_object.h"
#include "../../ui/WindowManager.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction
{
#pragma pack(push, 1)
    struct previewTrack
    {
        uint8_t index; // 0x00
        int16_t x;     // 0x01
        int16_t y;     // 0x03
        int16_t z;     // 0x05
        uint8_t var_07;
        uint8_t var_08;
        uint8_t flags; // 0x09
    };
#pragma pack(pop)

    enum previewTrackFlags
    {
        diagonal = 1 << 7,
    };

    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;
    static loco_global<previewTrack[1], 0x004F6D44> _unk_4F6D44;
    static loco_global<previewTrack[1], 0x004F6D4F> _unk_4F6D4F;
    static loco_global<previewTrack[1], 0x004F6D5A> _unk_4F6D5A;
    static loco_global<previewTrack[4], 0x004F6D65> _unk_4F6D65;
    static loco_global<previewTrack[4], 0x004F6D8E> _unk_4F6D8E;
    static loco_global<previewTrack[2], 0x004F6DB7> _unk_4F6DB7;
    static loco_global<previewTrack[2], 0x004F6DCC> _unk_4F6DCC;
    static loco_global<previewTrack[1], 0x004F6DE1> _unk_4F6DE1;
    static loco_global<previewTrack[1], 0x004F6DEC> _unk_4F6DEC;
    static loco_global<previewTrack[1], 0x004F6DF7> _unk_4F6DF7;

    static loco_global<previewTrack[1], 0x004F7488> _unk_4F7488;
    static loco_global<previewTrack[4], 0x004F7493> _unk_4F7493;
    static loco_global<previewTrack[1], 0x004F74BC> _unk_4F74BC;
    static loco_global<previewTrack[1], 0x004F74C7> _unk_4F74C7;
    static loco_global<previewTrack[4], 0x004F74D2> _unk_4F74D2;
    static loco_global<previewTrack[4], 0x004F74FB> _unk_4F74FB;
    static loco_global<previewTrack[5], 0x004F7524> _unk_4F7524;
    static loco_global<previewTrack[5], 0x004F7557> _unk_4F7557;
    static loco_global<previewTrack[5], 0x004F758A> _unk_4F758A;
    static loco_global<previewTrack[5], 0x004F75BD> _unk_4F75BD;
    static loco_global<previewTrack[5], 0x004F75F0> _unk_4F75F0;
    static loco_global<previewTrack[5], 0x004F7623> _unk_4F7623;
    static loco_global<previewTrack[4], 0x004F7656> _unk_4F7656;
    static loco_global<previewTrack[4], 0x004F767F> _unk_4F767F;
    static loco_global<previewTrack[2], 0x004F76A8> _unk_4F76A8;
    static loco_global<previewTrack[2], 0x004F76BD> _unk_4F76BD;
    static loco_global<previewTrack[1], 0x004F76D2> _unk_4F76D2;
    static loco_global<previewTrack[1], 0x004F76DD> _unk_4F76DD;
    static loco_global<previewTrack[4], 0x004F76E8> _unk_4F76E8;
    static loco_global<previewTrack[4], 0x004F7711> _unk_4F7711;
    static loco_global<previewTrack[4], 0x004F773A> _unk_4F773A;
    static loco_global<previewTrack[4], 0x004F7763> _unk_4F7763;
    static loco_global<previewTrack[4], 0x004F778C> _unk_4F778C;
    static loco_global<previewTrack[4], 0x004F77B5> _unk_4F77B5;
    static loco_global<previewTrack[4], 0x004F77DE> _unk_4F77DE;
    static loco_global<previewTrack[4], 0x004F7807> _unk_4F7807;
    static loco_global<previewTrack[1], 0x004F7830> _unk_4F7830;
    static loco_global<previewTrack[1], 0x004F783B> _unk_4F783B;
    static loco_global<previewTrack[1], 0x004F7846> _unk_4F7846;
    static loco_global<previewTrack[1], 0x004F7851> _unk_4F7851;
    static loco_global<previewTrack[1], 0x004F785C> _unk_4F785C;
    static loco_global<previewTrack[1], 0x004F7867> _unk_4F7867;
    static loco_global<previewTrack[1], 0x004F7872> _unk_4F7872;
    static loco_global<previewTrack[1], 0x004F787D> _unk_4F787D;
    static loco_global<previewTrack[1], 0x004F7888> _unk_4F7888;
    static loco_global<previewTrack[1], 0x004F7893> _unk_4F7893;
    static loco_global<previewTrack[1], 0x004F789E> _unk_4F789E;
    static loco_global<previewTrack[1], 0x004F78A9> _unk_4F78A9;
    static loco_global<previewTrack[1], 0x004F78B4> _unk_4F78B4;
    static loco_global<previewTrack[1], 0x004F78BF> _unk_4F78BF;
    static loco_global<previewTrack[1], 0x004F78CA> _unk_4F78CA;
    static loco_global<previewTrack[1], 0x004F78D5> _unk_4F78D5;
    static loco_global<previewTrack[1], 0x004F78E0> _unk_4F78E0;
    static loco_global<previewTrack[1], 0x004F78EB> _unk_4F78EB;

    static loco_global<uint16_t[351][4], 0x004F7B62> _word_4F7B62; // TODO: Not sure on size?
    static loco_global<uint8_t[31], 0x005045FA> _byte_5045FA;
    static loco_global<uint8_t, 0x00508F09> _byte_508F09;
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
    static loco_global<uint8_t, 0x00F24948> _byte_F24948;
    static loco_global<uint16_t, 0x00F24942> _word_F24942;
    static loco_global<uint16_t, 0x00F24944> _word_F24944;
    static loco_global<uint16_t, 0x00F24946> _word_F24946;
    static loco_global<company_id_t, 0x009C68EB> _updatingCompanyId;
    static loco_global<gfx::drawpixelinfo_t*, 0x00E0C3E0> _dword_E0C3E0;
    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;
    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<map_pos[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;
    static loco_global<string_id, 0x009C68E6> gGameCommandErrorText;
    static loco_global<int32_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> _stringFormatBuffer;
    static loco_global<uint32_t, 0x01135F3E> _trackCost;
    static loco_global<uint32_t, 0x01135F42> _dword_1135F42;
    static loco_global<uint32_t, 0x01135F46> _modCost;
    static loco_global<uint32_t, 0x01135F4E> _signalCost;
    static loco_global<uint32_t, 0x01135F6C> _stationCost;
    static loco_global<uint32_t, 0x01135F70> _constructingStationId;
    static loco_global<uint32_t, 0x01135F74> _constructingStationAcceptedCargoTypes;
    static loco_global<uint32_t, 0x01135F78> _constructingStationProducedCargoTypes;
    static loco_global<uint16_t, 0x01135F86> _word_1135F86;
    static loco_global<uint16_t, 0x01135FB4> _x;
    static loco_global<uint16_t, 0x01135FB6> _y;
    static loco_global<uint16_t, 0x01135FB8> _word_1135FB8;
    static loco_global<uint16_t, 0x01135FBA> _word_1135FBA;
    static loco_global<uint16_t, 0x01135FBC> _word_1135FBC;
    static loco_global<uint16_t, 0x01135FBE> _word_1135FBE;
    static loco_global<uint16_t, 0x01135FD6> _word_1135FD6;
    static loco_global<uint16_t, 0x01135FD8> _word_1135FD8;
    static loco_global<uint16_t, 0x01135FE4> _lastSelectedMods;
    static loco_global<uint16_t, 0x01135FFE> _word_1135FFE;
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
    static loco_global<uint8_t, 0x0113605D> _byte_113605D;
    static loco_global<uint8_t, 0x01136061> _constructionHover;
    static loco_global<uint8_t, 0x01136062> _trackType;
    static loco_global<uint8_t, 0x01136063> _byte_1136063;
    static loco_global<uint8_t, 0x01136064> _constructionRotation;
    static loco_global<uint8_t, 0x01136065> _byte_1136065;
    static loco_global<uint8_t, 0x01136066> _byte_1136066;
    static loco_global<uint8_t, 0x01136067> _lastSelectedTrackPiece;
    static loco_global<uint8_t, 0x01136068> _lastSelectedTrackGradient;
    static loco_global<uint8_t, 0x0113606E> _lastSelectedTrackModSection;
    static loco_global<uint8_t, 0x01136073> _byte_1136073;
    static loco_global<uint8_t, 0x01136075> _byte_1136075;
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

        struct trackPieceId
        {
            uint8_t id;
            uint8_t rotation;
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

        void prepare_draw(window* self);
        void switchTab(window* self, widget_index widgetIndex);
        void repositionTabs(window* self);
        void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        void init_events();
        std::optional<trackPieceId> getRoadPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation);
        std::optional<trackPieceId> getTrackPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation);
        void activateSelectedConstructionWidgets();
        void sub_49FEC7();
        void on_close(window* self);
        void on_update(window* self, uint8_t flag);
        void sub_4CD454();
        void setTrackOptions(const uint8_t trackType);
        void setDisabledWidgets(window* self);
        void createConstructionWindow();
        void refreshAirportList(uint8_t* stationList);
        void refreshDockList(uint8_t* stationList);
        void refreshStationList(uint8_t* stationList, uint8_t trackType, TransportMode transportMode);
        void refreshBridgeList(uint8_t* bridgeList, uint8_t trackType, TransportMode transportMode);
        void refreshModList(uint8_t* modList, uint8_t trackType, TransportMode transportMode);
        void sub_4A3A50();
        void refreshSignalList(uint8_t* signalList, uint8_t trackType);

        extern const uint8_t trackPieceWidgets[11];
        extern const previewTrack* roadPieces[10];
        extern const previewTrack* trackPieces[44];
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
        //clang-format on

        extern widget_t widgets[32];

        extern window_event_list events;
        constexpr uint64_t enabledWidgets = common::enabledWidgets | allConstruction;
        void tabReset(window* self);
        void init_events();        
        void drawTrack(uint16_t x, uint16_t y, uint16_t selectedMods, uint16_t di, uint8_t trackType, uint8_t trackPieceId, uint16_t colour, uint8_t bh);
        void drawRoad(uint16_t x, uint16_t y, uint16_t selectedMods, uint16_t di, uint8_t trackType, uint8_t trackPieceId, uint16_t colour, uint8_t bh);
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

        extern widget_t widgets[13];

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << station) | (1 << station_dropdown) | (1 << image) | (1 << rotate);

        extern window_event_list events;        
        void tabReset(window* self);
        void init_events();
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
        
        extern widget_t widgets[13];

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << signal) | (1 << signal_dropdown) | (1 << both_directions) | (1 << single_direction);

        extern window_event_list events;        
        void tabReset(window* self);
        void init_events();
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

        extern widget_t widgets[16];

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << checkbox_1) | (1 << checkbox_2) | (1 << checkbox_3) | (1 << checkbox_4) | (1 << image) | (1 << track) | (1 << track_dropdown);

        extern window_event_list events;        
        void tabReset(window* self);
        void init_events();
    }
}
