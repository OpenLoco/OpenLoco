#include "../company.h"
#include "../companymgr.h"
#include "../config.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/competitor_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyWindow
{
    loco_global<uint16_t[8], 0x112C826> _common_format_args;

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

        // 0x00431E9B
        static void enableRenameByCaption(window* self)
        {
            if (is_editor_mode() || self->number == companymgr::get_controlling_id())
            {
                self->enabled_widgets |= (1 << caption);
            }
            else
            {
                self->enabled_widgets &= ~(1 << caption);
            }
        }

        // Defined at the bottom of this file.
        static void initEvents();
        static void switchTabWidgets(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void repositionTabs(window* self);
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

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << widx::centre_on_viewport) | (1 << widx::face) | (1 << widx::change_owner_name);

        static window_event_list events;

        // 0x00431EBB
        static void prepare_draw(window* self)
        {
            printf("In prepare draw for company window %p with number = %d\n", self, self->number);

            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            *_common_format_args = company->name;

            self->disabled_widgets &= ~((1 << widx::centre_on_viewport) | (1 << widx::face));

            // No centering on a viewport that doesn't exist.
            if (self->viewports[0] == nullptr)
                self->disabled_widgets |= (1 << widx::centre_on_viewport);

            // No changing other player's faces, unless we're editing a scenario.
            if (self->number != companymgr::get_controlling_id() && !is_editor_mode())
                self->disabled_widgets |= (1 << widx::face);

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[widx::viewport].right = self->width - 119;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::unk_11].top = self->height - 12;
            self->widgets[widx::unk_11].bottom = self->height - 3;
            self->widgets[widx::unk_11].right = self->width - 14;

            self->widgets[widx::change_owner_name].right = self->width - 4;
            self->widgets[widx::change_owner_name].left = self->width - 116;

            self->widgets[widx::face].right = self->width - 28;
            self->widgets[widx::face].left = self->width - 93;

            self->widgets[common::widx::company_select].right = self->width - 3;
            self->widgets[common::widx::company_select].left = self->width - 28;

            if (self->number == companymgr::get_controlling_id())
                self->widgets[widx::change_owner_name].type = widget_type::wt_9;
            else
                self->widgets[widx::change_owner_name].type = widget_type::none;

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            common::repositionTabs(self);
        }

        // 0x00432055
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            const auto company = companymgr::get(self->number);
            const auto competitor = objectmgr::get<competitor_object>(company->competitor_id);

            // Draw company owner face on dropdown.
            {
                const uint32_t image = gfx::recolour(competitor->images[company->owner_emotion], company->colour.primary);
                const uint16_t x = self->x + self->widgets[common::widx::company_select].left + 1;
                const uint16_t y = self->y + self->widgets[common::widx::company_select].top + 1;
                gfx::draw_image(dpi, x, y, image);
            }

            // Draw 'owner' label
            {
                auto& widget = self->widgets[widx::face];
                gfx::draw_string_centred(
                    *dpi,
                    self->x + (widget.left + widget.right) / 2,
                    self->y + widget.top - 12,
                    colour::black,
                    string_ids::window_owner,
                    nullptr);
            }

            // Draw company owner image.
            {
                const uint32_t image = gfx::recolour(competitor->images[company->owner_emotion], company->colour.primary) + 1;
                const uint16_t x = self->x + self->widgets[widx::face].left + 1;
                const uint16_t y = self->y + self->widgets[widx::face].top + 1;
                gfx::draw_image(dpi, x, y, image);
            }

            // If the owner's been naughty, draw some jail bars over them.
            if (company->jail_status != 0)
            {
                const uint32_t image = image_ids::owner_jailed;
                const uint16_t x = self->x + self->widgets[widx::face].left + 1;
                const uint16_t y = self->y + self->widgets[widx::face].top + 1;
                gfx::draw_image(dpi, x, y, image);
            }

            // Draw owner name
            {
                *_common_format_args = company->var_02;
                auto& widget = self->widgets[widx::change_owner_name];
                auto origin = gfx::point_t(self->x + (widget.left + widget.right) / 2, self->y + widget.top + 5);
                gfx::draw_string_centred_wrapped(
                    dpi,
                    &origin,
                    widget.right - widget.left,
                    colour::black,
                    string_ids::white_stringid2,
                    &*_common_format_args);
            }

            // Draw owner status
            {
                uint32_t arg1{};
                uint32_t arg2{};
                string_id status = companymgr::getOwnerStatus(self->number, &arg1, &arg2);

                *_common_format_args = status;
                *(uint32_t*)&_common_format_args[1] = arg1;
                *(uint32_t*)&_common_format_args[3] = arg2;

                auto& widget = self->widgets[widx::unk_11];
                gfx::draw_string_494BBF(
                    *dpi,
                    self->x + widget.left - 1,
                    self->y + widget.top - 1,
                    widget.right - widget.left,
                    colour::black,
                    string_ids::white_stringid2,
                    &*_common_format_args);
            }

            if (self->viewports[0] != nullptr)
            {
                self->drawViewports(dpi);
                widget::drawViewportCentreButton(dpi, self, (widget_index)widx::centre_on_viewport);
            }
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
            if (widgetIndex == common::widx::company_select)
                dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
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
            self->frame_no += 1;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::company, self->number);
        }

        // 0x00432724
        static void on_resize(window* self)
        {
            common::enableRenameByCaption(self);

            self->set_size(gfx::ui_size_t(270, 182), gfx::ui_size_t(640, 400));

            if (self->viewports[0] != nullptr)
            {
                gfx::ui_size_t proposedDims(self->width - 123, self->height - 59);
                auto& viewport = self->viewports[0];
                if (proposedDims.width != viewport->width || proposedDims.height != viewport->height)
                {
                    viewport->width = proposedDims.width;
                    viewport->height = proposedDims.height;
                    viewport->view_width = proposedDims.width << viewport->zoom;
                    viewport->view_height = proposedDims.height << viewport->zoom;
                    self->saved_view.clear();
                }
            }

            self->call_viewport_rotate();
        }

        // 0x004327C8
        static void viewport_rotate(window* self)
        {
            // We're skipping the tab check and dive straight into the business to avoid a prepare_draw call.
            // registers regs;
            // regs.esi = (int32_t)self;
            // call(0x004340C6);
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
            events.viewport_rotate = viewport_rotate;
        }
    }

    static window* sub_4347D0(company_id_t companyId)
    {
        const uint32_t newFlags = window_flags::scrolling_to_location | window_flags::stick_to_front;
        auto window = WindowManager::createWindow(WindowType::company, status::windowSize, newFlags, &status::events);
        printf("window: %p\n", window);
        window->number = companyId;
        printf("window->number = %d\n", window->number);
        window->owner = companyId;
        printf("window->owner = %d\n", window->owner);

        window->current_tab = 0;
        window->frame_no = 0;
        window->saved_view.clear();

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[1] = skin->colour_0A;

        window->flags |= window_flags::resizable;

        return window;
    }

    // 0x0043454F
    window* open(company_id_t companyId)
    {
        auto window = WindowManager::bringToFront(WindowType::company, companyId);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
            {
                input::cancel_tool();
                window = WindowManager::bringToFront(WindowType::company, companyId);
            }
        }

        printf("New company window (0x0043454F) with companyId = %d\n", companyId);

        if (window == nullptr)
        {
            window = sub_4347D0(companyId);
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
            self->frame_no += 1;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::company, self->number);
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
            self->frame_no += 1;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::company, self->number);
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
            self->frame_no += 1;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::company, self->number);
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
            self->frame_no += 1;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::company, self->number);
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
            self->frame_no += 1;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::company, self->number);
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

        static void switchTabWidgets(window* self)
        {
            self->activated_widgets = 0;

            static widget_t* widgetCollectionsByTabId[] = {
                status::widgets,
                details::widgets,
                colour_scheme::widgets,
                finances::widgets,
                cargo_delivered::widgets,
                challenge::widgets,
            };

            widget_t* newWidgets = widgetCollectionsByTabId[self->current_tab];
            if (self->widgets != newWidgets)
            {
                self->widgets = newWidgets;
                // self->init_scroll_widgets();
            }

            static const widx tabWidgetIdxByTabId[] = {
                tab_status,
                tab_details,
                tab_colour_scheme,
                tab_finances,
                tab_cargo_delivered,
                tab_challenge,
            };

            self->activated_widgets &= ~((1 << tab_status) | (1 << tab_details) | (1 << tab_colour_scheme) | (1 << tab_finances) | (1 << tab_cargo_delivered) | (1 << tab_challenge));
            self->activated_widgets |= (1ULL << tabWidgetIdxByTabId[self->current_tab]);
        }

        // 0x00434413
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Status tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::tab_company;
                widget::draw_tab(self, dpi, imageId, widx::tab_status);
            }

            // Details tab
            {
                const uint32_t imageId = gfx::recolour(skin->img + interface_skin::image_ids::tab_company_details, self->colours[0]);
                widget::draw_tab(self, dpi, imageId, widx::tab_details);
            }

            // Colour scheme tab
            {
                static const uint32_t colourSchemeTabImageIds[] = {
                    interface_skin::image_ids::tab_colour_scheme_frame0,
                    interface_skin::image_ids::tab_colour_scheme_frame1,
                    interface_skin::image_ids::tab_colour_scheme_frame2,
                    interface_skin::image_ids::tab_colour_scheme_frame3,
                    interface_skin::image_ids::tab_colour_scheme_frame4,
                    interface_skin::image_ids::tab_colour_scheme_frame5,
                    interface_skin::image_ids::tab_colour_scheme_frame6,
                    interface_skin::image_ids::tab_colour_scheme_frame7,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_colour_scheme - widx::tab_status)
                    imageId += colourSchemeTabImageIds[(self->frame_no / 4) % std::size(colourSchemeTabImageIds)];
                else
                    imageId += colourSchemeTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_colour_scheme);
            }

            // Finances tab
            {
                static const uint32_t financesTabImageIds[] = {
                    interface_skin::image_ids::tab_finances_frame0,
                    interface_skin::image_ids::tab_finances_frame1,
                    interface_skin::image_ids::tab_finances_frame2,
                    interface_skin::image_ids::tab_finances_frame3,
                    interface_skin::image_ids::tab_finances_frame4,
                    interface_skin::image_ids::tab_finances_frame5,
                    interface_skin::image_ids::tab_finances_frame6,
                    interface_skin::image_ids::tab_finances_frame7,
                    interface_skin::image_ids::tab_finances_frame8,
                    interface_skin::image_ids::tab_finances_frame9,
                    interface_skin::image_ids::tab_finances_frame10,
                    interface_skin::image_ids::tab_finances_frame11,
                    interface_skin::image_ids::tab_finances_frame12,
                    interface_skin::image_ids::tab_finances_frame13,
                    interface_skin::image_ids::tab_finances_frame14,
                    interface_skin::image_ids::tab_finances_frame15,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_finances - widx::tab_status)
                    imageId += financesTabImageIds[(self->frame_no / 2) % std::size(financesTabImageIds)];
                else
                    imageId += financesTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_finances);
            }

            // Cargo delivered tab
            {
                static const uint32_t cargoDeliveredTabImageIds[] = {
                    interface_skin::image_ids::tab_cargo_delivered_frame0,
                    interface_skin::image_ids::tab_cargo_delivered_frame1,
                    interface_skin::image_ids::tab_cargo_delivered_frame2,
                    interface_skin::image_ids::tab_cargo_delivered_frame3,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_cargo_delivered - widx::tab_status)
                    imageId += cargoDeliveredTabImageIds[(self->frame_no / 4) % std::size(cargoDeliveredTabImageIds)];
                else
                    imageId += cargoDeliveredTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_cargo_delivered);
            }

            // Challenge tab
            {
                static const uint32_t challengeTabImageIds[] = {
                    interface_skin::image_ids::tab_cup_frame0,
                    interface_skin::image_ids::tab_cup_frame1,
                    interface_skin::image_ids::tab_cup_frame2,
                    interface_skin::image_ids::tab_cup_frame3,
                    interface_skin::image_ids::tab_cup_frame4,
                    interface_skin::image_ids::tab_cup_frame5,
                    interface_skin::image_ids::tab_cup_frame6,
                    interface_skin::image_ids::tab_cup_frame7,
                    interface_skin::image_ids::tab_cup_frame8,
                    interface_skin::image_ids::tab_cup_frame9,
                    interface_skin::image_ids::tab_cup_frame10,
                    interface_skin::image_ids::tab_cup_frame11,
                    interface_skin::image_ids::tab_cup_frame12,
                    interface_skin::image_ids::tab_cup_frame13,
                    interface_skin::image_ids::tab_cup_frame14,
                    interface_skin::image_ids::tab_cup_frame15,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_challenge - widx::tab_status)
                    imageId += challengeTabImageIds[(self->frame_no / 4) % std::size(challengeTabImageIds)];
                else
                    imageId += challengeTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_challenge);
            }
        }

        // 0x004343BC
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_status].left;
            const int16_t tabWidth = self->widgets[widx::tab_status].right - xPos;

            for (uint8_t i = widx::tab_status; i <= widx::tab_challenge; i++)
            {
                if (self->is_disabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }
    }
}
