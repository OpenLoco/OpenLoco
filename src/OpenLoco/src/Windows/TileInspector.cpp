#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Map/BuildingElement.h"
#include "Map/IndustryElement.h"
#include "Map/MapSelection.h"
#include "Map/RoadElement.h"
#include "Map/SignalElement.h"
#include "Map/StationElement.h"
#include "Map/SurfaceElement.h"
#include "Map/TileManager.h"
#include "Map/TrackElement.h"
#include "Map/TreeElement.h"
#include "Map/WallElement.h"
#include "Objects/AirportObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/DockObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainSignalObject.h"
#include "Objects/TrainStationObject.h"
#include "Objects/TreeObject.h"
#include "Objects/WallObject.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/Station.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <map>

using namespace OpenLoco::Interop;
using namespace OpenLoco::World;

namespace OpenLoco::Ui::Windows::TileInspector
{
    static TilePos2 _currentPosition{};

    static constexpr Ui::Size kWindowSize = { 350, 200 };

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
            nameTypeHeader,
            baseHeightHeader,
            clearHeightHeader,
            directionHeader,
            ghostHeader,
            scrollview,
            detailsGroup,
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { kWindowSize.width - 2, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::tile_inspector),
        makeWidget({ kWindowSize.width - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { kWindowSize.width, kWindowSize.height - 15 }, WidgetType::panel, WindowColour::secondary),
        makeStepperWidgets({ 19, 24 }, { 55, 12 }, WidgetType::textbox, WindowColour::secondary),
        makeStepperWidgets({ 92, 24 }, { 55, 12 }, WidgetType::textbox, WindowColour::secondary),
        makeWidget({ kWindowSize.width - 26, 18 }, { 24, 24 }, WidgetType::buttonWithImage, WindowColour::secondary, ImageIds::construction_new_position, StringIds::tile_inspector_select_btn_tooltip),
        makeWidget({ 4, 46 }, { kWindowSize.width - 98, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tileInspectorHeaderNameType, StringIds::tileInspectorHeaderNameTypeTip), // name
        makeWidget({ kWindowSize.width - 109, 46 }, { 30, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tileInspectorHeaderBaseHeight, StringIds::tileInspectorHeaderBaseHeightTip),
        makeWidget({ kWindowSize.width - 79, 46 }, { 30, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tileInspectorHeaderClearHeight, StringIds::tileInspectorHeaderClearHeightTip),
        makeWidget({ kWindowSize.width - 49, 46 }, { 15, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tileInspectorHeaderDirection, StringIds::tileInspectorHeaderDirectionTip),
        makeWidget({ kWindowSize.width - 34, 46 }, { 30, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tileInspectorHeaderGhost, StringIds::tileInspectorHeaderGhostTip),
        makeWidget({ 4, 60 }, { kWindowSize.width - 8, 103 }, WidgetType::scrollview, WindowColour::secondary, Ui::Scrollbars::vertical),
        makeWidget({ 4, 165 }, { kWindowSize.width - 8, 30 }, WidgetType::groupbox, WindowColour::secondary, StringIds::tile_element_data),
        widgetEnd(),
    };

    static WindowEventList _events;

    static loco_global<char[2], 0x005045F8> _strCheckmark;

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
            kWindowSize,
            WindowFlags::none,
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

    static void prepareDraw(Window& self)
    {
        if (Input::isToolActive(WindowType::tileInspector))
            self.activatedWidgets |= (1 << widx::select);
        else
            self.activatedWidgets &= ~(1 << widx::select);
    }

    static void draw(Ui::Window& self, Gfx::RenderTarget* const rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // Draw widgets.
        self.draw(rt);

        // Coord X/Y labels
        {
            auto args = FormatArguments::common(StringIds::tile_inspector_x_coord);
            auto& widget = self.widgets[widx::xPos];
            drawingCtx.drawStringLeft(*rt, self.x + widget.left - 15, self.y + widget.top + 1, Colour::black, StringIds::wcolour2_stringid, &args);
        }
        {
            auto args = FormatArguments::common(StringIds::tile_inspector_y_coord);
            auto& widget = self.widgets[widx::yPos];
            drawingCtx.drawStringLeft(*rt, self.x + widget.left - 15, self.y + widget.top + 1, Colour::black, StringIds::wcolour2_stringid, &args);
        }

        // Coord X/Y values
        {
            FormatArguments args = {};
            args.push<int16_t>(_currentPosition.x);
            auto& widget = self.widgets[widx::xPos];
            drawingCtx.drawStringLeft(*rt, self.x + widget.left + 2, self.y + widget.top + 1, Colour::black, StringIds::tile_inspector_coord, &args);
        }
        {
            FormatArguments args = {};
            args.push<int16_t>(_currentPosition.y);
            auto& widget = self.widgets[widx::yPos];
            drawingCtx.drawStringLeft(*rt, self.x + widget.left + 2, self.y + widget.top + 1, Colour::black, StringIds::tile_inspector_coord, &args);
        }

        // Selected element details
        if (self.var_842 != -1)
        {
            auto tile = TileManager::get(_currentPosition)[self.var_842];
            std::array<uint8_t, 8>& data = tile->rawData();

            char buffer[32] = {};
            buffer[0] = ControlCodes::windowColour2;
            snprintf(&buffer[1], std::size(buffer) - 1, "Data: %02x %02x %02x %02x %02x %02x %02x %02x", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

            auto widget = self.widgets[widx::detailsGroup];
            drawingCtx.drawString(*rt, self.x + widget.left + 7, self.y + widget.top + 14, Colour::black, buffer);
        }
    }

    static StringId getElementTypeName(const TileElement& element)
    {
        static const std::map<ElementType, StringId> typeToString = {
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

    static StringId getObjectName(const TileElement& element)
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
                auto object = ObjectManager::get<IndustryObject>(industry.industry()->objectId);
                return object->name;
            }
        }
        return StringIds::empty;
    }

    static StringId getOwnerName(const TileElement& element)
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

    static void drawScroll(Ui::Window& self, Gfx::RenderTarget& rt, const uint32_t)
    {
        if (_currentPosition == TilePos2(0, 0))
            return;

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto tile = TileManager::get(_currentPosition);
        auto yPos = 0;
        auto rowNum = 0;
        for (auto& element : tile)
        {
            if (yPos + self.rowHeight < rt.y)
            {
                yPos += self.rowHeight;
                continue;
            }
            else if (yPos > rt.y + rt.height)
            {
                break;
            }

            StringId formatString;
            if (self.var_842 == rowNum)
            {
                drawingCtx.fillRect(rt, 0, yPos, self.width, yPos + self.rowHeight, PaletteIndex::index_0A, Drawing::RectFlags::none);
                formatString = StringIds::white_stringid;
            }
            else if (self.rowHover == rowNum)
            {
                drawingCtx.fillRect(rt, 0, yPos, self.width, yPos + self.rowHeight, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
                formatString = StringIds::wcolour2_stringid;
            }
            else
            {
                formatString = StringIds::wcolour2_stringid;
            }

            FormatArguments args = {};

            StringId elementName = getElementTypeName(element);
            StringId objectName = getObjectName(element);
            StringId ownerName = getOwnerName(element);

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

            // Draw name and type
            auto* widget = &self.widgets[widx::nameTypeHeader];
            drawingCtx.drawStringLeftClipped(rt, 0, yPos, widget->width(), Colour::black, formatString, &args);

            // Draw base height
            widget = &self.widgets[widx::baseHeightHeader];
            args.rewind();
            args.push(StringIds::uint16_raw);
            args.push<uint16_t>(element.baseZ());
            drawingCtx.drawStringLeftClipped(rt, widget->left - 4, yPos, widget->width(), Colour::black, formatString, &args);

            // Draw clear height
            widget = &self.widgets[widx::clearHeightHeader];
            args.rewind();
            args.push(StringIds::uint16_raw);
            args.push<uint16_t>(element.clearZ());
            drawingCtx.drawStringLeftClipped(rt, widget->left - 4, yPos, widget->width(), Colour::black, formatString, &args);

            // Draw direction
            widget = &self.widgets[widx::directionHeader];
            args.rewind();
            args.push(StringIds::uint16_raw);
            args.push<uint16_t>(element.data()[0] & 0x03);
            drawingCtx.drawStringLeftClipped(rt, widget->left - 4, yPos, widget->width(), Colour::black, formatString, &args);

            // Draw ghost flag
            widget = &self.widgets[widx::ghostHeader];
            if (element.isGhost())
            {
                drawingCtx.drawString(rt, widget->left - 4, yPos, Colour::white, _strCheckmark);
            }

            rowNum++;
            yPos += self.rowHeight;
        }
    }

    static void scrollMouseDown(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
    {
        auto index = y / self.rowHeight;
        if (index >= self.rowCount)
            return;

        if (self.var_842 != index)
        {
            self.var_842 = index;
            self.invalidate();
            return;
        }
    }

    static void scrollMouseOver(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
    {
        auto index = y / self.rowHeight;
        if (index >= self.rowCount)
            return;

        if (self.rowHover != index)
        {
            self.rowHover = index;
            self.invalidate();
        }
    }

    static void onMouseUp(Ui::Window& self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(self.type);
                break;

            case widx::select:
                activateMapSelectionTool(&self);
                break;

            case widx::xPosDecrease:
                _currentPosition.x = std::clamp<coord_t>(_currentPosition.x - 1, 1, World::kMapColumns);
                self.invalidate();
                break;

            case widx::xPosIncrease:
                _currentPosition.x = std::clamp<coord_t>(_currentPosition.x + 1, 1, World::kMapColumns);
                self.invalidate();
                break;

            case widx::yPosDecrease:
                _currentPosition.y = std::clamp<coord_t>(_currentPosition.y - 1, 1, World::kMapRows);
                self.invalidate();
                break;

            case widx::yPosIncrease:
                _currentPosition.y = std::clamp<coord_t>(_currentPosition.y + 1, 1, World::kMapRows);
                self.invalidate();
                break;
        }
    }

    static void getScrollSize(Ui::Window& self, uint32_t, uint16_t*, uint16_t* const scrollHeight)
    {
        if (_currentPosition == TilePos2(0, 0))
        {
            *scrollHeight = 0;
            return;
        }

        *scrollHeight = self.rowCount * self.rowHeight;
    }

    static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::panel)
            return;

        World::mapInvalidateSelectionRect();
        Input::resetMapSelectionFlag(Input::MapSelectionFlags::enable);
        auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
        if (res)
        {
            World::setMapSelectionSingleTile(res->first);
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
        _currentPosition = World::toTileSpace(res->first);
        auto tile = TileManager::get(_currentPosition);

        self.rowCount = static_cast<uint16_t>(tile.size());
        self.rowHover = -1;
        self.var_842 = 0;
        self.invalidate();
    }

    static void onClose([[maybe_unused]] Window& self)
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
