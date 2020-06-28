#include "../../audio/audio.h"
#include "../../companymgr.h"
#include "../../graphics/image_ids.h"
#include "../../input.h"
#include "../../localisation/FormatArguments.hpp"
#include "../../objects/bridge_object.h"
#include "../../objects/objectmgr.h"
#include "../../objects/road_object.h"
#include "../../objects/track_object.h"
#include "../../ui/dropdown.h"
#include "Construction.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction::construction
{
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

    window_event_list events;

    // 0x0049F92D
    static void constructTrack(window* self, widget_index widgetIndex)
    {
        registers regs;
        regs.edx = widgetIndex;
        regs.esi = (int32_t)self;
        call(0x0049F92D, regs);
    }

    // 0x004A0121
    static void removeTrack(window* self, widget_index widgetIndex)
    {
        registers regs;
        regs.edx = widgetIndex;
        regs.esi = (int32_t)self;
        call(0x004A0121, regs);
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
        regs.esi = (int32_t)self;
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
                    constructTrack(self, widgetIndex);
                }
                break;

            case widx::remove:
                for (int i = 0; i < multiplier; i++)
                {
                    removeTrack(self, widgetIndex);
                }
                break;

            case widx::rotate_90:
            {
                if (_constructionHover == 1)
                {
                    _constructionRotation++;
                    _constructionRotation = _constructionRotation & 3;
                    _trackCost = 0x80000000;
                    common::activateSelectedConstructionWidgets();
                    break;
                }
                common::sub_49FEC7();
                WindowManager::viewportSetVisibility(WindowManager::viewport_visibility::overgroundView);
                input::toolSet(self, widx::construct, 12);
                input::set_flag(input::input_flags::flag6);

                _constructionHover = 1;
                _byte_113607E = 0;
                _constructionRotation = _constructionRotation & 3;

                common::activateSelectedConstructionWidgets();
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
    static void disableUnusedRoadPieces(window* self, uint64_t disabledWidgets)
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
            auto road = common::getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);
            if (!road)
                disabledWidgets |= (1 << widx::construct);
        }
        self->set_disabled_widgets_and_invalidate(disabledWidgets);
    }

    // 0x0049DB1F
    static void disableUnusedTrackPieces(window* self, track_object trackObj, uint64_t disabledWidgets)
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
            auto track = common::getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            if (!track)
                disabledWidgets |= (1 << widx::construct);
        }
        self->set_disabled_widgets_and_invalidate(disabledWidgets);
    }

    // 0x0049DAF3
    static void disableTrackSlopes(window* self, track_object trackObj, uint64_t disabledWidgets)
    {
        auto trackPieces = trackObj.track_pieces & ((1 << 5) | (1 << 8));

        if (trackPieces != ((1 << 5) | (1 << 8)))
            disabledWidgets |= (1 << widx::slope_down) | (1 << widx::slope_up);

        trackPieces = trackObj.track_pieces & ((1 << 6) | (1 << 8));

        if (trackPieces != ((1 << 6) | (1 << 8)))
            disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::steep_slope_up);

        disableUnusedTrackPieces(self, trackObj, disabledWidgets);
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

            if (_lastSelectedTrackPiece == 0xFF)
            {
                disableUnusedRoadPieces(self, disabledWidgets);
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
                    disableUnusedRoadPieces(self, disabledWidgets);
                    break;
                }

                case common::trackPiece::left_hand_curve_very_small:
                case common::trackPiece::right_hand_curve_very_small:
                case common::trackPiece::left_hand_curve_small:
                case common::trackPiece::right_hand_curve_small:
                case common::trackPiece::turnaround:
                {
                    disabledWidgets |= (1 << widx::steep_slope_down) | (1 << widx::slope_down) | (1 << widx::slope_up) | (1 << widx::steep_slope_up);
                    disableUnusedRoadPieces(self, disabledWidgets);
                    break;
                }
            }
        }
        else
        {
            auto trackObj = objectmgr::get<track_object>(trackType);
            if (_lastSelectedTrackPiece == 0xFF)
            {
                disableUnusedTrackPieces(self, *trackObj, disabledWidgets);
                return;
            }
            switch (_lastSelectedTrackPiece)
            {
                case common::trackPiece::straight:
                    disableUnusedTrackPieces(self, *trackObj, disabledWidgets);
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
                    disableUnusedTrackPieces(self, *trackObj, disabledWidgets);
                    break;
                }

                case common::trackPiece::left_hand_curve_small:
                case common::trackPiece::right_hand_curve_small:
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
        common::sub_49FEC7();

        if (slope)
            _lastSelectedTrackGradient = trackPiece;
        else
            _lastSelectedTrackPiece = trackPiece;

        _trackCost = 0x80000000;
        common::activateSelectedConstructionWidgets();
    }

    // 0x0049D83A
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
                common::activateSelectedConstructionWidgets();
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
                common::activateSelectedConstructionWidgets();
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

                // TODO: & ~(1 << 7) added to prevent crashing when selecting bridges for road/trams
                _scenarioBridges[_trackType & ~(1 << 7)] = bridge;
                common::sub_49FEC7();
                _trackCost = 0x80000000;
                common::activateSelectedConstructionWidgets();
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
            if (input::is_tool_active(WindowType::construction, self->number))
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

    // 0x004A2395
    static std::optional<int16_t> getConstructionHeight(const map_pos& mapPos, int16_t height, bool isSelected)
    {
        auto tile = tilemgr::get(mapPos);

        auto surfaceTile = tile.surface();

        if (surfaceTile == nullptr)
            return std::nullopt;

        int16_t tileHeight = surfaceTile->base_z() * 4;

        if (surfaceTile->slope_corners())
        {
            tileHeight += 16;
        }

        if (surfaceTile->is_slope_dbl_height())
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
    static std::optional<int16_t> sub_478361(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        auto flags = call(0x00478361, regs);

        if (flags & (1 << 8))
            return std::nullopt;

        return regs.di;
    }

    // 0x004A4011
    static std::optional<int16_t> sub_4A4011(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        auto flags = call(0x004A4011, regs);

        if (flags & (1 << 8))
            return std::nullopt;

        return regs.di;
    }

    // 0x00460781
    static map_pos sub_460781(int16_t x, int16_t y)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        call(0x00460781, regs);

        map_pos mapPos = { regs.ax, regs.bx };

        return mapPos;
    }

    static void constructionLoop(map_pos mapPos, uint32_t maxRetries, int16_t height)
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

            common::activateSelectedConstructionWidgets();
            auto window = WindowManager::find(WindowType::construction);

            if (window == nullptr)
                return;

            _byte_508F09 = _byte_508F09 | (1 << 0);

            on_mouse_up(window, widx::construct);

            _byte_508F09 = _byte_508F09 & ~(1 << 0);

            if (_dword_1135F42 == 0x80000000)
            {
                if (gGameCommandErrorText != string_ids::error_can_only_build_above_ground)
                {
                    maxRetries--;
                    if (maxRetries != 0)
                    {
                        height -= 16;
                        if (height >= 0)
                        {
                            if (input::has_key_modifier(input::key_modifier::shift))
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

            on_mouse_up(window, widx::rotate_90);

            audio::play_sound(audio::sound_id::error, int32_t(input::getMouseLocation().x));

            return;
        }
    }
    // 0x0049DC97
    static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::construct)
            return;

        if (_trackType & (1 << 7))
        {
            map_invalidate_map_selection_tiles();
            common::sub_49FEC7();

            auto road = common::getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            if (!road)
                return;

            _byte_1136065 = road->id;
            int16_t roadHeight = 0;

            auto i = 0;
            if (_mapSelectionFlags & (1 << 1))
            {
                for (auto& tile = _mapSelectedTiles[i]; tile.x != -1; tile = _mapSelectedTiles[++i])
                {
                    if (tile.x >= 0x2FFF)
                        continue;

                    if (tile.y >= 0x2FFF)
                        continue;

                    auto height = getConstructionHeight(_mapSelectedTiles[i], roadHeight, true);

                    if (height)
                        roadHeight = height.value();
                }
            }
            // loc_4A23F8
            _word_1136000 = roadHeight;
            _mapSelectionFlags = _mapSelectionFlags & ~((1 << 2) | (1 << 1) | (1 << 0));

            auto height = sub_478361(x, y);
            map_pos mapPos;

            if (height)
            {
                auto pos = screenGetMapXyWithZ(xy32(x, y), roadHeight * 8 | height.value());
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
                    roadHeight = constructionHeight.value();

                _byte_113605D = 0;
            }
            input::cancel_tool();

            auto maxRetries = 0;
            if (input::has_key_modifier(input::key_modifier::shift) || _byte_113605D != 1)
            {
                auto roadPiece = common::roadPieces[_byte_1136065];
                i = 0;
                auto maxRoadPieceHeight = 0;

                while (roadPiece[i].index != 0xFF)
                {
                    if (maxRoadPieceHeight > roadPiece[i].z)
                        maxRoadPieceHeight = roadPiece[i].z;
                    i++;
                }

                roadHeight -= maxRoadPieceHeight;
                roadHeight -= 16;
                maxRetries = 2;

                if (input::has_key_modifier(input::key_modifier::shift))
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
            map_invalidate_map_selection_tiles();
            common::sub_49FEC7();

            auto track = common::getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            if (!track)
                return;

            _byte_1136065 = track->id;
            int16_t trackHeight = 0;
            auto i = 0;

            if (_mapSelectionFlags & (1 << 1))
            {
                for (auto& tile = _mapSelectedTiles[i]; tile.x != -1; tile = _mapSelectedTiles[++i])
                {
                    if (tile.x >= 0x2FFF)
                        continue;

                    if (tile.y >= 0x2FFF)
                        continue;

                    auto height = getConstructionHeight(_mapSelectedTiles[i], trackHeight, true);

                    if (height)
                        trackHeight = height.value();
                }
            }
            _word_1136000 = trackHeight;
            _mapSelectionFlags = _mapSelectionFlags & ~((1 << 2) | (1 << 1) | (1 << 0));

            auto height = sub_4A4011(x, y);
            map_pos mapPos;

            if (height)
            {
                if (_word_4F7B62[trackHeight * 8] == 0)
                {
                    auto pos = screenGetMapXyWithZ(xy32(x, y), trackHeight * 8 | height.value());
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

            if (!height || mapPos.x == -32768)
            {
                mapPos = sub_460781(x, y);

                if (mapPos.x == -32768)
                    return;

                auto constructionHeight = getConstructionHeight(mapPos, trackHeight, false);

                if (constructionHeight)
                    trackHeight = constructionHeight.value();

                _byte_113605D = 0;
            }
            input::cancel_tool();

            auto maxRetries = 0;
            if (input::has_key_modifier(input::key_modifier::shift) || _byte_113605D != 1)
            {
                auto trackPiece = common::trackPieces[_byte_1136065];
                i = 0;
                auto maxTrackPieceHeight = 0;

                while (trackPiece[i].index != 0xFF)
                {
                    if (maxTrackPieceHeight > trackPiece[i].z)
                        maxTrackPieceHeight = trackPiece[i].z;
                    i++;
                }

                trackHeight -= maxTrackPieceHeight;
                trackHeight -= 16;
                maxRetries = 2;

                if (input::has_key_modifier(input::key_modifier::shift))
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
    static void drawCostString(window* self, gfx::drawpixelinfo_t* dpi)
    {
        auto x = self->widgets[widx::construct].mid_x();
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

    // 0x0049D106
    static void drawTrackCost(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, xy32 pos, uint16_t width, uint16_t height)
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

        drawCostString(self, dpi);
    }

    // 0x0049D325
    static void drawRoadCost(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, xy32 pos, uint16_t width, uint16_t height)
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

        drawCostString(self, dpi);
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
            auto road = common::getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

            _word_1135FD8 = _lastSelectedMods;

            if (!road)
                return;

            _byte_1136077 = _trackType;
            _byte_1136078 = road->rotation;
            _lastSelectedTrackPieceId = road->id;
            _word_1135FD6 = (_lastSelectedBridge << 8) & 0x1F;

            auto x = self->x + self->widgets[widx::construct].left + 1;
            auto y = self->y + self->widgets[widx::construct].top + 1;
            auto width = self->widgets[widx::construct].width();
            auto height = self->widgets[widx::construct].height();

            gfx::drawpixelinfo_t* clipped = nullptr;

            if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
            {
                auto roadPiece = common::roadPieces[_lastSelectedTrackPieceId];
                auto i = 0;

                while (roadPiece[i + 1].index != 0xFF)
                {
                    i++;
                }

                map_pos3 pos3D = { roadPiece[i].x, roadPiece[i].y, roadPiece[i].z };

                if (roadPiece[i].flags & (1 << 6))
                {
                    pos3D.x = 0;
                    pos3D.y = 0;
                }

                auto rotatedPos = rotate2DCoordinate({ pos3D.x, pos3D.y }, _byte_1136078 & 3);
                pos3D.x = rotatedPos.x / 2;
                pos3D.y = rotatedPos.y / 2;
                pos3D.x += 0x2010;
                pos3D.y += 0x2010;
                pos3D.z += 0x1CC;

                auto pos2D = coordinate_3d_to_2d(pos3D.x, pos3D.y, pos3D.z, gCurrentRotation);
                xy32 pos = { pos2D.x, pos2D.y };
                drawRoadCost(self, clipped, dpi, pos, width, height);
            }
            else
            {
                drawCostString(self, dpi);
            }
        }
        else
        {
            auto track = common::getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

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

            gfx::drawpixelinfo_t* clipped = nullptr;

            if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
            {
                auto trackPiece = common::trackPieces[_lastSelectedTrackPieceId];
                auto i = 0;

                while (trackPiece[i + 1].index != 0xFF)
                {
                    i++;
                }

                map_pos3 pos3D = { trackPiece[i].x, trackPiece[i].y, trackPiece[i].z };

                if (trackPiece[i].flags & (1 << 6))
                {
                    pos3D.x = 0;
                    pos3D.y = 0;
                }

                auto rotatedPos = rotate2DCoordinate({ pos3D.x, pos3D.y }, _byte_1136078 & 3);
                pos3D.x = rotatedPos.x / 2;
                pos3D.y = rotatedPos.y / 2;
                pos3D.x += 0x2010;
                pos3D.y += 0x2010;
                pos3D.z += 0x1CC;

                auto pos2D = coordinate_3d_to_2d(pos3D.x, pos3D.y, pos3D.z, gCurrentRotation);
                xy32 pos = { pos2D.x, pos2D.y };
                drawTrackCost(self, clipped, dpi, pos, width, height);
            }
            else
            {
                drawCostString(self, dpi);
            }
        }
    }

    void tabReset(window* self)
    {
        if (_constructionHover != 0)
        {
            _constructionHover = 0;
            _byte_113607E = 1;
            self->call_on_mouse_up(construction::widx::rotate_90);
        }
    }

    void init_events()
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
