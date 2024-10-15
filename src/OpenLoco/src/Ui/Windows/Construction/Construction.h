#pragma once

#include "Map/TileManager.h"
#include "Objects/VehicleObject.h"
#include "ScenarioConstruction.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::World::TileManager;

namespace OpenLoco::Ui::Windows::Construction
{
    enum class GhostVisibilityFlags : uint8_t
    {
        none = 0U,
        constructArrow = 1U << 0,
        track = 1U << 1,
        signal = 1U << 2,
        station = 1U << 3,
        overhead = 1U << 4,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(GhostVisibilityFlags);

    static loco_global<uint8_t, 0x00522093> _ghostRemovalTrackObjectId;
    static loco_global<uint8_t, 0x00522095> _byte_522095;
    static loco_global<GhostVisibilityFlags, 0x00522096> _ghostVisibilityFlags;

    constexpr uint16_t mapSelectedTilesSize = 300;
    static loco_global<Pos2[mapSelectedTilesSize], 0x00F24490> _mapSelectedTiles;
    static loco_global<char[512], 0x0112CC04> _stringFormatBuffer;

#pragma pack(push, 1)
    struct ConstructionState
    {
        uint32_t trackCost;                             // 0x01135F3E
        uint32_t dword_1135F42;                         // 0x01135F42
        uint32_t modCost;                               // 0x01135F46
        uint32_t pad_1135F4A;                           // 0x01135F4A
        uint32_t signalCost;                            // 0x01135F4E
        uint8_t pad_1135F52[26];                        // 0x01135F52
        uint32_t stationCost;                           // 0x01135F6C
        uint32_t constructingStationId;                 // 0x01135F70
        uint32_t constructingStationAcceptedCargoTypes; // 0x01135F74
        uint32_t constructingStationProducedCargoTypes; // 0x01135F78
        uint8_t pad_1135F7C[10];                        // 0x01135F7C
        ViewportFlags viewportFlags;                    // 0x01135F86
        uint8_t pad_1135F88[44];                        // 0x01135F88
        uint16_t x;                                     // 0x01135FB4
        uint16_t y;                                     // 0x01135FB6
        uint16_t constructionZ;                         // 0x01135FB8
        World::Pos3 ghostTrackPos;                      // 0x01135FBA
        World::Pos3 ghostRemovalTrackPos;               // 0x01135FC0
        World::Pos3 nextTile;                           // 0x01135FC6
        uint16_t nextTileRotation;                      // 0x01135FCC
        World::Pos3 previousTile;                       // 0x01135FCE
        uint16_t previousTileRotation;                  // 0x01135FD4
        uint16_t word_1135FD6;                          // 0x01135FD6
        uint16_t word_1135FD8;                          // 0x01135FD8
        uint8_t pad_1135FDA[10];                        // 0x01135FDA
        uint16_t lastSelectedMods;                      // 0x01135FE4
        World::Pos3 stationGhostPos;                    // 0x01135FE6
        uint8_t pad_1135FEC[2];                         // 0x01135FEC
        uint16_t stationGhostType;                      // 0x01135FEE
        uint8_t pad_1135FF0[8];                         // 0x01135FF0
        World::Pos3 modGhostPos;                        // 0x01135FF8
        uint16_t word_1135FFE;                          // 0x01135FFE
        int16_t word_1136000;                           // 0x01136000
        uint16_t signalGhostSides;                      // 0x01136002
        World::Pos3 signalGhostPos;                     // 0x01136004
        uint16_t signalGhostTrackObjId;                 // 0x0113600A
        uint8_t pad_113600C[4];                         // 0x0113600C
        uint8_t modGhostTrackObjId;                     // 0x01136010
        uint8_t pad_11360011[12];                       // 0x01136011
        uint8_t signalList[17];                         // 0x0113601D
        uint8_t lastSelectedSignal;                     // 0x0113602E
        uint8_t isSignalBothDirections;                 // 0x0113602F
        uint8_t bridgeList[9];                          // 0x01136030
        uint8_t lastSelectedBridge;                     // 0x01136039
        uint8_t byte_113603A;                           // 0x0113603A
        uint8_t stationList[17];                        // 0x0113603B
        uint8_t lastSelectedStationType;                // 0x0113604C
        uint8_t signalGhostRotation;                    // 0x0113604D
        uint8_t signalGhostTrackId;                     // 0x0113604E
        uint8_t signalGhostTileIndex;                   // 0x0113604F
        uint8_t pad_1136050[4];                         // 0x01136050
        uint8_t modList[4];                             // 0x01136054
        uint8_t modGhostRotation;                       // 0x01136058
        uint8_t modGhostTrackId;                        // 0x01136059
        uint8_t modGhostTileIndex;                      // 0x0113605A
        uint8_t pad_113605B[2];                         // 0x0113605B
        uint8_t makeJunction;                           // 0x0113605D
        uint8_t pad_113605E[3];                         // 0x0113605E
        uint8_t constructionHover;                      // 0x01136061
        uint8_t trackType;                              // 0x01136062
        uint8_t byte_1136063;                           // 0x01136063
        uint8_t constructionRotation;                   // 0x01136064
        uint8_t byte_1136065;                           // 0x01136065
        uint8_t constructionArrowFrameNum;              // 0x01136066
        uint8_t lastSelectedTrackPiece;                 // 0x01136067
        uint8_t lastSelectedTrackGradient;              // 0x01136068
        uint8_t ghostRemovalTrackRotation;              // 0x01136069
        uint8_t ghostRemovalTrackId;                    // 0x0113606A
        uint8_t stationGhostRotation;                   // 0x0113606B
        uint8_t stationGhostTrackId;                    // 0x0113606C
        uint8_t stationGhostTileIndex;                  // 0x0113606D
        uint8_t lastSelectedTrackModSection;            // 0x0113606E
        uint8_t pad_113606F[3];                         // 0x0113606F
        uint8_t byte_1136072;                           // 0x01136072
        uint8_t byte_1136073;                           // 0x01136073
        uint8_t pad_1136074;                            // 0x01136074
        uint8_t byte_1136075;                           // 0x01136075
        uint8_t byte_1136076;                           // 0x01136076
        uint8_t byte_1136077;                           // 0x01136077
        uint8_t byte_1136078;                           // 0x01136078
        uint8_t lastSelectedTrackPieceId;               // 0x01136079
        uint8_t pad_113607A[4];                         // 0x0113607A
        uint8_t byte_113607E;                           // 0x0113607E
        uint8_t pad_113607F[10];                        // 0x0113607F
        uint8_t stationGhostTypeDockAirport;            // 0x01136089
        uint8_t pad_113608A[6];                         // 0x0113608A
        World::TileElement backupTileElement;           // 0x01136090
    };
#pragma pack(pop)

    static_assert(sizeof(ConstructionState) == 0x01136090 + 8 - 0x01135F3E);
    static loco_global<ConstructionState, 0x01135F3E> _cState;

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

        constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight, StringId windowCaptionId)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { frameWidth, frameHeight }, WindowColour::primary),
                makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_24, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                makeWidget({ 0, 41 }, { frameWidth, frameHeight - 41 }, WidgetType::wt_3, WindowColour::secondary),
                makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_track_road_construction),
                makeRemapWidget({ 34, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_station_construction),
                makeRemapWidget({ 65, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_signal_construction),
                makeRemapWidget({ 96, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tab_electrification_construction));
        }

        constexpr uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_construction) | (1 << widx::tab_station) | (1 << widx::tab_signal) | (1 << widx::tab_overhead);

        void prepareDraw(Window* self);
        void resetWindow(Window& self, WidgetIndex_t tabWidgetIndex);
        void switchTab(Window* self, WidgetIndex_t widgetIndex);
        void repositionTabs(Window* self);
        void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx);
        void onClose(Window& self);
        void onUpdate(Window* self, GhostVisibilityFlags flag);
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
        void setNextAndPreviousTrackTile(const TrackElement& elTrack, const World::Pos2& pos);
        void setNextAndPreviousRoadTile(const RoadElement& elRoad, const World::Pos2& pos);
        bool isPointCloserToNextOrPreviousTile(const Point& point, const Viewport& viewport);
        void previousTab(Window* self);
        void nextTab(Window* self);
    }

    namespace Construction
    {
        static constexpr Ui::Size32 kWindowSize = { 138, 276 };

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

        std::span<const Widget> getWidgets();

        constexpr uint64_t enabledWidgets = Common::enabledWidgets | allConstruction;

        void reset();
        void activateSelectedConstructionWidgets();
        void tabReset(Window* self);
        void drawTrack(const World::Pos3& pos, uint16_t selectedMods, uint8_t trackType, uint8_t trackPieceId, uint8_t rotation, Gfx::DrawingContext& drawingCtx);
        void drawRoad(const World::Pos3& pos, uint16_t selectedMods, uint8_t trackType, uint8_t trackPieceId, uint8_t rotation, Gfx::DrawingContext& drawingCtx);
        void removeTrackGhosts();
        void previousTrackPiece(Window* self);
        void nextTrackPiece(Window* self);
        void previousSlope(Window* self);
        void nextSlope(Window* self);
        void buildAtCurrentPos(Window* self);
        void removeAtCurrentPos(Window* self);
        void selectPosition(Window* self);
        const WindowEventList& getEvents();
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

        std::span<const Widget> getWidgets();

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << station) | (1 << station_dropdown) | (1 << image) | (1 << rotate);

        void tabReset(Window* self);
        void removeStationGhost();
        const WindowEventList& getEvents();
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

        std::span<const Widget> getWidgets();

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << signal) | (1 << signal_dropdown) | (1 << both_directions) | (1 << single_direction);

        void tabReset(Window* self);
        void removeSignalGhost();
        const WindowEventList& getEvents();
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

        std::span<const Widget> getWidgets();

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << checkbox_1) | (1 << checkbox_2) | (1 << checkbox_3) | (1 << checkbox_4) | (1 << image) | (1 << track) | (1 << track_dropdown);

        void tabReset(Window* self);
        void removeTrackModsGhost();
        const WindowEventList& getEvents();
    }
}
