#include "../company.h"
#include "../companymgr.h"
#include "../config.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyWindow
{
    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_status,
            tab_details,
            tab_colour_scheme,
            tab_finances,
            tab_cargo_delivered,
            tab_challenge,
            company_select,
        };

        const uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_status) | (1 << widx::tab_details) | (1 << widx::tab_colour_scheme) | (1 << widx::tab_finances) | (1 << widx::tab_cargo_delivered) | (1 << widx::tab_challenge) | (1 << widx::company_select);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                                 \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                            \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),      \
        make_widget({ 0, 41 }, { frameWidth, 120 }, widget_type::panel, 1),                                                                    \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_owner_and_status),          \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_headquarters_and_details), \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_colour_scheme),            \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_finances),                 \
        make_remap_widget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_cargo_delivered),                 \
        make_remap_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_company_challenge_for_this_game), \
        make_widget({ 0, 14 }, { 26, 26 }, widget_type::wt_9, 0, image_ids::null, string_ids::tooltip_select_company)

        // 0x004343FC
        static void disableChallengeTab(window* self)
        {
            self->disabled_widgets = 0;
            if (self->number != companymgr::get_controlling_id())
                self->disabled_widgets |= (1 << widx::tab_challenge);
        }

        // Defined at the bottom of this file.
        static void initEvents();
    }

    namespace status
    {
        static const gfx::ui_size_t windowSize = { 270, 182 };

        enum widx
        {
            unk_11 = 11,
            viewport,
            centre_on_viewport,
            face,
            change_owner_name,
        };

        [[maybe_unused]] static widget_t widgets[] = {
            commonWidgets(270, 182, string_ids::title_company),
            make_widget({ 3, 160 }, { 242, 21 }, widget_type::wt_13, 1),
            make_widget({ 3, 44 }, { 96, 120 }, widget_type::viewport, 1, -2),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            make_widget({ 178, 57 }, { 66, 66 }, widget_type::wt_9, 1, image_ids::null),
            make_widget({ 154, 124 }, { 112, 22 }, widget_type::wt_9, 1, image_ids::null, string_ids::tooltip_change_owner_name),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x00431EBB
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00431EBB, regs);
        }

        // 0x00432055
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x00432055, regs);
        }

        // 0x00432244
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00432244, regs);
        }

        // 0x00432283
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00432283, regs);
        }

        // 0x0043228E
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.ax = itemIndex;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x0043228E, regs);
        }

        // 0x004322F6
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x004322F6, regs);
        }

        // 0x0043270A
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0043270A, regs);
        }

        // 0x00432724
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00432724, regs);
        }

        static void initEvents()
        {
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.text_input = text_input;
            events.on_update = on_update;
            events.on_resize = on_resize;
        }
    }

    // 0x0043454F
    window* open(company_id_t companyId)
    {
        auto window = WindowManager::bringToFront(WindowType::company, companyId);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            window = WindowManager::bringToFront(WindowType::company, companyId);
        }

        if (window == nullptr)
        {
            // 0x004347D0 start
            const uint32_t newFlags = window_flags::scrolling_to_location | window_flags::stick_to_front;
            window = WindowManager::createWindow(WindowType::company, status::windowSize, newFlags, &status::events);
            window->number = companyId;

            auto skin = objectmgr::get<interface_skin_object>();
            window->colours[1] = skin->colour_0A;
            // 0x004347D0 end

            window->saved_view.clear();
        }

        // TODO(avgeffen): only needs to be called once.
        common::initEvents();

        window->current_tab = 0;
        window->width = status::windowSize.width;
        window->height = status::windowSize.height;
        window->invalidate();

        window->widgets = status::widgets;
        window->enabled_widgets = status::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &status::events;
        window->activated_widgets = 0;

        common::disableChallengeTab(window);
        window->init_scroll_widgets();
        window->moveInsideScreenEdges();

        return window;
    }

    // 0x00435ACC
    window* openAndSetName()
    {
        company_id_t companyId = companymgr::get_controlling_id();
        window* self = open(companyId);

        // Allow setting company owner name if no preferred owner name has been set.
        if ((config::get().flags & config::flags::use_preferred_owner_name) == 0)
            status::on_mouse_up(self, status::widx::change_owner_name);

        return self;
    }

    namespace details
    {
        [[maybe_unused]] static widget_t widgets[] = {
            commonWidgets(340, 194, string_ids::title_company_details),
            make_widget({ 219, 54 }, { 96, 120 }, widget_type::viewport, 1, -2),
            make_widget({ 315, 92 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::tooltip_build_or_move_headquarters),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x004327CF
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x004327CF, regs);
        }

        // 0x00432919
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x00432919, regs);
        }

        // 0x00432BDD
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00432BDD, regs);
        }

        // 0x00432C08
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00432C08, regs);
        }

        // 0x00432C19
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.ax = itemIndex;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00432C19, regs);
        }

        // 0x00432C24
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x00432C24, regs);
        }

        // 0x00432CA1
        // static void event_10(window* self);

        // 0x00432D45
        // static void on_tool_down(window* self);

        // 0x00432D7A
        // static void on_tool_abort(window* self);

        // 0x0432D85
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0432D85, regs);
        }

        // 0x00432D9F
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00432D9F, regs);
        }

        // 0x00432E08
        // static void viewport_rotate(window* self);

        static void initEvents()
        {
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.on_dropdown = on_dropdown;
            events.text_input = text_input;
            // events.event_10 = event_10;
            // events.on_tool_down = on_tool_down;
            // events.on_tool_abort = on_tool_abort;
            events.on_update = on_update;
            events.on_resize = on_resize;
            // events.viewport_rotate = viewport_rotate;
        }
    }

    namespace colour_scheme
    {

        [[maybe_unused]] static widget_t widgets[] = {
            commonWidgets(265, 252, string_ids::title_company_colour_scheme),
            make_widget({ 15, 81 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_steam_locomotives, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 98 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_diesel_locomotives, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 115 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_electric_locomotives, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 132 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_multiple_units, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 149 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_passenger_vehicles, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 166 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_freight_vehicles, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 183 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_buses, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 200 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_trucks, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 217 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_aircraft, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 15, 234 }, { 204, 12 }, widget_type::checkbox, 1, string_ids::colour_ships, string_ids::tooltip_toggle_vehicle_colour_scheme),
            make_widget({ 221, 48 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 78 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 95 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 112 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 129 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 146 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 163 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 180 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 197 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 214 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 221, 231 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_main_colour),
            make_widget({ 239, 48 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 78 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 95 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 112 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 129 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 146 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 163 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 180 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 197 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 214 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            make_widget({ 239, 231 }, { 16, 16 }, widget_type::wt_10, 1, image_ids::null, string_ids::tooltip_select_secondary_colour),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x00432E0F
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00432E0F, regs);
        }

        // 0x00432F9A
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x00432F9A, regs);
        }

        // 0x00433032
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00433032, regs);
        }

        // 0x00433067
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00433067, regs);
        }

        // 0x00433092
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x00433092, regs);
        }

        // 0x0043309D
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.ax = itemIndex;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x0043309D, regs);
        }

        // 0x0043325F
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0043325F, regs);
        }

        // 0x00433279
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00433279, regs);
        }

        static void initEvents()
        {
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.text_input = text_input;
            events.on_dropdown = on_dropdown;
            events.on_update = on_update;
            events.on_resize = on_resize;
        }
    }

    namespace finances
    {
        [[maybe_unused]] static widget_t widgets[] = {
            commonWidgets(636, 319, string_ids::title_company_finances),
            make_widget({ 133, 45 }, { 499, 215 }, widget_type::scrollview, 1, horizontal),
            make_stepper_widgets({ 87, 264 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::company_current_loan_value),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x004332E4
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x004332E4, regs);
        }

        // 0x004333D0
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x004333D0, regs);
        }

        // 0x0043361E
        // static void draw_scroll(window* self);

        // 0x00433819
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00433819, regs);
        }

        // 0x0043383E
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x0043383E, regs);
        }

        // 0x0043385D
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x0043385D, regs);
        }

        // 0x00433868
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.ax = itemIndex;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00433868, regs);
        }

        // 0x0043386F
        // static void get_scroll_size(window* self);

        // 0x00433887
        // static void tooltip(window* self);

        // 0x0043399D
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0043399D, regs);
        }

        // 0x004339B7
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x004339B7, regs);
        }

        static void initEvents()
        {
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            // events.draw_scroll = draw_scroll;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.text_input = text_input;
            events.on_dropdown = on_dropdown;
            // events.get_scroll_size = get_scroll_size;
            // events.tooltip = tooltip;
            events.on_update = on_update;
            events.on_resize = on_resize;
        }
    }

    // 0x004345EE
    window* openFinances(company_id_t companyId)
    {
        registers regs;
        regs.eax = companyId;
        call(0x004345EE, regs);

        return (window*)regs.esi;
    }

    namespace cargo_delivered
    {
        [[maybe_unused]] static widget_t widgets[] = {
            commonWidgets(340, 382, string_ids::title_company_cargo_delivered),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x00433A22
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00433A22, regs);
        }

        // 0x00433ACD
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x00433ACD, regs);
        }

        // 0x00433BE6
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00433BE6, regs);
        }

        // 0x00433C0B
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00433C0B, regs);
        }

        // 0x00433C16
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x00433C16, regs);
        }

        // 0x00433C21
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            registers regs;
            regs.ax = itemIndex;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00433C21, regs);
        }

        // 0x00433C7D
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00433C7D, regs);
        }

        // 0x00433C97
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00433C97, regs);
        }

        static void initEvents()
        {
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.text_input = text_input;
            events.on_dropdown = on_dropdown;
            events.on_update = on_update;
            events.on_resize = on_resize;
        }
    }

    namespace challenge
    {
        [[maybe_unused]] static widget_t widgets[] = {
            commonWidgets(320, 182, string_ids::title_company_challenge),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x00433D39
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00433D39, regs);
        }

        // 0x00433DEB
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x00433DEB, regs);
        }

        // 0x00433FFE
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00433FFE, regs);
        }

        // 0x00434023
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x00434023, regs);
        }

        // 0x0043402E
        static void on_update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0043402E, regs);
        }

        // 0x00434048
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00434048, regs);
        }

        static void initEvents()
        {
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.text_input = text_input;
            events.on_update = on_update;
            events.on_resize = on_resize;
        }
    }

    namespace common
    {
        static void initEvents()
        {
            status::initEvents();
            details::initEvents();
            colour_scheme::initEvents();
            finances::initEvents();
            cargo_delivered::initEvents();
            challenge::initEvents();
        }
    }
}
