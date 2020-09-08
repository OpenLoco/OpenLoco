#include "../../CompanyManager.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Objects/AirportObject.h"
#include "../../Objects/cargo_object.h"
#include "../../Objects/dock_object.h"
#include "../../Objects/objectmgr.h"
#include "../../Objects/road_object.h"
#include "../../Objects/road_station_object.h"
#include "../../Objects/track_object.h"
#include "../../Objects/train_station_object.h"
#include "../../StationManager.h"
#include "../../ui/dropdown.h"
#include "Construction.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction::station
{
    widget_t widgets[] = {
        commonWidgets(138, 190, string_ids::stringid_2),
        makeWidget({ 3, 45 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, string_ids::tooltip_select_station_type),
        makeWidget({ 123, 46 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::tooltip_select_station_type),
        makeWidget({ 35, 60 }, { 68, 68 }, widget_type::wt_3, 1),
        makeWidget({ 112, 104 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rotate_object, string_ids::rotate_90),
        widgetEnd(),
    };

    window_event_list events;

    // 0x0049E228
    static void onMouseUp(window* self, widget_index widgetIndex)
    {
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
                dropdown::setHighlightedItem(stationIndex);

            auto obj = objectmgr::get<obj_type>(station);
            dropdown::add(stationIndex, obj->name);
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
                dropdown::show(xPos, yPos, width, height, self->colours[1], stationCount, (1 << 7));

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
                input::toolCancel();
                input::toolSet(self, widgetIndex, 44);
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
        common::onUpdate(self, (1 << 3));
    }

    // 0x0049E421
    static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E421, regs);
    }

    // 0x0049E42C
    static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E42C, regs);
    }

    // 0x0049DD39
    static void prepareDraw(window* self)
    {
        common::prepareDraw(self);

        self->widgets[widx::rotate].type = widget_type::none;

        auto args = FormatArguments();

        if (_byte_1136063 & (1 << 7))
        {
            self->widgets[widx::rotate].type = widget_type::wt_9;

            auto airportObj = objectmgr::get<airport_object>(_lastSelectedStationType);

            self->widgets[widx::station].text = airportObj->name;

            args.push(string_ids::title_airport);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            auto dockObj = objectmgr::get<dock_object>(_lastSelectedStationType);

            self->widgets[widx::station].text = dockObj->name;

            args.push(string_ids::title_ship_port);
        }
        else if (_trackType & (1 << 7))
        {
            auto trackType = _trackType & ~(1 << 7);

            auto roadObj = objectmgr::get<road_object>(trackType);

            args.push(roadObj->name);

            auto roadStationObject = objectmgr::get<road_station_object>(_lastSelectedStationType);

            self->widgets[widx::station].text = roadStationObject->name;
        }
        else
        {
            auto trackObj = objectmgr::get<track_object>(_trackType);

            args.push(trackObj->name);

            auto trainStationObject = objectmgr::get<train_station_object>(_lastSelectedStationType);

            self->widgets[widx::station].text = trainStationObject->name;
        }

        common::repositionTabs(self);
    }

    // 0x0049DE40
    static void draw(window* self, gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        common::drawTabs(self, dpi);

        auto company = companymgr::get(_playerCompany);
        auto companyColour = company->mainColours.primary;
        int16_t xPos = self->widgets[widx::image].left + self->x;
        int16_t yPos = self->widgets[widx::image].top + self->y;

        if (_byte_1136063 & (1 << 7))
        {
            auto airportObj = objectmgr::get<airport_object>(_lastSelectedStationType);

            auto imageId = gfx::recolour(airportObj->var_08, companyColour);

            gfx::drawImage(dpi, xPos, yPos, imageId);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            auto dockObj = objectmgr::get<dock_object>(_lastSelectedStationType);

            auto imageId = gfx::recolour(dockObj->var_08, companyColour);

            gfx::drawImage(dpi, xPos, yPos, imageId);
        }
        else if (_trackType & (1 << 7))
        {
            auto roadStationObj = objectmgr::get<road_station_object>(_lastSelectedStationType);

            auto imageId = gfx::recolour(roadStationObj->var_0C, companyColour);

            gfx::drawImage(dpi, xPos, yPos, imageId);

            auto colour = _byte_5045FA[companyColour];

            if (!(roadStationObj->flags & road_station_flags::recolourable))
            {
                colour = 46;
            }

            imageId = gfx::recolour(imageId, colour) + 1;

            gfx::drawImage(dpi, xPos, yPos, imageId);
        }
        else
        {
            auto trainStationObj = objectmgr::get<train_station_object>(_lastSelectedStationType);

            auto imageId = gfx::recolour(trainStationObj->var_0E, companyColour);

            gfx::drawImage(dpi, xPos, yPos, imageId);

            auto colour = _byte_5045FA[companyColour];

            if (!(trainStationObj->flags & train_station_flags::recolourable))
            {
                colour = 46;
            }

            imageId = gfx::recolour(imageId, colour) + 1;

            gfx::drawImage(dpi, xPos, yPos, imageId);
        }

        if (_stationCost != 0x80000000 && _stationCost != 0)
        {
            xPos = self->x + 69;
            yPos = self->widgets[widx::image].bottom + self->y + 4;

            auto args = FormatArguments();
            args.push<uint32_t>(_stationCost);

            gfx::drawStringCentred(*dpi, xPos, yPos, colour::black, string_ids::build_cost, &args);
        }

        xPos = self->x + 3;
        yPos = self->widgets[widx::image].bottom + self->y + 16;
        auto width = self->width - 4;
        gfx::drawRectInset(dpi, xPos, yPos, width, 1, self->colours[1], (1 << 5));

        if (!(_byte_522096 & (1 << 3)))
            return;

        auto args = FormatArguments();

        if (_constructingStationId == 0xFFFFFFFF)
        {
            args.push(string_ids::new_station);
        }
        else
        {
            auto station = stationmgr::get(_constructingStationId);
            args.push(station->name);
            args.push(station->town);
        }

        xPos = self->x + 69;
        yPos = self->widgets[widx::image].bottom + self->y + 18;
        width = self->width - 4;
        gfx::drawStringCentredClipped(*dpi, xPos, yPos, width, colour::black, string_ids::new_station_buffer, &args);

        xPos = self->x + 2;
        yPos = self->widgets[widx::image].bottom + self->y + 29;
        gfx::point_t origin = { xPos, yPos };

        gfx::drawString_494B3F(*dpi, &origin, colour::black, string_ids::catchment_area_accepts);

        if (_constructingStationAcceptedCargoTypes == 0)
        {
            gfx::drawString_494B3F(*dpi, origin.x, origin.y, colour::black, string_ids::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::cargo); i++)
            {
                if (_constructingStationAcceptedCargoTypes & (1 << i))
                {
                    auto xPosMax = self->x + self->width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = objectmgr::get<cargo_object>(i);

                        gfx::drawImage(dpi, origin.x, origin.y, cargoObj->unit_inline_sprite);
                        origin.x += 10;
                    }
                }
            }
        }

        xPos = self->x + 2;
        yPos = self->widgets[widx::image].bottom + self->y + 49;
        origin = { xPos, yPos };

        gfx::drawString_494B3F(*dpi, &origin, colour::black, string_ids::catchment_area_produces);

        if (_constructingStationProducedCargoTypes == 0)
        {
            gfx::drawString_494B3F(*dpi, origin.x, origin.y, colour::black, string_ids::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::cargo); i++)
            {
                if (_constructingStationProducedCargoTypes & (1 << i))
                {
                    auto xPosMax = self->x + self->width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = objectmgr::get<cargo_object>(i);

                        gfx::drawImage(dpi, origin.x, origin.y, cargoObj->unit_inline_sprite);
                        origin.x += 10;
                    }
                }
            }
        }
    }

    void tabReset(window* self)
    {
        self->callOnMouseDown(station::widx::image);
    }

    void initEvents()
    {
        events.on_close = common::onClose;
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
