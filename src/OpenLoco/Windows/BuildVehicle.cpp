#include "../CompanyManager.h"
#include "../Config.h"
#include "../Economy/Economy.h"
#include "../Entities/EntityManager.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Map/Map.hpp"
#include "../Objects/CargoObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadExtraObject.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackExtraObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/VehicleObject.h"
#include "../OpenLoco.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include "../Widget.h"
#include <algorithm>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::BuildVehicle
{
    static const Ui::Size window_size = { 380, 233 };

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
        scrollview_vehicle_preview
    };

    enum scrollIdx
    {
        vehicle_selection,
        vehicle_preview
    };

    static const uint32_t trainTabImages[16]{
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

    static const uint32_t aircraftTabImages[16]{
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

    static const uint32_t busTabImages[16]{
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

    static const uint32_t tramTabImages[16]{
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

    static const uint32_t truckTabImages[16]{
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

    static const uint32_t shipTabImages[16]{
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
        { VehicleType::train, tab_build_new_trains, trainTabImages },
        { VehicleType::bus, tab_build_new_buses, busTabImages },
        { VehicleType::truck, tab_build_new_trucks, truckTabImages },
        { VehicleType::tram, tab_build_new_trams, tramTabImages },
        { VehicleType::aircraft, tab_build_new_aircraft, aircraftTabImages },
        { VehicleType::ship, tab_build_new_ships, shipTabImages }
    };

    // 0x5231D0
    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 380, 233 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 378, 13 }, WidgetType::caption_24, WindowColour::primary),
        makeWidget({ 365, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 41 }, { 380, 192 }, WidgetType::panel, WindowColour::secondary),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_train_vehicles),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_buses),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_trucks),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_trams),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_aircraft),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_build_new_ships),
        makeRemapWidget({ 5, 43 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 36, 43 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 67, 43 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 98, 43 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 129, 43 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 160, 43 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 191, 43 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 222, 43 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeWidget({ 3, 72 }, { 374, 146 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        makeWidget({ 250, 44 }, { 180, 66 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::none),
        widgetEnd(),
    };

    static constexpr uint32_t widxToTrackTypeTab(WidgetIndex_t widgetIndex)
    {
        return widgetIndex - widx::tab_track_type_0;
    }

    static loco_global<int16_t, 0x01136268> _numAvailableVehicles;
    static loco_global<uint16_t[ObjectManager::getMaxObjects(ObjectType::vehicle)], 0x0113626A> _availableVehicles;
    static loco_global<EntityId, 0x0113642A> _113642A;
    static loco_global<int32_t, 0x011364E8> _buildTargetVehicle; // -1 for no target VehicleHead
    static loco_global<uint32_t, 0x011364EC> _numTrackTypeTabs;
    // Array of types if 0xFF then no type, flag (1<<7) as well
    static loco_global<uint8_t[widxToTrackTypeTab(widx::tab_track_type_7) + 1], 0x011364F0> _trackTypesForTab;
    static std::array<uint16_t, 6> _scrollRowHeight = { { 22, 22, 22, 22, 42, 30 } };

    loco_global<uint16_t[8], 0x112C826> _common_format_args;
    static loco_global<uint8_t, 0x00525FC5> _525FC5;
    static loco_global<uint8_t, 0x00525FAA> last_railroad_option;
    static loco_global<uint8_t, 0x00525FAB> last_road_option;
    static loco_global<uint8_t, 0x0052622C> last_build_vehicles_option; // Type is VehicleType
    static loco_global<uint16_t, 0x0052622E> _52622E;                   // Tick related

    static WindowEventList _events;

    static void setDisabledTransportTabs(Ui::Window* window);
    static void setTrackTypeTabs(Ui::Window* window);
    static void resetTrackTypeTabSelection(Ui::Window* window);
    static void setTopToolbarLastTrack(uint8_t trackType, bool isRoad);
    static void setTransportTypeTabs(Ui::Window* window);
    static void drawVehicleOverview(Gfx::Context* context, int16_t vehicleTypeIdx, CompanyId company, uint8_t eax, uint8_t esi, Ui::Point offset);
    static int16_t drawVehicleInline(Gfx::Context* context, int16_t vehicleTypeIdx, uint8_t unk_1, CompanyId company, Ui::Point loc);
    static void drawTransportTypeTabs(Ui::Window* window, Gfx::Context* context);
    static void drawTrackTypeTabs(Ui::Window* window, Gfx::Context* context);

    static void initEvents();

    // 0x4C1C64
    static Window* create(CompanyId company)
    {
        initEvents();
        auto window = WindowManager::createWindow(WindowType::buildVehicle, window_size, WindowFlags::flag_11, &_events);
        window->widgets = _widgets;
        window->number = enumValue(company);
        window->enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_track_type_0) | (1 << widx::tab_track_type_1) | (1 << widx::tab_track_type_2) | (1 << widx::tab_track_type_3) | (1 << widx::tab_track_type_4) | (1 << widx::tab_track_type_5) | (1 << widx::tab_track_type_6) | (1 << widx::tab_track_type_7) | (1 << widx::scrollview_vehicle_selection);
        window->owner = CompanyManager::getControllingId();
        window->frame_no = 0;
        auto skin = OpenLoco::ObjectManager::get<InterfaceSkinObject>();
        if (skin != nullptr)
        {
            window->setColour(WindowColour::secondary, skin->colour_0A);
        }
        setDisabledTransportTabs(window);
        return window;
    }

    /* 0x4C1AF7
     * depending on flags (1<<31) vehicle is a tab id or a VehicleHead thing_id
     */
    Window* open(uint32_t vehicle, uint32_t flags)
    {
        auto window = WindowManager::bringToFront(WindowType::buildVehicle, enumValue(CompanyManager::getControllingId()));
        bool tabMode = flags & (1 << 31);
        if (window)
        {
            WidgetIndex_t tab = widx::tab_build_new_trains;
            if (!tabMode)
            {
                auto veh = EntityManager::get<Vehicles::VehicleHead>(EntityId(vehicle));
                if (veh == nullptr)
                {
                    return nullptr;
                }
                tab += static_cast<uint8_t>(veh->vehicleType);
            }
            else
            {
                // Not a vehicle but a type
                tab += vehicle;
            }
            window->callOnMouseUp(tab);

            if (tabMode)
            {
                _buildTargetVehicle = -1;
            }
            else
            {
                _buildTargetVehicle = vehicle;
            }
        }
        else
        {
            window = create(CompanyManager::getControllingId());
            window->width = window_size.width;
            window->height = window_size.height;
            _buildTargetVehicle = -1;
            if (!tabMode)
            {
                _buildTargetVehicle = vehicle;
                auto veh = EntityManager::get<Vehicles::VehicleHead>(EntityId(vehicle));
                if (veh == nullptr)
                {
                    WindowManager::close(window);
                    return nullptr;
                }
                window->currentTab = static_cast<uint8_t>(veh->vehicleType);
            }
            else
            {
                window->currentTab = vehicle;
            }

            window->rowHeight = _scrollRowHeight[window->currentTab];
            window->rowCount = 0;
            window->var_83C = 0;
            window->rowHover = -1;
            window->invalidate();
            window->widgets = _widgets;
            window->enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_track_type_0) | (1 << widx::tab_track_type_1) | (1 << widx::tab_track_type_2) | (1 << widx::tab_track_type_3) | (1 << widx::tab_track_type_4) | (1 << widx::tab_track_type_5) | (1 << widx::tab_track_type_6) | (1 << widx::tab_track_type_7) | (1 << widx::scrollview_vehicle_selection);
            window->holdableWidgets = 0;
            window->eventHandlers = &_events;
            window->activatedWidgets = 0;
            setDisabledTransportTabs(window);
            setTrackTypeTabs(window);
            resetTrackTypeTabSelection(window);
            sub_4B92A5(window);

            window->callOnResize();
            window->callPrepareDraw();
            window->initScrollWidgets();
        }

        if (_buildTargetVehicle == -1)
        {
            return window;
        }
        auto veh = EntityManager::get<Vehicles::VehicleBase>(EntityId(*_buildTargetVehicle));
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
                targetTrackType = _525FC5;
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

        window->callOnMouseUp(widgetIndex);
        return window;
    }

    void registerHooks()
    {
        registerHook(
            0x004B92A5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4B92A5((Ui::Window*)regs.esi);
                regs = backup;
                return 0;
            });

        registerHook(
            0x4C1AF7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = open(regs.eax, regs.eax);
                regs = backup;
                regs.esi = X86Pointer(window);
                return 0;
            });
    }

    /* 0x4B9165
     * Works out which vehicles are able to be built for this vehicle_type or vehicle
     */
    static void generateBuildableVehiclesArray(VehicleType vehicleType, uint8_t trackType, Vehicles::VehicleBase* vehicle)
    {
        if (trackType != 0xFF && (trackType & (1 << 7)))
        {
            auto trackIdx = trackType & ~(1 << 7);
            auto roadObj = ObjectManager::get<RoadObject>(trackIdx);
            if (roadObj->flags & Flags12::unk_03)
            {
                trackType = 0xFE;
            }
        }

        auto companyId = CompanyManager::getControllingId();
        if (vehicle != nullptr)
        {
            companyId = vehicle->owner;
        }
        _numAvailableVehicles = 0;
        struct build_item
        {
            uint16_t vehicle_index;
            bool isPowered;
            uint16_t designed;
        };
        std::vector<build_item> buildableVehicles;

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
            if (!Config::getNew().displayLockedVehicles && !company->isVehicleIndexUnlocked(vehicleObjIndex))
            {
                continue;
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

            buildableVehicles.push_back({ vehicleObjIndex, vehicleObj->power > 0, vehicleObj->designed });
        }

        std::sort(buildableVehicles.begin(), buildableVehicles.end(), [](const build_item& item1, const build_item& item2) { return item1.designed < item2.designed; });
        std::stable_sort(buildableVehicles.begin(), buildableVehicles.end(), [](const build_item& item1, const build_item& item2) { return item1.isPowered > item2.isPowered; });
        for (size_t i = 0; i < buildableVehicles.size(); ++i)
        {
            _availableVehicles[i] = buildableVehicles[i].vehicle_index;
        }
        _numAvailableVehicles = static_cast<int16_t>(buildableVehicles.size());
    }

    static Ui::Window* getTopEditingVehicleWindow()
    {
        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            if (w->currentTab != 1)
                continue;

            auto vehicle = EntityManager::get<Vehicles::VehicleBase>(EntityId(w->number));
            if (vehicle == nullptr)
                continue;

            if (vehicle->owner != CompanyManager::getControllingId())
                continue;

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
            veh = EntityManager::get<Vehicles::VehicleBase>(EntityId(*_buildTargetVehicle));
        }

        generateBuildableVehiclesArray(vehicleType, trackType, veh);

        int numRows = _numAvailableVehicles;
        if (window->var_83C == numRows)
            return;

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
    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(window);
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
                    Input::toolCancel(window->type, window->number);
                }

                auto newTab = widgetIndex - widx::tab_build_new_trains;
                window->currentTab = newTab;
                window->rowHeight = _scrollRowHeight[newTab];
                window->frame_no = 0;
                window->currentSecondaryTab = 0;
                if (newTab != last_build_vehicles_option)
                {
                    last_build_vehicles_option = newTab;
                    WindowManager::invalidate(WindowType::topToolbar, 0);
                }

                auto curViewport = window->viewports[0];
                window->viewports[0] = 0;
                if (curViewport != 0)
                {
                    curViewport->width = 0;
                }

                window->enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_track_type_0) | (1 << widx::tab_track_type_1) | (1 << widx::tab_track_type_2) | (1 << widx::tab_track_type_3) | (1 << widx::tab_track_type_4) | (1 << widx::tab_track_type_5) | (1 << widx::tab_track_type_6) | (1 << widx::tab_track_type_7) | (1 << widx::scrollview_vehicle_selection);
                window->holdableWidgets = 0;
                window->eventHandlers = &_events;
                window->widgets = _widgets;
                setDisabledTransportTabs(window);
                window->invalidate();
                _buildTargetVehicle = -1;
                setTrackTypeTabs(window);
                resetTrackTypeTabSelection(window);
                window->rowCount = 0;
                window->var_83C = 0;
                window->rowHover = -1;
                window->callOnResize();
                window->callOnPeriodicUpdate();
                window->callPrepareDraw();
                window->initScrollWidgets();
                window->invalidate();
                window->moveInsideScreenEdges();
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
                if (window->currentSecondaryTab == tab)
                    break;

                window->currentSecondaryTab = tab;
                setTopToolbarLastTrack(_trackTypesForTab[tab] & ~(1 << 7), _trackTypesForTab[tab] & (1 << 7));
                _buildTargetVehicle = -1;
                window->rowCount = 0;
                window->var_83C = 0;
                window->rowHover = -1;
                window->callOnResize();
                window->callOnPeriodicUpdate();
                window->callPrepareDraw();
                window->initScrollWidgets();
                window->invalidate();
                break;
            }
        }
    }

    // 0x4C3929
    static void onResize(Window* window)
    {
        window->flags |= WindowFlags::resizable;
        auto minWidth = std::max<int16_t>(_numTrackTypeTabs * 31 + 195, 380);
        window->minWidth = minWidth;
        window->maxWidth = 520;
        window->minHeight = 233;
        window->maxHeight = 600;
        if (window->width < minWidth)
        {
            window->width = minWidth;
            window->invalidate();
        }

        if (window->height < window->minHeight)
        {
            window->height = window->minHeight;
            window->invalidate();
        }

        auto scrollPosition = window->scrollAreas[scrollIdx::vehicle_selection].contentHeight;
        scrollPosition -= window->widgets[widx::scrollview_vehicle_selection].bottom;
        scrollPosition += window->widgets[widx::scrollview_vehicle_selection].top;
        if (scrollPosition < 0)
        {
            scrollPosition = 0;
        }

        if (scrollPosition < window->scrollAreas[scrollIdx::vehicle_selection].contentOffsetY)
        {
            window->scrollAreas[scrollIdx::vehicle_selection].contentOffsetY = scrollPosition;
            Ui::ScrollView::updateThumbs(window, widx::scrollview_vehicle_selection);
        }

        if (window->rowHover != -1)
        {
            return;
        }

        if (window->var_83C == 0)
        {
            return;
        }

        window->rowHover = window->rowInfo[0];
        window->invalidate();
    }

    // 0x4C3923
    static void onPeriodicUpdate(Window* window)
    {
        sub_4B92A5(window);
    }

    // 0x4C377B
    static void onUpdate(Window* window)
    {
        window->frame_no++;
        window->callPrepareDraw();

        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, window->currentTab + 4);
        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, (window->currentSecondaryTab & 0xFF) + 10);
        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, 19);
    }

    // 0x4C37B9
    static void getScrollSize(Ui::Window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window->var_83C * window->rowHeight;
    }

    // 0x4C384B
    static void onScrollMouseDown(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        if (scroll_index != scrollIdx::vehicle_selection)
        {
            return;
        }

        auto scrollItem = y / window->rowHeight;
        if (scrollItem >= window->var_83C)
        {
            return;
        }

        auto pan = window->width / 2 + window->x;
        Audio::playSound(Audio::SoundId::clickDown, Map::Pos3{ x, y, static_cast<int16_t>(pan) }, pan);
        auto item = window->rowInfo[scrollItem];
        auto vehicleObj = ObjectManager::get<VehicleObject>(item);
        FormatArguments args{};
        // Skip 5 * 2 bytes
        args.skip(10);
        args.push(vehicleObj->name);
        GameCommands::setErrorTitle(StringIds::cant_build_pop_5_string_id);
        if (_buildTargetVehicle != -1)
        {
            auto vehicle = EntityManager::get<Vehicles::VehicleHead>(EntityId(*_buildTargetVehicle));
            if (vehicle != nullptr)
            {
                args.push(vehicle->name);
                args.push(vehicle->ordinalNumber);
                GameCommands::setErrorTitle(StringIds::cant_add_pop_5_string_id_string_id);
            }
        }

        if (GameCommands::do_5(item, EntityId(*_buildTargetVehicle)) == GameCommands::FAILURE)
        {
            return;
        }

        if (_buildTargetVehicle == -1)
        {
            auto vehicle = EntityManager::get<Vehicles::VehicleBase>(_113642A);
            Vehicle::Details::open(vehicle);
        }
        sub_4B92A5(window);
    }

    // 0x4C3802
    static void onScrollMouseOver(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        if (scroll_index != scrollIdx::vehicle_selection)
        {
            return;
        }

        auto scrollItem = y / window->rowHeight;
        int16_t item = -1;
        if (scrollItem < window->var_83C)
        {
            item = window->rowInfo[scrollItem];
        }

        if (item != -1 && item != window->rowHover)
        {
            window->rowHover = item;
            window->invalidate();
        }
    }

    // 0x4C370C
    static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex)
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
                if (_transportTypeTabInformation[window->currentTab].type == VehicleType::aircraft)
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
    static Ui::CursorId cursor(Window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        if (widgetIdx != widx::scrollview_vehicle_selection)
        {
            return fallback;
        }

        auto scrollItem = yPos / window->rowHeight;
        if (scrollItem >= window->var_83C)
        {
            return fallback;
        }

        if (window->rowInfo[scrollItem] == -1)
        {
            return fallback;
        }

        return CursorId::handPointer;
    }

    // 0x4C2E5C
    static void prepareDraw(Ui::Window* window)
    {
        if (window->widgets != _widgets)
        {
            window->widgets = _widgets;
            window->initScrollWidgets();
        }

        // Mask off all the tabs
        auto activeWidgets = window->activatedWidgets & ((1 << frame) | (1 << caption) | (1 << close_button) | (1 << panel) | (1 << scrollview_vehicle_selection) | (1 << scrollview_vehicle_preview));
        // Only activate the singular tabs
        activeWidgets |= 1ULL << _transportTypeTabInformation[window->currentTab].widgetIndex;
        activeWidgets |= 1ULL << (window->currentSecondaryTab + widx::tab_track_type_0);

        window->activatedWidgets = activeWidgets;

        window->widgets[widx::caption].text = window->currentTab + StringIds::build_trains;

        auto width = window->width;
        auto height = window->height;

        window->widgets[widx::frame].right = width - 1;
        window->widgets[widx::frame].bottom = height - 1;

        window->widgets[widx::panel].right = width - 1;
        window->widgets[widx::panel].bottom = height - 1;

        window->widgets[widx::caption].right = width - 2;

        window->widgets[widx::close_button].left = width - 15;
        window->widgets[widx::close_button].right = width - 3;

        window->widgets[widx::scrollview_vehicle_preview].right = width - 4;
        window->widgets[widx::scrollview_vehicle_preview].left = width - 184;

        window->widgets[widx::scrollview_vehicle_selection].right = width - 187;
        window->widgets[widx::scrollview_vehicle_selection].bottom = height - 14;

        setTransportTypeTabs(window);
    }

    // 0x4C2F23
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        window->draw(context);
        drawTransportTypeTabs(window, context);
        drawTrackTypeTabs(window, context);

        {
            auto x = window->x + 2;
            auto y = window->y + window->height - 13;
            auto bottomLeftMessage = StringIds::select_new_vehicle;
            FormatArguments args{};
            if (_buildTargetVehicle != -1)
            {
                auto vehicle = EntityManager::get<Vehicles::VehicleHead>(EntityId(*_buildTargetVehicle));
                if (vehicle != nullptr)
                {
                    args.push(vehicle->name);
                    args.push(vehicle->ordinalNumber);
                    bottomLeftMessage = StringIds::select_vehicle_to_add_to_string_id;
                }
            }

            Gfx::drawString_494BBF(*context, x, y, window->width - 186, Colour::black, bottomLeftMessage, &args);
        }

        if (window->rowHover == -1)
        {
            return;
        }

        auto vehicleObj = ObjectManager::get<VehicleObject>(window->rowHover);
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));

        {
            auto cost = Economy::getInflationAdjustedCost(vehicleObj->cost_factor, vehicleObj->cost_index, 6);
            FormatArguments args{};
            args.push(cost);
            buffer = StringManager::formatString(buffer, StringIds::stats_cost, &args);
        }

        {
            auto runningCost = Economy::getInflationAdjustedCost(vehicleObj->run_cost_factor, vehicleObj->run_cost_index, 10);
            FormatArguments args{};
            args.push(runningCost);
            buffer = StringManager::formatString(buffer, StringIds::stats_running_cost, &args);
        }

        if (vehicleObj->designed != 0)
        {
            FormatArguments args{};
            args.push(vehicleObj->designed);

            const auto* company = CompanyManager::get(CompanyManager::getControllingId());
            auto unlocked = company->isVehicleIndexUnlocked(window->rowHover);
            buffer = StringManager::formatString(
                buffer,
                unlocked ? StringIds::stats_designed : StringIds::stats_proposed_design,
                &args);
        }

        if (vehicleObj->obsolete != 0 && vehicleObj->obsolete != std::numeric_limits<uint16_t>::max())
        {
            FormatArguments args{};
            args.push(vehicleObj->obsolete);
            buffer = StringManager::formatString(buffer, StringIds::stats_obsolete, &args);
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

            for (auto i = 0; i < vehicleObj->num_mods; ++i)
            {
                strcpy(buffer, " + ");
                buffer += 3;
                if (vehicleObj->mode == TransportMode::road)
                {
                    auto roadExtraObj = ObjectManager::get<RoadExtraObject>(vehicleObj->required_track_extras[i]);
                    buffer = StringManager::formatString(buffer, roadExtraObj->name);
                }
                else
                {
                    auto trackExtraObj = ObjectManager::get<TrackExtraObject>(vehicleObj->required_track_extras[i]);
                    buffer = StringManager::formatString(buffer, trackExtraObj->name);
                }
            }

            if (vehicleObj->flags & FlagsE0::rack_rail)
            {
                auto trackExtraObj = ObjectManager::get<TrackExtraObject>(vehicleObj->rack_rail_type);
                FormatArguments args{};
                args.push(trackExtraObj->name);
                buffer = StringManager::formatString(buffer, StringIds::stats_string_steep_slope, &args);
            }
        }

        if (vehicleObj->power != 0)
        {
            if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
            {
                FormatArguments args{};
                args.push(vehicleObj->power);
                buffer = StringManager::formatString(buffer, StringIds::stats_power, &args);
            }
        }

        {
            FormatArguments args{};
            args.push(vehicleObj->weight);
            buffer = StringManager::formatString(buffer, StringIds::stats_weight, &args);
        }
        {
            FormatArguments args{};
            args.push(vehicleObj->speed.getRaw());
            buffer = StringManager::formatString(buffer, StringIds::stats_max_speed, &args);
        }
        if (vehicleObj->flags & FlagsE0::rack_rail)
        {
            auto trackExtraObj = ObjectManager::get<TrackExtraObject>(vehicleObj->rack_rail_type);
            FormatArguments args{};
            args.push(vehicleObj->rack_speed);
            args.push(trackExtraObj->name);
            buffer = StringManager::formatString(buffer, StringIds::stats_velocity_on_string, &args);
        }

        vehicleObj->getCargoString(buffer);

        auto x = window->widgets[widx::scrollview_vehicle_selection].right + window->x + 2;
        auto y = window->widgets[widx::scrollview_vehicle_preview].bottom + window->y + 2;
        Gfx::drawString_495224(*context, x, y, 180, Colour::black, StringIds::buffer_1250);
    }

    // 0x4C3307
    static void drawScroll(Ui::Window& window, Gfx::Context& context, const uint32_t scrollIndex)
    {
        switch (scrollIndex)
        {
            case scrollIdx::vehicle_selection:
            {
                auto colour = Colours::getShade(window.getColour(WindowColour::secondary).c(), 4);
                Gfx::clear(context, colour * 0x01010101);
                if (window.var_83C == 0)
                {
                    auto defaultMessage = StringIds::no_vehicles_available;
                    FormatArguments args{};
                    if (_buildTargetVehicle != -1)
                    {
                        auto vehicle = EntityManager::get<Vehicles::VehicleHead>(EntityId(*_buildTargetVehicle));
                        if (vehicle != nullptr)
                        {
                            defaultMessage = StringIds::no_compatible_vehicles_available;
                            args.push(vehicle->name);
                            args.push(vehicle->ordinalNumber);
                        }
                    }

                    auto widget = window.widgets[widx::scrollview_vehicle_selection];
                    auto width = widget.right - widget.left - 17;
                    auto y = (window.rowHeight - 10) / 2;
                    Gfx::drawString_495224(context, 3, y, width, Colour::black, defaultMessage, &args);
                }
                else
                {
                    int16_t y = 0;
                    for (auto i = 0; i < window.var_83C; ++i, y += window.rowHeight)
                    {
                        if (y + window.rowHeight + 30 <= context.y)
                        {
                            continue;
                        }

                        if (y >= context.y + context.height + 30)
                        {
                            continue;
                        }

                        auto vehicleType = window.rowInfo[i];
                        if (vehicleType == -1)
                        {
                            continue;
                        }

                        const auto* company = CompanyManager::get(CompanyManager::getControllingId());
                        auto rowIsALockedVehicle = Config::getNew().displayLockedVehicles
                            && !company->isVehicleIndexUnlocked(vehicleType)
                            && !Config::getNew().buildLockedVehicles;

                        auto colouredString = StringIds::black_stringid;

                        const int32_t lockedHoverRowColour = 0x0100003D;
                        const int32_t normalHoverRowColour = 0x02000030;
                        const int32_t lockedRowColour = 0x0100003F;

                        if (window.rowHover == vehicleType)
                        {
                            if (rowIsALockedVehicle)
                            {
                                Gfx::fillRect(context, 0, y, window.width, y + window.rowHeight - 1, lockedHoverRowColour);
                            }
                            else
                            {
                                Gfx::fillRect(context, 0, y, window.width, y + window.rowHeight - 1, normalHoverRowColour);
                            }
                            colouredString = StringIds::wcolour2_stringid;
                        }
                        else
                        {
                            if (rowIsALockedVehicle)
                            {
                                Gfx::fillRect(context, 0, y, window.width, y + window.rowHeight - 1, lockedRowColour);
                            }
                        }

                        int16_t half = (window.rowHeight - 22) / 2;
                        auto x = drawVehicleInline(&context, vehicleType, 0, CompanyManager::getControllingId(), { 0, static_cast<int16_t>(y + half) });

                        auto vehicleObj = ObjectManager::get<VehicleObject>(vehicleType);
                        FormatArguments args{};
                        args.push(vehicleObj->name);
                        half = (window.rowHeight - 10) / 2;
                        Gfx::drawString_494B3F(context, x + 3, y + half, Colour::black, colouredString, &args);
                    }
                }
                break;
            }
            case scrollIdx::vehicle_preview:
            {
                auto colour = Colours::getShade(window.getColour(WindowColour::secondary).c(), 0);
                // Gfx::clear needs the colour copied to each byte of eax
                Gfx::clear(context, colour * 0x01010101);

                if (window.rowHover == -1)
                {
                    break;
                }

                uint8_t unk1 = _52622E & 0x3F;
                uint8_t unk2 = ((_52622E + 2) / 4) & 0x3F;
                drawVehicleOverview(&context, window.rowHover, CompanyManager::getControllingId(), unk1, unk2, { 90, 37 });

                auto vehicleObj = ObjectManager::get<VehicleObject>(window.rowHover);
                auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
                buffer = StringManager::formatString(buffer, vehicleObj->name);
                auto usableCargoTypes = vehicleObj->primary_cargo_types | vehicleObj->secondary_cargo_types;

                for (auto cargoTypes = Utility::bitScanForward(usableCargoTypes); cargoTypes != -1; cargoTypes = Utility::bitScanForward(usableCargoTypes))
                {
                    usableCargoTypes &= ~(1 << cargoTypes);
                    auto cargoObj = ObjectManager::get<CargoObject>(cargoTypes);
                    *buffer++ = ' ';
                    *buffer++ = ControlCodes::inline_sprite_str;
                    *(reinterpret_cast<uint32_t*>(buffer)) = cargoObj->unit_inline_sprite;
                    buffer += 4;
                }

                *buffer++ = '\0';
                FormatArguments args{};
                args.push(StringIds::buffer_1250);
                Gfx::drawStringCentredClipped(context, 89, 52, 177, Colour::darkOrange, StringIds::wcolour2_stringid, &args);
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
                    trackType = _525FC5;
                }
                roadTrackTypes |= (1 << trackType);
            }
            else
            {
                // Reset the tabs
                _trackTypesForTab[0] = 0xFF;
                _numTrackTypeTabs = 1;
                window->widgets[tab_track_type_0].type = WidgetType::tab;
                for (WidgetIndex_t j = tab_track_type_1; j <= tab_track_type_7; ++j)
                {
                    window->widgets[j].type = WidgetType::none;
                }
                return;
            }
        }

        WidgetIndex_t trackTypeTab = tab_track_type_0;
        auto trackType = 0;
        for (trackType = Utility::bitScanForward(railTrackTypes); trackType != -1 && trackTypeTab <= tab_track_type_7; trackType = Utility::bitScanForward(railTrackTypes))
        {
            railTrackTypes &= ~(1 << trackType);
            window->widgets[trackTypeTab].type = WidgetType::tab;
            _trackTypesForTab[widxToTrackTypeTab(trackTypeTab)] = trackType;
            trackTypeTab++;
        }

        if (trackType == -1 && trackTypeTab <= tab_track_type_7)
        {
            for (trackType = Utility::bitScanForward(roadTrackTypes); trackType != -1 && trackTypeTab <= tab_track_type_7; trackType = Utility::bitScanForward(roadTrackTypes))
            {
                roadTrackTypes &= ~(1 << trackType);
                window->widgets[trackTypeTab].type = WidgetType::tab;
                _trackTypesForTab[widxToTrackTypeTab(trackTypeTab)] = trackType | (1 << 7);
                trackTypeTab++;
            }
        }

        _numTrackTypeTabs = widxToTrackTypeTab(trackTypeTab);

        for (; trackTypeTab <= tab_track_type_7; ++trackTypeTab)
        {
            window->widgets[trackTypeTab].type = WidgetType::none;
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
            if (last_railroad_option == _trackTypesForTab[trackTab])
            {
                found = true;
                break;
            }

            if (last_road_option == _trackTypesForTab[trackTab])
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
            if (road_obj && road_obj->flags & Flags12::unk_01)
            {
                setRail = true;
            }
        }
        else
        {
            auto rail_obj = ObjectManager::get<TrackObject>(trackType);
            if (rail_obj && !(rail_obj->flags & Flags22::unk_02))
            {
                setRail = true;
            }
        }

        if (setRail)
        {
            last_railroad_option = trackType | (isRoad ? (1 << 7) : 0);
        }
        else
        {
            last_road_option = trackType | (isRoad ? (1 << 7) : 0);
        }

        // The window number doesn't really matter as there is only one top toolbar
        WindowManager::invalidate(WindowType::topToolbar, 0);
    }

    // 0x4C2865 common for build vehicle window and vehicle list
    static void setTransportTypeTabs(Ui::Window* window)
    {
        auto disabledWidgets = window->disabledWidgets >> widx::tab_build_new_trains;
        auto widget = window->widgets + widx::tab_build_new_trains;
        auto tabWidth = widget->right - widget->left;
        auto tabX = widget->left;
        for (auto i = 0; i <= widx::tab_build_new_ships - widx::tab_build_new_trains; ++i, ++widget)
        {
            if (disabledWidgets & (1ULL << i))
            {
                widget->type = WidgetType::none;
            }
            else
            {
                widget->type = WidgetType::tab;
                widget->left = tabX;
                widget->right = tabX + tabWidth;
                tabX += tabWidth + 1;
            }
        }
    }

    // 0x4C2BFD
    static void drawTransportTypeTabs(Ui::Window* window, Gfx::Context* context)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();
        auto companyColour = CompanyManager::getCompanyColour(CompanyId(window->number));

        for (const auto& tab : _transportTypeTabInformation)
        {
            auto frameNo = 0;
            if (_transportTypeTabInformation[window->currentTab].type == tab.type)
            {
                frameNo = (window->frame_no / 2) & 0xF;
            }
            uint32_t image = Gfx::recolour(skin->img + tab.imageIds[frameNo], companyColour);
            Widget::drawTab(window, context, image, tab.widgetIndex);
        }
    }

    // 0x4C28F1
    static void drawTrackTypeTabs(Ui::Window* window, Gfx::Context* context)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();
        auto companyColour = CompanyManager::getCompanyColour(CompanyId(window->number));

        auto left = window->x;
        auto top = window->y + 69;
        auto right = left + window->width - 187;
        auto bottom = top;
        Gfx::fillRect(*context, left, top, right, bottom, Colours::getShade(window->getColour(WindowColour::secondary).c(), 7));

        left = window->x + window->width - 187;
        top = window->y + 41;
        right = left;
        bottom = top + 27;
        Gfx::fillRect(*context, left, top, right, bottom, Colours::getShade(window->getColour(WindowColour::secondary).c(), 7));

        for (uint32_t tab = 0; tab < _numTrackTypeTabs; ++tab)
        {
            const auto widget = window->widgets[tab + widx::tab_track_type_0];
            if (window->currentSecondaryTab == tab)
            {
                left = widget.left + window->x + 1;
                top = widget.top + window->y + 26;
                right = left + 29;
                bottom = top;
                Gfx::fillRect(*context, left, top, right, bottom, Colours::getShade(window->getColour(WindowColour::secondary).c(), 5));
            }

            auto img = 0;
            auto type = _trackTypesForTab[tab];
            if (type == 0xFF)
            {
                if (window->currentTab == (widx::tab_build_new_aircraft - widx::tab_build_new_trains))
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
                if (window->currentSecondaryTab == tab)
                {
                    img += (window->frame_no / 4) & 0x1F;
                }
                img = Gfx::recolour(img, companyColour);
            }
            else
            {
                auto trackObj = ObjectManager::get<TrackObject>(type);
                img = trackObj->image;
                if (window->currentSecondaryTab == tab)
                {
                    img += (window->frame_no / 4) & 0xF;
                }
                img = Gfx::recolour(img, companyColour);
            }

            Widget::drawTab(window, context, img, tab + widx::tab_track_type_0);
        }
    }

    // 0x4B7741
    static void drawVehicleOverview(Gfx::Context* context, int16_t vehicleTypeIdx, CompanyId company, uint8_t eax, uint8_t esi, Ui::Point offset)
    {
        registers regs;
        regs.cx = offset.x;
        regs.dx = offset.y;
        regs.eax = eax;
        regs.esi = esi;
        regs.ebx = enumValue(company);
        regs.ebp = vehicleTypeIdx;
        regs.edi = X86Pointer(context);
        call(0x4B7741, regs);
    }

    // 0x4B7711
    static int16_t drawVehicleInline(Gfx::Context* context, int16_t vehicleTypeIdx, uint8_t unk_1, CompanyId company, Ui::Point loc)
    {
        registers regs;

        regs.al = unk_1;
        regs.ebx = enumValue(company);
        regs.cx = loc.x;
        regs.dx = loc.y;
        regs.edi = X86Pointer(context);
        regs.ebp = vehicleTypeIdx;
        call(0x4B7711, regs);
        // Returns right coordinate of the drawing
        return regs.cx;
    }

    static void initEvents()
    {
        _events.onMouseUp = onMouseUp;
        _events.onResize = onResize;
        _events.onPeriodicUpdate = onPeriodicUpdate;
        _events.onUpdate = onUpdate;
        _events.getScrollSize = getScrollSize;
        _events.scrollMouseDown = onScrollMouseDown;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.cursor = cursor;
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;
        _events.drawScroll = drawScroll;
    }
}
