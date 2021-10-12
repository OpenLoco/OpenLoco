#include "../../CompanyManager.h"
#include "../../Date.h"
#include "../../GameCommands/GameCommands.h"
#include "../../Graphics/Colour.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Objects/AirportObject.h"
#include "../../Objects/BridgeObject.h"
#include "../../Objects/DockObject.h"
#include "../../Objects/InterfaceSkinObject.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/RoadExtraObject.h"
#include "../../Objects/RoadObject.h"
#include "../../Objects/RoadStationObject.h"
#include "../../Objects/TrackExtraObject.h"
#include "../../Objects/TrackObject.h"
#include "../../Objects/TrainSignalObject.h"
#include "../../Objects/TrainStationObject.h"
#include "../../StationManager.h"
#include "../../TrackData.h"
#include "../../Widget.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction
{
    static loco_global<Ui::WindowNumber_t, 0x00523390> _toolWindowNumber;

    static Window* nonTrackWindow()
    {
        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            Common::setDisabledWidgets(window);
        }

        window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            window->callOnMouseUp(Common::widx::tab_station);
        }
        return window;
    }

    static Window* trackWindow()
    {
        auto window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            Common::setDisabledWidgets(window);
        }

        Construction::activateSelectedConstructionWidgets();
        window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            window->callOnMouseUp(Construction::widx::rotate_90);
        }

        return window;
    }

    static Window* createTrackConstructionWindow()
    {
        Common::createConstructionWindow();

        Common::refreshSignalList(_signalList, _trackType);

        auto lastSignal = _scenarioSignals[_trackType];

        if (lastSignal == 0xFF)
            lastSignal = _signalList[0];

        _lastSelectedSignal = lastSignal;

        Common::refreshStationList(_stationList, _trackType, TransportMode::rail);

        auto lastStation = _scenarioTrainStations[_trackType];

        if (lastStation == 0xFF)
            lastStation = _stationList[0];

        _lastSelectedStationType = lastStation;

        Common::refreshBridgeList(_bridgeList, _trackType, TransportMode::rail);

        auto lastBridge = _scenarioBridges[_trackType];

        if (lastBridge == 0xFF)
            lastBridge = _bridgeList[0];

        _lastSelectedBridge = lastBridge;

        Common::refreshModList(_modList, _trackType, TransportMode::rail);

        auto lastMod = _scenarioTrackMods[_trackType];

        if (lastMod == 0xFF)
            lastMod = 0;

        _lastSelectedMods = lastMod;
        _byte_113603A = 0;

        return trackWindow();
    }

    static Window* createRoadConstructionWindow()
    {
        Common::createConstructionWindow();

        _lastSelectedSignal = 0xFF;

        Common::refreshStationList(_stationList, _trackType, TransportMode::road);

        auto lastStation = _scenarioRoadStations[(_trackType & ~(1ULL << 7))];

        if (lastStation == 0xFF)
            lastStation = _stationList[0];

        _lastSelectedStationType = lastStation;

        Common::refreshBridgeList(_bridgeList, _trackType, TransportMode::road);

        auto lastBridge = _scenarioBridges[(_trackType & ~(1ULL << 7))];

        if (lastBridge == 0xFF)
            lastBridge = _bridgeList[0];

        _lastSelectedBridge = lastBridge;

        Common::refreshModList(_modList, _trackType, TransportMode::road);

        auto lastMod = _scenarioRoadMods[(_trackType & ~(1ULL << 7))];

        if (lastMod == 0xff)
            lastMod = 0;

        _lastSelectedMods = lastMod;
        _byte_113603A = 0;

        return trackWindow();
    }

    static Window* createDockConstructionWindow()
    {
        Common::createConstructionWindow();

        _lastSelectedSignal = 0xFF;

        _modList[0] = 0xFF;
        _modList[1] = 0xFF;
        _modList[2] = 0xFF;
        _modList[3] = 0xFF;

        _lastSelectedMods = 0;
        _lastSelectedBridge = 0xFF;

        Common::refreshDockList(_stationList);

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

    static Window* createAirportConstructionWindow()
    {
        Common::createConstructionWindow();

        _lastSelectedSignal = 0xFF;
        _modList[0] = 0xFF;
        _modList[1] = 0xFF;
        _modList[2] = 0xFF;
        _modList[3] = 0xFF;
        _lastSelectedMods = 0;
        _lastSelectedBridge = 0xFF;

        Common::refreshAirportList(_stationList);

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
    Window* openAtTrack(Window* main, TrackElement* track, const Pos2 pos)
    {
        auto* viewport = main->viewports[0];
        _backupTileElement = *reinterpret_cast<TileElement*>(track);
        auto* copyElement = (*_backupTileElement).asTrack();
        if (copyElement == nullptr)
        {
            return nullptr;
        }

        if (copyElement->owner() != CompanyManager::getControllingId())
        {
            return nullptr;
        }

        removeConstructionGhosts();
        auto* wnd = WindowManager::find(WindowType::construction);
        if (wnd == nullptr)
        {
            WindowManager::closeConstructionWindows();
            Common::createConstructionWindow();
        }
        else
        {
            Common::resetWindow(*wnd, Common::widx::tab_construction);
        }

        _trackType = copyElement->trackObjectId();
        _byte_1136063 = 0;
        Common::setTrackOptions(_trackType);

        _constructionHover = 0;
        _byte_113607E = 1;
        _trackCost = 0x80000000;
        _byte_1136076 = 0;
        _lastSelectedTrackModSection = 0;

        Common::setNextAndPreviousTrackTile(*copyElement, pos);

        const bool isCloserToNext = Common::isPointCloserToNextOrPreviousTile(Input::getDragLastLocation(), *viewport);

        const auto chosenLoc = isCloserToNext ? *_nextTile : *_previousTile;
        const auto chosenRotation = isCloserToNext ? *_nextTileRotation : *_previousTileRotation;
        _x = chosenLoc.x;
        _y = chosenLoc.y;
        _constructionZ = chosenLoc.z;
        _constructionRotation = chosenRotation;
        _lastSelectedTrackPiece = 0;
        _lastSelectedTrackGradient = 0;

        Common::refreshSignalList(_signalList, _trackType);
        auto lastSignal = _scenarioSignals[_trackType];

        if (lastSignal == 0xFF)
            lastSignal = _signalList[0];

        _lastSelectedSignal = lastSignal;

        Common::refreshStationList(_stationList, _trackType, TransportMode::rail);

        auto lastStation = _scenarioTrainStations[_trackType];

        if (lastStation == 0xFF)
            lastStation = _stationList[0];

        _lastSelectedStationType = lastStation;

        Common::refreshBridgeList(_bridgeList, _trackType, TransportMode::rail);

        auto lastBridge = _scenarioBridges[_trackType];

        if (lastBridge == 0xFF)
            lastBridge = _bridgeList[0];

        _lastSelectedBridge = lastBridge;

        if (copyElement->hasBridge())
        {
            _lastSelectedBridge = copyElement->bridge();
        }
        Common::refreshModList(_modList, _trackType, TransportMode::rail);

        _lastSelectedMods = copyElement->mods();
        _byte_113603A = 0;
        auto* window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            Common::setDisabledWidgets(window);
        }

        return window;
    }

    // 0x004A147F
    Window* openAtRoad(Window* main, RoadElement* road, const Pos2 pos)
    {
        auto* viewport = main->viewports[0];
        _backupTileElement = *reinterpret_cast<TileElement*>(road);
        auto* copyElement = (*_backupTileElement).asRoad();
        if (copyElement == nullptr)
        {
            return nullptr;
        }

        removeConstructionGhosts();
        auto* wnd = WindowManager::find(WindowType::construction);
        if (wnd == nullptr)
        {
            WindowManager::closeConstructionWindows();
            Common::createConstructionWindow();
        }
        else
        {
            Common::resetWindow(*wnd, Common::widx::tab_construction);
        }

        _trackType = copyElement->roadObjectId() | (1 << 7);
        _byte_1136063 = 0;
        Common::setTrackOptions(_trackType);

        _constructionHover = 0;
        _byte_113607E = 1;
        _trackCost = 0x80000000;
        _byte_1136076 = 0;
        _lastSelectedTrackModSection = 0;

        Common::setNextAndPreviousRoadTile(*copyElement, pos);

        const bool isCloserToNext = Common::isPointCloserToNextOrPreviousTile(Input::getDragLastLocation(), *viewport);

        const auto chosenLoc = isCloserToNext ? *_nextTile : *_previousTile;
        const auto chosenRotation = isCloserToNext ? *_nextTileRotation : *_previousTileRotation;
        _x = chosenLoc.x;
        _y = chosenLoc.y;
        _constructionZ = chosenLoc.z;
        _constructionRotation = chosenRotation;
        _lastSelectedTrackPiece = 0;
        _lastSelectedTrackGradient = 0;
        _lastSelectedSignal = 0xFF;

        Common::refreshStationList(_stationList, _trackType, TransportMode::road);

        auto lastStation = _scenarioRoadStations[(_trackType & ~(1ULL << 7))];

        if (lastStation == 0xFF)
            lastStation = _stationList[0];

        _lastSelectedStationType = lastStation;

        Common::refreshBridgeList(_bridgeList, _trackType, TransportMode::road);

        auto lastBridge = _scenarioBridges[(_trackType & ~(1ULL << 7))];

        if (lastBridge == 0xFF)
            lastBridge = _bridgeList[0];

        _lastSelectedBridge = lastBridge;
        if (copyElement->hasBridge())
        {
            _lastSelectedBridge = copyElement->bridge();
        }

        Common::refreshModList(_modList, _trackType, TransportMode::road);

        _lastSelectedMods = 0;
        auto* roadObj = ObjectManager::get<RoadObject>(_trackType & ~(1ULL << 7));
        if (!(roadObj->flags & Flags12::unk_03))
        {
            _lastSelectedMods = copyElement->mods();
        }
        _byte_113603A = 0;

        auto* window = WindowManager::find(WindowType::construction);

        if (window != nullptr)
        {
            Common::setDisabledWidgets(window);
        }

        return window;
    }

    // 0x004A1303
    void setToTrackExtra(Window* main, TrackElement* track, const uint8_t bh, const Pos2 pos)
    {
        registers regs{};
        regs.esi = X86Pointer(main);
        regs.edx = X86Pointer(track);
        regs.bh = bh;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A1303, regs);
    }

    // 0x004A13C1
    void setToRoadExtra(Window* main, RoadElement* road, const uint8_t bh, const Pos2 pos)
    {
        registers regs{};
        regs.esi = X86Pointer(main);
        regs.edx = X86Pointer(road);
        regs.bh = bh;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x004A13C1, regs);
    }

    // 0x004A3B0D
    Window* openWithFlags(const uint32_t flags)
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
                auto roadObj = ObjectManager::get<RoadObject>(trackType);

                if (roadObj->flags & Flags12::unk_03)
                {
                    if (_trackType & (1 << 7))
                    {
                        trackType = _trackType & ~(1 << 7);
                        roadObj = ObjectManager::get<RoadObject>(trackType);

                        if (roadObj->flags & Flags12::unk_03)
                        {
                            _trackType = static_cast<uint8_t>(flags);

                            Common::sub_4A3A50();

                            _lastSelectedTrackPiece = 0;
                            _lastSelectedTrackGradient = 0;

                            return window;
                        }
                    }
                }
            }
        }

        WindowManager::closeConstructionWindows();
        Common::sub_4CD454();

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
        _constructionZ = 0x100;
        _constructionRotation = 0;
        _constructionHover = 0;
        _byte_113607E = 1;
        _trackCost = 0x80000000;
        _byte_1136076 = 0;
        _lastSelectedTrackPiece = 0;
        _lastSelectedTrackGradient = 0;
        _lastSelectedTrackModSection = 0;

        Common::setTrackOptions(flags);

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
        if (window->current_tab == Common::widx::tab_station - Common::widx::tab_construction)
        {
            if (_byte_1136063 & ((1 << 7) | (1 << 6)))
                WindowManager::close(window);
            else
                window->callOnMouseUp(Common::widx::tab_construction);
        }
    }

    // 0x004A6A0C
    bool isStationTabOpen()
    {
        auto* window = WindowManager::find(WindowType::construction);
        if (window == nullptr)
        {
            return false;
        }

        return window->current_tab == Common::widx::tab_station - Common::widx::tab_construction;
    }

    // 0x004A69EE
    bool isOverheadTabOpen()
    {
        auto* window = WindowManager::find(WindowType::construction);
        if (window == nullptr)
        {
            return false;
        }

        return window->current_tab == Common::widx::tab_overhead - Common::widx::tab_construction;
    }

    // 0x004A6A2A
    bool isSignalTabOpen()
    {
        auto* window = WindowManager::find(WindowType::construction);
        if (window == nullptr)
        {
            return false;
        }

        return window->current_tab == Common::widx::tab_signal - Common::widx::tab_construction;
    }

    // 0x0049FEC7
    void removeConstructionGhosts()
    {
        if (_byte_522096 & (1 << 0))
        {
            Map::TileManager::mapInvalidateTileFull(Map::Pos2(_x, _y));
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::unk_02);
            _byte_522096 = _byte_522096 & ~(1 << 0);
        }
        Construction::removeTrackGhosts();
        Signal::removeSignalGhost();
        Station::removeStationGhost();
        Overhead::removeTrackModsGhost();
    }

    namespace Common
    {
        struct TabInformation
        {
            Widget* widgets;
            const widx widgetIndex;
            WindowEventList* events;
            const uint64_t enabledWidgets;
            void (*tabReset)(Window*);
        };

        static TabInformation tabInformationByTabOffset[] = {
            { Construction::widgets, widx::tab_construction, &Construction::events, Construction::enabledWidgets, &Construction::tabReset },
            { Station::widgets, widx::tab_station, &Station::events, Station::enabledWidgets, &Station::tabReset },
            { Signal::widgets, widx::tab_signal, &Signal::events, Signal::enabledWidgets, &Signal::tabReset },
            { Overhead::widgets, widx::tab_overhead, &Overhead::events, Overhead::enabledWidgets, &Overhead::tabReset },
        };

        void prepareDraw(Window* self)
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
            self->activated_widgets |= (1ULL << Common::tabInformationByTabOffset[self->current_tab].widgetIndex);
        }

        // 0x004A0EF4
        void resetWindow(Window& self, WidgetIndex_t tabWidgetIndex)
        {
            self.current_tab = tabWidgetIndex - widx::tab_construction;

            const auto& tabInfo = tabInformationByTabOffset[tabWidgetIndex - widx::tab_construction];

            self.enabled_widgets = tabInfo.enabledWidgets;
            self.event_handlers = tabInfo.events;
            self.activated_widgets = 0;
            self.widgets = tabInfo.widgets;

            setDisabledWidgets(&self);

            self.width = self.widgets[widx::frame].right + 1;
            self.height = self.widgets[widx::frame].bottom + 1;
        }

        // Reverse direction map?
        static loco_global<uint8_t[16], 0x00503CAC> _503CAC;
        static loco_global<Map::Pos2[16], 0x00503C6C> _503C6C;

        void setNextAndPreviousTrackTile(const TrackElement& elTrack, const Map::Pos2& pos)
        {
            const auto& piece = TrackData::getTrackPiece(elTrack.trackId())[elTrack.sequenceIndex()];
            const auto firstTileOffset = Math::Vector::rotate(Map::Pos2(piece.x, piece.y), elTrack.unkDirection());
            const auto firstTile = Map::Pos3(pos.x, pos.y, elTrack.baseZ() * 4) - Map::Pos3(firstTileOffset.x, firstTileOffset.y, piece.z);

            // Get coordinates of the next tile after the end of the track piece
            const auto trackAndDirection = (elTrack.trackId() << 3) | elTrack.unkDirection();
            const auto& trackSize = TrackData::getUnkTrack(trackAndDirection);
            const auto nextTile = firstTile + trackSize.pos;
            _nextTile = nextTile;
            _nextTileRotation = trackSize.rotationEnd;

            // Get coordinates of the previous tile before the start of the track piece
            const auto unk = _503CAC[trackSize.rotationBegin];
            auto previousTile = firstTile;
            _previousTileRotation = unk;
            if (unk < 12)
            {
                previousTile += Map::Pos3{ _503C6C[unk] };
            }
            _previousTile = previousTile;
        }

        void setNextAndPreviousRoadTile(const RoadElement& elRoad, const Map::Pos2& pos)
        {
            const auto& piece = TrackData::getRoadPiece(elRoad.roadId())[elRoad.sequenceIndex()];
            const auto firstTileOffset = Math::Vector::rotate(Map::Pos2(piece.x, piece.y), elRoad.unkDirection());
            const auto firstTile = Map::Pos3(pos.x, pos.y, elRoad.baseZ() * 4) - Map::Pos3(firstTileOffset.x, firstTileOffset.y, piece.z);

            // Get coordinates of the next tile after the end of the track piece
            const auto trackAndDirection = (elRoad.roadId() << 3) | elRoad.unkDirection();
            const auto& trackSize = TrackData::getUnkRoad(trackAndDirection);
            const auto nextTile = firstTile + trackSize.pos;
            _nextTile = nextTile;
            _nextTileRotation = trackSize.rotationEnd;

            // Get coordinates of the previous tile before the start of the track piece
            const auto unk = _503CAC[trackSize.rotationBegin];
            auto previousTile = firstTile;
            _previousTileRotation = unk;
            if (unk < 12)
            {
                previousTile += Map::Pos3{ _503C6C[unk] };
            }
            _previousTile = previousTile;
        }

        // True for next, false for previous
        bool isPointCloserToNextOrPreviousTile(const Point& point, const Viewport& viewport)
        {
            const auto vpPosNext = gameToScreen(*_nextTile + Map::Pos3(16, 16, 0), viewport.getRotation());
            const auto uiPosNext = viewport.viewportToScreen(vpPosNext);
            const auto distanceToNext = Math::Vector::manhattanDistance(uiPosNext, point);

            const auto vpPosPrevious = gameToScreen(*_previousTile + Map::Pos3(16, 16, 0), viewport.getRotation());
            const auto uiPosPrevious = viewport.viewportToScreen(vpPosPrevious);
            const auto distanceToPrevious = Math::Vector::manhattanDistance(uiPosPrevious, point);

            return distanceToNext < distanceToPrevious;
        }

        // 0x0049D93A
        void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (self->current_tab == widgetIndex - widx::tab_construction)
                return;

            if (widgetIndex == widx::tab_station)
            {
                Ui::Windows::Station::showStationCatchment(StationId::null);
            }

            if (widgetIndex == widx::tab_construction)
            {
                Construction::activateSelectedConstructionWidgets();
            }

            removeConstructionGhosts();
            TileManager::mapInvalidateMapSelectionTiles();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enableConstruct);
            _trackCost = 0x80000000;
            _signalCost = 0x80000000;
            _stationCost = 0x80000000;
            _modCost = 0x80000000;
            _byte_1136076 = 0;

            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

            self->current_tab = widgetIndex - widx::tab_construction;
            self->frame_no = 0;
            self->flags &= ~(WindowFlags::flag_16);

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
        static void drawRoadTabs(Window* self, Gfx::Context* context)
        {
            auto company = CompanyManager::get(_playerCompany);
            auto companyColour = company->mainColours.primary;
            auto roadObj = ObjectManager::get<RoadObject>(_trackType & ~(1 << 7));
            // Construction Tab
            {
                auto imageId = roadObj->image;
                if (self->current_tab == widx::tab_construction - widx::tab_construction)
                    imageId += (self->frame_no / 4) % 32;

                Widget::drawTab(self, context, Gfx::recolour(imageId, companyColour), widx::tab_construction);
            }
            // Station Tab
            {
                Widget::drawTab(self, context, ImageIds::null, widx::tab_station);
                if (!self->isDisabled(widx::tab_station))
                {
                    auto x = self->widgets[widx::tab_station].left + self->x + 1;
                    auto y = self->widgets[widx::tab_station].top + self->y + 1;
                    auto width = 29;
                    auto height = 25;
                    if (self->current_tab == widx::tab_station - widx::tab_construction)
                        height++;

                    auto clipped = Gfx::clipContext(*context, Ui::Rect(x, y, width, height));
                    if (clipped)
                    {
                        clipped->zoom_level = 1;
                        clipped->width <<= 1;
                        clipped->height <<= 1;
                        clipped->x <<= 1;
                        clipped->y <<= 1;
                        auto roadStationObj = ObjectManager::get<RoadStationObject>(_lastSelectedStationType);
                        auto imageId = Gfx::recolour(roadStationObj->image, companyColour);
                        Gfx::drawImage(&*clipped, -4, -10, imageId);
                        auto colour = _byte_5045FA[companyColour];
                        if (!(roadStationObj->flags & RoadStationFlags::recolourable))
                        {
                            colour = PaletteIndex::index_2E;
                        }
                        imageId = Gfx::recolour(roadStationObj->image, colour) + 1;
                        Gfx::drawImage(&*clipped, -4, -10, imageId);
                    }

                    Widget::drawTab(self, context, -2, widx::tab_station);
                }
            }
            // Overhead tab
            {
                Widget::drawTab(self, context, ImageIds::null, widx::tab_overhead);
                if (!self->isDisabled(widx::tab_station))
                {
                    auto x = self->widgets[widx::tab_overhead].left + self->x + 2;
                    auto y = self->widgets[widx::tab_overhead].top + self->y + 2;

                    for (auto i = 0; i < 2; i++)
                    {
                        if (_modList[i] != 0xFF)
                        {
                            auto roadExtraObj = ObjectManager::get<RoadExtraObject>(_modList[i]);
                            auto imageId = roadExtraObj->var_0E;
                            if (self->current_tab == widx::tab_overhead - widx::tab_construction)
                                imageId += (self->frame_no / 2) % 8;
                            Gfx::drawImage(context, x, y, imageId);
                        }
                    }

                    Widget::drawTab(self, context, -2, widx::tab_overhead);
                }
            }
        }

        // 0x0049ED40
        static void drawTrackTabs(Window* self, Gfx::Context* context)
        {
            auto company = CompanyManager::get(_playerCompany);
            auto companyColour = company->mainColours.primary;
            auto trackObj = ObjectManager::get<TrackObject>(_trackType);
            // Construction Tab
            {
                auto imageId = trackObj->image;
                if (self->current_tab == widx::tab_construction - widx::tab_construction)
                    imageId += (self->frame_no / 4) % 15;

                Widget::drawTab(self, context, Gfx::recolour(imageId, companyColour), widx::tab_construction);
            }
            // Station Tab
            {
                if (_byte_1136063 & (1 << 7))
                {
                    auto imageId = ObjectManager::get<InterfaceSkinObject>()->img + InterfaceSkin::ImageIds::toolbar_menu_airport;

                    Widget::drawTab(self, context, imageId, widx::tab_station);
                }
                else
                {
                    if (_byte_1136063 & (1 << 6))
                    {
                        auto imageId = ObjectManager::get<InterfaceSkinObject>()->img + InterfaceSkin::ImageIds::toolbar_menu_ship_port;

                        Widget::drawTab(self, context, imageId, widx::tab_station);
                    }
                    else
                    {
                        Widget::drawTab(self, context, ImageIds::null, widx::tab_station);
                        if (!self->isDisabled(widx::tab_station))
                        {
                            auto x = self->widgets[widx::tab_station].left + self->x + 1;
                            auto y = self->widgets[widx::tab_station].top + self->y + 1;
                            auto width = 29;
                            auto height = 25;
                            if (self->current_tab == widx::tab_station - widx::tab_construction)
                                height++;

                            auto clipped = Gfx::clipContext(*context, Ui::Rect(x, y, width, height));
                            if (clipped)
                            {
                                clipped->zoom_level = 1;
                                clipped->width *= 2;
                                clipped->height *= 2;
                                clipped->x *= 2;
                                clipped->y *= 2;

                                auto trainStationObj = ObjectManager::get<TrainStationObject>(_lastSelectedStationType);
                                auto imageId = Gfx::recolour(trainStationObj->image + TrainStation::ImageIds::preview_image, companyColour);
                                Gfx::drawImage(&*clipped, -4, -9, imageId);

                                auto colour = _byte_5045FA[companyColour];
                                if (!(trainStationObj->flags & TrainStationFlags::recolourable))
                                {
                                    colour = PaletteIndex::index_2E;
                                }
                                imageId = Gfx::recolourTranslucent(trainStationObj->image + TrainStation::ImageIds::preview_image_windows, colour);
                                Gfx::drawImage(&*clipped, -4, -9, imageId);
                            }

                            Widget::drawTab(self, context, -2, widx::tab_station);
                        }
                    }
                }
            }
            // Signal Tab
            {
                Widget::drawTab(self, context, ImageIds::null, widx::tab_signal);
                if (!self->isDisabled(widx::tab_signal))
                {
                    auto x = self->widgets[widx::tab_signal].left + self->x + 1;
                    auto y = self->widgets[widx::tab_signal].top + self->y + 1;
                    auto width = 29;
                    auto height = 25;
                    if (self->current_tab == widx::tab_station - widx::tab_construction)
                        height++;

                    auto clipped = Gfx::clipContext(*context, Ui::Rect(x, y, width, height));
                    if (clipped)
                    {
                        auto trainSignalObject = ObjectManager::get<TrainSignalObject>(_lastSelectedSignal);
                        auto imageId = trainSignalObject->image;
                        if (self->current_tab == widx::tab_signal - widx::tab_construction)
                        {
                            auto frames = signalFrames[(((trainSignalObject->num_frames + 2) / 3) - 2)];
                            auto frameCount = std::size(frames) - 1;
                            frameCount &= (self->frame_no >> trainSignalObject->animationSpeed);
                            auto frameIndex = frames[frameCount];
                            frameIndex <<= 3;
                            imageId += frameIndex;
                        }
                        Gfx::drawImage(&*clipped, 15, 31, imageId);
                    }

                    Widget::drawTab(self, context, -2, widx::tab_signal);
                }
            }
            // Overhead Tab
            {
                Widget::drawTab(self, context, ImageIds::null, widx::tab_overhead);
                if (!self->isDisabled(widx::tab_station))
                {
                    auto x = self->widgets[widx::tab_overhead].left + self->x + 2;
                    auto y = self->widgets[widx::tab_overhead].top + self->y + 2;

                    for (auto i = 0; i < 4; i++)
                    {
                        if (_modList[i] != 0xFF)
                        {
                            auto trackExtraObj = ObjectManager::get<TrackExtraObject>(_modList[i]);
                            auto imageId = trackExtraObj->var_0E;
                            if (self->current_tab == widx::tab_overhead - widx::tab_construction)
                                imageId += (self->frame_no / 2) % 8;
                            Gfx::drawImage(context, x, y, imageId);
                        }
                    }

                    Widget::drawTab(self, context, -2, widx::tab_overhead);
                }
            }
        }

        // 0x0049ED33
        void drawTabs(Window* self, Gfx::Context* context)
        {
            if (_trackType & (1 << 7))
            {
                drawRoadTabs(self, context);
            }
            else
            {
                drawTrackTabs(self, context);
            }
        }

        // 0x004A09DE
        void repositionTabs(Window* self)
        {
            int16_t xPos = self->widgets[widx::tab_construction].left;
            const int16_t tabWidth = self->widgets[widx::tab_construction].right - xPos;

            for (uint8_t i = widx::tab_construction; i <= widx::tab_overhead; i++)
            {
                if (self->isDisabled(i))
                {
                    self->widgets[i].type = WidgetType::none;
                    continue;
                }

                self->widgets[i].type = WidgetType::wt_8;
                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        // 0x0049DD14
        void onClose(Window* self)
        {
            removeConstructionGhosts();
            WindowManager::viewportSetVisibility(WindowManager::ViewportVisibility::reset);
            TileManager::mapInvalidateMapSelectionTiles();
            Input::resetMapSelectionFlag(Input::MapSelectionFlags::enableConstruct);
            hideDirectionArrows();
            hideGridlines();
        }

        // 0x0049E437, 0x0049E76F, 0x0049ECD1
        void onUpdate(Window* self, uint8_t flag)
        {
            self->frame_no++;
            self->callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::construction, self->number, self->current_tab + Common::widx::tab_construction);

            if (Input::isToolActive(WindowType::construction, self->number))
                return;

            if (!(_byte_522096 & flag))
                return;

            removeConstructionGhosts();
        }

        void initEvents()
        {
            Construction::initEvents();
            Station::initEvents();
            Signal::initEvents();
            Overhead::initEvents();
        }

        // 0x004CD454
        void sub_4CD454()
        {
            if (isNetworkHost())
            {
                auto window = WindowManager::find(_toolWindowType, _toolWindowNumber);
                if (window != nullptr)
                    Input::toolCancel();
            }
        }

        // 0x004A3A06
        void setTrackOptions(const uint8_t trackType)
        {
            auto newTrackType = trackType;
            if (trackType & (1 << 7))
            {
                newTrackType &= ~(1 << 7);
                auto roadObj = ObjectManager::get<RoadObject>(newTrackType);
                if (!(roadObj->flags & Flags12::unk_01))
                    _lastRoadOption = trackType;
                else
                    _lastRailroadOption = trackType;
            }
            else
            {
                auto trackObj = ObjectManager::get<TrackObject>(newTrackType);
                if (!(trackObj->flags & Flags22::unk_02))
                    _lastRailroadOption = trackType;
                else
                    _lastRoadOption = trackType;
            }
            WindowManager::invalidate(WindowType::topToolbar, 0);
        }

        // 0x0049CE33
        void setDisabledWidgets(Window* self)
        {
            auto disabledWidgets = 0;
            if (isEditorMode())
                disabledWidgets |= (1ULL << Common::widx::tab_station);

            if (_byte_1136063 & (1 << 7 | 1 << 6))
                disabledWidgets |= (1ULL << Common::widx::tab_construction);

            if (_lastSelectedSignal == 0xFF)
                disabledWidgets |= (1ULL << Common::widx::tab_signal);

            if (_modList[0] == 0xFF && _modList[1] == 0xFF && _modList[2] == 0xFF && _modList[3] == 0xFF)
                disabledWidgets |= (1ULL << Common::widx::tab_overhead);

            if (_lastSelectedStationType == 0xFF)
                disabledWidgets |= (1ULL << Common::widx::tab_station);

            self->disabled_widgets = disabledWidgets;
        }

        // 0x004A0963
        void createConstructionWindow()
        {
            auto window = WindowManager::createWindow(
                WindowType::construction,
                Construction::windowSize,
                WindowFlags::flag_11 | WindowFlags::no_auto_close,
                &Construction::events);

            window->widgets = Construction::widgets;
            window->current_tab = 0;
            window->enabled_widgets = Construction::enabledWidgets;
            window->activated_widgets = 0;

            setDisabledWidgets(window);

            window->initScrollWidgets();
            window->owner = _playerCompany;

            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->colour_0D);

            WindowManager::sub_4CEE0B(window);
            Ui::Windows::showDirectionArrows();
            Ui::Windows::showGridlines();

            Common::initEvents();
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
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::airport); i++)
            {
                auto airportObj = ObjectManager::get<AirportObject>(i);
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
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::dock); i++)
            {
                auto dockObj = ObjectManager::get<DockObject>(i);
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
                auto roadObj = ObjectManager::get<RoadObject>(trackType);

                for (auto i = 0; i < roadObj->num_stations; i++)
                {
                    auto station = roadObj->stations[i];
                    if (station == 0xFF)
                        continue;
                    auto roadStationObj = ObjectManager::get<RoadStationObject>(station);
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
                auto trackObj = ObjectManager::get<TrackObject>(trackType);

                for (auto i = 0; i < trackObj->num_stations; i++)
                {
                    auto station = trackObj->stations[i];
                    if (station == 0xFF)
                        continue;
                    auto trainStationObj = ObjectManager::get<TrainStationObject>(station);
                    if (currentYear < trainStationObj->designed_year)
                        continue;
                    if (currentYear > trainStationObj->obsolete_year)
                        continue;
                    stationList[stationCount] = station;
                    stationCount++;
                }
            }

            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::roadStation); i++)
            {
                uint8_t numCompatible;
                uint8_t* mods;
                uint16_t designedYear;
                uint16_t obsoleteYear;

                if (transportMode == TransportMode::road)
                {
                    auto roadStationObj = ObjectManager::get<RoadStationObject>(i);

                    if (roadStationObj == nullptr)
                        continue;

                    numCompatible = roadStationObj->num_compatible;
                    mods = roadStationObj->mods;
                    designedYear = roadStationObj->designed_year;
                    obsoleteYear = roadStationObj->obsolete_year;
                }
                else if (transportMode == TransportMode::rail)
                {
                    auto trainStationObj = ObjectManager::get<TrainStationObject>(i);

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
                auto roadObj = ObjectManager::get<RoadObject>(trackType);
                for (auto i = 0; i < roadObj->num_bridges; i++)
                {
                    auto bridge = roadObj->bridges[i];
                    if (bridge == 0xFF)
                        continue;
                    auto bridgeObj = ObjectManager::get<BridgeObject>(bridge);
                    if (currentYear < bridgeObj->designed_year)
                        continue;
                    bridgeList[bridgeCount] = bridge;
                    bridgeCount++;
                }
            }

            if (transportMode == TransportMode::rail)
            {
                auto trackObj = ObjectManager::get<TrackObject>(trackType);
                for (auto i = 0; i < trackObj->num_bridges; i++)
                {
                    auto bridge = trackObj->bridges[i];
                    if (bridge == 0xFF)
                        continue;
                    auto bridgeObj = ObjectManager::get<BridgeObject>(bridge);
                    if (currentYear < bridgeObj->designed_year)
                        continue;
                    bridgeList[bridgeCount] = bridge;
                    bridgeCount++;
                }
            }

            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::bridge); i++)
            {
                auto bridgeObj = ObjectManager::get<BridgeObject>(i);
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

            auto companyId = CompanyManager::updatingCompanyId();

            modList[0] = 0xFF;
            modList[1] = 0xFF;
            modList[2] = 0xFF;
            modList[3] = 0xFF;
            auto flags = 0;

            for (uint8_t vehicle = 0; vehicle < ObjectManager::getMaxObjects(ObjectType::vehicle); vehicle++)
            {
                auto vehicleObj = ObjectManager::get<VehicleObject>(vehicle);

                if (vehicleObj == nullptr)
                    continue;

                if (vehicleObj->mode != transportMode)
                    continue;

                if (trackType != vehicleObj->track_type)
                    continue;

                auto company = CompanyManager::get(companyId);

                if (!company->isVehicleIndexUnlocked(vehicle))
                    continue;

                for (auto i = 0; i < vehicleObj->num_mods; i++)
                {
                    flags |= 1ULL << vehicleObj->required_track_extras[i];
                }

                if (!(vehicleObj->flags & FlagsE0::rack_rail))
                    continue;

                flags |= 1ULL << vehicleObj->rack_rail_type;
            }

            if (transportMode == TransportMode::road)
            {
                auto roadObj = ObjectManager::get<RoadObject>(trackType);

                for (auto i = 0; i < roadObj->num_mods; i++)
                {
                    if (flags & (1 << roadObj->mods[i]))
                        modList[i] = roadObj->mods[i];
                }
            }

            if (transportMode == TransportMode::rail)
            {
                auto trackObj = ObjectManager::get<TrackObject>(trackType);

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
            removeConstructionGhosts();
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
            Construction::activateSelectedConstructionWidgets();
        }

        // 0x00488B4D
        void refreshSignalList(uint8_t* signalList, uint8_t trackType)
        {
            auto currentYear = getCurrentYear();
            auto trackObj = ObjectManager::get<TrackObject>(trackType);
            auto signalCount = 0;
            auto var_0E = trackObj->var_0E;
            while (var_0E > 0)
            {
                auto ecx = Utility::bitScanForward(var_0E);
                if (ecx == -1)
                    break;
                var_0E &= ~(1 << ecx);
                auto signalObj = ObjectManager::get<TrainSignalObject>(ecx);

                if (currentYear > signalObj->obsolete_year)
                    continue;
                if (currentYear < signalObj->designed_year)
                    continue;
                signalList[signalCount] = ecx;
                signalCount++;
            }

            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::trackSignal); i++)
            {
                auto signalObj = ObjectManager::get<TrainSignalObject>(i);
                if (signalObj == nullptr)
                    continue;
                for (auto modCount = 0; modCount < signalObj->num_compatible; modCount++)
                {
                    if (trackType != ObjectManager::get<TrainSignalObject>(i)->mods[modCount])
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

        void previousTab(Window* self)
        {
            WidgetIndex_t prev = self->prevAvailableWidgetInRange(widx::tab_construction, widx::tab_overhead);
            if (prev != -1)
                self->callOnMouseUp(prev);
        }

        void nextTab(Window* self)
        {
            WidgetIndex_t next = self->nextAvailableWidgetInRange(widx::tab_construction, widx::tab_overhead);
            if (next != -1)
                self->callOnMouseUp(next);
        }
    }

    bool rotate(Window* self)
    {
        switch (self->current_tab)
        {
            case Common::widx::tab_construction - Common::widx::tab_construction:
                if (_constructionHover == 1)
                {
                    self->callOnMouseUp(Construction::widx::rotate_90);
                    removeConstructionGhosts();
                    return true;
                }
                break;

            case Common::widx::tab_station - Common::widx::tab_construction:
                if (self->widgets[Station::widx::rotate].type != WidgetType::none)
                {
                    self->callOnMouseUp(Station::widx::rotate);
                    return true;
                }
                break;
        }
        return false;
    }

    void registerHooks()
    {
        registerHook(
            0x0049F1B5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Construction::activateSelectedConstructionWidgets();
                regs = backup;
                return 0;
            });

        //registerHook(
        //    0x0049DC97,
        //    [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
        //        registers backup = regs;
        //        construction::on_tool_down(*((Ui::window*)regs.esi), regs.dx, regs.ax, regs.cx);
        //        regs = backup;
        //        return 0;
        //    });
    }
}
