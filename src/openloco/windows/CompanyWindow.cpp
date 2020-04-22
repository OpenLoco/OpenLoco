#include "../company.h"
#include "../companymgr.h"
#include "../config.h"
#include "../date.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../localisation/string_ids.h"
#include "../management/Expenditures.h"
#include "../objects/cargo_object.h"
#include "../objects/competitor_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"
#include "../ui/WindowManager.h"
#include "../ui/dropdown.h"
#include "../viewportmgr.h"
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
        static void renameCompanyPrompt(window* self, widget_index widgetIndex);
        static void switchTab(window* self, widget_index widgetIndex);
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

        static widget_t widgets[] = {
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
                *_common_format_args = company->owner_name;
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
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameCompanyPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_status:
                case common::widx::tab_details:
                case common::widx::tab_colour_scheme:
                case common::widx::tab_finances:
                case common::widx::tab_cargo_delivered:
                case common::widx::tab_challenge:
                    common::switchTab(self, widgetIndex);
                    break;

                case widx::centre_on_viewport:
                {
                    if (self->viewports[0] == nullptr)
                        break;

                    // Centre viewport on tile/thing.
                    // TODO(avgeffen): move/implement.
                    registers regs;
                    regs.esi = (int32_t)self;
                    call(0x004324DC, regs);
                    break;
                }

                case widx::face:
                {
                    // Open face selection window
                    // TODO(avgeffen): move/implement.
                    registers regs;
                    regs.eax = self->number;
                    call(0x00434F52, regs);
                    break;
                }

                case widx::change_owner_name:
                {
                    auto company = companymgr::get(self->number);
                    _common_format_args[2] = company->owner_name;
                    textinput::open_textinput(self, string_ids::title_name_owner, string_ids::prompt_enter_new_name_for_owner, company->owner_name, widgetIndex, &_common_format_args[2]);
                    break;
                }
            }

            // registers regs;
            // regs.edx = widgetIndex;
            // regs.esi = (int32_t)self;
            // call(0x00432244, regs);
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

            self->set_size(status::windowSize, gfx::ui_size_t(640, 400));

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
            self->call_prepare_draw();

            const auto& company = companymgr::get(self->number);

            if (company->observation_thing == thing_id::null)
            {
                // Observing a certain location?
                if (company->observation_x != -1)
                {
                    coord_t tileZ = openloco::map::tile_element_height(company->observation_x, company->observation_y) & 0xFFFF;
                    // if ((tileZ & 0xFFFF0000) == 0)
                    //     tileZ >>= 16;

                    // loc_43410A
                    int8_t rotation = static_cast<int8_t>(self->viewports[0]->getRotation());
                    SavedView view(
                        company->observation_x,
                        company->observation_y,
                        ZoomLevel::quarter,
                        rotation,
                        static_cast<int16_t>(tileZ));

                    if (self->viewports[0] == nullptr || self->saved_view.hasUnkFlag15())
                    {
                        self->saved_view = view;

                        // sub_434336
                        {
                            auto& widget = self->widgets[widx::viewport];
                            auto tile = openloco::map::map_pos3({ company->observation_x, company->observation_y, tileZ });
                            auto origin = gfx::point_t(widget.left + self->x + 1, widget.top + self->y + 1);
                            auto size = gfx::ui_size_t(widget.width() - 2, widget.height() - 2);
                            viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, tile);
                        }
                    }

                    // Centre viewport on tile.
                    registers regs;
                    regs.ax = self->saved_view.mapX;
                    regs.cx = self->saved_view.mapY & 0x3FFF;
                    regs.dx = self->saved_view.surfaceZ;
                    regs.esi = (int32_t)self;
                    call(0x004C6827, regs);
                }
                // Not observing anything at all?
                else
                {
                    // loc_434247
                    if (self->viewports[0] != nullptr)
                    {
                        self->viewports[0]->width = 0;
                        self->viewports[0] = nullptr;
                        self->invalidate();
                    }
                    return;
                }
            }
            else
            {
                // loc_434170
                auto thing = thingmgr::get<openloco::vehicle>(company->observation_thing);

                if (thing->base_type != thing_base_type::vehicle || thing->type != vehicle_thing_type::vehicle_0 || (thing->x & (1 << 15)) == 0)
                {
                    if (self->viewports[0] != nullptr)
                    {
                        self->viewports[0]->width = 0;
                        self->viewports[0] = nullptr;
                        self->invalidate();
                    }
                    return;
                }

                // loc_43419F
                auto car = thing->next_car()->next_car()->next_car()->next_car();

                int8_t rotation = static_cast<int8_t>(self->viewports[0]->getRotation());
                SavedView view(
                    car->next_car_id,
                    0xC000,
                    ZoomLevel::full,
                    rotation,
                    0);

                if (self->viewports[0] == nullptr)
                {
                    self->saved_view = view;

                    // sub_434336
                    {
                        auto& widget = self->widgets[widx::viewport];
                        auto thingId = self->saved_view.thingId;
                        auto origin = gfx::point_t(widget.left + self->x + 1, widget.top + self->y + 1);
                        auto size = gfx::ui_size_t(widget.width() - 2, widget.height() - 2);
                        viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, thingId);
                    }
                }

                // Centre viewport on tile.
                registers regs;
                regs.ax = self->saved_view.mapX;
                regs.cx = self->saved_view.mapY & 0x3FFF;
                regs.dx = self->saved_view.surfaceZ;
                regs.esi = (int32_t)self;
                call(0x004C6827, regs);
            }

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
        window->number = companyId;
        window->owner = companyId;
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
        const gfx::ui_size_t windowSize = { 340, 194 };

        static widget_t widgets[] = {
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
        const gfx::ui_size_t windowSize = { 265, 252 };

        static widget_t widgets[] = {
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
        const gfx::ui_size_t windowSize = { 636, 319 };

        enum widx
        {
            scrollview = 11,
            current_loan,
            loan_decrease,
            loan_increase,
        };

        static widget_t widgets[] = {
            commonWidgets(636, 319, string_ids::title_company_finances),
            make_widget({ 133, 45 }, { 499, 215 }, widget_type::scrollview, 1, horizontal),
            make_stepper_widgets({ 87, 264 }, { 100, 12 }, widget_type::wt_17, 1, string_ids::company_current_loan_value),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        const uint64_t holdableWidgets = (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        static window_event_list events;

        // 0x004332E4
        static void prepare_draw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            *_common_format_args = company->name;

            *(uint32_t*)&_common_format_args[4] = company->current_loan;

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::company_select].right = self->width - 3;
            self->widgets[common::widx::company_select].left = self->width - 28;

            if (self->number == companymgr::get_controlling_id())
            {
                self->widgets[widx::current_loan].type = widget_type::wt_17;
                self->widgets[widx::loan_decrease].type = widget_type::wt_11;
                self->widgets[widx::loan_increase].type = widget_type::wt_11;
            }
            else
            {
                self->widgets[widx::current_loan].type = widget_type::none;
                self->widgets[widx::loan_decrease].type = widget_type::none;
                self->widgets[widx::loan_increase].type = widget_type::none;
            }

            common::repositionTabs(self);
        }

        // 0x004333D0
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

            // Draw 'expenditure/income' label
            {
                gfx::draw_string_left_underline(
                    *dpi,
                    self->x + 5,
                    self->y + 47,
                    colour::black,
                    string_ids::expenditure_income,
                    nullptr);
            }

            const string_id ExpenditureLabels[] = {
                string_ids::train_income,
                string_ids::train_running_costs,
                string_ids::bus_income,
                string_ids::bus_running_costs,
                string_ids::truck_income,
                string_ids::truck_running_costs,
                string_ids::tram_income,
                string_ids::tram_running_costs,
                string_ids::aircraft_income,
                string_ids::aircraft_running_costs,
                string_ids::ship_income,
                string_ids::ship_running_costs,
                string_ids::construction,
                string_ids::vehicle_purchases,
                string_ids::vehicle_disposals,
                string_ids::loan_interest,
                string_ids::miscellaneous,
            };

            uint16_t y = self->y + 62;
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(ExpenditureLabels)); i++)
            {
                // Add zebra stripes to even labels.
                if (i % 2 == 0)
                {
                    auto colour = colour::get_shade(self->colours[1], 6) | 0x1000000;
                    gfx::fill_rect(dpi, 4, y, 129, y + 9, colour);
                }

                _common_format_args[0] = ExpenditureLabels[i];
                gfx::draw_string_494B3F(
                    *dpi,
                    self->x + 5,
                    y - 1,
                    colour::black,
                    string_ids::wcolour2_stringid2,
                    _common_format_args);

                y += 10;
            }

            // 'Current loan' label
            {
                gfx::draw_string_494B3F(
                    *dpi,
                    self->x + 7,
                    self->y + self->widgets[widx::current_loan].top,
                    colour::black,
                    string_ids::company_current_loan);
            }

            // '@ X% interest per' label
            {
                loco_global<uint8_t, 0x00525FC6> loanInterestRate;
                _common_format_args[0] = *loanInterestRate;
                gfx::draw_string_494B3F(
                    *dpi,
                    self->x + self->widgets[widx::current_loan].right + 3,
                    self->y + self->widgets[widx::current_loan].top + 1,
                    colour::black,
                    string_ids::interest_per_year,
                    _common_format_args);
            }

            // 'Cash' label with value
            {
                // Set cash value in format args.
                *(uint32_t*)&_common_format_args[0] = company->cash.var_00;
                *(uint16_t*)&_common_format_args[2] = company->cash.var_04;

                string_id cash_format = string_ids::cash_positive;
                if ((company->challenge_flags & company_flags::bankrupt) != 0)
                    cash_format = string_ids::cash_bankrupt;
                if (company->cash.var_04 < 0)
                    cash_format = string_ids::cash_negative;

                gfx::draw_string_494B3F(
                    *dpi,
                    self->x + 7,
                    self->y + self->widgets[widx::current_loan].top + 13,
                    colour::black,
                    cash_format,
                    _common_format_args);
            }

            // 'Company value' label with value
            {
                // Set company value in format args.
                *(uint32_t*)&_common_format_args[0] = company->companyValue.var_00;
                *(uint16_t*)&_common_format_args[2] = company->companyValue.var_04;

                gfx::draw_string_494B3F(
                    *dpi,
                    self->x + 7,
                    self->y + self->widgets[widx::current_loan].top + 26,
                    colour::black,
                    string_ids::company_value,
                    _common_format_args);
            }

            // 'Profit from vehicles' label with value
            {
                // Set company value in format args.
                *(uint32_t*)&_common_format_args[0] = company->vehicleProfit.var_00;
                *(uint16_t*)&_common_format_args[2] = company->vehicleProfit.var_04;

                gfx::draw_string_494B3F(
                    *dpi,
                    self->x + 7,
                    self->y + self->widgets[widx::current_loan].top + 39,
                    colour::black,
                    string_ids::profit_from_vehicles,
                    _common_format_args);
            }
        }

        // 0x0043361E
        static void draw_scroll(window* self, gfx::drawpixelinfo_t* context, uint32_t scrollIndex)
        {
            int16_t y = 47 - self->widgets[widx::scrollview].top + 14;

            for (uint8_t i = 0; i < static_cast<uint8_t>(ExpenditureType::Count); i++)
            {
                // Add zebra stripes to even labels.
                if (i % 2 == 0)
                {
                    auto colour = colour::get_shade(self->colours[1], 6) | 0x1000000;
                    gfx::fill_rect(context, 0, y, 2176, y + 9, colour);
                }

                y += 10;
            }

            const auto company = companymgr::get(self->number);

            uint32_t maxYear = current_year();
            uint8_t expenditureYears = std::min<uint8_t>(company->numExpenditureMonths, expenditureHistoryCapacity);

            // Paint years on top of scroll area.
            int16_t x = 132 - self->widgets[widx::scrollview].left;
            y = 46 - self->widgets[widx::scrollview].top;
            for (auto i = 0; i <= expenditureYears; i++)
            {
                FormatArguments args = {};
                args.push(string_ids::uint16_raw);

                uint16_t currentYear = maxYear - (expenditureYears - i);
                args.push(currentYear);

                string_id format = string_ids::wcolour2_stringid2;
                if (i == expenditureYears)
                    format = string_ids::white_stringid2;

                gfx::draw_string_underline(
                    *context,
                    x,
                    y,
                    colour::black,
                    format,
                    &args);

                x += 128;
            }

            y += 14;
            for (auto i = 0; i <= expenditureYears; i++)
            {
                currency48_t sum = 0;
                for (auto j = 0; j <= ExpenditureType::Count; j++)
                {
                    printf("i,j = (%d,%d) = %d\n", i, j, company->expenditures[i][j]);
                    currency48_t expenditures = company->expenditures[i][j];
                    sum += expenditures;

                    string_id mainFormat = string_ids::white_stringid2;
                    string_id currFormat = string_ids::plus_currency48;
                    if (expenditures < 0)
                    {
                        mainFormat = string_ids::red_stringid;
                        currFormat = string_ids::currency48;
                    }

                    FormatArguments args = {};
                    args.push<string_id>(currFormat);
                    args.push<currency48_t>(expenditures);

                    gfx::draw_string_494C78(
                        *context,
                        x,
                        y,
                        colour::black,
                        mainFormat,
                        &args);

                    y += 10;
                }

                // TODO: print sum
                // ...

                // TODO: probably incorrect.
                x += 128;
                y = 46 - self->widgets[widx::scrollview].top + 14;
            }

            // registers regs;
            // regs.edi = (int32_t)context;
            // regs.esi = (int32_t)self;
            // call(0x0043378C, regs);
        }

        // 0x00433819
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameCompanyPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_status:
                case common::widx::tab_details:
                case common::widx::tab_colour_scheme:
                case common::widx::tab_finances:
                case common::widx::tab_cargo_delivered:
                case common::widx::tab_challenge:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x0043383E
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::company_select:
                    dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
                    break;

                case widx::loan_decrease:
                {
                    registers regs;
                    regs.edx = widgetIndex;
                    regs.esi = (int32_t)self;
                    call(0x0043393A, regs);
                    break;
                }

                case widx::loan_increase:
                {
                    registers regs;
                    regs.edx = widgetIndex;
                    regs.esi = (int32_t)self;
                    call(0x004338EB, regs);
                    break;
                }
            }
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
        static void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            const auto& company = companymgr::get(self->number);
            *scrollWidth = company->numExpenditureMonths * 128;
        }

        // 0x00433887
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(string_ids::tooltip_scroll_list);
        }

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
            common::enableRenameByCaption(self);
            self->set_size(windowSize);
        }

        static void initEvents()
        {
            events.prepare_draw = prepare_draw;
            events.draw = draw;
            events.draw_scroll = draw_scroll;
            events.on_mouse_up = on_mouse_up;
            events.on_mouse_down = on_mouse_down;
            events.text_input = text_input;
            events.on_dropdown = on_dropdown;
            events.get_scroll_size = get_scroll_size;
            events.tooltip = tooltip;
            events.on_update = on_update;
            events.on_resize = on_resize;
        }

        static void sub_4C8DBF(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x004C8DBF, regs);
        }
    }

    // 0x004345EE
    window* openFinances(company_id_t companyId)
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

        if (window == nullptr)
        {
            window = sub_4347D0(companyId);
        }

        // TODO(avgeffen): only needs to be called once.
        common::initEvents();

        window->current_tab = common::tab_finances - common::tab_status;
        window->width = finances::windowSize.width;
        window->height = finances::windowSize.height;
        window->invalidate();

        window->widgets = finances::widgets;
        window->enabled_widgets = finances::enabledWidgets;
        window->holdable_widgets = finances::holdableWidgets;
        window->event_handlers = &finances::events;
        window->activated_widgets = 0;

        common::disableChallengeTab(window);
        window->init_scroll_widgets();
        window->moveInsideScreenEdges();
        finances::sub_4C8DBF(window);

        return window;
    }

    namespace cargo_delivered
    {
        const gfx::ui_size_t windowSize = { 240, 382 };

        static widget_t widgets[] = {
            commonWidgets(240, 382, string_ids::title_company_cargo_delivered),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x00433A22
        static void prepare_draw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            *_common_format_args = company->name;

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            common::repositionTabs(self);
        }

        // 0x00433ACD
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            uint16_t y = self->y + 47;

            // 'Cargo delivered'
            gfx::draw_string_494B3F(
                *dpi,
                self->x + 5,
                y,
                colour::black,
                string_ids::cargo_delivered);

            y += 10;

            uint8_t numPrinted = 0;
            const auto company = companymgr::get(self->number);
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(company->cargoDelivered)); i++)
            {
                auto cargo = objectmgr::get<cargo_object>(i);
                if (cargo == nullptr || company->cargoDelivered[i] == 0)
                    continue;

                if (company->cargoDelivered[i] == 1)
                    _common_format_args[0] = cargo->unit_name_singular;
                else
                    _common_format_args[0] = cargo->unit_name_plural;

                *(uint32_t*)&_common_format_args[1] = company->cargoDelivered[i];

                gfx::draw_string_494B3F(
                    *dpi,
                    self->x + 10,
                    y,
                    colour::black,
                    string_ids::white_stringid2,
                    _common_format_args);

                numPrinted++;
                y += 10;
            }

            // No cargo delivered yet?
            if (numPrinted == 0)
            {
                gfx::draw_string_494B3F(
                    *dpi,
                    self->x + 10,
                    y,
                    colour::black,
                    string_ids::cargo_delivered_none);
            }
        }

        // 0x00433BE6
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameCompanyPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_status:
                case common::widx::tab_details:
                case common::widx::tab_colour_scheme:
                case common::widx::tab_finances:
                case common::widx::tab_cargo_delivered:
                case common::widx::tab_challenge:
                    common::switchTab(self, widgetIndex);
                    break;
            }
        }

        // 0x00433C0B
        static void on_mouse_down(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::company_select)
                dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
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
            common::enableRenameByCaption(self);

            uint16_t cargoHeight = 0;
            const auto company = companymgr::get(self->number);
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(company->cargoDelivered)); i++)
            {
                auto cargo = objectmgr::get<cargo_object>(i);
                if (cargo == nullptr || company->cargoDelivered[i] == 0)
                    continue;

                cargoHeight += 10;
            }

            const uint16_t windowHeight = std::max<int16_t>(cargoHeight, 50) + 62;

            self->set_size({ windowSize.width, windowHeight });
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
        const gfx::ui_size_t windowSize = { 320, 182 };

        static widget_t widgets[] = {
            commonWidgets(320, 182, string_ids::title_company_challenge),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x00433D39
        static void prepare_draw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            *_common_format_args = company->name;

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            common::repositionTabs(self);
        }

        // 0x00433DEB
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x00433DF5, regs);
        }

        // 0x00433FFE
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameCompanyPrompt(self, widgetIndex);
                    break;

                case common::widx::close_button:
                    WindowManager::close(self);
                    break;

                case common::widx::tab_status:
                case common::widx::tab_details:
                case common::widx::tab_colour_scheme:
                case common::widx::tab_finances:
                case common::widx::tab_cargo_delivered:
                case common::widx::tab_challenge:
                    common::switchTab(self, widgetIndex);
                    break;
            }
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
            self->set_size(windowSize);
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

    // 00434731
    window* openChallenge(company_id_t companyId)
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

        if (window == nullptr)
        {
            window = sub_4347D0(companyId);
        }

        // TODO(avgeffen): only needs to be called once.
        common::initEvents();

        window->current_tab = common::tab_challenge - common::tab_status;
        window->width = challenge::windowSize.width;
        window->height = challenge::windowSize.height;
        window->invalidate();

        window->widgets = challenge::widgets;
        window->enabled_widgets = challenge::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &challenge::events;
        window->activated_widgets = 0;

        common::disableChallengeTab(window);
        window->init_scroll_widgets();
        window->moveInsideScreenEdges();

        return window;
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t* enabledWidgets;
            const gfx::ui_size_t* windowSize;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { status::widgets, widx::tab_status, &status::events, &status::enabledWidgets, &status::windowSize },
            { details::widgets, widx::tab_details, &details::events, &details::enabledWidgets, &details::windowSize },
            { colour_scheme::widgets, widx::tab_colour_scheme, &colour_scheme::events, &colour_scheme::enabledWidgets, &colour_scheme::windowSize },
            { finances::widgets, widx::tab_finances, &finances::events, &finances::enabledWidgets, &finances::windowSize },
            { cargo_delivered::widgets, widx::tab_cargo_delivered, &cargo_delivered::events, &cargo_delivered::enabledWidgets, &cargo_delivered::windowSize },
            { challenge::widgets, widx::tab_challenge, &challenge::events, &challenge::enabledWidgets, &challenge::windowSize }
        };

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

        // 0x0043230B
        static void switchTab(window* self, widget_index widgetIndex)
        {
            if (input::is_tool_active(self->type, self->number))
                input::cancel_tool();

            textinput::sub_4CE6C9(self->type, self->number);

            self->current_tab = widgetIndex - widx::tab_status;
            self->frame_no = 0;
            self->flags &= ~(window_flags::flag_16);

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
            }

            auto tabIndex = widgetIndex - widx::tab_status;
            auto tabInfo = tabInformationByTabOffset[tabIndex];

            self->enabled_widgets = *tabInfo.enabledWidgets;
            self->holdable_widgets = 0;
            self->event_handlers = tabInfo.events;
            self->activated_widgets = 0;
            self->widgets = tabInfo.widgets;

            if (tabInfo.widgetIndex == widx::tab_finances)
                self->holdable_widgets = finances::holdableWidgets;

            common::disableChallengeTab(self);

            self->set_size(*tabInfo.windowSize);
            self->call_on_resize();
            self->call_prepare_draw();
            self->init_scroll_widgets();
            self->invalidate();
            self->moveInsideScreenEdges();

            if (tabInfo.widgetIndex == widx::tab_finances)
                finances::sub_4C8DBF(self);
        }

        // 0x0043252E
        static void renameCompanyPrompt(window* self, widget_index widgetIndex)
        {
            auto company = companymgr::get(self->number);
            _common_format_args[2] = company->name;
            textinput::open_textinput(self, string_ids::title_name_company, string_ids::prompt_enter_new_company_name, company->name, widgetIndex, &_common_format_args[2]);
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
