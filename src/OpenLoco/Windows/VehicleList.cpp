#include "../CompanyManager.h"
#include "../Entities/EntityManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../OpenLoco.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../Vehicles/Vehicle.h"
#include "../Widget.h"
#include <stdexcept>
#include <utility>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::VehicleList
{
    static loco_global<uint8_t, 0x00525FAF> _lastVehiclesOption;

    static const Gfx::ui_size_t window_size = { 550, 213 };
    static const Gfx::ui_size_t max_dimensions = { 550, 1200 };
    static const Gfx::ui_size_t min_dimensions = { 220, 160 };

    static window_event_list _events;

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
    };

    widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 550, 213 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 548, 13 }, widget_type::caption_24, 0),
        makeWidget({ 535, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 41 }, { 550, 172 }, widget_type::panel, 1),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_trains),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_buses),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_trucks),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_trams),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_aircraft),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_ships),
        makeWidget({ 0, 14 }, { 26, 26 }, widget_type::wt_9, 0, StringIds::null, StringIds::tooltip_select_company),
        makeWidget({ 4, 43 }, { 310, 12 }, widget_type::wt_14, 1, StringIds::null, StringIds::tooltip_sort_by_name),
        makeWidget({ 314, 43 }, { 100, 12 }, widget_type::wt_14, 1, StringIds::null, StringIds::tooltip_sort_by_profit),
        makeWidget({ 414, 43 }, { 65, 12 }, widget_type::wt_14, 1, StringIds::null, StringIds::tooltip_sort_by_age),
        makeWidget({ 479, 43 }, { 67, 12 }, widget_type::wt_14, 1, StringIds::null, StringIds::tooltip_sort_by_reliability),
        makeWidget({ 3, 56 }, { 544, 138 }, widget_type::scrollview, 1, scrollbars::vertical),
        widgetEnd()
    };

    constexpr uint16_t _tabWidgets = (1 << Widx::tab_trains) | (1 << Widx::tab_buses) | (1 << Widx::tab_trucks) | (1 << Widx::tab_trams) | (1 << Widx::tab_aircraft) | (1 << Widx::tab_ships);
    constexpr uint64_t _enabledWidgets = (1 << Widx::close_button) | _tabWidgets | (1 << Widx::company_select) | (1 << Widx::sort_name) | (1 << Widx::sort_profit) | (1 << Widx::sort_age) | (1 << Widx::sort_reliability) | (1 << Widx::scrollview);

    enum SortMode : uint16_t
    {
        Name,
        Profit,
        Age,
        Reliability,
    };

    static const uint8_t row_heights[] = {
        28,
        28,
        28,
        28,
        48,
        36
    };

    static Widx getTabFromType(uint8_t type);
    static void initEvents();

    // 0x004C1D4F
    static void refreshVehicleList(window* self)
    {
        self->row_count = 0;
        for (auto vehicle : EntityManager::VehicleList())
        {
            if (vehicle->vehicleType != static_cast<VehicleType>(self->current_tab))
                continue;

            if (vehicle->owner != self->number)
                continue;

            vehicle->var_0C &= ~Vehicles::Flags0C::sorted;
        }
    }

    using VehicleHead = Vehicles::VehicleHead;

    // 0x004C1E4F
    static bool orderByName(const VehicleHead& lhs, const VehicleHead& rhs)
    {
        char lhsString[256] = { 0 };
        StringManager::formatString(lhsString, lhs.name, (void*)&lhs.ordinalNumber);

        char rhsString[256] = { 0 };
        StringManager::formatString(rhsString, rhs.name, (void*)&rhs.ordinalNumber);

        return strcmp(lhsString, rhsString) < 0;
    }

    // 0x004C1EC9
    static bool orderByProfit(const VehicleHead& lhs, const VehicleHead& rhs)
    {
        auto profitL = Vehicles::Vehicle(&lhs).veh2->totalRecentProfit();
        auto profitR = Vehicles::Vehicle(&rhs).veh2->totalRecentProfit();

        return profitR - profitL < 0;
    }

    // 0x004C1F1E
    static bool orderByAge(const VehicleHead& lhs, const VehicleHead& rhs)
    {

        return false;
    }

    // 0x004C1F45
    static bool orderByReliability(const VehicleHead& lhs, const VehicleHead& rhs)
    {

        return false;
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
    static void updateVehicleList(window* self)
    {
        int16_t insertId = -1;

        for (auto vehicle : EntityManager::VehicleList())
        {
            if (vehicle->vehicleType != static_cast<VehicleType>(self->current_tab))
                continue;

            if (vehicle->owner != self->number)
                continue;

            if (vehicle->var_0C & Vehicles::Flags0C::sorted)
                continue;

            if (insertId == -1)
            {
                insertId = vehicle->id;
                continue;
            }

            auto insertVehicle = EntityManager::get<VehicleHead>(insertId);
            if (getOrder(SortMode(self->sort_mode), *vehicle, *insertVehicle))
            {
                insertId = vehicle->id;
                continue;
            }
        }

        if (insertId != -1)
        {
            auto vehicle = EntityManager::get<VehicleHead>(insertId);
            vehicle->var_0C |= Vehicles::Flags0C::sorted;

            if (vehicle->id != self->row_info[self->row_count])
                self->row_info[self->row_count] = vehicle->id;

            self->row_count++;

            if (self->row_count > self->var_83C)
                self->var_83C = self->row_count;
        }
        else
        {
            if (self->var_83C != self->row_count)
                self->var_83C = self->row_count;

            refreshVehicleList(self);
        }
    }

    // 0x004C2A6E
    static void drawTabs(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        auto skin = ObjectManager::get<InterfaceSkinObject>();
        auto companyColour = CompanyManager::getCompanyColour(self->number);

        static std::pair<widget_index, std::array<uint32_t, 8>> tabAnimations[] = {
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

            auto isActive = tab == self->current_tab + Widx::tab_trains;
            auto imageId = isActive ? frames[self->frame_no / 2 % 8] : frames[0];

            uint32_t image = Gfx::recolour(skin->img + imageId, companyColour);
            Widget::draw_tab(self, dpi, image, tab);
        }
    }

    // 0x004C28A5
    static void disableUnavailableVehicleTypes(window* self)
    {
        // The original game looks at all companies here. We only look at the current company instead.
        auto company = CompanyManager::get(self->number);

        // Disable the tabs for the vehicles that are _not_ available for this company.
        self->disabled_widgets = (company->available_vehicles ^ 0x3F) << Widx::tab_trains;
    }

    // 0x004C1AA2
    static window* create(uint16_t companyId)
    {
        window* self = WindowManager::createWindow(
            WindowType::vehicleList,
            window_size,
            WindowFlags::flag_11,
            &_events);

        self->widgets = _widgets;
        self->enabled_widgets = _enabledWidgets;
        self->number = companyId;
        self->owner = companyId;
        self->frame_no = 0;

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        self->colours[1] = skin->colour_0A;

        disableUnavailableVehicleTypes(self);

        return self;
    }

    // 0x004C19DC
    window* open(uint16_t companyId, uint8_t type)
    {
        window* self = WindowManager::bringToFront(WindowType::vehicleList, companyId);
        if (self != nullptr)
        {
            self->callOnMouseUp(VehicleList::getTabFromType(type));
            return self;
        }

        initEvents();

        // 0x004C1A05
        self = create(companyId);
        self->current_tab = type;
        self->row_height = row_heights[type];
        self->width = window_size.width;
        self->height = window_size.height;
        self->sort_mode = 0;
        self->var_83C = 0;
        self->row_hover = -1;

        refreshVehicleList(self);

        self->invalidate();

        self->callOnResize();
        self->callPrepareDraw();
        self->initScrollWidgets();

        return self;
    }

    static Widx getTabFromType(uint8_t type)
    {
        if (type > 5)
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
        return type_to_widx[type];
    }

    // 0x4C2865
    static void setTransportTypeTabs(window* self)
    {
        auto disabledWidgets = self->disabled_widgets >> Widx::tab_trains;
        auto widget = self->widgets + Widx::tab_trains;
        auto tabWidth = widget->right - widget->left;
        auto tabX = widget->left;
        for (auto i = 0; i <= Widx::tab_ships - Widx::tab_trains; ++i, ++widget)
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

    // 0x004C1F88
    static void prepareDraw(window* self)
    {
        // The original game was setting widget sets here. As all tabs are the same, this has been omitted.
        self->activated_widgets &= ~_tabWidgets;
        self->activated_widgets |= 1ULL << (self->current_tab + Widx::tab_trains);

        auto company = CompanyManager::get(self->number);
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
        self->widgets[Widx::caption].text = typeToCaption[self->current_tab];

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
        self->widgets[Widx::sort_name].text = self->sort_mode == SortMode::Name ? StringIds::table_header_name_desc : StringIds::table_header_name;
        self->widgets[Widx::sort_profit].text = self->sort_mode == SortMode::Profit ? StringIds::table_header_monthly_profit_desc : StringIds::table_header_monthly_profit;
        self->widgets[Widx::sort_age].text = self->sort_mode == SortMode::Age ? StringIds::table_header_age_desc : StringIds::table_header_age;
        self->widgets[Widx::sort_reliability].text = self->sort_mode == SortMode::Reliability ? StringIds::table_header_reliability_desc : StringIds::table_header_reliability;

        setTransportTypeTabs(self);
    }

    // 0x004C211C
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        drawTabs(self, dpi);

        // Draw company owner image.
        auto company = CompanyManager::get(self->number);
        auto competitor = ObjectManager::get<CompetitorObject>(company->competitor_id);
        uint32_t image = Gfx::recolour(competitor->images[company->owner_emotion], company->mainColours.primary);
        uint16_t x = self->x + self->widgets[Widx::company_select].left + 1;
        uint16_t y = self->y + self->widgets[Widx::company_select].top + 1;
        Gfx::drawImage(dpi, x, y, image);

        static constexpr std::pair<string_id, string_id> typeToFooterStringIds[]{
            { StringIds::num_trains_singular, StringIds::num_trains_plural },
            { StringIds::num_buses_singular, StringIds::num_buses_plural },
            { StringIds::num_trucks_singular, StringIds::num_trucks_plural },
            { StringIds::num_trams_singular, StringIds::num_trams_plural },
            { StringIds::num_aircrafts_singular, StringIds::num_aircrafts_plural },
            { StringIds::num_ships_singular, StringIds::num_ships_plural },
        };

        auto& footerStringPair = typeToFooterStringIds[self->current_tab];
        string_id footerStringId = self->var_83C == 1 ? footerStringPair.first : footerStringPair.second;

        auto args = FormatArguments::common(footerStringId, self->var_83C);
        Gfx::drawString_494B3F(*dpi, self->x + 3, self->y + self->height - 13, Colour::black, StringIds::black_stringid, &args);
    }

    // 0x004C21CD
    static void drawScroll(window* self, Gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        registers regs;
        regs.esi = (int32_t)self;
        regs.edi = (int32_t)dpi;
        regs.eax = (int32_t)scrollIndex;
        call(0x004C21CD, regs);
    }

    // 0x004C24F7
    static void switchTab(window* self, uint8_t type)
    {
        if (Input::isToolActive(self->type, self->number))
            Input::toolCancel();

        self->current_tab = type;
        self->row_height = row_heights[type];
        self->frame_no = 0;

        if (CompanyManager::getControllingId() == self->number && _lastVehiclesOption != type)
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

        self->row_count = 0;
        refreshVehicleList(self);

        self->var_83C = 0;
        self->row_hover = -1;

        self->callOnResize();
        self->callOnPeriodicUpdate();
        self->callPrepareDraw();
        self->initScrollWidgets();
        self->invalidate();
        self->moveInsideScreenEdges();
    }

    // 0x004C2409
    static void onMouseUp(window* self, widget_index widgetIndex)
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
                switchTab(self, widgetIndex - Widx::tab_trains);
                break;
            }

            case Widx::sort_name:
            case Widx::sort_profit:
            case Widx::sort_age:
            case Widx::sort_reliability:
            {
                auto sortMode = widgetIndex - Widx::sort_name;
                if (self->sort_mode == sortMode)
                    return;

                self->sort_mode = sortMode;
                self->invalidate();
                refreshVehicleList(self);
                break;
            }
        }
    }

    // 0x004C2434
    static void onMouseDown(window* self, widget_index widgetIndex)
    {
        if (widgetIndex == Widx::company_select)
            Dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
    }

    // 0x004C243F
    static void onDropdown(Ui::window* self, widget_index widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex != Widx::company_select)
            return;

        if (itemIndex == -1)
            return;

        company_id_t companyId = Dropdown::getCompanyIdFromSelection(itemIndex);

        // Try to find an open vehicle list for this company.
        auto companyWindow = WindowManager::bringToFront(WindowType::vehicleList, companyId);
        if (companyWindow != nullptr)
            return;

        // If not, we'll turn this window into a window for the company selected.
        auto company = CompanyManager::get(companyId);
        if (company->name == StringIds::empty)
            return;

        self->number = companyId;
        self->owner = companyId;

        disableUnavailableVehicleTypes(self);

        self->row_count = 0;
        refreshVehicleList(self);

        self->var_83C = 0;
        self->row_hover = -1;

        self->callOnResize();
        self->callPrepareDraw();
        self->initScrollWidgets();
        self->invalidate();
    }

    // 0x004C24CA
    static std::optional<FormatArguments> tooltip(window* self, widget_index widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_vehicle_list);
        return args;
    }

    // 0x004C260B
    static void onUpdate(window* self)
    {
        self->frame_no++;
        self->callPrepareDraw();
        WindowManager::invalidateWidget(WindowType::vehicleList, self->number, getTabFromType(self->current_tab));

        updateVehicleList(self);
        updateVehicleList(self);
        updateVehicleList(self);

        self->invalidate();
    }

    // 0x004C2640
    static void event_08(window* self)
    {
        self->flags |= WindowFlags::not_scroll_view;
    }

    // 0x004C2648
    static void event_09(window* self)
    {
        if (self->flags & WindowFlags::not_scroll_view)
        {
            self->row_hover = -1;
        }
    }

    // 0x004C265B
    static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = self->var_83C * self->row_height;
    }

    // 0x004C266D
    static cursor_id cursor(window* self, int16_t widgetIdx, int16_t xPos, int16_t yPos, cursor_id fallback)
    {
        if (widgetIdx != Widx::scrollview)
            return fallback;

        uint16_t currentIndex = yPos / self->row_height;
        if (currentIndex < self->var_83C && self->row_info[currentIndex] != -1)
            return cursor_id::hand_pointer;

        return fallback;
    }

    // 0x004C26A4
    static void onScrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x004C26A4, regs);
    }

    // 0x004C27C0
    static void onScrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x004C27C0, regs);
    }

    // 0x004C2820
    static void onResize(window* self)
    {
        self->flags |= WindowFlags::resizable;

        self->min_width = min_dimensions.width;
        self->min_height = min_dimensions.height;

        self->max_width = max_dimensions.width;
        self->max_height = max_dimensions.height;

        if (self->width < self->min_width)
        {
            self->width = self->min_width;
            self->invalidate();
        }

        if (self->height < self->min_height)
        {
            self->height = self->min_height;
            self->invalidate();
        }
    }

    static void initEvents()
    {
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
        _events.on_mouse_up = onMouseUp;
        _events.on_mouse_down = onMouseDown;
        _events.on_dropdown = onDropdown;
        _events.tooltip = tooltip;
        _events.on_update = onUpdate;
        _events.event_08 = event_08;
        _events.event_09 = event_09;
        _events.get_scroll_size = getScrollSize;
        _events.cursor = cursor;
        _events.scroll_mouse_down = onScrollMouseDown;
        _events.scroll_mouse_over = onScrollMouseOver;
        _events.on_resize = onResize;
    }
}
