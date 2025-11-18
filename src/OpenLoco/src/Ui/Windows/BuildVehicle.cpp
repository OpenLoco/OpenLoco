#include "Config.h"
#include "Date.h"
#include "Economy/Economy.h"
#include "Entities/EntityManager.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/Vehicles/CreateVehicle.h"
#include "GameState.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Objects/CargoObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Objects/RoadExtraObject.h"
#include "Objects/RoadObject.h"
#include "Objects/TrackExtraObject.h"
#include "Objects/TrackObject.h"
#include "Objects/VehicleObject.h"
#include "OpenLoco.h"
#include "Ui/Dropdown.h"
#include "Ui/ScrollView.h"
#include "Ui/TextInput.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/DropdownWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/Widgets/TextBoxWidget.h"
#include "Ui/WindowManager.h"
#include "Vehicles/Vehicle.h"
#include "Vehicles/VehicleDraw.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Math/Trigonometry.hpp>
#include <algorithm>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::BuildVehicle
{
    static constexpr Ui::Size32 kWindowSize = { 400, 305 };

    enum widx
    {
        frame = 0,
        caption = 1,
        close_button = 2,
        panel = 3,
        tab_build_new_trains,
        tab_build_new_buses,
        tab_build_new_trucks,
        tab_build_new_trams,
        tab_build_new_aircraft,
        tab_build_new_ships,
        tab_track_type_0,
        tab_track_type_1,
        tab_track_type_2,
        tab_track_type_3,
        tab_track_type_4,
        tab_track_type_5,
        tab_track_type_6,
        tab_track_type_7,
        scrollview_vehicle_selection,
        scrollview_vehicle_preview,
        searchBox,
        searchClearButton,
        filterLabel,
        filterDropdown,
        cargoLabel,
        cargoDropdown
    };

    enum scrollIdx
    {
        vehicle_selection,
        vehicle_preview
    };

    static constexpr uint32_t kTrainTabImages[16]{
        InterfaceSkin::ImageIds::build_vehicle_train_frame_0,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_1,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_2,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_3,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_4,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_5,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_6,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_7,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_8,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_9,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_10,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_11,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_12,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_13,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_14,
        InterfaceSkin::ImageIds::build_vehicle_train_frame_15,
    };

    static constexpr uint32_t kAircraftTabImages[16]{
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_0,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_1,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_2,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_3,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_4,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_5,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_6,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_7,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_8,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_9,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_10,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_11,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_12,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_13,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_14,
        InterfaceSkin::ImageIds::build_vehicle_aircraft_frame_15,
    };

    static constexpr uint32_t kBusTabImages[16]{
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_0,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_1,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_2,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_3,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_4,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_5,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_6,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_7,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_8,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_9,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_10,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_11,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_12,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_13,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_14,
        InterfaceSkin::ImageIds::build_vehicle_bus_frame_15,
    };

    static constexpr uint32_t kTramTabImages[16]{
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_0,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_1,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_2,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_3,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_4,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_5,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_6,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_7,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_8,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_9,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_10,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_11,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_12,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_13,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_14,
        InterfaceSkin::ImageIds::build_vehicle_tram_frame_15,
    };

    static constexpr uint32_t kTruckTabImages[16]{
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_0,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_1,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_2,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_3,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_4,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_5,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_6,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_7,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_8,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_9,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_10,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_11,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_12,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_13,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_14,
        InterfaceSkin::ImageIds::build_vehicle_truck_frame_15,
    };

    static constexpr uint32_t kShipTabImages[16]{
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_0,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_1,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_2,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_3,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_4,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_5,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_6,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_7,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_8,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_9,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_10,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_11,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_12,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_13,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_14,
        InterfaceSkin::ImageIds::build_vehicle_ship_frame_15,
    };

    struct TabDetails
    {
        VehicleType type;
        widx widgetIndex;
        const uint32_t* imageIds;
    };

    static TabDetails _transportTypeTabInformation[] = {
        { VehicleType::train, tab_build_new_trains, kTrainTabImages },
        { VehicleType::bus, tab_build_new_buses, kBusTabImages },
        { VehicleType::truck, tab_build_new_trucks, kTruckTabImages },
        { VehicleType::tram, tab_build_new_trams, kTramTabImages },
        { VehicleType::aircraft, tab_build_new_aircraft, kAircraftTabImages },
        { VehicleType::ship, tab_build_new_ships, kShipTabImages }
    };

    // 0x5231D0
    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { 380, 233 }, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { 378, 13 }, Widgets::Caption::Style::colourText, WindowColour::primary),
        Widgets::ImageButton({ 365, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 41 }, { 380, 192 }, WindowColour::secondary),

        // Primary tabs
        Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_train_vehicles),
        Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_buses),
        Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_trucks),
        Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_trams),
        Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_aircraft),
        Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_ships),

        // Secondary tabs
        Widgets::Tab({ 5, 43 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        Widgets::Tab({ 36, 43 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        Widgets::Tab({ 67, 43 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        Widgets::Tab({ 98, 43 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        Widgets::Tab({ 129, 43 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        Widgets::Tab({ 160, 43 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        Widgets::Tab({ 191, 43 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        Widgets::Tab({ 222, 43 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),

        // Scroll and preview areas
        Widgets::ScrollView({ 3, 102 }, { 374, 146 }, WindowColour::secondary, Scrollbars::vertical),
        Widgets::ScrollView({ 250, 44 }, { 180, 66 }, WindowColour::secondary, Scrollbars::none),

        // Filter options
        // NB: deliberately defined after scrollview definitions to keep enums the same as original
        // TODO: can be moved after drawVehicleOverview has been implemented
        Widgets::TextBox({ 4, 72 }, { 246, 14 }, WindowColour::secondary),
        Widgets::Button({ 50, 72 }, { 38, 14 }, WindowColour::secondary, StringIds::clearInput),
        Widgets::dropdownWidgets({ 3, 87 }, { 90, 12 }, WindowColour::secondary, StringIds::filterComponents),
        Widgets::dropdownWidgets({ 48, 87 }, { 90, 12 }, WindowColour::secondary, StringIds::filterCargoSupported)

    );

    static constexpr uint32_t widxToTrackTypeTab(WidgetIndex_t widgetIndex)
    {
        return widgetIndex - widx::tab_track_type_0;
    }

    enum class VehicleFilterFlags : uint8_t
    {
        none = 0,
        powered = 1 << 0,
        unpowered = 1 << 1,
        locked = 1 << 2,
        unlocked = 1 << 3,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(VehicleFilterFlags);

    constexpr VehicleFilterFlags kMaskPoweredUnpowered = VehicleFilterFlags::powered | VehicleFilterFlags::unpowered;
    constexpr VehicleFilterFlags kMaskLockedUnlocked = VehicleFilterFlags::locked | VehicleFilterFlags::unlocked;

    enum class VehicleSortBy : uint8_t
    {
        designYear = 0,
        name = 1,
    };

    static bool _lastDisplayLockedVehiclesState;
    static uint16_t _lastRefreshYear;
    static VehicleFilterFlags _vehicleFilterFlags = kMaskPoweredUnpowered | kMaskLockedUnlocked;
    static VehicleSortBy _vehicleSortBy = VehicleSortBy::designYear;
    static uint8_t _cargoSupportedFilter = 0xFF;

    static uint32_t _numTrackTypeTabs;    // 0x011364EC
    static int16_t _numAvailableVehicles; // 0x01136268

    // Array of types if 0xFF then no type, flag (1<<7) as well
    static uint8_t _trackTypesForTab[widxToTrackTypeTab(widx::tab_track_type_7) + 1];      // 0x011364F0
    static uint16_t _availableVehicles[ObjectManager::getMaxObjects(ObjectType::vehicle)]; // 0x0113626A

    static int32_t _buildTargetVehicle; // 0x011364E8; -1 for no target VehicleHead

    static constexpr std::array<uint16_t, 6> kScrollRowHeight = { { 22, 22, 22, 22, 42, 30 } };

    static Ui::TextInput::InputSession inputSession;

    static void setDisabledTransportTabs(Ui::Window* window);
    static void setTrackTypeTabs(Ui::Window* window);
    static void resetTrackTypeTabSelection(Ui::Window* window);
    static void setTopToolbarLastTrack(uint8_t trackType, bool isRoad);
    static void drawTransportTypeTabs(Ui::Window& window, Gfx::DrawingContext& drawingCtx);
    static void drawTrackTypeTabs(Ui::Window& window, Gfx::DrawingContext& drawingCtx);

    static const WindowEventList& getEvents();

    // 0x4C1C64
    static Window* create(CompanyId company)
    {
        auto window = WindowManager::createWindow(WindowType::buildVehicle, kWindowSize, WindowFlags::flag_11, getEvents());
        window->setWidgets(_widgets);
        window->number = enumValue(company);
        window->owner = CompanyManager::getControllingId();
        window->frameNo = 0;
        auto skin = OpenLoco::ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::secondary, skin->windowPlayerColor);
        }
        setDisabledTransportTabs(window);
        return window;
    }

    // 0x004C1AF7
    static Window* open(uint32_t vehicleId, bool isTabId)
    {
        auto window = WindowManager::bringToFront(WindowType::buildVehicle, enumValue(CompanyManager::getControllingId()));
        if (window)
        {
            WidgetIndex_t tab = widx::tab_build_new_trains;
            if (!isTabId)
            {
                auto veh = EntityManager::get<Vehicles::VehicleHead>(EntityId(vehicleId));
                if (veh == nullptr)
                {
                    return nullptr;
                }
                tab += static_cast<uint8_t>(veh->vehicleType);
            }
            else
            {
                // Not a vehicle but a type
                tab += vehicleId;
            }
            window->callOnMouseUp(tab, window->widgets[tab].id);

            if (isTabId)
            {
                _buildTargetVehicle = -1;
            }
            else
            {
                _buildTargetVehicle = vehicleId;
            }
        }
        else
        {
            window = create(CompanyManager::getControllingId());
            window->width = kWindowSize.width;
            window->height = kWindowSize.height;
            _buildTargetVehicle = -1;
            if (!isTabId)
            {
                _buildTargetVehicle = vehicleId;
                auto veh = EntityManager::get<Vehicles::VehicleHead>(EntityId(vehicleId));
                if (veh == nullptr)
                {
                    WindowManager::close(window);
                    return nullptr;
                }
                window->currentTab = static_cast<uint8_t>(veh->vehicleType);
            }
            else
            {
                window->currentTab = vehicleId;
            }

            window->rowHeight = kScrollRowHeight[window->currentTab];
            window->rowCount = 0;
            window->var_83C = 0;
            window->rowHover = -1;
            window->invalidate();
            window->setWidgets(_widgets);
            window->holdableWidgets = 0;
            window->eventHandlers = &getEvents();
            window->activatedWidgets = 0;
            setDisabledTransportTabs(window);
            setTrackTypeTabs(window);
            resetTrackTypeTabSelection(window);
            sub_4B92A5(window);

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();

            inputSession = Ui::TextInput::InputSession();
            inputSession.calculateTextOffset(_widgets[widx::searchBox].width());
        }

        if (_buildTargetVehicle == -1)
        {
            return window;
        }
        auto veh = EntityManager::get<Vehicles::VehicleBase>(EntityId(_buildTargetVehicle));
        if (veh == nullptr)
        {
            return window;
        }
        auto targetTrackType = veh->getTrackType();
        if (veh->getTransportMode() != TransportMode::rail)
        {
            targetTrackType |= (1 << 7);
            if (targetTrackType == 0xFF)
            {
                targetTrackType = getGameState().lastTrackTypeOption;
            }
        }

        WidgetIndex_t widgetIndex = widx::tab_track_type_0;
        for (uint32_t trackTypeTab = 0; trackTypeTab < _numTrackTypeTabs; trackTypeTab++)
        {
            if (targetTrackType == _trackTypesForTab[trackTypeTab])
            {
                widgetIndex = widx::tab_track_type_0 + trackTypeTab;
                break;
            }
        }

        window->callOnMouseUp(widgetIndex, window->widgets[widgetIndex].id);
        return window;
    }

    Window* openByVehicleId(EntityId vehicleId)
    {
        return open(enumValue(vehicleId), false);
    }

    Window* openByType(VehicleType vehicleType)
    {
        return open(enumValue(vehicleType), true);
    }

    Window* openByVehicleObjectId(uint16_t vehicleObjectId)
    {
        auto* vehicleObj = ObjectManager::get<VehicleObject>(vehicleObjectId);

        auto window = openByType(vehicleObj->type);
        window->rowHover = vehicleObjectId;

        if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
        {
            if (vehicleObj->trackType != 0xFF)
            {
                for (uint8_t i = 0; i < _numTrackTypeTabs && i < std::size(_trackTypesForTab); ++i)
                {
                    if (vehicleObj->trackType == _trackTypesForTab[i])
                    {
                        window->currentSecondaryTab = i;
                        return window;
                    }
                }
            }
        }

        auto rowHover = window->rowHover;
        sub_4B92A5(window);
        window->rowHover = rowHover;

        return window;
    }

    static bool contains(const std::string_view& a, const std::string_view& b)
    {
        return std::search(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) {
                   return tolower(a) == tolower(b);
               })
            != a.end();
    }

    /* 0x4B9165
     * Works out which vehicles are able to be built for this vehicle_type or vehicle
     */
    static void generateBuildableVehiclesArray(VehicleType vehicleType, uint8_t trackType, Vehicles::VehicleBase* vehicle)
    {
        // Limit to available track types?
        if (trackType != 0xFF && (trackType & (1 << 7)))
        {
            auto trackIdx = trackType & ~(1 << 7);
            auto roadObj = ObjectManager::get<RoadObject>(trackIdx);
            if (roadObj->hasFlags(RoadObjectFlags::unk_03))
            {
                trackType = 0xFE;
            }
        }

        // Limit to what's available for a particular company?
        auto companyId = CompanyManager::getControllingId();
        if (vehicle != nullptr)
        {
            companyId = vehicle->owner;
        }

        struct BuildableVehicle
        {
            uint16_t vehicleIndex;
            StringId name;
            bool isPowered;
            uint16_t designed;
        };

        _numAvailableVehicles = 0;
        std::vector<BuildableVehicle> buildableVehicles;

        const bool showUnpoweredVehicles = (_vehicleFilterFlags & VehicleFilterFlags::unpowered) != VehicleFilterFlags::none;
        const bool showPoweredVehicles = (_vehicleFilterFlags & VehicleFilterFlags::powered) != VehicleFilterFlags::none;
        const bool showUnlockedVehicles = (_vehicleFilterFlags & VehicleFilterFlags::unlocked) != VehicleFilterFlags::none;
        const bool showLockedVehicles = (_vehicleFilterFlags & VehicleFilterFlags::locked) != VehicleFilterFlags::none && Config::get().displayLockedVehicles;

        for (uint16_t vehicleObjIndex = 0; vehicleObjIndex < ObjectManager::getMaxObjects(ObjectType::vehicle); ++vehicleObjIndex)
        {
            auto vehicleObj = ObjectManager::get<VehicleObject>(vehicleObjIndex);
            if (vehicleObj == nullptr)
            {
                continue;
            }

            if (vehicle && vehicle->isVehicleHead())
            {
                auto* const head = vehicle->asVehicleHead();
                if (!head->isVehicleTypeCompatible(vehicleObjIndex))
                {
                    continue;
                }
            }

            if (vehicleObj->type != vehicleType)
            {
                continue;
            }

            const auto* company = CompanyManager::get(companyId);
            if (!((showUnlockedVehicles && company->isVehicleIndexUnlocked(vehicleObjIndex)) || (showLockedVehicles && !company->isVehicleIndexUnlocked(vehicleObjIndex))))
            {
                continue;
            }

            std::string_view pattern = inputSession.buffer;

            if (!pattern.empty())
            {
                const std::string_view name = StringManager::getString(vehicleObj->name);
                if (!contains(name, pattern))
                {
                    continue;
                }
            }

            if (trackType != 0xFF)
            {
                uint8_t sanitisedTrackType = trackType;
                if (trackType & (1 << 7))
                {
                    if (vehicleObj->mode != TransportMode::road)
                    {
                        continue;
                    }

                    if (trackType == 0xFE)
                    {
                        sanitisedTrackType = 0xFF;
                    }
                    else
                    {
                        sanitisedTrackType = trackType & ~(1 << 7);
                    }
                }
                else
                {
                    if (vehicleObj->mode != TransportMode::rail)
                    {
                        continue;
                    }
                }

                if (sanitisedTrackType != vehicleObj->trackType)
                {
                    continue;
                }
            }

            if (_cargoSupportedFilter != 0xFF && _cargoSupportedFilter != 0xFE)
            {
                auto usableCargoTypes = vehicleObj->compatibleCargoCategories[0] | vehicleObj->compatibleCargoCategories[1];
                if ((usableCargoTypes & (1 << _cargoSupportedFilter)) == 0)
                {
                    continue;
                }
            }

            const bool isPowered = vehicleObj->power > 0;
            if (!((isPowered && showPoweredVehicles) || (!isPowered && showUnpoweredVehicles)))
            {
                continue;
            }

            const bool isCargoless = vehicleObj->compatibleCargoCategories[0] == 0 && vehicleObj->compatibleCargoCategories[1] == 0;
            if (_cargoSupportedFilter == 0xFE && !isCargoless)
            {
                continue;
            }

            buildableVehicles.push_back({ vehicleObjIndex, vehicleObj->name, isPowered, vehicleObj->designed });
        }

        // Sort by name or design year
        if (_vehicleSortBy == VehicleSortBy::name)
        {
            std::sort(buildableVehicles.begin(), buildableVehicles.end(), [](const BuildableVehicle& item1, const BuildableVehicle& item2) {
                const std::string_view str1 = StringManager::getString(item1.name);
                const std::string_view str2 = StringManager::getString(item2.name);
                return str1 < str2;
            });
        }
        else if (_vehicleSortBy == VehicleSortBy::designYear)
        {
            std::sort(buildableVehicles.begin(), buildableVehicles.end(), [](const BuildableVehicle& item1, const BuildableVehicle& item2) { return item1.designed < item2.designed; });
        }

        // Group powered vehicles, if were not leaving (un)powered out
        if ((_vehicleFilterFlags & kMaskPoweredUnpowered) == kMaskPoweredUnpowered)
        {
            std::stable_sort(buildableVehicles.begin(), buildableVehicles.end(), [](const BuildableVehicle& item1, const BuildableVehicle& item2) { return item1.isPowered > item2.isPowered; });
        }

        // Assign available vehicle positions
        for (size_t i = 0; i < buildableVehicles.size(); ++i)
        {
            _availableVehicles[i] = buildableVehicles[i].vehicleIndex;
        }

        _numAvailableVehicles = static_cast<int16_t>(buildableVehicles.size());
        _lastRefreshYear = getCurrentYear();
        _lastDisplayLockedVehiclesState = Config::get().displayLockedVehicles;
    }

    static Ui::Window* getTopEditingVehicleWindow()
    {
        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
            {
                continue;
            }

            if (w->currentTab != 1)
            {
                continue;
            }

            auto vehicle = EntityManager::get<Vehicles::VehicleBase>(EntityId(w->number));
            if (vehicle == nullptr)
            {
                continue;
            }

            if (vehicle->owner != CompanyManager::getControllingId())
            {
                continue;
            }

            return w;
        }

        return nullptr;
    }

    /**
     * 0x004B92A5
     *
     * @param window @<esi>
     */
    void sub_4B92A5(Ui::Window* window)
    {
        auto w = getTopEditingVehicleWindow();
        int32_t vehicleId = -1;
        if (w != nullptr)
        {
            vehicleId = w->number;
        }

        if (_buildTargetVehicle != vehicleId)
        {
            _buildTargetVehicle = vehicleId;
            window->var_83C = 0;
            window->invalidate();
        }

        VehicleType vehicleType = _transportTypeTabInformation[window->currentTab].type;
        uint8_t trackType = _trackTypesForTab[window->currentSecondaryTab];

        Vehicles::VehicleBase* veh = nullptr;
        if (_buildTargetVehicle != -1)
        {
            veh = EntityManager::get<Vehicles::VehicleBase>(EntityId(_buildTargetVehicle));
        }

        generateBuildableVehiclesArray(vehicleType, trackType, veh);

        int numRows = _numAvailableVehicles;
        uint16_t* src = _availableVehicles;
        int16_t* dest = window->rowInfo;
        window->var_83C = numRows;
        window->rowCount = 0;
        while (numRows != 0)
        {
            *dest = *src;
            dest++;
            src++;
            numRows--;
        }
        window->rowHover = -1;
        window->invalidate();
    }

    // 0x4C3576
    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(&window);
                break;

            case widx::tab_build_new_trains:
            case widx::tab_build_new_buses:
            case widx::tab_build_new_trucks:
            case widx::tab_build_new_trams:
            case widx::tab_build_new_aircraft:
            case widx::tab_build_new_ships:
            {

                if (Input::hasFlag(Input::Flags::toolActive))
                {
                    ToolManager::toolCancel(window.type, window.number);
                }

                auto newTab = widgetIndex - widx::tab_build_new_trains;
                window.currentTab = newTab;
                window.rowHeight = kScrollRowHeight[newTab];
                window.frameNo = 0;
                window.currentSecondaryTab = 0;
                if (newTab != enumValue(getGameState().lastBuildVehiclesOption))
                {
                    getGameState().lastBuildVehiclesOption = static_cast<VehicleType>(newTab);
                    WindowManager::invalidate(WindowType::topToolbar, 0);
                }

                auto curViewport = window.viewports[0];
                window.viewports[0] = nullptr;
                if (curViewport != nullptr)
                {
                    curViewport->width = 0;
                }

                window.holdableWidgets = 0;
                window.eventHandlers = &getEvents();
                window.setWidgets(_widgets);
                setDisabledTransportTabs(&window);
                window.invalidate();
                _buildTargetVehicle = -1;
                setTrackTypeTabs(&window);
                resetTrackTypeTabSelection(&window);
                window.rowCount = 0;
                window.var_83C = 0;
                window.rowHover = -1;
                sub_4B92A5(&window);
                window.callOnResize();
                window.callOnPeriodicUpdate();
                window.callPrepareDraw();
                window.initScrollWidgets();
                window.invalidate();
                window.moveInsideScreenEdges();
                break;
            }
            case widx::tab_track_type_0:
            case widx::tab_track_type_1:
            case widx::tab_track_type_2:
            case widx::tab_track_type_3:
            case widx::tab_track_type_4:
            case widx::tab_track_type_5:
            case widx::tab_track_type_6:
            case widx::tab_track_type_7:
            {
                auto tab = widxToTrackTypeTab(widgetIndex);
                if (window.currentSecondaryTab == tab)
                {
                    break;
                }

                window.currentSecondaryTab = tab;
                setTopToolbarLastTrack(_trackTypesForTab[tab] & ~(1 << 7), _trackTypesForTab[tab] & (1 << 7));
                _buildTargetVehicle = -1;
                window.rowCount = 0;
                window.var_83C = 0;
                window.rowHover = -1;
                sub_4B92A5(&window);
                window.callOnResize();
                window.callOnPeriodicUpdate();
                window.callPrepareDraw();
                window.initScrollWidgets();
                window.invalidate();
                break;
            }

            case widx::searchClearButton:
            {
                inputSession.clearInput();
                sub_4B92A5(&window);
                window.initScrollWidgets();
                window.invalidate();
                break;
            }
        }
    }

    static void onMouseDown(Window& self, const WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        if (widgetIndex == widx::filterDropdown)
        {
            auto& dropdown = self.widgets[widx::filterLabel];
            auto numItems = Config::get().displayLockedVehicles ? 7 : 5;
            Dropdown::showText(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), numItems, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::sortByDesignYear);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::sortByName);
            Dropdown::add(2, 0);
            Dropdown::add(3, StringIds::dropdown_without_checkmark, StringIds::componentUnpowered);
            Dropdown::add(4, StringIds::dropdown_without_checkmark, StringIds::componentPowered);

            if (Config::get().displayLockedVehicles)
            {
                Dropdown::add(5, StringIds::dropdown_without_checkmark, StringIds::componentUnlocked);
                Dropdown::add(6, StringIds::dropdown_without_checkmark, StringIds::componentLocked);
            }

            // Mark current sort order
            Dropdown::setItemSelected(enumValue(_vehicleSortBy));

            // Show unpowered vehicles?
            if ((_vehicleFilterFlags & VehicleFilterFlags::unpowered) != VehicleFilterFlags::none)
            {
                Dropdown::setItemSelected(3);
            }

            // Show powered vehicles?
            if ((_vehicleFilterFlags & VehicleFilterFlags::powered) != VehicleFilterFlags::none)
            {
                Dropdown::setItemSelected(4);
            }

            // Show unlocked vehicles?
            if ((_vehicleFilterFlags & VehicleFilterFlags::unlocked) != VehicleFilterFlags::none)
            {
                Dropdown::setItemSelected(5);
            }

            // Show locked vehicles?
            if ((_vehicleFilterFlags & VehicleFilterFlags::locked) != VehicleFilterFlags::none)
            {
                Dropdown::setItemSelected(6);
            }
        }
        else if (widgetIndex == widx::cargoDropdown)
        {
            auto index = 0U;
            auto selectedIndex = -1;

            Dropdown::add(index++, StringIds::dropdown_stringid, StringIds::allCargoTypes);
            if (_cargoSupportedFilter == 0xFF)
            {
                selectedIndex = 0;
            }

            Dropdown::add(index++, StringIds::dropdown_stringid, StringIds::filterCargoless);
            if (_cargoSupportedFilter == 0xFE)
            {
                selectedIndex = 1;
            }

            for (uint16_t cargoId = 0; cargoId < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoId)
            {
                auto cargoObj = ObjectManager::get<CargoObject>(cargoId);
                if (cargoObj == nullptr)
                {
                    continue;
                }

                FormatArguments args{};
                args.push(cargoObj->name);
                args.push(cargoObj->unitInlineSprite);
                args.push(cargoId);
                Dropdown::add(index, StringIds::supportsCargoIdSprite, args);

                if (_cargoSupportedFilter == cargoId)
                {
                    selectedIndex = index;
                }

                index++;
            }

            Widget dropdown = self.widgets[widx::cargoLabel];
            Dropdown::showText(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), index, 0x80);

            if (selectedIndex != -1)
            {
                Dropdown::setItemSelected(selectedIndex);
            }
        }
    }

    static void onDropdown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
    {
        if (itemIndex < 0)
        {
            return;
        }

        if (widgetIndex == widx::filterDropdown)
        {
            if (itemIndex == 0)
            {
                _vehicleSortBy = VehicleSortBy::designYear;
            }
            else if (itemIndex == 1)
            {
                _vehicleSortBy = VehicleSortBy::name;
            }
            else if (itemIndex == 3)
            {
                _vehicleFilterFlags ^= VehicleFilterFlags::unpowered;
            }
            else if (itemIndex == 4)
            {
                _vehicleFilterFlags ^= VehicleFilterFlags::powered;
            }
            else if (itemIndex == 5)
            {
                _vehicleFilterFlags ^= VehicleFilterFlags::unlocked;
            }
            else if (itemIndex == 6)
            {
                _vehicleFilterFlags ^= VehicleFilterFlags::locked;
            }
        }
        else if (widgetIndex == widx::cargoDropdown)
        {
            if (itemIndex >= 2)
            {
                _cargoSupportedFilter = Dropdown::getItemArgument(itemIndex, 3);
            }
            else if (itemIndex == 0)
            {
                _cargoSupportedFilter = 0xFF;
            }
            else if (itemIndex == 1)
            {
                _cargoSupportedFilter = 0xFE;
            }
        }

        sub_4B92A5(&self);
        self.invalidate();
    }

    // 0x4C3929
    static void onResize(Window& window)
    {
        window.flags |= WindowFlags::resizable;

        auto minWidth = std::max<uint16_t>(_numTrackTypeTabs * 31 + 195, 380);
        window.setSize({ minWidth, 233 }, { 520, 600 });

        auto& scrollArea = window.scrollAreas[scrollIdx::vehicle_selection];
        auto& scrollWidget = window.widgets[widx::scrollview_vehicle_selection];
        auto scrollPosition = std::max(0, scrollArea.contentHeight - scrollWidget.bottom + scrollWidget.top);

        if (scrollPosition < scrollArea.contentOffsetY)
        {
            scrollArea.contentOffsetY = scrollPosition;
            Ui::ScrollView::updateThumbs(window, widx::scrollview_vehicle_selection);
        }

        if (window.rowHover != -1)
        {
            return;
        }

        if (window.var_83C == 0)
        {
            return;
        }

        window.rowHover = window.rowInfo[0];
        window.invalidate();
    }

    // 0x4C377B, 0x4C3923
    static void onUpdate(Window& window)
    {
        // Is the linked vehicle still available?
        const bool linkedVehicleAvailable = window.number && WindowManager::find(WindowType::vehicle, window.number);

        // Do we need to refresh the component list?
        if (!linkedVehicleAvailable || _lastRefreshYear != getCurrentYear() || _lastDisplayLockedVehiclesState != Config::get().displayLockedVehicles)
        {
            sub_4B92A5(&window);
        }

        window.frameNo++;
        window.callPrepareDraw();

        WindowManager::invalidateWidget(WindowType::buildVehicle, window.number, widx::tab_build_new_trains + window.currentTab);
        WindowManager::invalidateWidget(WindowType::buildVehicle, window.number, widx::tab_track_type_0 + (window.currentSecondaryTab & 0xFF));
        WindowManager::invalidateWidget(WindowType::buildVehicle, window.number, widx::scrollview_vehicle_preview);

        inputSession.cursorFrame++;
        if ((inputSession.cursorFrame % 16) == 0)
        {
            WindowManager::invalidateWidget(WindowType::buildVehicle, window.number, widx::searchBox);
        }
    }

    // 0x4C37B9
    static void getScrollSize(Ui::Window& window, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
    {
        scrollHeight = window.var_83C * window.rowHeight;
    }

    // 0x4C384B
    static void onScrollMouseDown(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, uint8_t scroll_index)
    {
        if (scroll_index != scrollIdx::vehicle_selection)
        {
            return;
        }

        auto scrollItem = y / window.rowHeight;
        if (scrollItem >= window.var_83C)
        {
            return;
        }

        auto pan = window.width / 2 + window.x;
        Audio::playSound(Audio::SoundId::clickDown, pan);
        auto item = window.rowInfo[scrollItem];
        auto vehicleObj = ObjectManager::get<VehicleObject>(item);
        auto args = FormatArguments::common();
        // Skip 5 * 2 bytes
        args.skip(10);
        args.push(vehicleObj->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop_5_string_id);
        if (_buildTargetVehicle != -1)
        {
            auto vehicle = EntityManager::get<Vehicles::VehicleHead>(EntityId(_buildTargetVehicle));
            if (vehicle != nullptr)
            {
                args.push(vehicle->name);
                args.push(vehicle->ordinalNumber);
                GameCommands::setErrorTitle(StringIds::cant_add_pop_5_string_id_string_id);
            }
        }

        GameCommands::VehicleCreateArgs gcArgs{};
        gcArgs.vehicleId = EntityId(_buildTargetVehicle);
        gcArgs.vehicleType = item;
        if (GameCommands::doCommand(gcArgs, GameCommands::Flags::apply) == GameCommands::FAILURE)
        {
            return;
        }

        if (_buildTargetVehicle == -1)
        {
            auto vehicle = EntityManager::get<Vehicles::VehicleBase>(GameCommands::getLegacyReturnState().lastCreatedVehicleId);
            Vehicle::Details::open(vehicle);
        }
        sub_4B92A5(&window);
    }

    // 0x4C3802
    static void onScrollMouseOver(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, uint8_t scroll_index)
    {
        if (scroll_index != scrollIdx::vehicle_selection)
        {
            return;
        }

        auto scrollItem = y / window.rowHeight;
        int16_t item = -1;
        if (scrollItem < window.var_83C)
        {
            item = window.rowInfo[scrollItem];
        }

        if (item != -1 && item != window.rowHover)
        {
            window.rowHover = item;
            window.invalidate();
        }
    }

    // 0x4C370C
    static std::optional<FormatArguments> tooltip(Ui::Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        FormatArguments args{};
        if (widgetIndex < widx::tab_track_type_0 || widgetIndex >= widx::scrollview_vehicle_selection)
        {
            args.push(StringIds::tooltip_scroll_new_vehicle_list);
        }
        else
        {
            auto trackTypeTab = widxToTrackTypeTab(widgetIndex);
            auto type = _trackTypesForTab[trackTypeTab];
            if (type == 0xFF)
            {
                if (_transportTypeTabInformation[window.currentTab].type == VehicleType::aircraft)
                {

                    args.push(StringIds::airport);
                }
                else
                {
                    args.push(StringIds::docks);
                }
            }
            else
            {
                bool is_road = type & (1 << 7);
                type &= ~(1 << 7);
                if (is_road)
                {
                    auto roadObj = ObjectManager::get<RoadObject>(type);
                    args.push(roadObj->name);
                }
                else
                {
                    auto trackObj = ObjectManager::get<TrackObject>(type);
                    args.push(trackObj->name);
                }
            }
        }
        return args;
    }

    // 0x4C37CB
    static Ui::CursorId cursor(Window& window, WidgetIndex_t widgetIdx, [[maybe_unused]] const WidgetId id, [[maybe_unused]] int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        if (widgetIdx != widx::scrollview_vehicle_selection)
        {
            return fallback;
        }

        auto scrollItem = yPos / window.rowHeight;
        if (scrollItem >= window.var_83C)
        {
            return fallback;
        }

        if (window.rowInfo[scrollItem] == -1)
        {
            return fallback;
        }

        return CursorId::handPointer;
    }

    // 0x4C2E5C
    static void prepareDraw(Ui::Window& window)
    {
        setDisabledTransportTabs(&window);

        // Mask off all the tabs
        auto activeWidgets = window.activatedWidgets & ((1 << frame) | (1 << caption) | (1 << close_button) | (1 << panel) | (1 << scrollview_vehicle_selection) | (1 << scrollview_vehicle_preview));
        // Only activate the singular tabs
        activeWidgets |= 1ULL << _transportTypeTabInformation[window.currentTab].widgetIndex;
        activeWidgets |= 1ULL << (window.currentSecondaryTab + widx::tab_track_type_0);

        window.activatedWidgets = activeWidgets;

        window.widgets[widx::caption].text = window.currentTab + StringIds::build_trains;

        auto width = window.width;
        auto height = window.height;

        window.widgets[widx::frame].right = width - 1;
        window.widgets[widx::frame].bottom = height - 1;

        window.widgets[widx::panel].right = width - 1;
        window.widgets[widx::panel].bottom = height - 1;

        window.widgets[widx::caption].right = width - 2;

        window.widgets[widx::close_button].left = width - 15;
        window.widgets[widx::close_button].right = width - 3;

        window.widgets[widx::scrollview_vehicle_preview].right = width - 4;
        window.widgets[widx::scrollview_vehicle_preview].left = width - 184;

        auto& selectionList = window.widgets[widx::scrollview_vehicle_selection];
        selectionList.right = width - 187;
        selectionList.bottom = height - 14;

        window.widgets[widx::searchClearButton].right = selectionList.right;
        window.widgets[widx::searchClearButton].left = selectionList.right - 40;
        window.widgets[widx::searchBox].right = selectionList.right - 42;

        window.widgets[widx::cargoLabel].right = selectionList.right;
        window.widgets[widx::cargoLabel].left = selectionList.right - (selectionList.width() / 2);
        window.widgets[widx::cargoDropdown].right = selectionList.right;
        window.widgets[widx::cargoDropdown].left = selectionList.right - 12;

        if (_cargoSupportedFilter == 0xFF)
        {
            window.widgets[widx::cargoLabel].text = StringIds::filterCargoSupported;
        }
        else if (_cargoSupportedFilter == 0xFE)
        {
            window.widgets[widx::cargoLabel].text = StringIds::filterCargoless;
        }
        else
        {
            window.widgets[widx::cargoLabel].text = StringIds::empty;
        }

        window.widgets[widx::filterLabel].right = window.widgets[widx::cargoLabel].left - 1;
        window.widgets[widx::filterDropdown].right = window.widgets[widx::cargoLabel].left - 2;
        window.widgets[widx::filterDropdown].left = window.widgets[widx::filterDropdown].right - 11;

        Widget::leftAlignTabs(window, widx::tab_build_new_trains, widx::tab_build_new_ships);
    }

    static void drawSearchBox(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        char* textBuffer = (char*)StringManager::getString(StringIds::buffer_2039);
        strncpy(textBuffer, inputSession.buffer.c_str(), 256);

        auto& widget = _widgets[widx::searchBox];
        auto clipped = Gfx::clipRenderTarget(drawingCtx.currentRenderTarget(), Ui::Rect(self.x + widget.left, widget.top + 1 + self.y, widget.width() - 2, widget.height() - 2));
        if (!clipped)
        {
            return;
        }

        drawingCtx.pushRenderTarget(*clipped);

        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw search box input buffer
        FormatArguments args{};
        args.push(StringIds::buffer_2039);

        Ui::Point position = { inputSession.xOffset, 1 };
        tr.drawStringLeft(position, Colour::black, StringIds::black_stringid, args);

        // Draw search box cursor, blinking
        if (Input::isFocused(self.type, self.number, widx::searchBox) && (inputSession.cursorFrame % 32) < 16)
        {
            // We draw the string again to figure out where the cursor should go; position.x will be adjusted
            textBuffer[inputSession.cursorPosition] = '\0';
            position = { inputSession.xOffset, 1 };
            position = tr.drawStringLeft(position, Colour::black, StringIds::black_stringid, args);
            drawingCtx.fillRect(position.x, position.y, position.x, position.y + 9, Colours::getShade(self.getColour(WindowColour::secondary).c(), 9), Gfx::RectFlags::none);
        }

        drawingCtx.popRenderTarget();
    }

    // 0x4C2F23
    static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);

        drawTransportTypeTabs(self, drawingCtx);
        drawTrackTypeTabs(self, drawingCtx);
        drawSearchBox(self, drawingCtx);

        {
            auto bottomLeftMessage = StringIds::select_new_vehicle;
            FormatArguments args{};
            if (_buildTargetVehicle != -1)
            {
                auto vehicle = EntityManager::get<Vehicles::VehicleHead>(EntityId(_buildTargetVehicle));
                if (vehicle != nullptr)
                {
                    args.push(vehicle->name);
                    args.push(vehicle->ordinalNumber);
                    bottomLeftMessage = StringIds::select_vehicle_to_add_to_string_id;
                }
            }

            auto point = Point(self.x + 2, self.y + self.height - 13);
            tr.drawStringLeftClipped(point, self.width - 186, Colour::black, bottomLeftMessage, args);
        }

        if (_cargoSupportedFilter != 0xFF && _cargoSupportedFilter != 0xFE)
        {
            auto cargoObj = ObjectManager::get<CargoObject>(_cargoSupportedFilter);

            FormatArguments args{};
            args.push(StringIds::cargoIdSprite);
            args.push(cargoObj->name);
            args.push(cargoObj->unitInlineSprite);

            auto& widget = self.widgets[widx::cargoLabel];
            auto point = Point(self.x + widget.left + 2, self.y + widget.top);
            tr.drawStringLeftClipped(point, widget.width() - 15, Colour::black, StringIds::wcolour2_stringid, args);
        }

        if (self.rowHover == -1)
        {
            return;
        }

        auto vehicleObj = ObjectManager::get<VehicleObject>(self.rowHover);
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));

        {
            auto cost = Economy::getInflationAdjustedCost(vehicleObj->costFactor, vehicleObj->costIndex, 6);
            FormatArguments args{};
            args.push(cost);
            buffer = StringManager::formatString(buffer, StringIds::stats_cost, args);
        }

        {
            auto runningCost = Economy::getInflationAdjustedCost(vehicleObj->runCostFactor, vehicleObj->runCostIndex, 10);
            FormatArguments args{};
            args.push(runningCost);
            buffer = StringManager::formatString(buffer, StringIds::stats_running_cost, args);
        }

        if (vehicleObj->designed != 0)
        {
            FormatArguments args{};
            args.push(vehicleObj->designed);

            const auto* company = CompanyManager::get(CompanyManager::getControllingId());
            auto unlocked = company->isVehicleIndexUnlocked(self.rowHover);
            buffer = StringManager::formatString(
                buffer,
                unlocked ? StringIds::stats_designed : StringIds::stats_proposed_design,
                args);
        }

        if (vehicleObj->obsolete != 0 && vehicleObj->obsolete != std::numeric_limits<uint16_t>::max())
        {
            FormatArguments args{};
            args.push(vehicleObj->obsolete);
            buffer = StringManager::formatString(buffer, StringIds::stats_obsolete, args);
        }

        if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
        {
            buffer = StringManager::formatString(buffer, StringIds::stats_requires);
            auto trackName = StringIds::road;
            if (vehicleObj->mode == TransportMode::road)
            {
                if (vehicleObj->trackType != 0xFF)
                {
                    trackName = ObjectManager::get<RoadObject>(vehicleObj->trackType)->name;
                }
            }
            else
            {
                trackName = ObjectManager::get<TrackObject>(vehicleObj->trackType)->name;
            }

            buffer = StringManager::formatString(buffer, trackName);

            for (auto i = 0; i < vehicleObj->numTrackExtras; ++i)
            {
                strcpy(buffer, " + ");
                buffer += 3;
                if (vehicleObj->mode == TransportMode::road)
                {
                    auto roadExtraObj = ObjectManager::get<RoadExtraObject>(vehicleObj->requiredTrackExtras[i]);
                    buffer = StringManager::formatString(buffer, roadExtraObj->name);
                }
                else
                {
                    auto trackExtraObj = ObjectManager::get<TrackExtraObject>(vehicleObj->requiredTrackExtras[i]);
                    buffer = StringManager::formatString(buffer, trackExtraObj->name);
                }
            }

            if (vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
            {
                auto trackExtraObj = ObjectManager::get<TrackExtraObject>(vehicleObj->rackRailType);
                FormatArguments args{};
                args.push(trackExtraObj->name);
                buffer = StringManager::formatString(buffer, StringIds::stats_string_steep_slope, args);
            }
        }

        if (vehicleObj->power != 0)
        {
            if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
            {
                FormatArguments args{};
                args.push(vehicleObj->power);
                buffer = StringManager::formatString(buffer, StringIds::stats_power, args);
            }
        }

        {
            FormatArguments args{};
            args.push<uint32_t>(StringManager::internalLengthToComma1DP(vehicleObj->getLength()));
            buffer = StringManager::formatString(buffer, StringIds::stats_length, args);
        }
        {
            FormatArguments args{};
            args.push(vehicleObj->weight);
            buffer = StringManager::formatString(buffer, StringIds::stats_weight, args);
        }
        {
            FormatArguments args{};
            args.push(vehicleObj->speed.getRaw());
            buffer = StringManager::formatString(buffer, StringIds::stats_max_speed, args);
        }
        if (vehicleObj->hasFlags(VehicleObjectFlags::rackRail))
        {
            auto trackExtraObj = ObjectManager::get<TrackExtraObject>(vehicleObj->rackRailType);
            FormatArguments args{};
            args.push(vehicleObj->rackSpeed);
            args.push(trackExtraObj->name);
            buffer = StringManager::formatString(buffer, StringIds::stats_velocity_on_string, args);
        }

        vehicleObj->getCargoString(buffer);

        auto x = self.widgets[widx::scrollview_vehicle_selection].right + self.x + 2;
        auto y = self.widgets[widx::scrollview_vehicle_preview].bottom + self.y + 2;
        tr.drawStringLeftWrapped(Point(x, y), 180, Colour::black, StringIds::buffer_1250);
    }

    // 0x4C3307
    static void drawScroll(Ui::Window& window, Gfx::DrawingContext& drawingCtx, const uint32_t scrollIndex)
    {
        const auto& rt = drawingCtx.currentRenderTarget();
        auto tr = Gfx::TextRenderer(drawingCtx);

        switch (scrollIndex)
        {
            case scrollIdx::vehicle_selection:
            {
                auto colour = Colours::getShade(window.getColour(WindowColour::secondary).c(), 4);
                drawingCtx.clear(colour * 0x01010101);
                if (window.var_83C == 0)
                {
                    auto defaultMessage = StringIds::no_vehicles_available;
                    FormatArguments args{};
                    if (_buildTargetVehicle != -1)
                    {
                        auto vehicle = EntityManager::get<Vehicles::VehicleHead>(EntityId(_buildTargetVehicle));
                        if (vehicle != nullptr)
                        {
                            defaultMessage = StringIds::no_compatible_vehicles_available;
                            args.push(vehicle->name);
                            args.push(vehicle->ordinalNumber);
                        }
                    }

                    auto widget = window.widgets[widx::scrollview_vehicle_selection];
                    auto width = widget.right - widget.left - 17;
                    auto point = Point(3, (window.rowHeight - 10) / 2);
                    tr.drawStringLeftWrapped(point, width, Colour::black, defaultMessage, args);
                }
                else
                {
                    int16_t y = 0;
                    for (auto i = 0; i < window.var_83C; ++i, y += window.rowHeight)
                    {
                        if (y + window.rowHeight + 30 <= rt.y)
                        {
                            continue;
                        }

                        if (y >= rt.y + rt.height + 30)
                        {
                            break;
                        }

                        auto vehicleType = window.rowInfo[i];
                        if (vehicleType == -1)
                        {
                            continue;
                        }

                        const auto* company = CompanyManager::get(CompanyManager::getControllingId());
                        auto rowIsALockedVehicle = !company->isVehicleIndexUnlocked(vehicleType)
                            && !Config::get().buildLockedVehicles;

                        auto colouredString = StringIds::black_stringid;

                        const auto lockedHoverRowColour = PaletteIndex::mutedDarkRed3;
                        constexpr auto normalHoverRowColour = enumValue(ExtColour::unk30);
                        const auto lockedRowColour = PaletteIndex::mutedDarkRed5;

                        if (window.rowHover == vehicleType)
                        {
                            if (rowIsALockedVehicle)
                            {
                                drawingCtx.fillRect(0, y, window.width, y + window.rowHeight - 1, lockedHoverRowColour, Gfx::RectFlags::crossHatching);
                            }
                            else
                            {
                                drawingCtx.fillRect(0, y, window.width, y + window.rowHeight - 1, normalHoverRowColour, Gfx::RectFlags::transparent);
                            }
                            colouredString = StringIds::wcolour2_stringid;
                        }
                        else
                        {
                            if (rowIsALockedVehicle)
                            {
                                drawingCtx.fillRect(0, y, window.width, y + window.rowHeight - 1, lockedRowColour, Gfx::RectFlags::crossHatching);
                            }
                        }

                        int16_t half = (window.rowHeight - 22) / 2;
                        auto x = drawVehicleInline(drawingCtx, vehicleType, CompanyManager::getControllingId(), { 0, static_cast<int16_t>(y + half) });

                        auto vehicleObj = ObjectManager::get<VehicleObject>(vehicleType);
                        FormatArguments args{};
                        args.push(vehicleObj->name);
                        half = (window.rowHeight - 10) / 2;

                        auto point = Point(x + 3, y + half);
                        tr.drawStringLeft(point, Colour::black, colouredString, args);
                    }
                }
                break;
            }
            case scrollIdx::vehicle_preview:
            {
                auto colour = Colours::getShade(window.getColour(WindowColour::secondary).c(), 0);
                // Gfx::clear needs the colour copied to each byte of eax
                drawingCtx.clear(colour * 0x01010101);

                if (window.rowHover == -1)
                {
                    break;
                }

                uint8_t yaw = Ui::WindowManager::getVehiclePreviewRotationFrameYaw();
                uint8_t roll = Ui::WindowManager::getVehiclePreviewRotationFrameRoll();
                drawVehicleOverview(drawingCtx, { 90, 37 }, window.rowHover, yaw, roll, CompanyManager::getControllingId());

                auto vehicleObj = ObjectManager::get<VehicleObject>(window.rowHover);
                auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
                buffer = StringManager::formatString(buffer, vehicleObj->name);
                auto usableCargoTypes = vehicleObj->compatibleCargoCategories[0] | vehicleObj->compatibleCargoCategories[1];

                for (auto cargoTypes = Numerics::bitScanForward(usableCargoTypes); cargoTypes != -1; cargoTypes = Numerics::bitScanForward(usableCargoTypes))
                {
                    usableCargoTypes &= ~(1 << cargoTypes);
                    auto cargoObj = ObjectManager::get<CargoObject>(cargoTypes);
                    *buffer++ = ' ';
                    *buffer++ = ControlCodes::inlineSpriteStr;
                    *(reinterpret_cast<uint32_t*>(buffer)) = cargoObj->unitInlineSprite;
                    buffer += 4;
                }

                *buffer++ = '\0';
                FormatArguments args{};
                args.push(StringIds::buffer_1250);

                tr.drawStringCentredClipped(Point(89, 52), 177, Colour::darkOrange, StringIds::wcolour2_stringid, args);
                break;
            }
        }
    }

    // 0x4C28D2
    static void setDisabledTransportTabs(Ui::Window* window)
    {
        auto availableVehicles = CompanyManager::get(CompanyId(window->number))->availableVehicles;
        // By shifting by 4 the available_vehicles flags align with the tabs flags
        auto disabledTabs = (availableVehicles << 4) ^ ((1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships));
        window->disabledWidgets = disabledTabs;
    }

    // 0x4C2D8A
    static void setTrackTypeTabs(Ui::Window* window)
    {
        VehicleType currentTransportTabType = _transportTypeTabInformation[window->currentTab].type;
        generateBuildableVehiclesArray(currentTransportTabType, 0xFF, nullptr);

        auto railTrackTypes = 0;
        auto roadTrackTypes = 0;
        for (auto i = 0; i < _numAvailableVehicles; i++)
        {
            auto vehicleObj = ObjectManager::get<VehicleObject>(_availableVehicles[i]);
            if (vehicleObj && vehicleObj->mode == TransportMode::rail)
            {
                railTrackTypes |= (1 << vehicleObj->trackType);
            }
            else if (vehicleObj && vehicleObj->mode == TransportMode::road)
            {
                auto trackType = vehicleObj->trackType;
                if (trackType == 0xFF)
                {
                    trackType = getGameState().lastTrackTypeOption;
                }
                roadTrackTypes |= (1 << trackType);
            }
            else
            {
                // Reset the tabs
                _trackTypesForTab[0] = 0xFF;
                _numTrackTypeTabs = 1;
                window->widgets[tab_track_type_0].hidden = false;
                for (WidgetIndex_t j = tab_track_type_1; j <= tab_track_type_7; ++j)
                {
                    window->widgets[j].hidden = true;
                }
                return;
            }
        }

        WidgetIndex_t trackTypeTab = tab_track_type_0;
        auto trackType = 0;
        for (trackType = Numerics::bitScanForward(railTrackTypes); trackType != -1 && trackTypeTab <= tab_track_type_7; trackType = Numerics::bitScanForward(railTrackTypes))
        {
            railTrackTypes &= ~(1 << trackType);
            window->widgets[trackTypeTab].hidden = false;
            _trackTypesForTab[widxToTrackTypeTab(trackTypeTab)] = trackType;
            trackTypeTab++;
        }

        if (trackType == -1 && trackTypeTab <= tab_track_type_7)
        {
            for (trackType = Numerics::bitScanForward(roadTrackTypes); trackType != -1 && trackTypeTab <= tab_track_type_7; trackType = Numerics::bitScanForward(roadTrackTypes))
            {
                roadTrackTypes &= ~(1 << trackType);
                window->widgets[trackTypeTab].hidden = false;
                _trackTypesForTab[widxToTrackTypeTab(trackTypeTab)] = trackType | (1 << 7);
                trackTypeTab++;
            }
        }

        _numTrackTypeTabs = widxToTrackTypeTab(trackTypeTab);

        for (; trackTypeTab <= tab_track_type_7; ++trackTypeTab)
        {
            window->widgets[trackTypeTab].hidden = true;
        }
    }

    // 0x4C1CBE
    // if previous track tab on previous transport type tab is also compatible keeps it on that track type
    static void resetTrackTypeTabSelection(Ui::Window* window)
    {
        auto transportType = _transportTypeTabInformation[window->currentTab].type;
        if (transportType == VehicleType::aircraft || transportType == VehicleType::ship)
        {
            window->currentSecondaryTab = 0;
            return;
        }

        bool found = false;
        uint32_t trackTab = 0;
        for (; trackTab < _numTrackTypeTabs; trackTab++)
        {
            if (getGameState().lastRailroadOption == _trackTypesForTab[trackTab])
            {
                found = true;
                break;
            }

            if (getGameState().lastRoadOption == _trackTypesForTab[trackTab])
            {
                found = true;
                break;
            }
        }

        trackTab = found ? trackTab : 0;
        window->currentSecondaryTab = trackTab;

        bool isRoad = _trackTypesForTab[trackTab] & (1 << 7);
        uint8_t trackType = _trackTypesForTab[trackTab] & ~(1 << 7);
        setTopToolbarLastTrack(trackType, isRoad);
    }

    // 0x4A3A06
    static void setTopToolbarLastTrack(uint8_t trackType, bool isRoad)
    {
        bool setRail = false;
        if (isRoad)
        {
            auto road_obj = ObjectManager::get<RoadObject>(trackType);
            if (road_obj && road_obj->hasFlags(RoadObjectFlags::unk_01))
            {
                setRail = true;
            }
        }
        else
        {
            auto rail_obj = ObjectManager::get<TrackObject>(trackType);
            if (rail_obj && !rail_obj->hasFlags(TrackObjectFlags::unk_02))
            {
                setRail = true;
            }
        }

        if (setRail)
        {
            getGameState().lastRailroadOption = trackType | (isRoad ? (1 << 7) : 0);
        }
        else
        {
            getGameState().lastRoadOption = trackType | (isRoad ? (1 << 7) : 0);
        }

        // The window number doesn't really matter as there is only one top toolbar
        WindowManager::invalidate(WindowType::topToolbar, 0);
    }

    // 0x4C2BFD
    static void drawTransportTypeTabs(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();
        auto companyColour = CompanyManager::getCompanyColour(CompanyId(window.number));

        for (const auto& tab : _transportTypeTabInformation)
        {
            auto frameNo = 0;
            if (_transportTypeTabInformation[window.currentTab].type == tab.type)
            {
                frameNo = (window.frameNo / 2) & 0xF;
            }
            uint32_t image = Gfx::recolour(skin->img + tab.imageIds[frameNo], companyColour);
            Widget::drawTab(window, drawingCtx, image, tab.widgetIndex);
        }
    }

    // 0x4C28F1
    static void drawTrackTypeTabs(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();
        auto companyColour = CompanyManager::getCompanyColour(CompanyId(window.number));

        auto left = window.x;
        auto top = window.y + 69;
        auto right = left + window.width - 187;
        auto bottom = top;
        drawingCtx.fillRect(left, top, right, bottom, Colours::getShade(window.getColour(WindowColour::secondary).c(), 7), Gfx::RectFlags::none);

        left = window.x + window.width - 187;
        top = window.y + 41;
        right = left;
        bottom = top + 27;
        drawingCtx.fillRect(left, top, right, bottom, Colours::getShade(window.getColour(WindowColour::secondary).c(), 7), Gfx::RectFlags::none);

        for (uint32_t tab = 0; tab < _numTrackTypeTabs; ++tab)
        {
            const auto widget = window.widgets[tab + widx::tab_track_type_0];
            if (window.currentSecondaryTab == tab)
            {
                left = widget.left + window.x + 1;
                top = widget.top + window.y + 26;
                right = left + 29;
                bottom = top;
                drawingCtx.fillRect(left, top, right, bottom, Colours::getShade(window.getColour(WindowColour::secondary).c(), 5), Gfx::RectFlags::none);
            }

            auto img = 0;
            auto type = _trackTypesForTab[tab];
            if (type == 0xFF)
            {
                if (window.currentTab == (widx::tab_build_new_aircraft - widx::tab_build_new_trains))
                {
                    img = skin->img + InterfaceSkin::ImageIds::toolbar_menu_airport;
                }
                else
                {
                    img = skin->img + InterfaceSkin::ImageIds::toolbar_menu_ship_port;
                }
                // Original saved the company colour in the img but didn't set the recolour flag
            }
            else if (type & (1 << 7)) // is_road
            {
                type &= ~(1 << 7);
                auto roadObj = ObjectManager::get<RoadObject>(type);
                img = roadObj->image;
                if (window.currentSecondaryTab == tab)
                {
                    img += (window.frameNo / 4) & 0x1F;
                }
                img = Gfx::recolour(img, companyColour);
            }
            else
            {
                auto trackObj = ObjectManager::get<TrackObject>(type);
                img = trackObj->image + TrackObj::ImageIds::kUiPreviewImage0;
                if (window.currentSecondaryTab == tab)
                {
                    // TODO: Use array from Construction/Common.cpp
                    img += (window.frameNo / 4) & 0xF;
                }
                img = Gfx::recolour(img, companyColour);
            }

            Widget::drawTab(window, drawingCtx, img, tab + widx::tab_track_type_0);
        }
    }

    static bool keyUp(Window& w, uint32_t charCode, uint32_t keyCode)
    {
        if (!Input::isFocused(w.type, w.number, widx::searchBox))
        {
            return false;
        }

        if (!inputSession.handleInput(charCode, keyCode))
        {
            return false;
        }

        int containerWidth = _widgets[widx::searchBox].width() - 2;
        if (inputSession.needsReoffsetting(containerWidth))
        {
            inputSession.calculateTextOffset(containerWidth);
        }

        inputSession.cursorFrame = 0;

        sub_4B92A5(&w);

        w.initScrollWidgets();
        w.invalidate();
        return true;
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onResize = onResize,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = onScrollMouseDown,
        .scrollMouseOver = onScrollMouseOver,
        .tooltip = tooltip,
        .cursor = cursor,
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
