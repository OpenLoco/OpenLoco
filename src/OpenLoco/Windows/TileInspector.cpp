#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Industry.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/TileManager.h"
#include "../Objects/AirportObject.h"
#include "../Objects/BuildingObject.h"
#include "../Objects/DockObject.h"
#include "../Objects/IndustryObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadObject.h"
#include "../Objects/RoadStationObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/TrainSignalObject.h"
#include "../Objects/TrainStationObject.h"
#include "../Objects/TreeObject.h"
#include "../Objects/WallObject.h"
#include "../Station.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"
#include <map>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::Ui::Windows::TileInspector
{
    static TilePos2 _currentPosition{};

    constexpr Ui::Size windowSize = { 250, 182 };

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            xPos,
            xPosDecrease,
            xPosIncrease,
            yPos,
            yPosDecrease,
            yPosIncrease,
            select,
            scrollview,
            detailsGroup,
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, windowSize, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { windowSize.width - 2, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::tile_inspector),
        makeWidget({ windowSize.width - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { windowSize.width, windowSize.height - 15 }, WidgetType::panel, WindowColour::secondary),
        makeStepperWidgets({ 19, 24 }, { 55, 12 }, WidgetType::textbox, WindowColour::secondary),
        makeStepperWidgets({ 92, 24 }, { 55, 12 }, WidgetType::textbox, WindowColour::secondary),
        makeWidget({ windowSize.width - 26, 18 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_new_position, StringIds::tile_inspector_select_btn_tooltip),
        makeWidget({ 4, 46 }, { windowSize.width - 8, 100 }, WidgetType::scrollview, WindowColour::secondary, Ui::Scrollbars::vertical),
        makeWidget({ 4, 148 }, { windowSize.width - 8, 30 }, WidgetType::groupbox, WindowColour::secondary, StringIds::tile_element_data),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void initEvents();

    static void activateMapSelectionTool(Window* const self)
    {
        Input::toolSet(self, widx::panel, CursorId::crosshair);
        Input::setFlag(Input::Flags::flag6);
    }

    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::tileInspector);
        if (window != nullptr)
            return window;

        initEvents();

        window = WindowManager::createWindow(
            WindowType::tileInspector,
            windowSize,
            0,
            &_events);

        window->widgets = _widgets;
        window->enabledWidgets = (1 << widx::close) | (1 << widx::select) | (1 << widx::xPosDecrease) | (1 << widx::xPosIncrease) | (1 << widx::yPosDecrease) | (1 << widx::yPosIncrease);
        window->rowCount = 0;
        window->rowHeight = 10;
        window->var_842 = -1;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_0C);

        activateMapSelectionTool(window);

        return window;
    }

    static void prepareDraw(Window* self)
    {
        if (Input::isToolActive(WindowType::tileInspector))
            self->activatedWidgets |= (1 << widx::select);
        else
            self->activatedWidgets &= ~(1 << widx::select);
    }

    static void draw(Ui::Window* const self, Gfx::Context* const context)
    {
        // Draw widgets.
        self->draw(context);

        // Coord X/Y labels
        {
            auto args = FormatArguments::common(StringIds::tile_inspector_x_coord);
            auto& widget = self->widgets[widx::xPos];
            Gfx::drawString_494B3F(*context, self->x + widget.left - 15, self->y + widget.top + 1, Colour::black, StringIds::wcolour2_stringid, &args);
        }
        {
            auto args = FormatArguments::common(StringIds::tile_inspector_y_coord);
            auto& widget = self->widgets[widx::yPos];
            Gfx::drawString_494B3F(*context, self->x + widget.left - 15, self->y + widget.top + 1, Colour::black, StringIds::wcolour2_stringid, &args);
        }

        // Coord X/Y values
        {
            FormatArguments args = {};
            args.push<int16_t>(_currentPosition.x);
            auto& widget = self->widgets[widx::xPos];
            Gfx::drawString_494B3F(*context, self->x + widget.left + 2, self->y + widget.top + 1, Colour::black, StringIds::tile_inspector_coord, &args);
        }
        {
            FormatArguments args = {};
            args.push<int16_t>(_currentPosition.y);
            auto& widget = self->widgets[widx::yPos];
            Gfx::drawString_494B3F(*context, self->x + widget.left + 2, self->y + widget.top + 1, Colour::black, StringIds::tile_inspector_coord, &args);
        }

        // Selected element details
        if (self->var_842 != -1)
        {
            auto tile = TileManager::get(_currentPosition)[self->var_842];
            std::array<uint8_t, 8>& data = tile->rawData();

            char buffer[32] = {};
            buffer[0] = ControlCodes::window_colour_2;
            snprintf(&buffer[1], std::size(buffer) - 1, "Data: %02x %02x %02x %02x %02x %02x %02x %02x", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

            auto widget = self->widgets[widx::detailsGroup];
            Gfx::drawString(*context, self->x + widget.left + 7, self->y + widget.top + 14, Colour::black, buffer);
        }
    }

    static string_id getElementTypeName(const TileElement& element)
    {
        static const std::map<ElementType, string_id> typeToString = {
            { ElementType::surface, StringIds::tile_inspector_element_type_surface },
            { ElementType::track, StringIds::tile_inspector_element_type_track },
            { ElementType::station, StringIds::tile_inspector_element_type_station },
            { ElementType::signal, StringIds::tile_inspector_element_type_signal },
            { ElementType::building, StringIds::tile_inspector_element_type_building },
            { ElementType::tree, StringIds::tile_inspector_element_type_tree },
            { ElementType::wall, StringIds::tile_inspector_element_type_wall },
            { ElementType::road, StringIds::tile_inspector_element_type_road },
            { ElementType::industry, StringIds::tile_inspector_element_type_industry },
        };

        return typeToString.at(element.type());
    }

    static string_id getObjectName(const TileElement& element)
    {
        switch (element.type())
        {
            case ElementType::surface:
            {
                auto& surface = element.get<SurfaceElement>();
                auto terrainId = surface.terrain();
                auto object = ObjectManager::get<LandObject>(terrainId);
                return object->name;
            }
            case ElementType::track:
            {
                auto& track = element.get<TrackElement>();
                auto objectId = track.trackObjectId();
                auto object = ObjectManager::get<TrackObject>(objectId);
                return object->name;
            }
            case ElementType::station:
            {
                auto& station = element.get<StationElement>();
                auto objectId = station.objectId();
                auto stationType = station.stationType();
                switch (stationType)
                {
                    case StationType::trainStation:
                        return ObjectManager::get<TrainStationObject>(objectId)->name;
                    case StationType::roadStation:
                        return ObjectManager::get<RoadStationObject>(objectId)->name;
                    case StationType::airport:
                        return ObjectManager::get<AirportObject>(objectId)->name;
                    case StationType::docks:
                        return ObjectManager::get<DockObject>(objectId)->name;
                }
                break;
            }
            case ElementType::signal:
            {
                auto& signal = element.get<SignalElement>();

                TrainSignalObject* object = nullptr;
                if (signal.getLeft().hasSignal())
                    object = ObjectManager::get<TrainSignalObject>(signal.getLeft().signalObjectId());
                else if (signal.getRight().hasSignal())
                    object = ObjectManager::get<TrainSignalObject>(signal.getRight().signalObjectId());

                if (object != nullptr)
                    return object->name;

                break;
            }
            case ElementType::building:
            {
                auto& building = element.get<BuildingElement>();
                auto objectId = building.objectId();
                auto object = ObjectManager::get<BuildingObject>(objectId);
                return object->name;
            }
            case ElementType::tree:
            {
                auto& tree = element.get<TreeElement>();
                auto objectId = tree.treeObjectId();
                auto object = ObjectManager::get<TreeObject>(objectId);
                return object->name;
            }
            case ElementType::wall:
            {
                auto wall = element.get<WallElement>();
                auto objectId = wall.wallObjectId();
                auto object = ObjectManager::get<WallObject>(objectId);
                return object->name;
            }
            case ElementType::road:
            {
                auto& road = element.get<RoadElement>();
                auto objectId = road.roadObjectId();
                auto object = ObjectManager::get<RoadObject>(objectId);
                return object->name;
            }
            case ElementType::industry:
            {
                auto& industry = element.get<IndustryElement>();
                auto object = ObjectManager::get<IndustryObject>(industry.industry()->object_id);
                return object->name;
            }
        }
        return StringIds::empty;
    }

    static string_id getOwnerName(const TileElement& element)
    {
        if (element.type() == ElementType::road)
        {
            auto& road = element.get<RoadElement>();
            auto ownerId = road.owner();
            if (ownerId != CompanyId::neutral)
            {
                auto company = CompanyManager::get(ownerId);
                return company->name;
            }
        }
        else if (element.type() == ElementType::track)
        {
            auto& track = element.get<TrackElement>();
            auto ownerId = track.owner();
            if (ownerId != CompanyId::neutral)
            {
                auto company = CompanyManager::get(ownerId);
                return company->name;
            }
        }
        return StringIds::empty;
    }

    static void drawScroll(Ui::Window& self, Gfx::Context& context, const uint32_t)
    {
        if (_currentPosition == TilePos2(0, 0))
            return;

        auto tile = TileManager::get(_currentPosition);
        auto yPos = 0;
        auto rowNum = 0;
        for (auto& element : tile)
        {
            string_id formatString;
            if (self.var_842 == rowNum)
            {
                Gfx::fillRect(context, 0, yPos, self.width, yPos + self.rowHeight, enumValue(Colour::darkGreen));
                formatString = StringIds::white_stringid;
            }
            else if (self.rowHover == rowNum)
            {
                Gfx::fillRect(context, 0, yPos, self.width, yPos + self.rowHeight, 0x2000030);
                formatString = StringIds::wcolour2_stringid;
            }
            else
            {
                formatString = StringIds::wcolour2_stringid;
            }

            FormatArguments args = {};

            string_id elementName = getElementTypeName(element);
            string_id objectName = getObjectName(element);
            string_id ownerName = getOwnerName(element);

            if (ownerName != StringIds::empty)
            {
                args.push(StringIds::tile_inspector_entry_three_pos);
                args.push(objectName);
                args.push(ownerName);
                args.push(StringIds::empty);
                args.push(elementName);
            }
            else
            {
                args.push(StringIds::tile_inspector_entry_two_pos);
                args.push(objectName);
                args.push(elementName);
            }

            Gfx::drawString_494B3F(context, 0, yPos, Colour::black, formatString, &args);
            rowNum++;
            yPos += self.rowHeight;
        }
    }

    static void scrollMouseDown(Window* const self, const int16_t x, const int16_t y, const uint8_t scrollIndex)
    {
        auto index = y / self->rowHeight;
        if (index >= self->rowCount)
            return;

        if (self->var_842 != index)
        {
            self->var_842 = index;
            self->invalidate();
            return;
        }
    }

    static void scrollMouseOver(Window* const self, const int16_t x, const int16_t y, const uint8_t scrollIndex)
    {
        auto index = y / self->rowHeight;
        if (index >= self->rowCount)
            return;

        if (self->rowHover != index)
        {
            self->rowHover = index;
            self->invalidate();
        }
    }

    static void onMouseUp(Ui::Window* const self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(self->type);
                break;

            case widx::select:
                activateMapSelectionTool(self);
                break;

            case widx::xPosDecrease:
                _currentPosition.x = std::clamp<coord_t>(_currentPosition.x - 1, 1, Map::map_columns);
                self->invalidate();
                break;

            case widx::xPosIncrease:
                _currentPosition.x = std::clamp<coord_t>(_currentPosition.x + 1, 1, Map::map_columns);
                self->invalidate();
                break;

            case widx::yPosDecrease:
                _currentPosition.y = std::clamp<coord_t>(_currentPosition.y - 1, 1, Map::map_rows);
                self->invalidate();
                break;

            case widx::yPosIncrease:
                _currentPosition.y = std::clamp<coord_t>(_currentPosition.y + 1, 1, Map::map_rows);
                self->invalidate();
                break;
        }
    }

    static void getScrollSize(Ui::Window* self, uint32_t, uint16_t*, uint16_t* const scrollHeight)
    {
        if (_currentPosition == TilePos2(0, 0))
        {
            *scrollHeight = 0;
            return;
        }

        *scrollHeight = self->rowCount * self->rowHeight;
    }

    static void onToolUpdate(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::panel)
            return;

        TileManager::mapInvalidateSelectionRect();
        Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
        auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
        if (res)
        {
            TileManager::setMapSelectionSingleTile(res->first);
        }
    }

    static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::panel || !Input::hasMapSelectionFlag(Input::MapSelectionFlags::enable))
            return;

        auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
        if (!res)
        {
            return;
        }
        _currentPosition = res->first;
        auto tile = TileManager::get(_currentPosition);

        self.rowCount = static_cast<uint16_t>(tile.size());
        self.rowHover = -1;
        self.var_842 = 0;
        self.invalidate();
    }

    static void onClose(Window* self)
    {
        Input::toolCancel();
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.drawScroll = drawScroll;
        _events.getScrollSize = getScrollSize;
        _events.onClose = onClose;
        _events.onMouseUp = onMouseUp;
        _events.onToolUpdate = onToolUpdate;
        _events.onToolDown = onToolDown;
        _events.prepareDraw = prepareDraw;
        _events.scrollMouseDown = scrollMouseDown;
        _events.scrollMouseOver = scrollMouseOver;
    }
}
