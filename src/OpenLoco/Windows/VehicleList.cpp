#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../OpenLoco.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include <stdexcept>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::VehicleList
{
    static const Gfx::ui_size_t window_size = { 550, 213 };
    static const Gfx::ui_size_t max_dimensions = { 640, 1200 };
    static const Gfx::ui_size_t min_dimensions = { 192, 100 };

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

    constexpr uint64_t _enabledWidgets = (1 << Widx::close_button) | (1 << Widx::tab_trains) | (1 << Widx::tab_buses) | (1 << Widx::tab_trucks) | (1 << Widx::tab_trams) | (1 << Widx::tab_aircraft) | (1 << Widx::tab_ships) | (1 << Widx::company_select) | (1 << Widx::sort_name) | (1 << Widx::sort_profit) | (1 << Widx::sort_age) | (1 << Widx::sort_reliability) | (1 << Widx::scrollview);

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

    static void sub_4C1D4F(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x004C1D4F, regs);
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
        self->current_tab = VehicleList::getTabFromType(type);
        self->row_height = row_heights[type];
        self->width = window_size.width;
        self->height = window_size.height;
        self->sort_mode = 0;
        self->var_83C = 0;
        self->row_hover = -1;

        VehicleList::sub_4C1D4F(self);

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

    // 0x004C1F88
    static void prepareDraw(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x004C1F88, regs);
    }

    // 0x004C211C
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);

        registers regs;
        regs.esi = (int32_t)self;
        regs.edi = (int32_t)dpi;
        call(0x004C2121, regs);
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
                registers regs;
                regs.esi = (int32_t)self;
                regs.edx = widgetIndex;
                call(0x004C24F7, regs);
                break;
            }

            case Widx::sort_name:
            case Widx::sort_profit:
            case Widx::sort_age:
            case Widx::sort_reliability:
            {
                registers regs;
                regs.esi = (int32_t)self;
                regs.edx = widgetIndex;
                call(0x004C24D4, regs);
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
        sub_4C1D4F(self);

        self->var_83C = 0;
        self->row_hover = -1;

        self->callOnResize();
        self->callPrepareDraw();
        self->initScrollWidgets();
        self->invalidate();
    }

    static void initEvents()
    {
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;
        _events.on_mouse_up = onMouseUp;
        _events.on_mouse_down = onMouseDown;
        _events.on_dropdown = onDropdown;

        // TODO: the events below are not yet stubbed or implemented.
        // _events.cursor = cursor;
        // _events.draw_scroll = drawScroll;
        // _events.event_08 = event_08;
        // _events.event_09 = event_09;
        // _events.get_scroll_size = getScrollSize;
        // _events.on_resize = onResize;
        // _events.on_update = onUpdate;
        // _events.scroll_mouse_down = onScrollMouseDown;
        // _events.scroll_mouse_over = onScrollMouseOver;
        // _events.tooltip = tooltip;
    }
}
