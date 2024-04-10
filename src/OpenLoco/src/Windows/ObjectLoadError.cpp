#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include <fmt/format.h>

namespace OpenLoco::Ui::Windows::ObjectLoadError
{
    static constexpr Ui::Size kWindowSize = { 360, 238 };

    static constexpr uint8_t kRowHeight = 12; // CJK: 15

    static std::vector<ObjectHeader> _loadErrorObjectsList;

    enum Widx
    {
        frame,
        title,
        close,
        panel,
        nameHeader,
        typeHeader,
        checksumHeader,
        scrollview,
    };

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 360, 238 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 358, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::objectErrorWindowTitle),
        makeWidget({ 345, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 360, 223 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 4, 43 }, { 100, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tableHeaderObjectId),
        makeWidget({ 104, 43 }, { 152, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tableHeaderObjectType),
        makeWidget({ 256, 43 }, { 100, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tableHeaderObjectChecksum),
        makeWidget({ 4, 57 }, { 352, 176 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    Window* open(const std::vector<ObjectHeader>& list)
    {
        _loadErrorObjectsList = list;
        std::sort(_loadErrorObjectsList.begin(), _loadErrorObjectsList.end(), [](const auto& lhs, const auto& rhs) { return lhs.getName() < rhs.getName(); });

        Window* window = WindowManager::bringToFront(WindowType::objectLoadError);
        if (window != nullptr)
        {
            WindowManager::invalidate(WindowType::objectLoadError);
            return window;
        }

        window = WindowManager::createWindowCentred(
            WindowType::objectLoadError,
            kWindowSize,
            WindowFlags::stickToFront,
            getEvents());

        window->widgets = _widgets;
        window->enabledWidgets = 1 << Widx::close;
        window->initScrollWidgets();

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->colour_0B);
        window->setColour(WindowColour::secondary, interface->colour_10);

        window->rowCount = static_cast<uint16_t>(_loadErrorObjectsList.size());
        window->rowHover = -1;

        return window;
    }

    static void draw(Ui::Window& self, Gfx::RenderTarget* rt)
    {
        // Draw widgets
        self.draw(rt);

        // Draw explanatory text
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        auto point = Point(self.x + 3, self.y + 19);
        drawingCtx.drawStringLeftWrapped(*rt, point, self.width - 6, self.getColour(WindowColour::secondary), StringIds::objectErrorExplanation);
    }

    static StringId objectTypeToString(ObjectType type)
    {
        switch (type)
        {
            case ObjectType::interfaceSkin:
                return StringIds::object_interface_styles;
            case ObjectType::sound:
                return StringIds::object_sounds;
            case ObjectType::currency:
                return StringIds::object_currency;
            case ObjectType::steam:
                return StringIds::object_animation_effects;
            case ObjectType::cliffEdge:
                return StringIds::object_cliffs;
            case ObjectType::water:
                return StringIds::object_water;
            case ObjectType::land:
                return StringIds::object_land;
            case ObjectType::townNames:
                return StringIds::object_town_names;
            case ObjectType::cargo:
                return StringIds::object_cargo;
            case ObjectType::wall:
                return StringIds::object_walls;
            case ObjectType::trackSignal:
                return StringIds::object_signals;
            case ObjectType::levelCrossing:
                return StringIds::object_level_crossing;
            case ObjectType::streetLight:
                return StringIds::object_street_lights;
            case ObjectType::tunnel:
                return StringIds::object_tunnels;
            case ObjectType::bridge:
                return StringIds::object_bridges;
            case ObjectType::trainStation:
                return StringIds::object_track_stations;
            case ObjectType::trackExtra:
                return StringIds::object_track_extras;
            case ObjectType::track:
                return StringIds::object_tracks;
            case ObjectType::roadStation:
                return StringIds::object_road_stations;
            case ObjectType::roadExtra:
                return StringIds::object_road_extras;
            case ObjectType::road:
                return StringIds::object_roads;
            case ObjectType::airport:
                return StringIds::object_airports;
            case ObjectType::dock:
                return StringIds::object_docks;
            case ObjectType::vehicle:
                return StringIds::object_vehicles;
            case ObjectType::tree:
                return StringIds::object_trees;
            case ObjectType::snow:
                return StringIds::object_snow;
            case ObjectType::climate:
                return StringIds::object_climate;
            case ObjectType::hillShapes:
                return StringIds::object_map_generation_data;
            case ObjectType::building:
                return StringIds::object_buildings;
            case ObjectType::scaffolding:
                return StringIds::object_scaffolding;
            case ObjectType::industry:
                return StringIds::object_industries;
            case ObjectType::region:
                return StringIds::object_world_region;
            case ObjectType::competitor:
                return StringIds::object_company_owners;
            case ObjectType::scenarioText:
                return StringIds::object_scenario_descriptions;
            default:
                return StringIds::empty;
        }
    }

    static void drawScroll(Ui::Window& window, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        const auto shade = Colours::getShade(window.getColour(WindowColour::secondary).c(), 4);
        drawingCtx.clearSingle(rt, shade);

        // Acquire string buffer
        auto* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

        uint16_t y = 0;
        auto namePos = Point(1, y);
        auto typePos = Point(window.widgets[Widx::typeHeader].left - 4, y);
        auto typeWidth = window.widgets[Widx::typeHeader].width() - 6;
        auto checksumPos = Point(window.widgets[Widx::checksumHeader].left - 4, y);
        for (uint16_t i = 0; i < window.rowCount; i++)
        {
            if (y + kRowHeight < rt.y)
            {
                y += kRowHeight;
                continue;
            }
            else if (y > rt.y + rt.height)
            {
                break;
            }

            StringId textColourId = StringIds::black_stringid;

            // Draw hover rectangle
            if (i == window.rowHover)
            {
                drawingCtx.drawRect(rt, 0, y, 800, kRowHeight, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                textColourId = StringIds::wcolour2_stringid;
            }

            auto& header = _loadErrorObjectsList[i];

            FormatArguments args{};
            args.push(StringIds::buffer_2039);

            // Copy object name to buffer
            std::memcpy(buffer, header.name, 8);
            buffer[8] = '\0';

            // Draw object name
            namePos.y = y;
            drawingCtx.drawStringLeft(rt, namePos, window.getColour(WindowColour::secondary), textColourId, &args);

            // Copy object checksum to buffer
            const auto checksum = fmt::format("{:08X}", header.checksum);
            std::memcpy(buffer, checksum.c_str(), 8);
            buffer[8] = '\0';

            // Draw object checksum
            checksumPos.y = y;
            drawingCtx.drawStringLeft(rt, checksumPos, window.getColour(WindowColour::secondary), textColourId, &args);

            // Prepare object type for drawing
            args.rewind();
            args.push(objectTypeToString(header.getType()));

            // Draw object type
            typePos.y = y;
            drawingCtx.drawStringLeftWrapped(rt, typePos, typeWidth, window.getColour(WindowColour::secondary), textColourId, &args);

            y += kRowHeight;
        }
    }

    static void getScrollSize(Ui::Window& window, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = kRowHeight * window.rowCount;
    }

    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::close:
                WindowManager::close(window.type);
                break;
        }
    }

    static void onScrollMouseOver(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        uint16_t currentTrack = y / kRowHeight;
        if (currentTrack > window.rowCount || currentTrack == window.rowHover)
            return;

        window.rowHover = currentTrack;
        window.invalidate();
    }

    static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_object_list);
        return args;
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .getScrollSize = getScrollSize,
        .scrollMouseOver = onScrollMouseOver,
        .tooltip = tooltip,
        .draw = draw,
        .drawScroll = drawScroll,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
