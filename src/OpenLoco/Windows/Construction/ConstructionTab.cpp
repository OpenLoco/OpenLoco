#include "../../Audio/Audio.h"
#include "../../CompanyManager.h"
#include "../../GameCommands/GameCommands.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Localisation/StringIds.h"
#include "../../Objects/BridgeObject.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/RoadObject.h"
#include "../../Objects/TrackObject.h"
#include "../../TrackData.h"
#include "../../Ui/Dropdown.h"
#include "../../Widget.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Construction
{
    static loco_global<uint16_t[351][4], 0x004F7B62> _word_4F7B62; // TODO: Not sure on size?
    static loco_global<uint8_t, 0x00508F09> _byte_508F09;
    static loco_global<uint8_t, 0x00522090> _byte_522090;
    static loco_global<uint8_t, 0x00522091> _byte_522091;
    static loco_global<uint8_t, 0x00522092> _byte_522092;

    static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;
    static loco_global<uint32_t, 0x00523394> _toolWidgetIndex;

    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;

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
        makeWidget({ 3, 45 }, { 22, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_left_hand_curve_very_small, StringIds::tooltip_left_hand_curve_very_small),
        makeWidget({ 3, 45 }, { 22, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_left_hand_curve_small, StringIds::tooltip_left_hand_curve_small),
        makeWidget({ 25, 45 }, { 22, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_left_hand_curve, StringIds::tooltip_left_hand_curve),
        makeWidget({ 47, 45 }, { 22, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_left_hand_curve_large, StringIds::tooltip_left_hand_curve_large),
        makeWidget({ 69, 45 }, { 22, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_right_hand_curve_large, StringIds::tooltip_right_hand_curve_large),
        makeWidget({ 91, 45 }, { 22, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_right_hand_curve, StringIds::tooltip_right_hand_curve),
        makeWidget({ 113, 45 }, { 22, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_right_hand_curve_small, StringIds::tooltip_right_hand_curve_small),
        makeWidget({ 113, 45 }, { 22, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_right_hand_curve_very_small, StringIds::tooltip_right_hand_curve_very_small),
        makeWidget({ 9, 69 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_s_bend_dual_track_left, StringIds::tooltip_s_bend_left_dual_track),
        makeWidget({ 33, 69 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_s_bend_left, StringIds::tooltip_s_bend_left),
        makeWidget({ 57, 69 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_straight, StringIds::tooltip_straight),
        makeWidget({ 81, 69 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_s_bend_right, StringIds::tooltip_s_bend_right),
        makeWidget({ 105, 69 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_s_bend_dual_track_right, StringIds::tooltip_s_bend_right_dual_track),
        makeWidget({ 9, 96 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_steep_slope_down, StringIds::tooltip_steep_slope_down),
        makeWidget({ 33, 96 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_slope_down, StringIds::tooltip_slope_down),
        makeWidget({ 57, 96 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_level, StringIds::tooltip_level),
        makeWidget({ 81, 96 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_slope_up, StringIds::tooltip_slope_up),
        makeWidget({ 105, 96 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_steep_slope_up, StringIds::tooltip_steep_slope_up),
        makeWidget({ 40, 123 }, { 58, 20 }, WidgetType::wt_18, WindowColour::secondary, StringIds::empty, StringIds::tooltip_bridge_stats),
        makeWidget({ 86, 124 }, { 11, 18 }, WidgetType::wt_11, WindowColour::secondary, StringIds::dropdown, StringIds::tooltip_bridge_stats),
        makeWidget({ 3, 145 }, { 132, 100 }, WidgetType::wt_5, WindowColour::secondary, 0xFFFFFFFF, StringIds::tooltip_construct),
        makeWidget({ 6, 248 }, { 46, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::construction_remove, StringIds::tooltip_remove),
        makeWidget({ 57, 248 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_90),
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
    };

    WindowEventList events;

    static std::optional<TrackPieceId> getRoadPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation);
    static std::optional<TrackPieceId> getTrackPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation);

    // 0x0049B50C
    void reset()
    {
        call(0x0049B50C);
    }

    // 0x0049F92D
    static void constructTrack(Window* self, WidgetIndex_t widgetIndex)
    {
        registers regs;
        regs.edx = widgetIndex;
        regs.esi = (int32_t)self;
        call(0x0049F92D, regs);
    }

    // 0x004A0121
    static void removeTrack(Window* self, WidgetIndex_t widgetIndex)
    {
        registers regs;
        regs.edx = widgetIndex;
        regs.esi = (int32_t)self;
        call(0x004A0121, regs);
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
                Common::sub_49FEC7();
                WindowManager::viewportSetVisibility(WindowManager::ViewportVisibility::overgroundView);
                Input::toolSet(self, widx::construct, 12);
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
        auto posId = 0;
        rotation &= 3;

        for (const auto& roadPart : roadPiece)
        {
            if (roadPart.flags & Map::TrackData::PreviewTrackFlags::diagonal)
            {
                continue;
            }

            Pos2 pos = { roadPart.x, roadPart.y };

            pos = Math::Vector::rotate(pos, rotation);

            pos.x += x;
            pos.y += y;
            _mapSelectedTiles[posId] = pos;
            posId++;
        }

        _mapSelectedTiles[posId].x = -1;
        mapInvalidateMapSelectionTiles();
        window->holdable_widgets = (1 << widx::construct) | (1 << widx::remove);

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

            window->widgets[widx::left_hand_curve_very_small].type = WidgetType::wt_9;
            window->widgets[widx::right_hand_curve_very_small].type = WidgetType::wt_9;
        }

        if (roadObj->road_pieces & RoadPieceFlags::one_way)
        {
            window->widgets[widx::left_hand_curve_small].type = WidgetType::wt_9;
            window->widgets[widx::right_hand_curve_small].type = WidgetType::wt_9;
        }

        window->widgets[widx::s_bend_dual_track_left].type = WidgetType::none;
        window->widgets[widx::s_bend_dual_track_right].type = WidgetType::none;

        if (roadObj->road_pieces & RoadPieceFlags::one_sided)
        {
            window->widgets[widx::s_bend_dual_track_left].type = WidgetType::wt_9;
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
            window->widgets[widx::slope_down].type = WidgetType::wt_9;
            window->widgets[widx::slope_up].type = WidgetType::wt_9;
        }

        if (roadObj->road_pieces & RoadPieceFlags::steep_slope)
        {
            window->widgets[widx::steep_slope_down].type = WidgetType::wt_9;
            window->widgets[widx::steep_slope_up].type = WidgetType::wt_9;
        }

        window->widgets[widx::bridge].type = WidgetType::wt_18;
        window->widgets[widx::bridge_dropdown].type = WidgetType::wt_11;

        if (_lastSelectedBridge == 0xFF || (_constructionHover != 1 && !(_byte_1136076 & 1)))
        {
            window->widgets[widx::bridge].type = WidgetType::none;
            window->widgets[widx::bridge_dropdown].type = WidgetType::none;
        }

        auto activatedWidgets = window->activated_widgets;
        activatedWidgets &= ~(Construction::allTrack);

        window->widgets[widx::construct].type = WidgetType::none;
        window->widgets[widx::remove].type = WidgetType::wt_9;
        window->widgets[widx::rotate_90].type = WidgetType::none;

        if (_constructionHover == 1)
        {
            window->widgets[widx::construct].type = WidgetType::wt_5;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_start_construction;
            window->widgets[widx::remove].type = WidgetType::none;
            window->widgets[widx::rotate_90].type = WidgetType::wt_9;
            window->widgets[widx::rotate_90].image = ImageIds::rotate_object;
            window->widgets[widx::rotate_90].tooltip = StringIds::rotate_90;
        }
        else if (_constructionHover == 0)
        {
            window->widgets[widx::construct].type = WidgetType::wt_3;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_construct;
            window->widgets[widx::rotate_90].type = WidgetType::wt_9;
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
        window->activated_widgets = activatedWidgets;
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
        auto posId = 0;
        rotation &= 3;

        for (const auto& trackPart : trackPiece)
        {
            if (trackPart.flags & Map::TrackData::PreviewTrackFlags::diagonal)
            {
                continue;
            }
            Pos2 pos = { trackPart.x, trackPart.y };

            pos = Math::Vector::rotate(pos, rotation);

            pos.x += x;
            pos.y += y;
            _mapSelectedTiles[posId] = pos;
            posId++;
        }

        _mapSelectedTiles[posId].x = -1;
        mapInvalidateMapSelectionTiles();
        window->holdable_widgets = (1 << widx::construct) | (1 << widx::remove);

        auto trackObj = ObjectManager::get<TrackObject>(_trackType);

        window->widgets[widx::s_bend_left].type = WidgetType::wt_9;
        window->widgets[widx::s_bend_right].type = WidgetType::wt_9;
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

            window->widgets[widx::left_hand_curve_very_small].type = WidgetType::wt_9;
            window->widgets[widx::right_hand_curve_very_small].type = WidgetType::wt_9;
        }

        if (trackObj->track_pieces & TrackPieceFlags::large_curve)
        {
            window->widgets[widx::left_hand_curve_large].type = WidgetType::wt_9;
            window->widgets[widx::right_hand_curve_large].type = WidgetType::wt_9;
        }

        if (trackObj->track_pieces & TrackPieceFlags::normal_curve)
        {
            window->widgets[widx::left_hand_curve].type = WidgetType::wt_9;
            window->widgets[widx::right_hand_curve].type = WidgetType::wt_9;
        }

        if (trackObj->track_pieces & TrackPieceFlags::small_curve)
        {
            window->widgets[widx::left_hand_curve_small].type = WidgetType::wt_9;
            window->widgets[widx::right_hand_curve_small].type = WidgetType::wt_9;
        }

        window->widgets[widx::s_bend_dual_track_left].type = WidgetType::none;
        window->widgets[widx::s_bend_dual_track_right].type = WidgetType::none;

        if (trackObj->track_pieces & TrackPieceFlags::one_sided)
        {
            window->widgets[widx::s_bend_dual_track_left].type = WidgetType::wt_9;
            window->widgets[widx::s_bend_dual_track_right].type = WidgetType::wt_9;
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
            window->widgets[widx::slope_down].type = WidgetType::wt_9;
            window->widgets[widx::slope_up].type = WidgetType::wt_9;
        }

        if (trackObj->track_pieces & TrackPieceFlags::steep_slope)
        {
            window->widgets[widx::steep_slope_down].type = WidgetType::wt_9;
            window->widgets[widx::steep_slope_up].type = WidgetType::wt_9;
        }

        window->widgets[widx::bridge].type = WidgetType::wt_18;
        window->widgets[widx::bridge_dropdown].type = WidgetType::wt_11;

        if (_lastSelectedBridge == 0xFF || (_constructionHover != 1 && !(_byte_1136076 & 1)))
        {
            window->widgets[widx::bridge].type = WidgetType::none;
            window->widgets[widx::bridge_dropdown].type = WidgetType::none;
        }

        auto activatedWidgets = window->activated_widgets;
        activatedWidgets &= ~(Construction::allTrack);

        window->widgets[widx::construct].type = WidgetType::none;
        window->widgets[widx::remove].type = WidgetType::wt_9;
        window->widgets[widx::rotate_90].type = WidgetType::none;

        if (_constructionHover == 1)
        {
            window->widgets[widx::construct].type = WidgetType::wt_5;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_start_construction;
            window->widgets[widx::remove].type = WidgetType::none;
            window->widgets[widx::rotate_90].type = WidgetType::wt_9;
            window->widgets[widx::rotate_90].image = ImageIds::rotate_object;
            window->widgets[widx::rotate_90].tooltip = StringIds::rotate_90;
        }
        else if (_constructionHover == 0)
        {
            window->widgets[widx::construct].type = WidgetType::wt_3;
            window->widgets[widx::construct].tooltip = StringIds::tooltip_construct;
            window->widgets[widx::rotate_90].type = WidgetType::wt_9;
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
        window->activated_widgets = activatedWidgets;
        window->invalidate();
    }

    // 0x0049F1B5
    void activateSelectedConstructionWidgets()
    {
        auto window = WindowManager::find(WindowType::construction);

        if (window == nullptr)
            return;

        if (window->current_tab == Common::widx::tab_construction - Common::widx::tab_construction)
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
        self->enabled_widgets &= ~(1 << widx::construct);

        if (_constructionHover != 1)
            self->enabled_widgets |= (1 << widx::construct);

        auto disabledWidgets = self->disabled_widgets;
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
        Common::sub_49FEC7();

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

            if (bridgeObj->max_speed == 0xFFFF)
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
                Common::sub_49FEC7();
                _lastSelectedTrackPiece = TrackPiece::s_bend_to_dual_track;
                _trackCost = 0x80000000;
                if (self->widgets[widx::s_bend_dual_track_left].image != ImageIds::construction_s_bend_dual_track_left)
                {
                    _lastSelectedTrackPiece = TrackPiece::turnaround;
                    if (self->widgets[widx::s_bend_dual_track_left].image != ImageIds::construction_right_turnaround)
                    {
                        if (self->widgets[widx::s_bend_dual_track_left].image != ImageIds::construction_left_turnaround)
                            _lastSelectedTrackPiece = TrackPiece::s_bend_to_single_track;
                    }
                }
                activateSelectedConstructionWidgets();
                break;
            }

            case widx::s_bend_dual_track_right:
            {
                _byte_113603A = 0xFF;
                Common::sub_49FEC7();
                _lastSelectedTrackPiece = TrackPiece::s_bend_to_single_track;
                _trackCost = 0x80000000;
                if (self->widgets[widx::s_bend_dual_track_right].image != ImageIds::construction_s_bend_dual_track_right)
                {
                    _lastSelectedTrackPiece = TrackPiece::turnaround;
                    if (self->widgets[widx::s_bend_dual_track_left].image != ImageIds::construction_left_turnaround)
                        _lastSelectedTrackPiece = TrackPiece::s_bend_to_dual_track;
                }
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
                Common::sub_49FEC7();
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

    // 0x004A2395
    static std::optional<int16_t> getConstructionHeight(const Pos2& mapPos, int16_t height, bool isSelected)
    {
        auto tile = TileManager::get(mapPos);

        auto surfaceTile = tile.surface();

        if (surfaceTile == nullptr)
            return std::nullopt;

        int16_t tileHeight = surfaceTile->baseZ() * 4;

        if (surfaceTile->slopeCorners())
        {
            tileHeight += 16;
        }

        if (surfaceTile->isSlopeDoubleHeight())
        {
            tileHeight += 16;
        }

        if (isSelected)
        {
            if (tileHeight > height)
            {
                height = tileHeight;
            }
        }
        else
        {
            if (tileHeight > _word_1136000)
            {
                height = _word_1136000;
            }
        }

        if (isSelected)
        {
            if (surfaceTile->water())
            {
                tileHeight = surfaceTile->water() * 16;
                tileHeight += 16;

                if (tileHeight > height)
                {
                    height = tileHeight;
                }
            }
        }
        else
        {
            tileHeight = surfaceTile->water() * 16;
            if (tileHeight > height)
            {
                height = tileHeight;
            }
        }

        return height;
    }

    // 0x00478361
    static std::optional<std::pair<int16_t, int16_t>> sub_478361(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        auto flags = call(0x00478361, regs);

        if (flags & (1 << 8))
            return std::nullopt;

        return { std::make_pair(regs.di, regs.dl) };
    }

    // 0x004A4011
    static std::optional<std::pair<int16_t, int16_t>> sub_4A4011(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        auto flags = call(0x004A4011, regs);

        if (flags & (1 << 8))
            return std::nullopt;

        return { std::make_pair(regs.di, regs.dl) };
    }

    // 0x00460781
    // regs.ax = x;
    // regs.bx = y;
    // returns
    // regs.ax = 0x8000 - probably in case of failure
    static Pos2 sub_460781(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        call(0x00460781, regs);

        Pos2 mapPos = { regs.ax, regs.bx };

        return mapPos;
    }

    static void constructionLoop(Pos2 mapPos, uint32_t maxRetries, int16_t height)
    {
        while (true)
        {
            _constructionHover = 0;
            _byte_113607E = 0;
            _x = mapPos.x;
            _y = mapPos.y;
            _word_1135FB8 = height;
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

    // 0x0049DC8C
    static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049DC8C, regs);
    }

    // 0x0049DC97
    static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::construct)
            return;

        if (_trackType & (1 << 7))
        {
            mapInvalidateMapSelectionTiles();
            Common::sub_49FEC7();

            auto road = getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            if (!road)
                return;

            _byte_1136065 = road->id;
            int16_t roadHeight = 0;

            auto i = 0;
            if (Input::hasMapSelectionFlag(Input::MapSelectionFlags::enableConstruct))
            {
                for (auto& tile = _mapSelectedTiles[i]; tile.x != -1; tile = _mapSelectedTiles[++i])
                {
                    if (tile.x >= 0x2FFF)
                        continue;

                    if (tile.y >= 0x2FFF)
                        continue;

                    auto height = getConstructionHeight(_mapSelectedTiles[i], roadHeight, true);

                    if (height)
                        roadHeight = *height;
                }
            }
            // loc_4A23F8
            _word_1136000 = roadHeight;
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable | Input::MapSelectionFlags::enableConstruct | Input::MapSelectionFlags::unk_02);

            auto height = sub_478361(x, y);
            Pos2 mapPos;

            if (height)
            {
                auto pos = screenGetMapXyWithZ(xy32(x, y), height->first);
                if (pos)
                {
                    mapPos.x = pos->x;
                    mapPos.y = pos->y;
                    mapPos.x &= 0xFFE0;
                    mapPos.y &= 0xFFE0;
                    _byte_113605D = 1;
                    _word_1135FFE = roadHeight;
                }
                else
                {
                    mapPos.x = -32768;
                }
            }

            if (!height || mapPos.x == -32768)
            {
                mapPos = sub_460781(x, y);

                if (mapPos.x == -32768)
                    return;

                auto constructionHeight = getConstructionHeight(mapPos, roadHeight, false);

                if (constructionHeight)
                    roadHeight = *constructionHeight;

                _byte_113605D = 0;
            }
            Input::toolCancel();

            auto maxRetries = 0;
            if (Input::hasKeyModifier(Input::KeyModifier::shift) || _byte_113605D != 1)
            {
                const auto& roadPiece = Map::TrackData::getRoadPiece(_byte_1136065);
                auto maxRoadPieceHeight = 0;

                for (const auto& roadPart : roadPiece)
                {
                    if (maxRoadPieceHeight > roadPart.z)
                        maxRoadPieceHeight = roadPart.z;
                }

                roadHeight -= maxRoadPieceHeight;
                roadHeight -= 16;
                maxRetries = 2;

                if (Input::hasKeyModifier(Input::KeyModifier::shift))
                {
                    maxRetries = 0x80000008;
                    roadHeight -= 16;
                }
            }
            else
            {
                maxRetries = 1;
                roadHeight = _word_1135FFE;
            }

            constructionLoop(mapPos, maxRetries, roadHeight);
        }
        else
        {
            mapInvalidateMapSelectionTiles();
            Common::sub_49FEC7();

            auto track = getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            if (!track)
                return;

            _byte_1136065 = track->id;
            int16_t trackHeight = 0;
            auto i = 0;

            if (Input::hasMapSelectionFlag(Input::MapSelectionFlags::enableConstruct))
            {
                for (auto& tile = _mapSelectedTiles[i]; tile.x != -1; tile = _mapSelectedTiles[++i])
                {
                    if (tile.x >= 0x2FFF)
                        continue;

                    if (tile.y >= 0x2FFF)
                        continue;

                    auto height = getConstructionHeight(_mapSelectedTiles[i], trackHeight, true);

                    if (height)
                        trackHeight = *height;
                }
            }
            _word_1136000 = trackHeight;
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable | Input::MapSelectionFlags::enableConstruct | Input::MapSelectionFlags::unk_02);

            auto height = sub_4A4011(x, y);
            Pos2 mapPos;

            if (height)
            {
                if (_word_4F7B62[height->second] == 0)
                {
                    auto pos = screenGetMapXyWithZ(xy32(x, y), height->first);
                    if (pos)
                    {
                        mapPos.x = pos->x;
                        mapPos.y = pos->y;
                        mapPos.x &= 0xFFE0;
                        mapPos.y &= 0xFFE0;
                        _byte_113605D = 1;
                        _word_1135FFE = trackHeight;
                    }
                    else
                    {
                        mapPos.x = -32768;
                    }
                }
            }

            if (!height || mapPos.x == -32768 || _word_4F7B62[track->id * 8] != 0)
            {
                mapPos = sub_460781(x, y);

                if (mapPos.x == -32768)
                    return;

                auto constructionHeight = getConstructionHeight(mapPos, trackHeight, false);

                if (constructionHeight)
                    trackHeight = *constructionHeight;

                _byte_113605D = 0;
            }
            Input::toolCancel();

            auto maxRetries = 0;
            if (Input::hasKeyModifier(Input::KeyModifier::shift) || _byte_113605D != 1)
            {
                const auto& trackPiece = Map::TrackData::getTrackPiece(_byte_1136065);
                auto maxTrackPieceHeight = 0;

                for (const auto& trackPart : trackPiece)
                {
                    if (maxTrackPieceHeight > trackPart.z)
                        maxTrackPieceHeight = trackPart.z;
                }

                trackHeight -= maxTrackPieceHeight;
                trackHeight -= 16;
                maxRetries = 2;

                if (Input::hasKeyModifier(Input::KeyModifier::shift))
                {
                    maxRetries = 0x80000008;
                    trackHeight -= 16;
                }
            }
            else
            {
                maxRetries = 1;
                trackHeight = _word_1135FFE;
            }

            constructionLoop(mapPos, maxRetries, trackHeight);
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
                if (bridgeObj->max_speed == 0xFFFF)
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
    static void drawTrackCost(Window* self, Gfx::Context* clipped, Gfx::Context* context, xy32 pos, uint16_t width, uint16_t height)
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
    static void drawRoadCost(Window* self, Gfx::Context* clipped, Gfx::Context* context, xy32 pos, uint16_t width, uint16_t height)
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

            Gfx::Context* clipped = nullptr;

            if (Gfx::clipContext(&clipped, context, x, y, width, height))
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

                auto pos2D = coordinate3dTo2d(pos3D.x, pos3D.y, pos3D.z, gCurrentRotation);
                xy32 pos = { pos2D.x, pos2D.y };
                drawRoadCost(self, clipped, context, pos, width, height);
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

            Gfx::Context* clipped = nullptr;

            if (Gfx::clipContext(&clipped, context, x, y, width, height))
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

                auto pos2D = coordinate3dTo2d(pos3D.x, pos3D.y, pos3D.z, gCurrentRotation);
                xy32 pos = { pos2D.x, pos2D.y };
                drawTrackCost(self, clipped, context, pos, width, height);
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
        events.on_close = Common::onClose;
        events.on_mouse_up = onMouseUp;
        events.on_resize = onResize;
        events.on_mouse_down = onMouseDown;
        events.on_dropdown = onDropdown;
        events.on_update = onUpdate;
        events.on_tool_update = onToolUpdate;
        events.on_tool_down = onToolDown;
        events.cursor = cursor;
        events.prepare_draw = prepareDraw;
        events.draw = draw;
    }

}
