#include "../CompanyManager.h"
#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/CargoObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Objects/RoadExtraObject.h"
#include "../Objects/RoadObject.h"
#include "../Objects/TrackExtraObject.h"
#include "../Objects/TrackObject.h"
#include "../Objects/VehicleObject.h"
#include "../OpenLoco.h"
#include "../Ptr.h"
#include "../Things/ThingManager.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::BuildVehicle
{
    static const Gfx::ui_size_t window_size = { 380, 233 };

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
    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 380, 233 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 378, 13 }, widget_type::caption_24, 0),
        makeWidget({ 365, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 41 }, { 380, 192 }, widget_type::panel, 1),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_build_new_train_vehicles),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_build_new_buses),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_build_new_trucks),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_build_new_trams),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_build_new_aircraft),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_build_new_ships),
        makeRemapWidget({ 5, 43 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 36, 43 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 67, 43 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 98, 43 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 129, 43 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 160, 43 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 191, 43 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeRemapWidget({ 222, 43 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_vehicles_for),
        makeWidget({ 3, 72 }, { 374, 146 }, widget_type::scrollview, 1, scrollbars::vertical),
        makeWidget({ 250, 44 }, { 180, 66 }, widget_type::scrollview, 1, scrollbars::none),
        widgetEnd(),
    };

    static constexpr uint32_t widxToTrackTypeTab(widget_index widgetIndex)
    {
        return widgetIndex - widx::tab_track_type_0;
    }

    static loco_global<int16_t, 0x01136268> _numAvailableVehicles;
    static loco_global<uint16_t[ObjectManager::getMaxObjects(object_type::vehicle)], 0x0113626A> _availableVehicles;
    static loco_global<uint16_t, 0x0113642A> _113642A;
    static loco_global<int32_t, 0x011364E8> _buildTargetVehicle; // -1 for no target VehicleHead
    static loco_global<uint32_t, 0x011364EC> _numTrackTypeTabs;
    // Array of types if 0xFF then no type, flag (1<<7) as well
    static loco_global<int8_t[widxToTrackTypeTab(widx::tab_track_type_7) + 1], 0x011364F0> _TrackTypesForTab;
    static std::array<uint16_t, 6> _scrollRowHeight = { { 22, 22, 22, 22, 42, 30 } };

    loco_global<uint16_t[8], 0x112C826> _common_format_args;
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;
    static loco_global<uint32_t[32], 0x00525E5E> currencyMultiplicationFactor;
    static loco_global<uint8_t, 0x00525FC5> _525FC5;
    static loco_global<uint8_t, 0x00525FAA> last_railroad_option;
    static loco_global<uint8_t, 0x00525FAB> last_road_option;
    static loco_global<uint8_t, 0x0052622C> last_build_vehicles_option; // Type is VehicleType
    static loco_global<uint16_t, 0x0052622E> _52622E;                   // Tick related

    static window_event_list _events;

    static void setDisabledTransportTabs(Ui::window* window);
    static void setTrackTypeTabs(Ui::window* window);
    static void resetTrackTypeTabSelection(Ui::window* window);
    static void setTopToolbarLastTrack(uint8_t trackType, bool isRoad);
    static void setTransportTypeTabs(Ui::window* window);
    static void drawVehicleOverview(Gfx::drawpixelinfo_t* dpi, int16_t vehicleTypeIdx, company_id_t company, uint8_t eax, uint8_t esi, Gfx::point_t offset);
    static int16_t drawVehicleInline(Gfx::drawpixelinfo_t* dpi, int16_t vehicleTypeIdx, uint8_t unk_1, company_id_t company, Gfx::point_t loc);
    static void drawTransportTypeTabs(Ui::window* window, Gfx::drawpixelinfo_t* dpi);
    static void drawTrackTypeTabs(Ui::window* window, Gfx::drawpixelinfo_t* dpi);

    static void initEvents();

    // 0x4C1C64
    static window* create(company_id_t company)
    {
        initEvents();
        auto window = WindowManager::createWindow(WindowType::buildVehicle, window_size, WindowFlags::flag_11, &_events);
        window->widgets = _widgets;
        window->number = company;
        window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_track_type_0) | (1 << widx::tab_track_type_1) | (1 << widx::tab_track_type_2) | (1 << widx::tab_track_type_3) | (1 << widx::tab_track_type_4) | (1 << widx::tab_track_type_5) | (1 << widx::tab_track_type_6) | (1 << widx::tab_track_type_7) | (1 << widx::scrollview_vehicle_selection);
        window->owner = CompanyManager::getControllingId();
        window->frame_no = 0;
        auto skin = OpenLoco::ObjectManager::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[1] = skin->colour_0A;
        }
        setDisabledTransportTabs(window);
        return window;
    }

    /* 0x4C1AF7
     * depending on flags (1<<31) vehicle is a tab id or a VehicleHead thing_id
     */
    window* open(uint32_t vehicle, uint32_t flags)
    {
        auto window = WindowManager::bringToFront(WindowType::buildVehicle, CompanyManager::getControllingId());
        bool tabMode = flags & (1 << 31);
        if (window)
        {
            widget_index tab = widx::tab_build_new_trains;
            if (!tabMode)
            {
                auto veh = ThingManager::get<Vehicles::VehicleHead>(vehicle);
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
                auto veh = ThingManager::get<Vehicles::VehicleHead>(vehicle);
                window->current_tab = static_cast<uint8_t>(veh->vehicleType);
            }
            else
            {
                window->current_tab = vehicle;
            }

            window->row_height = _scrollRowHeight[window->current_tab];
            window->row_count = 0;
            window->var_83C = 0;
            window->row_hover = -1;
            window->invalidate();
            window->widgets = _widgets;
            window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_track_type_0) | (1 << widx::tab_track_type_1) | (1 << widx::tab_track_type_2) | (1 << widx::tab_track_type_3) | (1 << widx::tab_track_type_4) | (1 << widx::tab_track_type_5) | (1 << widx::tab_track_type_6) | (1 << widx::tab_track_type_7) | (1 << widx::scrollview_vehicle_selection);
            window->holdable_widgets = 0;
            window->event_handlers = &_events;
            window->activated_widgets = 0;
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
        auto veh = ThingManager::get<Vehicles::VehicleBase>(_buildTargetVehicle);
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

        widget_index widgetIndex = widx::tab_track_type_0;
        for (uint32_t trackTypeTab = 0; trackTypeTab < _numTrackTypeTabs; trackTypeTab++)
        {
            if (targetTrackType == _TrackTypesForTab[trackTypeTab])
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
                sub_4B92A5(ToPtr(Ui::window, regs.esi));
                regs = backup;
                return 0;
            });

        registerHook(
            0x4C1AF7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = open(regs.eax, regs.eax);
                regs = backup;
                regs.esi = ToInt(window);
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
            auto roadObj = ObjectManager::get<road_object>(trackIdx);
            if (roadObj->flags & Flags12::unk_03)
            {
                trackType = 0xFE;
            }
        }

        auto companyId = CompanyManager::getControllingId();
        if (vehicle != nullptr)
        {
            companyId = vehicle->getOwner();
        }
        _numAvailableVehicles = 0;
        struct build_item
        {
            uint16_t vehicle_index;
            uint8_t power;
            uint16_t designed;
        };
        std::vector<build_item> buildableVehicles;

        for (uint16_t vehicleObjIndex = 0; vehicleObjIndex < ObjectManager::getMaxObjects(object_type::vehicle); ++vehicleObjIndex)
        {
            auto vehicleObj = ObjectManager::get<vehicle_object>(vehicleObjIndex);
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

            // Is vehicle type unlocked
            if (!(CompanyManager::get(companyId)->unlocked_vehicles[vehicleObjIndex >> 5] & (1 << (vehicleObjIndex & 0x1F))))
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

                if (sanitisedTrackType != vehicleObj->track_type)
                {
                    continue;
                }
            }

            auto power = std::min<uint16_t>(vehicleObj->power, 1);
            // Unsure why power is only checked for first byte.
            buildableVehicles.push_back({ vehicleObjIndex, static_cast<uint8_t>(power), vehicleObj->designed });
        }

        std::sort(buildableVehicles.begin(), buildableVehicles.end(), [](const build_item& item1, const build_item& item2) { return item1.designed < item2.designed; });
        std::sort(buildableVehicles.begin(), buildableVehicles.end(), [](const build_item& item1, const build_item& item2) { return item1.power > item2.power; });
        for (size_t i = 0; i < buildableVehicles.size(); ++i)
        {
            _availableVehicles[i] = buildableVehicles[i].vehicle_index;
        }
        _numAvailableVehicles = static_cast<int16_t>(buildableVehicles.size());
    }

    static Ui::window* getTopEditingVehicleWindow()
    {
        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            if (w->current_tab != 1)
                continue;

            auto vehicle = ThingManager::get<Vehicles::VehicleBase>(w->number);
            if (vehicle->getOwner() != CompanyManager::getControllingId())
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
    void sub_4B92A5(Ui::window* window)
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

        VehicleType vehicleType = _transportTypeTabInformation[window->current_tab].type;
        uint8_t trackType = _TrackTypesForTab[window->current_secondary_tab];

        Vehicles::VehicleBase* veh = nullptr;
        if (_buildTargetVehicle != -1)
        {
            veh = ThingManager::get<Vehicles::VehicleBase>(_buildTargetVehicle);
        }

        generateBuildableVehiclesArray(vehicleType, trackType, veh);

        int numRows = _numAvailableVehicles;
        if (window->var_83C == numRows)
            return;

        uint16_t* src = _availableVehicles;
        int16_t* dest = window->row_info;
        window->var_83C = numRows;
        window->row_count = 0;
        while (numRows != 0)
        {
            *dest = *src;
            dest++;
            src++;
            numRows--;
        }
        window->row_hover = -1;
        window->invalidate();
    }

    // 0x4C3576
    static void onMouseUp(Ui::window* window, widget_index widgetIndex)
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

                if (Input::hasFlag(Input::input_flags::tool_active))
                {
                    Input::toolCancel(window->type, window->number);
                }

                auto newTab = widgetIndex - widx::tab_build_new_trains;
                window->current_tab = newTab;
                window->row_height = _scrollRowHeight[newTab];
                window->frame_no = 0;
                window->current_secondary_tab = 0;
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

                window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_track_type_0) | (1 << widx::tab_track_type_1) | (1 << widx::tab_track_type_2) | (1 << widx::tab_track_type_3) | (1 << widx::tab_track_type_4) | (1 << widx::tab_track_type_5) | (1 << widx::tab_track_type_6) | (1 << widx::tab_track_type_7) | (1 << widx::scrollview_vehicle_selection);
                window->holdable_widgets = 0;
                window->event_handlers = &_events;
                window->widgets = _widgets;
                setDisabledTransportTabs(window);
                window->invalidate();
                _buildTargetVehicle = -1;
                setTrackTypeTabs(window);
                resetTrackTypeTabSelection(window);
                window->row_count = 0;
                window->var_83C = 0;
                window->row_hover = -1;
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
                if (window->current_secondary_tab == tab)
                    break;

                window->current_secondary_tab = tab;
                setTopToolbarLastTrack(_TrackTypesForTab[tab] & ~(1 << 7), _TrackTypesForTab[tab] & (1 << 7));
                _buildTargetVehicle = -1;
                window->row_count = 0;
                window->var_83C = 0;
                window->row_hover = -1;
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
    static void onResize(window* window)
    {
        window->flags |= WindowFlags::resizable;
        auto minWidth = std::max<int16_t>(_numTrackTypeTabs * 31 + 195, 380);
        window->min_width = minWidth;
        window->max_width = 520;
        window->min_height = 233;
        window->max_height = 600;
        if (window->width < minWidth)
        {
            window->width = minWidth;
            window->invalidate();
        }

        if (window->height < window->min_height)
        {
            window->height = window->min_height;
            window->invalidate();
        }

        auto scrollPosition = window->scroll_areas[scrollIdx::vehicle_selection].contentHeight;
        scrollPosition -= window->widgets[widx::scrollview_vehicle_selection].bottom;
        scrollPosition += window->widgets[widx::scrollview_vehicle_selection].top;
        if (scrollPosition < 0)
        {
            scrollPosition = 0;
        }

        if (scrollPosition < window->scroll_areas[scrollIdx::vehicle_selection].contentOffsetY)
        {
            window->scroll_areas[scrollIdx::vehicle_selection].contentOffsetY = scrollPosition;
            Ui::ScrollView::updateThumbs(window, widx::scrollview_vehicle_selection);
        }

        if (window->row_hover != -1)
        {
            return;
        }

        if (window->var_83C == 0)
        {
            return;
        }

        window->row_hover = window->row_info[0];
        window->invalidate();
    }

    // 0x4C3923
    static void onPeriodicUpdate(window* window)
    {
        sub_4B92A5(window);
    }

    // 0x4C377B
    static void onUpdate(window* window)
    {
        window->frame_no++;
        window->callPrepareDraw();

        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, window->current_tab + 4);
        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, (window->current_secondary_tab & 0xFF) + 10);
        WindowManager::invalidateWidget(WindowType::buildVehicle, window->number, 19);
    }

    // 0x4C37B9
    static void getScrollSize(Ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window->var_83C * window->row_height;
    }

    // 0x4C384B
    static void onScrollMouseDown(Ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        if (scroll_index != scrollIdx::vehicle_selection)
        {
            return;
        }

        auto scrollItem = y / window->row_height;
        if (scrollItem >= window->var_83C)
        {
            return;
        }

        auto pan = window->width / 2 + window->x;
        Audio::playSound(Audio::sound_id::click_down, loc16{ x, y, static_cast<int16_t>(pan) }, pan);
        auto item = window->row_info[scrollItem];
        auto vehicleObj = ObjectManager::get<vehicle_object>(item);
        FormatArguments args{};
        // Skip 5 * 2 bytes
        args.skip(10);
        args.push(vehicleObj->name);
        gGameCommandErrorTitle = StringIds::cant_build_pop_5_string_id;
        if (_buildTargetVehicle != -1)
        {
            auto vehicle = ThingManager::get<Vehicles::VehicleHead>(_buildTargetVehicle);
            args.push(vehicle->var_22);
            args.push(vehicle->var_44);
            gGameCommandErrorTitle = StringIds::cant_add_pop_5_string_id_string_id;
        }

        if (GameCommands::do_5(item, _buildTargetVehicle) == GameCommands::FAILURE)
        {
            return;
        }

        if (_buildTargetVehicle == -1)
        {
            auto vehicle = ThingManager::get<Vehicles::VehicleBase>(_113642A);
            Vehicle::Details::open(vehicle);
        }
        sub_4B92A5(window);
    }

    // 0x4C3802
    static void onScrollMouseOver(Ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        if (scroll_index != scrollIdx::vehicle_selection)
        {
            return;
        }

        auto scrollItem = y / window->row_height;
        int16_t item = -1;
        if (scrollItem < window->var_83C)
        {
            item = window->row_info[scrollItem];
        }

        if (item != -1 && item != window->row_hover)
        {
            window->row_hover = item;
            window->invalidate();
        }
    }

    // 0x4C370C
    static std::optional<FormatArguments> tooltip(Ui::window* window, widget_index widgetIndex)
    {
        FormatArguments args{};
        if (widgetIndex < widx::tab_track_type_0 || widgetIndex >= widx::scrollview_vehicle_selection)
        {
            args.push(StringIds::tooltip_scroll_new_vehicle_list);
        }
        else
        {
            auto trackTypeTab = widxToTrackTypeTab(widgetIndex);
            auto type = _TrackTypesForTab[trackTypeTab];
            if (type == -1)
            {
                if (_transportTypeTabInformation[window->current_tab].type == VehicleType::aircraft)
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
                    auto roadObj = ObjectManager::get<road_object>(type);
                    args.push(roadObj->name);
                }
                else
                {
                    auto trackObj = ObjectManager::get<track_object>(type);
                    args.push(trackObj->name);
                }
            }
        }
        return args;
    }

    // 0x4C37CB
    static Ui::cursor_id cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::cursor_id fallback)
    {
        if (widgetIdx != widx::scrollview_vehicle_selection)
        {
            return fallback;
        }

        auto scrollItem = yPos / window->row_height;
        if (scrollItem >= window->var_83C)
        {
            return fallback;
        }

        if (window->row_info[scrollItem] == -1)
        {
            return fallback;
        }

        return cursor_id::hand_pointer;
    }

    // 0x4C2E5C
    static void prepareDraw(Ui::window* window)
    {
        if (window->widgets != _widgets)
        {
            window->widgets = _widgets;
            window->initScrollWidgets();
        }

        // Mask off all the tabs
        auto activeWidgets = window->activated_widgets & ((1 << frame) | (1 << caption) | (1 << close_button) | (1 << panel) | (1 << scrollview_vehicle_selection) | (1 << scrollview_vehicle_preview));
        // Only activate the singular tabs
        activeWidgets |= 1ULL << _transportTypeTabInformation[window->current_tab].widgetIndex;
        activeWidgets |= 1ULL << (window->current_secondary_tab + widx::tab_track_type_0);
        window->activated_widgets = activeWidgets;

        window->widgets[widx::caption].text = window->current_tab + StringIds::build_trains;

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
    static void draw(Ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        window->draw(dpi);
        drawTransportTypeTabs(window, dpi);
        drawTrackTypeTabs(window, dpi);

        {
            auto x = window->x + 2;
            auto y = window->y + window->height - 13;
            auto bottomLeftMessage = StringIds::select_new_vehicle;
            FormatArguments args{};
            if (_buildTargetVehicle != -1)
            {
                auto vehicle = ThingManager::get<Vehicles::VehicleHead>(_buildTargetVehicle);
                args.push(vehicle->var_22);
                args.push(vehicle->var_44);
                bottomLeftMessage = StringIds::select_vehicle_to_add_to_string_id;
            }

            Gfx::drawString_494BBF(*dpi, x, y, window->width - 186, Colour::black, bottomLeftMessage, &args);
        }

        if (window->row_hover == -1)
        {
            return;
        }

        auto vehicleObj = ObjectManager::get<vehicle_object>(window->row_hover);
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));

        {
            auto cost = (vehicleObj->cost_factor * currencyMultiplicationFactor[vehicleObj->cost_index]) / 64;
            FormatArguments args{};
            args.push(cost);
            buffer = StringManager::formatString(buffer, StringIds::stats_cost, &args);
        }

        {
            auto runningCost = (vehicleObj->run_cost_factor * currencyMultiplicationFactor[vehicleObj->run_cost_index]) / 1024;
            FormatArguments args{};
            args.push(runningCost);
            buffer = StringManager::formatString(buffer, StringIds::stats_running_cost, &args);
        }

        if (vehicleObj->designed != 0)
        {
            FormatArguments args{};
            args.push(vehicleObj->designed);
            buffer = StringManager::formatString(buffer, StringIds::stats_designed, &args);
        }

        if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
        {
            buffer = StringManager::formatString(buffer, StringIds::stats_requires);
            auto trackName = StringIds::road;
            if (vehicleObj->mode == TransportMode::road)
            {
                if (vehicleObj->track_type != 0xFF)
                {
                    trackName = ObjectManager::get<road_object>(vehicleObj->track_type)->name;
                }
            }
            else
            {
                trackName = ObjectManager::get<track_object>(vehicleObj->track_type)->name;
            }

            buffer = StringManager::formatString(buffer, trackName);

            for (auto i = 0; i < vehicleObj->num_mods; ++i)
            {
                strcpy(buffer, " + ");
                buffer += 3;
                if (vehicleObj->mode == TransportMode::road)
                {
                    auto roadExtraObj = ObjectManager::get<road_extra_object>(vehicleObj->required_track_extras[i]);
                    buffer = StringManager::formatString(buffer, roadExtraObj->name);
                }
                else
                {
                    auto trackExtraObj = ObjectManager::get<track_extra_object>(vehicleObj->required_track_extras[i]);
                    buffer = StringManager::formatString(buffer, trackExtraObj->name);
                }
            }

            if (vehicleObj->flags & FlagsE0::rack_rail)
            {
                auto trackExtraObj = ObjectManager::get<track_extra_object>(vehicleObj->rack_rail_type);
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
            auto trackExtraObj = ObjectManager::get<track_extra_object>(vehicleObj->rack_rail_type);
            FormatArguments args{};
            args.push(vehicleObj->rack_speed);
            args.push(trackExtraObj->name);
            buffer = StringManager::formatString(buffer, StringIds::stats_velocity_on_string, &args);
        }

        vehicleObj->getCargoString(buffer);

        auto x = window->widgets[widx::scrollview_vehicle_selection].right + window->x + 2;
        auto y = window->widgets[widx::scrollview_vehicle_preview].bottom + window->y + 2;
        Gfx::drawString_495224(*dpi, x, y, 180, Colour::black, StringIds::buffer_1250);
    }

    // 0x4C3307
    static void drawScroll(Ui::window* window, Gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        switch (scrollIndex)
        {
            case scrollIdx::vehicle_selection:
            {
                auto colour = Colour::getShade(window->colours[1], 4);
                Gfx::clear(*dpi, colour * 0x01010101);
                if (window->var_83C == 0)
                {
                    auto defaultMessage = StringIds::no_vehicles_available;
                    FormatArguments args{};
                    if (_buildTargetVehicle != -1)
                    {
                        auto vehicle = ThingManager::get<Vehicles::VehicleHead>(_buildTargetVehicle);
                        defaultMessage = StringIds::no_compatible_vehicles_available;
                        args.push(vehicle->var_22);
                        args.push(vehicle->var_44);
                    }

                    auto widget = window->widgets[widx::scrollview_vehicle_selection];
                    auto width = widget.right - widget.left - 17;
                    auto y = (window->row_height - 10) / 2;
                    Gfx::drawString_495224(*dpi, 3, y, width, Colour::black, defaultMessage, &args);
                }
                else
                {
                    int16_t y = 0;
                    for (auto i = 0; i < window->var_83C; ++i, y += window->row_height)
                    {
                        if (y + window->row_height + 30 <= dpi->y)
                        {
                            continue;
                        }

                        if (y >= dpi->y + dpi->height + 30)
                        {
                            continue;
                        }

                        auto vehicleType = window->row_info[i];
                        if (vehicleType == -1)
                        {
                            continue;
                        }

                        auto colouredString = StringIds::black_stringid;
                        if (window->row_hover == vehicleType)
                        {
                            Gfx::fillRect(dpi, 0, y, window->width, y + window->row_height - 1, 0x2000030);
                            colouredString = StringIds::wcolour2_stringid;
                        }

                        int16_t half = (window->row_height - 22) / 2;
                        auto x = drawVehicleInline(dpi, vehicleType, 0, CompanyManager::getControllingId(), { 0, static_cast<int16_t>(y + half) });

                        auto vehicleObj = ObjectManager::get<vehicle_object>(vehicleType);
                        FormatArguments args{};
                        args.push(vehicleObj->name);
                        half = (window->row_height - 10) / 2;
                        Gfx::drawString_494B3F(*dpi, x + 3, y + half, Colour::black, colouredString, &args);
                    }
                }
                break;
            }
            case scrollIdx::vehicle_preview:
            {
                auto colour = Colour::getShade(window->colours[1], 0);
                // Gfx::clear needs the colour copied to each byte of eax
                Gfx::clear(*dpi, colour * 0x01010101);

                if (window->row_hover == -1)
                {
                    break;
                }

                uint8_t unk1 = _52622E & 0x3F;
                uint8_t unk2 = ((_52622E + 2) / 4) & 0x3F;
                drawVehicleOverview(dpi, window->row_hover, CompanyManager::getControllingId(), unk1, unk2, { 90, 37 });

                auto vehicleObj = ObjectManager::get<vehicle_object>(window->row_hover);
                auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
                buffer = StringManager::formatString(buffer, vehicleObj->name);
                auto usableCargoTypes = vehicleObj->primary_cargo_types | vehicleObj->secondary_cargo_types;

                for (auto cargoTypes = Utility::bitScanForward(usableCargoTypes); cargoTypes != -1; cargoTypes = Utility::bitScanForward(usableCargoTypes))
                {
                    usableCargoTypes &= ~(1 << cargoTypes);
                    auto cargoObj = ObjectManager::get<cargo_object>(cargoTypes);
                    *buffer++ = ' ';
                    *buffer++ = ControlCodes::inline_sprite_str;
                    *(reinterpret_cast<uint32_t*>(buffer)) = cargoObj->unit_inline_sprite;
                    buffer += 4;
                }

                *buffer++ = '\0';
                FormatArguments args{};
                args.push(StringIds::buffer_1250);
                Gfx::drawStringCentredClipped(*dpi, 89, 52, 177, 0x20, StringIds::wcolour2_stringid, &args);
                break;
            }
        }
    }

    // 0x4C28D2
    static void setDisabledTransportTabs(Ui::window* window)
    {
        auto availableVehicles = CompanyManager::companies()[window->number].available_vehicles;
        // By shifting by 4 the available_vehicles flags align with the tabs flags
        auto disabledTabs = (availableVehicles << 4) ^ ((1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships));
        window->disabled_widgets = disabledTabs;
    }

    // 0x4C2D8A
    static void setTrackTypeTabs(Ui::window* window)
    {
        VehicleType currentTransportTabType = _transportTypeTabInformation[window->current_tab].type;
        generateBuildableVehiclesArray(currentTransportTabType, 0xFF, nullptr);

        auto railTrackTypes = 0;
        auto roadTrackTypes = 0;
        for (auto i = 0; i < _numAvailableVehicles; i++)
        {
            auto vehicleObj = ObjectManager::get<vehicle_object>(_availableVehicles[i]);
            if (vehicleObj && vehicleObj->mode == TransportMode::rail)
            {
                railTrackTypes |= (1 << vehicleObj->track_type);
            }
            else if (vehicleObj && vehicleObj->mode == TransportMode::road)
            {
                auto trackType = vehicleObj->track_type;
                if (trackType == 0xFF)
                {
                    trackType = _525FC5;
                }
                roadTrackTypes |= (1 << trackType);
            }
            else
            {
                // Reset the tabs
                _TrackTypesForTab[0] = -1;
                window->widgets[tab_track_type_0].type = widget_type::wt_8;
                for (widget_index j = tab_track_type_1; j <= tab_track_type_7; ++j)
                {
                    window->widgets[j].type = widget_type::none;
                }
                return;
            }
        }

        widget_index trackTypeTab = tab_track_type_0;
        auto trackType = 0;
        for (trackType = Utility::bitScanForward(railTrackTypes); trackType != -1 && trackTypeTab <= tab_track_type_7; trackType = Utility::bitScanForward(railTrackTypes))
        {
            railTrackTypes &= ~(1 << trackType);
            window->widgets[trackTypeTab].type = widget_type::wt_8;
            _TrackTypesForTab[widxToTrackTypeTab(trackTypeTab)] = trackType;
            trackTypeTab++;
        }

        if (trackType == -1 && trackTypeTab <= tab_track_type_7)
        {
            for (trackType = Utility::bitScanForward(roadTrackTypes); trackType != -1 && trackTypeTab <= tab_track_type_7; trackType = Utility::bitScanForward(roadTrackTypes))
            {
                roadTrackTypes &= ~(1 << trackType);
                window->widgets[trackTypeTab].type = widget_type::wt_8;
                _TrackTypesForTab[widxToTrackTypeTab(trackTypeTab)] = trackType | (1 << 7);
                trackTypeTab++;
            }
        }

        _numTrackTypeTabs = widxToTrackTypeTab(trackTypeTab);

        for (; trackTypeTab <= tab_track_type_7; ++trackTypeTab)
        {
            window->widgets[trackTypeTab].type = widget_type::none;
        }
    }

    // 0x4C1CBE
    // if previous track tab on previous transport type tab is also compatible keeps it on that track type
    static void resetTrackTypeTabSelection(Ui::window* window)
    {
        auto transportType = _transportTypeTabInformation[window->current_tab].type;
        if (transportType == VehicleType::aircraft || transportType == VehicleType::ship)
        {
            window->current_secondary_tab = 0;
            return;
        }

        bool found = false;
        uint32_t trackTab = 0;
        for (; trackTab < _numTrackTypeTabs; trackTab++)
        {
            if (last_railroad_option == _TrackTypesForTab[trackTab])
            {
                found = true;
                break;
            }

            if (last_road_option == _TrackTypesForTab[trackTab])
            {
                found = true;
                break;
            }
        }

        window->current_secondary_tab = found ? trackTab : 0;

        bool isRoad = _TrackTypesForTab[trackTab] & (1 << 7);
        uint8_t trackType = _TrackTypesForTab[trackTab] & ~(1 << 7);
        setTopToolbarLastTrack(trackType, isRoad);
    }

    // 0x4A3A06
    static void setTopToolbarLastTrack(uint8_t trackType, bool isRoad)
    {
        bool setRail = false;
        if (isRoad)
        {
            auto road_obj = ObjectManager::get<road_object>(trackType);
            if (road_obj && road_obj->flags & Flags12::unk_01)
            {
                setRail = true;
            }
        }
        else
        {
            auto rail_obj = ObjectManager::get<track_object>(trackType);
            if (rail_obj && !(rail_obj->flags & Flags22::unk_02))
            {
                setRail = true;
            }
        }

        if (setRail)
        {
            last_railroad_option = trackType;
        }
        else
        {
            last_road_option = trackType;
        }

        // The window number doesn't really matter as there is only one top toolbar
        WindowManager::invalidate(WindowType::topToolbar, 0);
    }

    // 0x4C2865 common for build vehicle window and vehicle list
    static void setTransportTypeTabs(Ui::window* window)
    {
        auto disabledWidgets = window->disabled_widgets >> widx::tab_build_new_trains;
        auto widget = window->widgets + widx::tab_build_new_trains;
        auto tabWidth = widget->right - widget->left;
        auto tabX = widget->left;
        for (auto i = 0; i <= widx::tab_build_new_ships - widx::tab_build_new_trains; ++i, ++widget)
        {
            if (disabledWidgets & (1ULL << i))
            {
                widget->type = widget_type::none;
            }
            else
            {
                widget->type = widget_type::wt_8;
                widget->left = tabX;
                widget->right = tabX + tabWidth;
                tabX += tabWidth + 1;
            }
        }
    }

    // 0x4C2BFD
    static void drawTransportTypeTabs(Ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        auto skin = ObjectManager::get<interface_skin_object>();
        auto companyColour = CompanyManager::getCompanyColour(window->number);

        for (auto tab : _transportTypeTabInformation)
        {
            auto frameNo = 0;
            if (_transportTypeTabInformation[window->current_tab].type == tab.type)
            {
                frameNo = (window->frame_no / 2) & 0xF;
            }
            uint32_t image = Gfx::recolour(skin->img + tab.imageIds[frameNo], companyColour);
            Widget::draw_tab(window, dpi, image, tab.widgetIndex);
        }
    }

    // 0x4C28F1
    static void drawTrackTypeTabs(Ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        auto skin = ObjectManager::get<interface_skin_object>();
        auto companyColour = CompanyManager::getCompanyColour(window->number);

        auto left = window->x;
        auto top = window->y + 69;
        auto right = left + window->width - 187;
        auto bottom = top;
        Gfx::fillRect(dpi, left, top, right, bottom, Colour::getShade(window->colours[1], 7));

        left = window->x + window->width - 187;
        top = window->y + 41;
        right = left;
        bottom = top + 27;
        Gfx::fillRect(dpi, left, top, right, bottom, Colour::getShade(window->colours[1], 7));

        for (uint32_t tab = 0; tab < _numTrackTypeTabs; ++tab)
        {
            const auto widget = window->widgets[tab + widx::tab_track_type_0];
            if (window->current_secondary_tab == tab)
            {
                left = widget.left + window->x + 1;
                top = widget.top + window->y + 26;
                right = left + 29;
                bottom = top;
                Gfx::fillRect(dpi, left, top, right, bottom, Colour::getShade(window->colours[1], 5));
            }

            auto img = 0;
            auto type = _TrackTypesForTab[tab];
            if (type == -1)
            {
                if (window->current_tab == (widx::tab_build_new_aircraft - widx::tab_build_new_trains))
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
                auto roadObj = ObjectManager::get<road_object>(type);
                img = roadObj->image;
                if (window->current_secondary_tab == tab)
                {
                    img += (window->frame_no / 4) & 0x1F;
                }
                img = Gfx::recolour(img, companyColour);
            }
            else
            {
                auto trackObj = ObjectManager::get<track_object>(type);
                img = trackObj->image;
                if (window->current_secondary_tab == tab)
                {
                    img += (window->frame_no / 4) & 0xF;
                }
                img = Gfx::recolour(img, companyColour);
            }

            Widget::draw_tab(window, dpi, img, tab + widx::tab_track_type_0);
        }
    }

    // 0x4B7741
    static void drawVehicleOverview(Gfx::drawpixelinfo_t* dpi, int16_t vehicleTypeIdx, company_id_t company, uint8_t eax, uint8_t esi, Gfx::point_t offset)
    {
        registers regs;
        regs.cx = offset.x;
        regs.dx = offset.y;
        regs.eax = eax;
        regs.esi = esi;
        regs.ebx = company;
        regs.ebp = vehicleTypeIdx;
        regs.edi = (uintptr_t)dpi;
        call(0x4B7741, regs);
    }

    // 0x4B7711
    static int16_t drawVehicleInline(Gfx::drawpixelinfo_t* dpi, int16_t vehicleTypeIdx, uint8_t unk_1, company_id_t company, Gfx::point_t loc)
    {
        registers regs;

        regs.al = unk_1;
        regs.ebx = company;
        regs.cx = loc.x;
        regs.dx = loc.y;
        regs.edi = (uintptr_t)dpi;
        regs.ebp = vehicleTypeIdx;
        call(0x4B7711, regs);
        // Returns right coordinate of the drawing
        return regs.cx;
    }

    static void initEvents()
    {
        _events.on_mouse_up = onMouseUp;
        _events.on_resize = onResize;
        _events.on_periodic_update = onPeriodicUpdate;
        _events.on_update = onUpdate;
        _events.get_scroll_size = getScrollSize;
        _events.scroll_mouse_down = onScrollMouseDown;
        _events.scroll_mouse_over = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.cursor = cursor;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
    }
}
