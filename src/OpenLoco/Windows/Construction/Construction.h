#pragma once

#include "../../Interop/Interop.hpp"
#include "../../Map/TileManager.h"
#include "../../Objects/VehicleObject.h"
#include "../../Ui/WindowManager.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction
{
    static loco_global<uint8_t, 0x00522095> _byte_522095;
    static loco_global<uint8_t, 0x00522096> _byte_522096;
    static loco_global<Ui::WindowType, 0x00523392> _toolWindowType;
    static loco_global<CompanyId, 0x00525E3C> _playerCompany;
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
    static loco_global<Gfx::Context*, 0x00E0C3E0> _dword_E0C3E0;
    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<Pos2[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;
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
    static loco_global<uint16_t, 0x01135FB8> _constructionZ;
    static loco_global<Map::Pos3, 0x01135FBA> _ghostTrackPos;
    static loco_global<Map::Pos3, 0x01135FC0> _ghostRemovalTrackPos;
    static loco_global<uint8_t, 0x0113606A> _ghostRemovalTrackId;
    static loco_global<uint8_t, 0x0113606B> _stationGhostRotation;
    static loco_global<uint8_t, 0x0113606C> _stationGhostTrackId;
    static loco_global<uint8_t, 0x0113606D> _stationGhostTileIndex;
    static loco_global<uint8_t, 0x01136069> _ghostRemovalTrackRotation;
    static loco_global<uint8_t, 0x00522093> _ghostRemovalTrackObjectId;
    static loco_global<Map::Pos3, 0x01135FC6> _nextTile;
    static loco_global<uint16_t, 0x01135FCC> _nextTileRotation;
    static loco_global<Map::Pos3, 0x01135FCE> _previousTile;
    static loco_global<uint16_t, 0x01135FD4> _previousTileRotation;
    static loco_global<uint16_t, 0x01135FD6> _word_1135FD6;
    static loco_global<uint16_t, 0x01135FD8> _word_1135FD8;
    static loco_global<uint16_t, 0x01135FE4> _lastSelectedMods;
    static loco_global<Map::Pos3, 0x01135FE6> _stationGhostPos;
    static loco_global<uint16_t, 0x01135FEE> _stationGhostType;
    static loco_global<Map::Pos3, 0x01135FF8> _modGhostPos;
    static loco_global<uint16_t, 0x01135FFE> _word_1135FFE;
    static loco_global<int16_t, 0x01136000> _word_1136000;
    static loco_global<uint16_t, 0x01136002> _signalGhostSides;
    static loco_global<Map::Pos3, 0x01136004> _signalGhostPos;
    static loco_global<uint16_t, 0x0113600A> _signalGhostTrackObjId;
    static loco_global<uint8_t, 0x01136010> _modGhostTrackObjId;
    static loco_global<uint8_t[17], 0x0113601D> _signalList;
    static loco_global<uint8_t, 0x0113602E> _lastSelectedSignal;
    static loco_global<uint8_t, 0x0113602F> _isSignalBothDirections;
    static loco_global<uint8_t[9], 0x01136030> _bridgeList;
    static loco_global<uint8_t, 0x01136039> _lastSelectedBridge;
    static loco_global<uint8_t, 0x0113603A> _byte_113603A;
    static loco_global<uint8_t[17], 0x0113603B> _stationList;
    static loco_global<uint8_t, 0x0113604C> _lastSelectedStationType;
    static loco_global<uint8_t, 0x0113604D> _signalGhostRotation;
    static loco_global<uint8_t, 0x0113604E> _signalGhostTrackId;
    static loco_global<uint8_t, 0x0113604F> _signalGhostTileIndex;
    static loco_global<uint8_t[4], 0x01136054> _modList;
    static loco_global<uint8_t, 0x01136058> _modGhostRotation;
    static loco_global<uint8_t, 0x01136059> _modGhostTrackId;
    static loco_global<uint8_t, 0x0113605A> _modGhostTileIndex;
    static loco_global<uint8_t, 0x0113605D> _makeJunction;
    static loco_global<uint8_t, 0x01136061> _constructionHover;
    static loco_global<uint8_t, 0x01136062> _trackType;
    static loco_global<uint8_t, 0x01136063> _byte_1136063;
    static loco_global<uint8_t, 0x01136064> _constructionRotation;
    static loco_global<uint8_t, 0x01136065> _byte_1136065;
    static loco_global<uint8_t, 0x01136066> _byte_1136066;
    static loco_global<uint8_t, 0x01136067> _lastSelectedTrackPiece;
    static loco_global<uint8_t, 0x01136068> _lastSelectedTrackGradient;
    static loco_global<uint8_t, 0x0113606E> _lastSelectedTrackModSection;
    static loco_global<uint8_t, 0x01136072> _byte_1136072;
    static loco_global<uint8_t, 0x01136073> _byte_1136073;
    static loco_global<uint8_t, 0x01136075> _byte_1136075;
    static loco_global<uint8_t, 0x01136076> _byte_1136076;
    static loco_global<uint8_t, 0x01136077> _byte_1136077;
    static loco_global<uint8_t, 0x01136078> _byte_1136078;
    static loco_global<uint8_t, 0x01136079> _lastSelectedTrackPieceId;
    static loco_global<uint8_t, 0x0113607E> _byte_113607E;
    static loco_global<Map::TileElement, 0x01136090> _backupTileElement;

    namespace Common
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

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                                      \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, WidgetType::frame, WindowColour::primary),                                                                     \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_24, WindowColour::primary, windowCaptionId),                                                \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 0, 41 }, { frameWidth, frameHeight - 41 }, WidgetType::wt_3, WindowColour::secondary),                                                          \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_track_road_construction),                     \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_station_construction),                       \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_signal_construction),                        \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_electrification_construction)

        constexpr uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_construction) | (1 << widx::tab_station) | (1 << widx::tab_signal) | (1 << widx::tab_overhead);

        void prepareDraw(Window* self);
        void resetWindow(Window& self, WidgetIndex_t tabWidgetIndex);
        void switchTab(Window* self, WidgetIndex_t widgetIndex);
        void repositionTabs(Window* self);
        void drawTabs(Window* self, Gfx::Context* context);
        void initEvents();
        void onClose(Window* self);
        void onUpdate(Window* self, uint8_t flag);
        void sub_4CD454();
        void setTrackOptions(const uint8_t trackType);
        void setDisabledWidgets(Window* self);
        void createConstructionWindow();
        void refreshAirportList(uint8_t* stationList);
        void refreshDockList(uint8_t* stationList);
        void refreshStationList(uint8_t* stationList, uint8_t trackType, TransportMode transportMode);
        void refreshBridgeList(uint8_t* bridgeList, uint8_t trackType, TransportMode transportMode);
        void refreshModList(uint8_t* modList, uint8_t trackType, TransportMode transportMode);
        void sub_4A3A50();
        void refreshSignalList(uint8_t* signalList, uint8_t trackType);
        void setNextAndPreviousTrackTile(const TrackElement& elTrack, const Map::Pos2& pos);
        void setNextAndPreviousRoadTile(const RoadElement& elRoad, const Map::Pos2& pos);
        bool isPointCloserToNextOrPreviousTile(const Point& point, const Viewport& viewport);
        void previousTab(Window* self);
        void nextTab(Window* self);
    }

    namespace Construction
    {
        static const Ui::Size windowSize = { 138, 276 };

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

        extern Widget widgets[32];

        extern WindowEventList events;
        constexpr uint64_t enabledWidgets = Common::enabledWidgets | allConstruction;

        void reset();
        void activateSelectedConstructionWidgets();
        void tabReset(Window* self);
        void initEvents();
        void drawTrack(uint16_t x, uint16_t y, uint16_t selectedMods, uint16_t di, uint8_t trackType, uint8_t trackPieceId, uint16_t colour, uint8_t bh);
        void drawRoad(uint16_t x, uint16_t y, uint16_t selectedMods, uint16_t di, uint8_t trackType, uint8_t trackPieceId, uint16_t colour, uint8_t bh);
        void removeTrackGhosts();
        void previousTrackPiece(Window* self);
        void nextTrackPiece(Window* self);
        void previousSlope(Window* self);
        void nextSlope(Window* self);
        void buildAtCurrentPos(Window* self);
        void removeAtCurrentPos(Window* self);
        void selectPosition(Window* self);
    }

    namespace Station
    {
        enum widx
        {
            station = 8,
            station_dropdown,
            image,
            rotate,
        };

        extern Widget widgets[13];

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << station) | (1 << station_dropdown) | (1 << image) | (1 << rotate);

        extern WindowEventList events;
        void tabReset(Window* self);
        void initEvents();
        void removeStationGhost();
    }

    namespace Signal
    {
        enum widx
        {
            signal = 8,
            signal_dropdown,
            both_directions,
            single_direction,
        };

        extern Widget widgets[13];

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << signal) | (1 << signal_dropdown) | (1 << both_directions) | (1 << single_direction);

        extern WindowEventList events;
        void tabReset(Window* self);
        void initEvents();
        void removeSignalGhost();
    }

    namespace Overhead
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

        extern Widget widgets[16];

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << checkbox_1) | (1 << checkbox_2) | (1 << checkbox_3) | (1 << checkbox_4) | (1 << image) | (1 << track) | (1 << track_dropdown);

        extern WindowEventList events;
        void tabReset(Window* self);
        void initEvents();
        void removeTrackModsGhost();
    }
}
