#include "Audio/Audio.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
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
#include "Ui/Dropdown.h"
#include "Ui/TextInput.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/TextBoxWidget.h"
#include "Ui/Window.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Diagnostics/Logging.h>
#include <array>
#include <numeric>
#include <ranges>
#include <vector>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Ui::Windows::ObjectSelectionWindow
{
    static constexpr int kRowHeight = 12;
    static constexpr Ui::Size32 kWindowSize = { 600, 398 };

    enum class ObjectTabFlags : uint8_t
    {
        none = 0U,
        alwaysHidden = 1U << 0,
        advanced = 1U << 1,
        hideInGame = 1U << 2,
        hideInEditor = 1U << 3,
        showEvenIfSingular = 1U << 4,
        filterByVehicleType = 1U << 5,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(ObjectTabFlags);

    struct SubTabInfo
    {
        StringId name;
        ObjectType objectType;
        VehicleType vehicleType;
        uint32_t baseImage;
        uint8_t animationLength;
        uint8_t animationDivisor;
        ObjectTabFlags flags;
    };

    struct MainTabInfo
    {
        StringId name;
        ObjectType objectType;
        uint32_t image;
        std::span<const SubTabInfo> subTabs;
        ObjectTabFlags flags;
    };

    // clang-format off
    static constexpr std::array kWorldRegionSubTabs{
        SubTabInfo{ StringIds::object_world_region, ObjectType::region,    {}, ImageIds::tab_object_world,      1, 1, ObjectTabFlags::none     },
        SubTabInfo{ StringIds::object_currency,     ObjectType::currency,  {}, ImageIds::tab_object_currency,   1, 1, ObjectTabFlags::advanced },
        SubTabInfo{ StringIds::object_town_names,   ObjectType::townNames, {}, ImageIds::tab_object_town_names, 1, 1, ObjectTabFlags::advanced },
    };

    static constexpr std::array kVehicleSubTabs{
        SubTabInfo{ StringIds::object_vehicles, ObjectType::vehicle, VehicleType::train,    InterfaceSkin::ImageIds::tab_vehicle_train_frame0,    8, 1, ObjectTabFlags::none },
        SubTabInfo{ StringIds::object_vehicles, ObjectType::vehicle, VehicleType::bus,      InterfaceSkin::ImageIds::tab_vehicle_bus_frame0,      8, 1, ObjectTabFlags::none },
        SubTabInfo{ StringIds::object_vehicles, ObjectType::vehicle, VehicleType::truck,    InterfaceSkin::ImageIds::tab_vehicle_truck_frame0,    8, 1, ObjectTabFlags::none },
        SubTabInfo{ StringIds::object_vehicles, ObjectType::vehicle, VehicleType::tram,     InterfaceSkin::ImageIds::tab_vehicle_tram_frame0,     8, 1, ObjectTabFlags::none },
        SubTabInfo{ StringIds::object_vehicles, ObjectType::vehicle, VehicleType::aircraft, InterfaceSkin::ImageIds::tab_vehicle_aircraft_frame0, 8, 2, ObjectTabFlags::none },
        SubTabInfo{ StringIds::object_vehicles, ObjectType::vehicle, VehicleType::ship,     InterfaceSkin::ImageIds::tab_vehicle_ship_frame0,     8, 3, ObjectTabFlags::none },
    };

    static constexpr std::array kLandSubTabs{
        SubTabInfo{ StringIds::object_land,                ObjectType::land,       {}, ImageIds::tab_object_landscape, 1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_trees,               ObjectType::tree,       {}, ImageIds::tab_object_trees,     1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_water,               ObjectType::water,      {}, ImageIds::tab_object_water,     1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_walls,               ObjectType::wall,       {}, ImageIds::tab_object_walls,     1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_map_generation_data, ObjectType::hillShapes, {}, ImageIds::tab_object_map,       1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_snow,                ObjectType::snow,       {}, ImageIds::tab_object_snow,      1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_climate,             ObjectType::climate,    {}, ImageIds::tab_object_climate,   1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_cliffs,              ObjectType::cliffEdge,  {}, ImageIds::tab_object_cliff,     1, 1, ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
    };

    static constexpr std::array kTrackSubTabs{
        SubTabInfo{ StringIds::object_tracks,         ObjectType::track,        {}, ImageIds::tab_object_track,          1, 1, ObjectTabFlags::advanced                                      },
        SubTabInfo{ StringIds::object_track_stations, ObjectType::trainStation, {}, ImageIds::tab_object_track_stations, 1, 1, ObjectTabFlags::advanced                                      },
        SubTabInfo{ StringIds::object_track_extras,   ObjectType::trackExtra,   {}, ImageIds::tab_object_track_mods,     1, 1, ObjectTabFlags::advanced | ObjectTabFlags::showEvenIfSingular },
        SubTabInfo{ StringIds::object_signals,        ObjectType::trackSignal,  {}, ImageIds::tab_object_signals,        1, 1, ObjectTabFlags::advanced                                      },
    };

    static constexpr std::array kRoadSubTabs{
        SubTabInfo{ StringIds::object_roads,          ObjectType::road,          {}, ImageIds::tab_object_road,            1, 1, ObjectTabFlags::advanced                                      },
        SubTabInfo{ StringIds::object_road_stations,  ObjectType::roadStation,   {}, ImageIds::tab_object_road_stations,   1, 1, ObjectTabFlags::advanced                                      },
        SubTabInfo{ StringIds::object_road_extras,    ObjectType::roadExtra,     {}, ImageIds::tab_object_road_mods,       1, 1, ObjectTabFlags::advanced | ObjectTabFlags::showEvenIfSingular },
        SubTabInfo{ StringIds::object_level_crossing, ObjectType::levelCrossing, {}, ImageIds::tab_object_level_crossings, 1, 1, ObjectTabFlags::advanced                                      },
        SubTabInfo{ StringIds::object_street_lights,  ObjectType::streetLight,   {}, ImageIds::tab_object_streetlights,    1, 1, ObjectTabFlags::advanced                                      },
    };

    static constexpr std::array kBuildingSubTabs{
        SubTabInfo{ StringIds::object_buildings,   ObjectType::building,    {}, ImageIds::tab_object_buildings,    1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_industries,  ObjectType::industry,    {}, ImageIds::tab_object_industries,   1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_scaffolding, ObjectType::scaffolding, {}, ImageIds::tab_object_construction, 1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_cargo,       ObjectType::cargo,       {}, ImageIds::tab_object_cargo,        1, 1, ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
    };

    static constexpr std::array kMiscSubTabs{
        SubTabInfo{ StringIds::object_interface_styles,      ObjectType::interfaceSkin, {}, ImageIds::tab_object_settings,  1, 1, ObjectTabFlags::advanced                                },
        SubTabInfo{ StringIds::object_scenario_descriptions, ObjectType::scenarioText,  {}, ImageIds::tab_object_scenarios, 1, 1, ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
        SubTabInfo{ StringIds::object_animation_effects,     ObjectType::steam,         {}, ImageIds::tab_object_smoke,     1, 1, ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
        SubTabInfo{ StringIds::object_sounds,                ObjectType::sound,         {}, ImageIds::tab_object_audio,     1, 1, ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
    };

    static constexpr std::array kMainTabInfo{
        MainTabInfo{ StringIds::object_world_region,     ObjectType::region,        ImageIds::tab_object_world,     kWorldRegionSubTabs, ObjectTabFlags::none                                    },
        MainTabInfo{ StringIds::object_vehicles,         ObjectType::vehicle,       ImageIds::tab_object_vehicles,  kVehicleSubTabs,     ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_land,             ObjectType::land,          ImageIds::tab_object_landscape, kLandSubTabs,        ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_tracks,           ObjectType::track,         ImageIds::tab_object_track,     kTrackSubTabs,       ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_roads,            ObjectType::road,          ImageIds::tab_object_road,      kRoadSubTabs,        ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_airports,         ObjectType::airport,       ImageIds::tab_object_airports,  {},                  ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_docks,            ObjectType::dock,          ImageIds::tab_object_docks,     {},                  ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_buildings,        ObjectType::building,      ImageIds::tab_object_buildings, kBuildingSubTabs,    ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_bridges,          ObjectType::bridge,        ImageIds::tab_object_bridges,   {},                  ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_tunnels,          ObjectType::tunnel,        ImageIds::tab_object_tunnels,   {},                  ObjectTabFlags::advanced | ObjectTabFlags::alwaysHidden },
        MainTabInfo{ StringIds::object_interface_styles, ObjectType::interfaceSkin, ImageIds::tab_object_settings,  kMiscSubTabs,        ObjectTabFlags::advanced                                },
        MainTabInfo{ StringIds::object_company_owners,   ObjectType::competitor,    ImageIds::tab_object_companies, {},                  ObjectTabFlags::hideInEditor                            },
    };
    // clang-format on

    using TabPosition = uint8_t;

    // Used for TabObjectEntry::display
    enum class Visibility : uint8_t
    {
        hidden = 0,
        shown = 1,
    };

    // Used for Window::filterLevel
    enum class FilterLevel : uint8_t
    {
        beginner = 0,
        advanced = 1,
        expert = 2,
    };

    // Used for Window::var_858
    enum class FilterFlags : uint8_t
    {
        none = 0,
        vanilla = 1 << 0,
        openLoco = 1 << 1,
        custom = 1 << 2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(FilterFlags);

    struct TabObjectEntry
    {
        ObjectManager::ObjectIndexId index;
        ObjectManager::ObjectIndexEntry object;
        Visibility display;
    };

    static loco_global<ObjectManager::SelectedObjectsFlags*, 0x50D144> _objectSelection;

    static std::span<ObjectManager::SelectedObjectsFlags> getSelectedObjectFlags()
    {
        return std::span<ObjectManager::SelectedObjectsFlags>(*_objectSelection, ObjectManager::getNumInstalledObjects());
    }

    // _tabObjectCounts can be integrated after implementing sub_473A95
    static loco_global<uint16_t[33], 0x00112C181> _tabObjectCounts;

    // 0x0112C21C
    static std::vector<TabPosition> _tabPositions;
    static std::vector<TabObjectEntry> _tabObjectList;
    static uint16_t _numVisibleObjectsListed;
    static bool _filterByVehicleType = false;
    static VehicleType _currentVehicleType;

    static Ui::TextInput::InputSession inputSession;

    static void assignTabPositions(Window* self);

    enum widx
    {
        frame,
        caption,
        closeButton,
        panel,
        tabArea,
        filterLabel,
        filterDropdown,
        textInput,
        clearButton,
        secondaryTab1,
        secondaryTab2,
        secondaryTab3,
        secondaryTab4,
        secondaryTab5,
        secondaryTab6,
        secondaryTab7,
        secondaryTab8,
        scrollviewFrame,
        scrollview,
        objectImage,
    };

    static constexpr uint8_t kMaxNumSecondaryTabs = 8;

    static constexpr auto widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { 600, 398 }, WindowColour::primary),
        makeWidget({ 1, 1 }, { 598, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::title_object_selection),
        Widgets::ImageButton({ 585, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 42 }, { 600, 356 }, WindowColour::secondary),

        // Primary tab area
        makeWidget({ 3, 15 }, { 589, 27 }, WidgetType::wt_6, WindowColour::secondary),

        // Filter options
        makeDropdownWidgets({ 492, 20 }, { 100, 12 }, WindowColour::primary, StringIds::empty),
        Widgets::TextBox({ 4, 45 }, { 246, 14 }, WindowColour::secondary),
        Widgets::Button({ 254, 45 }, { 38, 14 }, WindowColour::secondary, StringIds::clearInput),

        // Secondary tabs
        makeWidget({ 3, 62 }, { 31, 27 }, WidgetType::none, WindowColour::secondary, ImageIds::tab),
        makeWidget({ 34, 62 }, { 31, 27 }, WidgetType::none, WindowColour::secondary, ImageIds::tab),
        makeWidget({ 65, 62 }, { 31, 27 }, WidgetType::none, WindowColour::secondary, ImageIds::tab),
        makeWidget({ 96, 62 }, { 31, 27 }, WidgetType::none, WindowColour::secondary, ImageIds::tab),
        makeWidget({ 127, 62 }, { 31, 27 }, WidgetType::none, WindowColour::secondary, ImageIds::tab),
        makeWidget({ 158, 62 }, { 31, 27 }, WidgetType::none, WindowColour::secondary, ImageIds::tab),
        makeWidget({ 189, 62 }, { 31, 27 }, WidgetType::none, WindowColour::secondary, ImageIds::tab),
        makeWidget({ 220, 62 }, { 31, 27 }, WidgetType::none, WindowColour::secondary, ImageIds::tab),

        // Scroll and preview areas
        Widgets::Panel({ 3, 83 }, { 290, 303 }, WindowColour::secondary),
        makeWidget({ 4, 85 }, { 288, 300 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        Widgets::ImageButton({ 391, 45 }, { 114, 114 }, WindowColour::secondary)

    );

    template<typename TTabInfo>
    static bool shouldShowTab(TTabInfo& tabInfo, FilterLevel filterLevel)
    {
        const ObjectTabFlags tabFlags = tabInfo.flags;

        if (filterLevel == FilterLevel::expert)
        {
            return true;
        }

        if ((tabFlags & ObjectTabFlags::alwaysHidden) != ObjectTabFlags::none)
        {
            return false;
        }

        // Skip all types that don't have any objects
        auto objectType = enumValue(tabInfo.objectType);
        if (_tabObjectCounts[objectType] == 0)
        {
            return false;
        }

        // Skip certain object types that only have one entry in game
        if ((tabFlags & ObjectTabFlags::showEvenIfSingular) == ObjectTabFlags::none && _tabObjectCounts[objectType] == 1)
        {
            return false;
        }

        // Hide advanced object types in beginner mode
        if (filterLevel == FilterLevel::beginner && (tabFlags & ObjectTabFlags::advanced) != ObjectTabFlags::none)
        {
            return false;
        }

        if (isEditorMode() && (tabFlags & ObjectTabFlags::hideInEditor) != ObjectTabFlags::none)
        {
            return false;
        }

        if (!isEditorMode() && (tabFlags & ObjectTabFlags::hideInGame) != ObjectTabFlags::none)
        {
            return false;
        }

        return true;
    }

    static bool shouldShowPrimaryTab(uint8_t index, FilterLevel filterLevel)
    {
        return shouldShowTab(kMainTabInfo[index], filterLevel);
    }

    static bool shouldShowSubTab(std::span<const SubTabInfo> subTabs, uint8_t index, FilterLevel filterLevel)
    {
        return shouldShowTab(subTabs[index], filterLevel);
    }

    // 0x00473154
    static void assignTabPositions(Window* self)
    {
        _tabPositions.clear();
        for (auto i = 0U; i < kMainTabInfo.size(); i++)
        {
            if (!shouldShowPrimaryTab(i, FilterLevel(self->filterLevel)))
            {
                continue;
            }

            // Assign tab position
            _tabPositions.emplace_back(i);
        }
    }

    static bool contains(const std::string_view& a, const std::string_view& b)
    {
        return std::search(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
                   return tolower(a) == tolower(b);
               })
            != a.end();
    }

    static std::optional<VehicleType> getVehicleTypeFromObject(TabObjectEntry& entry)
    {
        auto& displayData = entry.object._displayData;
        if (displayData.vehicleSubType == 0xFF)
        {
            Logging::info("Could not load determine vehicle type for object '{}', skipping", entry.object._header.getName());
            return std::nullopt;
        }

        return static_cast<VehicleType>(displayData.vehicleSubType);
    }

    static void applyFilterToObjectList(FilterFlags filterFlags)
    {
        std::string_view pattern = inputSession.buffer;
        _numVisibleObjectsListed = 0;
        for (auto& entry : _tabObjectList)
        {
            // Apply vanilla/custom object filters
            const bool isVanillaObj = entry.object._header.isVanilla();
            const bool isOpenLocoObj = entry.object._header.getSourceGame() == SourceGame::openLoco;
            const bool isCustomObj = !isVanillaObj && !isOpenLocoObj;
            if (isVanillaObj && (filterFlags & FilterFlags::vanilla) == FilterFlags::none)
            {
                entry.display = Visibility::hidden;
                continue;
            }
            if (isOpenLocoObj && (filterFlags & FilterFlags::openLoco) == FilterFlags::none)
            {
                entry.display = Visibility::hidden;
                continue;
            }
            if (isCustomObj && (filterFlags & FilterFlags::custom) == FilterFlags::none)
            {
                entry.display = Visibility::hidden;
                continue;
            }

            if (_filterByVehicleType)
            {
                auto vehicleType = getVehicleTypeFromObject(entry);
                if (vehicleType != _currentVehicleType)
                {
                    entry.display = Visibility::hidden;
                    continue;
                }
            }

            if (pattern.empty())
            {
                entry.display = Visibility::shown;
                _numVisibleObjectsListed++;
                continue;
            }

            const std::string_view name = entry.object._name;
            const auto filename = fs::u8path(entry.object._filepath).filename().u8string();

            const bool containsName = contains(name, pattern);
            const bool containsFileName = contains(filename, pattern);

            entry.display = containsName || containsFileName ? Visibility::shown : Visibility::hidden;

            if (entry.display == Visibility::shown)
            {
                _numVisibleObjectsListed++;
            }
        }
    }

    static void populateTabObjectList(ObjectType objectType, FilterFlags filterFlags)
    {
        _tabObjectList.clear();

        const auto objects = ObjectManager::getAvailableObjects(objectType);
        _tabObjectList.reserve(objects.size());
        for (auto& [index, object] : objects)
        {
            auto entry = TabObjectEntry{ index, object, Visibility::shown };
            _tabObjectList.emplace_back(std::move(entry));
        }

        applyFilterToObjectList(filterFlags);
    }

    // 0x00472BBC
    static ObjectManager::ObjIndexPair getFirstAvailableSelectedObject([[maybe_unused]] Window* self)
    {
        const auto selectionFlags = getSelectedObjectFlags();
        for (auto& entry : _tabObjectList)
        {
            if ((selectionFlags[entry.index] & ObjectManager::SelectedObjectsFlags::selected) != ObjectManager::SelectedObjectsFlags::none)
            {
                return { static_cast<int16_t>(entry.index), entry.object };
            }
        }

        if (_tabObjectList.size() > 0)
        {
            return { static_cast<int16_t>(_tabObjectList[0].index), _tabObjectList[0].object };
        }

        return { ObjectManager::kNullObjectIndex, ObjectManager::ObjectIndexEntry{} };
    }

    static const WindowEventList& getEvents();

    // 0x00472A20
    Ui::Window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::objectSelection);

        if (window != nullptr)
        {
            return window;
        }

        ObjectManager::prepareSelectionList(true);

        window = WindowManager::createWindowCentred(WindowType::objectSelection, { kWindowSize }, WindowFlags::none, getEvents());
        window->setWidgets(widgets);
        window->enabledWidgets = (1ULL << widx::closeButton) | (1ULL << widx::tabArea) | (1ULL << widx::filterLabel) | (1ULL << widx::filterDropdown) | (1ULL << widx::clearButton);
        window->initScrollWidgets();
        window->frameNo = 0;
        window->rowHover = -1;
        window->filterLevel = enumValue(isEditorMode() ? FilterLevel::beginner : FilterLevel::advanced);
        window->var_858 = enumValue(FilterFlags::vanilla | FilterFlags::openLoco | FilterFlags::custom);
        window->currentSecondaryTab = 0;
        window->object = nullptr;

        assignTabPositions(window);
        static_assert(kMainTabInfo[0].objectType == ObjectType::region);
        populateTabObjectList(ObjectType::region, FilterFlags(window->var_858));

        ObjectManager::freeTemporaryObject();

        auto objIndex = getFirstAvailableSelectedObject(window);
        if (objIndex.index != ObjectManager::kNullObjectIndex)
        {
            window->rowHover = objIndex.index;
            window->object = reinterpret_cast<std::byte*>(&objIndex.object._header);
            ObjectManager::loadTemporaryObject(objIndex.object._header);
        }

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_0C);

        inputSession = Ui::TextInput::InputSession();
        inputSession.calculateTextOffset(widgets[widx::textInput].width());

        return window;
    }

    static void switchPrimaryTab(Window& self, uint8_t tabIndex);
    static void switchTabByObjectType(Window& self, ObjectType objectType);

    Window& openInTab(ObjectType objectType)
    {
        auto& window = *open();
        window.filterLevel = enumValue(FilterLevel::advanced);
        assignTabPositions(&window);
        switchTabByObjectType(window, objectType);
        return window;
    }

    // 0x004733AC
    static void prepareDraw(Ui::Window& self)
    {
        self.activatedWidgets |= (1 << widx::objectImage);
        self.widgets[widx::closeButton].type = WidgetType::buttonWithImage;

        if (isEditorMode())
        {
            self.widgets[widx::closeButton].type = WidgetType::none;
        }

        const auto& currentTab = kMainTabInfo[self.currentTab];
        const auto& subTabs = currentTab.subTabs;
        const bool showSecondaryTabs = !subTabs.empty() && FilterLevel(self.filterLevel) != FilterLevel::beginner;

        // Update page title
        auto args = FormatArguments(self.widgets[widx::caption].textArgs);
        if (showSecondaryTabs)
        {
            args.push(subTabs[self.currentSecondaryTab].name);
        }
        else
        {
            args.push(kMainTabInfo[self.currentTab].name);
        }

        // Toggle secondary tabs
        for (auto i = 0U; i < kMaxNumSecondaryTabs; i++)
        {
            const auto widgetIndex = i + widx::secondaryTab1;

            const bool subTabIsVisible = showSecondaryTabs && i < subTabs.size() && shouldShowSubTab(subTabs, i, FilterLevel(self.filterLevel));
            if (!subTabIsVisible)
            {
                self.disabledWidgets |= 1ULL << widgetIndex;
            }
            else
            {
                self.disabledWidgets &= ~(1ULL << widgetIndex);
            }

            if (subTabIsVisible)
            {
                self.enabledWidgets |= 1ULL << widgetIndex;
            }
            else
            {
                self.enabledWidgets &= ~(1ULL << widgetIndex);
            }

            if (self.currentSecondaryTab == i)
            {
                self.activatedWidgets |= 1ULL << widgetIndex;
            }
            else
            {
                self.activatedWidgets &= ~(1ULL << widgetIndex);
            }
        }

        Widget::leftAlignTabs(self, widx::secondaryTab1, widx::secondaryTab8, 30);

        if (showSecondaryTabs)
        {
            self.widgets[widx::scrollview].top = 62 + 28;
            self.widgets[widx::scrollviewFrame].type = WidgetType::panel;
            self.widgets[widx::scrollviewFrame].top = self.widgets[widx::scrollview].top - 2;
        }
        else
        {
            self.widgets[widx::scrollview].top = 62;
            self.widgets[widx::scrollviewFrame].type = WidgetType::none;
        }
    }

    static loco_global<uint16_t[kMaxObjectTypes], 0x0112C1C5> _112C1C5;
    static loco_global<uint32_t, 0x0112C209> _112C209;

    // 0x0047328D
    static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx)
    {
        auto yPos = self->y + self->widgets[widx::tabArea].top;
        auto xPos = self->x + 3;

        for (auto index : _tabPositions)
        {
            auto image = Gfx::recolour(ImageIds::tab, self->getColour(WindowColour::secondary).c());
            if (self->currentTab == index)
            {
                image = Gfx::recolour(ImageIds::selected_tab, self->getColour(WindowColour::secondary).c());
                drawingCtx.drawImage(xPos, yPos, image);

                image = Gfx::recolour(kMainTabInfo[index].image, Colour::mutedSeaGreen);
                drawingCtx.drawImage(xPos, yPos, image);
            }
            else
            {
                drawingCtx.drawImage(xPos, yPos, image);

                image = Gfx::recolour(kMainTabInfo[index].image, Colour::mutedSeaGreen);
                drawingCtx.drawImage(xPos, yPos, image);

                image = Gfx::recolourTranslucent(ImageIds::tab, ExtColour::unk33);
                drawingCtx.drawImage(xPos, yPos, image);
            }
            xPos += 31;
        }
    }

    static void drawSecondaryTabs(Window* self, Gfx::DrawingContext& drawingCtx)
    {
        const auto& currentTab = kMainTabInfo[self->currentTab];
        const auto& subTabs = currentTab.subTabs;
        const bool showSecondaryTabs = !subTabs.empty();
        if (!showSecondaryTabs)
        {
            return;
        }

        auto skin = ObjectManager::get<InterfaceSkinObject>();

        for (auto i = 0U; i < subTabs.size(); i++)
        {
            auto widgetIndex = i + widx::secondaryTab1;
            if (self->widgets[widgetIndex].type == WidgetType::none)
            {
                continue;
            }

            auto& tabData = subTabs[i];
            auto frame = 0;
            if (self->currentSecondaryTab == i)
            {
                frame = (self->frameNo >> tabData.animationDivisor) % tabData.animationLength;
            }

            auto baseImage = currentTab.objectType == ObjectType::vehicle ? skin->img : 0;
            auto image = Gfx::recolour(baseImage + tabData.baseImage + frame, CompanyManager::getCompanyColour(CompanyId::neutral));
            Widget::drawTab(self, drawingCtx, image, widgetIndex);
        }
    }

    static constexpr Ui::Point kObjectPreviewOffset = { 56, 56 };
    static constexpr Ui::Size kObjectPreviewSize = { 112, 112 };
    static constexpr uint8_t kDescriptionRowHeight = 10;

    template<typename T>
    static void callDrawPreviewImage(Gfx::DrawingContext& drawingCtx, const Ui::Point& drawingOffset, const Object& objectPtr)
    {
        auto object = reinterpret_cast<const T*>(&objectPtr);
        object->drawPreviewImage(drawingCtx, drawingOffset.x, drawingOffset.y);
    }

    // 0x00473579
    static void drawPreviewImage(const ObjectHeader& header, Gfx::DrawingContext& drawingCtx, int16_t x, int16_t y, const Object& objectPtr)
    {
        auto type = header.getType();

        // Clip the draw area to simplify image draw
        Ui::Point drawAreaPos = Ui::Point{ x, y } - kObjectPreviewOffset;
        const auto& rt = drawingCtx.currentRenderTarget();
        auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(drawAreaPos.x, drawAreaPos.y, kObjectPreviewSize.width, kObjectPreviewSize.height));
        if (!clipped)
        {
            return;
        }

        drawingCtx.pushRenderTarget(*clipped);

        switch (type)
        {
            case ObjectType::interfaceSkin:
                callDrawPreviewImage<InterfaceSkinObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::currency:
                callDrawPreviewImage<CurrencyObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::cliffEdge:
                callDrawPreviewImage<CliffEdgeObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::water:
                callDrawPreviewImage<WaterObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::land:
                callDrawPreviewImage<LandObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::wall:
                callDrawPreviewImage<WallObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trackSignal:
                callDrawPreviewImage<TrainSignalObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::levelCrossing:
                callDrawPreviewImage<LevelCrossingObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::streetLight:
                callDrawPreviewImage<StreetLightObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::tunnel:
                callDrawPreviewImage<TunnelObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::bridge:
                callDrawPreviewImage<BridgeObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trainStation:
                callDrawPreviewImage<TrainStationObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::trackExtra:
                callDrawPreviewImage<TrackExtraObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::track:
                callDrawPreviewImage<TrackObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::roadStation:
                callDrawPreviewImage<RoadStationObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::roadExtra:
                callDrawPreviewImage<RoadExtraObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::road:
                callDrawPreviewImage<RoadObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::airport:
                callDrawPreviewImage<AirportObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::dock:
                callDrawPreviewImage<DockObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::vehicle:
                callDrawPreviewImage<VehicleObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::tree:
                callDrawPreviewImage<TreeObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::snow:
                callDrawPreviewImage<SnowObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::hillShapes:
                callDrawPreviewImage<HillShapesObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::building:
                callDrawPreviewImage<BuildingObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::scaffolding:
                callDrawPreviewImage<ScaffoldingObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::industry:
                callDrawPreviewImage<IndustryObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::region:
                callDrawPreviewImage<RegionObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            case ObjectType::competitor:
                callDrawPreviewImage<CompetitorObject>(drawingCtx, kObjectPreviewOffset, objectPtr);
                break;

            default:
                // null
                break;
        }

        drawingCtx.popRenderTarget();
    }

    template<typename T>
    static void callDrawDescription(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y, const int16_t width, const Object& objectPtr)
    {
        auto object = reinterpret_cast<const T*>(&objectPtr);
        object->drawDescription(drawingCtx, x, y, width);
    }

    static void drawDescription(const ObjectHeader& header, Window* self, Gfx::DrawingContext& drawingCtx, int16_t x, int16_t y, Object& objectPtr)
    {
        int16_t width = self->x + self->width - x;
        int16_t height = self->y + self->height - y;

        // Clip the draw area to simplify image draw
        const auto& rt = drawingCtx.currentRenderTarget();
        auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(x, y, width, height));
        if (!clipped)
        {
            return;
        }

        drawingCtx.pushRenderTarget(*clipped);

        switch (header.getType())
        {
            case ObjectType::levelCrossing:
                callDrawDescription<LevelCrossingObject>(drawingCtx, 0, 0, width, objectPtr);
                break;

            case ObjectType::trainStation:
                callDrawDescription<TrainStationObject>(drawingCtx, 0, 0, width, objectPtr);
                break;

            case ObjectType::roadStation:
                callDrawDescription<RoadStationObject>(drawingCtx, 0, 0, width, objectPtr);
                break;

            case ObjectType::airport:
                callDrawDescription<AirportObject>(drawingCtx, 0, 0, width, objectPtr);
                break;

            case ObjectType::dock:
                callDrawDescription<DockObject>(drawingCtx, 0, 0, width, objectPtr);
                break;

            case ObjectType::vehicle:
                callDrawDescription<VehicleObject>(drawingCtx, 0, 0, width, objectPtr);
                break;

            case ObjectType::building:
                callDrawDescription<BuildingObject>(drawingCtx, 0, 0, width, objectPtr);
                break;

            case ObjectType::competitor:
                callDrawDescription<CompetitorObject>(drawingCtx, 0, 0, width, objectPtr);
                break;

            default:
                // null
                break;
        }

        drawingCtx.popRenderTarget();
    }

    static void drawDatDetails(const ObjectManager::ObjectIndexEntry& indexEntry, Window* self, Gfx::DrawingContext& drawingCtx, int16_t x, int16_t y)
    {
        int16_t width = self->x + self->width - x;
        int16_t height = self->y + self->height - y;

        // Clip the draw area to simplify image draw
        const auto& rt = drawingCtx.currentRenderTarget();
        auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(x, y, width, height));
        if (!clipped)
        {
            return;
        }

        drawingCtx.pushRenderTarget(*clipped);

        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw object filename
        {
            auto filename = fs::u8path(indexEntry._filepath).filename().u8string();
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
            strncpy(buffer, filename.c_str(), filename.length() + 1);

            FormatArguments args{};
            args.push<StringId>(StringIds::buffer_1250);

            auto point = Point(18, height - kDescriptionRowHeight * 3 - 4);
            tr.drawStringLeft(point, Colour::black, StringIds::object_selection_filename, args);
        }

        drawingCtx.popRenderTarget();
    }

    static void drawSearchBox(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        char* textBuffer = (char*)StringManager::getString(StringIds::buffer_2039);
        strncpy(textBuffer, inputSession.buffer.c_str(), 256);

        auto& widget = widgets[widx::textInput];
        const auto& rt = drawingCtx.currentRenderTarget();
        auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(widget.left + 1 + self.x, widget.top + 1 + self.y, widget.width() - 2, widget.height() - 2));
        if (!clipped)
        {
            return;
        }

        drawingCtx.pushRenderTarget(*clipped);

        FormatArguments args{};
        args.push(StringIds::buffer_2039);

        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw search box input buffer
        Ui::Point position = { inputSession.xOffset, 1 };
        tr.drawStringLeft(position, Colour::black, StringIds::black_stringid, args);

        // Draw search box cursor, blinking
        if (Input::isFocused(self.type, self.number, widx::textInput) && (inputSession.cursorFrame % 32) < 16)
        {
            // We draw the string again to figure out where the cursor should go; position.x will be adjusted
            textBuffer[inputSession.cursorPosition] = '\0';
            position = { inputSession.xOffset, 1 };
            position = tr.drawStringLeft(position, Colour::black, StringIds::black_stringid, args);
            drawingCtx.fillRect(position.x, position.y, position.x, position.y + 9, Colours::getShade(self.getColour(WindowColour::secondary).c(), 9), Gfx::RectFlags::none);
        }

        drawingCtx.popRenderTarget();
    }

    // 0x004733F5
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        drawingCtx.fillRectInset(self.x, self.y + 20, self.x + self.width - 1, self.y + 20 + 60, self.getColour(WindowColour::primary), Gfx::RectInsetFlags::none);
        self.draw(drawingCtx);

        drawTabs(&self, drawingCtx);
        drawSecondaryTabs(&self, drawingCtx);
        drawSearchBox(self, drawingCtx);

        {
            static constexpr std::array<StringId, 3> levelStringIds = {
                StringIds::objSelectionFilterBeginner,
                StringIds::objSelectionFilterAdvanced,
                StringIds::objSelectionFilterExpert,
            };

            FormatArguments args{};
            args.push(levelStringIds[self.filterLevel]);

            auto& widget = self.widgets[widx::filterLabel];
            auto point = Point(self.x + widget.left, self.y + widget.top);

            // Draw current level on combobox
            tr.drawStringLeftClipped(point, widget.width() - 15, Colour::black, StringIds::wcolour2_stringid, args);
        }

        bool doDefault = true;
        if (self.object != nullptr)
        {
            auto& objectHeader = ObjectManager::getObjectInIndex(self.rowHover)._header;
            if (objectHeader.getType() != ObjectType::townNames && objectHeader.getType() != ObjectType::climate)
            {
                doDefault = false;
            }
        }

        if (doDefault)
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 5);
            drawingCtx.drawRect(self.x + widget.left, self.y + widget.top, widget.width(), widget.height(), colour, Gfx::RectFlags::none);
        }
        else
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 0);
            drawingCtx.drawRect(self.x + widget.left + 1, self.y + widget.top + 1, widget.width() - 2, widget.height() - 2, colour, Gfx::RectFlags::none);
        }

        ObjectType type{};
        auto& currentTab = kMainTabInfo[self.currentTab];
        if (!currentTab.subTabs.empty())
        {
            type = currentTab.subTabs[self.currentSecondaryTab].objectType;
        }
        else
        {
            type = currentTab.objectType;
        }

        auto args = FormatArguments();
        args.push(_112C1C5[enumValue(type)]);
        args.push(ObjectManager::getMaxObjects(type));

        {
            auto point = Point(self.x + 3, self.y + self.height - 12);
            tr.drawStringLeft(point, Colour::black, StringIds::num_selected_num_max, args);
        }

        if (self.rowHover == -1)
        {
            return;
        }

        auto* temporaryObject = ObjectManager::getTemporaryObject();

        if (temporaryObject == nullptr)
        {
            return;
        }

        {
            auto& objectHeader = ObjectManager::getObjectInIndex(self.rowHover)._header;

            drawPreviewImage(
                objectHeader,
                drawingCtx,
                widgets[widx::objectImage].midX() + 1 + self.x,
                widgets[widx::objectImage].midY() + 1 + self.y,
                *temporaryObject);
        }

        auto x = self.widgets[widx::objectImage].midX() + self.x;
        auto y = self.widgets[widx::objectImage].bottom + 3 + self.y;
        auto width = self.width - self.widgets[widx::scrollview].right - 6;

        {
            auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

            *buffer++ = ControlCodes::windowColour2;

            strncpy(buffer, ObjectManager::getObjectInIndex(self.rowHover)._name.c_str(), 510);

            auto point = Point(x, y);
            tr.drawStringCentredClipped(point, width, Colour::black, StringIds::buffer_2039);
        }

        {
            auto& objHeader = ObjectManager::getObjectInIndex(self.rowHover)._header;

            drawDescription(
                objHeader,
                &self,
                drawingCtx,
                self.widgets[widx::scrollview].right + self.x + 4,
                y + kDescriptionRowHeight,
                *temporaryObject);
        }

        {
            drawDatDetails(
                ObjectManager::getObjectInIndex(self.rowHover),
                &self,
                drawingCtx,
                self.widgets[widx::scrollview].right + self.x + 4,
                y + kDescriptionRowHeight);
        }
    }

    // 0x0047361D
    static void drawScroll(Window& self, Gfx::DrawingContext& drawingCtx, const uint32_t)
    {
        const auto& rt = drawingCtx.currentRenderTarget();

        auto tr = Gfx::TextRenderer(drawingCtx);

        drawingCtx.clearSingle(Colours::getShade(self.getColour(WindowColour::secondary).c(), 4));

        if (ObjectManager::getNumInstalledObjects() == 0)
        {
            return;
        }

        const auto selectionFlags = getSelectedObjectFlags();
        int y = 0;
        for (auto& entry : _tabObjectList)
        {
            if (entry.display == Visibility::hidden)
            {
                continue;
            }

            if (y + kRowHeight < rt.y)
            {
                y += kRowHeight;
                continue;
            }
            else if (y > rt.y + rt.height)
            {
                break;
            }

            Gfx::RectInsetFlags flags = Gfx::RectInsetFlags::colourLight | Gfx::RectInsetFlags::fillDarker | Gfx::RectInsetFlags::borderInset;
            drawingCtx.fillRectInset(2, y, 11, y + 10, self.getColour(WindowColour::secondary), flags);

            uint8_t textColour = ControlCodes::Colour::black;

            auto objectPtr = self.object;
            if (objectPtr != nullptr)
            {
                auto& hoverObject = ObjectManager::getObjectInIndex(self.rowHover)._header;
                if (entry.object._header == hoverObject)
                {
                    drawingCtx.fillRect(0, y, self.width, y + kRowHeight - 1, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                    textColour = ControlCodes::windowColour2;
                }
            }

            using namespace ObjectManager;

            if ((selectionFlags[entry.index] & SelectedObjectsFlags::selected) != SelectedObjectsFlags::none)
            {
                auto x = 2;
                tr.setCurrentFont(Gfx::Font::m2);

                if (textColour != ControlCodes::windowColour2)
                {
                    tr.setCurrentFont(Gfx::Font::m1);
                }

                auto checkColour = self.getColour(WindowColour::secondary).opaque();

                if ((selectionFlags[entry.index] & (SelectedObjectsFlags::inUse | SelectedObjectsFlags::requiredByAnother | SelectedObjectsFlags::alwaysRequired)) != ObjectManager::SelectedObjectsFlags::none)
                {
                    checkColour = checkColour.inset();
                }

                static constexpr char strCheckmark[] = "\xAC";
                auto point = Point(x, y);
                tr.drawString(point, checkColour, strCheckmark);
            }

            char buffer[512]{};
            buffer[0] = textColour;
            strncpy(&buffer[1], entry.object._name.c_str(), 510);
            tr.setCurrentFont(Gfx::Font::medium_bold);

            auto point = Point(15, y);
            tr.drawString(point, Colour::black, buffer);
            y += kRowHeight;
        }
    }

    static void switchTabByObjectType(Window& self, ObjectType objectType)
    {
        auto targetTab = 0;
        auto targetSubTab = 0;
        auto targetType = kMainTabInfo[_tabPositions[0]].objectType;

        for (auto i = 0U; i < std::size(_tabPositions); i++)
        {
            auto mainIndex = _tabPositions[i];
            auto& mainTabInfo = kMainTabInfo[mainIndex];

            if (objectType == mainTabInfo.objectType)
            {
                targetTab = i;
                targetType = objectType;
                break;
            }

            auto& subTabs = mainTabInfo.subTabs;
            if (subTabs.empty())
            {
                continue;
            }

            for (auto j = 0U; j < subTabs.size(); j++)
            {
                if (!shouldShowTab(subTabs[j], FilterLevel(self.var_858)))
                {
                    continue;
                }

                if (objectType == subTabs[j].objectType)
                {
                    targetTab = i;
                    targetSubTab = j;
                    targetType = objectType;
                    break;
                }
            }

            if (targetSubTab != 0)
            {
                break;
            }
        }

        self.currentTab = targetTab;
        self.currentSecondaryTab = targetSubTab;

        populateTabObjectList(targetType, FilterFlags(self.var_858));
    }

    // 0x00473A13
    bool tryCloseWindow()
    {
        const auto res = ObjectManager::validateObjectSelection(getSelectedObjectFlags());
        if (!res.has_value())
        {
            // All okay selection is good!
            auto* w = WindowManager::find(WindowType::objectSelection);
            if (w != nullptr)
            {
                WindowManager::close(w);
            }
            return true;
        }
        else
        {
            // Selection was bad so throw up an error message
            // and switch tabs to the bad type

            Windows::Error::open(StringIds::invalid_selection_of_objects, GameCommands::getErrorText());

            auto* w = WindowManager::find(WindowType::objectSelection);
            if (w != nullptr)
            {
                // TODO: switch modes as needed?
                auto objectType = res.value();
                switchTabByObjectType(*w, objectType);

                w->rowHover = -1;
                w->object = nullptr;
                w->scrollAreas[0].contentWidth = 0;
                ObjectManager::freeTemporaryObject();
                w->invalidate();

                auto objIndex = getFirstAvailableSelectedObject(w);
                if (objIndex.index != ObjectManager::kNullObjectIndex)
                {
                    w->rowHover = objIndex.index;
                    w->object = reinterpret_cast<std::byte*>(&objIndex.object._header);

                    ObjectManager::loadTemporaryObject(objIndex.object._header);
                }
            }
            return false;
        }
    }

    static void switchPrimaryTab(Window& self, uint8_t tabIndex)
    {
        self.currentTab = tabIndex;
        self.currentSecondaryTab = 0;

        const auto& currentTab = kMainTabInfo[self.currentTab];
        _filterByVehicleType = currentTab.objectType == ObjectType::vehicle;
        _currentVehicleType = VehicleType::train;

        auto objectType = kMainTabInfo[tabIndex].objectType;
        populateTabObjectList(objectType, FilterFlags(self.var_858));

        self.rowHover = -1;
        self.object = nullptr;
        self.scrollAreas[0].contentWidth = 0;
        ObjectManager::freeTemporaryObject();
        auto objIndex = getFirstAvailableSelectedObject(&self);

        if (objIndex.index != ObjectManager::kNullObjectIndex)
        {
            self.rowHover = objIndex.index;
            self.object = reinterpret_cast<std::byte*>(&objIndex.object._header);

            ObjectManager::loadTemporaryObject(objIndex.object._header);
        }

        applyFilterToObjectList(FilterFlags(self.var_858));
        self.initScrollWidgets();
        self.invalidate();
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
                int yPos = self.y + widgets[widx::tabArea].top;
                int xPos = self.x + 3;
                auto mousePos = Input::getCursorPressedLocation();

                for (auto index : _tabPositions)
                {
                    if (mousePos.x >= xPos && mousePos.y >= yPos)
                    {
                        if (mousePos.x < xPos + 31 && yPos + 27 > mousePos.y)
                        {
                            clickedTab = index;
                            break;
                        }
                    }

                    xPos += 31;
                }

                if (clickedTab != -1 && self.currentTab != clickedTab)
                {
                    switchPrimaryTab(self, clickedTab);
                }

                break;
            }

            case widx::clearButton:
            {
                inputSession.clearInput();
                applyFilterToObjectList(FilterFlags(self.var_858));
                self.initScrollWidgets();
                self.invalidate();
                break;
            }

            case widx::secondaryTab1:
            case widx::secondaryTab2:
            case widx::secondaryTab3:
            case widx::secondaryTab4:
            case widx::secondaryTab5:
            case widx::secondaryTab6:
            case widx::secondaryTab7:
            case widx::secondaryTab8:
            {
                auto& subTabs = kMainTabInfo[self.currentTab].subTabs;
                auto previousSubType = subTabs[self.currentSecondaryTab].objectType;

                self.currentSecondaryTab = w - widx::secondaryTab1;
                auto currentSubType = subTabs[self.currentSecondaryTab].objectType;
                _currentVehicleType = static_cast<VehicleType>(self.currentSecondaryTab);

                // Do we need to reload the object list?
                auto flags = FilterFlags(self.var_858);
                if (previousSubType != currentSubType)
                {
                    populateTabObjectList(subTabs[self.currentSecondaryTab].objectType, flags);
                }

                applyFilterToObjectList(flags);
                self.initScrollWidgets();
                self.invalidate();
            }
        }
    }

    static void onMouseDown(Window& self, const WidgetIndex_t widgetIndex)
    {
        if (widgetIndex == widx::filterDropdown)
        {
            auto& dropdown = self.widgets[widx::filterLabel];
            Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 7, 0);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::objSelectionFilterBeginner);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::objSelectionFilterAdvanced);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::objSelectionFilterExpert);
            Dropdown::add(3, 0);
            Dropdown::add(4, StringIds::dropdown_without_checkmark, StringIds::objSelectionFilterVanilla);
            Dropdown::add(5, StringIds::dropdown_without_checkmark, StringIds::objSelectionFilterOpenLoco);
            Dropdown::add(6, StringIds::dropdown_without_checkmark, StringIds::objSelectionFilterCustom);

            // Mark current level
            Dropdown::setItemSelected(self.filterLevel);

            // Show vanilla objects?
            if ((FilterFlags(self.var_858) & FilterFlags::vanilla) != FilterFlags::none)
            {
                Dropdown::setItemSelected(4);
            }

            // Show OpenLoco objects?
            if ((FilterFlags(self.var_858) & FilterFlags::openLoco) != FilterFlags::none)
            {
                Dropdown::setItemSelected(5);
            }

            // Show custom objects?
            if ((FilterFlags(self.var_858) & FilterFlags::custom) != FilterFlags::none)
            {
                Dropdown::setItemSelected(6);
            }
        }
    }

    static void onDropdown(Window& self, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex != widx::filterDropdown)
        {
            return;
        }

        if (itemIndex < 0)
        {
            self.filterLevel = (self.filterLevel ^ 1) % 3;
            assignTabPositions(&self);
        }

        // Switch level?
        else if (itemIndex >= 0 && itemIndex <= 2)
        {
            // Keep track of currently selected object type
            auto& currentTab = kMainTabInfo[self.currentTab];
            auto currentObjectType = currentTab.objectType;
            if (!currentTab.subTabs.empty())
            {
                auto& currentSubType = currentTab.subTabs[self.currentSecondaryTab];
                currentObjectType = currentSubType.objectType;
            }

            self.filterLevel = itemIndex;
            assignTabPositions(&self);

            // Switch back to previously selected object type, if possible
            switchTabByObjectType(self, currentObjectType);
        }
        else if (itemIndex == 4) // Toggle vanilla objects
        {
            self.var_858 = enumValue(FilterFlags(self.var_858) ^ FilterFlags::vanilla);
            auto objectType = kMainTabInfo[self.currentTab].objectType;
            populateTabObjectList(objectType, FilterFlags(self.var_858));
        }
        else if (itemIndex == 5) // Toggle OpenLoco objects
        {
            self.var_858 = enumValue(FilterFlags(self.var_858) ^ FilterFlags::openLoco);
            auto objectType = kMainTabInfo[self.currentTab].objectType;
            populateTabObjectList(objectType, FilterFlags(self.var_858));
        }
        else if (itemIndex == 6) // Toggle custom objects
        {
            self.var_858 = enumValue(FilterFlags(self.var_858) ^ FilterFlags::custom);
            auto objectType = kMainTabInfo[self.currentTab].objectType;
            populateTabObjectList(objectType, FilterFlags(self.var_858));
        }

        self.invalidate();
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
            {
                continue;
            }

            y -= kRowHeight;
            if (y < 0)
            {
                return { static_cast<int16_t>(entry.index), entry.object };
            }
        }

        return { ObjectManager::kNullObjectIndex, ObjectManager::ObjectIndexEntry{} };
    }

    // 0x0047390A
    static void onScrollMouseOver(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(&self, y);

        if (objIndex.index == self.rowHover || objIndex.index == ObjectManager::kNullObjectIndex)
        {
            return;
        }

        self.rowHover = objIndex.index;
        self.object = reinterpret_cast<std::byte*>(&objIndex.object._header);
        ObjectManager::freeTemporaryObject();

        if (objIndex.index != ObjectManager::kNullObjectIndex)
        {
            ObjectManager::loadTemporaryObject(objIndex.object._header);
        }

        self.invalidate();
    }

    // 0x00473948
    static void onScrollMouseDown(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(&self, y);
        auto index = objIndex.index;
        auto object = objIndex.object._header;

        if (index == ObjectManager::kNullObjectIndex)
        {
            return;
        }

        self.invalidate();
        Audio::playSound(Audio::SoundId::clickDown, Input::getMouseLocation().x);

        auto type = objIndex.object._header.getType();

        const auto selectionFlags = getSelectedObjectFlags();
        if (ObjectManager::getMaxObjects(type) == 1)
        {
            if ((selectionFlags[index] & ObjectManager::SelectedObjectsFlags::selected) == ObjectManager::SelectedObjectsFlags::none)
            {
                auto [oldIndex, oldObject] = ObjectManager::getActiveObject(type, selectionFlags);

                if (oldIndex != ObjectManager ::kNullObjectIndex)
                {
                    ObjectManager::ObjectSelectionMeta meta{};
                    std::copy(std::begin(_112C1C5), std::end(_112C1C5), std::begin(meta.numSelectedObjects));
                    meta.numImages = _112C209;
                    ObjectManager::selectObjectFromIndex(ObjectManager::SelectObjectModes::defaultDeselect, oldObject._header, selectionFlags, meta);
                    std::copy(std::begin(meta.numSelectedObjects), std::end(meta.numSelectedObjects), std::begin(_112C1C5));
                    _112C209 = meta.numImages;
                }
            }
        }

        auto mode = ObjectManager::SelectObjectModes::defaultDeselect;

        if ((selectionFlags[index] & ObjectManager::SelectedObjectsFlags::selected) == ObjectManager::SelectedObjectsFlags::none)
        {
            mode = ObjectManager::SelectObjectModes::defaultSelect;
        }

        ObjectManager::ObjectSelectionMeta meta{};
        std::copy(std::begin(_112C1C5), std::end(_112C1C5), std::begin(meta.numSelectedObjects));
        meta.numImages = _112C209;
        bool success = ObjectManager::selectObjectFromIndex(mode, object, selectionFlags, meta);
        std::copy(std::begin(meta.numSelectedObjects), std::end(meta.numSelectedObjects), std::begin(_112C1C5));
        _112C209 = meta.numImages;

        if (success)
        {
            return;
        }

        auto errorTitle = StringIds::error_unable_to_select_object;

        if ((mode & ObjectManager::SelectObjectModes::select) == ObjectManager::SelectObjectModes::none)
        {
            errorTitle = StringIds::error_unable_to_deselect_object;
        }

        Ui::Windows::Error::open(errorTitle, GameCommands::getErrorText());
    }

    // 0x004739DD
    static void onClose([[maybe_unused]] Window& self)
    {
        ObjectManager::unloadUnselectedSelectionListObjects(getSelectedObjectFlags());
        ObjectManager::loadSelectionListObjects(getSelectedObjectFlags());
        ObjectManager::reloadAll();
        ObjectManager::freeTemporaryObject();

        if (!isEditorMode())
        {
            // Make new selection available in-game.
            ObjectManager::updateYearly2();
            ObjectManager::sub_4748FA();
            Gfx::loadCurrency();
            Gfx::loadDefaultPalette();
            Gfx::invalidateScreen();
        }
        ObjectManager::freeSelectionList();
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

    static bool keyUp(Window& w, uint32_t charCode, uint32_t keyCode)
    {
        if (!Input::isFocused(w.type, w.number, widx::textInput))
        {
            return false;
        }

        if (!inputSession.handleInput(charCode, keyCode))
        {
            return false;
        }

        int containerWidth = widgets[widx::textInput].width() - 2;
        if (inputSession.needsReoffsetting(containerWidth))
        {
            inputSession.calculateTextOffset(containerWidth);
        }

        inputSession.cursorFrame = 0;

        applyFilterToObjectList(FilterFlags(w.var_858));

        w.initScrollWidgets();
        w.invalidate();
        return true;
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onMouseUp = onMouseUp,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = onScrollMouseDown,
        .scrollMouseOver = onScrollMouseOver,
        .tooltip = tooltip,
        .prepareDraw = prepareDraw,
        .draw = draw,
        .drawScroll = drawScroll,
        .keyUp = keyUp,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
