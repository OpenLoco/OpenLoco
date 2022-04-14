#include "../CompanyManager.h"
#include "../Date.h"
#include "../Entities/EntityManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/CargoObject.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../StationManager.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Utility/String.hpp"
#include "../Vehicles/Orders.h"
#include "../Vehicles/Vehicle.h"
#include "../Widget.h"
#include <stdexcept>
#include <utility>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::VehicleList
{
    static loco_global<VehicleType, 0x00525FAF> _lastVehiclesOption;

    static const Ui::Size window_size = { 550, 213 };
    static const Ui::Size max_dimensions = { 550, 1200 };
    static const Ui::Size min_dimensions = { 220, 160 };

    static WindowEventList _events;

    enum Widx
    {
        frame = 0,
        caption = 1,
        close_button = 2,
        panel = 3,
        tab_trains,
        tab_buses,
        tab_trucks,
        tab_trams,
        tab_aircraft,
        tab_ships,
        company_select,
        sort_name,
        sort_profit,
        sort_age,
        sort_reliability,
        scrollview,
        filter_type,
        filter_type_btn,
        cargo_type,
        cargo_type_btn,
    };

    Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 550, 213 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 548, 13 }, WidgetType::caption_24, WindowColour::primary),
        makeWidget({ 535, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 41 }, { 550, 172 }, WidgetType::panel, WindowColour::secondary),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_trains),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_buses),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_trucks),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_trams),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_aircraft),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, WidgetType::tab, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_ships),
        makeWidget({ 0, 14 }, { 26, 26 }, WidgetType::buttonWithImage, WindowColour::primary, StringIds::null, StringIds::tooltip_select_company),
        makeWidget({ 4, 43 }, { 310, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::null, StringIds::tooltip_sort_by_name),
        makeWidget({ 314, 43 }, { 100, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::null, StringIds::tooltip_sort_by_profit),
        makeWidget({ 414, 43 }, { 65, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::null, StringIds::tooltip_sort_by_age),
        makeWidget({ 479, 43 }, { 67, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::null, StringIds::tooltip_sort_by_reliability),
        makeWidget({ 3, 56 }, { 544, 138 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        makeDropdownWidgets({ 280 - 16, 200 }, { 120, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty),
        makeDropdownWidgets({ 402 - 16, 200 }, { 150, 12 }, WidgetType::combobox, WindowColour::secondary, StringIds::empty),
        widgetEnd()
    };

    // clang-format off
    constexpr uint16_t _tabWidgets = (1 << Widx::tab_trains) | (1 << Widx::tab_buses) | (1 << Widx::tab_trucks) | (1 << Widx::tab_trams) | (1 << Widx::tab_aircraft) | (1 << Widx::tab_ships);
    constexpr uint64_t _enabledWidgets = (1ULL << Widx::close_button) | _tabWidgets | (1ULL << Widx::company_select) |
        (1ULL << Widx::sort_name) | (1ULL << Widx::sort_profit) | (1ULL << Widx::sort_age) | (1ULL << Widx::sort_reliability) |
        (1ULL << Widx::scrollview) | (1ULL << Widx::filter_type) | (1ULL << Widx::filter_type_btn) | (1ULL << Widx::cargo_type) | (1ULL << Widx::cargo_type_btn);
    // clang-format on

    enum SortMode : uint16_t
    {
        Name,
        Profit,
        Age,
        Reliability,
    };

    enum FilterMode : uint8_t
    {
        allVehicles,
        stoppingAt,
        transportingCargo,
    };

    static const uint8_t row_heights[] = {
        28,
        28,
        28,
        28,
        48,
        36
    };

    static Widx getTabFromType(VehicleType type);
    static void initEvents();

    constexpr bool isStationFilterActive(const Window* self, bool checkSelection = true)
    {
        return self->var_88A == static_cast<int16_t>(FilterMode::stoppingAt) && (!checkSelection || self->var_88C != -1);
    }

    constexpr bool isCargoFilterActive(const Window* self, bool checkSelection = true)
    {
        return self->var_88A == static_cast<int16_t>(FilterMode::transportingCargo) && (!checkSelection || self->var_88C != -1);
    }

    static bool refreshActiveStation(Window* self)
    {
        if (!isStationFilterActive(self, false))
            return false;

        auto stationWindow = WindowManager::find(WindowType::station);
        if (stationWindow != nullptr)
        {
            self->var_88C = stationWindow->number;
            return true;
        }
        else
        {
            self->var_88C = -1;
            return false;
        }
    }

    using Vehicles::VehicleHead;

    static bool vehicleStopsAtActiveStation(const VehicleHead* head, StationId filterStationId)
    {
        auto orders = Vehicles::OrderRingView(head->orderTableOffset);
        for (auto& order : orders)
        {
            auto* stationOrder = order.as<Vehicles::OrderStation>();
            if (stationOrder == nullptr)
                continue;

            const auto stationId = stationOrder->getStation();
            if (stationId == filterStationId)
                return true;
        }
        return false;
    }

    static bool vehicleIsTransportingCargo(const VehicleHead* head, int16_t filterCargoId)
    {
        auto orders = Vehicles::OrderRingView(head->orderTableOffset);
        for (auto& order : orders)
        {
            Vehicles::OrderCargo* cargoOrder = order.as<Vehicles::OrderUnloadAll>();
            if (cargoOrder == nullptr)
                cargoOrder = order.as<Vehicles::OrderWaitFor>();
            if (cargoOrder == nullptr)
                continue;

            const auto cargoId = cargoOrder->getCargo();
            if (cargoId == filterCargoId)
                return true;
        }
        return false;
    }

    // 0x004C1D4F
    static void refreshVehicleList(Window* self)
    {
        refreshActiveStation(self);
        self->rowCount = 0;
        for (auto vehicle : EntityManager::VehicleList())
        {
            if (vehicle->vehicleType != static_cast<VehicleType>(self->currentTab))
                continue;

            if (vehicle->owner != CompanyId(self->number))
                continue;

            if (isStationFilterActive(self) && !vehicleStopsAtActiveStation(vehicle, StationId(self->var_88C)))
                continue;

            if (isCargoFilterActive(self) && !vehicleIsTransportingCargo(vehicle, self->var_88C))
                continue;

            vehicle->var_0C &= ~Vehicles::Flags0C::sorted;
        }
    }

    // 0x004C1E4F
    static bool orderByName(const VehicleHead& lhs, const VehicleHead& rhs)
    {
        char lhsString[256] = { 0 };
        auto args = FormatArguments::common(lhs.ordinalNumber);
        StringManager::formatString(lhsString, lhs.name, &args);

        char rhsString[256] = { 0 };
        args = FormatArguments::common(rhs.ordinalNumber);
        StringManager::formatString(rhsString, rhs.name, &args);

        return Utility::strlogicalcmp(lhsString, rhsString) < 0;
    }

    // 0x004C1EC9
    static bool orderByProfit(const VehicleHead& lhs, const VehicleHead& rhs)
    {
        auto profitL = Vehicles::Vehicle(lhs).veh2->totalRecentProfit();
        auto profitR = Vehicles::Vehicle(rhs).veh2->totalRecentProfit();

        return profitR - profitL < 0;
    }

    // 0x004C1F1E
    static bool orderByAge(const VehicleHead& lhs, const VehicleHead& rhs)
    {
        auto dayCreatedL = Vehicles::Vehicle(lhs).veh1->dayCreated;
        auto dayCreatedR = Vehicles::Vehicle(rhs).veh1->dayCreated;

        return static_cast<int32_t>(dayCreatedL - dayCreatedR) < 0;
    }

    // 0x004C1F45
    static bool orderByReliability(const VehicleHead& lhs, const VehicleHead& rhs)
    {
        auto reliabilityL = Vehicles::Vehicle(lhs).veh2->reliability;
        auto reliabilityR = Vehicles::Vehicle(rhs).veh2->reliability;

        return static_cast<int32_t>(reliabilityR - reliabilityL) < 0;
    }

    static bool getOrder(const SortMode mode, const VehicleHead& lhs, const VehicleHead& rhs)
    {
        switch (mode)
        {
            case SortMode::Name:
                return orderByName(lhs, rhs);

            case SortMode::Profit:
                return orderByProfit(lhs, rhs);

            case SortMode::Age:
                return orderByAge(lhs, rhs);

            case SortMode::Reliability:
                return orderByReliability(lhs, rhs);
        }

        return false;
    }

    // 0x004C1D92
    static void updateVehicleList(Window* self)
    {
        EntityId insertId = EntityId::null;

        for (auto vehicle : EntityManager::VehicleList())
        {
            if (vehicle->vehicleType != static_cast<VehicleType>(self->currentTab))
                continue;

            if (vehicle->owner != CompanyId(self->number))
                continue;

            if (vehicle->var_0C & Vehicles::Flags0C::sorted)
                continue;

            if (isStationFilterActive(self) && !vehicleStopsAtActiveStation(vehicle, StationId(self->var_88C)))
                continue;

            if (isCargoFilterActive(self) && !vehicleIsTransportingCargo(vehicle, self->var_88C))
                continue;

            if (insertId == EntityId::null)
            {
                insertId = vehicle->id;
                continue;
            }

            auto* insertVehicle = EntityManager::get<VehicleHead>(insertId);
            if (insertVehicle == nullptr)
            {
                continue;
            }
            if (getOrder(SortMode(self->sortMode), *vehicle, *insertVehicle))
            {
                insertId = vehicle->id;
                continue;
            }
        }

        if (insertId != EntityId::null)
        {
            auto vehicle = EntityManager::get<VehicleHead>(insertId);
            if (vehicle == nullptr)
            {
                self->var_83C = self->rowCount;
                refreshVehicleList(self);
                return;
            }
            vehicle->var_0C |= Vehicles::Flags0C::sorted;

            if (vehicle->id != EntityId(self->rowInfo[self->rowCount]))
                self->rowInfo[self->rowCount] = enumValue(vehicle->id);

            self->rowCount++;

            if (self->rowCount > self->var_83C)
                self->var_83C = self->rowCount;
        }
        else
        {
            if (self->var_83C != self->rowCount)
                self->var_83C = self->rowCount;

            refreshVehicleList(self);
        }
    }

    // 0x004C2A6E
    static void drawTabs(Window* self, Gfx::Context* context)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();
        auto companyColour = CompanyManager::getCompanyColour(CompanyId(self->number));

        static std::pair<WidgetIndex_t, std::array<uint32_t, 8>> tabAnimations[] = {
            { Widx::tab_trains, {
                                    InterfaceSkin::ImageIds::vehicle_train_frame_0,
                                    InterfaceSkin::ImageIds::vehicle_train_frame_1,
                                    InterfaceSkin::ImageIds::vehicle_train_frame_2,
                                    InterfaceSkin::ImageIds::vehicle_train_frame_3,
                                    InterfaceSkin::ImageIds::vehicle_train_frame_4,
                                    InterfaceSkin::ImageIds::vehicle_train_frame_5,
                                    InterfaceSkin::ImageIds::vehicle_train_frame_6,
                                    InterfaceSkin::ImageIds::vehicle_train_frame_7,
                                } },
            { Widx::tab_aircraft, {
                                      InterfaceSkin::ImageIds::vehicle_aircraft_frame_0,
                                      InterfaceSkin::ImageIds::vehicle_aircraft_frame_1,
                                      InterfaceSkin::ImageIds::vehicle_aircraft_frame_2,
                                      InterfaceSkin::ImageIds::vehicle_aircraft_frame_3,
                                      InterfaceSkin::ImageIds::vehicle_aircraft_frame_4,
                                      InterfaceSkin::ImageIds::vehicle_aircraft_frame_5,
                                      InterfaceSkin::ImageIds::vehicle_aircraft_frame_6,
                                      InterfaceSkin::ImageIds::vehicle_aircraft_frame_7,
                                  } },
            { Widx::tab_buses, {
                                   InterfaceSkin::ImageIds::vehicle_buses_frame_0,
                                   InterfaceSkin::ImageIds::vehicle_buses_frame_1,
                                   InterfaceSkin::ImageIds::vehicle_buses_frame_2,
                                   InterfaceSkin::ImageIds::vehicle_buses_frame_3,
                                   InterfaceSkin::ImageIds::vehicle_buses_frame_4,
                                   InterfaceSkin::ImageIds::vehicle_buses_frame_5,
                                   InterfaceSkin::ImageIds::vehicle_buses_frame_6,
                                   InterfaceSkin::ImageIds::vehicle_buses_frame_7,
                               } },
            { Widx::tab_trams, {
                                   InterfaceSkin::ImageIds::vehicle_trams_frame_0,
                                   InterfaceSkin::ImageIds::vehicle_trams_frame_1,
                                   InterfaceSkin::ImageIds::vehicle_trams_frame_2,
                                   InterfaceSkin::ImageIds::vehicle_trams_frame_3,
                                   InterfaceSkin::ImageIds::vehicle_trams_frame_4,
                                   InterfaceSkin::ImageIds::vehicle_trams_frame_5,
                                   InterfaceSkin::ImageIds::vehicle_trams_frame_6,
                                   InterfaceSkin::ImageIds::vehicle_trams_frame_7,
                               } },
            { Widx::tab_trucks, {
                                    InterfaceSkin::ImageIds::vehicle_trucks_frame_0,
                                    InterfaceSkin::ImageIds::vehicle_trucks_frame_1,
                                    InterfaceSkin::ImageIds::vehicle_trucks_frame_2,
                                    InterfaceSkin::ImageIds::vehicle_trucks_frame_3,
                                    InterfaceSkin::ImageIds::vehicle_trucks_frame_4,
                                    InterfaceSkin::ImageIds::vehicle_trucks_frame_5,
                                    InterfaceSkin::ImageIds::vehicle_trucks_frame_6,
                                    InterfaceSkin::ImageIds::vehicle_trucks_frame_7,
                                } },
            { Widx::tab_ships, {
                                   InterfaceSkin::ImageIds::vehicle_ships_frame_0,
                                   InterfaceSkin::ImageIds::vehicle_ships_frame_1,
                                   InterfaceSkin::ImageIds::vehicle_ships_frame_2,
                                   InterfaceSkin::ImageIds::vehicle_ships_frame_3,
                                   InterfaceSkin::ImageIds::vehicle_ships_frame_4,
                                   InterfaceSkin::ImageIds::vehicle_ships_frame_5,
                                   InterfaceSkin::ImageIds::vehicle_ships_frame_6,
                                   InterfaceSkin::ImageIds::vehicle_ships_frame_7,
                               } },
        };

        for (auto [tab, frames] : tabAnimations)
        {
            if (self->isDisabled(tab))
                continue;

            auto isActive = tab == self->currentTab + Widx::tab_trains;
            auto imageId = isActive ? frames[self->frame_no / 2 % 8] : frames[0];

            uint32_t image = Gfx::recolour(skin->img + imageId, companyColour);
            Widget::drawTab(self, context, image, tab);
        }
    }

    // 0x004C28A5
    static void disableUnavailableVehicleTypes(Window* self)
    {
        // The original game looks at all companies here. We only look at the current company instead.
        auto company = CompanyManager::get(CompanyId(self->number));

        // Disable the tabs for the vehicles that are _not_ available for this company.
        self->disabledWidgets = (static_cast<uint64_t>(company->availableVehicles ^ 0x3F)) << Widx::tab_trains;
    }

    // 0x004C1AA2
    static Window* create(CompanyId companyId)
    {
        Window* self = WindowManager::createWindow(
            WindowType::vehicleList,
            window_size,
            WindowFlags::flag_11,
            &_events);

        self->widgets = _widgets;
        self->enabledWidgets = _enabledWidgets;
        self->number = enumValue(companyId);
        self->owner = companyId;
        self->frame_no = 0;

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        self->setColour(WindowColour::secondary, skin->colour_0A);

        disableUnavailableVehicleTypes(self);

        return self;
    }

    // 0x004C19DC
    Window* open(CompanyId companyId, VehicleType type)
    {
        Window* self = WindowManager::bringToFront(WindowType::vehicleList, enumValue(companyId));
        if (self != nullptr)
        {
            self->callOnMouseUp(VehicleList::getTabFromType(type));
            return self;
        }

        initEvents();

        // 0x004C1A05
        self = create(companyId);
        auto tabIndex = static_cast<uint8_t>(type);
        self->currentTab = tabIndex;
        self->rowHeight = row_heights[tabIndex];
        self->width = window_size.width;
        self->height = window_size.height;
        self->sortMode = 0;
        self->var_83C = 0;
        self->rowHover = -1;
        self->var_88A = static_cast<int16_t>(FilterMode::allVehicles);
        self->var_88C = -1;

        refreshVehicleList(self);

        self->invalidate();

        self->callOnResize();
        self->callPrepareDraw();
        self->initScrollWidgets();

        return self;
    }

    static Widx getTabFromType(VehicleType type)
    {
        auto tabIndex = static_cast<uint8_t>(type);
        if (tabIndex > 5)
        {
            throw std::domain_error("Unexpected vehicle type");
        }

        static constexpr Widx type_to_widx[] = {
            tab_trains,
            tab_buses,
            tab_trucks,
            tab_trams,
            tab_aircraft,
            tab_ships,
        };
        return type_to_widx[tabIndex];
    }

    // 0x4C2865
    static void setTransportTypeTabs(Window* self)
    {
        auto disabledWidgets = self->disabledWidgets >> Widx::tab_trains;
        auto widget = self->widgets + Widx::tab_trains;
        auto tabWidth = widget->right - widget->left;
        auto tabX = widget->left;
        for (auto i = 0; i <= Widx::tab_ships - Widx::tab_trains; ++i, ++widget)
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

    // 0x004C1F88
    static void prepareDraw(Window* self)
    {
        // The original game was setting widget sets here. As all tabs are the same, this has been omitted.
        self->activatedWidgets &= ~_tabWidgets;
        self->activatedWidgets |= 1ULL << (self->currentTab + Widx::tab_trains);

        auto company = CompanyManager::get(CompanyId(self->number));
        [[maybe_unused]] auto args = FormatArguments::common(company->name);

        static constexpr string_id typeToCaption[] = {
            StringIds::stringid_trains,
            StringIds::stringid_buses,
            StringIds::stringid_trucks,
            StringIds::stringid_trams,
            StringIds::stringid_aircraft,
            StringIds::stringid_ships,
        };

        // Basic frame widget dimensions
        self->widgets[Widx::frame].right = self->width - 1;
        self->widgets[Widx::frame].bottom = self->height - 1;

        self->widgets[Widx::panel].right = self->width - 1;
        self->widgets[Widx::panel].bottom = self->height - 1;

        self->widgets[Widx::caption].right = self->width - 2;
        self->widgets[Widx::caption].text = typeToCaption[self->currentTab];

        self->widgets[Widx::close_button].left = self->width - 15;
        self->widgets[Widx::close_button].right = self->width - 3;

        self->widgets[Widx::scrollview].right = self->width - 4;
        self->widgets[Widx::scrollview].bottom = self->height - 14;

        // Reposition table headers
        self->widgets[Widx::sort_name].right = std::min(self->width - 4, 313);

        self->widgets[Widx::sort_profit].left = std::min(self->width - 4, 314);
        self->widgets[Widx::sort_profit].right = std::min(self->width - 4, 413);

        self->widgets[Widx::sort_age].left = std::min(self->width - 4, 414);
        self->widgets[Widx::sort_age].right = std::min(self->width - 4, 478);

        self->widgets[Widx::sort_reliability].left = std::min(self->width - 4, 479);
        self->widgets[Widx::sort_reliability].right = std::min(self->width - 4, 545);

        // Reposition company selection
        self->widgets[Widx::company_select].left = self->width - 28;
        self->widgets[Widx::company_select].right = self->width - 3;

        // Set header button captions.
        self->widgets[Widx::sort_name].text = self->sortMode == SortMode::Name ? StringIds::table_header_name_desc : StringIds::table_header_name;
        self->widgets[Widx::sort_profit].text = self->sortMode == SortMode::Profit ? StringIds::table_header_monthly_profit_desc : StringIds::table_header_monthly_profit;
        self->widgets[Widx::sort_age].text = self->sortMode == SortMode::Age ? StringIds::table_header_age_desc : StringIds::table_header_age;
        self->widgets[Widx::sort_reliability].text = self->sortMode == SortMode::Reliability ? StringIds::table_header_reliability_desc : StringIds::table_header_reliability;

        // Reposition filter dropdowns
        self->widgets[Widx::filter_type].top = self->height - 13;
        self->widgets[Widx::filter_type].bottom = self->height - 2;

        self->widgets[Widx::filter_type_btn].top = self->height - 12;
        self->widgets[Widx::filter_type_btn].bottom = self->height - 3;

        self->widgets[Widx::cargo_type].top = self->height - 13;
        self->widgets[Widx::cargo_type].bottom = self->height - 2;

        self->widgets[Widx::cargo_type_btn].top = self->height - 12;
        self->widgets[Widx::cargo_type_btn].bottom = self->height - 3;

        // Disable cargo dropdown if not applicable
        if (self->var_88A != FilterMode::transportingCargo)
            self->disabledWidgets |= (1 << Widx::cargo_type) | (1 << Widx::cargo_type_btn);
        else
            self->disabledWidgets &= ~((1 << Widx::cargo_type) | (1 << Widx::cargo_type_btn));

        // Set appropriate tooltip
        static constexpr std::array<string_id, 3> filterTooltipByType = {
            StringIds::null,
            StringIds::tooltip_open_station_window_to_filter,
            StringIds::tooltip_select_cargo_type,
        };
        self->widgets[Widx::cargo_type_btn].tooltip = filterTooltipByType[self->var_88A];

        setTransportTypeTabs(self);
    }

    // 0x004C211C
    static void draw(Window* self, Gfx::Context* context)
    {
        self->draw(context);
        drawTabs(self, context);

        // Draw company owner image.
        auto company = CompanyManager::get(CompanyId(self->number));
        auto competitorObj = ObjectManager::get<CompetitorObject>(company->competitorId);
        uint32_t image = Gfx::recolour(competitorObj->images[company->ownerEmotion], company->mainColours.primary);
        uint16_t x = self->x + self->widgets[Widx::company_select].left + 1;
        uint16_t y = self->y + self->widgets[Widx::company_select].top + 1;
        Gfx::drawImage(context, x, y, image);

        static constexpr std::pair<string_id, string_id> typeToFooterStringIds[]{
            { StringIds::num_trains_singular, StringIds::num_trains_plural },
            { StringIds::num_buses_singular, StringIds::num_buses_plural },
            { StringIds::num_trucks_singular, StringIds::num_trucks_plural },
            { StringIds::num_trams_singular, StringIds::num_trams_plural },
            { StringIds::num_aircrafts_singular, StringIds::num_aircrafts_plural },
            { StringIds::num_ships_singular, StringIds::num_ships_plural },
        };

        FormatArguments args = {};

        {
            auto& footerStringPair = typeToFooterStringIds[self->currentTab];
            string_id footerStringId = self->var_83C == 1 ? footerStringPair.first : footerStringPair.second;

            args = FormatArguments::common(footerStringId, self->var_83C);
            Gfx::drawString_494B3F(*context, self->x + 3, self->y + self->height - 13, Colour::black, StringIds::black_stringid, &args);
        }

        static constexpr std::array<string_id, 3> typeToFilterStringIds{
            StringIds::all_vehicles,
            StringIds::stopping_at_station,
            StringIds::transporting_cargo,
        };

        {
            // Show current filter type
            string_id filter = typeToFilterStringIds[self->var_88A];
            args = FormatArguments::common(filter);
            auto* widget = &self->widgets[Widx::filter_type];
            Gfx::drawString_494BBF(*context, self->x + widget->left + 1, self->y + widget->top, widget->width() - 15, Colour::black, StringIds::wcolour2_stringid, &args);
        }

        auto* widget = &self->widgets[Widx::cargo_type];
        auto xPos = self->x + widget->left + 1;
        bool filterActive = false;

        if (isStationFilterActive(self, false))
        {
            filterActive = true;
            if (self->var_88C != -1)
            {
                auto station = StationManager::get(StationId(self->var_88C));
                args = FormatArguments::common(station->name, station->town);
            }
            else
            {
                args = FormatArguments::common(StringIds::no_station_selected);
            }
        }

        else if (isCargoFilterActive(self, false))
        {
            filterActive = true;
            if (self->var_88C != -1)
            {
                // Show current cargo
                auto cargoObj = ObjectManager::get<CargoObject>(self->var_88C);
                args = FormatArguments::common(StringIds::carrying_cargoid_sprite, cargoObj->name, cargoObj->unit_inline_sprite);

                // NB: the -9 in the xpos is to compensate for a hack due to the cargo dropdown limitation (only three args per item)
                xPos = self->x + widget->left - 9;
            }
            else
            {
                args = FormatArguments::common(StringIds::no_cargo_selected);
            }
        }

        if (filterActive)
        {
            // Draw filter text as prepared
            Gfx::drawString_494BBF(*context, xPos, self->y + widget->top, widget->width() - 15, Colour::black, StringIds::wcolour2_stringid, &args);
        }
    }

    // 0x004B6D43
    static void drawVehicle(VehicleHead* vehicle, Gfx::Context* context, uint16_t yPos)
    {
        registers regs;
        regs.esi = X86Pointer(vehicle);
        regs.edi = X86Pointer(context);
        regs.al = 0x40;
        regs.cx = 0;
        regs.dx = yPos;
        call(0x004B6D43, regs);
    }

    // 0x004C21CD
    static void drawScroll(Window& self, Gfx::Context& context, const uint32_t scrollIndex)
    {
        auto shade = Colours::getShade(self.getColour(WindowColour::secondary).c(), 1);
        Gfx::clearSingle(context, shade);

        auto yPos = 0;
        for (auto i = 0; i < self.var_83C; i++)
        {
            const auto vehicleId = EntityId(self.rowInfo[i]);

            // Item not in rendering context, or no vehicle available for this slot?
            if (yPos + self.rowHeight < context.y || yPos >= context.y + context.height + self.rowHeight || vehicleId == EntityId::null)
            {
                yPos += self.rowHeight;
                continue;
            }

            auto head = EntityManager::get<VehicleHead>(vehicleId);
            if (head == nullptr)
            {
                continue;
            }
            // Highlight selection.
            if (head->id == EntityId(self.rowHover))
                Gfx::drawRect(context, 0, yPos, self.width, self.rowHeight, Colours::getShade(self.getColour(WindowColour::secondary).c(), 0));

            // Draw vehicle at the bottom of the row.
            drawVehicle(head, &context, yPos + (self.rowHeight - 28) / 2 + 6);

            // Draw vehicle status
            {
                // Prepare status for drawing
                auto status = head->getStatus();
                auto args = FormatArguments::common();
                args.push(head->name);
                args.push(head->ordinalNumber);
                args.push(status.status1);
                args.push(status.status1Args);
                args.push(status.status2);
                args.push(status.status2Args);

                string_id format = StringIds::vehicle_list_status_2pos;
                if (status.status2 != StringIds::null)
                    format = StringIds::vehicle_list_status_3pos;

                // Draw status
                yPos += 2;
                Gfx::drawString_494BBF(context, 1, yPos, 308, AdvancedColour(Colour::black).outline(), format, &args);
            }

            auto vehicle = Vehicles::Vehicle(*head);

            // Vehicle profit
            {
                string_id format = StringIds::vehicle_list_profit_pos;
                currency32_t profit = vehicle.veh2->totalRecentProfit() / 4;
                if (profit < 0)
                {
                    format = StringIds::vehicle_list_profit_neg;
                    profit *= -1;
                }

                auto args = FormatArguments::common(profit);
                Gfx::drawString_494BBF(context, 310, yPos, 98, AdvancedColour(Colour::black).outline(), format, &args);
            }

            // Vehicle age
            {
                string_id format = StringIds::vehicle_list_age_years;
                auto age = (getCurrentDay() - vehicle.veh1->dayCreated) / 365;
                if (age == 1)
                    format = StringIds::vehicle_list_age_year;

                auto args = FormatArguments::common(age);
                Gfx::drawString_494BBF(context, 410, yPos, 63, AdvancedColour(Colour::black).outline(), format, &args);
            }

            // Vehicle reliability
            {
                int16_t reliability = vehicle.veh2->reliability;
                auto args = FormatArguments::common(reliability);
                Gfx::drawString_494BBF(context, 475, yPos, 65, AdvancedColour(Colour::black).outline(), StringIds::vehicle_list_reliability, &args);
            }

            yPos += self.rowHeight - 2;
        }
    }

    // 0x004C24F7
    static void switchTab(Window* self, VehicleType type)
    {
        if (Input::isToolActive(self->type, self->number))
            Input::toolCancel();

        auto tabIndex = static_cast<uint8_t>(type);
        self->currentTab = tabIndex;
        self->rowHeight = row_heights[tabIndex];
        self->frame_no = 0;

        if (CompanyManager::getControllingId() == CompanyId(self->number) && _lastVehiclesOption != type)
        {
            *_lastVehiclesOption = type;
            WindowManager::invalidate(WindowType::topToolbar);
        }

        // The original game was setting viewports and (enabled/disabled) widgets here.
        // As all tabs are the same, we've simplified this.

        disableUnavailableVehicleTypes(self);
        self->invalidate();

        if (self->width < 220)
            self->width = 220;

        self->rowCount = 0;
        refreshVehicleList(self);

        self->var_83C = 0;
        self->rowHover = -1;

        self->callOnResize();
        self->callOnPeriodicUpdate();
        self->callPrepareDraw();
        self->initScrollWidgets();
        self->invalidate();
        self->moveInsideScreenEdges();
    }

    // 0x004C2409
    static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::close_button:
                WindowManager::close(self);
                break;

            case Widx::tab_trains:
            case Widx::tab_buses:
            case Widx::tab_trucks:
            case Widx::tab_trams:
            case Widx::tab_aircraft:
            case Widx::tab_ships:
            {
                auto vehicleType = VehicleType(widgetIndex - Widx::tab_trains);
                switchTab(self, vehicleType);
                break;
            }

            case Widx::sort_name:
            case Widx::sort_profit:
            case Widx::sort_age:
            case Widx::sort_reliability:
            {
                auto sortMode = widgetIndex - Widx::sort_name;
                if (self->sortMode == sortMode)
                    return;

                self->sortMode = sortMode;
                self->invalidate();
                refreshVehicleList(self);
                break;
            }
        }
    }

    // 0x004C2434
    static void onMouseDown(Window* self, WidgetIndex_t widgetIndex)
    {
        if (widgetIndex == Widx::company_select)
            Dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);

        else if (widgetIndex == Widx::filter_type_btn)
        {
            Widget dropdown = self->widgets[Widx::filter_type];
            Dropdown::show(self->x + dropdown.left, self->y + dropdown.top, dropdown.width() - 4, dropdown.height(), self->getColour(WindowColour::secondary), 3, 0x80);

            Dropdown::add(0, StringIds::dropdown_stringid, StringIds::all_vehicles);
            Dropdown::add(1, StringIds::dropdown_stringid, StringIds::stopping_at_station);
            Dropdown::add(2, StringIds::dropdown_stringid, StringIds::transporting_cargo);
            Dropdown::setItemSelected(self->var_88A);
        }
        else if (widgetIndex == Widx::cargo_type_btn)
        {
            auto index = 0;
            auto selectedIndex = -1;
            for (uint16_t cargoId = 0; cargoId < ObjectManager::getMaxObjects(ObjectType::cargo); ++cargoId)
            {
                auto cargoObj = ObjectManager::get<CargoObject>(cargoId);
                if (cargoObj == nullptr)
                    continue;

                FormatArguments args{};
                args.push(cargoObj->name);
                args.push(cargoObj->unit_inline_sprite);
                args.push(cargoId);
                Dropdown::add(index, StringIds::carrying_cargoid_sprite, args);

                if (index == self->var_88C)
                    selectedIndex = index;

                index++;
            }

            Widget dropdown = self->widgets[Widx::cargo_type];
            Dropdown::showText(self->x + dropdown.left, self->y + dropdown.top, dropdown.width() - 4, dropdown.height(), self->getColour(WindowColour::secondary), index, 0);
            if (selectedIndex != -1)
                Dropdown::setItemSelected(selectedIndex);
        }
    }

    // 0x004C243F
    static void onCompanyDropdown(Ui::Window* self, int16_t itemIndex)
    {
        if (itemIndex == -1)
            return;

        CompanyId companyId = Dropdown::getCompanyIdFromSelection(itemIndex);

        // Try to find an open vehicle list for this company.
        auto companyWindow = WindowManager::bringToFront(WindowType::vehicleList, enumValue(companyId));
        if (companyWindow != nullptr)
            return;

        // If not, we'll turn this window into a window for the company selected.
        auto company = CompanyManager::get(companyId);
        if (company->name == StringIds::empty)
            return;

        self->number = enumValue(companyId);
        self->owner = companyId;

        disableUnavailableVehicleTypes(self);

        self->rowCount = 0;
        refreshVehicleList(self);

        self->var_83C = 0;
        self->rowHover = -1;

        self->callOnResize();
        self->callPrepareDraw();
        self->initScrollWidgets();
        self->invalidate();
    }

    static void onDropdown(Ui::Window* self, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex == Widx::company_select)
            return onCompanyDropdown(self, itemIndex);

        if (widgetIndex == filter_type_btn && itemIndex != -1)
        {
            if (self->var_88A != itemIndex)
            {
                self->var_88A = itemIndex;
                self->var_88C = -1;
            }
        }

        else if (widgetIndex == cargo_type_btn && itemIndex != -1)
        {
            self->var_88C = Dropdown::getItemArgument(itemIndex, 3);
        }
    }

    // 0x004C24CA
    static std::optional<FormatArguments> tooltip(Window* self, WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_vehicle_list);
        return args;
    }

    // 0x004C260B
    static void onUpdate(Window* self)
    {
        self->frame_no++;
        self->callPrepareDraw();

        auto widgetIndex = getTabFromType(static_cast<VehicleType>(self->currentTab));
        WindowManager::invalidateWidget(WindowType::vehicleList, self->number, widgetIndex);

        updateVehicleList(self);
        updateVehicleList(self);
        updateVehicleList(self);

        self->invalidate();
    }

    // 0x004C2640
    static void event_08(Window* self)
    {
        self->flags |= WindowFlags::notScrollView;
    }

    // 0x004C2648
    static void event_09(Window* self)
    {
        if (self->flags & WindowFlags::notScrollView)
        {
            self->rowHover = -1;
        }
    }

    // 0x004C265B
    static void getScrollSize(Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = self->var_83C * self->rowHeight;
    }

    // 0x004C266D
    static CursorId cursor(Window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, CursorId fallback)
    {
        if (widgetIdx != Widx::scrollview)
            return fallback;

        uint16_t currentIndex = yPos / self->rowHeight;
        if (currentIndex < self->var_83C && self->rowInfo[currentIndex] != -1)
            return CursorId::handPointer;

        return fallback;
    }

    // 0x004C26A4
    static void onScrollMouseOver(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        Input::setTooltipTimeout(2000);

        self->flags &= ~WindowFlags::notScrollView;

        uint16_t currentRow = y / self->rowHeight;
        if (currentRow < self->var_83C)
            self->rowHover = self->rowInfo[currentRow];
        else
            self->rowHover = -1;

        string_id tooltipId = StringIds::buffer_337;
        if (self->rowHover == -1)
            tooltipId = StringIds::null;

        char* tooltipBuffer = const_cast<char*>(StringManager::getString(StringIds::buffer_337));

        // Have we already got the right tooltip?
        if (tooltipBuffer[0] != '\0' && self->widgets[Widx::scrollview].tooltip == tooltipId && self->rowHover == self->var_85C)
            return;

        self->widgets[Widx::scrollview].tooltip = tooltipId;
        self->var_85C = self->rowHover;
        Ui::Windows::ToolTip::closeAndReset();

        if (self->rowHover == -1)
            return;

        // Initialise tooltip buffer.
        char* buffer = StringManager::formatString(tooltipBuffer, StringIds::vehicle_list_tooltip_load);

        // Append load to buffer.
        auto head = EntityManager::get<VehicleHead>(EntityId(self->var_85C));
        if (head == nullptr)
        {
            return;
        }
        buffer = head->generateCargoTotalString(buffer);

        // Figure out what stations the vehicle stops at.
        auto orders = Vehicles::OrderRingView(head->orderTableOffset);
        bool isFirstStop = true;
        for (auto& order : orders)
        {
            // Is this order a station?
            auto* stopOrder = order.as<Vehicles::OrderStopAt>();
            if (stopOrder == nullptr)
                continue;

            string_id stopFormat = StringIds::vehicle_list_tooltip_comma_stringid;
            if (isFirstStop)
                stopFormat = StringIds::vehicle_list_tooltip_stops_at_stringid;

            // Append station name to the tooltip buffer
            auto args = FormatArguments::common();
            stopOrder->setFormatArguments(args);
            buffer = StringManager::formatString(buffer, stopFormat, &args);

            isFirstStop = false;
        }
    }

    // 0x004C27C0
    static void onScrollMouseDown(Window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentRow = y / self->rowHeight;
        if (currentRow >= self->var_83C)
            return;

        EntityId currentVehicleId = EntityId(self->rowInfo[currentRow]);
        if (currentVehicleId == EntityId::null)
            return;

        auto* head = EntityManager::get<VehicleHead>(currentVehicleId);
        if (head == nullptr)
        {
            return;
        }

        if (head->isPlaced())
            Ui::Windows::Vehicle::Main::open(head);
        else
            Ui::Windows::Vehicle::Details::open(head);
    }

    // 0x004C2820
    static void onResize(Window* self)
    {
        self->flags |= WindowFlags::resizable;

        self->minWidth = min_dimensions.width;
        self->minHeight = min_dimensions.height;

        self->maxWidth = max_dimensions.width;
        self->maxHeight = max_dimensions.height;

        if (self->width < self->minWidth)
        {
            self->width = self->minWidth;
            self->invalidate();
        }

        if (self->height < self->minHeight)
        {
            self->height = self->minHeight;
            self->invalidate();
        }
    }

    static void initEvents()
    {
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;
        _events.drawScroll = drawScroll;
        _events.onMouseUp = onMouseUp;
        _events.onMouseDown = onMouseDown;
        _events.onDropdown = onDropdown;
        _events.tooltip = tooltip;
        _events.onUpdate = onUpdate;
        _events.event_08 = event_08;
        _events.event_09 = event_09;
        _events.getScrollSize = getScrollSize;
        _events.cursor = cursor;
        _events.scrollMouseDown = onScrollMouseDown;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.onResize = onResize;
    }
}
