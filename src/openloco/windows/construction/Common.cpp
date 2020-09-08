#include "../../CompanyManager.h"
#include "../../Date.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Objects/AirportObject.h"
#include "../../Objects/BridgeObject.h"
#include "../../Objects/DockObject.h"
#include "../../Objects/InterfaceSkinObject.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/road_extra_object.h"
#include "../../Objects/road_object.h"
#include "../../Objects/road_station_object.h"
#include "../../Objects/track_extra_object.h"
#include "../../Objects/track_object.h"
#include "../../Objects/train_signal_object.h"
#include "../../Objects/train_station_object.h"
#include "../../StationManager.h"
#include "../../Widget.h"
#include "Construction.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction
{
    static loco_global<ui::window_number, 0x00523390> _toolWindowNumber;

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
            window->callOnMouseUp(common::widx::tab_station);
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

        construction::activateSelectedConstructionWidgets();
        window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            window->callOnMouseUp(construction::widx::rotate_90);
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

        common::refreshStationList(_stationList, _trackType, TransportMode::rail);

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

    // 0x004A0EAD
    window* openAtTrack(window* main, track_element* track, const map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(track);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A0EAD, regs);

        return reinterpret_cast<window*>(regs.esi);
    }

    // 0x004A147F
    window* openAtRoad(window* main, road_element* track, const map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(track);
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A147F, regs);

        return reinterpret_cast<window*>(regs.esi);
    }

    // 0x004A1303
    void setToTrackExtra(window* main, track_element* track, const uint8_t bh, const map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(track);
        regs.bh = bh;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A1303, regs);
    }

    // 0x004A13C1
    void setToRoadExtra(window* main, road_element* road, const uint8_t bh, const map_pos pos)
    {
        registers regs{};
        regs.esi = reinterpret_cast<uint32_t>(main);
        regs.edx = reinterpret_cast<uint32_t>(road);
        regs.bh = bh;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A13C1, regs);
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
                            _trackType = static_cast<uint8_t>(flags);

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
            if (_byte_1136063 & ((1 << 7) | (1 << 6)))
                WindowManager::close(window);
            else
                window->callOnMouseUp(common::widx::tab_construction);
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
            void (*tabReset)(window*);
        };

        static TabInformation tabInformationByTabOffset[] = {
            { construction::widgets, widx::tab_construction, &construction::events, construction::enabledWidgets, &construction::tabReset },
            { station::widgets, widx::tab_station, &station::events, station::enabledWidgets, &station::tabReset },
            { signal::widgets, widx::tab_signal, &signal::events, signal::enabledWidgets, &signal::tabReset },
            { overhead::widgets, widx::tab_overhead, &overhead::events, overhead::enabledWidgets, &overhead::tabReset },
        };

        void prepareDraw(window* self)
        {
            // Reset tab widgets if needed
            const auto& tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->initScrollWidgets();
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

            if (widgetIndex == widx::tab_construction)
            {
                construction::activateSelectedConstructionWidgets();
            }

            common::sub_49FEC7();
            tilemgr::mapInvalidateMapSelectionTiles();
            _mapSelectionFlags = _mapSelectionFlags & ~MapSelectFlag::enableConstruct;
            _trackCost = 0x80000000;
            _signalCost = 0x80000000;
            _stationCost = 0x80000000;
            _modCost = 0x80000000;
            _byte_1136076 = 0;

            if (input::isToolActive(self->type, self->number))
                input::toolCancel();

            self->current_tab = widgetIndex - widx::tab_construction;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);

            auto tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_construction];

            self->enabled_widgets = tabInfo.enabledWidgets;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;
            self->holdable_widgets = 0;

            setDisabledWidgets(self);

            self->invalidate();

            self->width = self->widgets[widx::frame].right + 1;
            self->height = self->widgets[widx::frame].bottom + 1;

            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();

            tabInfo.tabReset(self);

            self->moveInsideScreenEdges();
        }

        // 0x0049EFEF
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
                if (!self->isDisabled(widx::tab_station))
                {
                    auto x = self->widgets[widx::tab_station].left + self->x + 1;
                    auto y = self->widgets[widx::tab_station].top + self->y + 1;
                    auto width = 29;
                    auto height = 25;
                    if (self->current_tab == widx::tab_station - widx::tab_construction)
                        height++;

                    gfx::drawpixelinfo_t* clipped = nullptr;

                    if (gfx::clipDrawpixelinfo(&clipped, dpi, x, y, width, height))
                    {
                        clipped->zoom_level = 1;
                        clipped->width <<= 1;
                        clipped->height <<= 1;
                        clipped->x <<= 1;
                        clipped->y <<= 1;
                        auto roadStationObj = objectmgr::get<road_station_object>(_lastSelectedStationType);
                        auto imageId = gfx::recolour(roadStationObj->var_0C, companyColour);
                        gfx::drawImage(clipped, -4, -10, imageId);
                        auto colour = _byte_5045FA[companyColour];
                        if (!(roadStationObj->flags & road_station_flags::recolourable))
                        {
                            colour = 46;
                        }
                        imageId = gfx::recolour(roadStationObj->var_0C, colour) + 1;
                        gfx::drawImage(clipped, -4, -10, imageId);
                    }

                    widget::draw_tab(self, dpi, -2, widx::tab_station);
                }
            }
            // Overhead tab
            {
                widget::draw_tab(self, dpi, image_ids::null, widx::tab_overhead);
                if (!self->isDisabled(widx::tab_station))
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
                            gfx::drawImage(dpi, x, y, imageId);
                        }
                    }

                    widget::draw_tab(self, dpi, -2, widx::tab_overhead);
                }
            }
        }

        // 0x0049ED40
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
                        if (!self->isDisabled(widx::tab_station))
                        {
                            auto x = self->widgets[widx::tab_station].left + self->x + 1;
                            auto y = self->widgets[widx::tab_station].top + self->y + 1;
                            auto width = 29;
                            auto height = 25;
                            if (self->current_tab == widx::tab_station - widx::tab_construction)
                                height++;

                            gfx::drawpixelinfo_t* clipped = nullptr;

                            if (gfx::clipDrawpixelinfo(&clipped, dpi, x, y, width, height))
                            {
                                clipped->zoom_level = 1;
                                clipped->width *= 2;
                                clipped->height *= 2;
                                clipped->x *= 2;
                                clipped->y *= 2;
                                auto trainStationObj = objectmgr::get<train_station_object>(_lastSelectedStationType);
                                auto imageId = gfx::recolour(trainStationObj->var_0E, companyColour);
                                gfx::drawImage(clipped, -4, -9, imageId);
                                auto colour = _byte_5045FA[companyColour];
                                if (!(trainStationObj->flags & train_station_flags::recolourable))
                                {
                                    colour = 46;
                                }
                                imageId = gfx::recolour(imageId, colour) + 1;
                                gfx::drawImage(clipped, -4, -9, imageId);
                            }

                            widget::draw_tab(self, dpi, -2, widx::tab_station);
                        }
                    }
                }
            }
            // Signal Tab
            {
                widget::draw_tab(self, dpi, image_ids::null, widx::tab_signal);
                if (!self->isDisabled(widx::tab_signal))
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

                    if (gfx::clipDrawpixelinfo(&clipped, dpi, x, y, width, height))
                    {
                        auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);
                        auto imageId = trainSignalObject->var_0E;
                        if (self->current_tab == widx::tab_signal - widx::tab_construction)
                        {
                            auto frames = signalFrames[(((trainSignalObject->num_frames + 2) / 3) - 2)];
                            auto frameCount = std::size(frames) - 1;
                            frameCount &= (self->frame_no >> trainSignalObject->var_04);
                            auto frameIndex = frames[frameCount];
                            frameIndex <<= 3;
                            imageId += frameIndex;
                        }
                        gfx::drawImage(clipped, 15, 31, imageId);
                    }

                    widget::draw_tab(self, dpi, -2, widx::tab_signal);
                }
            }
            // Overhead Tab
            {
                widget::draw_tab(self, dpi, image_ids::null, widx::tab_overhead);
                if (!self->isDisabled(widx::tab_station))
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
                            gfx::drawImage(dpi, x, y, imageId);
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
                if (self->isDisabled(i))
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
        void onClose(window* self)
        {
            sub_49FEC7();
            WindowManager::viewportSetVisibility(WindowManager::viewport_visibility::reset);
            tilemgr::mapInvalidateMapSelectionTiles();
            _mapSelectionFlags = _mapSelectionFlags & ~MapSelectFlag::enableConstruct;
            windows::hideDirectionArrows();
            windows::hideGridlines();
        }

        // 0x0049E437, 0x0049E76F, 0x0049ECD1
        void onUpdate(window* self, uint8_t flag)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::construction, self->number, self->current_tab + common::widx::tab_construction);

            if (input::isToolActive(WindowType::construction, self->number))
                return;

            if (!(_byte_522096 & flag))
                return;

            sub_49FEC7();
        }

        void initEvents()
        {
            construction::initEvents();
            station::initEvents();
            signal::initEvents();
            overhead::initEvents();
        }

        // 0x004CD454
        void sub_4CD454()
        {
            if (isTrackUpgradeMode())
            {
                auto window = WindowManager::find(_toolWindowType, _toolWindowNumber);
                if (window != nullptr)
                    input::toolCancel();
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
            if (isEditorMode())
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

            window->initScrollWidgets();
            window->owner = _playerCompany;

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[1] = skin->colour_0D;

            WindowManager::sub_4CEE0B(window);
            ui::windows::showDirectionArrows();
            ui::windows::showGridlines();

            common::initEvents();
        }

        // 0x004723BD
        static void sortList(uint8_t* list)
        {
            size_t count = 0;
            while (list[count] != 0xFF)
            {
                count++;
            }
            while (count > 1)
            {
                for (size_t i = 1; i < count; i++)
                {
                    uint8_t item1 = list[i];
                    uint8_t item2 = list[i - 1];
                    if (item2 > item1)
                    {
                        list[i - 1] = item1;
                        list[i] = item2;
                    }
                }
                count--;
            }
        }

        // 0x0048D70C
        void refreshAirportList(uint8_t* stationList)
        {
            auto currentYear = getCurrentYear();
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

            sortList(stationList);
        }

        // 0x0048D753
        void refreshDockList(uint8_t* stationList)
        {
            auto currentYear = getCurrentYear();
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

            sortList(stationList);
        }

        // 0x0048D678, 0x0048D5E4
        void refreshStationList(uint8_t* stationList, uint8_t trackType, TransportMode transportMode)
        {
            auto currentYear = getCurrentYear();
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

            sortList(stationList);
        }

        // 0x0042C518, 0x0042C490
        void refreshBridgeList(uint8_t* bridgeList, uint8_t trackType, TransportMode transportMode)
        {
            auto currentYear = getCurrentYear();
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

            sortList(_bridgeList);
        }

        // 0x004781C5, 0x004A693D
        void refreshModList(uint8_t* modList, uint8_t trackType, TransportMode transportMode)
        {
            if (transportMode == TransportMode::road)
            {
                trackType &= ~(1 << 7);
            }

            auto companyId = companymgr::updatingCompanyId();

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
            construction::activateSelectedConstructionWidgets();
        }

        // 0x00488B4D
        void refreshSignalList(uint8_t* signalList, uint8_t trackType)
        {
            auto currentYear = getCurrentYear();
            auto trackObj = objectmgr::get<track_object>(trackType);
            auto signalCount = 0;
            auto var_0E = trackObj->var_0E;
            while (var_0E > 0)
            {
                auto ecx = utility::bitScanForward(var_0E);
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

            sortList(signalList);
        }
    }

    void registerHooks()
    {
        registerHook(
            0x0049F1B5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                construction::activateSelectedConstructionWidgets();
                regs = backup;
                return 0;
            });

        //registerHook(
        //    0x0049DC97,
        //    [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
        //        registers backup = regs;
        //        construction::on_tool_down(*((ui::window*)regs.esi), regs.dx, regs.ax, regs.cx);
        //        regs = backup;
        //        return 0;
        //    });
    }
}
