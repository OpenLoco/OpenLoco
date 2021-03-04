#include "../../CompanyManager.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Objects/AirportObject.h"
#include "../../Objects/CargoObject.h"
#include "../../Objects/DockObject.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/RoadObject.h"
#include "../../Objects/RoadStationObject.h"
#include "../../Objects/TrackObject.h"
#include "../../Objects/TrainStationObject.h"
#include "../../Ptr.h"
#include "../../StationManager.h"
#include "../../Ui/Dropdown.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Station
{
    widget_t widgets[] = {
        commonWidgets(138, 190, StringIds::stringid_2),
        makeWidget({ 3, 45 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, StringIds::tooltip_select_station_type),
        makeWidget({ 123, 46 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown, StringIds::tooltip_select_station_type),
        makeWidget({ 35, 60 }, { 68, 68 }, widget_type::wt_3, 1),
        makeWidget({ 112, 104 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::rotate_object, StringIds::rotate_90),
        widgetEnd(),
    };

    window_event_list events;

    // 0x0049E228
    static void onMouseUp(window* self, widget_index widgetIndex)
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

            case widx::rotate:
                _constructionRotation++;
                _constructionRotation = _constructionRotation & 3;
                _stationCost = 0x80000000;
                self->invalidate();
                break;
        }
    }

    template<typename obj_type>
    void AddStationsToDropdown(const uint8_t stationCount)
    {
        for (auto stationIndex = 0; stationIndex < stationCount; stationIndex++)
        {
            auto station = _stationList[stationIndex];
            if (station == _lastSelectedStationType)
                Dropdown::setHighlightedItem(stationIndex);

            auto obj = ObjectManager::get<obj_type>(station);
            Dropdown::add(stationIndex, obj->name);
        }
    }

    // 0x0049E249
    static void onMouseDown(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::station_dropdown:
            {
                uint8_t stationCount = 0;
                while (_stationList[stationCount] != 0xFF)
                    stationCount++;

                auto widget = self->widgets[widx::station];
                auto xPos = widget.left + self->x;
                auto yPos = widget.top + self->y;
                auto width = widget.width() + 2;
                auto height = widget.height();
                Dropdown::show(xPos, yPos, width, height, self->colours[1], stationCount, (1 << 7));

                if (_byte_1136063 & (1 << 7))
                {
                    AddStationsToDropdown<airport_object>(stationCount);
                }
                else if (_byte_1136063 & (1 << 6))
                {
                    AddStationsToDropdown<dock_object>(stationCount);
                }
                else if (_trackType & (1 << 7))
                {
                    AddStationsToDropdown<road_station_object>(stationCount);
                }
                else
                {
                    AddStationsToDropdown<train_station_object>(stationCount);
                }
                break;
            }
            case widx::image:
            {
                Input::toolCancel();
                Input::toolSet(self, widgetIndex, 44);
                break;
            }
        }
    }

    // 0x0049E256
    static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex == widx::station_dropdown)
        {
            if (itemIndex == -1)
                return;

            auto selectedStation = _stationList[itemIndex];
            _lastSelectedStationType = selectedStation;

            if (_byte_1136063 & (1 << 7))
            {
                _lastAirport = selectedStation;
            }
            else if (_byte_1136063 & (1 << 6))
            {
                _lastShipPort = selectedStation;
            }
            else if (_trackType & (1 << 7))
            {
                auto trackType = _trackType & ~(1 << 7);
                _scenarioRoadStations[trackType] = selectedStation;
            }
            else
            {
                _scenarioTrainStations[_trackType] = selectedStation;
            }

            self->invalidate();
        }
    }

    // 0x0049E437
    static void onUpdate(window* self)
    {
        Common::onUpdate(self, (1 << 3));
    }

    // 0x0049E421
    static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = ToInt(&self);
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E421, regs);
    }

    // 0x0049E42C
    static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = ToInt(&self);
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E42C, regs);
    }

    // 0x0049DD39
    static void prepareDraw(window* self)
    {
        Common::prepareDraw(self);

        self->widgets[widx::rotate].type = widget_type::none;

        auto args = FormatArguments();

        if (_byte_1136063 & (1 << 7))
        {
            self->widgets[widx::rotate].type = widget_type::wt_9;

            auto airportObj = ObjectManager::get<airport_object>(_lastSelectedStationType);

            self->widgets[widx::station].text = airportObj->name;

            args.push(StringIds::title_airport);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            auto dockObj = ObjectManager::get<dock_object>(_lastSelectedStationType);

            self->widgets[widx::station].text = dockObj->name;

            args.push(StringIds::title_ship_port);
        }
        else if (_trackType & (1 << 7))
        {
            auto trackType = _trackType & ~(1 << 7);

            auto roadObj = ObjectManager::get<road_object>(trackType);

            args.push(roadObj->name);

            auto roadStationObject = ObjectManager::get<road_station_object>(_lastSelectedStationType);

            self->widgets[widx::station].text = roadStationObject->name;
        }
        else
        {
            auto trackObj = ObjectManager::get<track_object>(_trackType);

            args.push(trackObj->name);

            auto trainStationObject = ObjectManager::get<train_station_object>(_lastSelectedStationType);

            self->widgets[widx::station].text = trainStationObject->name;
        }

        Common::repositionTabs(self);
    }

    // 0x0049DE40
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        Common::drawTabs(self, dpi);

        auto company = CompanyManager::get(_playerCompany);
        auto companyColour = company->mainColours.primary;
        int16_t xPos = self->widgets[widx::image].left + self->x;
        int16_t yPos = self->widgets[widx::image].top + self->y;

        if (_byte_1136063 & (1 << 7))
        {
            auto airportObj = ObjectManager::get<airport_object>(_lastSelectedStationType);

            auto imageId = Gfx::recolour(airportObj->image, companyColour);

            Gfx::drawImage(dpi, xPos, yPos, imageId);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            auto dockObj = ObjectManager::get<dock_object>(_lastSelectedStationType);

            auto imageId = Gfx::recolour(dockObj->image, companyColour);

            Gfx::drawImage(dpi, xPos, yPos, imageId);
        }
        else if (_trackType & (1 << 7))
        {
            auto roadStationObj = ObjectManager::get<road_station_object>(_lastSelectedStationType);

            auto imageId = Gfx::recolour(roadStationObj->image, companyColour);

            Gfx::drawImage(dpi, xPos, yPos, imageId);

            auto colour = _byte_5045FA[companyColour];

            if (!(roadStationObj->flags & RoadStationFlags::recolourable))
            {
                colour = 46;
            }

            imageId = Gfx::recolour(imageId, colour) + 1;

            Gfx::drawImage(dpi, xPos, yPos, imageId);
        }
        else
        {
            auto trainStationObj = ObjectManager::get<train_station_object>(_lastSelectedStationType);

            auto imageId = Gfx::recolour(trainStationObj->image, companyColour);

            Gfx::drawImage(dpi, xPos, yPos, imageId);

            auto colour = _byte_5045FA[companyColour];

            if (!(trainStationObj->flags & TrainStationFlags::recolourable))
            {
                colour = 46;
            }

            imageId = Gfx::recolour(imageId, colour) + 1;

            Gfx::drawImage(dpi, xPos, yPos, imageId);
        }

        if (_stationCost != 0x80000000 && _stationCost != 0)
        {
            xPos = self->x + 69;
            yPos = self->widgets[widx::image].bottom + self->y + 4;

            auto args = FormatArguments();
            args.push<uint32_t>(_stationCost);

            Gfx::drawStringCentred(*dpi, xPos, yPos, Colour::black, StringIds::build_cost, &args);
        }

        xPos = self->x + 3;
        yPos = self->widgets[widx::image].bottom + self->y + 16;
        auto width = self->width - 4;
        Gfx::drawRectInset(dpi, xPos, yPos, width, 1, self->colours[1], (1 << 5));

        if (!(_byte_522096 & (1 << 3)))
            return;

        auto args = FormatArguments();

        if (_constructingStationId == 0xFFFFFFFF)
        {
            args.push(StringIds::new_station);
        }
        else
        {
            auto station = StationManager::get(_constructingStationId);
            args.push(station->name);
            args.push(station->town);
        }

        xPos = self->x + 69;
        yPos = self->widgets[widx::image].bottom + self->y + 18;
        width = self->width - 4;
        Gfx::drawStringCentredClipped(*dpi, xPos, yPos, width, Colour::black, StringIds::new_station_buffer, &args);

        xPos = self->x + 2;
        yPos = self->widgets[widx::image].bottom + self->y + 29;
        Gfx::point_t origin = { xPos, yPos };

        Gfx::drawString_494B3F(*dpi, &origin, Colour::black, StringIds::catchment_area_accepts);

        if (_constructingStationAcceptedCargoTypes == 0)
        {
            Gfx::drawString_494B3F(*dpi, origin.x, origin.y, Colour::black, StringIds::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(object_type::cargo); i++)
            {
                if (_constructingStationAcceptedCargoTypes & (1 << i))
                {
                    auto xPosMax = self->x + self->width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = ObjectManager::get<cargo_object>(i);

                        Gfx::drawImage(dpi, origin.x, origin.y, cargoObj->unit_inline_sprite);
                        origin.x += 10;
                    }
                }
            }
        }

        xPos = self->x + 2;
        yPos = self->widgets[widx::image].bottom + self->y + 49;
        origin = { xPos, yPos };

        Gfx::drawString_494B3F(*dpi, &origin, Colour::black, StringIds::catchment_area_produces);

        if (_constructingStationProducedCargoTypes == 0)
        {
            Gfx::drawString_494B3F(*dpi, origin.x, origin.y, Colour::black, StringIds::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(object_type::cargo); i++)
            {
                if (_constructingStationProducedCargoTypes & (1 << i))
                {
                    auto xPosMax = self->x + self->width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = ObjectManager::get<cargo_object>(i);

                        Gfx::drawImage(dpi, origin.x, origin.y, cargoObj->unit_inline_sprite);
                        origin.x += 10;
                    }
                }
            }
        }
    }

    void tabReset(window* self)
    {
        self->callOnMouseDown(Station::widx::image);
    }

    void initEvents()
    {
        events.on_close = Common::onClose;
        events.on_mouse_up = onMouseUp;
        events.on_mouse_down = onMouseDown;
        events.on_dropdown = onDropdown;
        events.on_update = onUpdate;
        events.on_tool_update = onToolUpdate;
        events.on_tool_down = onToolDown;
        events.prepare_draw = prepareDraw;
        events.draw = draw;
    }
}
