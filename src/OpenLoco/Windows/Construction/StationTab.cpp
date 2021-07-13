#include "../../CompanyManager.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Localisation/StringIds.h"
#include "../../Objects/AirportObject.h"
#include "../../Objects/CargoObject.h"
#include "../../Objects/DockObject.h"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/RoadObject.h"
#include "../../Objects/RoadStationObject.h"
#include "../../Objects/TrackObject.h"
#include "../../Objects/TrainStationObject.h"
#include "../../StationManager.h"
#include "../../Ui/Dropdown.h"
#include "../../Widget.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Station
{
    Widget widgets[] = {
        commonWidgets(138, 190, StringIds::stringid_2),
        makeWidget({ 3, 45 }, { 132, 12 }, WidgetType::wt_18, WindowColour::secondary, 0xFFFFFFFF, StringIds::tooltip_select_station_type),
        makeWidget({ 123, 46 }, { 11, 10 }, WidgetType::wt_11, WindowColour::secondary, StringIds::dropdown, StringIds::tooltip_select_station_type),
        makeWidget({ 35, 60 }, { 68, 68 }, WidgetType::wt_3, WindowColour::secondary),
        makeWidget({ 112, 104 }, { 24, 24 }, WidgetType::wt_9, WindowColour::secondary, ImageIds::rotate_object, StringIds::rotate_90),
        widgetEnd(),
    };

    WindowEventList events;

    // 0x0049E228
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
    static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
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
                Dropdown::show(xPos, yPos, width, height, self->getColour(WindowColour::secondary), stationCount, (1 << 7));

                if (_byte_1136063 & (1 << 7))
                {
                    AddStationsToDropdown<AirportObject>(stationCount);
                }
                else if (_byte_1136063 & (1 << 6))
                {
                    AddStationsToDropdown<DockObject>(stationCount);
                }
                else if (_trackType & (1 << 7))
                {
                    AddStationsToDropdown<RoadStationObject>(stationCount);
                }
                else
                {
                    AddStationsToDropdown<TrainStationObject>(stationCount);
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
    static void onDropdown(Window* self, WidgetIndex_t widgetIndex, int16_t itemIndex)
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
    static void onUpdate(Window* self)
    {
        Common::onUpdate(self, (1 << 3));
    }

    // 0x0049E421
    static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E421, regs);
    }

    // 0x0049E42C
    static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E42C, regs);
    }

    // 0x0049DD39
    static void prepareDraw(Window* self)
    {
        Common::prepareDraw(self);

        self->widgets[widx::rotate].type = WidgetType::none;

        auto args = FormatArguments();

        if (_byte_1136063 & (1 << 7))
        {
            self->widgets[widx::rotate].type = WidgetType::wt_9;

            auto airportObj = ObjectManager::get<AirportObject>(_lastSelectedStationType);

            self->widgets[widx::station].text = airportObj->name;

            args.push(StringIds::title_airport);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            auto dockObj = ObjectManager::get<DockObject>(_lastSelectedStationType);

            self->widgets[widx::station].text = dockObj->name;

            args.push(StringIds::title_ship_port);
        }
        else if (_trackType & (1 << 7))
        {
            auto trackType = _trackType & ~(1 << 7);

            auto roadObj = ObjectManager::get<RoadObject>(trackType);

            args.push(roadObj->name);

            auto roadStationObject = ObjectManager::get<RoadStationObject>(_lastSelectedStationType);

            self->widgets[widx::station].text = roadStationObject->name;
        }
        else
        {
            auto trackObj = ObjectManager::get<TrackObject>(_trackType);

            args.push(trackObj->name);

            auto trainStationObject = ObjectManager::get<TrainStationObject>(_lastSelectedStationType);

            self->widgets[widx::station].text = trainStationObject->name;
        }

        Common::repositionTabs(self);
    }

    // 0x0049DE40
    static void draw(Window* self, Gfx::Context* context)
    {
        self->draw(context);
        Common::drawTabs(self, context);

        auto company = CompanyManager::get(_playerCompany);
        auto companyColour = company->mainColours.primary;
        int16_t xPos = self->widgets[widx::image].left + self->x;
        int16_t yPos = self->widgets[widx::image].top + self->y;

        if (_byte_1136063 & (1 << 7))
        {
            auto airportObj = ObjectManager::get<AirportObject>(_lastSelectedStationType);
            auto imageId = Gfx::recolour(airportObj->image, companyColour);
            Gfx::drawImage(context, xPos, yPos, imageId);
        }
        else if (_byte_1136063 & (1 << 6))
        {
            auto dockObj = ObjectManager::get<DockObject>(_lastSelectedStationType);
            auto imageId = Gfx::recolour(dockObj->image, companyColour);
            Gfx::drawImage(context, xPos, yPos, imageId);
        }
        else if (_trackType & (1 << 7))
        {
            auto roadStationObj = ObjectManager::get<RoadStationObject>(_lastSelectedStationType);

            auto imageId = Gfx::recolour(roadStationObj->image + RoadStation::ImageIds::preview_image, companyColour);
            Gfx::drawImage(context, xPos, yPos, imageId);

            auto colour = _byte_5045FA[companyColour];
            if (!(roadStationObj->flags & RoadStationFlags::recolourable))
            {
                colour = PaletteIndex::index_2E;
            }

            imageId = Gfx::recolourTranslucent(roadStationObj->image + RoadStation::ImageIds::preview_image_windows, colour);
            Gfx::drawImage(context, xPos, yPos, imageId);
        }
        else
        {
            auto trainStationObj = ObjectManager::get<TrainStationObject>(_lastSelectedStationType);

            auto imageId = Gfx::recolour(trainStationObj->image + TrainStation::ImageIds::preview_image, companyColour);
            Gfx::drawImage(context, xPos, yPos, imageId);

            auto colour = _byte_5045FA[companyColour];
            if (!(trainStationObj->flags & TrainStationFlags::recolourable))
            {
                colour = PaletteIndex::index_2E;
            }

            imageId = Gfx::recolourTranslucent(trainStationObj->image + TrainStation::ImageIds::preview_image_windows, colour);
            Gfx::drawImage(context, xPos, yPos, imageId);
        }

        if (_stationCost != 0x80000000 && _stationCost != 0)
        {
            xPos = self->x + 69;
            yPos = self->widgets[widx::image].bottom + self->y + 4;

            auto args = FormatArguments();
            args.push<uint32_t>(_stationCost);

            Gfx::drawStringCentred(*context, xPos, yPos, Colour::black, StringIds::build_cost, &args);
        }

        xPos = self->x + 3;
        yPos = self->widgets[widx::image].bottom + self->y + 16;
        auto width = self->width - 4;
        Gfx::drawRectInset(context, xPos, yPos, width, 1, self->getColour(WindowColour::secondary), (1 << 5));

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
        Gfx::drawStringCentredClipped(*context, xPos, yPos, width, Colour::black, StringIds::new_station_buffer, &args);

        xPos = self->x + 2;
        yPos = self->widgets[widx::image].bottom + self->y + 29;
        Gfx::point_t origin = { xPos, yPos };

        Gfx::drawString_494B3F(*context, &origin, Colour::black, StringIds::catchment_area_accepts);

        if (_constructingStationAcceptedCargoTypes == 0)
        {
            Gfx::drawString_494B3F(*context, origin.x, origin.y, Colour::black, StringIds::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                if (_constructingStationAcceptedCargoTypes & (1 << i))
                {
                    auto xPosMax = self->x + self->width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(i);

                        Gfx::drawImage(context, origin.x, origin.y, cargoObj->unit_inline_sprite);
                        origin.x += 10;
                    }
                }
            }
        }

        xPos = self->x + 2;
        yPos = self->widgets[widx::image].bottom + self->y + 49;
        origin = { xPos, yPos };

        Gfx::drawString_494B3F(*context, &origin, Colour::black, StringIds::catchment_area_produces);

        if (_constructingStationProducedCargoTypes == 0)
        {
            Gfx::drawString_494B3F(*context, origin.x, origin.y, Colour::black, StringIds::catchment_area_nothing);
        }
        else
        {
            yPos--;
            for (uint8_t i = 0; i < ObjectManager::getMaxObjects(ObjectType::cargo); i++)
            {
                if (_constructingStationProducedCargoTypes & (1 << i))
                {
                    auto xPosMax = self->x + self->width - 12;
                    if (origin.x <= xPosMax)
                    {
                        auto cargoObj = ObjectManager::get<CargoObject>(i);

                        Gfx::drawImage(context, origin.x, origin.y, cargoObj->unit_inline_sprite);
                        origin.x += 10;
                    }
                }
            }
        }
    }

    void tabReset(Window* self)
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
