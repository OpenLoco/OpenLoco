#include "Audio/Audio.h"
#include "Construction.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Road/CreateRoad.h"
#include "GameCommands/Road/RemoveRoad.h"
#include "GameCommands/Track/CreateTrack.h"
#include "GameCommands/Track/RemoveTrack.h"
#include "GameState.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Map/MapSelection.h"
#include "Map/RoadElement.h"
#include "Map/SurfaceElement.h"
#include "Map/Track/Track.h"
#include "Map/Track/TrackData.h"
#include "Map/TrackElement.h"
#include "Objects/BridgeObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Paint/Paint.h"
#include "Paint/PaintTile.h"
#include "Ui/Dropdown.h"
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "World/CompanyManager.h"
#include "World/Station.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;
using namespace OpenLoco::World::TileManager;
using namespace OpenLoco::Literals;

namespace OpenLoco::Ui::Windows::Construction::Construction
{
    static loco_global<uint8_t, 0x00508F09> _suppressErrorSound;
    static loco_global<uint8_t, 0x00522090> _byte_522090;
    static loco_global<uint8_t, 0x00522091> _byte_522091;
    static loco_global<uint8_t, 0x00522092> _byte_522092;

    static loco_global<World::Pos3, 0x00F24942> _constructionArrowPos;
    static loco_global<uint8_t, 0x00F24948> _constructionArrowDirection;

    static loco_global<uint8_t, 0x0112C2E9> _alternateTrackObjectId; // set from GameCommands::createRoad
    static loco_global<uint8_t[18], 0x0050A006> _availableObjects;   // top toolbar

    namespace TrackPiece
    {
        enum
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
    }

    namespace TrackGradient
    {
        enum
        {
            level = 0,
            slope_up = 2,
            steep_slope_up = 4,
            slope_down = 6,
            steep_slope_down = 8,
        };
    };

    struct TrackPieceId
    {
        uint8_t id;
        uint8_t rotation;
    };

    static constexpr auto widgets = makeWidgets(
        Common::makeCommonWidgets(138, 276, StringIds::stringid_2),
        Widgets::ImageButton({ 3, 45 }, { 22, 24 }, WindowColour::secondary, ImageIds::construction_left_hand_curve_very_small, StringIds::tooltip_left_hand_curve_very_small),
        Widgets::ImageButton({ 3, 45 }, { 22, 24 }, WindowColour::secondary, ImageIds::construction_left_hand_curve_small, StringIds::tooltip_left_hand_curve_small),
        Widgets::ImageButton({ 25, 45 }, { 22, 24 }, WindowColour::secondary, ImageIds::construction_left_hand_curve, StringIds::tooltip_left_hand_curve),
        Widgets::ImageButton({ 47, 45 }, { 22, 24 }, WindowColour::secondary, ImageIds::construction_left_hand_curve_large, StringIds::tooltip_left_hand_curve_large),
        Widgets::ImageButton({ 69, 45 }, { 22, 24 }, WindowColour::secondary, ImageIds::construction_right_hand_curve_large, StringIds::tooltip_right_hand_curve_large),
        Widgets::ImageButton({ 91, 45 }, { 22, 24 }, WindowColour::secondary, ImageIds::construction_right_hand_curve, StringIds::tooltip_right_hand_curve),
        Widgets::ImageButton({ 113, 45 }, { 22, 24 }, WindowColour::secondary, ImageIds::construction_right_hand_curve_small, StringIds::tooltip_right_hand_curve_small),
        Widgets::ImageButton({ 113, 45 }, { 22, 24 }, WindowColour::secondary, ImageIds::construction_right_hand_curve_very_small, StringIds::tooltip_right_hand_curve_very_small),
        Widgets::ImageButton({ 9, 69 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_s_bend_dual_track_left, StringIds::tooltip_s_bend_left_dual_track),
        Widgets::ImageButton({ 33, 69 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_s_bend_left, StringIds::tooltip_s_bend_left),
        Widgets::ImageButton({ 57, 69 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_straight, StringIds::tooltip_straight),
        Widgets::ImageButton({ 81, 69 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_s_bend_right, StringIds::tooltip_s_bend_right),
        Widgets::ImageButton({ 105, 69 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_s_bend_dual_track_right, StringIds::tooltip_s_bend_right_dual_track),
        Widgets::ImageButton({ 9, 96 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_steep_slope_down, StringIds::tooltip_steep_slope_down),
        Widgets::ImageButton({ 33, 96 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_slope_down, StringIds::tooltip_slope_down),
        Widgets::ImageButton({ 57, 96 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_level, StringIds::tooltip_level),
        Widgets::ImageButton({ 81, 96 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_slope_up, StringIds::tooltip_slope_up),
        Widgets::ImageButton({ 105, 96 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_steep_slope_up, StringIds::tooltip_steep_slope_up),
        makeDropdownWidgets({ 40, 123 }, { 58, 20 }, WindowColour::secondary, StringIds::empty, StringIds::tooltip_bridge_stats),
        makeWidget({ 3, 145 }, { 132, 100 }, WidgetType::wt_6, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_construct),
        Widgets::ImageButton({ 6, 248 }, { 46, 24 }, WindowColour::secondary, ImageIds::construction_remove, StringIds::tooltip_remove),
        Widgets::ImageButton({ 57, 248 }, { 24, 24 }, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_90));

    std::span<const Widget> getWidgets()
    {
        return widgets;
    }

    const uint8_t trackPieceWidgets[] = {
        widx::straight,
        widx::left_hand_curve_very_small,
        widx::right_hand_curve_very_small,
        widx::left_hand_curve_small,
        widx::right_hand_curve_small,
        widx::left_hand_curve,
        widx::right_hand_curve,
        widx::left_hand_curve_large,
        widx::right_hand_curve_large,
        widx::s_bend_left,
        widx::s_bend_right,
        widx::s_bend_dual_track_left, // s_bend_to_dual_track; unused?
        widx::s_bend_dual_track_left, // s_bend_to_single_track; unused?
        widx::s_bend_dual_track_left, // turnaround
    };

    WindowEventList events;

    static std::optional<TrackPieceId> getRoadPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation);
    static std::optional<TrackPieceId> getTrackPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation);

    // 0x0049B50C
    void reset()
    {
        Scenario::resetTrackObjects();

        getGameState().lastAirport = 0xFF;
        getGameState().lastShipPort = 0xFF;
        getGameState().pickupDirection = 0;
    }

    // 0x004A18D4
    static void sub_4A18D4()
    {
        if (_alternateTrackObjectId == 0xFF)
        {
            return;
        }

        if ((_alternateTrackObjectId | (1 << 7)) == _cState->trackType)
        {
            return;
        }

        auto* alternativeRoadObj = ObjectManager::get<RoadObject>(_alternateTrackObjectId);
        if (!alternativeRoadObj->hasFlags(RoadObjectFlags::unk_03))
        {
            return;
        }
        auto* curRoadObj = ObjectManager::get<RoadObject>(_cState->trackType & ~(1 << 7));
        if (!curRoadObj->hasFlags(RoadObjectFlags::unk_03))
        {
            return;
        }

        for (const auto objId : _availableObjects)
        {
            if (objId == 0xFF)
            {
                return;
            }

            if (objId == (_alternateTrackObjectId | (1 << 7)))
            {
                _cState->trackType = _alternateTrackObjectId | (1 << 7);
                Common::sub_4A3A50();
            }
        }
    }

    // 0x0049FA10
    static void constructRoad()
    {
        _cState->trackCost = 0x80000000;
        _cState->byte_1136076 = 0;
        _cState->dword_1135F42 = 0x80000000;
        removeConstructionGhosts();
        auto roadPiece = getRoadPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);
        if (!roadPiece)
        {
            return;
        }
        auto* roadObj = ObjectManager::get<RoadObject>(_cState->trackType & ~(1 << 7));
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(StringId));
        formatArgs.push(roadObj->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

        GameCommands::RoadPlacementArgs args;
        args.pos = World::Pos3(_cState->x, _cState->y, _cState->constructionZ);
        args.rotation = roadPiece->rotation;
        args.roadId = roadPiece->id;
        args.mods = _cState->lastSelectedMods;
        args.bridge = _cState->lastSelectedBridge;
        args.roadObjectId = _cState->trackType & ~(1 << 7);
        args.unkFlags = 0;

        _cState->dword_1135F42 = GameCommands::doCommand(args, GameCommands::Flags::apply);
        if (_cState->dword_1135F42 == GameCommands::FAILURE)
        {
            if (GameCommands::getErrorText() != StringIds::unable_to_cross_or_create_junction_with_string
                || _alternateTrackObjectId == 0xFF)
            {
                return;
            }

            sub_4A18D4();
            roadPiece = getRoadPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);
            if (!roadPiece)
            {
                return;
            }

            WindowManager::close(WindowType::error);
            args.pos = World::Pos3(_cState->x, _cState->y, _cState->constructionZ);
            args.rotation = roadPiece->rotation;
            args.roadId = roadPiece->id;
            args.mods = _cState->lastSelectedMods;
            args.bridge = _cState->lastSelectedBridge;
            args.roadObjectId = _cState->trackType & ~(1 << 7);

            _cState->dword_1135F42 = GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        if (_cState->dword_1135F42 != GameCommands::FAILURE)
        {
            const auto& trackSize = TrackData::getUnkRoad((args.roadId << 3) | (args.rotation & 0x3));
            const auto newPosition = args.pos + trackSize.pos;
            _cState->x = newPosition.x;
            _cState->y = newPosition.y;
            _cState->constructionZ = newPosition.z;
            _cState->constructionRotation = trackSize.rotationEnd;
            _ghostVisibilityFlags = GhostVisibilityFlags::none;
            _cState->constructionArrowFrameNum = 0;
            if (_cState->lastSelectedTrackPiece >= 9)
            {
                _cState->lastSelectedTrackPiece = 0;
            }
        }
    }

    // 0x0049F93A
    static void constructTrack()
    {
        _cState->trackCost = 0x80000000;
        _cState->byte_1136076 = 0;
        _cState->dword_1135F42 = 0x80000000;
        removeConstructionGhosts();
        auto trackPiece = getTrackPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);
        if (!trackPiece)
        {
            return;
        }
        auto* roadObj = ObjectManager::get<TrackObject>(_cState->trackType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(StringId));
        formatArgs.push(roadObj->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

        GameCommands::TrackPlacementArgs args;
        args.pos = World::Pos3(_cState->x, _cState->y, _cState->constructionZ);
        args.rotation = trackPiece->rotation;
        args.trackId = trackPiece->id;
        args.mods = _cState->lastSelectedMods;
        args.bridge = _cState->lastSelectedBridge;
        args.trackObjectId = _cState->trackType;
        args.unk = _cState->byte_113607E & (1 << 0);
        args.unkFlags = 0;

        _cState->dword_1135F42 = GameCommands::doCommand(args, GameCommands::Flags::apply);

        if (_cState->dword_1135F42 == GameCommands::FAILURE)
        {
            return;
        }

        const auto& trackSize = TrackData::getUnkTrack((args.trackId << 3) | (args.rotation & 0x3));
        const auto newPosition = args.pos + trackSize.pos;
        _cState->x = newPosition.x;
        _cState->y = newPosition.y;
        _cState->constructionZ = newPosition.z;
        _cState->constructionRotation = trackSize.rotationEnd;
        _ghostVisibilityFlags = GhostVisibilityFlags::none;
        _cState->constructionArrowFrameNum = 0;
        if (_cState->lastSelectedTrackPiece >= 9)
        {
            _cState->lastSelectedTrackPiece = 0;
        }
    }

    // 0x0049F92D
    static void constructTrack([[maybe_unused]] Window* self, [[maybe_unused]] WidgetIndex_t widgetIndex)
    {
        if (_cState->trackType & (1 << 7))
        {
            constructRoad();
        }
        else
        {
            constructTrack();
        }
        activateSelectedConstructionWidgets();
    }

    static loco_global<World::Track::LegacyTrackConnections, 0x0113609C> _113609C;

    // 0x004A012E
    static void removeTrack()
    {
        _cState->trackCost = 0x80000000;
        _cState->byte_1136076 = 0;
        removeConstructionGhosts();
        if (_cState->constructionHover != 0)
        {
            return;
        }

        World::Pos3 loc(_cState->x, _cState->y, _cState->constructionZ);
        uint32_t trackAndDirection = 0;

        if (_cState->constructionRotation < 4)
        {
            trackAndDirection = 0;
        }
        else if (_cState->constructionRotation < 8)
        {
            trackAndDirection = 26 << 3;
        }
        else if (_cState->constructionRotation < 12)
        {
            trackAndDirection = 27 << 3;
        }
        else
        {
            trackAndDirection = 1 << 3;
            loc += World::Pos3{ World::kRotationOffset[_cState->constructionRotation], 0 };
        }
        trackAndDirection |= (1 << 2) | (_cState->constructionRotation & 0x3);
        _113609C->size = 0;
        auto trackEnd = World::Track::getTrackConnectionEnd(loc, trackAndDirection);
        auto tc = World::Track::getTrackConnections(trackEnd.nextPos, trackEnd.nextRotation, CompanyManager::getControllingId(), _cState->trackType, 0, 0);
        World::Track::toLegacyConnections(tc, _113609C); // Unsure if still needed
        if (tc.connections.empty())
        {
            return;
        }

        const auto trackAndDirection2 = (tc.connections.back() & World::Track::AdditionalTaDFlags::basicTaDMask) ^ (1 << 2);
        World::Pos3 loc2(_cState->x, _cState->y, _cState->constructionZ);
        loc2 -= TrackData::getUnkTrack(trackAndDirection2).pos;
        if (trackAndDirection2 & (1 << 2))
        {
            loc2.z += TrackData::getUnkTrack(trackAndDirection2).pos.z;
        }

        const auto& trackPiece = TrackData::getTrackPiece(trackAndDirection2 >> 3);
        const auto i = (trackAndDirection2 & (1 << 2)) ? trackPiece.size() - 1 : 0;
        loc2.z += trackPiece[i].z;

        GameCommands::TrackRemovalArgs args;
        args.pos = loc2;
        args.index = trackPiece[i].index;
        args.rotation = trackAndDirection2 & 0x3;
        args.trackId = trackAndDirection2 >> 3;
        args.trackObjectId = _cState->trackType;

        auto* trackObj = ObjectManager::get<TrackObject>(_cState->trackType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(StringId));
        formatArgs.push(trackObj->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            World::Pos3 newConstructLoc = World::Pos3(_cState->x, _cState->y, _cState->constructionZ) - TrackData::getUnkTrack(trackAndDirection2).pos;
            _cState->x = newConstructLoc.x;
            _cState->y = newConstructLoc.y;
            _cState->constructionZ = newConstructLoc.z;
            _cState->constructionRotation = TrackData::getUnkTrack(trackAndDirection2).rotationBegin;
            _cState->lastSelectedTrackPiece = 0;
            _cState->lastSelectedTrackGradient = 0;
            activateSelectedConstructionWidgets();
        }
    }

    // 0x004A02F2
    static void removeRoad()
    {
        _cState->trackCost = 0x80000000;
        _cState->byte_1136076 = 0;
        removeConstructionGhosts();
        if (_cState->constructionHover != 0)
        {
            return;
        }

        World::Pos3 loc(_cState->x, _cState->y, _cState->constructionZ);
        uint32_t trackAndDirection = (1 << 2) | (_cState->constructionRotation & 0x3);
        _113609C->size = 0;
        const auto roadEnd = World::Track::getRoadConnectionEnd(loc, trackAndDirection);
        auto rc = World::Track::getRoadConnections(roadEnd.nextPos, roadEnd.nextRotation, CompanyManager::getControllingId(), _cState->trackType & ~(1 << 7), 0, 0);

        if (rc.connections.empty())
        {
            return;
        }

        for (auto& c : rc.connections)
        {
            // If trackId is zero
            if ((c & 0x1F8) == 0)
            {
                std::swap(c, rc.connections[0]);
            }
        }

        auto* roadObj = ObjectManager::get<RoadObject>(_cState->trackType & ~(1 << 7));
        if (!roadObj->hasFlags(RoadObjectFlags::unk_02))
        {
            rc.connections.resize(1);
        }

        uint16_t trackAndDirection2 = 0;
        for (auto c : rc.connections)
        {
            trackAndDirection2 = (c & World::Track::AdditionalTaDFlags::basicTaDMask) ^ (1 << 2);
            World::Pos3 loc2(_cState->x, _cState->y, _cState->constructionZ);
            loc2 -= TrackData::getUnkRoad(trackAndDirection2).pos;
            if (trackAndDirection2 & (1 << 2))
            {
                loc2.z += TrackData::getUnkRoad(trackAndDirection2).pos.z;
            }

            const auto& roadPiece = TrackData::getRoadPiece(trackAndDirection2 >> 3);
            const auto i = (trackAndDirection2 & (1 << 2)) ? roadPiece.size() - 1 : 0;
            loc2.z += roadPiece[i].z;

            GameCommands::RoadRemovalArgs args;
            args.pos = loc2;
            args.sequenceIndex = roadPiece[i].index;
            args.rotation = trackAndDirection2 & 0x3;
            args.roadId = trackAndDirection2 >> 3;
            args.objectId = _cState->trackType & ~(1 << 7);

            auto* trackObj = ObjectManager::get<RoadObject>(_cState->trackType & ~(1 << 7));
            auto formatArgs = FormatArguments::common();
            formatArgs.skip(3 * sizeof(StringId));
            formatArgs.push(trackObj->name);
            GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

            if (GameCommands::doCommand(args, GameCommands::Flags::apply) == GameCommands::FAILURE)
            {
                return;
            }
        }

        World::Pos3 newConstructLoc = World::Pos3(_cState->x, _cState->y, _cState->constructionZ) - TrackData::getUnkRoad(trackAndDirection2).pos;
        _cState->x = newConstructLoc.x;
        _cState->y = newConstructLoc.y;
        _cState->constructionZ = newConstructLoc.z;
        _cState->constructionRotation = TrackData::getUnkRoad(trackAndDirection2).rotationBegin;
        _cState->lastSelectedTrackPiece = 0;
        _cState->lastSelectedTrackGradient = 0;
        activateSelectedConstructionWidgets();
    }

    // 0x004A0121
    static void removeTrack([[maybe_unused]] Window* self, [[maybe_unused]] WidgetIndex_t widgetIndex)
    {
        if (_cState->trackType & (1 << 7))
        {
            removeRoad();
        }
        else
        {
            removeTrack();
        }
    }

    // 0x0049D3F6
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::widx::close_button:
                WindowManager::close(&self);
                break;

            case Common::widx::tab_construction:
            case Common::widx::tab_overhead:
            case Common::widx::tab_signal:
            case Common::widx::tab_station:
                Common::switchTab(&self, widgetIndex);
                break;

            case widx::construct:
                constructTrack(&self, widgetIndex);
                break;

            case widx::remove:
                removeTrack(&self, widgetIndex);
                break;

            case widx::rotate_90:
            {
                if (_cState->constructionHover == 1)
                {
                    _cState->constructionRotation++;
                    _cState->constructionRotation = _cState->constructionRotation & 3;
                    _cState->trackCost = 0x80000000;
                    activateSelectedConstructionWidgets();
                    break;
                }
                removeConstructionGhosts();
                WindowManager::viewportSetVisibility(WindowManager::ViewportVisibility::overgroundView);
                ToolManager::toolSet(&self, widx::construct, CursorId::crosshair);
                Input::setFlag(Input::Flags::flag6);

                _cState->constructionHover = 1;
                _cState->byte_113607E = 0;
                _cState->constructionRotation = _cState->constructionRotation & 3;

                activateSelectedConstructionWidgets();
                break;
            }
        }
    }

    // 0x0049DB71
    static void disableUnusedPiecesRotation(uint64_t* disabledWidgets)
    {
        if (_cState->constructionRotation < 12)
        {
            if (_cState->constructionRotation >= 8)
            {
                *disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_small);
                *disabledWidgets |= (1 << widx::s_bend_right) | (1 << widx::slope_down) | (1 << widx::slope_up);
            }
            else
            {
                if (_cState->constructionRotation >= 4)
                {
                    *disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_small);
                    *disabledWidgets |= (1 << widx::s_bend_left) | (1 << widx::slope_down) | (1 << widx::slope_up);
                }
            }
        }
        else
        {
            *disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small) | (1 << widx::right_hand_curve_small);
            *disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right) | (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
        }
    }

    // 0x0049DBEC
    static void disableUnusedRoadPieces(Window* self, uint64_t disabledWidgets)
    {
        if (_cState->lastSelectedTrackGradient == 2 || _cState->lastSelectedTrackGradient == 6 || _cState->lastSelectedTrackGradient == 4 || _cState->lastSelectedTrackGradient == 8)
        {
            disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
            disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);
            disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
        }

        disableUnusedPiecesRotation(&disabledWidgets);

        if (_cState->constructionHover == 0)
        {
            auto road = getRoadPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);
            if (!road)
                disabledWidgets |= (1 << widx::construct);
        }
        self->setDisabledWidgetsAndInvalidate(disabledWidgets);
    }

    // 0x0049DB1F
    static void disableUnusedTrackPieces(Window* self, TrackObject trackObj, uint64_t disabledWidgets)
    {
        if (_cState->lastSelectedTrackGradient == 2 || _cState->lastSelectedTrackGradient == 6 || _cState->lastSelectedTrackGradient == 4 || _cState->lastSelectedTrackGradient == 8)
        {
            disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
            disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);

            if (!trackObj.hasTraitFlags(Track::TrackTraitFlags::slopedCurve))
                disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
        }

        disableUnusedPiecesRotation(&disabledWidgets);

        if (_cState->constructionHover == 0)
        {
            auto track = getTrackPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);

            if (!track)
                disabledWidgets |= (1 << widx::construct);
        }
        self->setDisabledWidgetsAndInvalidate(disabledWidgets);
    }

    // 0x0049DAF3
    static void disableTrackSlopes(Window* self, TrackObject trackObj, uint64_t disabledWidgets)
    {
        if (!trackObj.hasTraitFlags(Track::TrackTraitFlags::slope)
            && !trackObj.hasTraitFlags(Track::TrackTraitFlags::slopedCurve))
            disabledWidgets |= (1 << widx::slope_down) | (1 << widx::slope_up);

        if (!trackObj.hasTraitFlags(Track::TrackTraitFlags::steepSlope)
            && !trackObj.hasTraitFlags(Track::TrackTraitFlags::slopedCurve))
            disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::steep_slope_up);

        disableUnusedTrackPieces(self, trackObj, disabledWidgets);
    }

    static void setMapSelectedTilesFromPiece(const std::span<const TrackData::PreviewTrack> pieces, const World::Pos2& origin, const uint8_t rotation)
    {
        size_t i = 0;
        for (const auto& piece : pieces)
        {
            if (piece.hasFlags(World::TrackData::PreviewTrackFlags::diagonal))
            {
                continue;
            }
            _mapSelectedTiles[i++] = origin + Math::Vector::rotate(World::Pos2{ piece.x, piece.y }, rotation);
        }

        _mapSelectedTiles[i].x = -1;
        mapInvalidateMapSelectionTiles();
    }

    static void activateSelectedRoadWidgets(Window* window)
    {
        World::mapInvalidateMapSelectionTiles();
        World::setMapSelectionFlags(World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::unk_03);

        auto road = getRoadPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);

        uint8_t rotation;
        uint8_t roadId;

        const uint16_t x = _cState->x;
        const uint16_t y = _cState->y;

        if (!road)
        {
            rotation = _cState->constructionRotation;
            roadId = 0;
        }
        else
        {
            rotation = road->rotation;
            roadId = road->id;
        }

        const auto& roadPiece = World::TrackData::getRoadPiece(roadId);
        rotation &= 3;

        setMapSelectedTilesFromPiece(roadPiece, World::Pos2(x, y), rotation);
        window->holdableWidgets = (1 << widx::construct) | (1 << widx::remove);

        auto trackType = _cState->trackType & ~(1 << 7);
        auto roadObj = ObjectManager::get<RoadObject>(trackType);

        window->widgets[widx::s_bend_left].type = WidgetType::none;
        window->widgets[widx::s_bend_right].type = WidgetType::none;
        window->widgets[widx::left_hand_curve_large].type = WidgetType::none;
        window->widgets[widx::right_hand_curve_large].type = WidgetType::none;
        window->widgets[widx::left_hand_curve].type = WidgetType::none;
        window->widgets[widx::right_hand_curve].type = WidgetType::none;
        window->widgets[widx::left_hand_curve_small].type = WidgetType::none;
        window->widgets[widx::right_hand_curve_small].type = WidgetType::none;
        window->widgets[widx::left_hand_curve_very_small].type = WidgetType::none;
        window->widgets[widx::right_hand_curve_very_small].type = WidgetType::none;

        window->widgets[widx::left_hand_curve_small].left = 3;
        window->widgets[widx::left_hand_curve_small].right = 24;
        window->widgets[widx::right_hand_curve_small].left = 113;
        window->widgets[widx::right_hand_curve_small].right = 134;
        window->widgets[widx::left_hand_curve].left = 25;
        window->widgets[widx::left_hand_curve].right = 46;
        window->widgets[widx::right_hand_curve].left = 91;
        window->widgets[widx::right_hand_curve].right = 112;

        if (roadObj->hasTraitFlags(World::Track::RoadTraitFlags::verySmallCurve))
        {
            window->widgets[widx::left_hand_curve_small].left = 25;
            window->widgets[widx::left_hand_curve_small].right = 46;
            window->widgets[widx::right_hand_curve_small].left = 91;
            window->widgets[widx::right_hand_curve_small].right = 112;
            window->widgets[widx::left_hand_curve].left = 47;
            window->widgets[widx::left_hand_curve].right = 68;
            window->widgets[widx::right_hand_curve].left = 69;
            window->widgets[widx::right_hand_curve].right = 90;

            window->widgets[widx::left_hand_curve_very_small].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve_very_small].type = WidgetType::buttonWithImage;
        }

        if (roadObj->hasTraitFlags(World::Track::RoadTraitFlags::smallCurve))
        {
            window->widgets[widx::left_hand_curve_small].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve_small].type = WidgetType::buttonWithImage;
        }

        window->widgets[widx::s_bend_dual_track_left].type = WidgetType::none;
        window->widgets[widx::s_bend_dual_track_right].type = WidgetType::none;

        if (roadObj->hasTraitFlags(World::Track::RoadTraitFlags::turnaround))
        {
            window->widgets[widx::s_bend_dual_track_left].type = WidgetType::buttonWithImage;
            window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_right_turnaround;
            window->widgets[widx::s_bend_dual_track_left].tooltip = StringIds::tooltip_turnaround;

            if (getGameState().trafficHandedness == 0)
                window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_left_turnaround;
        }

        window->widgets[widx::steep_slope_down].type = WidgetType::none;
        window->widgets[widx::slope_down].type = WidgetType::none;
        window->widgets[widx::slope_up].type = WidgetType::none;
        window->widgets[widx::steep_slope_up].type = WidgetType::none;

        if (roadObj->hasTraitFlags(World::Track::RoadTraitFlags::slope))
        {
            window->widgets[widx::slope_down].type = WidgetType::buttonWithImage;
            window->widgets[widx::slope_up].type = WidgetType::buttonWithImage;
        }

        if (roadObj->hasTraitFlags(World::Track::RoadTraitFlags::steepSlope))
        {
            window->widgets[widx::steep_slope_down].type = WidgetType::buttonWithImage;
            window->widgets[widx::steep_slope_up].type = WidgetType::buttonWithImage;
        }

        window->widgets[widx::bridge].type = WidgetType::combobox;
        window->widgets[widx::bridge_dropdown].type = WidgetType::button;

        if (_cState->lastSelectedBridge == 0xFF || (_cState->constructionHover != 1 && !(_cState->byte_1136076 & 1)))
        {
            window->widgets[widx::bridge].type = WidgetType::none;
            window->widgets[widx::bridge_dropdown].type = WidgetType::none;
        }

        auto activatedWidgets = window->activatedWidgets;
        activatedWidgets &= ~(Construction::allTrack);

        window->widgets[widx::construct].type = WidgetType::none;
        window->widgets[widx::remove].type = WidgetType::buttonWithImage;
        window->widgets[widx::rotate_90].type = WidgetType::none;

        if (_cState->constructionHover == 1)
        {
            window->widgets[widx::construct].type = WidgetType::wt_6;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_start_construction;
            window->widgets[widx::remove].type = WidgetType::none;
            window->widgets[widx::rotate_90].type = WidgetType::buttonWithImage;
            window->widgets[widx::rotate_90].image = ImageIds::rotate_object;
            window->widgets[widx::rotate_90].tooltip = StringIds::rotate_90;
        }
        else if (_cState->constructionHover == 0)
        {
            window->widgets[widx::construct].type = WidgetType::wt_3;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_construct;
            window->widgets[widx::rotate_90].type = WidgetType::buttonWithImage;
            window->widgets[widx::rotate_90].image = ImageIds::construction_new_position;
            window->widgets[widx::rotate_90].tooltip = StringIds::new_construction_position;
        }
        if (_cState->constructionHover == 0 || _cState->constructionHover == 1)
        {
            if (_cState->lastSelectedTrackPiece != 0xFF)
            {
                auto trackPieceWidget = trackPieceWidgets[_cState->lastSelectedTrackPiece];
                activatedWidgets |= 1ULL << trackPieceWidget;
            }

            uint8_t trackGradient = widx::level;

            switch (_cState->lastSelectedTrackGradient)
            {
                case TrackGradient::level:
                    trackGradient = widx::level;
                    break;

                case TrackGradient::slope_up:
                    trackGradient = widx::slope_up;
                    break;

                case TrackGradient::slope_down:
                    trackGradient = widx::slope_down;
                    break;

                case TrackGradient::steep_slope_up:
                    trackGradient = widx::steep_slope_up;
                    break;

                case TrackGradient::steep_slope_down:
                    trackGradient = widx::steep_slope_down;
                    break;
            }

            activatedWidgets |= 1ULL << trackGradient;
        }
        window->activatedWidgets = activatedWidgets;
        window->invalidate();
    }

    static void activateSelectedTrackWidgets(Window* window)
    {
        World::mapInvalidateMapSelectionTiles();
        World::setMapSelectionFlags(World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::unk_03);

        auto track = getTrackPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);

        uint8_t rotation;
        uint8_t trackId;
        const uint16_t x = _cState->x;
        const uint16_t y = _cState->y;

        if (!track)
        {
            rotation = _cState->constructionRotation;
            trackId = 0;
        }
        else
        {
            rotation = track->rotation;
            trackId = track->id;
        }

        const auto& trackPiece = World::TrackData::getTrackPiece(trackId);
        rotation &= 3;

        setMapSelectedTilesFromPiece(trackPiece, World::Pos2(x, y), rotation);
        window->holdableWidgets = (1 << widx::construct) | (1 << widx::remove);

        auto trackObj = ObjectManager::get<TrackObject>(_cState->trackType);

        window->widgets[widx::s_bend_left].type = WidgetType::buttonWithImage;
        window->widgets[widx::s_bend_right].type = WidgetType::buttonWithImage;
        window->widgets[widx::left_hand_curve_large].type = WidgetType::none;
        window->widgets[widx::right_hand_curve_large].type = WidgetType::none;
        window->widgets[widx::left_hand_curve].type = WidgetType::none;
        window->widgets[widx::right_hand_curve].type = WidgetType::none;
        window->widgets[widx::left_hand_curve_small].type = WidgetType::none;
        window->widgets[widx::right_hand_curve_small].type = WidgetType::none;
        window->widgets[widx::left_hand_curve_very_small].type = WidgetType::none;
        window->widgets[widx::right_hand_curve_very_small].type = WidgetType::none;

        window->widgets[widx::left_hand_curve_small].left = 3;
        window->widgets[widx::left_hand_curve_small].right = 24;
        window->widgets[widx::right_hand_curve_small].left = 113;
        window->widgets[widx::right_hand_curve_small].right = 134;
        window->widgets[widx::left_hand_curve].left = 25;
        window->widgets[widx::left_hand_curve].right = 46;
        window->widgets[widx::right_hand_curve].left = 91;
        window->widgets[widx::right_hand_curve].right = 112;

        if (trackObj->hasTraitFlags(Track::TrackTraitFlags::verySmallCurve))
        {
            window->widgets[widx::left_hand_curve_small].left = 25;
            window->widgets[widx::left_hand_curve_small].right = 46;
            window->widgets[widx::right_hand_curve_small].left = 91;
            window->widgets[widx::right_hand_curve_small].right = 112;
            window->widgets[widx::left_hand_curve].left = 47;
            window->widgets[widx::left_hand_curve].right = 68;
            window->widgets[widx::right_hand_curve].left = 69;
            window->widgets[widx::right_hand_curve].right = 90;

            window->widgets[widx::left_hand_curve_very_small].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve_very_small].type = WidgetType::buttonWithImage;
        }

        if (trackObj->hasTraitFlags(Track::TrackTraitFlags::largeCurve))
        {
            window->widgets[widx::left_hand_curve_large].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve_large].type = WidgetType::buttonWithImage;
        }

        if (trackObj->hasTraitFlags(Track::TrackTraitFlags::normalCurve))
        {
            window->widgets[widx::left_hand_curve].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve].type = WidgetType::buttonWithImage;
        }

        if (trackObj->hasTraitFlags(Track::TrackTraitFlags::smallCurve))
        {
            window->widgets[widx::left_hand_curve_small].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve_small].type = WidgetType::buttonWithImage;
        }

        window->widgets[widx::s_bend_dual_track_left].type = WidgetType::none;
        window->widgets[widx::s_bend_dual_track_right].type = WidgetType::none;

        if (trackObj->hasTraitFlags(Track::TrackTraitFlags::oneSided))
        {
            window->widgets[widx::s_bend_dual_track_left].type = WidgetType::buttonWithImage;
            window->widgets[widx::s_bend_dual_track_right].type = WidgetType::buttonWithImage;
            window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_s_bend_dual_track_left;
            window->widgets[widx::s_bend_dual_track_right].image = ImageIds::construction_s_bend_dual_track_right;
            window->widgets[widx::s_bend_dual_track_left].tooltip = StringIds::tooltip_s_bend_left_dual_track;
            window->widgets[widx::s_bend_dual_track_right].tooltip = StringIds::tooltip_s_bend_right_dual_track;

            _byte_522090 = 16;
            _byte_522091 = 20;

            if (_cState->constructionRotation >= 4 && _cState->constructionRotation < 12)
            {
                window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_right_turnaround;
                window->widgets[widx::s_bend_dual_track_right].image = ImageIds::construction_s_bend_to_single_track_left;
                window->widgets[widx::s_bend_dual_track_left].tooltip = StringIds::tooltip_turnaround;
                window->widgets[widx::s_bend_dual_track_right].tooltip = StringIds::tooltip_s_bend_to_single_track;
                _byte_522090 = 20;
                _byte_522092 = 16;
                if (_cState->constructionRotation >= 8)
                {
                    window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_s_bend_to_single_track_right;
                    window->widgets[widx::s_bend_dual_track_right].image = ImageIds::construction_left_turnaround;
                    window->widgets[widx::s_bend_dual_track_left].tooltip = StringIds::tooltip_s_bend_to_single_track;
                    window->widgets[widx::s_bend_dual_track_right].tooltip = StringIds::tooltip_turnaround;
                    _byte_522091 = 16;
                    _byte_522092 = 20;
                }
            }
        }
        window->widgets[widx::steep_slope_down].type = WidgetType::none;
        window->widgets[widx::slope_down].type = WidgetType::none;
        window->widgets[widx::slope_up].type = WidgetType::none;
        window->widgets[widx::steep_slope_up].type = WidgetType::none;

        if (trackObj->hasTraitFlags(Track::TrackTraitFlags::slope))
        {
            window->widgets[widx::slope_down].type = WidgetType::buttonWithImage;
            window->widgets[widx::slope_up].type = WidgetType::buttonWithImage;
        }

        if (trackObj->hasTraitFlags(Track::TrackTraitFlags::steepSlope))
        {
            window->widgets[widx::steep_slope_down].type = WidgetType::buttonWithImage;
            window->widgets[widx::steep_slope_up].type = WidgetType::buttonWithImage;
        }

        window->widgets[widx::bridge].type = WidgetType::combobox;
        window->widgets[widx::bridge_dropdown].type = WidgetType::button;

        if (_cState->lastSelectedBridge == 0xFF || (_cState->constructionHover != 1 && !(_cState->byte_1136076 & 1)))
        {
            window->widgets[widx::bridge].type = WidgetType::none;
            window->widgets[widx::bridge_dropdown].type = WidgetType::none;
        }

        auto activatedWidgets = window->activatedWidgets;
        activatedWidgets &= ~(Construction::allTrack);

        window->widgets[widx::construct].type = WidgetType::none;
        window->widgets[widx::remove].type = WidgetType::buttonWithImage;
        window->widgets[widx::rotate_90].type = WidgetType::none;

        if (_cState->constructionHover == 1)
        {
            window->widgets[widx::construct].type = WidgetType::wt_6;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_start_construction;
            window->widgets[widx::remove].type = WidgetType::none;
            window->widgets[widx::rotate_90].type = WidgetType::buttonWithImage;
            window->widgets[widx::rotate_90].image = ImageIds::rotate_object;
            window->widgets[widx::rotate_90].tooltip = StringIds::rotate_90;
        }
        else if (_cState->constructionHover == 0)
        {
            window->widgets[widx::construct].type = WidgetType::wt_3;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_construct;
            window->widgets[widx::rotate_90].type = WidgetType::buttonWithImage;
            window->widgets[widx::rotate_90].image = ImageIds::construction_new_position;
            window->widgets[widx::rotate_90].tooltip = StringIds::new_construction_position;
        }
        if (_cState->constructionHover == 0 || _cState->constructionHover == 1)
        {
            if (_cState->lastSelectedTrackPiece != 0xFF)
            {
                auto trackPieceWidget = trackPieceWidgets[_cState->lastSelectedTrackPiece];
                activatedWidgets |= 1ULL << trackPieceWidget;
            }

            uint8_t trackGradient = widx::level;

            switch (_cState->lastSelectedTrackGradient)
            {
                case TrackGradient::level:
                    trackGradient = widx::level;
                    break;

                case TrackGradient::slope_up:
                    trackGradient = widx::slope_up;
                    break;

                case TrackGradient::slope_down:
                    trackGradient = widx::slope_down;
                    break;

                case TrackGradient::steep_slope_up:
                    trackGradient = widx::steep_slope_up;
                    break;

                case TrackGradient::steep_slope_down:
                    trackGradient = widx::steep_slope_down;
                    break;
            }

            activatedWidgets |= 1ULL << trackGradient;
        }
        window->activatedWidgets = activatedWidgets;
        window->invalidate();
    }

    // 0x0049F1B5
    void activateSelectedConstructionWidgets()
    {
        auto window = WindowManager::find(WindowType::construction);

        if (window == nullptr)
            return;

        if (window->currentTab == Common::widx::tab_construction - Common::widx::tab_construction)
        {
            if (_cState->trackType & (1 << 7))
            {
                activateSelectedRoadWidgets(window);
            }
            else
            {
                activateSelectedTrackWidgets(window);
            }
        }
    }

    // 0x004A0832
    static std::optional<TrackPieceId> getRoadPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation)
    {
        if (trackPiece == 0xFF)
            return std::nullopt;

        uint8_t id = 0;

        switch (trackPiece)
        {
            case TrackPiece::straight: // 0x004A0856
            {
                if (rotation >= 4)
                    return std::nullopt;
                switch (gradient)
                {
                    default:
                        return std::nullopt;
                    case TrackGradient::level:
                        id = 0;
                        break;
                    case TrackGradient::slope_up:
                        id = 5;
                        break;
                    case TrackGradient::slope_down:
                        id = 6;
                        break;
                    case TrackGradient::steep_slope_up:
                        id = 7;
                        break;
                    case TrackGradient::steep_slope_down:
                        id = 8;
                        break;
                }
                break;
            }
            case TrackPiece::left_hand_curve_very_small: // 0x004A08A5
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 4)
                    return std::nullopt;
                id = 1;
                break;
            }
            case TrackPiece::right_hand_curve_very_small: // 0x004A08CD
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 4)
                    return std::nullopt;
                id = 2;
                break;
            }
            case TrackPiece::left_hand_curve_small: // 0x004A08ED
            {
                if (rotation >= 4)
                    return std::nullopt;
                id = 3;
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                break;
            }
            case TrackPiece::right_hand_curve_small: // 0x004A08FB
            {
                if (rotation >= 4)
                    return std::nullopt;
                id = 4;
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                break;
            }
            case TrackPiece::left_hand_curve: // 0x004A095F
            case TrackPiece::right_hand_curve:
            case TrackPiece::left_hand_curve_large:
            case TrackPiece::right_hand_curve_large:
            case TrackPiece::s_bend_left:
            case TrackPiece::s_bend_right:
            case TrackPiece::s_bend_to_dual_track:
            case TrackPiece::s_bend_to_single_track:
            {
                return std::nullopt;
            }
            case TrackPiece::turnaround: // 0x004A0909
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 12)
                    return std::nullopt;
                id = 9;
                break;
            }
        }

        if (rotation < 12)
            rotation &= 3;

        return TrackPieceId{ id, rotation };
    }

    // 0x004A04F8
    std::optional<TrackPieceId> getTrackPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation)
    {
        if (trackPiece == 0xFF)
            return std::nullopt;

        uint8_t id = 0;

        switch (trackPiece)
        {
            case TrackPiece::straight: // loc_4A051C
            {
                if (rotation >= 12)
                {
                    id = 1;
                    if (gradient != TrackGradient::level)
                        return std::nullopt;
                }
                else
                {
                    if (rotation >= 8)
                    {
                        switch (gradient)
                        {
                            default:
                                return std::nullopt;
                            case TrackGradient::level:
                                id = 27;
                                break;
                            case TrackGradient::steep_slope_up:
                                id = 35;
                                break;
                            case TrackGradient::steep_slope_down:
                                id = 37;
                                break;
                        }
                    }
                    else if (rotation >= 4)
                    {
                        switch (gradient)
                        {
                            default:
                                return std::nullopt;
                            case TrackGradient::level:
                                id = 26;
                                break;
                            case TrackGradient::steep_slope_up:
                                id = 34;
                                break;
                            case TrackGradient::steep_slope_down:
                                id = 36;
                                break;
                        }
                    }
                    else
                    {
                        switch (gradient)
                        {
                            default:
                                return std::nullopt;
                            case TrackGradient::level:
                                id = 0;
                                break;
                            case TrackGradient::slope_up:
                                id = 14;
                                break;
                            case TrackGradient::slope_down:
                                id = 15;
                                break;
                            case TrackGradient::steep_slope_up:
                                id = 16;
                                break;
                            case TrackGradient::steep_slope_down:
                                id = 17;
                                break;
                        }
                    }
                }
                break;
            }

            case TrackPiece::left_hand_curve_very_small: // loc_4A05C3
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 12)
                    return std::nullopt;
                if (rotation >= 8)
                {
                    id = 29;
                    break;
                }
                if (rotation >= 4)
                {
                    id = 28;
                    break;
                }
                id = 2;
                break;
            }

            case TrackPiece::right_hand_curve_very_small: // loc_4A05F4
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 12)
                    return std::nullopt;
                if (rotation >= 8)
                {
                    id = 31;
                    break;
                }
                if (rotation >= 4)
                {
                    id = 30;
                    break;
                }
                id = 3;
                break;
            }

            case TrackPiece::left_hand_curve_small: // loc_4A0625
            {
                if (rotation >= 4)
                    return std::nullopt;
                switch (gradient)
                {
                    default:
                        return std::nullopt;
                    case TrackGradient::level:
                        id = 4;
                        break;
                    case TrackGradient::slope_up:
                        id = 18;
                        break;
                    case TrackGradient::slope_down:
                        id = 20;
                        break;
                    case TrackGradient::steep_slope_up:
                        id = 22;
                        break;
                    case TrackGradient::steep_slope_down:
                        id = 24;
                        break;
                }
                break;
            }

            case TrackPiece::right_hand_curve_small: // loc_4A066A
            {
                if (rotation >= 4)
                    return std::nullopt;
                switch (gradient)
                {
                    default:
                        return std::nullopt;
                    case TrackGradient::level:
                        id = 5;
                        break;
                    case TrackGradient::slope_up:
                        id = 19;
                        break;
                    case TrackGradient::slope_down:
                        id = 21;
                        break;
                    case TrackGradient::steep_slope_up:
                        id = 23;
                        break;
                    case TrackGradient::steep_slope_down:
                        id = 25;
                        break;
                }
                break;
            }

            case TrackPiece::left_hand_curve: // loc_4A06AF
            {
                if (rotation >= 4)
                    return std::nullopt;
                id = 6;
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                break;
            }

            case TrackPiece::right_hand_curve: // loc_4A06C8
            {
                if (rotation >= 4)
                    return std::nullopt;
                id = 7;
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                break;
            }

            case TrackPiece::left_hand_curve_large: // loc_4A06E1
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                id = 10;
                if (rotation >= 12)
                    break;
                if (rotation >= 4)
                    return std::nullopt;
                id = 8;
                break;
            }

            case TrackPiece::right_hand_curve_large: // loc_4A0705
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                id = 11;
                if (rotation >= 12)
                    break;
                if (rotation >= 4)
                    return std::nullopt;
                id = 9;
                break;
            }

            case TrackPiece::s_bend_left: // loc_4A0729
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 12)
                    return std::nullopt;
                id = 33;
                if (rotation >= 8)
                    break;
                if (rotation >= 4)
                    return std::nullopt;
                id = 12;
                break;
            }

            case TrackPiece::s_bend_right: // loc_4A0756
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 8)
                    return std::nullopt;
                id = 32;
                if (rotation >= 4)
                    break;
                id = 13;
                break;
            }

            case TrackPiece::s_bend_to_dual_track: // loc_4A077C
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 8)
                    return std::nullopt;
                id = 40;
                if (rotation >= 4)
                    break;
                id = 38;
                break;
            }

            case TrackPiece::s_bend_to_single_track: // loc_4A07A2
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 12)
                    return std::nullopt;
                id = 41;
                if (rotation >= 8)
                    break;
                if (rotation >= 4)
                    return std::nullopt;
                id = 39;
                break;
            }

            case TrackPiece::turnaround: // loc_4A07C0
            {
                if (gradient != TrackGradient::level)
                    return std::nullopt;
                if (rotation >= 12)
                    return std::nullopt;
                id = 43;
                if (rotation >= 8)
                    break;
                id = 42;
                if (rotation >= 4)
                    return std::nullopt;
                break;
            }
        }

        if (rotation < 12)
            rotation &= 3;

        return TrackPieceId{ id, rotation };
    }

    // 0x0049DAA5
    static void onResize(Window& self)
    {
        self.enabledWidgets &= ~(1 << widx::construct);

        if (_cState->constructionHover != 1)
            self.enabledWidgets |= (1 << widx::construct);

        auto disabledWidgets = self.disabledWidgets;
        disabledWidgets &= (1 << Common::widx::tab_construction | 1 << Common::widx::tab_overhead | 1 << Common::widx::tab_signal | 1 << Common::widx::tab_station);
        uint8_t trackType = _cState->trackType;

        if (trackType & (1 << 7))
        {
            trackType &= ~(1 << 7);

            if (_cState->lastSelectedTrackPiece == 0xFF)
            {
                disableUnusedRoadPieces(&self, disabledWidgets);
                return;
            }
            switch (_cState->lastSelectedTrackPiece)
            {
                case TrackPiece::straight:
                case TrackPiece::left_hand_curve:
                case TrackPiece::right_hand_curve:
                case TrackPiece::left_hand_curve_large:
                case TrackPiece::right_hand_curve_large:
                case TrackPiece::s_bend_left:
                case TrackPiece::s_bend_right:
                case TrackPiece::s_bend_to_dual_track:
                case TrackPiece::s_bend_to_single_track:
                {
                    disableUnusedRoadPieces(&self, disabledWidgets);
                    break;
                }

                case TrackPiece::left_hand_curve_very_small:
                case TrackPiece::right_hand_curve_very_small:
                case TrackPiece::left_hand_curve_small:
                case TrackPiece::right_hand_curve_small:
                case TrackPiece::turnaround:
                {
                    disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
                    disableUnusedRoadPieces(&self, disabledWidgets);
                    break;
                }
            }
        }
        else
        {
            auto trackObj = ObjectManager::get<TrackObject>(trackType);
            if (_cState->lastSelectedTrackPiece == 0xFF)
            {
                disableUnusedTrackPieces(&self, *trackObj, disabledWidgets);
                return;
            }
            switch (_cState->lastSelectedTrackPiece)
            {
                case TrackPiece::straight:
                    disableUnusedTrackPieces(&self, *trackObj, disabledWidgets);
                    break;

                case TrackPiece::left_hand_curve_very_small:
                case TrackPiece::right_hand_curve_very_small:
                case TrackPiece::left_hand_curve:
                case TrackPiece::right_hand_curve:
                case TrackPiece::left_hand_curve_large:
                case TrackPiece::right_hand_curve_large:
                case TrackPiece::s_bend_left:
                case TrackPiece::s_bend_right:
                case TrackPiece::s_bend_to_dual_track:
                case TrackPiece::s_bend_to_single_track:
                case TrackPiece::turnaround:
                {
                    disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
                    disableUnusedTrackPieces(&self, *trackObj, disabledWidgets);
                    break;
                }

                case TrackPiece::left_hand_curve_small:
                case TrackPiece::right_hand_curve_small:
                {
                    disableTrackSlopes(&self, *trackObj, disabledWidgets);
                    break;
                }
            }
        }
    }

    // 0x0049d600 (based on)
    static void changeTrackPiece(uint8_t trackPiece, bool slope)
    {
        _cState->byte_113603A = 0xFF;
        removeConstructionGhosts();

        if (slope)
            _cState->lastSelectedTrackGradient = trackPiece;
        else
            _cState->lastSelectedTrackPiece = trackPiece;

        _cState->trackCost = 0x80000000;
        activateSelectedConstructionWidgets();
    }

    // 0x0049D83A
    static void bridgeDropdown(Window* self)
    {
        auto bridgeCount = 0;
        for (; bridgeCount < 9; bridgeCount++)
        {
            if (_cState->bridgeList[bridgeCount] == 0xFF)
                break;
        }

        uint8_t flags = (1 << 7) | (1 << 6);
        auto widget = self->widgets[widx::bridge];
        auto x = widget.left + self->x;
        auto y = widget.top + self->y;
        auto width = 155;
        auto height = widget.height();

        Dropdown::show(x, y, width, height, self->getColour(WindowColour::secondary), bridgeCount, 22, flags);
        for (auto i = 0; i < 9; i++)
        {
            auto bridge = _cState->bridgeList[i];

            if (bridge == 0xFF)
                return;

            if (bridge == _cState->lastSelectedBridge)
                Dropdown::setHighlightedItem(i);

            auto bridgeObj = ObjectManager::get<BridgeObject>(bridge);
            auto company = CompanyManager::getPlayerCompany();
            auto companyColour = company->mainColours.primary;
            auto imageId = Gfx::recolour(bridgeObj->image, companyColour);

            auto args = FormatArguments();
            args.push(imageId);

            if (bridgeObj->maxSpeed == kSpeed16Null)
            {
                args.push(StringIds::unlimited_speed);
                args.push<uint16_t>(0);
            }
            else
            {
                args.push(StringIds::velocity);
                args.push(bridgeObj->maxSpeed);
            }
            args.push<uint16_t>(bridgeObj->maxHeight);

            Dropdown::add(i, StringIds::dropdown_bridge_stats, args);
        }
    }

    // 0x0049D42F
    static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::left_hand_curve:
                changeTrackPiece(TrackPiece::left_hand_curve, false);
                break;

            case widx::right_hand_curve:
                changeTrackPiece(TrackPiece::right_hand_curve, false);
                break;

            case widx::left_hand_curve_small:
                changeTrackPiece(TrackPiece::left_hand_curve_small, false);
                break;

            case widx::right_hand_curve_small:
                changeTrackPiece(TrackPiece::right_hand_curve_small, false);
                break;

            case widx::left_hand_curve_very_small:
                changeTrackPiece(TrackPiece::left_hand_curve_very_small, false);
                break;

            case widx::right_hand_curve_very_small:
                changeTrackPiece(TrackPiece::right_hand_curve_very_small, false);
                break;

            case widx::left_hand_curve_large:
                changeTrackPiece(TrackPiece::left_hand_curve_large, false);
                break;

            case widx::right_hand_curve_large:
                changeTrackPiece(TrackPiece::right_hand_curve_large, false);
                break;

            case widx::straight:
                changeTrackPiece(TrackPiece::straight, false);
                break;

            case widx::s_bend_left:
                changeTrackPiece(TrackPiece::s_bend_left, false);
                break;

            case widx::s_bend_right:
                changeTrackPiece(TrackPiece::s_bend_right, false);
                break;

            case widx::s_bend_dual_track_left:
            {
                _cState->byte_113603A = 0xFF;
                removeConstructionGhosts();
                _cState->trackCost = 0x80000000;

                if (self.widgets[widx::s_bend_dual_track_left].image == ImageIds::construction_s_bend_dual_track_left)
                    _cState->lastSelectedTrackPiece = TrackPiece::s_bend_to_dual_track;
                else if (self.widgets[widx::s_bend_dual_track_left].image == ImageIds::construction_left_turnaround || self.widgets[widx::s_bend_dual_track_left].image == ImageIds::construction_right_turnaround)
                    _cState->lastSelectedTrackPiece = TrackPiece::turnaround;
                else
                    _cState->lastSelectedTrackPiece = TrackPiece::s_bend_to_single_track;

                activateSelectedConstructionWidgets();
                break;
            }

            case widx::s_bend_dual_track_right:
            {
                _cState->byte_113603A = 0xFF;
                removeConstructionGhosts();
                _cState->trackCost = 0x80000000;

                if (self.widgets[widx::s_bend_dual_track_right].image == ImageIds::construction_s_bend_dual_track_right)
                    _cState->lastSelectedTrackPiece = TrackPiece::s_bend_to_dual_track;
                else if (self.widgets[widx::s_bend_dual_track_right].image == ImageIds::construction_left_turnaround)
                    _cState->lastSelectedTrackPiece = TrackPiece::turnaround;
                else
                    _cState->lastSelectedTrackPiece = TrackPiece::s_bend_to_single_track;

                activateSelectedConstructionWidgets();
                break;
            }

            case widx::steep_slope_down:
                changeTrackPiece(TrackGradient::steep_slope_down, true);
                break;

            case widx::slope_down:
                changeTrackPiece(TrackGradient::slope_down, true);
                break;

            case widx::level:
                changeTrackPiece(TrackGradient::level, true);
                break;

            case widx::slope_up:
                changeTrackPiece(TrackGradient::slope_up, true);
                break;

            case widx::steep_slope_up:
                changeTrackPiece(TrackGradient::steep_slope_up, true);
                break;

            case widx::bridge_dropdown:
            {
                bridgeDropdown(&self);
                break;
            }

            case widx::construct:
            {
                if (Input::getClickRepeatTicks() >= 40)
                    constructTrack(&self, widgetIndex);
                break;
            }

            case widx::remove:
            {
                if (Input::getClickRepeatTicks() >= 40)
                    removeTrack(&self, widgetIndex);
                break;
            }
        }
    }

    // 0x0049D4EA
    static void onDropdown([[maybe_unused]] Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex == widx::bridge_dropdown)
        {
            if (itemIndex != -1)
            {
                auto bridge = _cState->bridgeList[itemIndex];
                _cState->lastSelectedBridge = bridge;

                // TODO: & ~(1 << 7) added to prevent crashing when selecting bridges for road/trams
                Scenario::getConstruction().bridges[_cState->trackType & ~(1 << 7)] = bridge;
                removeConstructionGhosts();
                _cState->trackCost = 0x80000000;
                activateSelectedConstructionWidgets();
            }
        }
    }

    static std::optional<GameCommands::RoadPlacementArgs> getRoadPlacementArgs(const World::Pos3& pos, const uint8_t trackPiece, const uint8_t gradient, const uint8_t rotation);
    static std::optional<GameCommands::TrackPlacementArgs> getTrackPlacementArgs(const World::Pos3& pos, const uint8_t trackPiece, const uint8_t gradient, const uint8_t rotation);
    static uint32_t placeRoadGhost(const GameCommands::RoadPlacementArgs& args);
    static uint32_t placeTrackGhost(const GameCommands::TrackPlacementArgs& args);
    static void sub_4A193B();

    static void updateConstructionArrow()
    {
        _cState->constructionArrowFrameNum = _cState->constructionArrowFrameNum - 1;
        if (_cState->constructionArrowFrameNum == 0xFF)
        {
            _cState->constructionArrowFrameNum = 5;
            _ghostVisibilityFlags = _ghostVisibilityFlags ^ GhostVisibilityFlags::constructArrow;
            _constructionArrowPos = World::Pos3(_cState->x, _cState->y, _cState->constructionZ);
            _constructionArrowDirection = _cState->constructionRotation;
            World::resetMapSelectionFlag(World::MapSelectionFlags::enableConstructionArrow);
            if ((_ghostVisibilityFlags & GhostVisibilityFlags::constructArrow) != GhostVisibilityFlags::none)
            {
                World::setMapSelectionFlags(World::MapSelectionFlags::enableConstructionArrow);
            }
            World::TileManager::mapInvalidateTileFull(World::Pos2(_cState->x, _cState->y));
        }
    }

    // 0x0049FD66
    // Places/removes the ghost, gets price, resets active widgets, updates the construction arrow
    // Ideally only construction arrow should be updated here the reason for doing the rest every update
    // instead of only in toolUpdate is unknown (but probably just to catch edge cases)
    static void updateConstruction()
    {
        if (_cState->constructionHover != 0)
        {
            return;
        }
        if ((_ghostVisibilityFlags & GhostVisibilityFlags::track) == GhostVisibilityFlags::none)
        {
            if (_cState->trackType & (1 << 7))
            {

                auto args = getRoadPlacementArgs(World::Pos3(_cState->x, _cState->y, _cState->constructionZ), _cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);
                if (args)
                {
                    _cState->trackCost = placeRoadGhost(*args);
                    _cState->byte_1136076 = _cState->byte_1136073;
                    sub_4A193B();
                    activateSelectedConstructionWidgets();
                }
                else
                {
                    removeTrackGhosts();
                }
            }
            else
            {
                auto args = getTrackPlacementArgs(World::Pos3(_cState->x, _cState->y, _cState->constructionZ), _cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);
                if (args)
                {
                    _cState->trackCost = placeTrackGhost(*args);
                    _cState->byte_1136076 = _cState->byte_1136073;
                    sub_4A193B();
                    activateSelectedConstructionWidgets();
                }
                else
                {
                    removeTrackGhosts();
                }
            }
        }
        updateConstructionArrow();
    }

    // 0x0049DCA2
    static void onUpdate(Window& self)
    {
        self.frameNo++;
        self.callPrepareDraw();
        WindowManager::invalidate(WindowType::construction, self.number);

        if (_cState->constructionHover == 1)
        {
            if (!ToolManager::isToolActive(WindowType::construction, self.number) || ToolManager::getToolWidgetIndex() != widx::construct)
                WindowManager::close(&self);
        }
        if (_cState->constructionHover == 0)
        {
            if (ToolManager::isToolActive(WindowType::construction, self.number))
                ToolManager::toolCancel();
        }
        updateConstruction();
    }

    // Simplified TileManager::getHeight that only considers flat height
    static std::optional<World::TileHeight> getConstructionHeight(const Pos2& mapPos)
    {
        auto tile = TileManager::get(mapPos);

        auto surfaceTile = tile.surface();

        if (surfaceTile == nullptr)
            return std::nullopt;

        World::TileHeight height = { static_cast<coord_t>(surfaceTile->baseHeight()), surfaceTile->waterHeight() };
        if (surfaceTile->slopeCorners())
        {
            height.landHeight += 16;
        }

        if (surfaceTile->isSlopeDoubleHeight())
        {
            height.landHeight += 16;
        }
        return { height };
    }

    // 0x00478361
    static std::optional<int16_t> getExistingRoadAtLoc(int16_t x, int16_t y)
    {
        static loco_global<Viewport*, 0x01135F52> _1135F52;

        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::roadAndTram));
        _1135F52 = viewport;

        if (interaction.type != ViewportInteraction::InteractionItem::road)
        {
            return std::nullopt;
        }

        const auto* elTrack = reinterpret_cast<World::TileElement*>(interaction.object)->as<RoadElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        const auto& roadPieces = TrackData::getRoadPiece(elTrack->roadId());
        const auto& roadPiece = roadPieces[elTrack->sequenceIndex()];

        const auto startHeight = elTrack->baseHeight() - roadPiece.z;

        return { startHeight };
    }

    // 0x004A4011
    static std::optional<std::pair<int16_t, int16_t>> getExistingTrackAtLoc(int16_t x, int16_t y)
    {
        static loco_global<Viewport*, 0x01135F52> _1135F52;

        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::track));
        _1135F52 = viewport;

        if (interaction.type != ViewportInteraction::InteractionItem::track)
        {
            return std::nullopt;
        }

        const auto* elTrack = reinterpret_cast<World::TileElement*>(interaction.object)->as<TrackElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        const auto& trackPieces = TrackData::getTrackPiece(elTrack->trackId());
        const auto& trackPiece = trackPieces[elTrack->sequenceIndex()];

        const auto startHeight = elTrack->baseHeight() - trackPiece.z;

        return { std::make_pair(startHeight, elTrack->trackId()) };
    }

    static void constructionLoop(const Pos2& mapPos, uint32_t maxRetries, int16_t height)
    {
        while (true)
        {
            _cState->constructionHover = 0;
            _cState->byte_113607E = 0;
            _cState->x = mapPos.x;
            _cState->y = mapPos.y;
            _cState->constructionZ = height;
            _ghostVisibilityFlags = GhostVisibilityFlags::none;
            _cState->constructionArrowFrameNum = 0;

            activateSelectedConstructionWidgets();
            auto window = WindowManager::find(WindowType::construction);

            // Attempt to place track piece -- in silent
            _suppressErrorSound = true;
            onMouseUp(*window, widx::construct);
            _suppressErrorSound = false;

            if (_cState->dword_1135F42 != 0x80000000)
            {
                _cState->byte_113607E = 1;
                WindowManager::close(WindowType::error, 0);
                return;
            }

            if (GameCommands::getErrorText() != StringIds::error_can_only_build_above_ground)
            {
                maxRetries--;
                if (maxRetries != 0)
                {
                    height -= 16;
                    if (height >= 0)
                    {
                        if (!Input::hasKeyModifier(Input::KeyModifier::shift))
                        {
                            height += 32;
                        }
                        continue;
                    }
                }
            }

            // Failed to place track piece -- rotate and make error sound
            onMouseUp(*window, widx::rotate_90);
            Audio::playSound(Audio::SoundId::error, int32_t(Input::getMouseLocation().x));
            return;
        }
    }

    // 0 if nothing currently selected
    static int16_t getMaxConstructHeightFromExistingSelection()
    {
        int16_t maxHeight = 0;

        if (World::hasMapSelectionFlag(World::MapSelectionFlags::enableConstruct))
        {
            for (const auto& tile : _mapSelectedTiles)
            {
                if (tile.x == -1)
                {
                    break;
                }
                if (!World::validCoords(tile))
                    continue;

                const auto tileHeight = getConstructionHeight(tile);
                if (!tileHeight)
                {
                    continue;
                }

                maxHeight = std::max(tileHeight->landHeight, maxHeight);

                if (tileHeight->waterHeight)
                {
                    // Constructing over water is always +16
                    maxHeight = std::max<int16_t>(tileHeight->waterHeight + 16, maxHeight);
                }
            }
        }
        return maxHeight;
    }

    static std::optional<std::pair<World::TilePos2, int16_t>> tryMakeRoadJunctionAtLoc(const int16_t x, const int16_t y)
    {
        const auto existingRoad = getExistingRoadAtLoc(x, y);

        if (existingRoad)
        {
            const auto& existingHeight = *existingRoad;
            const auto mapPos = screenGetMapXyWithZ(Point(x, y), existingHeight);
            if (mapPos)
            {
                return { std::make_pair(World::toTileSpace(*mapPos), existingHeight) };
            }
        }
        return std::nullopt;
    }

    static std::optional<std::pair<World::TilePos2, int16_t>> tryMakeTrackJunctionAtLoc(const int16_t x, const int16_t y)
    {
        const auto existingTrack = getExistingTrackAtLoc(x, y);

        if (existingTrack)
        {
            const auto [existingHeight, existingTrackId] = *existingTrack;
            if (TrackData::getUnkTrack(existingTrackId << 3).pos.z == 0)
            {
                const auto mapPos = screenGetMapXyWithZ(Point(x, y), existingHeight);
                if (mapPos)
                {
                    return { std::make_pair(World::toTileSpace(*mapPos), existingHeight) };
                }
            }
        }
        return std::nullopt;
    }

    static std::optional<std::pair<World::TilePos2, int16_t>> getConstructionPos(const int16_t x, const int16_t y, const int16_t baseHeight = std::numeric_limits<int16_t>::max())
    {
        auto mapPos = ViewportInteraction::getSurfaceOrWaterLocFromUi({ x, y });

        if (!mapPos)
            return std::nullopt;

        auto tileHeight = getConstructionHeight(*mapPos);
        if (!tileHeight)
        {
            return std::nullopt;
        }
        auto height = std::min(tileHeight->landHeight, baseHeight);
        height = std::max(height, tileHeight->waterHeight);

        return { std::make_pair(World::toTileSpace(*mapPos), height) };
    }

    static int16_t getMaxPieceHeight(const std::span<const TrackData::PreviewTrack> piece)
    {
        int16_t maxPieceHeight = 0;

        for (const auto& part : piece)
        {
            maxPieceHeight = std::max(maxPieceHeight, part.z);
        }
        return maxPieceHeight;
    }

    // 0x004A193B
    static void sub_4A193B()
    {
        for (const auto bridgeType : _cState->bridgeList)
        {
            if (bridgeType == 0xFF)
            {
                return;
            }
            if (_cState->byte_1136075 == bridgeType)
            {
                _cState->lastSelectedBridge = bridgeType;
                return;
            }
        }
    }

    // 0x004A006C
    void removeTrackGhosts()
    {
        if ((_ghostVisibilityFlags & GhostVisibilityFlags::track) != GhostVisibilityFlags::none)
        {
            if (_ghostRemovalTrackObjectId & (1 << 7))
            {
                GameCommands::RoadRemovalArgs args;
                args.pos = _cState->ghostRemovalTrackPos;
                args.pos.z += TrackData::getRoadPiece(_cState->ghostRemovalTrackId)[0].z;
                args.rotation = _cState->ghostRemovalTrackRotation & 3;
                args.sequenceIndex = 0;
                args.roadId = _cState->ghostRemovalTrackId;
                args.objectId = _ghostRemovalTrackObjectId & ~(1 << 7);
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
            else
            {
                GameCommands::TrackRemovalArgs args;
                args.pos = _cState->ghostRemovalTrackPos;
                args.pos.z += TrackData::getTrackPiece(_cState->ghostRemovalTrackId)[0].z;
                args.rotation = _cState->ghostRemovalTrackRotation & 3;
                args.index = 0;
                args.trackId = _cState->ghostRemovalTrackId;
                args.trackObjectId = _ghostRemovalTrackObjectId;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
            }
            _ghostVisibilityFlags = _ghostVisibilityFlags & ~GhostVisibilityFlags::track;
        }
    }

    // 0x0049FB63
    static uint32_t placeTrackGhost(const GameCommands::TrackPlacementArgs& args)
    {
        removeTrackGhosts();
        const auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
        if (res == GameCommands::FAILURE)
        {
            if (GameCommands::getErrorText() == StringIds::bridge_type_unsuitable_for_this_configuration)
            {
                _cState->byte_113603A = 0;
                for (const auto bridgeType : _cState->bridgeList)
                {
                    if (bridgeType == 0xFF)
                    {
                        break;
                    }
                    const auto* bridgeObj = ObjectManager::get<BridgeObject>(bridgeType);
                    if ((bridgeObj->disabledTrackCfg & TrackData::getTrackMiscData(args.trackId).flags) != Track::CommonTraitFlags::none)
                    {
                        continue;
                    }

                    if (bridgeType == _cState->lastSelectedBridge)
                    {
                        break;
                    }

                    auto newArgs(args);
                    newArgs.bridge = bridgeType;
                    _cState->lastSelectedBridge = bridgeType;
                    WindowManager::invalidate(WindowType::construction);
                    return placeTrackGhost(args);
                }
            }
        }
        else
        {
            _cState->ghostRemovalTrackPos = args.pos;
            _cState->ghostRemovalTrackId = args.trackId;
            _ghostRemovalTrackObjectId = args.trackObjectId;
            _cState->ghostRemovalTrackRotation = args.rotation;
            _ghostVisibilityFlags = GhostVisibilityFlags::track | *_ghostVisibilityFlags;
            const auto newViewState = (_cState->byte_1136072 & (1 << 1)) ? WindowManager::ViewportVisibility::undergroundView : WindowManager::ViewportVisibility::overgroundView;
            WindowManager::viewportSetVisibility(newViewState);
            if (_cState->lastSelectedTrackGradient != 0)
            {
                WindowManager::viewportSetVisibility(WindowManager::ViewportVisibility::heightMarksOnTrack);
            }
        }
        _cState->byte_113603A = 0;
        return res;
    }

    // 0x0049FC60
    static uint32_t placeRoadGhost(const GameCommands::RoadPlacementArgs& args)
    {
        removeTrackGhosts();
        const auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::noErrorWindow | GameCommands::Flags::noPayment | GameCommands::Flags::ghost);
        if (res == GameCommands::FAILURE)
        {
            if (GameCommands::getErrorText() == StringIds::bridge_type_unsuitable_for_this_configuration)
            {
                _cState->byte_113603A = 0;
                for (const auto bridgeType : _cState->bridgeList)
                {
                    if (bridgeType == 0xFF)
                    {
                        break;
                    }
                    const auto* bridgeObj = ObjectManager::get<BridgeObject>(bridgeType);
                    if ((bridgeObj->disabledTrackCfg & TrackData::getRoadMiscData(args.roadId).flags) != Track::CommonTraitFlags::none)
                    {
                        continue;
                    }

                    if (bridgeType == _cState->lastSelectedBridge)
                    {
                        break;
                    }

                    auto newArgs(args);
                    newArgs.bridge = bridgeType;
                    _cState->lastSelectedBridge = bridgeType;
                    WindowManager::invalidate(WindowType::construction);
                    return placeRoadGhost(args);
                }
            }
        }
        else
        {
            _cState->ghostRemovalTrackPos = args.pos;
            _cState->ghostRemovalTrackId = args.roadId;
            _ghostRemovalTrackObjectId = args.roadObjectId | (1 << 7);
            _cState->ghostRemovalTrackRotation = args.rotation;
            _ghostVisibilityFlags = GhostVisibilityFlags::track | *_ghostVisibilityFlags;
            const auto newViewState = (_cState->byte_1136072 & (1 << 1)) ? WindowManager::ViewportVisibility::undergroundView : WindowManager::ViewportVisibility::overgroundView;
            WindowManager::viewportSetVisibility(newViewState);
            if (_cState->lastSelectedTrackGradient != 0)
            {
                WindowManager::viewportSetVisibility(WindowManager::ViewportVisibility::heightMarksOnTrack);
            }
        }
        _cState->byte_113603A = 0;
        return res;
    }

    static std::optional<GameCommands::TrackPlacementArgs> getTrackPlacementArgs(const World::Pos3& pos, const uint8_t trackPiece, const uint8_t gradient, const uint8_t rotation)
    {
        auto trackId = getTrackPieceId(trackPiece, gradient, rotation);
        if (!trackId)
        {
            return std::nullopt;
        }
        GameCommands::TrackPlacementArgs args;
        args.pos = pos;
        args.bridge = _cState->lastSelectedBridge;
        args.mods = _cState->lastSelectedMods;
        args.rotation = trackId->rotation;
        args.trackObjectId = _cState->trackType;
        args.trackId = trackId->id;
        args.unk = _cState->byte_113607E & (1 << 0);
        args.unkFlags = 0;
        return args;
    }

    static std::optional<GameCommands::RoadPlacementArgs> getRoadPlacementArgs(const World::Pos3& pos, const uint8_t trackPiece, const uint8_t gradient, const uint8_t rotation)
    {
        auto roadId = getRoadPieceId(trackPiece, gradient, rotation);
        if (!roadId)
        {
            return std::nullopt;
        }
        GameCommands::RoadPlacementArgs args;
        args.pos = pos;
        args.bridge = _cState->lastSelectedBridge;
        args.mods = _cState->lastSelectedMods;
        args.rotation = roadId->rotation;
        args.roadObjectId = _cState->trackType & ~(1 << 7);
        args.roadId = roadId->id;
        args.unkFlags = 0;
        return args;
    }

    template<typename GetPlacementArgsFunc, typename PlaceGhostFunc>
    static void constructionGhostLoop(const Pos3& mapPos, uint32_t maxRetries, GetPlacementArgsFunc&& getPlacementArgs, PlaceGhostFunc&& placeGhost)
    {
        _cState->x = mapPos.x;
        _cState->y = mapPos.y;
        _cState->constructionZ = mapPos.z;
        if ((_ghostVisibilityFlags & GhostVisibilityFlags::track) != GhostVisibilityFlags::none)
        {
            if (_cState->ghostTrackPos == mapPos)
            {
                return;
            }
        }
        _cState->ghostTrackPos = mapPos;

        auto args = getPlacementArgs(mapPos, _cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);
        if (!args)
        {
            return;
        }
        while (true)
        {

            auto res = placeGhost(*args);
            _cState->trackCost = res;
            _cState->byte_1136076 = _cState->byte_1136073;
            sub_4A193B();

            if (_cState->trackCost == 0x80000000)
            {
                maxRetries--;
                if (maxRetries != 0)
                {
                    args->pos.z -= 16;
                    if (args->pos.z >= 0)
                    {
                        if (Input::hasKeyModifier(Input::KeyModifier::shift))
                        {
                            continue;
                        }
                        else
                        {
                            args->pos.z += 32;
                            continue;
                        }
                    }
                }
            }
            activateSelectedConstructionWidgets();
            return;
        }
    }

    // 0x004A1968
    template<typename TGetPieceId, typename TTryMakeJunction, typename TGetPiece, typename GetPlacementArgsFunc, typename PlaceGhostFunc>
    static void onToolUpdateTrack(const int16_t x, const int16_t y, TGetPieceId&& getPieceId, TTryMakeJunction&& tryMakeJunction, TGetPiece&& getPiece, GetPlacementArgsFunc&& getPlacementArgs, PlaceGhostFunc&& placeGhost)
    {
        World::mapInvalidateMapSelectionTiles();
        World::resetMapSelectionFlag(World::MapSelectionFlags::enable | World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::enableConstructionArrow);

        Pos2 constructPos;
        int16_t constructHeight = 0;
        const auto junctionRes = tryMakeJunction(x, y);
        if (junctionRes)
        {
            constructPos = World::toWorldSpace(junctionRes->first);
            constructHeight = junctionRes->second;

            _cState->makeJunction = 1;
        }
        else
        {
            const auto constRes = getConstructionPos(x, y);
            if (!constRes)
            {
                return;
            }
            constructPos = World::toWorldSpace(constRes->first);
            constructHeight = constRes->second;

            _cState->makeJunction = 0;
        }

        World::setMapSelectionFlags(World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::enableConstructionArrow);
        World::resetMapSelectionFlag(World::MapSelectionFlags::unk_03);

        _constructionArrowPos = World::Pos3(constructPos.x, constructPos.y, constructHeight);
        _constructionArrowDirection = _cState->constructionRotation;
        _mapSelectedTiles[0] = constructPos;
        _mapSelectedTiles[1].x = -1;

        auto pieceId = getPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);
        if (!pieceId)
        {
            removeConstructionGhosts();
            World::mapInvalidateMapSelectionTiles();
            return;
        }
        _cState->byte_1136065 = pieceId->id;
        const auto trackPieces = getPiece(pieceId->id);
        setMapSelectedTilesFromPiece(trackPieces, constructPos, _cState->constructionRotation);

        if (_cState->makeJunction != 1)
        {
            constructHeight = std::max(getMaxConstructHeightFromExistingSelection(), constructHeight);
        }

        _constructionArrowPos->z = constructHeight - getMaxPieceHeight(trackPieces);
        constructHeight -= 16;
        auto maxRetries = 2;

        if (Input::hasKeyModifier(Input::KeyModifier::shift))
        {
            maxRetries = 0x80000008;
            constructHeight -= 16;
            _constructionArrowPos->z = constructHeight;
        }
        constructionGhostLoop({ constructPos.x, constructPos.y, constructHeight }, maxRetries, getPlacementArgs, placeGhost);
        World::mapInvalidateMapSelectionTiles();
    }

    // 0x0049DC8C
    static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::construct)
        {
            return;
        }

        if (_cState->trackType & (1 << 7))
        {
            onToolUpdateTrack(x, y, getRoadPieceId, tryMakeRoadJunctionAtLoc, TrackData::getRoadPiece, getRoadPlacementArgs, placeRoadGhost);
        }
        else
        {
            onToolUpdateTrack(x, y, getTrackPieceId, tryMakeTrackJunctionAtLoc, TrackData::getTrackPiece, getTrackPlacementArgs, placeTrackGhost);
        }
    }

    template<typename TGetPieceId, typename TTryMakeJunction, typename TGetPiece>
    static void onToolDownT(const int16_t x, const int16_t y, TGetPieceId&& getPieceId, TTryMakeJunction&& tryMakeJunction, TGetPiece&& getPiece)
    {
        mapInvalidateMapSelectionTiles();
        removeConstructionGhosts();

        auto pieceId = getPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);

        if (!pieceId)
            return;

        _cState->byte_1136065 = pieceId->id;

        int16_t constructHeight = getMaxConstructHeightFromExistingSelection();
        _cState->word_1136000 = constructHeight;

        World::resetMapSelectionFlag(World::MapSelectionFlags::enable | World::MapSelectionFlags::enableConstruct | World::MapSelectionFlags::enableConstructionArrow);

        Pos2 constructPos;
        const auto junctionRes = tryMakeJunction(x, y);
        if (junctionRes)
        {
            constructPos = World::toWorldSpace(junctionRes->first);
            _cState->makeJunction = 1;
            _cState->word_1135FFE = junctionRes->second;
        }
        else
        {
            const auto constRes = getConstructionPos(x, y, _cState->word_1136000);
            if (!constRes)
            {
                return;
            }
            constructPos = World::toWorldSpace(constRes->first);
            constructHeight = constRes->second;

            _cState->makeJunction = 0;
        }
        ToolManager::toolCancel();

        auto maxRetries = 0;
        if (Input::hasKeyModifier(Input::KeyModifier::shift) || _cState->makeJunction != 1)
        {
            const auto piece = getPiece(_cState->byte_1136065);

            constructHeight -= getMaxPieceHeight(piece);
            constructHeight -= 16;
            maxRetries = 2;

            if (Input::hasKeyModifier(Input::KeyModifier::shift))
            {
                maxRetries = 0x80000008;
                constructHeight -= 16;
            }
        }
        else
        {
            maxRetries = 1;
            constructHeight = _cState->word_1135FFE;
        }

        // Height should never go negative
        constructHeight = std::max<int16_t>(0, constructHeight);

        constructionLoop(constructPos, maxRetries, constructHeight);
    }

    // 0x0049DC97
    static void onToolDown([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::construct)
            return;

        if (_cState->trackType & (1 << 7))
        {
            onToolDownT(x, y, getRoadPieceId, tryMakeRoadJunctionAtLoc, TrackData::getRoadPiece);
        }
        else
        {
            onToolDownT(x, y, getTrackPieceId, tryMakeTrackJunctionAtLoc, TrackData::getTrackPiece);
        }
    }

    // 0x0049D4F5
    static Ui::CursorId cursor([[maybe_unused]] Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] int16_t xPos, [[maybe_unused]] int16_t yPos, Ui::CursorId fallback)
    {
        if (widgetIndex == widx::bridge || widgetIndex == widx::bridge_dropdown)
            Input::setTooltipTimeout(2000);
        return fallback;
    }

    // 0x0049CE79
    static void prepareDraw(Window& self)
    {
        Common::prepareDraw(&self);

        auto args = FormatArguments(self.widgets[Common::widx::caption].textArgs);
        if (_cState->trackType & (1 << 7))
        {
            auto roadObj = ObjectManager::get<RoadObject>(_cState->trackType & ~(1 << 7));
            args.push(roadObj->name);
        }
        else
        {
            auto trackObj = ObjectManager::get<TrackObject>(_cState->trackType);
            args.push(trackObj->name);
        }
        Common::repositionTabs(&self);
    }

    static std::optional<FormatArguments> tooltip(Ui::Window&, WidgetIndex_t)
    {
        FormatArguments args{};
        args.skip(2);
        if (_cState->lastSelectedBridge != 0xFF)
        {
            auto bridgeObj = ObjectManager::get<BridgeObject>(_cState->lastSelectedBridge);
            if (bridgeObj != nullptr)
            {
                args.push(bridgeObj->name);
                if (bridgeObj->maxSpeed == kSpeed16Null)
                {
                    args.push(StringIds::unlimited_speed);
                    args.push<uint16_t>(0);
                }
                else
                {
                    args.push(StringIds::velocity);
                    args.push(bridgeObj->maxSpeed);
                }
                args.push<uint16_t>(bridgeObj->maxHeight);
            }
        }
        return args;
    }

    // 0x004A0AE5
    void drawTrack(const World::Pos3& pos, uint16_t selectedMods, uint8_t trackType, uint8_t trackPieceId, uint8_t direction, Gfx::DrawingContext& drawingCtx)
    {
        const ViewportFlags backupViewFlags = addr<0x00E3F0BC, ViewportFlags>(); // After all users of 0x00E3F0BC implemented this is not required
        Paint::SessionOptions options{};
        options.rotation = WindowManager::getCurrentRotation(); // This shouldn't be needed...
        auto* session = Paint::allocateSession(drawingCtx.currentRenderTarget(), options);

        const auto backupSelectionFlags = World::getMapSelectionFlags();
        const World::Pos3 backupConstructionArrowPos = _constructionArrowPos;
        const uint8_t backupConstructionArrowDir = _constructionArrowDirection;

        World::resetMapSelectionFlag(World::MapSelectionFlags::enableConstructionArrow);
        if (_byte_522095 & (1 << 1))
        {
            World::setMapSelectionFlags(World::MapSelectionFlags::enableConstructionArrow);
            _constructionArrowPos = pos;
            _constructionArrowDirection = direction;
        }

        const auto* trackObj = ObjectManager::get<TrackObject>(trackType);
        // Remove any none compatible track mods
        for (auto mod = 0; mod < 4; ++mod)
        {
            if (selectedMods & (1 << mod))
            {
                const auto* trackExtraObj = ObjectManager::get<TrackExtraObject>(trackObj->mods[mod]);
                if ((trackExtraObj->trackPieces & TrackData::getTrackMiscData(trackPieceId).compatibleFlags) != TrackData::getTrackMiscData(trackPieceId).compatibleFlags)
                {
                    selectedMods &= ~(1 << mod);
                }
            }
        }

        const auto& trackPieces = TrackData::getTrackPiece(trackPieceId);
        const auto trackDirection = direction & 3;

        World::TileElement backupTileElements[5] = {};

        World::SurfaceElement previewSideSurfaceTileElement{ 255, 255, 0xF, true };
        previewSideSurfaceTileElement.setLastFlag(true);

        for (const auto& trackPiece : trackPieces)
        {
            const auto pieceOffset = World::Pos3{ Math::Vector::rotate(World::Pos2{ trackPiece.x, trackPiece.y }, trackDirection), trackPiece.z };
            const auto quarterTile = trackPiece.subTileClearance.rotate(trackDirection);
            const auto trackPos = pos + pieceOffset;
            const auto baseZ = trackPos.z / kSmallZStep;
            const auto clearZ = baseZ + (trackPiece.clearZ + 32) / kSmallZStep;

            const auto centreTileCoords = World::toTileSpace(trackPos);
            const auto eastTileCoords = centreTileCoords + World::toTileSpace(World::kOffsets[1]);
            const auto westTileCoords = centreTileCoords - World::toTileSpace(World::kOffsets[1]);
            const auto northTileCoords = centreTileCoords + World::toTileSpace(World::kOffsets[3]);
            const auto southTileCoords = centreTileCoords - World::toTileSpace(World::kOffsets[3]);

            // Copy map elements which will be replaced with temporary ones containing track
            backupTileElements[0] = *World::TileManager::get(centreTileCoords)[0];
            backupTileElements[1] = *World::TileManager::get(eastTileCoords)[0];
            backupTileElements[2] = *World::TileManager::get(westTileCoords)[0];
            backupTileElements[3] = *World::TileManager::get(northTileCoords)[0];
            backupTileElements[4] = *World::TileManager::get(southTileCoords)[0];

            // Set the temporary track element
            World::TrackElement newTrackEl(baseZ, clearZ, trackDirection, quarterTile.getBaseQuarterOccupied(), trackPiece.index, trackType, trackPieceId, std::nullopt, CompanyManager::getControllingId(), selectedMods);
            newTrackEl.setLastFlag(true);

            // Replace map elements with temp ones
            *World::TileManager::get(centreTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&newTrackEl);
            *World::TileManager::get(eastTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&previewSideSurfaceTileElement);
            *World::TileManager::get(westTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&previewSideSurfaceTileElement);
            *World::TileManager::get(northTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&previewSideSurfaceTileElement);
            *World::TileManager::get(southTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&previewSideSurfaceTileElement);

            // Draw this map tile
            Paint::paintTileElements(*session, trackPos);

            // Restore map elements
            *World::TileManager::get(centreTileCoords)[0] = backupTileElements[0];
            *World::TileManager::get(eastTileCoords)[0] = backupTileElements[1];
            *World::TileManager::get(westTileCoords)[0] = backupTileElements[2];
            *World::TileManager::get(northTileCoords)[0] = backupTileElements[3];
            *World::TileManager::get(southTileCoords)[0] = backupTileElements[4];
        }

        session->arrangeStructs();
        session->drawStructs(drawingCtx);

        // setMapSelectionFlags OR's flags so reset them to zero to set the backup
        World::resetMapSelectionFlags();
        World::setMapSelectionFlags(backupSelectionFlags);
        _constructionArrowPos = backupConstructionArrowPos;
        _constructionArrowDirection = backupConstructionArrowDir;

        options.viewFlags = backupViewFlags;
        Paint::allocateSession(drawingCtx.currentRenderTarget(), options); // After all users of 0x00E3F0BC implemented this is not required
    }

    // 0x00478F1F
    void drawRoad(const World::Pos3& pos, uint16_t selectedMods, uint8_t roadType, uint8_t roadPieceId, uint8_t direction, Gfx::DrawingContext& drawingCtx)
    {
        const ViewportFlags backupViewFlags = addr<0x00E3F0BC, ViewportFlags>(); // After all users of 0x00E3F0BC implemented this is not required
        Paint::SessionOptions options{};
        options.rotation = WindowManager::getCurrentRotation(); // This shouldn't be needed...
        auto* session = Paint::allocateSession(drawingCtx.currentRenderTarget(), options);

        const auto backupSelectionFlags = World::getMapSelectionFlags();
        const World::Pos3 backupConstructionArrowPos = _constructionArrowPos;
        const uint8_t backupConstructionArrowDir = _constructionArrowDirection;

        World::resetMapSelectionFlag(World::MapSelectionFlags::enableConstructionArrow);
        if (_byte_522095 & (1 << 1))
        {
            World::setMapSelectionFlags(World::MapSelectionFlags::enableConstructionArrow);
            _constructionArrowPos = pos;
            _constructionArrowDirection = direction;
        }

        const auto* roadObj = ObjectManager::get<RoadObject>(roadType);
        // Remove any none compatible road mods
        for (auto mod = 0; mod < 2; ++mod)
        {
            if (selectedMods & (1 << mod))
            {
                const auto* roadExtraObj = ObjectManager::get<RoadExtraObject>(roadObj->mods[mod]);
                if ((roadExtraObj->roadPieces & TrackData::getRoadMiscData(roadPieceId).compatibleFlags) != TrackData::getRoadMiscData(roadPieceId).compatibleFlags)
                {
                    selectedMods &= ~(1 << mod);
                }
            }
        }

        const auto& roadPieces = TrackData::getRoadPiece(roadPieceId);
        const auto roadDirection = direction & 3;

        World::TileElement backupTileElements[5] = {};

        World::SurfaceElement previewSideSurfaceTileElement{ 255, 255, 0xF, true };
        previewSideSurfaceTileElement.setLastFlag(true);

        for (const auto& roadPiece : roadPieces)
        {
            const auto pieceOffset = World::Pos3{ Math::Vector::rotate(World::Pos2{ roadPiece.x, roadPiece.y }, roadDirection), roadPiece.z };
            const auto quarterTile = roadPiece.subTileClearance.rotate(roadDirection);
            const auto trackPos = pos + pieceOffset;
            const auto baseZ = trackPos.z / kSmallZStep;
            const auto clearZ = baseZ + (roadPiece.clearZ + 32) / kSmallZStep;

            const auto centreTileCoords = World::toTileSpace(trackPos);
            const auto eastTileCoords = centreTileCoords + World::toTileSpace(World::kOffsets[1]);
            const auto westTileCoords = centreTileCoords - World::toTileSpace(World::kOffsets[1]);
            const auto northTileCoords = centreTileCoords + World::toTileSpace(World::kOffsets[3]);
            const auto southTileCoords = centreTileCoords - World::toTileSpace(World::kOffsets[3]);

            // Copy map elements which will be replaced with temporary ones containing road
            backupTileElements[0] = *World::TileManager::get(centreTileCoords)[0];
            backupTileElements[1] = *World::TileManager::get(eastTileCoords)[0];
            backupTileElements[2] = *World::TileManager::get(westTileCoords)[0];
            backupTileElements[3] = *World::TileManager::get(northTileCoords)[0];
            backupTileElements[4] = *World::TileManager::get(southTileCoords)[0];

            // Set the temporary road element
            World::RoadElement newRoadEl(baseZ, clearZ);
            newRoadEl.setRotation(roadDirection);
            newRoadEl.setSequenceIndex(roadPiece.index);
            newRoadEl.setRoadObjectId(roadType);
            newRoadEl.setRoadId(roadPieceId);
            newRoadEl.setMods(selectedMods);
            newRoadEl.setOccupiedQuarter(quarterTile.getBaseQuarterOccupied());
            newRoadEl.setOwner(CompanyManager::getControllingId());
            newRoadEl.setLastFlag(true);

            // Replace map elements with temp ones
            *World::TileManager::get(centreTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&newRoadEl);
            *World::TileManager::get(eastTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&previewSideSurfaceTileElement);
            *World::TileManager::get(westTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&previewSideSurfaceTileElement);
            *World::TileManager::get(northTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&previewSideSurfaceTileElement);
            *World::TileManager::get(southTileCoords)[0] = *reinterpret_cast<World::TileElement*>(&previewSideSurfaceTileElement);

            // Draw this map tile
            Paint::paintTileElements(*session, trackPos);

            // Restore map elements
            *World::TileManager::get(centreTileCoords)[0] = backupTileElements[0];
            *World::TileManager::get(eastTileCoords)[0] = backupTileElements[1];
            *World::TileManager::get(westTileCoords)[0] = backupTileElements[2];
            *World::TileManager::get(northTileCoords)[0] = backupTileElements[3];
            *World::TileManager::get(southTileCoords)[0] = backupTileElements[4];
        }

        session->arrangeStructs();
        session->drawStructs(drawingCtx);

        // setMapSelectionFlags OR's flags so reset them to zero to set the backup
        World::resetMapSelectionFlags();
        World::setMapSelectionFlags(backupSelectionFlags);
        _constructionArrowPos = backupConstructionArrowPos;
        _constructionArrowDirection = backupConstructionArrowDir;

        options.viewFlags = backupViewFlags;
        Paint::allocateSession(drawingCtx.currentRenderTarget(), options); // After all users of 0x00E3F0BC implemented this is not required
    }

    // 0x0049D38A and 0x0049D16B
    static void drawCostString(Window* self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        auto x = self->widgets[widx::construct].midX();
        x += self->x;
        auto y = self->widgets[widx::construct].bottom + self->y - 23;

        if (_cState->constructionHover != 1)
        {
            tr.drawStringCentred(Point(x, y), Colour::black, StringIds::build_this);
        }

        y += 11;

        if (_cState->trackCost != 0x80000000)
        {
            if (_cState->trackCost != 0)
            {
                FormatArguments args{};
                args.push<uint32_t>(_cState->trackCost);
                tr.drawStringCentred(Point(x, y), Colour::black, StringIds::build_cost, args);
            }
        }
    }

    // 0x0049D106
    static void drawTrackCost(Window* self, Gfx::RenderTarget* clipped, Gfx::DrawingContext& drawingCtx, Point pos, uint16_t width, uint16_t height)
    {
        width >>= 1;
        height >>= 1;
        height += 16;
        pos.x -= width;
        pos.y -= height;
        clipped->x += pos.x;
        clipped->y += pos.y;

        _byte_522095 = _byte_522095 | (1 << 1);

        drawingCtx.pushRenderTarget(*clipped);

        drawTrack(
            World::Pos3(256 * World::kTileSize, 256 * World::kTileSize, 120 * World::kSmallZStep),
            _cState->word_1135FD8,
            _cState->byte_1136077,
            _cState->lastSelectedTrackPieceId,
            _cState->byte_1136078,
            drawingCtx);

        drawingCtx.popRenderTarget();

        _byte_522095 = _byte_522095 & ~(1 << 1);

        drawCostString(self, drawingCtx);
    }

    // 0x0049D325
    static void drawRoadCost(Window* self, Gfx::RenderTarget* clipped, Gfx::DrawingContext& drawingCtx, Point pos, uint16_t width, uint16_t height)
    {
        width >>= 1;
        height >>= 1;
        height += 16;
        pos.x -= width;
        pos.y -= height;
        clipped->x += pos.x;
        clipped->y += pos.y;

        _byte_522095 = _byte_522095 | (1 << 1);

        drawingCtx.pushRenderTarget(*clipped);

        drawRoad(
            World::Pos3(256 * World::kTileSize, 256 * World::kTileSize, 120 * World::kSmallZStep),
            _cState->word_1135FD8,
            _cState->byte_1136077,
            _cState->lastSelectedTrackPieceId,
            _cState->byte_1136078,
            drawingCtx);

        drawingCtx.popRenderTarget();

        _byte_522095 = _byte_522095 & ~(1 << 1);

        drawCostString(self, drawingCtx);
    }

    // 0x0049CF36
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
        Common::drawTabs(&self, drawingCtx);

        if (self.widgets[widx::bridge].type != WidgetType::none)
        {
            if (_cState->lastSelectedBridge != 0xFF)
            {
                auto bridgeObj = ObjectManager::get<BridgeObject>(_cState->lastSelectedBridge);
                if (bridgeObj != nullptr)
                {
                    auto company = CompanyManager::getPlayerCompany();
                    auto imageId = Gfx::recolour(bridgeObj->image, company->mainColours.primary);
                    auto x = self.x + self.widgets[widx::bridge].left + 2;
                    auto y = self.y + self.widgets[widx::bridge].top + 1;

                    drawingCtx.drawImage(x, y, imageId);
                }
            }
        }

        if (self.widgets[widx::construct].type == WidgetType::none)
            return;

        if (_cState->trackType & (1 << 7))
        {
            auto road = getRoadPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);

            _cState->word_1135FD8 = _cState->lastSelectedMods;

            if (!road)
                return;

            _cState->byte_1136077 = _cState->trackType & ~(1 << 7);
            _cState->byte_1136078 = road->rotation;
            _cState->lastSelectedTrackPieceId = road->id;
            _cState->word_1135FD6 = (_cState->lastSelectedBridge << 8) & 0x1F;

            auto x = self.x + self.widgets[widx::construct].left + 1;
            auto y = self.y + self.widgets[widx::construct].top + 1;
            auto width = self.widgets[widx::construct].width();
            auto height = self.widgets[widx::construct].height();

            const auto& rt = drawingCtx.currentRenderTarget();
            auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(x, y, width, height));
            if (clipped)
            {
                const auto& roadPiece = World::TrackData::getRoadPiece(_cState->lastSelectedTrackPieceId);
                const auto& lastRoadPart = roadPiece.back();

                Pos3 pos3D = { lastRoadPart.x, lastRoadPart.y, lastRoadPart.z };

                if (lastRoadPart.hasFlags(World::TrackData::PreviewTrackFlags::unused))
                {
                    pos3D.x = 0;
                    pos3D.y = 0;
                }

                auto rotatedPos = Math::Vector::rotate(pos3D, _cState->byte_1136078 & 3);
                pos3D.x = rotatedPos.x / 2;
                pos3D.y = rotatedPos.y / 2;
                pos3D.x += 0x2010;
                pos3D.y += 0x2010;
                pos3D.z += 0x1CC;

                auto pos2D = gameToScreen(pos3D, WindowManager::getCurrentRotation());

                Point pos = { pos2D.x, pos2D.y };
                drawRoadCost(&self, &*clipped, drawingCtx, pos, width, height);
            }
            else
            {
                drawCostString(&self, drawingCtx);
            }
        }
        else
        {
            auto track = getTrackPieceId(_cState->lastSelectedTrackPiece, _cState->lastSelectedTrackGradient, _cState->constructionRotation);

            _cState->word_1135FD8 = _cState->lastSelectedMods;

            if (!track)
                return;

            _cState->byte_1136077 = _cState->trackType;
            _cState->byte_1136078 = track->rotation;
            _cState->lastSelectedTrackPieceId = track->id;
            _cState->word_1135FD6 = (_cState->lastSelectedBridge << 8) & 0x1F;

            auto x = self.x + self.widgets[widx::construct].left + 1;
            auto y = self.y + self.widgets[widx::construct].top + 1;
            auto width = self.widgets[widx::construct].width();
            auto height = self.widgets[widx::construct].height();

            const auto& rt = drawingCtx.currentRenderTarget();
            auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(x, y, width, height));
            if (clipped)
            {
                const auto& trackPiece = World::TrackData::getTrackPiece(_cState->lastSelectedTrackPieceId);
                const auto& lastTrackPart = trackPiece.back();

                Pos3 pos3D = { lastTrackPart.x, lastTrackPart.y, lastTrackPart.z };

                if (lastTrackPart.hasFlags(World::TrackData::PreviewTrackFlags::unused))
                {
                    pos3D.x = 0;
                    pos3D.y = 0;
                }

                auto rotatedPos = Math::Vector::rotate(pos3D, _cState->byte_1136078 & 3);
                pos3D.x = rotatedPos.x / 2;
                pos3D.y = rotatedPos.y / 2;
                pos3D.x += 0x2010;
                pos3D.y += 0x2010;
                pos3D.z += 0x1CC;

                auto pos2D = gameToScreen(pos3D, WindowManager::getCurrentRotation());

                Point pos = { pos2D.x, pos2D.y };
                drawTrackCost(&self, &*clipped, drawingCtx, pos, width, height);
            }
            else
            {
                drawCostString(&self, drawingCtx);
            }
        }
    }

    void tabReset(Window* self)
    {
        if (_cState->constructionHover != 0)
        {
            _cState->constructionHover = 0;
            _cState->byte_113607E = 1;
            self->callOnMouseUp(widx::rotate_90);
        }
    }

    static constexpr WindowEventList kEvents = {
        .onClose = Common::onClose,
        .onMouseUp = onMouseUp,
        .onResize = onResize,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .onToolUpdate = onToolUpdate,
        .onToolDown = onToolDown,
        .tooltip = tooltip,
        .cursor = cursor,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    const WindowEventList& getEvents()
    {
        return kEvents;
    }

    void previousTrackPiece(Window* self)
    {
        WidgetIndex_t prev = self->prevAvailableWidgetInRange(widx::left_hand_curve_very_small, widx::s_bend_dual_track_right);
        if (prev != -1)
            self->callOnMouseDown(prev);
    }

    void nextTrackPiece(Window* self)
    {
        WidgetIndex_t next = self->nextAvailableWidgetInRange(widx::left_hand_curve_very_small, widx::s_bend_dual_track_right);
        if (next != -1)
            self->callOnMouseDown(next);
    }

    void previousSlope(Window* self)
    {
        WidgetIndex_t prev = self->prevAvailableWidgetInRange(widx::steep_slope_down, widx::steep_slope_up);
        if (prev != -1)
            self->callOnMouseDown(prev);
    }

    void nextSlope(Window* self)
    {
        WidgetIndex_t next = self->nextAvailableWidgetInRange(widx::steep_slope_down, widx::steep_slope_up);
        if (next != -1)
            self->callOnMouseDown(next);
    }

    void buildAtCurrentPos(Window* self)
    {
        if (self->currentTab != Common::widx::tab_construction - Common::widx::tab_construction)
            return;

        if (_cState->constructionHover == 0)
            self->callOnMouseUp(widx::construct);
    }

    void removeAtCurrentPos(Window* self)
    {
        if (self->currentTab == Common::widx::tab_construction - Common::widx::tab_construction)
            self->callOnMouseUp(widx::remove);
    }

    void selectPosition(Window* self)
    {
        if (self->currentTab != Common::widx::tab_construction - Common::widx::tab_construction)
            return;

        if (_cState->constructionHover == 0)
            self->callOnMouseUp(widx::rotate_90);
    }
}
