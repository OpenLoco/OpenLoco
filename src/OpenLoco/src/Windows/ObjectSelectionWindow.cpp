#include "Audio/Audio.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Objects/AirportObject.h"
#include "Objects/BridgeObject.h"
#include "Objects/BuildingObject.h"
#include "Objects/CargoObject.h"
#include "Objects/CliffEdgeObject.h"
#include "Objects/CompetitorObject.h"
#include "Objects/CurrencyObject.h"
#include "Objects/DockObject.h"
#include "Objects/HillShapesObject.h"
#include "Objects/IndustryObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/LandObject.h"
#include "Objects/LevelCrossingObject.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "Objects/RegionObject.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/RoadStationObject.h"
#include "Objects/ScaffoldingObject.h"
#include "Objects/SnowObject.h"
#include "Objects/StreetLightObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/TrainSignalObject.h"
#include "Objects/TrainStationObject.h"
#include "Objects/TreeObject.h"
#include "Objects/TunnelObject.h"
#include "Objects/VehicleObject.h"
#include "Objects/WallObject.h"
#include "Objects/WaterObject.h"
#include "SceneManager.h"
#include "Ui/TextInput.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "Window.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <array>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ObjectSelectionWindow
{
    static constexpr int kRowHeight = 12;
    static constexpr int kPrimaryTabRowCapacity = 19;
    static constexpr int kSecondaryTabRowCapacity = 18;
    static constexpr Ui::Size kWindowSize = { 600, 398 };

    enum class ObjectTabFlags : uint8_t
    {
        none = 0U,
        alwaysHidden = 1U << 0,
        advanced = 1U << 1,
        hideInGame = 1U << 2,
        hideInEditor = 1U << 3,
        showEvenIfSingular = 1U << 4,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ObjectTabFlags);

    struct TabDisplayInfo
    {
        string_id name;
        uint32_t image;
        ObjectTabFlags flags;
    };

    // clang-format off
    static constexpr std::array<TabDisplayInfo, ObjectManager::maxObjectTypes> _tabDisplayInfo = {
        TabDisplayInfo{ StringIds::object_interface_styles,      ImageIds::tab_object_settings,        ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_sounds,                ImageIds::tab_object_audio,           ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
        TabDisplayInfo{ StringIds::object_currency,              ImageIds::tab_object_currency,        ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_animation_effects,     ImageIds::tab_object_smoke,           ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
        TabDisplayInfo{ StringIds::object_cliffs,                ImageIds::tab_object_cliff,           ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
        TabDisplayInfo{ StringIds::object_water,                 ImageIds::tab_object_water,           ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_land,                  ImageIds::tab_object_landscape,       ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_town_names,            ImageIds::tab_object_town_names,      ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_cargo,                 ImageIds::tab_object_cargo,           ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
        TabDisplayInfo{ StringIds::object_walls,                 ImageIds::tab_object_walls,           ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_signals,               ImageIds::tab_object_signals,         ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_level_crossing,        ImageIds::tab_object_level_crossings, ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_street_lights,         ImageIds::tab_object_streetlights,    ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_tunnels,               ImageIds::tab_object_tunnels,         ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
        TabDisplayInfo{ StringIds::object_bridges,               ImageIds::tab_object_bridges,         ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_track_stations,        ImageIds::tab_object_track_stations,  ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_track_extras,          ImageIds::tab_object_track_mods,      ObjectTabFlags::advanced | ObjectTabFlags::showEvenIfSingular },
        TabDisplayInfo{ StringIds::object_tracks,                ImageIds::tab_object_track,           ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_road_stations,         ImageIds::tab_object_road_stations,   ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_road_extras,           ImageIds::tab_object_road_mods,       ObjectTabFlags::advanced | ObjectTabFlags::showEvenIfSingular },
        TabDisplayInfo{ StringIds::object_roads,                 ImageIds::tab_object_road,            ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_airports,              ImageIds::tab_object_airports,        ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_docks,                 ImageIds::tab_object_docks,           ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_vehicles,              ImageIds::tab_object_vehicles,        ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_trees,                 ImageIds::tab_object_trees,           ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_snow,                  ImageIds::tab_object_snow,            ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_climate,               ImageIds::tab_object_climate,         ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_map_generation_data,   ImageIds::tab_object_map,             ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_buildings,             ImageIds::tab_object_buildings,       ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_scaffolding,           ImageIds::tab_object_construction,    ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_industries,            ImageIds::tab_object_industries,      ObjectTabFlags::advanced },
        TabDisplayInfo{ StringIds::object_world_region,          ImageIds::tab_object_world,           ObjectTabFlags::none },
        TabDisplayInfo{ StringIds::object_company_owners,        ImageIds::tab_object_companies,       ObjectTabFlags::hideInEditor },
        TabDisplayInfo{ StringIds::object_scenario_descriptions, ImageIds::tab_object_scenarios,       ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden }
    };
    // clang-format on

    struct TabPosition
    {
        uint8_t index;
        uint8_t row;
    };

    enum class Visibility
    {
        hidden = 0,
        shown = 1,
    };

    struct TabObjectEntry
    {
        ObjectManager::ObjectIndexId index;
        ObjectManager::ObjectIndexEntry object;
        Visibility display;
    };

    static loco_global<char[2], 0x005045F8> _strCheckmark;
    static loco_global<uint8_t*, 0x50D144> _objectSelection;

    static loco_global<uint16_t, 0x0052334A> _mousePosX;
    static loco_global<uint16_t, 0x0052334C> _mousePosY;

    // _tabObjectCounts can be integrated after implementing sub_473A95
    static loco_global<uint16_t[33], 0x00112C181> _tabObjectCounts;

    // 0x0112C21C
    static TabPosition _tabPositions[36];
    static std::vector<TabObjectEntry> _tabObjectList;
    static uint16_t _numVisibleObjectsListed;

    static Ui::TextInput::InputSession inputSession;

    static void initEvents();
    static void assignTabPositions(Window* self);

    enum widx
    {
        frame,
        caption,
        closeButton,
        panel,
        tabArea,
        advancedButton,
        textInput,
        clearButton,
        scrollview,
        objectImage,
    };

    Widget widgets[] = {
        makeWidget({ 0, 0 }, { 600, 398 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 598, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::title_object_selection),
        makeWidget({ 585, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 65 }, { 600, 333 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 3, 15 }, { 589, 50 }, WidgetType::wt_6, WindowColour::secondary),
        makeWidget({ 470, 20 }, { 122, 12 }, WidgetType::button, WindowColour::primary, StringIds::object_selection_advanced, StringIds::object_selection_advanced_tooltip),
        makeWidget({ 4, 68 }, { 246, 14 }, WidgetType::textbox, WindowColour::secondary),
        makeWidget({ 254, 68 }, { 38, 14 }, WidgetType::button, WindowColour::secondary, StringIds::clearInput),
        makeWidget({ 4, 85 }, { 288, 300 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        makeWidget({ 391, 68 }, { 114, 114 }, WidgetType::buttonWithImage, WindowColour::secondary),
        widgetEnd(),
    };

    static WindowEventList _events;

    // 0x0047322A
    static void rotateTabs(uint8_t newStartPosition)
    {
        auto isSentinel = [](auto& entry) { return entry.index == 0xFF; };
        auto sentinelPos = std::find_if(std::begin(_tabPositions), std::end(_tabPositions), isSentinel);

        std::rotate(std::begin(_tabPositions), std::begin(_tabPositions) + newStartPosition, sentinelPos);

        for (uint8_t i = 0; _tabPositions[i].index != 0xFF; i++)
        {
            _tabPositions[i].row = i < kPrimaryTabRowCapacity ? 0 : 1;
        }
    }

    // 0x004731EE
    static void repositionTargetTab(Window* self, ObjectType targetTab)
    {
        self->currentTab = enumValue(targetTab);
        for (auto i = 0U; i < std::size(_tabPositions); i++)
        {
            // Ended up in a position without info? Reassign positions first.
            if (_tabPositions[i].index == 0xFF)
            {
                self->var_856 |= (1 << 0);
                assignTabPositions(self);
                return;
            }

            if (_tabPositions[i].index == enumValue(targetTab))
            {
                // Found current tab, and its in bottom row? No change required
                if (_tabPositions[i].row == 0)
                    return;
                // Otherwise, we'll rotate the tabs around, such that this one is in the bottom row
                else
                    return rotateTabs(i);
            }
        }
    }

    // 0x00473154
    static void assignTabPositions(Window* self)
    {
        uint8_t currentRow = 0;
        uint8_t currentPos = 0;
        uint8_t rowCapacity = kPrimaryTabRowCapacity;
        uint8_t tabPos = 0;

        for (int8_t currentType = ObjectManager::maxObjectTypes - 1; currentType >= 0; currentType--)
        {
            const ObjectTabFlags tabFlags = _tabDisplayInfo[currentType].flags;

            if ((tabFlags & ObjectTabFlags::alwaysHidden) != ObjectTabFlags::none)
                continue;

            // Skip all types that don't have any objects
            if (_tabObjectCounts[currentType] == 0)
                continue;

            // Skip certain object types that only have one entry in game
            if ((tabFlags & ObjectTabFlags::showEvenIfSingular) == ObjectTabFlags::none && _tabObjectCounts[currentType] == 1)
                continue;

            // Hide advanced object types as needed
            if ((self->var_856 & (1 << 0)) == 0 && (tabFlags & ObjectTabFlags::advanced) != ObjectTabFlags::none)
                continue;

            if (isEditorMode() && (tabFlags & ObjectTabFlags::hideInEditor) != ObjectTabFlags::none)
                continue;

            if (!isEditorMode() && (tabFlags & ObjectTabFlags::hideInGame) != ObjectTabFlags::none)
                continue;

            // Assign tab position
            _tabPositions[tabPos].index = static_cast<uint8_t>(currentType);
            _tabPositions[tabPos].row = currentRow;
            tabPos++;

            // Distribute tabs over two rows -- ensure there's capacity left in current row
            currentPos++;
            if (currentPos >= rowCapacity)
            {
                currentPos = 0;
                rowCapacity = kSecondaryTabRowCapacity;
                currentRow++;
            }
        }

        // Add a marker to denote the last tab
        _tabPositions[tabPos].index = 0xFF;

        const auto firstTabIndex = ObjectType(_tabPositions[0].index);
        repositionTargetTab(self, firstTabIndex);
    }

    static bool contains(const std::string_view& a, const std::string_view& b)
    {
        return std::search(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
                   return tolower(a) == tolower(b);
               })
            != a.end();
    }

    static void applyFilterToObjectList()
    {
        std::string_view pattern = inputSession.buffer;
        _numVisibleObjectsListed = 0;
        for (auto& entry : _tabObjectList)
        {
            if (pattern.empty())
            {
                entry.display = Visibility::shown;
                _numVisibleObjectsListed++;
                continue;
            }

            const std::string_view name = entry.object._name;
            const std::string_view filename = entry.object._filename;

            const bool containsName = contains(name, pattern);
            const bool containsFileName = contains(filename, pattern);

            entry.display = containsName || containsFileName ? Visibility::shown : Visibility::hidden;

            if (entry.display == Visibility::shown)
                _numVisibleObjectsListed++;
        }
    }

    static void populateTabObjectList(ObjectType objectType)
    {
        _tabObjectList.clear();

        const auto objects = ObjectManager::getAvailableObjects(objectType);
        _tabObjectList.reserve(objects.size());
        for (auto [index, object] : objects)
        {
            auto entry = TabObjectEntry{ index, object, Visibility::shown };
            _tabObjectList.emplace_back(std::move(entry));
        }

        applyFilterToObjectList();
    }

    // 0x00472BBC
    static ObjectManager::ObjIndexPair getFirstAvailableSelectedObject([[maybe_unused]] Window* self)
    {
        for (auto& entry : _tabObjectList)
        {
            if (_objectSelection[entry.index] & (1 << 0))
            {
                return { static_cast<int16_t>(entry.index), entry.object };
            }
        }

        if (_tabObjectList.size() > 0)
        {
            return { static_cast<int16_t>(_tabObjectList[0].index), _tabObjectList[0].object };
        }

        return { -1, ObjectManager::ObjectIndexEntry{} };
    }

    // 0x00473A95
    static void sub_473A95()
    {
        // Title::sub_473A95
        registers regs;
        regs.eax = 0;
        call(0x00473A95, regs);
    }

    // 0x00472A20
    Ui::Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::objectSelection);

        if (window != nullptr)
            return window;

        sub_473A95();

        window = WindowManager::createWindowCentred(WindowType::objectSelection, { kWindowSize }, WindowFlags::none, &_events);
        window->widgets = widgets;
        window->enabledWidgets = (1ULL << widx::closeButton) | (1ULL << widx::tabArea) | (1ULL << widx::advancedButton) | (1ULL << widx::clearButton);
        window->initScrollWidgets();
        window->frameNo = 0;
        window->rowHover = -1;
        window->var_856 = isEditorMode() ? 0 : 1;

        initEvents();

        window->object = nullptr;

        assignTabPositions(window);
        repositionTargetTab(window, ObjectType::region);
        populateTabObjectList(ObjectType::region);

        ObjectManager::freeTemporaryObject();

        auto objIndex = getFirstAvailableSelectedObject(window);
        if (objIndex.index != -1)
        {
            window->rowHover = objIndex.index;
            window->object = reinterpret_cast<std::byte*>(objIndex.object._header);
            ObjectManager::loadTemporaryObject(*objIndex.object._header);
        }

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_0C);

        inputSession = Ui::TextInput::InputSession();
        inputSession.calculateTextOffset(widgets[widx::textInput].width());

        return window;
    }

    // 0x004733AC
    static void prepareDraw(Ui::Window& self)
    {
        self.activatedWidgets |= (1 << widx::objectImage);
        widgets[widx::closeButton].type = WidgetType::buttonWithImage;

        if (isEditorMode())
        {
            widgets[widx::closeButton].type = WidgetType::none;
        }

        self.activatedWidgets &= ~(1 << widx::advancedButton);

        if (self.var_856 & (1 << 0))
        {
            self.activatedWidgets |= (1 << widx::advancedButton);
        }

        auto args = FormatArguments();
        args.push(_tabDisplayInfo[self.currentTab].name);
    }

    static loco_global<uint16_t[40], 0x0112C1C5> _112C1C5;

    static constexpr uint8_t kRowOffsetX = 10;
    static constexpr uint8_t kRowOffsetY = 24;

    // 0x0047328D
    static void drawTabs(Window* self, Gfx::RenderTarget* rt)
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto y = self->widgets[widx::panel].top + self->y - 26;
        auto x = self->x + 3;

        for (auto row = 1; row >= 0; row--)
        {
            auto xPos = x + (row * kRowOffsetX);
            auto yPos = y - (row * kRowOffsetY);
            for (auto index = 0; _tabPositions[index].index != 0xFF; index++)
            {
                if (_tabPositions[index].row != row)
                    continue;

                auto image = Gfx::recolour(ImageIds::tab, self->getColour(WindowColour::secondary).c());
                if (_tabPositions[index].index == self->currentTab)
                {
                    image = Gfx::recolour(ImageIds::selected_tab, self->getColour(WindowColour::secondary).c());
                    drawingCtx.drawImage(rt, xPos, yPos, image);

                    image = Gfx::recolour(_tabDisplayInfo[_tabPositions[index].index].image, Colour::mutedSeaGreen);
                    drawingCtx.drawImage(rt, xPos, yPos, image);
                }
                else
                {
                    drawingCtx.drawImage(rt, xPos, yPos, image);

                    image = Gfx::recolour(_tabDisplayInfo[_tabPositions[index].index].image, Colour::mutedSeaGreen);
                    drawingCtx.drawImage(rt, xPos, yPos, image);

                    image = Gfx::recolourTranslucent(ImageIds::tab, ExtColour::unk33);
                    drawingCtx.drawImage(rt, xPos, yPos, image);

                    if (row < 1)
                    {
                        auto colour = Colours::getShade(self->getColour(WindowColour::secondary).c(), 7);
                        drawingCtx.drawRect(*rt, xPos, yPos + 26, 31, 1, colour, Drawing::RectFlags::none);
                    }
                }
                xPos += 31;
            }
        }
    }

    static constexpr Ui::Point kObjectPreviewOffset = { 56, 56 };
    static constexpr Ui::Size kObjectPreviewSize = { 112, 112 };
    static constexpr uint8_t kDescriptionRowHeight = 10;

    template<typename T>
    static void callDrawPreviewImage(Gfx::RenderTarget& rt, const Ui::Point& drawingOffset, Object* objectPtr)
    {
        auto object = reinterpret_cast<T*>(objectPtr);
        object->drawPreviewImage(rt, drawingOffset.x, drawingOffset.y);
    }

    // 0x00473579
    static void drawPreviewImage(ObjectHeader* header, Gfx::RenderTarget* rt, int16_t x, int16_t y, Object* objectPtr)
    {
        auto type = header->getType();

        // Clip the draw area to simplify image draw
        Ui::Point drawAreaPos = Ui::Point{ x, y } - kObjectPreviewOffset;
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(drawAreaPos.x, drawAreaPos.y, kObjectPreviewSize.width, kObjectPreviewSize.height));
        if (!clipped)
            return;

        switch (type)
        {
            case ObjectType::interfaceSkin:
                callDrawPreviewImage<InterfaceSkinObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::currency:
                callDrawPreviewImage<CurrencyObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::cliffEdge:
                callDrawPreviewImage<CliffEdgeObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::water:
                callDrawPreviewImage<WaterObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::land:
                callDrawPreviewImage<LandObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::wall:
                callDrawPreviewImage<WallObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trackSignal:
                callDrawPreviewImage<TrainSignalObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::levelCrossing:
                callDrawPreviewImage<LevelCrossingObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::streetLight:
                callDrawPreviewImage<StreetLightObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::tunnel:
                callDrawPreviewImage<TunnelObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::bridge:
                callDrawPreviewImage<BridgeObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trackStation:
                callDrawPreviewImage<TrainStationObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trackExtra:
                callDrawPreviewImage<TrackExtraObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::track:
                callDrawPreviewImage<TrackObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::roadStation:
                callDrawPreviewImage<RoadStationObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::roadExtra:
                callDrawPreviewImage<RoadExtraObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::road:
                callDrawPreviewImage<RoadObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::airport:
                callDrawPreviewImage<AirportObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::dock:
                callDrawPreviewImage<DockObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::vehicle:
                callDrawPreviewImage<VehicleObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::tree:
                callDrawPreviewImage<TreeObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::snow:
                callDrawPreviewImage<SnowObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::hillShapes:
                callDrawPreviewImage<HillShapesObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::building:
                callDrawPreviewImage<BuildingObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::scaffolding:
                callDrawPreviewImage<ScaffoldingObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::industry:
                callDrawPreviewImage<IndustryObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::region:
                callDrawPreviewImage<RegionObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::competitor:
                callDrawPreviewImage<CompetitorObject>(*clipped, kObjectPreviewOffset, objectPtr);
                break;

            default:
                // null
                break;
        }
    }

    template<typename T>
    static void callDrawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, const int16_t width, Object* objectPtr)
    {
        auto object = reinterpret_cast<T*>(objectPtr);
        object->drawDescription(rt, x, y, width);
    }

    static void drawDescription(ObjectHeader* header, Window* self, Gfx::RenderTarget* rt, int16_t x, int16_t y, Object* objectPtr)
    {
        int16_t width = self->x + self->width - x;
        int16_t height = self->y + self->height - y;
        // Clip the draw area to simplify image draw
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(x, y, width, height));
        if (!clipped)
            return;

        switch (header->getType())
        {
            case ObjectType::levelCrossing:
                callDrawDescription<LevelCrossingObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::trackStation:
                callDrawDescription<TrainStationObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::roadStation:
                callDrawDescription<RoadStationObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::airport:
                callDrawDescription<AirportObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::dock:
                callDrawDescription<DockObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::vehicle:
                callDrawDescription<VehicleObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::building:
                callDrawDescription<BuildingObject>(*clipped, 0, 0, width, objectPtr);
                break;

            case ObjectType::competitor:
                callDrawDescription<CompetitorObject>(*clipped, 0, 0, width, objectPtr);
                break;

            default:
                // null
                break;
        }
    }

    static void drawDatDetails(const ObjectManager::ObjectIndexEntry& indexEntry, Window* self, Gfx::RenderTarget* rt, int16_t x, int16_t y)
    {
        int16_t width = self->x + self->width - x;
        int16_t height = self->y + self->height - y;
        // Clip the draw area to simplify image draw
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(x, y, width, height));
        if (!clipped)
            return;

        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            strncpy(buffer, indexEntry._filename, strlen(indexEntry._filename) + 1);
            FormatArguments args{};
            args.push<string_id>(StringIds::buffer_1250);
            drawingCtx.drawStringLeft(*clipped, 18, height - kDescriptionRowHeight * 3 - 4, Colour::black, StringIds::object_selection_filename, &args);
        }
    }

    static void drawSearchBox(Window& self, Gfx::RenderTarget* rt)
    {
        char* textBuffer = (char*)StringManager::getString(StringIds::buffer_2039);
        strncpy(textBuffer, inputSession.buffer.c_str(), 256);

        auto& widget = widgets[widx::textInput];
        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(widget.left + 1 + self.x, widget.top + 1 + self.y, widget.width() - 2, widget.height() - 2));
        if (!clipped)
            return;

        FormatArguments args{};
        args.push(StringIds::buffer_2039);

        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        // Draw search box input buffer
        Ui::Point position = { inputSession.xOffset, 1 };
        drawingCtx.drawStringLeft(*clipped, &position, Colour::black, StringIds::black_stringid, &args);

        // Draw search box cursor, blinking
        if ((inputSession.cursorFrame % 32) < 16)
        {
            // We draw the string again to figure out where the cursor should go; position.x will be adjusted
            textBuffer[inputSession.cursorPosition] = '\0';
            position = { inputSession.xOffset, 1 };
            drawingCtx.drawStringLeft(*clipped, &position, Colour::black, StringIds::black_stringid, &args);
            drawingCtx.fillRect(*clipped, position.x, position.y, position.x, position.y + 9, Colours::getShade(self.getColour(WindowColour::secondary).c(), 9), Drawing::RectFlags::none);
        }
    }

    // 0x004733F5
    static void draw(Window& self, Gfx::RenderTarget* rt)
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        drawingCtx.fillRectInset(*rt, self.x, self.y + 20, self.x + self.width - 1, self.y + 20 + 60, self.getColour(WindowColour::primary), Drawing::RectInsetFlags::none);
        self.draw(rt);

        drawTabs(&self, rt);
        drawSearchBox(self, rt);

        bool doDefault = true;
        if (self.object != nullptr)
        {
            auto objectPtr = self.object;
            auto var = ObjectManager::ObjectIndexEntry::read(&objectPtr)._header;
            if (var->getType() != ObjectType::townNames && var->getType() != ObjectType::climate)
            {
                doDefault = false;
            }
        }

        if (doDefault)
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 5);
            drawingCtx.drawRect(*rt, self.x + widget.left, self.y + widget.top, widget.width(), widget.height(), colour, Drawing::RectFlags::none);
        }
        else
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 0);
            drawingCtx.drawRect(*rt, self.x + widget.left + 1, self.y + widget.top + 1, widget.width() - 2, widget.height() - 2, colour, Drawing::RectFlags::none);
        }

        auto type = self.currentTab;

        auto args = FormatArguments();
        args.push(_112C1C5[type]);
        args.push(ObjectManager::getMaxObjects(static_cast<ObjectType>(type)));

        drawingCtx.drawStringLeft(*rt, self.x + 3, self.y + self.height - 12, Colour::black, 2038, &args);

        if (self.rowHover == -1)
            return;

        auto* temporaryObject = ObjectManager::getTemporaryObject();

        if (temporaryObject == nullptr)
            return;

        {
            auto objectPtr = self.object;

            drawPreviewImage(
                ObjectManager::ObjectIndexEntry::read(&objectPtr)._header,
                rt,
                widgets[widx::objectImage].midX() + 1 + self.x,
                widgets[widx::objectImage].midY() + 1 + self.y,
                temporaryObject);
        }

        auto x = self.widgets[widx::objectImage].midX() + self.x;
        auto y = self.widgets[widx::objectImage].bottom + 3 + self.y;
        auto width = self.width - self.widgets[widx::scrollview].right - 6;

        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            *buffer++ = ControlCodes::windowColour2;
            auto objectPtr = self.object;

            strncpy(buffer, ObjectManager::ObjectIndexEntry::read(&objectPtr)._name, 510);

            drawingCtx.drawStringCentredClipped(*rt, x, y, width, Colour::black, StringIds::buffer_2039);
        }

        {
            auto objectPtr = self.object;

            drawDescription(
                ObjectManager::ObjectIndexEntry::read(&objectPtr)._header,
                &self,
                rt,
                self.widgets[widx::scrollview].right + self.x + 4,
                y + kDescriptionRowHeight,
                temporaryObject);
        }

        {
            auto objectPtr = self.object;

            drawDatDetails(
                ObjectManager::ObjectIndexEntry::read(&objectPtr),
                &self,
                rt,
                self.widgets[widx::scrollview].right + self.x + 4,
                y + kDescriptionRowHeight);
        }
    }

    // 0x0047361D
    static void drawScroll(Window& self, Gfx::RenderTarget& rt, const uint32_t)
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        drawingCtx.clearSingle(rt, Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

        if (ObjectManager::getNumInstalledObjects() == 0)
            return;

        int y = 0;
        for (auto& entry : _tabObjectList)
        {
            if (entry.display == Visibility::hidden)
                continue;

            if (y + kRowHeight < rt.y)
            {
                y += kRowHeight;
                continue;
            }
            else if (y > rt.y + rt.height)
            {
                break;
            }

            Drawing::RectInsetFlags flags = Drawing::RectInsetFlags::colourLight | Drawing::RectInsetFlags::fillDarker | Drawing::RectInsetFlags::borderInset;
            drawingCtx.fillRectInset(rt, 2, y, 11, y + 10, self.getColour(WindowColour::secondary), flags);

            uint8_t textColour = ControlCodes::Colour::black;

            auto objectPtr = self.object;
            if (objectPtr != nullptr)
            {
                auto windowObjectName = ObjectManager::ObjectIndexEntry::read(&objectPtr)._name;
                if (entry.object._name == windowObjectName)
                {
                    drawingCtx.fillRect(rt, 0, y, self.width, y + kRowHeight - 1, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
                    textColour = ControlCodes::windowColour2;
                }
            }

            if (_objectSelection[entry.index] & (1 << 0))
            {
                auto x = 2;
                drawingCtx.setCurrentFontSpriteBase(Font::m2);

                if (textColour != ControlCodes::windowColour2)
                {
                    drawingCtx.setCurrentFontSpriteBase(Font::m1);
                }

                auto checkColour = self.getColour(WindowColour::secondary).opaque();

                if (_objectSelection[entry.index] & 0x1C)
                {
                    checkColour = checkColour.inset();
                }

                drawingCtx.drawString(rt, x, y, checkColour, _strCheckmark);
            }

            char buffer[512]{};
            buffer[0] = textColour;
            strncpy(&buffer[1], entry.object._name, 510);
            drawingCtx.setCurrentFontSpriteBase(Font::medium_bold);

            drawingCtx.drawString(rt, 15, y, Colour::black, buffer);
            y += kRowHeight;
        }
    }

    // 0x00473A13
    bool tryCloseWindow()
    {
        return !(call(0x00473A13) & X86_FLAG_CARRY);
    }

    // 0x004737BA
    static void onMouseUp(Window& self, WidgetIndex_t w)
    {
        switch (w)
        {
            case widx::closeButton:
                tryCloseWindow();
                break;

            case widx::tabArea:
            {
                int clickedTab = -1;
                int y = widgets[widx::panel].top + self.y - 26;
                int x = self.x + 3;

                for (int row = 0; row < 2; row++)
                {
                    auto xPos = x + (row * kRowOffsetX);
                    auto yPos = y - (row * kRowOffsetY);

                    for (int i = 0; _tabPositions[i].index != 0xFF; i++)
                    {
                        if (_tabPositions[i].row != row)
                            continue;

                        if (_mousePosX >= xPos && _mousePosY >= yPos)
                        {
                            if (_mousePosX < xPos + 31 && yPos + 27 > _mousePosY)
                            {
                                clickedTab = _tabPositions[i].index;
                                break;
                            }
                        }

                        xPos += 31;
                    }
                }

                if (clickedTab != -1 && self.currentTab != clickedTab)
                {
                    repositionTargetTab(&self, static_cast<ObjectType>(clickedTab));
                    populateTabObjectList(static_cast<ObjectType>(clickedTab));

                    self.rowHover = -1;
                    self.object = nullptr;
                    self.scrollAreas[0].contentWidth = 0;
                    ObjectManager::freeTemporaryObject();
                    auto objIndex = getFirstAvailableSelectedObject(&self);

                    if (objIndex.index != -1)
                    {
                        self.rowHover = objIndex.index;
                        self.object = reinterpret_cast<std::byte*>(objIndex.object._header);

                        ObjectManager::loadTemporaryObject(*objIndex.object._header);
                    }

                    self.invalidate();
                }

                break;
            }

            case widx::advancedButton:
            {
                self.var_856 ^= 1;
                int currentTab = self.currentTab;
                assignTabPositions(&self);

                if ((self.var_856 & 1) == 0)
                {
                    const ObjectTabFlags tabFlags = _tabDisplayInfo[currentTab].flags;
                    if ((tabFlags & ObjectTabFlags::advanced) != ObjectTabFlags::none)
                    {
                        currentTab = _tabPositions[0].index;
                        populateTabObjectList(static_cast<ObjectType>(currentTab));
                    }
                }
                repositionTargetTab(&self, static_cast<ObjectType>(currentTab));
                self.invalidate();

                break;
            }

            case widx::clearButton:
            {
                inputSession.clearInput();
                applyFilterToObjectList();
                self.invalidate();
            }
        }
    }

    // 0x004738ED
    static void getScrollSize([[maybe_unused]] Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = _numVisibleObjectsListed * kRowHeight;
    }

    // 0x00473900
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_object_list);
        return args;
    }

    // 0x00472B54
    static ObjectManager::ObjIndexPair getObjectFromSelection([[maybe_unused]] Window* self, int16_t& y)
    {
        for (auto& entry : _tabObjectList)
        {
            if (entry.display == Visibility::hidden)
                continue;

            y -= kRowHeight;
            if (y < 0)
            {
                return { static_cast<int16_t>(entry.index), entry.object };
            }
        }

        return { -1, ObjectManager::ObjectIndexEntry{} };
    }

    // 0x0047390A
    static void onScrollMouseOver(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(&self, y);

        if (objIndex.index == self.rowHover || objIndex.index == -1)
            return;

        self.rowHover = objIndex.index;
        self.object = reinterpret_cast<std::byte*>(objIndex.object._header);
        ObjectManager::freeTemporaryObject();

        if (objIndex.index != -1)
        {
            ObjectManager::loadTemporaryObject(*objIndex.object._header);
        }

        self.invalidate();
    }

    // 0x00473D1D
    static bool windowEditorObjectSelectionSelectObject(uint16_t bx, void* ebp)
    {
        registers regs;
        regs.bx = bx;
        regs.ebp = X86Pointer(ebp);

        return call(0x00473D1D, regs) & X86_FLAG_CARRY;
    }

    // 0x00473948
    static void onScrollMouseDown(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(&self, y);
        auto index = objIndex.index;
        auto object = objIndex.object._header;

        if (index == -1)
            return;

        self.invalidate();
        Audio::playSound(Audio::SoundId::clickDown, Input::getMouseLocation().x);

        auto type = objIndex.object._header->getType();

        if (ObjectManager::getMaxObjects(type) == 1)
        {
            if (!(_objectSelection[index] & (1 << 0)))
            {
                auto [oldIndex, oldObject] = ObjectManager::getActiveObject(type, _objectSelection);

                if (oldIndex != -1)
                {
                    windowEditorObjectSelectionSelectObject(6, oldObject._header);
                }
            }
        }

        auto bx = 0;

        if (!(_objectSelection[index] & (1 << 0)))
        {
            bx |= (1 << 0);
        }

        bx |= 6;

        if (!windowEditorObjectSelectionSelectObject(bx, object))
            return;

        auto errorTitle = StringIds::error_unable_to_select_object;

        if (!(bx & (1 << 0)))
        {
            errorTitle = StringIds::error_unable_to_deselect_object;
        }

        Ui::Windows::Error::open(errorTitle, GameCommands::getErrorText());
    }

    // 0x00474821
    static void unloadUnselectedObjects()
    {
        for (ObjectType type = ObjectType::interfaceSkin; enumValue(type) <= enumValue(ObjectType::scenarioText); type = static_cast<ObjectType>(enumValue(type) + 1))
        {
            auto objects = ObjectManager::getAvailableObjects(type);
            for (auto [i, object] : objects)
            {
                if (!(_objectSelection[i] & (1 << 0)))
                {
                    ObjectManager::unload(*object._header);
                }
            }
        }
    }

    // 0x00474874
    static void editorLoadSelectedObjects()
    {
        for (ObjectType type = ObjectType::interfaceSkin; enumValue(type) <= enumValue(ObjectType::scenarioText); type = static_cast<ObjectType>(enumValue(type) + 1))
        {
            auto objects = ObjectManager::getAvailableObjects(type);
            for (auto [i, object] : objects)
            {
                if (_objectSelection[i] & (1 << 0))
                {
                    if (!ObjectManager::findObjectHandle(*object._header))
                    {
                        ObjectManager::load(*object._header);
                    }
                }
            }
        }
    }

    // 0x00473B91
    static void editorObjectFlagsFree0()
    {
        call(0x00473B91); // editor_object_flags_free_0
    }

    // 0x004739DD
    static void onClose([[maybe_unused]] Window& self)
    {
        unloadUnselectedObjects();
        editorLoadSelectedObjects();
        ObjectManager::reloadAll();
        ObjectManager::freeTemporaryObject();

        if (!isEditorMode())
        {
            // Make new selection available in-game.
            ObjectManager::updateYearly2();
            ObjectManager::sub_4748FA();
            Gfx::loadCurrency();
            Gfx::loadPalette();
            Gfx::invalidateScreen();
        }
        else
        {
            editorObjectFlagsFree0();
        }
    }

    // 0x00473A04
    static void onUpdate(Window& self)
    {
        WindowManager::invalidateWidget(WindowType::objectSelection, self.number, widx::objectImage);

        inputSession.cursorFrame++;
        if ((inputSession.cursorFrame % 16) == 0)
        {
            WindowManager::invalidateWidget(WindowType::objectSelection, self.number, widx::textInput);
        }
    }

    void handleInput(uint32_t charCode, uint32_t keyCode)
    {
        auto* w = WindowManager::find(WindowType::objectSelection);
        if (w == nullptr)
            return;

        if (!inputSession.handleInput(charCode, keyCode))
            return;

        int containerWidth = widgets[widx::textInput].width() - 2;
        if (inputSession.needsReoffsetting(containerWidth))
            inputSession.calculateTextOffset(containerWidth);

        inputSession.cursorFrame = 0;
        WindowManager::invalidateWidget(WindowType::objectSelection, 0, widx::textInput);

        applyFilterToObjectList();
    }

    static void initEvents()
    {
        _events.onClose = onClose;
        _events.onMouseUp = onMouseUp;
        _events.onUpdate = onUpdate;
        _events.getScrollSize = getScrollSize;
        _events.scrollMouseDown = onScrollMouseDown;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;
        _events.drawScroll = drawScroll;
    }
}
