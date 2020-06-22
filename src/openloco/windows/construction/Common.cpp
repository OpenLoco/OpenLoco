#include "Construction.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction
{
    static window* nonTrackWindow()
    {
        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            common::setDisabledWidgets(window);
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
            common::setDisabledWidgets(window);
        }

        common::activateSelectedConstructionWidgets();
        window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            window->call_on_mouse_up(construction::widx::rotate_90);
        }
        return window;
    }

    static window* createTrackConstructionWindow()
    {
        common::createConstructionWindow();

        common::refreshSignalList(_signalList, _trackType);

        auto lastSignal = _scenarioSignals[_trackType];

        if (lastSignal == 0xFF)
            lastSignal = _signalList[0];

        _lastSelectedSignal = lastSignal;

        common::refreshStationList(_stationList, _trackType, TransportMode::road);

        auto lastStation = _scenarioTrainStations[_trackType];

        if (lastStation == 0xFF)
            lastStation = _stationList[0];

        _lastSelectedStationType = lastStation;

        common::refreshBridgeList(_bridgeList, _trackType, TransportMode::rail);

        auto lastBridge = _scenarioBridges[_trackType];

        if (lastBridge == 0xFF)
            lastBridge = _bridgeList[0];

        _lastSelectedBridge = lastBridge;

        common::refreshModList(_modList, _trackType, TransportMode::rail);

        auto lastMod = _scenarioTrackMods[_trackType];

        if (lastMod == 0xFF)
            lastMod = 0;

        _lastSelectedMods = lastMod;
        _byte_113603A = 0;

        return trackWindow();
    }

    static window* createRoadConstructionWindow()
    {
        common::createConstructionWindow();

        _lastSelectedSignal = 0xFF;

        common::refreshStationList(_stationList, _trackType, TransportMode::road);

        auto lastStation = _scenarioRoadStations[(_trackType & ~(1ULL << 7))];

        if (lastStation == 0xFF)
            lastStation = _stationList[0];

        _lastSelectedStationType = lastStation;

        common::refreshBridgeList(_bridgeList, _trackType, TransportMode::road);

        auto lastBridge = _scenarioBridges[(_trackType & ~(1ULL << 7))];

        if (lastBridge == 0xFF)
            lastBridge = _bridgeList[0];

        _lastSelectedBridge = lastBridge;

        common::refreshModList(_modList, _trackType, TransportMode::road);

        auto lastMod = _scenarioRoadMods[(_trackType & ~(1ULL << 7))];

        if (lastMod == 0xff)
            lastMod = 0;

        _lastSelectedMods = lastMod;
        _byte_113603A = 0;

        return trackWindow();
    }

    static window* createDockConstructionWindow()
    {
        common::createConstructionWindow();

        _lastSelectedSignal = 0xFF;

        _modList[0] = 0xFF;
        _modList[1] = 0xFF;
        _modList[2] = 0xFF;
        _modList[3] = 0xFF;

        _lastSelectedMods = 0;
        _lastSelectedBridge = 0xFF;

        common::refreshDockList(_stationList);

        if (_lastShipPort == 0xFF)
        {
            _lastSelectedStationType = _stationList[0];
        }
        else
        {
            _lastSelectedStationType = _lastShipPort;
        }

        return nonTrackWindow();
    }

    static window* createAirportConstructionWindow()
    {
        common::createConstructionWindow();

        _lastSelectedSignal = 0xFF;
        _modList[0] = 0xFF;
        _modList[1] = 0xFF;
        _modList[2] = 0xFF;
        _modList[3] = 0xFF;
        _lastSelectedMods = 0;
        _lastSelectedBridge = 0xFF;

        common::refreshAirportList(_stationList);

        if (_lastAirport == 0xFF)
        {
            _lastSelectedStationType = _stationList[0];
        }
        else
        {
            _lastSelectedStationType = _lastAirport;
        }

        return nonTrackWindow();
    }

    // 0x004A3B0D
    window* openWithFlags(const uint32_t flags)
    {
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

                if (roadObj->flags & flags_12::unk_03)
                {
                    if (_trackType & (1 << 7))
                    {
                        trackType = _trackType & ~(1 << 7);
                        roadObj = objectmgr::get<road_object>(trackType);

                        if (roadObj->flags & flags_12::unk_03)
                        {
                            _trackType = flags;

                            common::sub_4A3A50();

                            _lastSelectedTrackPiece = 0;
                            _lastSelectedTrackGradient = 0;

                            return window;
                        }
                    }
                }
            }
        }

        WindowManager::closeConstructionWindows();
        common::sub_4CD454();

        mainWindow = WindowManager::getMainWindow();

        if (mainWindow)
        {
            auto viewport = mainWindow->viewports[0];
            viewport->flags = _word_1135F86;
        }

        _trackType = static_cast<uint8_t>(flags);
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
        _lastSelectedTrackModSection = 0;

        common::setTrackOptions(flags);

        if (flags & (1 << 31))
        {
            return createAirportConstructionWindow();
        }
        else if (flags & (1 << 30))
        {
            return createDockConstructionWindow();
        }
        else if (flags & (1 << 7))
        {
            return createRoadConstructionWindow();
        }

        return createTrackConstructionWindow();
    }

    // 0x004A6FAC
    void sub_4A6FAC()
    {
        auto window = WindowManager::find(WindowType::construction);
        if (window == nullptr)
            return;
        if (window->current_tab == common::widx::tab_station - common::widx::tab_construction)
        {
            if (_byte_1136063 & (1 << 7) | (1 << 6))
                WindowManager::close(window);
            else
                window->call_on_mouse_up(common::widx::tab_construction);
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
            void tabReset(window* self)
            {
                switch (self->current_tab)
                {
                    case common::widx::tab_construction - common::widx::tab_construction:
                        construction::tabReset(self);
                        break;
                    case common::widx::tab_station - common::widx::tab_construction:
                        construction::tabReset(self);
                        break;
                    case common::widx::tab_signal - common::widx::tab_construction:
                        construction::tabReset(self);
                        break;
                    case common::widx::tab_overhead - common::widx::tab_construction:
                        construction::tabReset(self);
                        break;
                }
            }
        };

        static TabInformation tabInformationByTabOffset[] = {
            { construction::widgets, widx::tab_construction, &construction::events, construction::enabledWidgets },
            { station::widgets, widx::tab_station, &station::events, station::enabledWidgets },
            { signal::widgets, widx::tab_signal, &signal::events, signal::enabledWidgets },
            { overhead::widgets, widx::tab_overhead, &overhead::events, overhead::enabledWidgets },
        };

        void prepare_draw(window* self)
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
        void switchTab(window* self, widget_index widgetIndex)
        {
            if (self->current_tab == widgetIndex - widx::tab_construction)
                return;
            if (widgetIndex == widx::tab_station)
            {
                ui::windows::station::showStationCatchment(station_id::null);
            }
            common::sub_49FEC7();
            tilemgr::map_invalidate_map_selection_tiles();
            _mapSelectionFlags = _mapSelectionFlags & ~MapSelectFlag::enableConstruct;
            _trackCost = 0x80000000;
            _signalCost = 0x80000000;
            _stationCost = 0x80000000;
            _modCost = 0x80000000;
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

            tabInfo.tabReset(self);

            self->moveInsideScreenEdges();
        }

        static void drawRoadTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto company = companymgr::get(_playerCompany);
            auto companyColour = company->mainColours.primary;
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
                        gfx::draw_image(clipped, -4, -10, imageId);
                        auto colour = _byte_5045FA[companyColour];
                        if (!(roadStationObj->flags & road_station_flags::recolourable))
                        {
                            colour = 46;
                        }
                        imageId = gfx::recolour(roadStationObj->var_0C, colour) + 1;
                        gfx::draw_image(clipped, -4, -10, imageId);
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

        static void drawTrackTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto company = companymgr::get(_playerCompany);
            auto companyColour = company->mainColours.primary;
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
                    auto imageId = objectmgr::get<interface_skin_object>()->img + interface_skin::image_ids::toolbar_menu_airport;

                    widget::draw_tab(self, dpi, imageId, widx::tab_station);
                }
                else
                {
                    if (_byte_1136063 & (1 << 6))
                    {
                        auto imageId = objectmgr::get<interface_skin_object>()->img + interface_skin::image_ids::toolbar_menu_ship_port;

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
                                clipped->width *= 2;
                                clipped->height *= 2;
                                clipped->x *= 2;
                                clipped->y *= 2;
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

                    const std::vector<uint8_t> signalFrames2State = { 1, 2, 3, 3, 3, 3, 3, 3, 2, 1, 0, 0, 0, 0, 0 };
                    const std::vector<uint8_t> signalFrames3State = { 1, 2, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0 };
                    const std::vector<uint8_t> signalFrames4State = { 1, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

                    static const std::vector<std::vector<uint8_t>> signalFrames = {
                        signalFrames2State,
                        signalFrames3State,
                        signalFrames4State,
                    };

                    if (gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height))
                    {
                        auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);
                        auto imageId = trainSignalObject->var_0E;
                        if (self->current_tab == widx::tab_signal - widx::tab_construction)
                        {
                            auto frames = signalFrames[(((trainSignalObject->num_frames + 2) / 3) - 2)];
                            auto frameCount = std::size(frames);
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

        // 0x0049ED33
        void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            if (_trackType & (1 << 7))
            {
                drawRoadTabs(self, dpi);
            }
            else
            {
                drawTrackTabs(self, dpi);
            }
        }

        // 0x004A09DE
        void repositionTabs(window* self)
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
        void sub_49FEC7()
        {
            registers regs;
            call(0x0049FEC7, regs);
        }

        // 0x0049DD14
        void on_close(window* self)
        {
            sub_49FEC7();
            WindowManager::viewportSetVisibility(WindowManager::viewport_visibility::reset);
            tilemgr::map_invalidate_map_selection_tiles();
            _mapSelectionFlags = _mapSelectionFlags & ~MapSelectFlag::enableConstruct;
            windows::hideDirectionArrows();
            windows::hideGridlines();
        }

        // 0x0049E437, 0x0049E76F, 0x0049ECD1
        void on_update(window* self, uint8_t flag)
        {
            self->frame_no++;
            self->call_prepare_draw();
            WindowManager::invalidateWidget(WindowType::construction, self->number, self->current_tab + common::widx::tab_construction);

            if (input::is_tool_active(WindowType::construction, self->number))
                return;

            if (!(_byte_522096 & flag))
                return;

            sub_49FEC7();
        }

        void init_events()
        {
            construction::init_events();
            station::init_events();
            signal::init_events();
            overhead::init_events();
        }

        // 0x004A0832
        std::optional<trackPieceId> getRoadPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation)
        {
            if (trackPiece == 0xFF)
                return std::nullopt;

            uint8_t id = 0;

            switch (trackPiece)
            {
                case common::trackPiece::straight: // 0x004A0856
                {
                    if (rotation >= 4)
                        return std::nullopt;
                    switch (gradient)
                    {
                        default:
                            return std::nullopt;
                        case common::trackGradient::level:
                            id = 0;
                            break;
                        case common::trackGradient::slope_up:
                            id = 5;
                            break;
                        case common::trackGradient::slope_down:
                            id = 6;
                            break;
                        case common::trackGradient::steep_slope_up:
                            id = 7;
                            break;
                        case common::trackGradient::steep_slope_down:
                            id = 8;
                            break;
                    }
                    break;
                }
                case common::trackPiece::left_hand_curve_very_small: // 0x004A08A5
                {
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    if (rotation >= 4)
                        return std::nullopt;
                    id = 1;
                    break;
                }
                case common::trackPiece::right_hand_curve_very_small: // 0x004A08CD
                {
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    if (rotation >= 4)
                        return std::nullopt;
                    id = 2;
                    break;
                }
                case common::trackPiece::left_hand_curve_small: // 0x004A08ED
                {
                    if (rotation >= 4)
                        return std::nullopt;
                    id = 3;
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    break;
                }
                case common::trackPiece::right_hand_curve_small: // 0x004A08FB
                {
                    if (rotation >= 4)
                        return std::nullopt;
                    id = 4;
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
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
                    return std::nullopt;
                }
                case common::trackPiece::turnaround: // 0x004A0909
                {
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    if (rotation >= 12)
                        return std::nullopt;
                    id = 9;
                    break;
                }
            }

            if (rotation < 12)
                rotation &= 3;

            return trackPieceId{ id, rotation };
        }

        // 0x004A04F8
        std::optional<trackPieceId> getTrackPieceId(uint8_t trackPiece, uint8_t gradient, uint8_t rotation)
        {
            if (trackPiece == 0xFF)
                return std::nullopt;

            uint8_t id = 0;

            switch (trackPiece)
            {
                case common::trackPiece::straight: // loc_4A051C
                {
                    if (rotation >= 12)
                    {
                        id = 1;
                        if (gradient != common::trackGradient::level)
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
                                case common::trackGradient::level:
                                    id = 27;
                                    break;
                                case common::trackGradient::steep_slope_up:
                                    id = 35;
                                    break;
                                case common::trackGradient::steep_slope_down:
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
                                case common::trackGradient::level:
                                    id = 26;
                                    break;
                                case common::trackGradient::steep_slope_up:
                                    id = 34;
                                    break;
                                case common::trackGradient::steep_slope_down:
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
                                case common::trackGradient::level:
                                    id = 0;
                                    break;
                                case common::trackGradient::slope_up:
                                    id = 14;
                                    break;
                                case common::trackGradient::slope_down:
                                    id = 15;
                                    break;
                                case common::trackGradient::steep_slope_up:
                                    id = 16;
                                    break;
                                case common::trackGradient::steep_slope_down:
                                    id = 17;
                                    break;
                            }
                        }
                    }
                    break;
                }

                case common::trackPiece::left_hand_curve_very_small: // loc_4A05C3
                {
                    if (gradient != common::trackGradient::level)
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

                case common::trackPiece::right_hand_curve_very_small: // loc_4A05F4
                {
                    if (gradient != common::trackGradient::level)
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

                case common::trackPiece::left_hand_curve_small: // loc_4A0625
                {
                    if (rotation >= 4)
                        return std::nullopt;
                    switch (gradient)
                    {
                        default:
                            return std::nullopt;
                        case common::trackGradient::level:
                            id = 4;
                            break;
                        case common::trackGradient::slope_up:
                            id = 18;
                            break;
                        case common::trackGradient::slope_down:
                            id = 20;
                            break;
                        case common::trackGradient::steep_slope_up:
                            id = 22;
                            break;
                        case common::trackGradient::steep_slope_down:
                            id = 24;
                            break;
                    }
                    break;
                }

                case common::trackPiece::right_hand_curve_small: // loc_4A066A
                {
                    if (rotation >= 4)
                        return std::nullopt;
                    switch (gradient)
                    {
                        default:
                            return std::nullopt;
                        case common::trackGradient::level:
                            id = 5;
                            break;
                        case common::trackGradient::slope_up:
                            id = 19;
                            break;
                        case common::trackGradient::slope_down:
                            id = 21;
                            break;
                        case common::trackGradient::steep_slope_up:
                            id = 23;
                            break;
                        case common::trackGradient::steep_slope_down:
                            id = 25;
                            break;
                    }
                    break;
                }

                case common::trackPiece::left_hand_curve: // loc_4A06AF
                {
                    if (rotation >= 4)
                        return std::nullopt;
                    id = 6;
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    break;
                }

                case common::trackPiece::right_hand_curve: // loc_4A06C8
                {
                    if (rotation >= 4)
                        return std::nullopt;
                    id = 7;
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    break;
                }

                case common::trackPiece::left_hand_curve_large: // loc_4A06E1
                {
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    id = 10;
                    if (rotation >= 12)
                        break;
                    if (rotation >= 4)
                        return std::nullopt;
                    id = 8;
                    break;
                }

                case common::trackPiece::right_hand_curve_large: // loc_4A0705
                {
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    id = 11;
                    if (rotation >= 12)
                        break;
                    if (rotation >= 4)
                        return std::nullopt;
                    id = 9;
                    break;
                }

                case common::trackPiece::s_bend_left: // loc_4A0729
                {
                    if (gradient != common::trackGradient::level)
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

                case common::trackPiece::s_bend_right: // loc_4A0756
                {
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    if (rotation >= 8)
                        return std::nullopt;
                    id = 32;
                    if (rotation >= 4)
                        break;
                    id = 13;
                    break;
                }

                case common::trackPiece::s_bend_to_dual_track: // loc_4A077C
                {
                    if (gradient != common::trackGradient::level)
                        return std::nullopt;
                    if (rotation >= 8)
                        return std::nullopt;
                    id = 40;
                    if (rotation >= 4)
                        break;
                    id = 38;
                    break;
                }

                case common::trackPiece::s_bend_to_single_track: // loc_4A07A2
                {
                    if (gradient != common::trackGradient::level)
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

                case common::trackPiece::turnaround: // loc_4A07C0
                {
                    if (gradient != common::trackGradient::level)
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

            return trackPieceId{ id, rotation };
        }

        static void activateSelectedRoadWidgets(window* window)
        {
            tilemgr::map_invalidate_map_selection_tiles();
            _mapSelectionFlags = _mapSelectionFlags | (1 << 3) | (1 << 1);

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
                rotation = (*road).rotation;
                roadId = (*road).id;
            }

            auto roadPiece = roadPieces[roadId];
            auto i = 0;
            auto posId = 0;

            while (roadPiece[i].index != 0xFF)
            {
                if (roadPiece[i].flags & (1 << 7))
                {
                    i++;
                }

                map_pos pos = { roadPiece[i].x, roadPiece[i].y };

                pos = rotate2DCoordinate(pos, rotation);

                pos.x += x;
                pos.y += y;
                _mapSelectedTiles[posId] = pos;
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

        static void activateSelectedTrackWidgets(window* window)
        {
            tilemgr::map_invalidate_map_selection_tiles();
            _mapSelectionFlags = _mapSelectionFlags | (1 << 3) | (1 << 1);

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
                rotation = (*track).rotation;
                trackId = (*track).id;
            }

            auto trackPiece = trackPieces[trackId];
            auto i = 0;
            auto posId = 0;

            while (trackPiece[i].index != 0xFF)
            {
                if (trackPiece[i].flags & (1 << 7))
                {
                    i++;
                }
                map_pos pos = { trackPiece[i].x, trackPiece[i].y };

                pos = rotate2DCoordinate(pos, rotation);

                pos.x += x;
                pos.y += y;
                _mapSelectedTiles[posId] = pos;
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

        // 0x0049F1B5
        void activateSelectedConstructionWidgets()
        {
            auto window = WindowManager::find(WindowType::construction);

            if (window == nullptr)
                return;

            if (_trackType & (1 << 7))
            {
                activateSelectedRoadWidgets(window);
            }
            else
            {
                activateSelectedTrackWidgets(window);
            }
        }

        // 0x004CD454
        void sub_4CD454()
        {
            if (is_unknown_3_mode())
            {
                auto window = WindowManager::find(_toolWindowType, _toolWindowNumber);
                if (window != nullptr)
                    input::cancel_tool();
            }
        }

        // 0x004A3A06
        void setTrackOptions(const uint8_t trackType)
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
        void setDisabledWidgets(window* self)
        {
            auto disabledWidgets = 0;
            if (is_editor_mode())
                disabledWidgets |= (1ULL << common::widx::tab_station);

            if (_byte_1136063 & (1 << 7 | 1 << 6))
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
        void createConstructionWindow()
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
        static void sortList(uint8_t* list, size_t listSize)
        {
            //size_t count = 0;
            //while (list[count] != 0xFF)
            //{
            //    count++;
            //}
            //while (count > 1)
            //{
            //    for (size_t i = 1; i < count; i++)
            //    {
            //        uint8_t item1 = list[i];
            //        uint8_t item2 = list[i - 1];
            //        if (item2 > item1)
            //        {
            //            list[i - 1] = item1;
            //            list[i] = item2;
            //        }
            //    }
            //    count--;
            //}

            std::sort(list, list + listSize);
        }

        // 0x0048D70C
        void refreshAirportList(uint8_t* stationList)
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
                stationList[airportCount] = i;
                airportCount++;
            }
            stationList[airportCount] = 0xFF;

            sortList(stationList, std::size(_stationList));
        }

        // 0x0048D753
        void refreshDockList(uint8_t* stationList)
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
                stationList[dockCount] = i;
                dockCount++;
            }
            stationList[dockCount] = 0xFF;

            sortList(stationList, std::size(_stationList));
        }

        // 0x0048D678, 0x0048D5E4
        void refreshStationList(uint8_t* stationList, uint8_t trackType, TransportMode transportMode)
        {
            auto currentYear = current_year();
            auto stationCount = 0;

            if (transportMode == TransportMode::road)
            {
                trackType &= ~(1 << 7);
                auto roadObj = objectmgr::get<road_object>(trackType);

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
                    stationList[stationCount] = station;
                    stationCount++;
                }
            }

            if (transportMode == TransportMode::rail)
            {
                auto trackObj = objectmgr::get<track_object>(trackType);

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
                    stationList[stationCount] = station;
                    stationCount++;
                }
            }

            for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::road_station); i++)
            {
                uint8_t numCompatible;
                uint8_t* mods;
                uint16_t designedYear;
                uint16_t obsoleteYear;

                if (transportMode == TransportMode::road)
                {
                    auto roadStationObj = objectmgr::get<road_station_object>(i);

                    if (roadStationObj == nullptr)
                        continue;

                    numCompatible = roadStationObj->num_compatible;
                    mods = roadStationObj->mods;
                    designedYear = roadStationObj->designed_year;
                    obsoleteYear = roadStationObj->obsolete_year;
                }
                else if (transportMode == TransportMode::rail)
                {
                    auto trainStationObj = objectmgr::get<train_station_object>(i);

                    if (trainStationObj == nullptr)
                        continue;

                    numCompatible = trainStationObj->num_compatible;
                    mods = trainStationObj->mods;
                    designedYear = trainStationObj->designed_year;
                    obsoleteYear = trainStationObj->obsolete_year;
                }
                else
                {
                    return;
                }

                for (auto modCount = 0; modCount < numCompatible; modCount++)
                {
                    if (trackType != mods[modCount])
                        continue;
                    if (currentYear < designedYear)
                        continue;
                    if (currentYear > obsoleteYear)
                        continue;
                    for (size_t k = 0; k < std::size(_stationList); k++)
                    {
                        if (&stationList[k] == &stationList[stationCount])
                        {
                            stationList[stationCount] = i;
                            stationCount++;
                            break;
                        }
                        if (i == stationList[k])
                            break;
                    }
                }
            }

            stationList[stationCount] = 0xFF;

            sortList(stationList, std::size(_stationList));
        }

        // 0x0042C518, 0x0042C490
        void refreshBridgeList(uint8_t* bridgeList, uint8_t trackType, TransportMode transportMode)
        {
            auto currentYear = current_year();
            auto bridgeCount = 0;

            if (transportMode == TransportMode::road)
            {
                trackType &= ~(1 << 7);
                auto roadObj = objectmgr::get<road_object>(trackType);
                for (auto i = 0; i < roadObj->num_bridges; i++)
                {
                    auto bridge = roadObj->bridges[i];
                    if (bridge == 0xFF)
                        continue;
                    auto bridgeObj = objectmgr::get<bridge_object>(bridge);
                    if (currentYear < bridgeObj->designed_year)
                        continue;
                    bridgeList[bridgeCount] = bridge;
                    bridgeCount++;
                }
            }

            if (transportMode == TransportMode::rail)
            {
                auto trackObj = objectmgr::get<track_object>(trackType);
                for (auto i = 0; i < trackObj->num_bridges; i++)
                {
                    auto bridge = trackObj->bridges[i];
                    if (bridge == 0xFF)
                        continue;
                    auto bridgeObj = objectmgr::get<bridge_object>(bridge);
                    if (currentYear < bridgeObj->designed_year)
                        continue;
                    bridgeList[bridgeCount] = bridge;
                    bridgeCount++;
                }
            }

            for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::bridge); i++)
            {
                auto bridgeObj = objectmgr::get<bridge_object>(i);
                if (bridgeObj == nullptr)
                    continue;

                uint8_t numCompatible;
                uint8_t* mods;

                if (transportMode == TransportMode::road)
                {
                    numCompatible = bridgeObj->road_num_compatible;
                    mods = bridgeObj->road_mods;
                }
                else if (transportMode == TransportMode::rail)
                {
                    numCompatible = bridgeObj->track_num_compatible;
                    mods = bridgeObj->track_mods;
                }
                else
                {
                    return;
                }

                for (auto modCount = 0; modCount < numCompatible; modCount++)
                {
                    if (trackType != mods[modCount])
                        continue;
                    if (currentYear < bridgeObj->designed_year)
                        continue;
                    for (size_t k = 0; k < std::size(_bridgeList); k++)
                    {
                        if (&bridgeList[k] == &bridgeList[bridgeCount])
                        {
                            _bridgeList[bridgeCount] = i;
                            bridgeCount++;
                            break;
                        }
                        if (i == bridgeList[k])
                            break;
                    }
                }
            }

            _bridgeList[bridgeCount] = 0xFF;

            sortList(_bridgeList, std::size(_bridgeList));
        }

        // 0x004781C5, 0x004A693D
        void refreshModList(uint8_t* modList, uint8_t trackType, TransportMode transportMode)
        {
            if (transportMode == TransportMode::road)
            {
                trackType &= ~(1 << 7);
            }

            auto companyId = _updatingCompanyId;

            modList[0] = 0xFF;
            modList[1] = 0xFF;
            modList[2] = 0xFF;
            modList[3] = 0xFF;
            auto flags = 0;

            for (uint8_t vehicle = 0; vehicle < objectmgr::get_max_objects(object_type::vehicle); vehicle++)
            {
                auto vehicleObj = objectmgr::get<vehicle_object>(vehicle);

                if (vehicleObj == nullptr)
                    continue;

                if (vehicleObj->mode != transportMode)
                    continue;

                if (trackType != vehicleObj->track_type)
                    continue;

                auto company = companymgr::get(companyId);

                if (!company->isVehicleIndexUnlocked(vehicle))
                    continue;

                for (auto i = 0; i < vehicleObj->num_mods; i++)
                {
                    flags |= 1ULL << vehicleObj->required_track_extras[i];
                }

                if (!(vehicleObj->flags & flags_E0::rack_rail))
                    continue;

                flags |= 1ULL << vehicleObj->rack_rail_type;
            }

            if (transportMode == TransportMode::road)
            {
                auto roadObj = objectmgr::get<road_object>(trackType);

                for (auto i = 0; i < roadObj->num_mods; i++)
                {
                    if (flags & (1 << roadObj->mods[i]))
                        modList[i] = roadObj->mods[i];
                }
            }

            if (transportMode == TransportMode::rail)
            {
                auto trackObj = objectmgr::get<track_object>(trackType);

                for (auto i = 0; i < trackObj->num_mods; i++)
                {
                    if (flags & (1 << trackObj->mods[i]))
                        modList[i] = trackObj->mods[i];
                }
            }
        }

        // 0x004A3A50
        void sub_4A3A50()
        {
            common::sub_49FEC7();
            setTrackOptions(_trackType);
            refreshStationList(_stationList, _trackType, TransportMode::road);

            auto lastStation = _scenarioRoadStations[(_trackType & ~(1ULL << 7))];
            if (lastStation == 0xFF)
                lastStation = _stationList[0];
            _lastSelectedStationType = lastStation;

            refreshBridgeList(_bridgeList, _trackType, TransportMode::road);

            auto lastBridge = _scenarioBridges[(_trackType & ~(1ULL << 7))];
            if (lastBridge == 0xFF)
                lastBridge = _bridgeList[0];
            _lastSelectedBridge = lastBridge;

            refreshModList(_modList, _trackType, TransportMode::road);

            auto lastMod = _scenarioRoadMods[(_trackType & ~(1ULL << 7))];
            if (lastMod == 0xFF)
                lastMod = 0;
            _lastSelectedMods = lastMod;

            auto window = WindowManager::find(WindowType::construction);

            if (window != nullptr)
            {
                setDisabledWidgets(window);
            }
            common::activateSelectedConstructionWidgets();
        }

        // 0x00488B4D
        void refreshSignalList(uint8_t* signalList, uint8_t trackType)
        {
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

                if (currentYear > signalObj->obsolete_year)
                    continue;
                if (currentYear < signalObj->designed_year)
                    continue;
                signalList[signalCount] = ecx;
                signalCount++;
            }

            for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::track_signal); i++)
            {
                auto signalObj = objectmgr::get<train_signal_object>(i);
                if (signalObj == nullptr)
                    continue;
                for (auto modCount = 0; modCount < signalObj->num_compatible; modCount++)
                {
                    if (trackType != objectmgr::get<train_signal_object>(i)->mods[modCount])
                        continue;
                    if (currentYear < signalObj->designed_year)
                        continue;
                    if (currentYear > signalObj->obsolete_year)
                        continue;
                    for (size_t k = 0; k < std::size(_signalList); k++)
                    {
                        if (&signalList[k] == &signalList[signalCount])
                        {
                            signalList[signalCount] = i;
                            signalCount++;
                            break;
                        }
                        if (i == signalList[k])
                            break;
                    }
                }
            }

            signalList[signalCount] = 0xFF;

            sortList(signalList, std::size(_signalList));
        }
    }

    void registerHooks()
    {
        register_hook(
            0x0049F1B5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                common::activateSelectedConstructionWidgets();
                regs = backup;
                return 0;
            });

        //register_hook(
        //    0x0049DC97,
        //    [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
        //        registers backup = regs;
        //        construction::on_tool_down(*((ui::window*)regs.esi), regs.dx, regs.ax, regs.cx);
        //        regs = backup;
        //        return 0;
        //    });
    }
}
