#include "../../Audio/Audio.h"
#include "../../CompanyManager.h"
#include "../../GameCommands/GameCommands.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Localisation/StringIds.h"
#include "../../Map/Track/Track.h"
#include "../../Map/Track/TrackData.h"
#include "../../Objects/BridgeObject.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/RoadObject.h"
#include "../../Objects/TrackObject.h"
#include "../../Station.h"
#include "../../Ui/Dropdown.h"
#include "../../Widget.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;
using namespace OpenLoco::Literals;

namespace OpenLoco::Ui::Windows::Construction::Construction
{
    static loco_global<uint8_t, 0x00508F09> _byte_508F09;
    static loco_global<uint8_t, 0x00522090> _byte_522090;
    static loco_global<uint8_t, 0x00522091> _byte_522091;
    static loco_global<uint8_t, 0x00522092> _byte_522092;

    static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;
    static loco_global<uint32_t, 0x00523394> _toolWidgetIndex;

    static loco_global<Map::Pos3, 0x00F24942> _constructionArrowPos;
    static loco_global<uint8_t, 0x00F24948> _constructionArrowDirection;

    static loco_global<uint8_t, 0x0112C2E9> _alternateTrackObjectId; // set from GameCommands::createRoad
    static loco_global<uint8_t[18], 0x0050A006> available_objects;   // toptoolbar

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

    Widget widgets[] = {
        commonWidgets(138, 276, StringIds::stringid_2),
        makeWidget({ 3, 45 }, { 22, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_left_hand_curve_very_small, StringIds::tooltip_left_hand_curve_very_small),
        makeWidget({ 3, 45 }, { 22, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_left_hand_curve_small, StringIds::tooltip_left_hand_curve_small),
        makeWidget({ 25, 45 }, { 22, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_left_hand_curve, StringIds::tooltip_left_hand_curve),
        makeWidget({ 47, 45 }, { 22, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_left_hand_curve_large, StringIds::tooltip_left_hand_curve_large),
        makeWidget({ 69, 45 }, { 22, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_right_hand_curve_large, StringIds::tooltip_right_hand_curve_large),
        makeWidget({ 91, 45 }, { 22, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_right_hand_curve, StringIds::tooltip_right_hand_curve),
        makeWidget({ 113, 45 }, { 22, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_right_hand_curve_small, StringIds::tooltip_right_hand_curve_small),
        makeWidget({ 113, 45 }, { 22, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_right_hand_curve_very_small, StringIds::tooltip_right_hand_curve_very_small),
        makeWidget({ 9, 69 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_s_bend_dual_track_left, StringIds::tooltip_s_bend_left_dual_track),
        makeWidget({ 33, 69 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_s_bend_left, StringIds::tooltip_s_bend_left),
        makeWidget({ 57, 69 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_straight, StringIds::tooltip_straight),
        makeWidget({ 81, 69 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_s_bend_right, StringIds::tooltip_s_bend_right),
        makeWidget({ 105, 69 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_s_bend_dual_track_right, StringIds::tooltip_s_bend_right_dual_track),
        makeWidget({ 9, 96 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_steep_slope_down, StringIds::tooltip_steep_slope_down),
        makeWidget({ 33, 96 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_slope_down, StringIds::tooltip_slope_down),
        makeWidget({ 57, 96 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_level, StringIds::tooltip_level),
        makeWidget({ 81, 96 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_slope_up, StringIds::tooltip_slope_up),
        makeWidget({ 105, 96 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_steep_slope_up, StringIds::tooltip_steep_slope_up),
        makeDropdownWidgets({ 40, 123 }, { 58, 20 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty, StringIds::tooltip_bridge_stats),
        makeWidget({ 3, 145 }, { 132, 100 }, WidgetType::wt_5, WindowColour::secondary, 0xFFFFFFFF, StringIds::tooltip_construct),
        makeWidget({ 6, 248 }, { 46, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_remove, StringIds::tooltip_remove),
        makeWidget({ 57, 248 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_90),
        widgetEnd(),
    };

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

    static loco_global<uint8_t, 0x00525FB0> _pickupDirection; // From Vehicle.cpp window

    // 0x0049B50C
    void reset()
    {
        std::fill(std::begin(_scenarioSignals), std::end(_scenarioSignals), 0xFF);
        std::fill(std::begin(_scenarioBridges), std::end(_scenarioBridges), 0xFF);
        std::fill(std::begin(_scenarioTrainStations), std::end(_scenarioTrainStations), 0xFF);
        std::fill(std::begin(_scenarioTrackMods), std::end(_scenarioTrackMods), 0xFF);

        _lastAirport = 0xFF;
        _lastShipPort = 0xFF;
        _pickupDirection = 0;
    }

    // 0x004A18D4
    static void sub_4A18D4()
    {
        if (_alternateTrackObjectId == 0xFF)
        {
            return;
        }

        if ((_alternateTrackObjectId | (1 << 7)) == _trackType)
        {
            return;
        }

        auto* alternativeRoadObj = ObjectManager::get<RoadObject>(_alternateTrackObjectId);
        if (!(alternativeRoadObj->flags & Flags12::unk_03))
        {
            return;
        }
        auto* curRoadObj = ObjectManager::get<RoadObject>(_trackType & ~(1 << 7));
        if (!(curRoadObj->flags & Flags12::unk_03))
        {
            return;
        }

        for (const auto objId : available_objects)
        {
            if (objId == 0xFF)
            {
                return;
            }

            if (objId == (_alternateTrackObjectId | (1 << 7)))
            {
                _trackType = _alternateTrackObjectId | (1 << 7);
                Common::sub_4A3A50();
            }
        }
    }

    // 0x0049FA10
    static void constructRoad()
    {
        _trackCost = 0x80000000;
        _byte_1136076 = 0;
        _dword_1135F42 = 0x80000000;
        removeConstructionGhosts();
        auto roadPiece = getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);
        if (!roadPiece)
        {
            return;
        }
        auto* roadObj = ObjectManager::get<RoadObject>(_trackType & ~(1 << 7));
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(string_id));
        formatArgs.push(roadObj->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

        GameCommands::RoadPlacementArgs args;
        args.pos = Map::Pos3(_x, _y, _constructionZ);
        args.rotation = roadPiece->rotation;
        args.roadId = roadPiece->id;
        args.mods = _lastSelectedMods;
        args.bridge = _lastSelectedBridge;
        args.roadObjectId = _trackType & ~(1 << 7);

        _dword_1135F42 = GameCommands::doCommand(args, GameCommands::Flags::apply);
        if (_dword_1135F42 == GameCommands::FAILURE)
        {
            if (GameCommands::getErrorText() != StringIds::unable_to_cross_or_create_junction_with_string
                || _alternateTrackObjectId == 0xFF)
            {
                return;
            }

            sub_4A18D4();
            roadPiece = getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);
            if (!roadPiece)
            {
                return;
            }

            WindowManager::close(WindowType::error);
            args.pos = Map::Pos3(_x, _y, _constructionZ);
            args.rotation = roadPiece->rotation;
            args.roadId = roadPiece->id;
            args.mods = _lastSelectedMods;
            args.bridge = _lastSelectedBridge;
            args.roadObjectId = _trackType & ~(1 << 7);

            _dword_1135F42 = GameCommands::doCommand(args, GameCommands::Flags::apply);
        }

        if (_dword_1135F42 != GameCommands::FAILURE)
        {
            const auto& trackSize = TrackData::getUnkRoad((args.roadId << 3) | (args.rotation & 0x3));
            const auto newPosition = args.pos + trackSize.pos;
            _x = newPosition.x;
            _y = newPosition.y;
            _constructionZ = newPosition.z;
            _constructionRotation = trackSize.rotationEnd;
            _byte_522096 = 0;
            _byte_1136066 = 0;
            if (_lastSelectedTrackPiece >= 9)
            {
                _lastSelectedTrackPiece = 0;
            }
        }
    }

    // 0x0049F93A
    static void constructTrack()
    {
        _trackCost = 0x80000000;
        _byte_1136076 = 0;
        _dword_1135F42 = 0x80000000;
        removeConstructionGhosts();
        auto trackPiece = getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);
        if (!trackPiece)
        {
            return;
        }
        auto* roadObj = ObjectManager::get<TrackObject>(_trackType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(string_id));
        formatArgs.push(roadObj->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

        GameCommands::TrackPlacementArgs args;
        args.pos = Map::Pos3(_x, _y, _constructionZ);
        args.rotation = trackPiece->rotation;
        args.trackId = trackPiece->id;
        args.mods = _lastSelectedMods;
        args.bridge = _lastSelectedBridge;
        args.trackObjectId = _trackType;
        args.unk = _byte_113607E & (1 << 0);

        _dword_1135F42 = GameCommands::doCommand(args, GameCommands::Flags::apply);

        if (_dword_1135F42 == GameCommands::FAILURE)
        {
            return;
        }

        const auto& trackSize = TrackData::getUnkTrack((args.trackId << 3) | (args.rotation & 0x3));
        const auto newPosition = args.pos + trackSize.pos;
        _x = newPosition.x;
        _y = newPosition.y;
        _constructionZ = newPosition.z;
        _constructionRotation = trackSize.rotationEnd;
        _byte_522096 = 0;
        _byte_1136066 = 0;
        if (_lastSelectedTrackPiece >= 9)
        {
            _lastSelectedTrackPiece = 0;
        }
    }

    // 0x0049F92D
    static void constructTrack(Window* self, WidgetIndex_t widgetIndex)
    {
        if (_trackType & (1 << 7))
        {
            constructRoad();
        }
        else
        {
            constructTrack();
        }
        activateSelectedConstructionWidgets();
    }

    static loco_global<Map::Pos2[16], 0x00503C6C> _503C6C;
    static loco_global<Map::Track::TrackConnections, 0x0113609C> _113609C;
    static loco_global<uint8_t[2], 0x0113601A> _113601A;

    // 0x004A012E
    static void removeTrack()
    {
        _trackCost = 0x80000000;
        _byte_1136076 = 0;
        removeConstructionGhosts();
        if (_constructionHover != 0)
        {
            return;
        }

        Map::Pos3 loc(_x, _y, _constructionZ);
        uint32_t trackAndDirection = 0;

        if (_constructionRotation < 4)
        {
            trackAndDirection = 0;
        }
        else if (_constructionRotation < 8)
        {
            trackAndDirection = 26 << 3;
        }
        else if (_constructionRotation < 12)
        {
            trackAndDirection = 27 << 3;
        }
        else
        {
            trackAndDirection = 1 << 3;
            loc += Map::Pos3{ _503C6C[_constructionRotation], 0 };
        }
        trackAndDirection |= (1 << 2) | (_constructionRotation & 0x3);
        _113601A[0] = 0;
        _113601A[1] = 0;
        _113609C->size = 0;
        auto trackEnd = Map::Track::getTrackConnectionEnd(loc, trackAndDirection);
        Map::Track::getTrackConnections(trackEnd.first, trackEnd.second, _113609C, CompanyManager::getControllingId(), _trackType);

        if (_113609C->size == 0)
        {
            return;
        }

        const auto trackAndDirection2 = (_113609C->data[_113609C->size - 1] & 0x1FF) ^ (1 << 2);
        Map::Pos3 loc2(_x, _y, _constructionZ);
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
        args.trackObjectId = _trackType;

        auto* trackObj = ObjectManager::get<TrackObject>(_trackType);
        auto formatArgs = FormatArguments::common();
        formatArgs.skip(3 * sizeof(string_id));
        formatArgs.push(trackObj->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

        if (GameCommands::doCommand(args, GameCommands::Flags::apply) != GameCommands::FAILURE)
        {
            Map::Pos3 newConstructLoc = Map::Pos3(_x, _y, _constructionZ) - TrackData::getUnkTrack(trackAndDirection2).pos;
            _x = newConstructLoc.x;
            _y = newConstructLoc.y;
            _constructionZ = newConstructLoc.z;
            _constructionRotation = TrackData::getUnkTrack(trackAndDirection2).rotationBegin;
            _lastSelectedTrackPiece = 0;
            _lastSelectedTrackGradient = 0;
            activateSelectedConstructionWidgets();
        }
    }

    // 0x004A02F2
    static void removeRoad()
    {
        _trackCost = 0x80000000;
        _byte_1136076 = 0;
        removeConstructionGhosts();
        if (_constructionHover != 0)
        {
            return;
        }

        Map::Pos3 loc(_x, _y, _constructionZ);
        uint32_t trackAndDirection = (1 << 2) | (_constructionRotation & 0x3);
        _113601A[0] = 0;
        _113601A[1] = 0;
        _113609C->size = 0;
        Map::Track::getRoadConnections(loc, _113609C, CompanyManager::getControllingId(), _trackType & ~(1 << 7), trackAndDirection);

        if (_113609C->size == 0)
        {
            return;
        }

        for (size_t i = 0; i < _113609C->size; ++i)
        {
            // If trackId is zero
            if ((_113609C->data[i] & 0x1F8) == 0)
            {
                std::swap(_113609C->data[0], _113609C->data[i]);
            }
        }

        auto* roadObj = ObjectManager::get<RoadObject>(_trackType & ~(1 << 7));
        if (!(roadObj->flags & Flags12::unk_02))
        {
            _113609C->size = 1;
            _113609C->data[1] = 0xFFFF;
        }

        uint16_t trackAndDirection2 = 0;
        while (_113609C->size != 0)
        {
            trackAndDirection2 = (_113609C->pop_back() & 0x1FF) ^ (1 << 2);
            Map::Pos3 loc2(_x, _y, _constructionZ);
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
            args.unkDirection = trackAndDirection2 & 0x3;
            args.roadId = trackAndDirection2 >> 3;
            args.objectId = _trackType & ~(1 << 7);

            auto* trackObj = ObjectManager::get<RoadObject>(_trackType & ~(1 << 7));
            auto formatArgs = FormatArguments::common();
            formatArgs.skip(3 * sizeof(string_id));
            formatArgs.push(trackObj->name);
            GameCommands::setErrorTitle(StringIds::cant_build_pop3_string);

            if (GameCommands::doCommand(args, GameCommands::Flags::apply) == GameCommands::FAILURE)
            {
                return;
            }
        }

        Map::Pos3 newConstructLoc = Map::Pos3(_x, _y, _constructionZ) - TrackData::getUnkRoad(trackAndDirection2).pos;
        _x = newConstructLoc.x;
        _y = newConstructLoc.y;
        _constructionZ = newConstructLoc.z;
        _constructionRotation = TrackData::getUnkRoad(trackAndDirection2).rotationBegin;
        _lastSelectedTrackPiece = 0;
        _lastSelectedTrackGradient = 0;
        activateSelectedConstructionWidgets();
    }

    // 0x004A0121
    static void removeTrack(Window* self, WidgetIndex_t widgetIndex)
    {
        if (_trackType & (1 << 7))
        {
            removeRoad();
        }
        else
        {
            removeTrack();
        }
    }

    // 0x0049D3F6
    static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::widx::close_button:
                WindowManager::close(self);
                break;

            case Common::widx::tab_construction:
            case Common::widx::tab_overhead:
            case Common::widx::tab_signal:
            case Common::widx::tab_station:
                Common::switchTab(self, widgetIndex);
                break;

            case widx::construct:
                constructTrack(self, widgetIndex);
                break;

            case widx::remove:
                removeTrack(self, widgetIndex);
                break;

            case widx::rotate_90:
            {
                if (_constructionHover == 1)
                {
                    _constructionRotation++;
                    _constructionRotation = _constructionRotation & 3;
                    _trackCost = 0x80000000;
                    activateSelectedConstructionWidgets();
                    break;
                }
                removeConstructionGhosts();
                WindowManager::viewportSetVisibility(WindowManager::ViewportVisibility::overgroundView);
                Input::toolSet(self, widx::construct, CursorId::crosshair);
                Input::setFlag(Input::Flags::flag6);

                _constructionHover = 1;
                _byte_113607E = 0;
                _constructionRotation = _constructionRotation & 3;

                activateSelectedConstructionWidgets();
                break;
            }
        }
    }

    // 0x0049DB71
    static void disableUnusedPiecesRotation(uint64_t* disabledWidgets)
    {
        if (_constructionRotation < 12)
        {
            if (_constructionRotation >= 8)
            {
                *disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_small);
                *disabledWidgets |= (1 << widx::s_bend_right) | (1 << widx::slope_down) | (1 << widx::slope_up);
            }
            else
            {
                if (_constructionRotation >= 4)
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
        if (_lastSelectedTrackGradient == 2 || _lastSelectedTrackGradient == 6 || _lastSelectedTrackGradient == 4 || _lastSelectedTrackGradient == 8)
        {
            disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
            disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);
            disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
        }

        disableUnusedPiecesRotation(&disabledWidgets);

        if (_constructionHover == 0)
        {
            auto road = getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);
            if (!road)
                disabledWidgets |= (1 << widx::construct);
        }
        self->setDisabledWidgetsAndInvalidate(disabledWidgets);
    }

    // 0x0049DB1F
    static void disableUnusedTrackPieces(Window* self, TrackObject trackObj, uint64_t disabledWidgets)
    {
        if (_lastSelectedTrackGradient == 2 || _lastSelectedTrackGradient == 6 || _lastSelectedTrackGradient == 4 || _lastSelectedTrackGradient == 8)
        {
            disabledWidgets |= (1 << widx::left_hand_curve_very_small) | (1 << widx::left_hand_curve) | (1 << widx::left_hand_curve_large) | (1 << widx::right_hand_curve_large) | (1 << widx::right_hand_curve) | (1 << widx::right_hand_curve_very_small);
            disabledWidgets |= (1 << widx::s_bend_dual_track_left) | (1 << widx::s_bend_left) | (1 << widx::s_bend_right) | (1 << widx::s_bend_dual_track_right);

            if (!(trackObj.track_pieces & (1 << 8)))
                disabledWidgets |= (1 << widx::left_hand_curve_small) | (1 << widx::right_hand_curve_small);
        }

        disableUnusedPiecesRotation(&disabledWidgets);

        if (_constructionHover == 0)
        {
            auto track = getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            if (!track)
                disabledWidgets |= (1 << widx::construct);
        }
        self->setDisabledWidgetsAndInvalidate(disabledWidgets);
    }

    // 0x0049DAF3
    static void disableTrackSlopes(Window* self, TrackObject trackObj, uint64_t disabledWidgets)
    {
        auto maskedTrackPieces = trackObj.track_pieces & ((1 << 5) | (1 << 8));

        if (maskedTrackPieces != ((1 << 5) | (1 << 8)))
            disabledWidgets |= (1 << widx::slope_down) | (1 << widx::slope_up);

        maskedTrackPieces = trackObj.track_pieces & ((1 << 6) | (1 << 8));

        if (maskedTrackPieces != ((1 << 6) | (1 << 8)))
            disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::steep_slope_up);

        disableUnusedTrackPieces(self, trackObj, disabledWidgets);
    }

    static void setMapSelectedTilesFromPiece(const stdx::span<const TrackData::PreviewTrack> pieces, const Map::Pos2& origin, const uint8_t rotation)
    {
        size_t i = 0;
        for (const auto& piece : pieces)
        {
            if (piece.flags & Map::TrackData::PreviewTrackFlags::diagonal)
            {
                continue;
            }
            _mapSelectedTiles[i++] = origin + Math::Vector::rotate(Map::Pos2{ piece.x, piece.y }, rotation);
        }

        _mapSelectedTiles[i].x = -1;
        mapInvalidateMapSelectionTiles();
    }

    static void activateSelectedRoadWidgets(Window* window)
    {
        TileManager::mapInvalidateMapSelectionTiles();
        Input::setMapSelectionFlags(Input::MapSelectionFlags::enableConstruct | Input::MapSelectionFlags::unk_03);

        auto road = getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

        uint8_t rotation;
        uint8_t roadId;

        auto x = _x;
        auto y = _y;

        if (!road)
        {
            rotation = _constructionRotation;
            roadId = 0;
        }
        else
        {
            rotation = road->rotation;
            roadId = road->id;
        }

        const auto& roadPiece = Map::TrackData::getRoadPiece(roadId);
        rotation &= 3;

        setMapSelectedTilesFromPiece(roadPiece, Map::Pos2(x, y), rotation);
        window->holdableWidgets = (1 << widx::construct) | (1 << widx::remove);

        auto trackType = _trackType & ~(1 << 7);
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

        if (roadObj->road_pieces & RoadPieceFlags::track)
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

        if (roadObj->road_pieces & RoadPieceFlags::one_way)
        {
            window->widgets[widx::left_hand_curve_small].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve_small].type = WidgetType::buttonWithImage;
        }

        window->widgets[widx::s_bend_dual_track_left].type = WidgetType::none;
        window->widgets[widx::s_bend_dual_track_right].type = WidgetType::none;

        if (roadObj->road_pieces & RoadPieceFlags::one_sided)
        {
            window->widgets[widx::s_bend_dual_track_left].type = WidgetType::buttonWithImage;
            window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_right_turnaround;
            window->widgets[widx::s_bend_dual_track_left].tooltip = StringIds::tooltip_turnaround;

            if (_byte_525FAE == 0)
                window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_left_turnaround;
        }

        window->widgets[widx::steep_slope_down].type = WidgetType::none;
        window->widgets[widx::slope_down].type = WidgetType::none;
        window->widgets[widx::slope_up].type = WidgetType::none;
        window->widgets[widx::steep_slope_up].type = WidgetType::none;

        if (roadObj->road_pieces & RoadPieceFlags::slope)
        {
            window->widgets[widx::slope_down].type = WidgetType::buttonWithImage;
            window->widgets[widx::slope_up].type = WidgetType::buttonWithImage;
        }

        if (roadObj->road_pieces & RoadPieceFlags::steep_slope)
        {
            window->widgets[widx::steep_slope_down].type = WidgetType::buttonWithImage;
            window->widgets[widx::steep_slope_up].type = WidgetType::buttonWithImage;
        }

        window->widgets[widx::bridge].type = WidgetType::combobox;
        window->widgets[widx::bridge_dropdown].type = WidgetType::button;

        if (_lastSelectedBridge == 0xFF || (_constructionHover != 1 && !(_byte_1136076 & 1)))
        {
            window->widgets[widx::bridge].type = WidgetType::none;
            window->widgets[widx::bridge_dropdown].type = WidgetType::none;
        }

        auto activatedWidgets = window->activatedWidgets;
        activatedWidgets &= ~(Construction::allTrack);

        window->widgets[widx::construct].type = WidgetType::none;
        window->widgets[widx::remove].type = WidgetType::buttonWithImage;
        window->widgets[widx::rotate_90].type = WidgetType::none;

        if (_constructionHover == 1)
        {
            window->widgets[widx::construct].type = WidgetType::wt_5;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_start_construction;
            window->widgets[widx::remove].type = WidgetType::none;
            window->widgets[widx::rotate_90].type = WidgetType::buttonWithImage;
            window->widgets[widx::rotate_90].image = ImageIds::rotate_object;
            window->widgets[widx::rotate_90].tooltip = StringIds::rotate_90;
        }
        else if (_constructionHover == 0)
        {
            window->widgets[widx::construct].type = WidgetType::wt_3;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_construct;
            window->widgets[widx::rotate_90].type = WidgetType::buttonWithImage;
            window->widgets[widx::rotate_90].image = ImageIds::construction_new_position;
            window->widgets[widx::rotate_90].tooltip = StringIds::new_construction_position;
        }
        if (_constructionHover == 0 || _constructionHover == 1)
        {
            if (_lastSelectedTrackPiece != 0xFF)
            {
                auto trackPieceWidget = trackPieceWidgets[_lastSelectedTrackPiece];
                activatedWidgets |= 1ULL << trackPieceWidget;
            }

            uint8_t trackGradient = widx::level;

            switch (_lastSelectedTrackGradient)
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
        TileManager::mapInvalidateMapSelectionTiles();
        Input::setMapSelectionFlags(Input::MapSelectionFlags::enableConstruct | Input::MapSelectionFlags::unk_03);

        auto track = getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

        uint8_t rotation;
        uint8_t trackId;
        auto x = _x;
        auto y = _y;

        if (!track)
        {
            rotation = _constructionRotation;
            trackId = 0;
        }
        else
        {
            rotation = track->rotation;
            trackId = track->id;
        }

        const auto& trackPiece = Map::TrackData::getTrackPiece(trackId);
        rotation &= 3;

        setMapSelectedTilesFromPiece(trackPiece, Map::Pos2(x, y), rotation);
        window->holdableWidgets = (1 << widx::construct) | (1 << widx::remove);

        auto trackObj = ObjectManager::get<TrackObject>(_trackType);

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

        if (trackObj->track_pieces & TrackPieceFlags::very_small_curve)
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

        if (trackObj->track_pieces & TrackPieceFlags::large_curve)
        {
            window->widgets[widx::left_hand_curve_large].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve_large].type = WidgetType::buttonWithImage;
        }

        if (trackObj->track_pieces & TrackPieceFlags::normal_curve)
        {
            window->widgets[widx::left_hand_curve].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve].type = WidgetType::buttonWithImage;
        }

        if (trackObj->track_pieces & TrackPieceFlags::small_curve)
        {
            window->widgets[widx::left_hand_curve_small].type = WidgetType::buttonWithImage;
            window->widgets[widx::right_hand_curve_small].type = WidgetType::buttonWithImage;
        }

        window->widgets[widx::s_bend_dual_track_left].type = WidgetType::none;
        window->widgets[widx::s_bend_dual_track_right].type = WidgetType::none;

        if (trackObj->track_pieces & TrackPieceFlags::one_sided)
        {
            window->widgets[widx::s_bend_dual_track_left].type = WidgetType::buttonWithImage;
            window->widgets[widx::s_bend_dual_track_right].type = WidgetType::buttonWithImage;
            window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_s_bend_dual_track_left;
            window->widgets[widx::s_bend_dual_track_right].image = ImageIds::construction_s_bend_dual_track_right;
            window->widgets[widx::s_bend_dual_track_left].tooltip = StringIds::tooltip_s_bend_left_dual_track;
            window->widgets[widx::s_bend_dual_track_right].tooltip = StringIds::tooltip_s_bend_right_dual_track;

            _byte_522090 = 16;
            _byte_522091 = 20;

            if (_constructionRotation >= 4 && _constructionRotation < 12)
            {
                window->widgets[widx::s_bend_dual_track_left].image = ImageIds::construction_right_turnaround;
                window->widgets[widx::s_bend_dual_track_right].image = ImageIds::construction_s_bend_to_single_track_left;
                window->widgets[widx::s_bend_dual_track_left].tooltip = StringIds::tooltip_turnaround;
                window->widgets[widx::s_bend_dual_track_right].tooltip = StringIds::tooltip_s_bend_to_single_track;
                _byte_522090 = 20;
                _byte_522092 = 16;
                if (_constructionRotation >= 8)
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

        if (trackObj->track_pieces & TrackPieceFlags::slope)
        {
            window->widgets[widx::slope_down].type = WidgetType::buttonWithImage;
            window->widgets[widx::slope_up].type = WidgetType::buttonWithImage;
        }

        if (trackObj->track_pieces & TrackPieceFlags::steep_slope)
        {
            window->widgets[widx::steep_slope_down].type = WidgetType::buttonWithImage;
            window->widgets[widx::steep_slope_up].type = WidgetType::buttonWithImage;
        }

        window->widgets[widx::bridge].type = WidgetType::combobox;
        window->widgets[widx::bridge_dropdown].type = WidgetType::button;

        if (_lastSelectedBridge == 0xFF || (_constructionHover != 1 && !(_byte_1136076 & 1)))
        {
            window->widgets[widx::bridge].type = WidgetType::none;
            window->widgets[widx::bridge_dropdown].type = WidgetType::none;
        }

        auto activatedWidgets = window->activatedWidgets;
        activatedWidgets &= ~(Construction::allTrack);

        window->widgets[widx::construct].type = WidgetType::none;
        window->widgets[widx::remove].type = WidgetType::buttonWithImage;
        window->widgets[widx::rotate_90].type = WidgetType::none;

        if (_constructionHover == 1)
        {
            window->widgets[widx::construct].type = WidgetType::wt_5;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_start_construction;
            window->widgets[widx::remove].type = WidgetType::none;
            window->widgets[widx::rotate_90].type = WidgetType::buttonWithImage;
            window->widgets[widx::rotate_90].image = ImageIds::rotate_object;
            window->widgets[widx::rotate_90].tooltip = StringIds::rotate_90;
        }
        else if (_constructionHover == 0)
        {
            window->widgets[widx::construct].type = WidgetType::wt_3;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_construct;
            window->widgets[widx::rotate_90].type = WidgetType::buttonWithImage;
            window->widgets[widx::rotate_90].image = ImageIds::construction_new_position;
            window->widgets[widx::rotate_90].tooltip = StringIds::new_construction_position;
        }
        if (_constructionHover == 0 || _constructionHover == 1)
        {
            if (_lastSelectedTrackPiece != 0xFF)
            {
                auto trackPieceWidget = trackPieceWidgets[_lastSelectedTrackPiece];
                activatedWidgets |= 1ULL << trackPieceWidget;
            }

            uint8_t trackGradient = widx::level;

            switch (_lastSelectedTrackGradient)
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
            if (_trackType & (1 << 7))
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
    static void onResize(Window* self)
    {
        self->enabledWidgets &= ~(1 << widx::construct);

        if (_constructionHover != 1)
            self->enabledWidgets |= (1 << widx::construct);

        auto disabledWidgets = self->disabledWidgets;
        disabledWidgets &= (1 << Common::widx::tab_construction | 1 << Common::widx::tab_overhead | 1 << Common::widx::tab_signal | 1 << Common::widx::tab_station);
        uint8_t trackType = _trackType;

        if (trackType & (1 << 7))
        {
            trackType &= ~(1 << 7);

            if (_lastSelectedTrackPiece == 0xFF)
            {
                disableUnusedRoadPieces(self, disabledWidgets);
                return;
            }
            switch (_lastSelectedTrackPiece)
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
                    disableUnusedRoadPieces(self, disabledWidgets);
                    break;
                }

                case TrackPiece::left_hand_curve_very_small:
                case TrackPiece::right_hand_curve_very_small:
                case TrackPiece::left_hand_curve_small:
                case TrackPiece::right_hand_curve_small:
                case TrackPiece::turnaround:
                {
                    disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
                    disableUnusedRoadPieces(self, disabledWidgets);
                    break;
                }
            }
        }
        else
        {
            auto trackObj = ObjectManager::get<TrackObject>(trackType);
            if (_lastSelectedTrackPiece == 0xFF)
            {
                disableUnusedTrackPieces(self, *trackObj, disabledWidgets);
                return;
            }
            switch (_lastSelectedTrackPiece)
            {
                case TrackPiece::straight:
                    disableUnusedTrackPieces(self, *trackObj, disabledWidgets);
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
                    disableUnusedTrackPieces(self, *trackObj, disabledWidgets);
                    break;
                }

                case TrackPiece::left_hand_curve_small:
                case TrackPiece::right_hand_curve_small:
                {
                    disableTrackSlopes(self, *trackObj, disabledWidgets);
                    break;
                }
            }
        }
    }

    // 0x0049d600 (based on)
    static void changeTrackPiece(uint8_t trackPiece, bool slope)
    {
        _byte_113603A = 0xFF;
        removeConstructionGhosts();

        if (slope)
            _lastSelectedTrackGradient = trackPiece;
        else
            _lastSelectedTrackPiece = trackPiece;

        _trackCost = 0x80000000;
        activateSelectedConstructionWidgets();
    }

    // 0x0049D83A
    static void bridgeDropdown(Window* self)
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

        Dropdown::show(x, y, width, height, self->getColour(WindowColour::secondary), bridgeCount, 22, flags);
        for (auto i = 0; i < 9; i++)
        {
            auto bridge = _bridgeList[i];

            if (bridge == 0xFF)
                return;

            if (bridge == _lastSelectedBridge)
                Dropdown::setHighlightedItem(i);

            auto bridgeObj = ObjectManager::get<BridgeObject>(bridge);
            auto company = CompanyManager::get(_playerCompany);
            auto companyColour = company->mainColours.primary;
            auto imageId = Gfx::recolour(bridgeObj->image, companyColour);

            auto args = FormatArguments();
            args.push(imageId);

            if (bridgeObj->max_speed == kSpeed16Null)
            {
                args.push(StringIds::unlimited_speed);
                args.push<uint16_t>(0);
            }
            else
            {
                args.push(StringIds::velocity);
                args.push(bridgeObj->max_speed);
            }
            args.push<uint16_t>(bridgeObj->max_height);

            Dropdown::add(i, StringIds::dropdown_bridge_stats, args);
        }
    }

    // 0x0049D42F
    static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
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
                _byte_113603A = 0xFF;
                removeConstructionGhosts();
                _trackCost = 0x80000000;

                if (self->widgets[widx::s_bend_dual_track_left].image == ImageIds::construction_s_bend_dual_track_left)
                    _lastSelectedTrackPiece = TrackPiece::s_bend_to_dual_track;
                else if (self->widgets[widx::s_bend_dual_track_left].image == ImageIds::construction_left_turnaround || self->widgets[widx::s_bend_dual_track_left].image == ImageIds::construction_right_turnaround)
                    _lastSelectedTrackPiece = TrackPiece::turnaround;
                else
                    _lastSelectedTrackPiece = TrackPiece::s_bend_to_single_track;

                activateSelectedConstructionWidgets();
                break;
            }

            case widx::s_bend_dual_track_right:
            {
                _byte_113603A = 0xFF;
                removeConstructionGhosts();
                _trackCost = 0x80000000;

                if (self->widgets[widx::s_bend_dual_track_right].image == ImageIds::construction_s_bend_dual_track_right)
                    _lastSelectedTrackPiece = TrackPiece::s_bend_to_dual_track;
                else if (self->widgets[widx::s_bend_dual_track_right].image == ImageIds::construction_left_turnaround)
                    _lastSelectedTrackPiece = TrackPiece::turnaround;
                else
                    _lastSelectedTrackPiece = TrackPiece::s_bend_to_single_track;

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
                bridgeDropdown(self);
                break;
            }

            case widx::construct:
            {
                if (*_clickRepeatTicks >= 40)
                    constructTrack(self, widgetIndex);
                break;
            }

            case widx::remove:
            {
                if (*_clickRepeatTicks >= 40)
                    removeTrack(self, widgetIndex);
                break;
            }
        }
    }

    // 0x0049D4EA
    static void onDropdown(Window* self, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex == widx::bridge_dropdown)
        {
            if (itemIndex != -1)
            {
                auto bridge = _bridgeList[itemIndex];
                _lastSelectedBridge = bridge;

                // TODO: & ~(1 << 7) added to prevent crashing when selecting bridges for road/trams
                _scenarioBridges[_trackType & ~(1 << 7)] = bridge;
                removeConstructionGhosts();
                _trackCost = 0x80000000;
                activateSelectedConstructionWidgets();
            }
        }
    }

    static void sub_49FD66()
    {
        registers regs;
        call(0x0049FD66, regs);
    }

    // 0x0049DCA2
    static void onUpdate(Window* self)
    {
        self->frame_no++;
        self->callPrepareDraw();
        WindowManager::invalidate(WindowType::construction, self->number);

        if (_constructionHover == 1)
        {
            if (!Input::isToolActive(WindowType::construction, self->number) || _toolWidgetIndex != widx::construct)
                WindowManager::close(self);
        }
        if (_constructionHover == 0)
        {
            if (Input::isToolActive(WindowType::construction, self->number))
                Input::toolCancel();
        }
        sub_49FD66();
    }

    // Simplified TileManager::getHeight that only considers flat height
    static std::optional<Map::TileHeight> getConstructionHeight(const Pos2& mapPos)
    {
        auto tile = TileManager::get(mapPos);

        auto surfaceTile = tile.surface();

        if (surfaceTile == nullptr)
            return std::nullopt;

        Map::TileHeight height = { static_cast<coord_t>(surfaceTile->baseZ() * 4), static_cast<coord_t>(surfaceTile->water() * 16) };
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
        static loco_global<Ui::Point, 0x0113600C> _113600C;
        static loco_global<Viewport*, 0x01135F52> _1135F52;
        _113600C = { x, y };

        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::roadAndTram));
        _1135F52 = viewport;

        if (interaction.type != ViewportInteraction::InteractionItem::road)
        {
            return std::nullopt;
        }

        const auto* elTrack = reinterpret_cast<Map::TileElement*>(interaction.object)->as<RoadElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        const auto& roadPieces = TrackData::getRoadPiece(elTrack->roadId());
        const auto& roadPiece = roadPieces[elTrack->sequenceIndex()];

        const auto startHeight = elTrack->baseZ() * 4 - roadPiece.z;

        return { startHeight };
    }

    // 0x004A4011
    static std::optional<std::pair<int16_t, int16_t>> getExistingTrackAtLoc(int16_t x, int16_t y)
    {
        static loco_global<Ui::Point, 0x0113600C> _113600C;
        static loco_global<Viewport*, 0x01135F52> _1135F52;
        _113600C = { x, y };

        auto [interaction, viewport] = ViewportInteraction::getMapCoordinatesFromPos(x, y, ~(ViewportInteraction::InteractionItemFlags::track));
        _1135F52 = viewport;

        if (interaction.type != ViewportInteraction::InteractionItem::track)
        {
            return std::nullopt;
        }

        const auto* elTrack = reinterpret_cast<Map::TileElement*>(interaction.object)->as<TrackElement>();
        if (elTrack == nullptr)
        {
            return std::nullopt;
        }

        const auto& trackPieces = TrackData::getTrackPiece(elTrack->trackId());
        const auto& trackPiece = trackPieces[elTrack->sequenceIndex()];

        const auto startHeight = elTrack->baseZ() * 4 - trackPiece.z;

        return { std::make_pair(startHeight, elTrack->trackId()) };
    }

    static void constructionLoop(const Pos2& mapPos, uint32_t maxRetries, int16_t height)
    {
        while (true)
        {
            _constructionHover = 0;
            _byte_113607E = 0;
            _x = mapPos.x;
            _y = mapPos.y;
            _constructionZ = height;
            _byte_522096 = 0;
            _byte_1136066 = 0;

            activateSelectedConstructionWidgets();
            auto window = WindowManager::find(WindowType::construction);

            if (window == nullptr)
                return;

            _byte_508F09 = _byte_508F09 | (1 << 0);

            onMouseUp(window, widx::construct);

            _byte_508F09 = _byte_508F09 & ~(1 << 0);

            if (_dword_1135F42 == 0x80000000)
            {
                if (GameCommands::getErrorText() != StringIds::error_can_only_build_above_ground)
                {
                    maxRetries--;
                    if (maxRetries != 0)
                    {
                        height -= 16;
                        if (height >= 0)
                        {
                            if (Input::hasKeyModifier(Input::KeyModifier::shift))
                            {
                                continue;
                            }
                            else
                            {
                                height += 32;
                                continue;
                            }
                        }
                    }
                }
            }
            else
            {
                _byte_113607E = 1;
                WindowManager::close(WindowType::error, 0);
                return;
            }

            onMouseUp(window, widx::rotate_90);

            Audio::playSound(Audio::SoundId::error, int32_t(Input::getMouseLocation().x));

            return;
        }
    }

    // 0 if nothing currently selected
    static int16_t getMaxConstructHeightFromExistingSelection()
    {
        int16_t maxHeight = 0;

        if (Input::hasMapSelectionFlag(Input::MapSelectionFlags::enableConstruct))
        {
            for (const auto& tile : _mapSelectedTiles)
            {
                if (tile.x == -1)
                {
                    break;
                }
                if (!Map::validCoords(tile))
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

    static std::optional<std::pair<Map::TilePos2, int16_t>> tryMakeRoadJunctionAtLoc(const int16_t x, const int16_t y)
    {
        const auto existingRoad = getExistingRoadAtLoc(x, y);

        if (existingRoad)
        {
            const auto& existingHeight = *existingRoad;
            const auto mapPos = screenGetMapXyWithZ(Point(x, y), existingHeight);
            if (mapPos)
            {
                return { std::make_pair(Map::TilePos2(*mapPos), existingHeight) };
            }
        }
        return std::nullopt;
    }

    static std::optional<std::pair<Map::TilePos2, int16_t>> tryMakeTrackJunctionAtLoc(const int16_t x, const int16_t y)
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
                    return { std::make_pair(Map::TilePos2(*mapPos), existingHeight) };
                }
            }
        }
        return std::nullopt;
    }

    static std::optional<std::pair<Map::TilePos2, int16_t>> getConstructionPos(const int16_t x, const int16_t y, const int16_t baseHeight = std::numeric_limits<int16_t>::max())
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

        return { std::make_pair(Map::TilePos2(*mapPos), height) };
    }

    static int16_t getMaxPieceHeight(const stdx::span<const TrackData::PreviewTrack> piece)
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
        for (const auto bridgeType : _bridgeList)
        {
            if (bridgeType == 0xFF)
            {
                return;
            }
            if (*_byte_1136075 == bridgeType)
            {
                _lastSelectedBridge = bridgeType;
                return;
            }
        }
    }

    // 0x004A006C
    void removeTrackGhosts()
    {
        if (_byte_522096 & (1 << 1))
        {
            if (_ghostRemovalTrackObjectId & (1 << 7))
            {
                GameCommands::RoadRemovalArgs args;
                args.pos = _ghostRemovalTrackPos;
                args.pos.z += TrackData::getRoadPiece(_ghostRemovalTrackId)[0].z;
                args.unkDirection = _ghostRemovalTrackRotation & 3;
                args.sequenceIndex = 0;
                args.roadId = _ghostRemovalTrackId;
                args.objectId = _ghostRemovalTrackObjectId & ~(1 << 7);
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
            else
            {
                GameCommands::TrackRemovalArgs args;
                args.pos = _ghostRemovalTrackPos;
                args.pos.z += TrackData::getTrackPiece(_ghostRemovalTrackId)[0].z;
                args.rotation = _ghostRemovalTrackRotation & 3;
                args.index = 0;
                args.trackId = _ghostRemovalTrackId;
                args.trackObjectId = _ghostRemovalTrackObjectId;
                GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
            }
            _byte_522096 = _byte_522096 & ~(1 << 1);
        }
    }

    static loco_global<uint16_t[44], 0x004F8764> _4F8764;
    static loco_global<uint16_t[10], 0x004F8764> _4F7284;

    // 0x0049FB63
    static uint32_t placeTrackGhost(const GameCommands::TrackPlacementArgs& args)
    {
        removeTrackGhosts();
        const auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
        if (res == GameCommands::FAILURE)
        {
            if (GameCommands::getErrorText() == StringIds::bridge_type_unsuitable_for_this_configuration)
            {
                _byte_113603A = 0;
                for (const auto bridgeType : _bridgeList)
                {
                    if (bridgeType == 0xFF)
                    {
                        break;
                    }
                    const auto* bridgeObj = ObjectManager::get<BridgeObject>(bridgeType);
                    if (bridgeObj->disabled_track_cfg & _4F8764[args.trackId])
                    {
                        continue;
                    }

                    if (bridgeType == _lastSelectedBridge)
                    {
                        break;
                    }

                    auto newArgs(args);
                    newArgs.bridge = bridgeType;
                    _lastSelectedBridge = bridgeType;
                    WindowManager::invalidate(WindowType::construction);
                    return placeTrackGhost(args);
                }
            }
        }
        else
        {
            _ghostRemovalTrackPos = args.pos;
            _ghostRemovalTrackId = args.trackId;
            _ghostRemovalTrackObjectId = args.trackObjectId;
            _ghostRemovalTrackRotation = args.rotation;
            _byte_522096 = (1 << 1) | *_byte_522096;
            const auto newViewState = (_byte_1136072 & (1 << 1)) ? WindowManager::ViewportVisibility::undergroundView : WindowManager::ViewportVisibility::overgroundView;
            WindowManager::viewportSetVisibility(newViewState);
            if (_lastSelectedTrackGradient != 0)
            {
                WindowManager::viewportSetVisibility(WindowManager::ViewportVisibility::heightMarksOnLand);
            }
        }
        _byte_113603A = 0;
        return res;
    }

    // 0x0049FC60
    static uint32_t placeRoadGhost(const GameCommands::RoadPlacementArgs& args)
    {
        removeTrackGhosts();
        const auto res = GameCommands::doCommand(args, GameCommands::Flags::apply | GameCommands::Flags::flag_1 | GameCommands::Flags::flag_3 | GameCommands::Flags::flag_5 | GameCommands::Flags::flag_6);
        if (res == GameCommands::FAILURE)
        {
            if (GameCommands::getErrorText() == StringIds::bridge_type_unsuitable_for_this_configuration)
            {
                _byte_113603A = 0;
                for (const auto bridgeType : _bridgeList)
                {
                    if (bridgeType == 0xFF)
                    {
                        break;
                    }
                    const auto* bridgeObj = ObjectManager::get<BridgeObject>(bridgeType);
                    if (bridgeObj->disabled_track_cfg & _4F7284[args.roadId])
                    {
                        continue;
                    }

                    if (bridgeType == _lastSelectedBridge)
                    {
                        break;
                    }

                    auto newArgs(args);
                    newArgs.bridge = bridgeType;
                    _lastSelectedBridge = bridgeType;
                    WindowManager::invalidate(WindowType::construction);
                    return placeRoadGhost(args);
                }
            }
        }
        else
        {
            _ghostRemovalTrackPos = args.pos;
            _ghostRemovalTrackId = args.roadId;
            _ghostRemovalTrackObjectId = args.roadObjectId | (1 << 7);
            _ghostRemovalTrackRotation = args.rotation;
            _byte_522096 = (1 << 1) | *_byte_522096;
            const auto newViewState = (_byte_1136072 & (1 << 1)) ? WindowManager::ViewportVisibility::undergroundView : WindowManager::ViewportVisibility::overgroundView;
            WindowManager::viewportSetVisibility(newViewState);
            if (_lastSelectedTrackGradient != 0)
            {
                WindowManager::viewportSetVisibility(WindowManager::ViewportVisibility::heightMarksOnLand);
            }
        }
        _byte_113603A = 0;
        return res;
    }

    static std::optional<GameCommands::TrackPlacementArgs> getTrackPlacementArgs(const Map::Pos3& pos, const uint8_t trackPiece, const uint8_t gradient, const uint8_t rotation)
    {
        auto trackId = getTrackPieceId(trackPiece, gradient, rotation);
        if (!trackId)
        {
            return std::nullopt;
        }
        GameCommands::TrackPlacementArgs args;
        args.pos = pos;
        args.bridge = _lastSelectedBridge;
        args.mods = _lastSelectedMods;
        args.rotation = trackId->rotation;
        args.trackObjectId = _trackType;
        args.trackId = trackId->id;
        args.unk = _byte_113607E & (1 << 0);
        return args;
    }

    static std::optional<GameCommands::RoadPlacementArgs> getRoadPlacementArgs(const Map::Pos3& pos, const uint8_t trackPiece, const uint8_t gradient, const uint8_t rotation)
    {
        auto roadId = getRoadPieceId(trackPiece, gradient, rotation);
        if (!roadId)
        {
            return std::nullopt;
        }
        GameCommands::RoadPlacementArgs args;
        args.pos = pos;
        args.bridge = _lastSelectedBridge;
        args.mods = _lastSelectedMods;
        args.rotation = roadId->rotation;
        args.roadObjectId = _trackType & ~(1 << 7);
        args.roadId = roadId->id;
        return args;
    }

    template<typename GetPlacementArgsFunc, typename PlaceGhostFunc>
    static void constructionGhostLoop(const Pos3& mapPos, uint32_t maxRetries, GetPlacementArgsFunc&& getPlacementArgs, PlaceGhostFunc&& placeGhost)
    {
        _x = mapPos.x;
        _y = mapPos.y;
        _constructionZ = mapPos.z;
        if (_byte_522096 & (1 << 1))
        {
            if (*_ghostTrackPos == mapPos)
            {
                return;
            }
        }
        _ghostTrackPos = mapPos;

        auto args = getPlacementArgs(mapPos, _lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);
        if (!args)
        {
            return;
        }
        while (true)
        {

            auto res = placeGhost(*args);
            _trackCost = res;
            _byte_1136076 = _byte_1136073;
            sub_4A193B();

            if (_trackCost == 0x80000000)
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
        Map::TileManager::mapInvalidateMapSelectionTiles();
        Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable | Input::MapSelectionFlags::enableConstruct | Input::MapSelectionFlags::unk_02);

        Pos2 constructPos;
        int16_t constructHeight = 0;
        const auto junctionRes = tryMakeJunction(x, y);
        if (junctionRes)
        {
            constructPos = junctionRes->first;
            constructHeight = junctionRes->second;

            _makeJunction = 1;
        }
        else
        {
            const auto constRes = getConstructionPos(x, y);
            if (!constRes)
            {
                return;
            }
            constructPos = constRes->first;
            constructHeight = constRes->second;

            _makeJunction = 0;
        }

        Input::setMapSelectionFlags(Input::MapSelectionFlags::enableConstruct | Input::MapSelectionFlags::unk_02);
        Input::resetMapSelectionFlag(Input::MapSelectionFlags::unk_03);

        _constructionArrowPos = Map::Pos3(constructPos.x, constructPos.y, constructHeight);
        _constructionArrowDirection = _constructionRotation;
        _mapSelectedTiles[0] = constructPos;
        _mapSelectedTiles[1].x = -1;

        auto pieceId = getPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);
        if (!pieceId)
        {
            removeConstructionGhosts();
            Map::TileManager::mapInvalidateMapSelectionTiles();
            return;
        }
        _byte_1136065 = pieceId->id;
        const auto trackPieces = getPiece(pieceId->id);
        setMapSelectedTilesFromPiece(trackPieces, constructPos, _constructionRotation);

        if (_makeJunction != 1)
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
        Map::TileManager::mapInvalidateMapSelectionTiles();
    }

    // 0x0049DC8C
    static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::construct)
        {
            return;
        }

        if (_trackType & (1 << 7))
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

        auto pieceId = getPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

        if (!pieceId)
            return;

        _byte_1136065 = pieceId->id;

        int16_t constructHeight = getMaxConstructHeightFromExistingSelection();
        _word_1136000 = constructHeight;

        Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable | Input::MapSelectionFlags::enableConstruct | Input::MapSelectionFlags::unk_02);

        Pos2 constructPos;
        const auto junctionRes = tryMakeJunction(x, y);
        if (junctionRes)
        {
            constructPos = junctionRes->first;
            _makeJunction = 1;
            _word_1135FFE = junctionRes->second;
        }
        else
        {
            const auto constRes = getConstructionPos(x, y, _word_1136000);
            if (!constRes)
            {
                return;
            }
            constructPos = constRes->first;
            constructHeight = constRes->second;

            _makeJunction = 0;
        }
        Input::toolCancel();

        auto maxRetries = 0;
        if (Input::hasKeyModifier(Input::KeyModifier::shift) || _makeJunction != 1)
        {
            const auto piece = getPiece(_byte_1136065);

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
            constructHeight = _word_1135FFE;
        }

        constructionLoop(constructPos, maxRetries, constructHeight);
    }

    // 0x0049DC97
    static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::construct)
            return;

        if (_trackType & (1 << 7))
        {
            onToolDownT(x, y, getRoadPieceId, tryMakeRoadJunctionAtLoc, TrackData::getRoadPiece);
        }
        else
        {
            onToolDownT(x, y, getTrackPieceId, tryMakeTrackJunctionAtLoc, TrackData::getTrackPiece);
        }
    }

    // 0x0049D4F5
    static Ui::CursorId cursor(Window* self, int16_t widgetIndex, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        if (widgetIndex == widx::bridge || widgetIndex == widx::bridge_dropdown)
            Input::setTooltipTimeout(2000);
        return fallback;
    }

    // 0x0049CE79
    static void prepareDraw(Window* self)
    {
        Common::prepareDraw(self);
        auto args = FormatArguments();
        if (_trackType & (1 << 7))
        {
            auto roadObj = ObjectManager::get<RoadObject>(_trackType & ~(1 << 7));
            args.push(roadObj->name);
        }
        else
        {
            auto trackObj = ObjectManager::get<TrackObject>(_trackType);
            args.push(trackObj->name);
        }
        if (_lastSelectedBridge != 0xFF)
        {
            auto bridgeObj = ObjectManager::get<BridgeObject>(_lastSelectedBridge);
            if (bridgeObj != nullptr)
            {
                args.push(bridgeObj->name);
                if (bridgeObj->max_speed == kSpeed16Null)
                {
                    args.push(StringIds::unlimited_speed);
                    args.push<uint16_t>(0);
                }
                else
                {
                    args.push(StringIds::velocity);
                    args.push(bridgeObj->max_speed);
                }
                args.push<uint16_t>(bridgeObj->max_height);
            }
        }
        Common::repositionTabs(self);
    }

    // 0x004A0AE5
    void drawTrack(uint16_t x, uint16_t y, uint16_t selectedMods, uint16_t di, uint8_t trackType, uint8_t trackPieceId, uint16_t colour, uint8_t bh)
    {
        registers regs;
        regs.ax = x;
        regs.cx = y;
        regs.edi = selectedMods << 16 | di;
        regs.bh = bh;
        regs.edx = colour << 16 | trackPieceId << 8 | trackType;
        call(0x004A0AE5, regs);
    }

    // 0x00478F1F
    void drawRoad(uint16_t x, uint16_t y, uint16_t selectedMods, uint16_t di, uint8_t trackType, uint8_t trackPieceId, uint16_t colour, uint8_t bh)
    {
        registers regs;
        regs.ax = x;
        regs.cx = y;
        regs.edi = selectedMods << 16 | di;
        regs.bh = bh;
        regs.edx = colour << 16 | trackPieceId << 8 | trackType;
        call(0x00478F1F, regs);
    }

    // 0x0049D38A and 0x0049D16B
    static void drawCostString(Window* self, Gfx::Context* context)
    {
        auto x = self->widgets[widx::construct].mid_x();
        x += self->x;
        auto y = self->widgets[widx::construct].bottom + self->y - 23;

        if (_constructionHover != 1)
            Gfx::drawStringCentred(*context, x, y, Colour::black, StringIds::build_this);

        y += 11;

        if (_trackCost != 0x80000000)
        {
            if (_trackCost != 0)
            {
                auto args = FormatArguments();
                args.push<uint32_t>(_trackCost);
                Gfx::drawStringCentred(*context, x, y, Colour::black, StringIds::build_cost, &args);
            }
        }
    }

    // 0x0049D106
    static void drawTrackCost(Window* self, Gfx::Context* clipped, Gfx::Context* context, Point pos, uint16_t width, uint16_t height)
    {
        width >>= 1;
        height >>= 1;
        height += 16;
        pos.x -= width;
        pos.y -= height;
        clipped->x += pos.x;
        clipped->y += pos.y;
        _dword_E0C3E0 = clipped;

        _byte_522095 = _byte_522095 | (1 << 1);

        drawTrack(0x2000, 0x2000, _word_1135FD8, 0x1E0, _byte_1136077, _lastSelectedTrackPieceId, _word_1135FD6, _byte_1136078);

        _byte_522095 = _byte_522095 & ~(1 << 1);

        drawCostString(self, context);
    }

    // 0x0049D325
    static void drawRoadCost(Window* self, Gfx::Context* clipped, Gfx::Context* context, Point pos, uint16_t width, uint16_t height)
    {
        width >>= 1;
        height >>= 1;
        height += 16;
        pos.x -= width;
        pos.y -= height;
        clipped->x += pos.x;
        clipped->y += pos.y;
        _dword_E0C3E0 = clipped;

        _byte_522095 = _byte_522095 | (1 << 1);

        drawRoad(0x2000, 0x2000, _word_1135FD8, 0x1E0, _byte_1136077, _lastSelectedTrackPieceId, _word_1135FD6, _byte_1136078);

        _byte_522095 = _byte_522095 & ~(1 << 1);

        drawCostString(self, context);
    }

    // 0x0049CF36
    static void draw(Window* self, Gfx::Context* context)
    {
        self->draw(context);
        Common::drawTabs(self, context);

        if (self->widgets[widx::bridge].type != WidgetType::none)
        {
            if (_lastSelectedBridge != 0xFF)
            {
                auto bridgeObj = ObjectManager::get<BridgeObject>(_lastSelectedBridge);
                if (bridgeObj != nullptr)
                {
                    auto company = CompanyManager::get(_playerCompany);
                    auto imageId = Gfx::recolour(bridgeObj->image, company->mainColours.primary);
                    auto x = self->x + self->widgets[widx::bridge].left + 2;
                    auto y = self->y + self->widgets[widx::bridge].top + 1;

                    Gfx::drawImage(context, x, y, imageId);
                }
            }
        }

        if (self->widgets[widx::construct].type == WidgetType::none)
            return;

        if (_trackType & (1 << 7))
        {
            auto road = getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            _word_1135FD8 = _lastSelectedMods;

            if (!road)
                return;

            _byte_1136077 = _trackType & ~(1 << 7);
            _byte_1136078 = road->rotation;
            _lastSelectedTrackPieceId = road->id;
            _word_1135FD6 = (_lastSelectedBridge << 8) & 0x1F;

            auto x = self->x + self->widgets[widx::construct].left + 1;
            auto y = self->y + self->widgets[widx::construct].top + 1;
            auto width = self->widgets[widx::construct].width();
            auto height = self->widgets[widx::construct].height();

            auto clipped = Gfx::clipContext(*context, Ui::Rect(x, y, width, height));
            if (clipped)
            {
                const auto& roadPiece = Map::TrackData::getRoadPiece(_lastSelectedTrackPieceId);
                const auto& lastRoadPart = roadPiece.back();

                Pos3 pos3D = { lastRoadPart.x, lastRoadPart.y, lastRoadPart.z };

                if (lastRoadPart.flags & Map::TrackData::PreviewTrackFlags::unused)
                {
                    pos3D.x = 0;
                    pos3D.y = 0;
                }

                auto rotatedPos = Math::Vector::rotate(pos3D, _byte_1136078 & 3);
                pos3D.x = rotatedPos.x / 2;
                pos3D.y = rotatedPos.y / 2;
                pos3D.x += 0x2010;
                pos3D.y += 0x2010;
                pos3D.z += 0x1CC;

                auto pos2D = gameToScreen(pos3D, WindowManager::getCurrentRotation());

                Point pos = { pos2D.x, pos2D.y };
                drawRoadCost(self, &*clipped, context, pos, width, height);
            }
            else
            {
                drawCostString(self, context);
            }
        }
        else
        {
            auto track = getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            _word_1135FD8 = _lastSelectedMods;

            if (!track)
                return;

            _byte_1136077 = _trackType;
            _byte_1136078 = track->rotation;
            _lastSelectedTrackPieceId = track->id;
            _word_1135FD6 = (_lastSelectedBridge << 8) & 0x1F;

            auto x = self->x + self->widgets[widx::construct].left + 1;
            auto y = self->y + self->widgets[widx::construct].top + 1;
            auto width = self->widgets[widx::construct].width();
            auto height = self->widgets[widx::construct].height();

            auto clipped = Gfx::clipContext(*context, Ui::Rect(x, y, width, height));
            if (clipped)
            {
                const auto& trackPiece = Map::TrackData::getTrackPiece(_lastSelectedTrackPieceId);
                const auto& lastTrackPart = trackPiece.back();

                Pos3 pos3D = { lastTrackPart.x, lastTrackPart.y, lastTrackPart.z };

                if (lastTrackPart.flags & Map::TrackData::PreviewTrackFlags::unused)
                {
                    pos3D.x = 0;
                    pos3D.y = 0;
                }

                auto rotatedPos = Math::Vector::rotate(pos3D, _byte_1136078 & 3);
                pos3D.x = rotatedPos.x / 2;
                pos3D.y = rotatedPos.y / 2;
                pos3D.x += 0x2010;
                pos3D.y += 0x2010;
                pos3D.z += 0x1CC;

                auto pos2D = gameToScreen(pos3D, WindowManager::getCurrentRotation());

                Point pos = { pos2D.x, pos2D.y };
                drawTrackCost(self, &*clipped, context, pos, width, height);
            }
            else
            {
                drawCostString(self, context);
            }
        }
    }

    void tabReset(Window* self)
    {
        if (_constructionHover != 0)
        {
            _constructionHover = 0;
            _byte_113607E = 1;
            self->callOnMouseUp(widx::rotate_90);
        }
    }

    void initEvents()
    {
        events.onClose = Common::onClose;
        events.onMouseUp = onMouseUp;
        events.onResize = onResize;
        events.onMouseDown = onMouseDown;
        events.onDropdown = onDropdown;
        events.onUpdate = onUpdate;
        events.onToolUpdate = onToolUpdate;
        events.onToolDown = onToolDown;
        events.cursor = cursor;
        events.prepareDraw = prepareDraw;
        events.draw = draw;
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

        if (_constructionHover == 0)
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

        if (_constructionHover == 0)
            self->callOnMouseUp(widx::rotate_90);
    }
}
