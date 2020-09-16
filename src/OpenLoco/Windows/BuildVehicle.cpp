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
#include "../Things/ThingManager.h"
#include "../Ui/ScrollView.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::interop;

namespace OpenLoco::ui::BuildVehicle
{
    static const gfx::ui_size_t window_size = { 380, 233 };

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
        interface_skin::image_ids::build_vehicle_train_frame_0,
        interface_skin::image_ids::build_vehicle_train_frame_1,
        interface_skin::image_ids::build_vehicle_train_frame_2,
        interface_skin::image_ids::build_vehicle_train_frame_3,
        interface_skin::image_ids::build_vehicle_train_frame_4,
        interface_skin::image_ids::build_vehicle_train_frame_5,
        interface_skin::image_ids::build_vehicle_train_frame_6,
        interface_skin::image_ids::build_vehicle_train_frame_7,
        interface_skin::image_ids::build_vehicle_train_frame_8,
        interface_skin::image_ids::build_vehicle_train_frame_9,
        interface_skin::image_ids::build_vehicle_train_frame_10,
        interface_skin::image_ids::build_vehicle_train_frame_11,
        interface_skin::image_ids::build_vehicle_train_frame_12,
        interface_skin::image_ids::build_vehicle_train_frame_13,
        interface_skin::image_ids::build_vehicle_train_frame_14,
        interface_skin::image_ids::build_vehicle_train_frame_15,
    };

    static const uint32_t aircraftTabImages[16]{
        interface_skin::image_ids::build_vehicle_aircraft_frame_0,
        interface_skin::image_ids::build_vehicle_aircraft_frame_1,
        interface_skin::image_ids::build_vehicle_aircraft_frame_2,
        interface_skin::image_ids::build_vehicle_aircraft_frame_3,
        interface_skin::image_ids::build_vehicle_aircraft_frame_4,
        interface_skin::image_ids::build_vehicle_aircraft_frame_5,
        interface_skin::image_ids::build_vehicle_aircraft_frame_6,
        interface_skin::image_ids::build_vehicle_aircraft_frame_7,
        interface_skin::image_ids::build_vehicle_aircraft_frame_8,
        interface_skin::image_ids::build_vehicle_aircraft_frame_9,
        interface_skin::image_ids::build_vehicle_aircraft_frame_10,
        interface_skin::image_ids::build_vehicle_aircraft_frame_11,
        interface_skin::image_ids::build_vehicle_aircraft_frame_12,
        interface_skin::image_ids::build_vehicle_aircraft_frame_13,
        interface_skin::image_ids::build_vehicle_aircraft_frame_14,
        interface_skin::image_ids::build_vehicle_aircraft_frame_15,
    };

    static const uint32_t busTabImages[16]{
        interface_skin::image_ids::build_vehicle_bus_frame_0,
        interface_skin::image_ids::build_vehicle_bus_frame_1,
        interface_skin::image_ids::build_vehicle_bus_frame_2,
        interface_skin::image_ids::build_vehicle_bus_frame_3,
        interface_skin::image_ids::build_vehicle_bus_frame_4,
        interface_skin::image_ids::build_vehicle_bus_frame_5,
        interface_skin::image_ids::build_vehicle_bus_frame_6,
        interface_skin::image_ids::build_vehicle_bus_frame_7,
        interface_skin::image_ids::build_vehicle_bus_frame_8,
        interface_skin::image_ids::build_vehicle_bus_frame_9,
        interface_skin::image_ids::build_vehicle_bus_frame_10,
        interface_skin::image_ids::build_vehicle_bus_frame_11,
        interface_skin::image_ids::build_vehicle_bus_frame_12,
        interface_skin::image_ids::build_vehicle_bus_frame_13,
        interface_skin::image_ids::build_vehicle_bus_frame_14,
        interface_skin::image_ids::build_vehicle_bus_frame_15,
    };

    static const uint32_t tramTabImages[16]{
        interface_skin::image_ids::build_vehicle_tram_frame_0,
        interface_skin::image_ids::build_vehicle_tram_frame_1,
        interface_skin::image_ids::build_vehicle_tram_frame_2,
        interface_skin::image_ids::build_vehicle_tram_frame_3,
        interface_skin::image_ids::build_vehicle_tram_frame_4,
        interface_skin::image_ids::build_vehicle_tram_frame_5,
        interface_skin::image_ids::build_vehicle_tram_frame_6,
        interface_skin::image_ids::build_vehicle_tram_frame_7,
        interface_skin::image_ids::build_vehicle_tram_frame_8,
        interface_skin::image_ids::build_vehicle_tram_frame_9,
        interface_skin::image_ids::build_vehicle_tram_frame_10,
        interface_skin::image_ids::build_vehicle_tram_frame_11,
        interface_skin::image_ids::build_vehicle_tram_frame_12,
        interface_skin::image_ids::build_vehicle_tram_frame_13,
        interface_skin::image_ids::build_vehicle_tram_frame_14,
        interface_skin::image_ids::build_vehicle_tram_frame_15,
    };

    static const uint32_t truckTabImages[16]{
        interface_skin::image_ids::build_vehicle_truck_frame_0,
        interface_skin::image_ids::build_vehicle_truck_frame_1,
        interface_skin::image_ids::build_vehicle_truck_frame_2,
        interface_skin::image_ids::build_vehicle_truck_frame_3,
        interface_skin::image_ids::build_vehicle_truck_frame_4,
        interface_skin::image_ids::build_vehicle_truck_frame_5,
        interface_skin::image_ids::build_vehicle_truck_frame_6,
        interface_skin::image_ids::build_vehicle_truck_frame_7,
        interface_skin::image_ids::build_vehicle_truck_frame_8,
        interface_skin::image_ids::build_vehicle_truck_frame_9,
        interface_skin::image_ids::build_vehicle_truck_frame_10,
        interface_skin::image_ids::build_vehicle_truck_frame_11,
        interface_skin::image_ids::build_vehicle_truck_frame_12,
        interface_skin::image_ids::build_vehicle_truck_frame_13,
        interface_skin::image_ids::build_vehicle_truck_frame_14,
        interface_skin::image_ids::build_vehicle_truck_frame_15,
    };

    static const uint32_t shipTabImages[16]{
        interface_skin::image_ids::build_vehicle_ship_frame_0,
        interface_skin::image_ids::build_vehicle_ship_frame_1,
        interface_skin::image_ids::build_vehicle_ship_frame_2,
        interface_skin::image_ids::build_vehicle_ship_frame_3,
        interface_skin::image_ids::build_vehicle_ship_frame_4,
        interface_skin::image_ids::build_vehicle_ship_frame_5,
        interface_skin::image_ids::build_vehicle_ship_frame_6,
        interface_skin::image_ids::build_vehicle_ship_frame_7,
        interface_skin::image_ids::build_vehicle_ship_frame_8,
        interface_skin::image_ids::build_vehicle_ship_frame_9,
        interface_skin::image_ids::build_vehicle_ship_frame_10,
        interface_skin::image_ids::build_vehicle_ship_frame_11,
        interface_skin::image_ids::build_vehicle_ship_frame_12,
        interface_skin::image_ids::build_vehicle_ship_frame_13,
        interface_skin::image_ids::build_vehicle_ship_frame_14,
        interface_skin::image_ids::build_vehicle_ship_frame_15,
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
        { VehicleType::plane, tab_build_new_aircraft, aircraftTabImages },
        { VehicleType::ship, tab_build_new_ships, shipTabImages }
    };

    // 0x5231D0
    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 380, 233 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 378, 13 }, widget_type::caption_24, 0),
        makeWidget({ 365, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        makeWidget({ 0, 41 }, { 380, 192 }, widget_type::panel, 1),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_train_vehicles),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_buses),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_trucks),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_trams),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_aircraft),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_build_new_ships),
        makeRemapWidget({ 5, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        makeRemapWidget({ 36, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        makeRemapWidget({ 67, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        makeRemapWidget({ 98, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        makeRemapWidget({ 129, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        makeRemapWidget({ 160, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        makeRemapWidget({ 191, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        makeRemapWidget({ 222, 43 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_vehicles_for),
        makeWidget({ 3, 72 }, { 374, 146 }, widget_type::scrollview, 1, scrollbars::vertical),
        makeWidget({ 250, 44 }, { 180, 66 }, widget_type::scrollview, 1, scrollbars::none),
        widgetEnd(),
    };

    static constexpr uint32_t widxToTrackTypeTab(widget_index widgetIndex)
    {
        return widgetIndex - widx::tab_track_type_0;
    }

    static loco_global<int16_t, 0x01136268> _numAvailableVehicles;
    static loco_global<uint16_t[objectmgr::getMaxObjects(object_type::vehicle)], 0x0113626A> _availableVehicles;
    static loco_global<uint16_t, 0x0113642A> _113642A;
    static loco_global<int32_t, 0x011364E8> _buildTargetVehicle; // -1 for no target
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

    static void setDisabledTransportTabs(ui::window* window);
    static void setTrackTypeTabs(ui::window* window);
    static void resetTrackTypeTabSelection(ui::window* window);
    static void setTopToolbarLastTrack(uint8_t trackType, bool isRoad);
    static void setTransportTypeTabs(ui::window* window);
    static void sub_4B60CC(OpenLoco::vehicle* vehicle);
    static void drawVehicleOverview(gfx::drawpixelinfo_t* dpi, int16_t vehicleTypeIdx, company_id_t company, uint8_t eax, uint8_t esi, gfx::point_t offset);
    static int16_t drawVehicleInline(gfx::drawpixelinfo_t* dpi, int16_t vehicleTypeIdx, uint8_t unk_1, company_id_t company, gfx::point_t loc);
    static void drawTransportTypeTabs(ui::window* window, gfx::drawpixelinfo_t* dpi);
    static void drawTrackTypeTabs(ui::window* window, gfx::drawpixelinfo_t* dpi);

    static void initEvents();

    // 0x4C1C64
    static window* create(company_id_t company)
    {
        initEvents();
        auto window = WindowManager::createWindow(WindowType::buildVehicle, window_size, window_flags::flag_11, &_events);
        window->widgets = _widgets;
        window->number = company;
        window->enabled_widgets = (1 << widx::close_button) | (1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships) | (1 << widx::tab_track_type_0) | (1 << widx::tab_track_type_1) | (1 << widx::tab_track_type_2) | (1 << widx::tab_track_type_3) | (1 << widx::tab_track_type_4) | (1 << widx::tab_track_type_5) | (1 << widx::tab_track_type_6) | (1 << widx::tab_track_type_7) | (1 << widx::scrollview_vehicle_selection);
        window->owner = companymgr::getControllingId();
        window->frame_no = 0;
        auto skin = OpenLoco::objectmgr::get<interface_skin_object>();
        if (skin != nullptr)
        {
            window->colours[1] = skin->colour_0A;
        }
        setDisabledTransportTabs(window);
        return window;
    }

    /* 0x4C1AF7
     * depending on flags (1<<31) vehicle is a tab id or a vehicle thing_id
     */
    window* open(uint32_t vehicle, uint32_t flags)
    {
        auto window = WindowManager::bringToFront(WindowType::buildVehicle, companymgr::getControllingId());
        bool tabMode = flags & (1 << 31);
        if (window)
        {
            widget_index tab = widx::tab_build_new_trains;
            if (!tabMode)
            {
                auto veh = thingmgr::get<OpenLoco::vehicle>(vehicle);
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
            window = create(companymgr::getControllingId());
            window->width = window_size.width;
            window->height = window_size.height;
            _buildTargetVehicle = -1;
            if (!tabMode)
            {
                _buildTargetVehicle = vehicle;
                auto veh = thingmgr::get<OpenLoco::vehicle>(vehicle);
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
        auto veh = thingmgr::get<OpenLoco::vehicle>(_buildTargetVehicle);
        if (veh == nullptr)
        {
            return window;
        }
        auto targetTrackType = veh->track_type;
        if (veh->mode != TransportMode::rail)
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
                sub_4B92A5((ui::window*)regs.esi);
                regs = backup;
                return 0;
            });

        registerHook(
            0x4C1AF7,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                auto window = open(regs.eax, regs.eax);
                regs = backup;
                regs.esi = (int32_t)window;
                return 0;
            });
    }

    /* 0x4B9165
     * Works out which vehicles are able to be built for this vehicle_type or vehicle
     */
    static void generateBuildableVehiclesArray(VehicleType vehicleType, uint8_t trackType, OpenLoco::vehicle* vehicle)
    {
        if (trackType != 0xFF && (trackType & (1 << 7)))
        {
            auto trackIdx = trackType & ~(1 << 7);
            auto roadObj = objectmgr::get<road_object>(trackIdx);
            if (roadObj->flags & flags_12::unk_03)
            {
                trackType = 0xFE;
            }
        }

        auto companyId = companymgr::getControllingId();
        if (vehicle != nullptr)
        {
            companyId = vehicle->owner;
        }
        _numAvailableVehicles = 0;
        struct build_item
        {
            uint16_t vehicle_index;
            uint8_t power;
            uint16_t designed;
        };
        std::vector<build_item> buildableVehicles;

        for (uint16_t vehicleObjIndex = 0; vehicleObjIndex < objectmgr::getMaxObjects(object_type::vehicle); ++vehicleObjIndex)
        {
            auto vehicleObj = objectmgr::get<vehicle_object>(vehicleObjIndex);
            if (vehicleObj == nullptr)
            {
                continue;
            }

            if (vehicle)
            {
                auto* const head = vehicle->asVehicleHead();
                if (head && !head->isVehicleTypeCompatible(vehicleObjIndex))
                {
                    continue;
                }
            }

            if (vehicleObj->type != vehicleType)
            {
                continue;
            }

            // Is vehicle type unlocked
            if (!(companymgr::get(companyId)->unlocked_vehicles[vehicleObjIndex >> 5] & (1 << (vehicleObjIndex & 0x1F))))
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

    static ui::window* getTopEditingVehicleWindow()
    {
        for (auto i = (int32_t)WindowManager::count() - 1; i >= 0; i--)
        {
            auto w = WindowManager::get(i);

            if (w->type != WindowType::vehicle)
                continue;

            if (w->current_tab != 1)
                continue;

            auto vehicle = thingmgr::get<OpenLoco::vehicle>(w->number);
            if (vehicle->owner != companymgr::getControllingId())
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
    void sub_4B92A5(ui::window* window)
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

        OpenLoco::vehicle* veh = nullptr;
        if (_buildTargetVehicle != -1)
        {
            veh = thingmgr::get<OpenLoco::vehicle>(_buildTargetVehicle);
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
    static void onMouseUp(ui::window* window, widget_index widgetIndex)
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

                if (input::hasFlag(input::input_flags::tool_active))
                {
                    input::toolCancel(window->type, window->number);
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
        window->flags |= window_flags::resizable;
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
            ui::scrollview::updateThumbs(window, widx::scrollview_vehicle_selection);
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
    static void getScrollSize(ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = window->var_83C * window->row_height;
    }

    // 0x4C384B
    static void onScrollMouseDown(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
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
        auto vehicleObj = objectmgr::get<vehicle_object>(item);
        FormatArguments args{};
        // Skip 5 * 2 bytes
        args.skip(10);
        args.push(vehicleObj->name);
        gGameCommandErrorTitle = string_ids::cant_build_pop_5_string_id;
        if (_buildTargetVehicle != -1)
        {
            auto vehicle = thingmgr::get<OpenLoco::vehicle>(_buildTargetVehicle);
            args.push(vehicle->var_22);
            args.push(vehicle->var_44);
            gGameCommandErrorTitle = string_ids::cant_add_pop_5_string_id_string_id;
        }

        if (!game_commands::do_5(item, _buildTargetVehicle))
        {
            return;
        }

        if (_buildTargetVehicle == -1)
        {
            auto vehicle = thingmgr::get<OpenLoco::vehicle>(_113642A);
            sub_4B60CC(vehicle);
        }
        sub_4B92A5(window);
    }

    // 0x4C3802
    static void onScrollMouseOver(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
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
    static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
    {
        if (widgetIndex < widx::tab_track_type_0 || widgetIndex >= widx::scrollview_vehicle_selection)
        {
            args.push(string_ids::tooltip_scroll_new_vehicle_list);
        }
        else
        {
            auto trackTypeTab = widxToTrackTypeTab(widgetIndex);
            auto type = _TrackTypesForTab[trackTypeTab];
            if (type == -1)
            {
                if (_transportTypeTabInformation[window->current_tab].type == VehicleType::plane)
                {

                    args.push(string_ids::airport);
                }
                else
                {
                    args.push(string_ids::docks);
                }
            }
            else
            {
                bool is_road = type & (1 << 7);
                type &= ~(1 << 7);
                if (is_road)
                {
                    auto roadObj = objectmgr::get<road_object>(type);
                    args.push(roadObj->name);
                }
                else
                {
                    auto trackObj = objectmgr::get<track_object>(type);
                    args.push(trackObj->name);
                }
            }
        }
    }

    // 0x4C37CB
    static ui::cursor_id cursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
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
    static void prepareDraw(ui::window* window)
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

        window->widgets[widx::caption].text = window->current_tab + string_ids::build_trains;

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
    static void draw(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        window->draw(dpi);
        drawTransportTypeTabs(window, dpi);
        drawTrackTypeTabs(window, dpi);

        {
            auto x = window->x + 2;
            auto y = window->y + window->height - 13;
            auto bottomLeftMessage = string_ids::select_new_vehicle;
            FormatArguments args{};
            if (_buildTargetVehicle != -1)
            {
                auto vehicle = thingmgr::get<OpenLoco::vehicle>(_buildTargetVehicle);
                args.push(vehicle->var_22);
                args.push(vehicle->var_44);
                bottomLeftMessage = string_ids::select_vehicle_to_add_to_string_id;
            }

            gfx::drawString_494BBF(*dpi, x, y, window->width - 186, colour::black, bottomLeftMessage, &args);
        }

        if (window->row_hover == -1)
        {
            return;
        }

        auto vehicleObj = objectmgr::get<vehicle_object>(window->row_hover);
        auto buffer = const_cast<char*>(stringmgr::getString(string_ids::buffer_1250));

        {
            auto cost = (vehicleObj->cost_factor * currencyMultiplicationFactor[vehicleObj->cost_index]) / 64;
            FormatArguments args{};
            args.push(cost);
            buffer = stringmgr::formatString(buffer, string_ids::stats_cost, &args);
        }

        {
            auto runningCost = (vehicleObj->run_cost_factor * currencyMultiplicationFactor[vehicleObj->run_cost_index]) / 1024;
            FormatArguments args{};
            args.push(runningCost);
            buffer = stringmgr::formatString(buffer, string_ids::stats_running_cost, &args);
        }

        if (vehicleObj->designed != 0)
        {
            FormatArguments args{};
            args.push(vehicleObj->designed);
            buffer = stringmgr::formatString(buffer, string_ids::stats_designed, &args);
        }

        if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
        {
            buffer = stringmgr::formatString(buffer, string_ids::stats_requires);
            auto trackName = string_ids::road;
            if (vehicleObj->mode == TransportMode::road)
            {
                if (vehicleObj->track_type != 0xFF)
                {
                    trackName = objectmgr::get<road_object>(vehicleObj->track_type)->name;
                }
            }
            else
            {
                trackName = objectmgr::get<track_object>(vehicleObj->track_type)->name;
            }

            buffer = stringmgr::formatString(buffer, trackName);

            for (auto i = 0; i < vehicleObj->num_mods; ++i)
            {
                strcpy(buffer, " + ");
                buffer += 3;
                if (vehicleObj->mode == TransportMode::road)
                {
                    auto roadExtraObj = objectmgr::get<road_extra_object>(vehicleObj->required_track_extras[i]);
                    buffer = stringmgr::formatString(buffer, roadExtraObj->name);
                }
                else
                {
                    auto trackExtraObj = objectmgr::get<track_extra_object>(vehicleObj->required_track_extras[i]);
                    buffer = stringmgr::formatString(buffer, trackExtraObj->name);
                }
            }

            if (vehicleObj->flags & flags_E0::rack_rail)
            {
                auto trackExtraObj = objectmgr::get<track_extra_object>(vehicleObj->rack_rail_type);
                FormatArguments args{};
                args.push(trackExtraObj->name);
                buffer = stringmgr::formatString(buffer, string_ids::stats_string_steep_slope, &args);
            }
        }

        if (vehicleObj->power != 0)
        {
            if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
            {
                FormatArguments args{};
                args.push(vehicleObj->power);
                buffer = stringmgr::formatString(buffer, string_ids::stats_power, &args);
            }
        }

        {
            FormatArguments args{};
            args.push(vehicleObj->weight);
            buffer = stringmgr::formatString(buffer, string_ids::stats_weight, &args);
        }
        {
            FormatArguments args{};
            args.push(vehicleObj->speed);
            buffer = stringmgr::formatString(buffer, string_ids::stats_max_speed, &args);
        }
        if (vehicleObj->flags & flags_E0::rack_rail)
        {
            auto trackExtraObj = objectmgr::get<track_extra_object>(vehicleObj->rack_rail_type);
            FormatArguments args{};
            args.push(vehicleObj->rack_speed);
            args.push(trackExtraObj->name);
            buffer = stringmgr::formatString(buffer, string_ids::stats_velocity_on_string, &args);
        }

        if (vehicleObj->num_simultaneous_cargo_types != 0)
        {
            {
                auto cargoType = utility::bitScanForward(vehicleObj->primary_cargo_types);
                if (cargoType != -1)
                {
                    auto cargoTypes = vehicleObj->primary_cargo_types & ~(1 << cargoType);
                    {
                        auto cargoObj = objectmgr::get<cargo_object>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unit_name_plural;
                        if (vehicleObj->max_primary_cargo == 1)
                        {
                            cargoUnitName = cargoObj->unit_name_singular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint16_t>(vehicleObj->max_primary_cargo);
                        buffer = stringmgr::formatString(buffer, string_ids::stats_capacity, &args);
                    }
                    cargoType = utility::bitScanForward(cargoTypes);
                    if (cargoType != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargoType != -1; cargoType = utility::bitScanForward(cargoTypes))
                        {
                            cargoTypes &= ~(1 << cargoType);
                            if (buffer[-1] != '(')
                            {
                                strcpy(buffer, " ");
                                buffer++;
                            }

                            auto cargoObj = objectmgr::get<cargo_object>(cargoType);
                            FormatArguments args{};
                            args.push(cargoObj->name);
                            buffer = stringmgr::formatString(buffer, string_ids::stats_or_string, &args);
                            strcpy(buffer, " ");
                            buffer++;
                        }
                        buffer[-1] = ')';
                    }
                }
            }

            if (vehicleObj->flags & flags_E0::refittable)
            {
                buffer = stringmgr::formatString(buffer, string_ids::stats_refittable);
            }

            if (vehicleObj->num_simultaneous_cargo_types > 1)
            {
                auto cargoType = utility::bitScanForward(vehicleObj->secondary_cargo_types);
                if (cargoType != -1)
                {
                    auto cargoTypes = vehicleObj->secondary_cargo_types & ~(1 << cargoType);
                    {
                        auto cargoObj = objectmgr::get<cargo_object>(cargoType);
                        FormatArguments args{};
                        auto cargoUnitName = cargoObj->unit_name_plural;
                        if (vehicleObj->max_secondary_cargo == 1)
                        {
                            cargoUnitName = cargoObj->unit_name_singular;
                        }
                        args.push(cargoUnitName);
                        args.push<uint16_t>(vehicleObj->max_secondary_cargo);
                        buffer = stringmgr::formatString(buffer, string_ids::stats_plus_string, &args);
                    }

                    cargoType = utility::bitScanForward(cargoTypes);
                    if (cargoType != -1)
                    {
                        strcpy(buffer, " (");
                        buffer += 2;
                        for (; cargoType != -1; cargoType = utility::bitScanForward(cargoTypes))
                        {
                            cargoTypes &= ~(1 << cargoType);
                            if (buffer[-1] != '(')
                            {
                                strcpy(buffer, " ");
                                buffer++;
                            }

                            auto cargoObj = objectmgr::get<cargo_object>(cargoType);
                            FormatArguments args{};
                            args.push(cargoObj->name);
                            buffer = stringmgr::formatString(buffer, string_ids::stats_or_string, &args);
                            strcpy(buffer, " ");
                            buffer++;
                        }
                        buffer[-1] = ')';
                    }
                }
            }
        }

        auto x = window->widgets[widx::scrollview_vehicle_selection].right + window->x + 2;
        auto y = window->widgets[widx::scrollview_vehicle_preview].bottom + window->y + 2;
        gfx::drawString_495224(*dpi, x, y, 180, colour::black, string_ids::buffer_1250);
    }

    // 0x4C3307
    static void drawScroll(ui::window* window, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        switch (scrollIndex)
        {
            case scrollIdx::vehicle_selection:
            {
                auto colour = colour::getShade(window->colours[1], 4);
                gfx::clear(*dpi, colour * 0x01010101);
                if (window->var_83C == 0)
                {
                    auto defaultMessage = string_ids::no_vehicles_available;
                    FormatArguments args{};
                    if (_buildTargetVehicle != -1)
                    {
                        auto vehicle = thingmgr::get<OpenLoco::vehicle>(_buildTargetVehicle);
                        defaultMessage = string_ids::no_compatible_vehicles_available;
                        args.push(vehicle->var_22);
                        args.push(vehicle->var_44);
                    }

                    auto widget = window->widgets[widx::scrollview_vehicle_selection];
                    auto width = widget.right - widget.left - 17;
                    auto y = (window->row_height - 10) / 2;
                    gfx::drawString_495224(*dpi, 3, y, width, colour::black, defaultMessage, &args);
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

                        auto colouredString = string_ids::black_stringid;
                        if (window->row_hover == vehicleType)
                        {
                            gfx::fillRect(dpi, 0, y, window->width, y + window->row_height - 1, 0x2000030);
                            colouredString = string_ids::wcolour2_stringid;
                        }

                        int16_t half = (window->row_height - 22) / 2;
                        auto x = drawVehicleInline(dpi, vehicleType, 0, companymgr::getControllingId(), { 0, static_cast<int16_t>(y + half) });

                        auto vehicleObj = objectmgr::get<vehicle_object>(vehicleType);
                        FormatArguments args{};
                        args.push(vehicleObj->name);
                        half = (window->row_height - 10) / 2;
                        gfx::drawString_494B3F(*dpi, x + 3, y + half, colour::black, colouredString, &args);
                    }
                }
                break;
            }
            case scrollIdx::vehicle_preview:
            {
                auto colour = colour::getShade(window->colours[1], 0);
                // gfx::clear needs the colour copied to each byte of eax
                gfx::clear(*dpi, colour * 0x01010101);

                if (window->row_hover == -1)
                {
                    break;
                }

                uint8_t unk1 = _52622E & 0x3F;
                uint8_t unk2 = ((_52622E + 2) / 4) & 0x3F;
                drawVehicleOverview(dpi, window->row_hover, companymgr::getControllingId(), unk1, unk2, { 90, 37 });

                auto vehicleObj = objectmgr::get<vehicle_object>(window->row_hover);
                auto buffer = const_cast<char*>(stringmgr::getString(string_ids::buffer_1250));
                buffer = stringmgr::formatString(buffer, vehicleObj->name);
                auto usableCargoTypes = vehicleObj->primary_cargo_types | vehicleObj->secondary_cargo_types;

                for (auto cargoTypes = utility::bitScanForward(usableCargoTypes); cargoTypes != -1; cargoTypes = utility::bitScanForward(usableCargoTypes))
                {
                    usableCargoTypes &= ~(1 << cargoTypes);
                    auto cargoObj = objectmgr::get<cargo_object>(cargoTypes);
                    *buffer++ = ' ';
                    *buffer++ = control_codes::inline_sprite_str;
                    *(reinterpret_cast<uint32_t*>(buffer)) = cargoObj->unit_inline_sprite;
                    buffer += 4;
                }

                *buffer++ = '\0';
                FormatArguments args{};
                args.push(string_ids::buffer_1250);
                gfx::drawStringCentredClipped(*dpi, 89, 52, 177, 0x20, string_ids::wcolour2_stringid, &args);
                break;
            }
        }
    }

    // 0x4C28D2
    static void setDisabledTransportTabs(ui::window* window)
    {
        auto availableVehicles = companymgr::companies()[window->number].available_vehicles;
        // By shifting by 4 the available_vehicles flags align with the tabs flags
        auto disabledTabs = (availableVehicles << 4) ^ ((1 << widx::tab_build_new_trains) | (1 << widx::tab_build_new_buses) | (1 << widx::tab_build_new_trucks) | (1 << widx::tab_build_new_trams) | (1 << widx::tab_build_new_aircraft) | (1 << widx::tab_build_new_ships));
        window->disabled_widgets = disabledTabs;
    }

    // 0x4C2D8A
    static void setTrackTypeTabs(ui::window* window)
    {
        VehicleType currentTransportTabType = _transportTypeTabInformation[window->current_tab].type;
        generateBuildableVehiclesArray(currentTransportTabType, 0xFF, nullptr);

        auto railTrackTypes = 0;
        auto roadTrackTypes = 0;
        for (auto i = 0; i < _numAvailableVehicles; i++)
        {
            auto vehicleObj = objectmgr::get<vehicle_object>(_availableVehicles[i]);
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
        for (trackType = utility::bitScanForward(railTrackTypes); trackType != -1 && trackTypeTab <= tab_track_type_7; trackType = utility::bitScanForward(railTrackTypes))
        {
            railTrackTypes &= ~(1 << trackType);
            window->widgets[trackTypeTab].type = widget_type::wt_8;
            _TrackTypesForTab[widxToTrackTypeTab(trackTypeTab)] = trackType;
            trackTypeTab++;
        }

        if (trackType == -1 && trackTypeTab <= tab_track_type_7)
        {
            for (trackType = utility::bitScanForward(roadTrackTypes); trackType != -1 && trackTypeTab <= tab_track_type_7; trackType = utility::bitScanForward(roadTrackTypes))
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
    static void resetTrackTypeTabSelection(ui::window* window)
    {
        auto transportType = _transportTypeTabInformation[window->current_tab].type;
        if (transportType == VehicleType::plane || transportType == VehicleType::ship)
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
            auto road_obj = objectmgr::get<road_object>(trackType);
            if (road_obj && road_obj->flags & flags_12::unk_01)
            {
                setRail = true;
            }
        }
        else
        {
            auto rail_obj = objectmgr::get<track_object>(trackType);
            if (rail_obj && !(rail_obj->flags & flags_22::unk_02))
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
    static void setTransportTypeTabs(ui::window* window)
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

    /* 0x4B60CC
     * Opens vehicle window and clicks???
     */
    static void sub_4B60CC(OpenLoco::vehicle* vehicle)
    {
        registers regs;
        regs.edx = (int32_t)vehicle;
        call(0x4B60CC, regs);
    }

    // 0x4C2BFD
    static void drawTransportTypeTabs(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto skin = objectmgr::get<interface_skin_object>();
        auto companyColour = companymgr::getCompanyColour(window->number);

        for (auto tab : _transportTypeTabInformation)
        {
            auto frameNo = 0;
            if (_transportTypeTabInformation[window->current_tab].type == tab.type)
            {
                frameNo = (window->frame_no / 2) & 0xF;
            }
            uint32_t image = gfx::recolour(skin->img + tab.imageIds[frameNo], companyColour);
            widget::draw_tab(window, dpi, image, tab.widgetIndex);
        }
    }

    // 0x4C28F1
    static void drawTrackTypeTabs(ui::window* window, gfx::drawpixelinfo_t* dpi)
    {
        auto skin = objectmgr::get<interface_skin_object>();
        auto companyColour = companymgr::getCompanyColour(window->number);

        auto left = window->x;
        auto top = window->y + 69;
        auto right = left + window->width - 187;
        auto bottom = top;
        gfx::fillRect(dpi, left, top, right, bottom, colour::getShade(window->colours[1], 7));

        left = window->x + window->width - 187;
        top = window->y + 41;
        right = left;
        bottom = top + 27;
        gfx::fillRect(dpi, left, top, right, bottom, colour::getShade(window->colours[1], 7));

        for (uint32_t tab = 0; tab < _numTrackTypeTabs; ++tab)
        {
            const auto widget = window->widgets[tab + widx::tab_track_type_0];
            if (window->current_secondary_tab == tab)
            {
                left = widget.left + window->x + 1;
                top = widget.top + window->y + 26;
                right = left + 29;
                bottom = top;
                gfx::fillRect(dpi, left, top, right, bottom, colour::getShade(window->colours[1], 5));
            }

            auto img = 0;
            auto type = _TrackTypesForTab[tab];
            if (type == -1)
            {
                if (window->current_tab == (widx::tab_build_new_aircraft - widx::tab_build_new_trains))
                {
                    img = skin->img + interface_skin::image_ids::toolbar_menu_airport;
                }
                else
                {
                    img = skin->img + interface_skin::image_ids::toolbar_menu_ship_port;
                }
                // Original saved the company colour in the img but didn't set the recolour flag
            }
            else if (type & (1 << 7)) // is_road
            {
                type &= ~(1 << 7);
                auto roadObj = objectmgr::get<road_object>(type);
                img = roadObj->var_0E;
                if (window->current_secondary_tab == tab)
                {
                    img += (window->frame_no / 4) & 0x1F;
                }
                img = gfx::recolour(img, companyColour);
            }
            else
            {
                auto trackObj = objectmgr::get<track_object>(type);
                img = trackObj->var_1E;
                if (window->current_secondary_tab == tab)
                {
                    img += (window->frame_no / 4) & 0xF;
                }
                img = gfx::recolour(img, companyColour);
            }

            widget::draw_tab(window, dpi, img, tab + widx::tab_track_type_0);
        }
    }

    // 0x4B7741
    static void drawVehicleOverview(gfx::drawpixelinfo_t* dpi, int16_t vehicleTypeIdx, company_id_t company, uint8_t eax, uint8_t esi, gfx::point_t offset)
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
    static int16_t drawVehicleInline(gfx::drawpixelinfo_t* dpi, int16_t vehicleTypeIdx, uint8_t unk_1, company_id_t company, gfx::point_t loc)
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
