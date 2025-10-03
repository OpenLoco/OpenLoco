#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
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
#include "Ui/ToolManager.h"
#include "Ui/ViewportInteraction.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/GroupBoxWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/StepperWidget.h"
#include "Ui/Widgets/TableHeaderWidget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include "World/Industry.h"
#include "World/Station.h"
#include <map>

using namespace OpenLoco::World;

namespace OpenLoco::Ui::Windows::TileInspector
{
    static TilePos2 _currentPosition{};

    static constexpr Ui::Size32 kWindowSize = { 350, 200 };

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

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::tile_inspector),
        Widgets::ImageButton({ kWindowSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { kWindowSize.width, kWindowSize.height - 15 }, WindowColour::secondary),
        Widgets::stepperWidgets({ 19, 24 }, { 55, 12 }, WindowColour::secondary),
        Widgets::stepperWidgets({ 92, 24 }, { 55, 12 }, WindowColour::secondary),
        Widgets::ImageButton({ kWindowSize.width - 26, 18 }, { 24, 24 }, WindowColour::secondary, ImageIds::construction_new_position, StringIds::tile_inspector_select_btn_tooltip),
        Widgets::TableHeader({ 4, 46 }, { kWindowSize.width - 98, 12 }, WindowColour::secondary, StringIds::tileInspectorHeaderNameType, StringIds::tileInspectorHeaderNameTypeTip), // name
        Widgets::TableHeader({ kWindowSize.width - 109, 46 }, { 30, 12 }, WindowColour::secondary, StringIds::tileInspectorHeaderBaseHeight, StringIds::tileInspectorHeaderBaseHeightTip),
        Widgets::TableHeader({ kWindowSize.width - 79, 46 }, { 30, 12 }, WindowColour::secondary, StringIds::tileInspectorHeaderClearHeight, StringIds::tileInspectorHeaderClearHeightTip),
        Widgets::TableHeader({ kWindowSize.width - 49, 46 }, { 15, 12 }, WindowColour::secondary, StringIds::tileInspectorHeaderDirection, StringIds::tileInspectorHeaderDirectionTip),
        Widgets::TableHeader({ kWindowSize.width - 34, 46 }, { 30, 12 }, WindowColour::secondary, StringIds::tileInspectorHeaderGhost, StringIds::tileInspectorHeaderGhostTip),
        Widgets::ScrollView({ 4, 60 }, { kWindowSize.width - 8, 103 }, WindowColour::secondary, Ui::Scrollbars::vertical),
        Widgets::GroupBox({ 4, 165 }, { kWindowSize.width - 8, 30 }, WindowColour::secondary, StringIds::tile_element_data)

    );

    static void activateMapSelectionTool(const Window& self)
    {
        ToolManager::toolSet(self, widx::panel, CursorId::crosshair);
        Input::setFlag(Input::Flags::flag6);
    }

    static const WindowEventList& getEvents();

    Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::tileInspector);
        if (window != nullptr)
        {
            return window;
        }

        window = WindowManager::createWindow(
            WindowType::tileInspector,
            kWindowSize,
            WindowFlags::none,
            getEvents());

        window->setWidgets(_widgets);
        window->rowCount = 0;
        window->rowHeight = 10;
        window->selectedTileIndex = -1;
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->windowTitlebarColour);
        window->setColour(WindowColour::secondary, skin->windowColour);

        activateMapSelectionTool(*window);

        return window;
    }

    static void prepareDraw(Window& self)
    {
        if (ToolManager::isToolActive(WindowType::tileInspector))
        {
            self.activatedWidgets |= (1 << widx::select);
        }
        else
        {
            self.activatedWidgets &= ~(1 << widx::select);
        }
    }

    static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw widgets.
        self.draw(drawingCtx);
        // Coord X/Y labels
        {
            FormatArguments args{};
            args.push(StringIds::tile_inspector_x_coord);
            auto& widget = self.widgets[widx::xPos];
            auto point = Point(widget.left - 15, widget.top + 1);
            tr.drawStringLeft(point, Colour::black, StringIds::wcolour2_stringid, args);
        }
        {
            FormatArguments args{};
            args.push(StringIds::tile_inspector_y_coord);
            auto& widget = self.widgets[widx::yPos];
            auto point = Point(widget.left - 15, widget.top + 1);
            tr.drawStringLeft(point, Colour::black, StringIds::wcolour2_stringid, args);
        }

        // Coord X/Y values
        {
            FormatArguments args{};
            args.push<int16_t>(_currentPosition.x);
            auto& widget = self.widgets[widx::xPos];
            auto point = Point(widget.left + 2, widget.top + 1);
            tr.drawStringLeft(point, Colour::black, StringIds::tile_inspector_coord, args);
        }
        {
            FormatArguments args{};
            args.push<int16_t>(_currentPosition.y);
            auto& widget = self.widgets[widx::yPos];
            auto point = Point(widget.left + 2, widget.top + 1);
            tr.drawStringLeft(point, Colour::black, StringIds::tile_inspector_coord, args);
        }

        // Selected element details
        if (self.selectedTileIndex != -1)
        {
            auto tile = TileManager::get(_currentPosition)[self.selectedTileIndex];
            const auto data = tile->rawData();

            char buffer[32]{};
            buffer[0] = ControlCodes::windowColour2;
            snprintf(&buffer[1], std::size(buffer) - 1, "Data: %02x %02x %02x %02x %02x %02x %02x %02x", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

            auto widget = self.widgets[widx::detailsGroup];
            auto point = Point(widget.left + 7, widget.top + 14);
            tr.drawString(point, Colour::black, buffer);
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

                const TrainSignalObject* object = nullptr;
                if (signal.getLeft().hasSignal())
                {
                    object = ObjectManager::get<TrainSignalObject>(signal.getLeft().signalObjectId());
                }
                else if (signal.getRight().hasSignal())
                {
                    object = ObjectManager::get<TrainSignalObject>(signal.getRight().signalObjectId());
                }

                if (object != nullptr)
                {
                    return object->name;
                }

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

    static void drawScroll(Ui::Window& self, Gfx::DrawingContext& drawingCtx, const uint32_t)
    {
        if (_currentPosition == TilePos2(0, 0))
        {
            return;
        }

        const auto& rt = drawingCtx.currentRenderTarget();
        auto tr = Gfx::TextRenderer(drawingCtx);

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
            if (self.selectedTileIndex == rowNum)
            {
                drawingCtx.fillRect(0, yPos, self.width, yPos + self.rowHeight, PaletteIndex::black0, Gfx::RectFlags::none);
                formatString = StringIds::white_stringid;
            }
            else if (self.rowHover == rowNum)
            {
                drawingCtx.fillRect(0, yPos, self.width, yPos + self.rowHeight, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
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
            auto point = Point(0, yPos);
            tr.drawStringLeftClipped(point, widget->width(), Colour::black, formatString, args);

            // Draw base height
            widget = &self.widgets[widx::baseHeightHeader];
            args.rewind();
            args.push(StringIds::uint16_raw);
            args.push<uint16_t>(element.baseZ());
            point = Point(widget->left - 4, yPos);
            tr.drawStringLeftClipped(point, widget->width(), Colour::black, formatString, args);

            // Draw clear height
            widget = &self.widgets[widx::clearHeightHeader];
            args.rewind();
            args.push(StringIds::uint16_raw);
            args.push<uint16_t>(element.clearZ());
            point = Point(widget->left - 4, yPos);
            tr.drawStringLeftClipped(point, widget->width(), Colour::black, formatString, args);

            // Draw direction
            widget = &self.widgets[widx::directionHeader];
            args.rewind();
            args.push(StringIds::uint16_raw);
            args.push<uint16_t>(element.data()[0] & 0x03);
            point = Point(widget->left - 4, yPos);
            tr.drawStringLeftClipped(point, widget->width(), Colour::black, formatString, args);

            // Draw ghost flag
            widget = &self.widgets[widx::ghostHeader];
            if (element.isGhost())
            {
                static constexpr char strCheckmark[] = "\xAC";
                point = Point(widget->left - 4, yPos);
                tr.drawString(point, Colour::white, strCheckmark);
            }

            rowNum++;
            yPos += self.rowHeight;
        }
    }

    static void scrollMouseDown(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
    {
        auto index = y / self.rowHeight;
        if (index >= self.rowCount)
        {
            return;
        }

        if (self.selectedTileIndex != index)
        {
            self.selectedTileIndex = index;
            self.invalidate();
            return;
        }
    }

    static void scrollMouseOver(Window& self, [[maybe_unused]] const int16_t x, const int16_t y, [[maybe_unused]] const uint8_t scrollIndex)
    {
        auto index = y / self.rowHeight;
        if (index >= self.rowCount)
        {
            return;
        }

        if (self.rowHover != index)
        {
            self.rowHover = index;
            self.invalidate();
        }
    }

    static void onMouseUp(Ui::Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(self.type);
                break;

            case widx::select:
                activateMapSelectionTool(self);
                break;
        }
    }

    static void onMouseDown(Ui::Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::xPosDecrease:
                _currentPosition.x = std::clamp<coord_t>(_currentPosition.x - 1, 1, World::TileManager::getMapColumns());
                self.invalidate();
                break;

            case widx::xPosIncrease:
                _currentPosition.x = std::clamp<coord_t>(_currentPosition.x + 1, 1, World::TileManager::getMapColumns());
                self.invalidate();
                break;

            case widx::yPosDecrease:
                _currentPosition.y = std::clamp<coord_t>(_currentPosition.y - 1, 1, World::TileManager::getMapRows());
                self.invalidate();
                break;

            case widx::yPosIncrease:
                _currentPosition.y = std::clamp<coord_t>(_currentPosition.y + 1, 1, World::TileManager::getMapRows());
                self.invalidate();
                break;
        }
    }

    static void getScrollSize(Ui::Window& self, uint32_t, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
    {
        if (_currentPosition == TilePos2(0, 0))
        {
            scrollHeight = 0;
            return;
        }

        scrollHeight = self.rowCount * self.rowHeight;
    }

    static void onToolUpdate([[maybe_unused]] Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::panel)
        {
            return;
        }

        World::mapInvalidateSelectionRect();
        World::resetMapSelectionFlag(World::MapSelectionFlags::enable);
        auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
        if (res)
        {
            World::setMapSelectionSingleTile(res->first);
        }
    }

    static void onToolDown(Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::panel || !World::hasMapSelectionFlag(World::MapSelectionFlags::enable))
        {
            return;
        }

        auto res = Ui::ViewportInteraction::getSurfaceLocFromUi({ x, y });
        if (!res)
        {
            return;
        }
        _currentPosition = World::toTileSpace(res->first);
        auto tile = TileManager::get(_currentPosition);

        self.rowCount = static_cast<uint16_t>(tile.size());
        self.rowHover = -1;
        self.selectedTileIndex = 0;
        self.invalidate();
    }

    static void onClose([[maybe_unused]] Window& self)
    {
        ToolManager::toolCancel();
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .onMouseDown = onMouseDown,
        .onToolUpdate = onToolUpdate,
        .onToolDown = onToolDown,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = scrollMouseDown,
        .scrollMouseOver = scrollMouseOver,
        .prepareDraw = prepareDraw,
        .draw = draw,
        .drawScroll = drawScroll,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
