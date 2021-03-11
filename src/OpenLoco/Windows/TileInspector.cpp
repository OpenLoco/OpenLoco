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
#include <map>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;

namespace OpenLoco::Ui::Windows::TileInspector
{
    static loco_global<int8_t, 0x00523393> _currentTool;
    static loco_global<uint16_t, 0x00F24484> _mapSelectionFlags;

    static map_pos _currentPosition{};

    constexpr Gfx::ui_size_t windowSize = { 400, 260 };

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            scrollview,
            select,
        };
    }

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, windowSize, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { windowSize.width - 2, 13 }, widget_type::caption_25, 0, StringIds::tile_inspector),
        makeWidget({ windowSize.width - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { windowSize.width, 245 }, widget_type::panel, 1),
        makeWidget({ 4, 18 }, { windowSize.width - 8, 289 }, widget_type::scrollview, 1, Ui::scrollbars::vertical),
        makeWidget({ 100, 234 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::construction_new_position),
        widgetEnd(),
    };

    static window_event_list _events;

    static void initEvents();

    static void activateMapSelectionTool(window* const self)
    {
        Input::toolSet(self, widx::panel, 42);
        Input::setFlag(Input::input_flags::flag6);
    }

    window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::tileInspector);
        if (window != nullptr)
            return window;

        initEvents();

        window = WindowManager::createWindowCentred(
            WindowType::tileInspector,
            windowSize,
            0,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::close) | (1 << widx::select);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0C;

        activateMapSelectionTool(window);

        return window;
    }

    static void draw(Ui::window* const self, Gfx::drawpixelinfo_t* const dpi)
    {
        // Draw widgets.
        self->draw(dpi);
    }

    static string_id getElementTypeName(const tile_element& element)
    {
        static const std::map<element_type, string_id> typeToString = {
            { element_type::surface, StringIds::tile_inspector_element_type_surface },
            { element_type::track, StringIds::tile_inspector_element_type_track },
            { element_type::station, StringIds::tile_inspector_element_type_station },
            { element_type::signal, StringIds::tile_inspector_element_type_signal },
            { element_type::building, StringIds::tile_inspector_element_type_building },
            { element_type::tree, StringIds::tile_inspector_element_type_tree },
            { element_type::wall, StringIds::tile_inspector_element_type_wall },
            { element_type::road, StringIds::tile_inspector_element_type_road },
            { element_type::industry, StringIds::tile_inspector_element_type_industry },
        };

        return typeToString.at(element.type());
    }

    static string_id getObjectName(const tile_element& element)
    {
        switch (element.type())
        {
            case element_type::surface:
            {
                auto surface = element.asSurface();
                if (surface != nullptr)
                {
                    auto terrainId = surface->terrain();
                    auto object = ObjectManager::get<LandObject>(terrainId);
                    return object->name;
                }
                break;
            }
            case element_type::track:
            {
                auto track = element.asTrack();
                if (track != nullptr)
                {
                    auto objectId = track->trackObjectId();
                    auto object = ObjectManager::get<TrackObject>(objectId);
                    return object->name;
                }
                break;
            }
            case element_type::station:
            {
                auto station = element.asStation();
                if (station != nullptr)
                {
                    auto objectId = station->objectId();
                    auto stationType = station->stationType();
                    switch (stationType)
                    {
                        case stationType::trainStation:
                            return ObjectManager::get<TrainStationObject>(objectId)->name;
                        case stationType::roadStation:
                            return ObjectManager::get<RoadStationObject>(objectId)->name;
                        case stationType::airport:
                            return ObjectManager::get<AirportObject>(objectId)->name;
                        case stationType::docks:
                            return ObjectManager::get<DockObject>(objectId)->name;
                    }
                }
                break;
            }
            case element_type::signal:
            {
                auto signal = element.asSignal();
                if (signal != nullptr)
                {
                    TrainSignalObject* object = nullptr;
                    if (signal->hasLeftSignal())
                        object = ObjectManager::get<TrainSignalObject>(signal->leftSignalObjectId());
                    else if (signal->hasRightSignal())
                        object = ObjectManager::get<TrainSignalObject>(signal->rightSignalObjectId());

                    if (object != nullptr)
                        return object->name;
                }
                break;
            }
            case element_type::building:
            {
                auto building = element.asBuilding();
                if (building != nullptr)
                {
                    auto objectId = building->objectId();
                    auto object = ObjectManager::get<BuildingObject>(objectId);
                    return object->name;
                }
                break;
            }
            case element_type::tree:
            {
                auto tree = element.asTree();
                if (tree != nullptr)
                {
                    auto objectId = tree->treeObjectId();
                    auto object = ObjectManager::get<TreeObject>(objectId);
                    return object->name;
                }
                break;
            }
            case element_type::wall:
            {
                auto wall = element.asWall();
                if (wall != nullptr)
                {
                    auto objectId = wall->wallObjectId();
                    auto object = ObjectManager::get<WallObject>(objectId);
                    return object->name;
                }
                break;
            }
            case element_type::road:
            {
                auto road = element.asRoad();
                if (road != nullptr)
                {
                    auto objectId = road->roadObjectId();
                    auto object = ObjectManager::get<RoadObject>(objectId);
                    return object->name;
                }
                break;
            }
            case element_type::industry:
            {
                auto industry = element.asIndustry();
                if (industry != nullptr)
                {
                    auto object = ObjectManager::get<IndustryObject>(industry->industry()->object_id);
                    return object->name;
                }
            }
        }
        return StringIds::empty;
    }

    static string_id getOwnerName(const tile_element& element)
    {
        if (element.type() == element_type::road)
        {
            auto road = element.asRoad();
            if (road != nullptr)
            {
                auto ownerId = road->owner();
                if (ownerId != CompanyId::neutral)
                {
                    auto company = CompanyManager::get(ownerId);
                    return company->name;
                }
            }
        }
        else if (element.type() == element_type::track)
        {
            auto track = element.asTrack();
            if (track != nullptr)
            {
                auto ownerId = track->owner();
                if (ownerId != CompanyId::neutral)
                {
                    auto company = CompanyManager::get(ownerId);
                    return company->name;
                }
            }
        }
        return StringIds::empty;
    }

    static void drawScroll(Ui::window*, Gfx::drawpixelinfo_t* const context, uint32_t)
    {
        if (_currentPosition == map_pos(0, 0))
            return;

        auto tile = TileManager::get(_currentPosition);
        auto yPos = 0;
        for (auto& element : tile)
        {
            FormatArguments args = {};

            string_id elementName = getElementTypeName(element);
            string_id objectName = getObjectName(element);
            string_id ownerName = getOwnerName(element);

            if (ownerName != StringIds::empty)
            {
                args.push(StringIds::competitor_vehicle);
                args.push(objectName);
                args.push(ownerName);
                args.push(elementName);
            }
            else
            {
                args.push(objectName);
                args.push(elementName);
            }

            Gfx::drawString_494B3F(*context, 0, yPos, Colour::black, StringIds::wcolour2_stringid_stringid, &args);
            yPos += 10;
        }
    }

    static void onMouseUp(Ui::window* const self, const widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(self->type);
                break;

            case widx::select:
                activateMapSelectionTool(self);
                break;
        }
    }

    static void getScrollSize(Ui::window*, uint32_t, uint16_t*, uint16_t* const scrollHeight)
    {
        if (_currentPosition == map_pos(0, 0))
        {
            *scrollHeight = 0;
            return;
        }

        auto tile = TileManager::get(_currentPosition);
        *scrollHeight = static_cast<uint16_t>(tile.size()) * 10;
    }

    static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::panel)
            return;

        TileManager::mapInvalidateSelectionRect();
        *_mapSelectionFlags &= ~(1 << 0);
        TileManager::setMapSelectionSingleTile(x, y);
    }

    static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        if (widgetIndex != widx::panel || !(_mapSelectionFlags & 1))
            return;

        _currentPosition = TileManager::screenPosToMapPos(x, y);
        WindowManager::invalidateWidget(self.type, self.number, widx::scrollview);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
        _events.get_scroll_size = getScrollSize;
        _events.on_mouse_up = onMouseUp;
        _events.on_tool_update = onToolUpdate;
        _events.on_tool_down = onToolDown;
    }
}
