#include "Construction.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction
{
    namespace construction
    {
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

        // 0x0049DBEC
        static void disableUnusedRoadPieces(window* self, uint64_t disabledWidgets)
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

            if (_constructionRotation < 12)
            {
                if (_constructionRotation >= 8)
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
                auto road = common::getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);
                if (!road)
                    disabledWidgets |= (1 << widx::construct);
            }
            self->set_disabled_widgets_and_invalidate(disabledWidgets);
        }

        // 0x0049DB1F
        static void disableUnusedTrackPieces(window* self, track_object trackObj, uint64_t disabledWidgets)
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

            if (_constructionRotation < 12)
            {
                if (_constructionRotation >= 8)
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

                    // TODO: & ~(1 << 7) added to prevent crashing when selecting/deselecting overhead wires for trams
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

        static void getConstructionHeight(map_pos mapPos, int16_t* height, bool isSelected)
        {
            auto tile = tilemgr::get(mapPos);

            auto tileElement = tile.begin();

            if (tileElement->type() != element_type::surface)
            {
                while (tileElement->type() != element_type::surface)
                {
                    tileElement += 8;
                }
            }

            auto surfaceTile = tileElement->as_surface();

            if (surfaceTile == nullptr)
                return;

            int16_t tileHeight = surfaceTile->base_z() << 2;

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
                if (tileHeight > *height)
                {
                    *height = tileHeight;
                }
            }
            else
            {
                if (tileHeight > _word_1136000)
                {
                    *height = _word_1136000;
                }
            }

            if (surfaceTile->water())
            {
                tileHeight = surfaceTile->water() << 4;
                tileHeight += 16;
            }

            if (tileHeight > *height)
            {
                *height = tileHeight;
            }
        }

        // 0x0049DC97
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            if (widgetIndex != 28)
                return;

            if (_trackType & (1 << 7))
            {
                map_invalidate_map_selection_tiles();
                common::sub_49FEC7();

                auto road = common::getRoadPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

                if (!road)
                    return;

                _byte_1136065 = (*road).id;
                int16_t roadHeight = 0;

                auto i = 0;
                if (_mapSelectionFlags & 1 << 1)
                {
                    auto xPos = _mapSelectedTiles[i].x;
                    while (xPos != -1)
                    {
                        auto yPos = _mapSelectedTiles[i].y;

                        if (xPos >= 0x2FFF)
                        {
                            i++;
                            xPos = _mapSelectedTiles[i].x;
                            continue;
                        }

                        if (yPos >= 0x2FFF)
                        {
                            i++;
                            xPos = _mapSelectedTiles[i].x;
                            continue;
                        }

                        getConstructionHeight(_mapSelectedTiles[i], &roadHeight, true);

                        i++;
                        xPos = _mapSelectedTiles[i].x;
                    }
                }
                // loc_4A23F8
                _word_1136000 = roadHeight;
                _mapSelectionFlags = _mapSelectionFlags & ~(0x7);

                registers regs2;
                regs2.ax = x;
                regs2.bx = y;
                auto flags = call(0x00478361, regs2);
                map_pos mapPos = { regs2.ax, regs2.bx };

                if (!(flags & 1 << 8))
                {
                    auto pos = screenGetMapXyWithZ(xy32(x, y), roadHeight << 3);
                    if (pos)
                    {
                        mapPos.x = (*pos).x;
                        mapPos.y = (*pos).y;
                        mapPos.x &= 0xFFE0;
                        mapPos.y &= 0xFFE0;
                        _byte_113605D = 1;
                        _word_1135FFE = roadHeight;
                    }
                    else
                    {
                        mapPos.x = -1;
                    }
                }

                if (flags & 1 << 8 || mapPos.x == -1)
                {
                    registers regs3;
                    regs3.ax = x;
                    regs3.bx = y;
                    regs3.edi = _mapSelectedTiles[i].x << 16 | _mapSelectedTiles[i].x;
                    call(0x00460781, regs3);

                    if (regs3.ax == -32768)
                        return;

                    mapPos.x = regs3.ax;
                    mapPos.y = regs3.bx;

                    getConstructionHeight(mapPos, &roadHeight, true);

                    _byte_113605D = 0;
                }
                input::cancel_tool();

                auto ebx = 0;
                if (input::has_key_modifier(input::key_modifier::shift) || _byte_113605D != 1)
                {
                    auto roadPiece = common::roadPieces[_byte_1136065];
                    i = 0;
                    auto height = 0;

                    while (roadPiece[i].index != 0xFF)
                    {
                        if (height > roadPiece[i].z)
                            height = roadPiece[i].z;
                        i++;
                    }

                    roadHeight -= height;
                    roadHeight -= 16;
                    ebx = 2;

                    if (input::has_key_modifier(input::key_modifier::shift))
                    {
                        ebx = 0x80000000;
                        roadHeight -= 16;
                    }
                }
                else
                {
                    ebx = 1;
                    roadHeight = _word_1135FFE;
                }

                while (true)
                {
                    _constructionHover = 0;
                    _byte_113607E = 0;
                    _x = mapPos.x;
                    _y = mapPos.y;
                    _word_1135FB8 = roadHeight;
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
                            ebx--;
                            if (ebx != 0)
                            {
                                roadHeight -= 16;
                                if (roadHeight >= 0)
                                {
                                    if (ebx < 0)
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        roadHeight += 32;
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
            else
            {
                map_invalidate_map_selection_tiles();
                common::sub_49FEC7();

                auto track = common::getTrackPieceId(_lastSelectedTrackPiece, _lastSelectedTrackGradient, _constructionRotation);

                if (!track)
                    return;

                _byte_1136065 = (*track).id;
                int16_t trackHeight = 0;
                auto i = 0;

                if (_mapSelectionFlags & 1 << 1)
                {
                    auto xPos = _mapSelectedTiles[i].x;
                    while (xPos != -1)
                    {
                        auto yPos = _mapSelectedTiles[i].y;

                        if (xPos >= 0x2FFF)
                        {
                            i++;
                            xPos = _mapSelectedTiles[i].x;
                            continue;
                        }

                        if (yPos >= 0x2FFF)
                        {
                            i++;
                            xPos = _mapSelectedTiles[i].x;
                            continue;
                        }

                        getConstructionHeight(_mapSelectedTiles[i], &trackHeight, true);

                        i++;
                        xPos = _mapSelectedTiles[i].x;
                    }
                }
                _word_1136000 = trackHeight;
                _mapSelectionFlags = _mapSelectionFlags & ~(0x7);

                registers regs2;
                regs2.ax = x;
                regs2.bx = y;
                auto flags = call(0x004A4011, regs2);
                map_pos mapPos = { regs2.ax, regs2.bx };

                if (!(flags & 1 << 8))
                {
                    if (_word_4F7B62[trackHeight << 3] == 0)
                    {
                        auto pos = screenGetMapXyWithZ(xy32(x, y), trackHeight << 3);
                        if (pos)
                        {
                            mapPos.x = (*pos).x;
                            mapPos.y = (*pos).y;
                            mapPos.x &= 0xFFE0;
                            mapPos.y &= 0xFFE0;
                            _byte_113605D = 1;
                            _word_1135FFE = trackHeight;
                        }
                        else
                        {
                            mapPos.x = -1;
                        }
                    }
                }

                if (flags & 1 << 8 || mapPos.x == -1)
                {
                    registers regs3;
                    regs3.ax = x;
                    regs3.bx = y;
                    regs3.edi = _mapSelectedTiles[i].x << 16 | _mapSelectedTiles[i].x;
                    call(0x00460781, regs3);

                    if (regs3.ax == -32768)
                        return;

                    mapPos.x = regs3.ax;
                    mapPos.y = regs3.bx;

                    getConstructionHeight(mapPos, &trackHeight, false);

                    _byte_113605D = 0;
                }
                input::cancel_tool();

                auto ebx = 0;
                if (input::has_key_modifier(input::key_modifier::shift) || _byte_113605D != 1)
                {
                    auto trackPiece = common::trackPieces[_byte_1136065];
                    i = 0;
                    auto height = 0;

                    while (trackPiece[i].index != 0xFF)
                    {
                        if (height > trackPiece[i].z)
                            height = trackPiece[i].z;
                        i++;
                    }

                    trackHeight -= height;
                    trackHeight -= 16;
                    ebx = 2;

                    if (input::has_key_modifier(input::key_modifier::shift))
                    {
                        ebx = 0x80000000;
                        trackHeight -= 16;
                    }
                }
                else
                {
                    ebx = 1;
                    trackHeight = _word_1135FFE;
                }

                while (true)
                {
                    _constructionHover = 0;
                    _byte_113607E = 0;
                    _x = mapPos.x;
                    _y = mapPos.y;
                    _word_1135FB8 = trackHeight;
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
                            ebx--;
                            if (ebx != 0)
                            {
                                trackHeight -= 16;
                                if (trackHeight >= 0)
                                {
                                    if (ebx < 0)
                                    {
                                        continue;
                                    }
                                    else
                                    {
                                        trackHeight += 32;
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
        void drawTrack(uint16_t x, uint16_t y, uint32_t edi, uint8_t bh, uint32_t edx)
        {
            registers regs;
            regs.ax = x;
            regs.cx = y;
            regs.edi = edi;
            regs.bh = bh;
            regs.edx = edx;
            call(0x004A0AE5, regs);
        }

        // 0x00478F1F
        void drawRoad(uint16_t x, uint16_t y, uint32_t edi, uint8_t bh, uint32_t edx)
        {
            registers regs;
            regs.ax = x;
            regs.cx = y;
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

        // 0x0049D106
        static void drawTrackCost(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, map_pos pos, uint16_t width, uint16_t height)
        {
            width >>= 1;
            height >>= 1;
            height += 16;
            pos.x -= width;
            pos.y -= height;
            clipped->x += pos.x;
            clipped->y += pos.y;
            _dword_E0C3E0 = (uint32_t)clipped;

            uint32_t edi = _word_1135FD8 << 16 | 0x1E0;
            uint32_t trackPieceImage = _word_1135FD6 << 16 | _lastSelectedTrackPieceId << 8 | _byte_1136077;
            _byte_522095 = _byte_522095 | (1 << 1);

            drawTrack(0x2000, 0x2000, edi, _byte_1136078, trackPieceImage);

            _byte_522095 = _byte_522095 & ~(1 << 1);

            drawCostString(self, dpi);
        }

        // 0x0049D325
        static void drawRoadCost(window* self, gfx::drawpixelinfo_t* clipped, gfx::drawpixelinfo_t* dpi, map_pos pos, uint16_t width, uint16_t height)
        {
            width >>= 1;
            height >>= 1;
            height += 16;
            pos.x -= width;
            pos.y -= height;
            clipped->x += pos.x;
            clipped->y += pos.y;
            _dword_E0C3E0 = (uint32_t)clipped;

            uint32_t edi = _word_1135FD8 << 16 | 0x1E0;
            uint32_t roadPieceImage = _word_1135FD6 << 16 | _lastSelectedTrackPieceId << 8 | _byte_1136077;
            _byte_522095 = _byte_522095 | (1 << 1);

            drawRoad(0x2000, 0x2000, edi, _byte_1136078, roadPieceImage);

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
                _byte_1136078 = (*road).rotation;
                _lastSelectedTrackPieceId = (*road).id;
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

                    drawRoadCost(self, clipped, dpi, pos2D, width, height);
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
                _byte_1136078 = (*track).rotation;
                _lastSelectedTrackPieceId = (*track).id;
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

                    drawTrackCost(self, clipped, dpi, pos2D, width, height);
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
}
