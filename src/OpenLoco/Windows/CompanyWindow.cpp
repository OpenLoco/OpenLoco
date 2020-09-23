#include "../Company.h"
#include "../CompanyManager.h"
#include "../Config.h"
#include "../Date.h"
#include "../GameCommands.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Management/Expenditures.h"
#include "../Objects/CargoObject.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Things/ThingManager.h"
#include "../Things/Vehicle.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::ui::windows::CompanyWindow
{
    static loco_global<string_id, 0x009C68E8> gGameCommandErrorTitle;

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

        constexpr uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_status) | (1 << widx::tab_details) | (1 << widx::tab_colour_scheme) | (1 << widx::tab_finances) | (1 << widx::tab_cargo_delivered) | (1 << widx::tab_challenge);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                            \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                              \
        makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                         \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),     \
        makeWidget({ 0, 41 }, { frameWidth, 120 }, widget_type::panel, 1),                                                                 \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_company_owner_and_status),          \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_company_headquarters_and_details), \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_company_colour_scheme),            \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_company_finances),                 \
        makeRemapWidget({ 127, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_cargo_delivered),                 \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab, StringIds::tooltip_company_challenge_for_this_game), \
        makeWidget({ 0, 14 }, { 26, 26 }, widget_type::wt_9, 0, ImageIds::null, StringIds::tooltip_select_company)

        // 0x004343FC
        static void disableChallengeTab(window* self)
        {
            self->disabled_widgets = 0;
            if (self->number != companymgr::getControllingId())
                self->disabled_widgets |= (1 << widx::tab_challenge);
        }

        // 0x00431E9B
        static void enableRenameByCaption(window* self)
        {
            if (isEditorMode() || self->number == companymgr::getControllingId())
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
        static void renameCompany(window* self, char* input);
        static void switchCompany(window* self, int16_t itemIndex);
        static void switchTab(window* self, widget_index widgetIndex);
        static void switchTabWidgets(window* self);
        static void drawCompanySelect(const window* const self, Gfx::drawpixelinfo_t* const dpi);
        static void drawTabs(window* self, Gfx::drawpixelinfo_t* dpi);
        static void repositionTabs(window* self);
    }

    namespace status
    {
        static const Gfx::ui_size_t windowSize = { 270, 182 };

        enum widx
        {
            unk_11 = 11,
            viewport,
            centre_on_viewport,
            face,
            change_owner_name,
        };

        static widget_t widgets[] = {
            commonWidgets(270, 182, StringIds::title_company),
            makeWidget({ 3, 160 }, { 242, 21 }, widget_type::wt_13, 1),
            makeWidget({ 3, 44 }, { 96, 120 }, widget_type::viewport, 1, -2),
            makeWidget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::null, StringIds::move_main_view_to_show_this),
            makeWidget({ 178, 57 }, { 66, 66 }, widget_type::wt_9, 1, ImageIds::null),
            makeWidget({ 154, 124 }, { 112, 22 }, widget_type::wt_9, 1, ImageIds::null, StringIds::tooltip_change_owner_name),
            widgetEnd(),
        };

        constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << common::widx::company_select) | (1 << widx::centre_on_viewport) | (1 << widx::face) | (1 << widx::change_owner_name);

        static window_event_list events;

        // 0x00431EBB
        static void prepareDraw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name in title.
            auto company = companymgr::get(self->number);
            FormatArguments args{};
            args.push(company->name);

            self->disabled_widgets &= ~((1 << widx::centre_on_viewport) | (1 << widx::face));

            // No centering on a viewport that doesn't exist.
            if (self->viewports[0] == nullptr)
                self->disabled_widgets |= (1 << widx::centre_on_viewport);

            // No changing other player's faces, unless we're editing a scenario.
            if (self->number != companymgr::getControllingId() && !isEditorMode())
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

            if (self->number == companymgr::getControllingId())
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
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            common::drawCompanySelect(self, dpi);
            const auto company = companymgr::get(self->number);
            const auto competitor = ObjectManager::get<competitor_object>(company->competitor_id);

            // Draw 'owner' label
            {
                auto& widget = self->widgets[widx::face];
                Gfx::drawStringCentred(
                    *dpi,
                    self->x + (widget.left + widget.right) / 2,
                    self->y + widget.top - 12,
                    Colour::black,
                    StringIds::window_owner,
                    nullptr);
            }

            // Draw company owner image.
            {
                const uint32_t image = Gfx::recolour(competitor->images[company->owner_emotion], company->mainColours.primary) + 1;
                const uint16_t x = self->x + self->widgets[widx::face].left + 1;
                const uint16_t y = self->y + self->widgets[widx::face].top + 1;
                Gfx::drawImage(dpi, x, y, image);
            }

            // If the owner's been naughty, draw some jail bars over them.
            if (company->jail_status != 0)
            {
                const uint32_t image = ImageIds::owner_jailed;
                const uint16_t x = self->x + self->widgets[widx::face].left + 1;
                const uint16_t y = self->y + self->widgets[widx::face].top + 1;
                Gfx::drawImage(dpi, x, y, image);
            }

            // Draw owner name
            {
                FormatArguments args{};
                args.push(company->owner_name);
                auto& widget = self->widgets[widx::change_owner_name];
                auto origin = Gfx::point_t(self->x + (widget.left + widget.right) / 2, self->y + widget.top + 5);
                Gfx::drawStringCentredWrapped(
                    dpi,
                    &origin,
                    widget.right - widget.left,
                    Colour::black,
                    StringIds::black_stringid,
                    &args);
            }

            // Draw owner status
            {
                // TODO: df fix this
                // Until format arguments can allow pushing to the front we will have to call twice once for the status
                FormatArguments args{};
                string_id status = companymgr::getOwnerStatus(self->number, args);
                args = FormatArguments{};
                args.push(status);
                // and once for the args
                companymgr::getOwnerStatus(self->number, args);

                auto& widget = self->widgets[widx::unk_11];
                Gfx::drawString_494BBF(
                    *dpi,
                    self->x + widget.left - 1,
                    self->y + widget.top - 1,
                    widget.right - widget.left,
                    Colour::black,
                    StringIds::black_stringid,
                    &args);
            }

            if (self->viewports[0] != nullptr)
            {
                self->drawViewports(dpi);
                widget::drawViewportCentreButton(dpi, self, (widget_index)widx::centre_on_viewport);
            }
        }

        // 0x00432244
        static void onMouseUp(window* self, widget_index widgetIndex)
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
                    auto main = WindowManager::getMainWindow();
                    if (self->saved_view.isThingView())
                    {
                        auto thing = thingmgr::get<Thing>(self->saved_view.thingId);
                        main->viewportCentreOnTile({ thing->x, thing->y, thing->z });
                    }
                    else
                    {
                        main->viewportCentreOnTile(self->saved_view.getPos());
                    }
                    break;
                }

                case widx::face:
                {
                    CompanyFaceSelection::open(self->number);
                    break;
                }

                case widx::change_owner_name:
                {
                    auto company = companymgr::get(self->number);
                    textinput::openTextinput(self, StringIds::title_name_owner, StringIds::prompt_enter_new_name_for_owner, company->owner_name, widgetIndex, nullptr);
                    break;
                }
            }
        }

        // 0x00432283
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::company_select)
                dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
        }

        // 0x0043228E
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == common::widx::company_select)
                common::switchCompany(self, itemIndex);
        }

        // 0x004325DF
        static void renameCompanyOwnerName(window* self, char* input)
        {
            if (strlen(input) == 0)
                return;

            gGameCommandErrorTitle = StringIds::cannot_change_owner_name;

            bool success = false;
            {
                uint32_t* buffer = (uint32_t*)input;
                game_commands::do_31(self->number, 1, buffer[0], buffer[1], buffer[2]);
                game_commands::do_31(0, 2, buffer[3], buffer[4], buffer[5]);
                success = game_commands::do_31(0, 0, buffer[6], buffer[7], buffer[8]);
            }

            // No need to propate the name if it could not be set.
            if (!success)
                return;

            // Only name company after owner if this is a new company.
            const auto& company = companymgr::get(self->number);
            if (company->name != StringIds::new_company)
                return;

            // Temporarily store the new name in buffer string 2039.
            // TODO: replace with a fixed length!
            char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));
            strcpy(buffer, input);

            FormatArguments args = {};
            args.push(StringIds::buffer_2039);

            // Add the ' Transport' suffix to the company name, and rename the company.
            StringManager::formatString(input, StringIds::company_owner_name_transport, const_cast<void*>(&args));
            common::renameCompany(self, input);
        }

        // 0x004322F6
        static void textInput(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget == common::widx::caption)
            {
                common::renameCompany(self, input);
            }
            else if (callingWidget == widx::change_owner_name)
            {
                renameCompanyOwnerName(self, input);
            }
        }

        // 0x0043270A
        static void onUpdate(window* self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self->number);
        }

        // 0x00432724
        static void onResize(window* self)
        {
            common::enableRenameByCaption(self);

            self->setSize(status::windowSize, Gfx::ui_size_t(640, 400));

            if (self->viewports[0] != nullptr)
            {
                Gfx::ui_size_t proposedDims(self->width - 123, self->height - 59);
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

            self->callViewportRotate();
        }

        static void sub_434336(window* self, const SavedView& view)
        {
            if (self->viewports[0] != nullptr)
            {
                return;
            }

            auto& widget = self->widgets[widx::viewport];
            auto origin = Gfx::point_t(widget.left + self->x + 1, widget.top + self->y + 1);
            auto size = Gfx::ui_size_t(widget.width() - 2, widget.height() - 2);
            if (view.isThingView())
            {
                viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, view.thingId);
            }
            else
            {
                viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, view.getPos());
            }
        }

        static void sub_434223(window* const self, const SavedView& view, const uint16_t vpFlags)
        {
            self->saved_view = view;
            sub_434336(self, view);
            self->viewports[0]->flags |= vpFlags;
            self->invalidate();
        }

        static void differentViewportSettings(window* const self, const SavedView& view)
        {
            auto vpFlags = self->viewports[0]->flags;
            self->viewports[0]->width = 0;
            self->viewports[0] = nullptr;
            viewportmgr::collectGarbage();
            sub_434223(self, view, vpFlags);
        }

        static void noViewportPresent(window* const self, const SavedView& view)
        {
            uint16_t vpFlags = 0;
            if (config::get().flags & config::flags::gridlines_on_landscape)
            {
                vpFlags |= viewport_flags::gridlines_on_landscape;
            }
            sub_434223(self, view, vpFlags);
        }

        static void invalidViewport(window* const self)
        {
            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                self->invalidate();
            }
        }

        // 0x004327C8
        static void viewportRotate(window* self)
        {
            if (self->current_tab != 0)
            {
                return;
            }

            self->callPrepareDraw();

            const auto& company = companymgr::get(self->number);

            if (company->observation_thing == thing_id::null)
            {
                // Observing a certain location?
                if (company->observation_x != -1)
                {
                    auto tileZAndWater = OpenLoco::Map::tileElementHeight(company->observation_x, company->observation_y);
                    coord_t tileZ = tileZAndWater.landHeight;
                    coord_t waterZ = tileZAndWater.waterHeight;
                    if (waterZ != 0)
                    {
                        tileZ = waterZ;
                    }

                    // loc_43410A
                    int8_t rotation = static_cast<int8_t>(self->viewports[0]->getRotation());
                    SavedView view(
                        company->observation_x,
                        company->observation_y,
                        ZoomLevel::half,
                        rotation,
                        static_cast<int16_t>(tileZ + 16));
                    view.flags |= (1 << 14);

                    if (self->viewports[0] == nullptr)
                    {
                        noViewportPresent(self, view);
                        return;
                    }

                    if (self->saved_view.isThingView() || self->saved_view.rotation != view.rotation || self->saved_view.zoomLevel != view.zoomLevel)
                    {
                        if (self->saved_view != view)
                        {
                            differentViewportSettings(self, view);
                            return;
                        }
                        return;
                    }

                    self->saved_view = view;
                    self->viewportCentreOnTile(view.getPos());
                    return;
                }
                // Not observing anything at all?
                else
                {
                    invalidViewport(self);
                }
            }
            else
            {
                // loc_434170
                auto thing = thingmgr::get<OpenLoco::vehicle_head>(company->observation_thing);

                if (thing->base_type != thing_base_type::vehicle || thing->type != VehicleThingType::head || (thing->x == location::null))
                {
                    invalidViewport(self);
                    return;
                }

                things::vehicle::Vehicle train(thing);

                int8_t rotation = static_cast<int8_t>(self->viewports[0]->getRotation());
                SavedView view(
                    train.cars.firstCar.body->id,
                    0xC000,
                    ZoomLevel::full,
                    rotation,
                    0);

                if (self->viewports[0] == nullptr)
                {
                    noViewportPresent(self, view);
                    return;
                }

                if (self->saved_view != view)
                {
                    differentViewportSettings(self, view);
                    return;
                }
            }
        }

        static void initEvents()
        {
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.on_mouse_down = onMouseDown;
            events.on_dropdown = onDropdown;
            events.text_input = textInput;
            events.on_update = onUpdate;
            events.on_resize = onResize;
            events.viewport_rotate = viewportRotate;
        }
    }

    // 0x004347D0
    static window* create(company_id_t companyId)
    {
        const uint32_t newFlags = window_flags::flag_8 | window_flags::flag_11;
        auto window = WindowManager::createWindow(WindowType::company, status::windowSize, newFlags, &status::events);
        window->number = companyId;
        window->owner = companyId;
        window->current_tab = 0;
        window->frame_no = 0;
        window->saved_view.clear();

        auto skin = ObjectManager::get<interface_skin_object>();
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
            if (Input::isToolActive(window->type, window->number))
            {
                Input::toolCancel();
                window = WindowManager::bringToFront(WindowType::company, companyId);
            }
        }

        if (window == nullptr)
        {
            window = create(companyId);
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
        window->initScrollWidgets();
        window->moveInsideScreenEdges();

        return window;
    }

    // 0x00435ACC
    window* openAndSetName()
    {
        company_id_t companyId = companymgr::getControllingId();
        window* self = open(companyId);

        // Allow setting company owner name if no preferred owner name has been set.
        if ((config::get().flags & config::flags::use_preferred_owner_name) == 0)
            status::onMouseUp(self, status::widx::change_owner_name);

        return self;
    }

    namespace details
    {
        const Gfx::ui_size_t windowSize = { 340, 194 };

        loco_global<coord_t, 0x009C68D6> _9C68D6; // likely tool x,y,z
        loco_global<coord_t, 0x009C68D8> _9C68D8;
        loco_global<coord_t, 0x009C68DA> _9C68DA;
        loco_global<uint8_t, 0x009C68EF> _9C68EF;

        enum widx
        {
            viewport = 11,
            build_hq,
            centre_on_viewport,
        };

        static widget_t widgets[] = {
            commonWidgets(340, 194, StringIds::title_company_details),
            makeWidget({ 219, 54 }, { 96, 120 }, widget_type::viewport, 1, -2),
            makeWidget({ 315, 92 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::null, StringIds::tooltip_build_or_move_headquarters),
            makeWidget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::null, StringIds::move_main_view_to_show_this),
            widgetEnd(),
        };

        constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << common::widx::company_select) | (1 << build_hq) | (1 << centre_on_viewport);

        static window_event_list events;

        // 0x004327CF
        static void prepareDraw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            FormatArguments args{};
            args.push(company->name);
            auto companyColour = companymgr::getCompanyColour(self->number);
            auto skin = ObjectManager::get<interface_skin_object>();
            uint32_t image = skin->img + InterfaceSkin::ImageIds::build_headquarters;
            self->widgets[widx::build_hq].image = Gfx::recolour(image, companyColour) | (1 << 30);

            self->disabled_widgets &= ~(1 << widx::centre_on_viewport);
            if (company->headquarters_x == -1)
            {
                self->disabled_widgets |= (1 << widx::centre_on_viewport);
            }

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[widx::viewport].right = self->width - 26;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[common::widx::company_select].right = self->width - 3;
            self->widgets[common::widx::company_select].left = self->width - 28;

            if (self->number == companymgr::getControllingId())
                self->widgets[widx::build_hq].type = widget_type::wt_9;
            else
                self->widgets[widx::build_hq].type = widget_type::none;

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            common::repositionTabs(self);
        }

        static std::array<string_id, 10> aiRatingToLevelArray = {
            {
                StringIds::low,
                StringIds::low,
                StringIds::low,
                StringIds::low,
                StringIds::medium,
                StringIds::medium,
                StringIds::medium,
                StringIds::high,
                StringIds::high,
                StringIds::high,
            }
        };

        constexpr string_id aiRatingToLevel(const uint8_t rating)
        {
            return aiRatingToLevelArray[std::min(rating, static_cast<uint8_t>(aiRatingToLevelArray.size()))];
        }

        static void drawAIdetails(Gfx::drawpixelinfo_t& dpi, const int32_t x, int32_t& y, const OpenLoco::company& company)
        {
            const auto competitor = ObjectManager::get<competitor_object>(company.competitor_id);
            {
                FormatArguments args{};
                args.push<uint16_t>(competitor->intelligence);
                args.push(aiRatingToLevel(competitor->intelligence));
                Gfx::drawString_494B3F(dpi, x, y, Colour::black, StringIds::company_details_intelligence, &args);
                y += 10;
            }
            {
                FormatArguments args{};
                args.push<uint16_t>(competitor->agressiveness);
                args.push(aiRatingToLevel(competitor->agressiveness));
                Gfx::drawString_494B3F(dpi, x, y, Colour::black, StringIds::company_details_aggressiveness, &args);
                y += 10;
            }
            {
                FormatArguments args{};
                args.push<uint16_t>(competitor->competitiveness);
                args.push(aiRatingToLevel(competitor->competitiveness));
                Gfx::drawString_494B3F(dpi, x, y, Colour::black, StringIds::company_details_competitiveness, &args);
                y += 10;
            }
        }

        static std::array<string_id, 6> transportTypeCountString = {
            {
                StringIds::company_details_trains_count,
                StringIds::company_details_buses_count,
                StringIds::company_details_trucks_count,
                StringIds::company_details_trams_count,
                StringIds::company_details_aircraft_count,
                StringIds::company_details_ships_count,
            }
        };

        // 0x00432919
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            common::drawCompanySelect(self, dpi);

            auto company = companymgr::get(self->number);
            auto x = self->x + 3;
            auto y = self->y + 48;
            {
                FormatArguments args{};
                args.push(company->startedDate);
                Gfx::drawString_494B3F(*dpi, x, y, Colour::black, StringIds::company_details_started, &args);
                y += 10;
            }

            {
                FormatArguments args{};
                formatPerformanceIndex(company->performance_index, args);

                string_id formatId = StringIds::company_details_performance;
                if (company->challenge_flags & company_flags::decreased_performance)
                {
                    formatId = StringIds::company_details_performance_decreasing;
                }
                else if (company->challenge_flags & company_flags::increased_performance)
                {
                    formatId = StringIds::company_details_performance_increasing;
                }
                Gfx::drawString_494B3F(*dpi, x, y, Colour::black, formatId, &args);
                y += 25;
            }

            {
                FormatArguments args{};
                args.push(company->owner_name);
                Gfx::drawString_494BBF(*dpi, x, y, 213, Colour::black, StringIds::owner_label, &args);
                y += 10;
            }

            if (!isPlayerCompany(self->number))
            {
                drawAIdetails(*dpi, x + 5, y, *company);
            }
            y += 5;

            {
                for (auto i = 0; i < 6; ++i)
                {
                    auto count = company->transportTypeCount[i];
                    if (count != 0)
                    {
                        FormatArguments args{};
                        args.push(count);
                        Gfx::drawString_494B3F(*dpi, x, y, Colour::black, transportTypeCountString[i], &args);
                        y += 10;
                    }
                }
            }

            {
                x = self->x + (self->widgets[widx::viewport].left + self->widgets[widx::viewport].right) / 2;
                y = self->y + self->widgets[widx::viewport].top - 12;
                Gfx::drawStringCentred(*dpi, x, y, Colour::black, StringIds::headquarters);
            }

            if (company->headquarters_x == -1)
            {
                auto width = self->widgets[widx::viewport].width();
                Gfx::point_t loc = {
                    static_cast<int16_t>(self->x + self->widgets[widx::viewport].left + width / 2),
                    static_cast<int16_t>(self->y + self->widgets[widx::viewport].top + self->widgets[widx::viewport].height() / 2 - 5)
                };
                width -= 2;
                Gfx::drawStringCentredWrapped(dpi, &loc, width, Colour::black, StringIds::not_yet_constructed);
            }

            if (self->viewports[0] != nullptr)
            {
                self->drawViewports(dpi);
                widget::drawViewportCentreButton(dpi, self, widx::centre_on_viewport);
            }
        }

        // 0x00432BDD
        static void onMouseUp(window* self, widget_index widgetIndex)
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

                    // Centre viewport on HQ.
                    // TODO(avgeffen): move/implement.
                    registers regs;
                    regs.esi = (int32_t)self;
                    call(0x00432C45, regs);
                    break;
                }
            }
        }

        // 0x00432C08
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::company_select:
                    dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
                    break;

                case widx::build_hq:
                    Input::toolSet(self, widgetIndex, 43);
                    Input::setFlag(Input::input_flags::flag5);
                    break;
            }
        }

        // 0x00432C19
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == common::widx::company_select)
                common::switchCompany(self, itemIndex);
        }

        // 0x00432C24
        static void textInput(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget == common::widx::caption)
            {
                common::renameCompany(self, input);
            }
        }

        static void sub_434E94()
        {
            if (_9C68EF & (1 << 0))
            {
                _9C68EF = _9C68EF & ~(1 << 0);
                auto flags = game_commands::GameCommandFlag::apply | game_commands::GameCommandFlag::flag_3 | game_commands::GameCommandFlag::flag_5 | game_commands::GameCommandFlag::flag_6;
                game_commands::do_55(flags, _9C68D6, _9C68D8, _9C68DA);
            }
        }

        // 0x00468FFE hide_gridlines
        static void sub_468FFE()
        {
            registers regs;
            call(0x00468FFE, regs);
        }

        // 0x00434EC7
        // input:
        // regs.ax = mouseX;
        // regs.bx = mouseY;
        // output (not verified):
        // regs.cx = tileX (tile coordinate)
        // regs.ax = tileY (tile coordinate)
        // regs.di = tileZ (height)
        // regs.dx = dx - company index (value 1 in testing case)
        static void sub_434EC7(const int16_t mouseX, const int16_t mouseY, coord_t& tileX, coord_t& tileY, coord_t& tileZ, int16_t& dx)
        {
            registers regs;
            regs.ax = mouseX;
            regs.bx = mouseY;

            call(0x00434EC7, regs);
            tileX = regs.cx;
            tileY = regs.ax;
            tileZ = regs.di;
            dx = regs.dx;
        }

        // 0x00432CA1
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x00432CA1, regs);
        }

        // 0x00432D45
        // regs.esi = window* w;
        // regs.dx = widgetIndex;
        // regs.ax = mouseX;
        // regs.bx = mouseY;
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t mouseX, const int16_t mouseY)
        {
            sub_434E94();

            coord_t tileX, tileY, tileZ;
            int16_t dx;
            sub_434EC7(mouseX, mouseY, tileX, tileY, tileZ, dx);
            if (tileY == location::null)
            {
                return;
            }

            gGameCommandErrorTitle = StringIds::error_cant_build_this_here;
            uint8_t flags = game_commands::GameCommandFlag::apply | game_commands::GameCommandFlag::flag_1;
            auto commandResult = game_commands::do_54(flags, tileY, tileX, tileZ, dx);
            if (commandResult != game_commands::FAILURE)
            {
                Input::toolCancel();
            }
        }

        // 0x00432D7A
        static void onToolAbort(window& self, const widget_index widgetIndex)
        {
            sub_434E94();
            sub_468FFE();
        }

        // 0x0432D85
        static void onUpdate(window* self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self->number);
        }

        // 0x00432D9F
        static void onResize(window* self)
        {
            common::enableRenameByCaption(self);
            self->setSize(windowSize);
            self->callViewportRotate();
        }

        static void sub_434377(window* self, const SavedView& view)
        {
            if (self->viewports[0] != nullptr)
            {
                return;
            }

            auto& widget = self->widgets[widx::viewport];
            auto origin = Gfx::point_t(widget.left + self->x + 1, widget.top + self->y + 1);
            auto size = Gfx::ui_size_t(widget.width() - 2, widget.height() - 2);

            viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, view.getPos());
            self->flags |= window_flags::viewport_no_scrolling;
            self->invalidate();
        }

        // 0x00432E08
        static void viewportRotate(window* self)
        {
            if (self->current_tab != common::tab_details - common::tab_status)
                return;

            self->callPrepareDraw();
            auto company = companymgr::get(self->number);
            if (company->headquarters_x == -1)
            {
                // If headquarters not placed destroy the viewport
                if (self->viewports[0] == nullptr)
                {
                    return;
                }

                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                self->invalidate();
                return;
            }
            int8_t rotation = static_cast<int8_t>(self->viewports[0]->getRotation());
            OpenLoco::Map::map_pos3 loc = {
                static_cast<coord_t>(company->headquarters_x + 32),
                static_cast<coord_t>(company->headquarters_y + 32),
                static_cast<coord_t>((company->headquarters_z + 8) * 4)
            };
            SavedView view{
                loc.x,
                loc.y,
                ZoomLevel::full,
                rotation,
                loc.z
            };
            view.flags |= (1 << 14);

            uint16_t vpFlags = 0;
            if (self->viewports[0] == nullptr)
            {
                if (config::get().flags & config::flags::gridlines_on_landscape)
                {
                    vpFlags |= viewport_flags::gridlines_on_landscape;
                }
            }
            else if (self->saved_view != view)
            {
                vpFlags = self->viewports[0]->flags;
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                viewportmgr::collectGarbage();
            }
            else
            {
                return;
            }

            self->saved_view = view;
            sub_434377(self, view);
            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->flags = vpFlags;
                self->invalidate();
            }
        }

        static void initEvents()
        {
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.on_mouse_down = onMouseDown;
            events.on_dropdown = onDropdown;
            events.text_input = textInput;
            events.on_tool_update = onToolUpdate;
            events.on_tool_down = onToolDown;
            events.on_tool_abort = onToolAbort;
            events.on_update = onUpdate;
            events.on_resize = onResize;
            events.viewport_rotate = viewportRotate;
        }
    }

    namespace colour_scheme
    {
        const Gfx::ui_size_t windowSize = { 265, 252 };

        enum widx
        {
            check_steam_locomotives = 11,
            check_diesel_locomotives,
            check_electric_locomotives,
            check_multiple_units,
            check_passenger_vehicles,
            check_freight_vehicles,
            check_buses,
            check_trucks,
            check_aircraft,
            check_ships,
            main_colour_scheme,
            main_colour_steam_locomotives,
            main_colour_diesel_locomotives,
            main_colour_electric_locomotives,
            main_colour_multiple_units,
            main_colour_passenger_vehicles,
            main_colour_freight_vehicles,
            main_colour_buses,
            main_colour_trucks,
            main_colour_aircraft,
            main_colour_ships,
            secondary_colour_scheme,
            secondary_colour_steam_locomotives,
            secondary_colour_diesel_locomotives,
            secondary_colour_electric_locomotives,
            secondary_colour_multiple_units,
            secondary_colour_passenger_vehicles,
            secondary_colour_freight_vehicles,
            secondary_colour_buses,
            secondary_colour_trucks,
            secondary_colour_aircraft,
            secondary_colour_ships,
        };

        // clang-format off
        constexpr uint64_t allMainColours = {
            (1ULL << widx::main_colour_scheme) |
            (1ULL << widx::main_colour_steam_locomotives) |
            (1ULL << widx::main_colour_diesel_locomotives) |
            (1ULL << widx::main_colour_electric_locomotives) |
            (1ULL << widx::main_colour_multiple_units) |
            (1ULL << widx::main_colour_passenger_vehicles) |
            (1ULL << widx::main_colour_freight_vehicles) |
            (1ULL << widx::main_colour_buses) |
            (1ULL << widx::main_colour_trucks) |
            (1ULL << widx::main_colour_aircraft) |
            (1ULL << widx::main_colour_ships)
        };

        constexpr uint64_t allSecondaryColours = {
            (1ULL << widx::secondary_colour_scheme) |
            (1ULL << widx::secondary_colour_steam_locomotives) |
            (1ULL << widx::secondary_colour_diesel_locomotives) |
            (1ULL << widx::secondary_colour_electric_locomotives) |
            (1ULL << widx::secondary_colour_multiple_units) |
            (1ULL << widx::secondary_colour_passenger_vehicles) |
            (1ULL << widx::secondary_colour_freight_vehicles) |
            (1ULL << widx::secondary_colour_buses) |
            (1ULL << widx::secondary_colour_trucks) |
            (1ULL << widx::secondary_colour_aircraft) |
            (1ULL << widx::secondary_colour_ships)
        };

        constexpr uint64_t allColourChecks = {
            (1ULL << widx::check_steam_locomotives) |
            (1ULL << widx::check_diesel_locomotives) |
            (1ULL << widx::check_electric_locomotives) |
            (1ULL << widx::check_multiple_units) |
            (1ULL << widx::check_passenger_vehicles) |
            (1ULL << widx::check_freight_vehicles) |
            (1ULL << widx::check_buses) |
            (1ULL << widx::check_trucks) |
            (1ULL << widx::check_aircraft) |
            (1ULL << widx::check_ships)
        };
        // clang-format on

        static widget_t widgets[] = {
            commonWidgets(265, 252, StringIds::title_company_colour_scheme),
            makeWidget({ 15, 81 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_steam_locomotives, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 98 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_diesel_locomotives, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 115 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_electric_locomotives, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 132 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_multiple_units, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 149 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_passenger_vehicles, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 166 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_freight_vehicles, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 183 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_buses, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 200 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_trucks, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 217 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_aircraft, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 15, 234 }, { 204, 12 }, widget_type::checkbox, 1, StringIds::colour_ships, StringIds::tooltip_toggle_vehicle_colour_scheme),
            makeWidget({ 221, 48 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 78 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 95 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 112 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 129 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 146 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 163 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 180 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 197 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 214 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 221, 231 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_main_colour),
            makeWidget({ 239, 48 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 78 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 95 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 112 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 129 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 146 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 163 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 180 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 197 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 214 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            makeWidget({ 239, 231 }, { 16, 16 }, widget_type::wt_10, 1, ImageIds::null, StringIds::tooltip_select_secondary_colour),
            widgetEnd(),
        };

        constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << common::widx::company_select) | allMainColours | allSecondaryColours | allColourChecks;

        static window_event_list events;

        // 0x00432E0F
        static void prepareDraw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            FormatArguments args{};
            args.push(company->name);

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::company_select].right = self->width - 3;
            self->widgets[common::widx::company_select].left = self->width - 28;

            common::repositionTabs(self);

            // Set company's main colour
            self->widgets[widx::main_colour_scheme].image = (1 << 30) | Gfx::recolour(ImageIds::colour_swatch_recolourable, company->mainColours.primary);

            // Set company's secondary colour
            self->widgets[widx::secondary_colour_scheme].image = (1 << 30) | Gfx::recolour(ImageIds::colour_swatch_recolourable, company->mainColours.secondary);

            struct ColourSchemeTuple
            {
                widget_index checkbox;
                widget_index primary;
                widget_index secondary;
            };

            // clang-format off
            static const ColourSchemeTuple tuples[] =
            {
                { widx::check_steam_locomotives,     widx::main_colour_steam_locomotives,     widx::secondary_colour_steam_locomotives },
                { widx::check_diesel_locomotives,    widx::main_colour_diesel_locomotives,    widx::secondary_colour_diesel_locomotives },
                { widx::check_electric_locomotives,  widx::main_colour_electric_locomotives,  widx::secondary_colour_electric_locomotives },
                { widx::check_multiple_units,        widx::main_colour_multiple_units,        widx::secondary_colour_multiple_units },
                { widx::check_passenger_vehicles,    widx::main_colour_passenger_vehicles,    widx::secondary_colour_passenger_vehicles },
                { widx::check_freight_vehicles,      widx::main_colour_freight_vehicles,      widx::secondary_colour_freight_vehicles },
                { widx::check_buses,                 widx::main_colour_buses,                 widx::secondary_colour_buses },
                { widx::check_trucks,                widx::main_colour_trucks,                widx::secondary_colour_trucks },
                { widx::check_aircraft,              widx::main_colour_aircraft,              widx::secondary_colour_aircraft },
                { widx::check_ships,                 widx::main_colour_ships,                 widx::secondary_colour_ships },
            };
            // clang-format on

            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(tuples)); i++)
            {
                // customVehicleColoursSet appears to reserve its first bit for something else, so skip it.
                if ((company->customVehicleColoursSet & (1 << (i + 1))) != 0)
                {
                    self->activated_widgets |= (1ULL << tuples[i].checkbox);

                    self->widgets[tuples[i].primary].image = (1ULL << 30) | Gfx::recolour(ImageIds::colour_swatch_recolourable, company->vehicleColours[i].primary);
                    self->widgets[tuples[i].secondary].image = (1ULL << 30) | Gfx::recolour(ImageIds::colour_swatch_recolourable, company->vehicleColours[i].secondary);

                    self->widgets[tuples[i].primary].type = widget_type::wt_10;
                    self->widgets[tuples[i].secondary].type = widget_type::wt_10;
                }
                else
                {
                    self->activated_widgets &= ~(1ULL << tuples[i].checkbox);

                    self->widgets[tuples[i].primary].type = widget_type::none;
                    self->widgets[tuples[i].secondary].type = widget_type::none;
                }
            }

            if (self->number == companymgr::getControllingId())
                self->enabled_widgets |= allColourChecks | allMainColours | allSecondaryColours;
            else
                self->enabled_widgets &= ~(allColourChecks | allMainColours | allSecondaryColours);
        }

        // 0x00432F9A
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            common::drawCompanySelect(self, dpi);

            const auto& widget = self->widgets[widx::main_colour_scheme];
            const uint16_t x = self->x + 6;
            uint16_t y = self->y + widget.top + 3;

            // 'Main colour scheme'
            Gfx::drawString_494B3F(
                *dpi,
                x,
                y,
                Colour::black,
                StringIds::main_colour_scheme);

            // 'Special colour schemes used for'
            y += 17;
            Gfx::drawString_494B3F(
                *dpi,
                x,
                y,
                Colour::black,
                StringIds::special_colour_schemes_used_for);
        }

        // 0x00433032
        static void onMouseUp(window* self, widget_index widgetIndex)
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

                case widx::check_steam_locomotives:
                case widx::check_diesel_locomotives:
                case widx::check_electric_locomotives:
                case widx::check_multiple_units:
                case widx::check_passenger_vehicles:
                case widx::check_freight_vehicles:
                case widx::check_buses:
                case widx::check_trucks:
                case widx::check_aircraft:
                case widx::check_ships:
                    // customVehicleColoursSet reserves first bit for main colour scheme even though it can't be changed, so skip it.
                    const auto vehicleType = widgetIndex - widx::check_steam_locomotives + 1;
                    const auto company = companymgr::get(self->number);
                    const auto newMode = (company->customVehicleColoursSet & (1 << vehicleType)) == 0 ? 1 : 0;

                    gGameCommandErrorTitle = StringIds::error_cant_change_colour_scheme;

                    game_commands::do_19(0, newMode, vehicleType, 1, self->number);

                    break;
            }
        }

        // 0x00433067
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::company_select:
                    dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
                    break;

                case main_colour_scheme:
                case main_colour_steam_locomotives:
                case main_colour_diesel_locomotives:
                case main_colour_electric_locomotives:
                case main_colour_multiple_units:
                case main_colour_passenger_vehicles:
                case main_colour_freight_vehicles:
                case main_colour_buses:
                case main_colour_trucks:
                case main_colour_aircraft:
                case main_colour_ships:
                {
                    registers regs;
                    regs.edx = widgetIndex;
                    regs.esi = (int32_t)self;
                    regs.edi = (int32_t)&self->widgets[widgetIndex];
                    call(0x00433119, regs);
                    break;
                }

                case secondary_colour_scheme:
                case secondary_colour_steam_locomotives:
                case secondary_colour_diesel_locomotives:
                case secondary_colour_electric_locomotives:
                case secondary_colour_multiple_units:
                case secondary_colour_passenger_vehicles:
                case secondary_colour_freight_vehicles:
                case secondary_colour_buses:
                case secondary_colour_trucks:
                case secondary_colour_aircraft:
                case secondary_colour_ships:
                {
                    registers regs;
                    regs.edx = widgetIndex;
                    regs.esi = (int32_t)self;
                    regs.edi = (int32_t)&self->widgets[widgetIndex];
                    call(0x00433183, regs);
                    break;
                }
            }
        }

        // 0x00433092
        static void textInput(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget == common::widx::caption)
            {
                common::renameCompany(self, input);
            }
        }

        // 0x0043309D
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::company_select:
                    common::switchCompany(self, itemIndex);
                    break;

                case widx::main_colour_scheme:
                case widx::main_colour_steam_locomotives:
                case widx::main_colour_diesel_locomotives:
                case widx::main_colour_electric_locomotives:
                case widx::main_colour_multiple_units:
                case widx::main_colour_passenger_vehicles:
                case widx::main_colour_freight_vehicles:
                case widx::main_colour_buses:
                case widx::main_colour_trucks:
                case widx::main_colour_aircraft:
                case widx::main_colour_ships:
                {
                    if (itemIndex == -1)
                        return;

                    gGameCommandErrorTitle = StringIds::error_cant_change_colour_scheme;

                    const int8_t colour = dropdown::getItemArgument(itemIndex, 2);
                    const auto vehicleType = widgetIndex - widx::main_colour_scheme;

                    game_commands::do_19(0, colour, vehicleType, 0, self->number);
                    break;
                }

                case widx::secondary_colour_scheme:
                case widx::secondary_colour_steam_locomotives:
                case widx::secondary_colour_diesel_locomotives:
                case widx::secondary_colour_electric_locomotives:
                case widx::secondary_colour_multiple_units:
                case widx::secondary_colour_passenger_vehicles:
                case widx::secondary_colour_freight_vehicles:
                case widx::secondary_colour_buses:
                case widx::secondary_colour_trucks:
                case widx::secondary_colour_aircraft:
                case widx::secondary_colour_ships:
                {
                    if (itemIndex == -1)
                        return;

                    gGameCommandErrorTitle = StringIds::error_cant_change_colour_scheme;

                    const int8_t colour = dropdown::getItemArgument(itemIndex, 2);
                    const auto vehicleType = widgetIndex - widx::secondary_colour_scheme;

                    game_commands::do_19(1, colour, vehicleType, 0, self->number);
                    break;
                }
            }
        }

        // 0x0043325F
        static void onUpdate(window* self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self->number);
        }

        // 0x00433279
        static void onResize(window* self)
        {
            common::enableRenameByCaption(self);
            self->setSize(windowSize);
        }

        static void initEvents()
        {
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.on_mouse_down = onMouseDown;
            events.text_input = textInput;
            events.on_dropdown = onDropdown;
            events.on_update = onUpdate;
            events.on_resize = onResize;
        }
    }

    namespace finances
    {
        const Gfx::ui_size_t windowSize = { 636, 319 };

        enum widx
        {
            scrollview = 11,
            current_loan,
            loan_decrease,
            loan_increase,
        };

        constexpr uint16_t expenditureColumnWidth = 128;

        static widget_t widgets[] = {
            commonWidgets(636, 319, StringIds::title_company_finances),
            makeWidget({ 133, 45 }, { 499, 215 }, widget_type::scrollview, 1, horizontal),
            makeStepperWidgets({ 87, 264 }, { 100, 12 }, widget_type::wt_17, 1, StringIds::company_current_loan_value),
            widgetEnd(),
        };

        constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << common::widx::company_select) | (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        const uint64_t holdableWidgets = (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        static window_event_list events;

        // 0x004332E4
        static void prepareDraw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            FormatArguments args{};
            args.push(company->name);
            args.push<uint32_t>(0);
            args.push<uint16_t>(0);
            // Used for the loan stepper current value at offset 4
            args.push(company->current_loan);

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::company_select].right = self->width - 3;
            self->widgets[common::widx::company_select].left = self->width - 28;

            if (self->number == companymgr::getControllingId())
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
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            common::drawCompanySelect(self, dpi);

            const auto company = companymgr::get(self->number);

            // Draw 'expenditure/income' label
            {
                Gfx::drawStringLeftUnderline(
                    *dpi,
                    self->x + 5,
                    self->y + 47,
                    Colour::black,
                    StringIds::expenditure_income,
                    nullptr);
            }

            const string_id ExpenditureLabels[] = {
                StringIds::train_income,
                StringIds::train_running_costs,
                StringIds::bus_income,
                StringIds::bus_running_costs,
                StringIds::truck_income,
                StringIds::truck_running_costs,
                StringIds::tram_income,
                StringIds::tram_running_costs,
                StringIds::aircraft_income,
                StringIds::aircraft_running_costs,
                StringIds::ship_income,
                StringIds::ship_running_costs,
                StringIds::construction,
                StringIds::vehicle_purchases,
                StringIds::vehicle_disposals,
                StringIds::loan_interest,
                StringIds::miscellaneous,
            };

            uint16_t y = self->y + 62;
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(ExpenditureLabels)); i++)
            {
                // Add zebra stripes to even labels.
                if (i % 2 == 0)
                {
                    auto colour = Colour::getShade(self->colours[1], 6) | 0x1000000;
                    Gfx::fillRect(dpi, self->x + 4, y, self->x + 129, y + 9, colour);
                }

                FormatArguments args{};
                args.push(ExpenditureLabels[i]);
                Gfx::drawString_494B3F(
                    *dpi,
                    self->x + 5,
                    y - 1,
                    Colour::black,
                    StringIds::wcolour2_stringid,
                    &args);

                y += 10;
            }

            // 'Current loan' label
            {
                Gfx::drawString_494B3F(
                    *dpi,
                    self->x + 7,
                    self->y + self->widgets[widx::current_loan].top,
                    Colour::black,
                    StringIds::company_current_loan);
            }

            // '@ X% interest per' label
            {
                loco_global<uint8_t, 0x00525FC6> loanInterestRate;
                FormatArguments args{};
                args.push<uint16_t>(loanInterestRate);
                Gfx::drawString_494B3F(
                    *dpi,
                    self->x + self->widgets[widx::current_loan].right + 3,
                    self->y + self->widgets[widx::current_loan].top + 1,
                    Colour::black,
                    StringIds::interest_per_year,
                    &args);
            }

            // 'Cash' label with value
            {
                // Set cash value in format args.
                FormatArguments args{};
                args.push(company->cash);

                string_id cash_format = StringIds::cash_positive;
                if ((company->challenge_flags & company_flags::bankrupt) != 0)
                    cash_format = StringIds::cash_bankrupt;
                if (company->cash.var_04 < 0)
                    cash_format = StringIds::cash_negative;

                Gfx::drawString_494B3F(
                    *dpi,
                    self->x + 7,
                    self->y + self->widgets[widx::current_loan].top + 13,
                    Colour::black,
                    cash_format,
                    &args);
            }

            // 'Company value' label with value
            {
                // Set company value in format args.
                FormatArguments args{};
                args.push(company->companyValueHistory[0]);

                Gfx::drawString_494B3F(
                    *dpi,
                    self->x + 7,
                    self->y + self->widgets[widx::current_loan].top + 26,
                    Colour::black,
                    StringIds::company_value,
                    &args);
            }

            // 'Profit from vehicles' label with value
            {
                // Set company value in format args.
                FormatArguments args{};
                args.push(company->vehicleProfit);

                Gfx::drawString_494B3F(
                    *dpi,
                    self->x + 7,
                    self->y + self->widgets[widx::current_loan].top + 39,
                    Colour::black,
                    StringIds::profit_from_vehicles,
                    &args);
            }
        }

        static void drawFinanceYear(Gfx::drawpixelinfo_t* context, int16_t x, int16_t& y, uint16_t columnYear, uint16_t currentYear)
        {
            FormatArguments args = {};
            args.push(StringIds::uint16_raw);
            args.push(columnYear);

            string_id format = StringIds::wcolour2_stringid;
            if (columnYear != currentYear)
            {
                format = StringIds::black_stringid;
            }

            Gfx::drawStringUnderline(
                *context,
                x,
                y,
                Colour::black,
                format,
                &args);
            y += 14;
        }

        static currency48_t drawFinanceExpenditureColumn(Gfx::drawpixelinfo_t* context, const int16_t x, int16_t& y, uint8_t columnIndex, company& company)
        {
            currency48_t sum = 0;
            for (auto j = 0; j < ExpenditureType::Count; j++)
            {
                currency48_t expenditures = company.expenditures[columnIndex][j];
                sum += expenditures;

                string_id mainFormat = StringIds::black_stringid;
                string_id currFormat = StringIds::plus_currency48;
                if (expenditures < 0)
                {
                    mainFormat = StringIds::red_stringid;
                    currFormat = StringIds::currency48;
                }

                if (expenditures != 0)
                {

                    FormatArguments args = {};
                    args.push<string_id>(currFormat);
                    args.push<currency48_t>(expenditures);

                    Gfx::drawString_494C78(
                        *context,
                        x,
                        y,
                        Colour::black,
                        mainFormat,
                        &args);
                }
                y += 10;
            }
            return sum;
        }

        static void drawFinanceSum(Gfx::drawpixelinfo_t* context, int16_t x, int16_t& y, currency48_t sum)
        {
            FormatArguments args{};

            auto mainFormat = StringIds::black_stringid;
            auto sumFormat = StringIds::plus_currency48;
            if (sum < 0)
            {
                mainFormat = StringIds::red_stringid;
                sumFormat = StringIds::currency48;
            }
            args.push(sumFormat);
            args.push(sum);

            y += 4;

            Gfx::drawString_494C78(*context, x, y, Colour::black, mainFormat, &args);

            Gfx::fillRect(context, x - expenditureColumnWidth + 10, y - 2, x, y - 2, Colour::aquamarine);
        }

        // 0x0043361E
        static void drawScroll(window* self, Gfx::drawpixelinfo_t* context, uint32_t scrollIndex)
        {
            int16_t y = 47 - self->widgets[widx::scrollview].top + 14;

            for (uint8_t i = 0; i < static_cast<uint8_t>(ExpenditureType::Count); i++)
            {
                // Add zebra stripes to even labels.
                if (i % 2 == 0)
                {
                    auto colour = Colour::getShade(self->colours[1], 6) | 0x1000000;
                    Gfx::fillRect(context, 0, y, expenditureColumnWidth * 17, y + 9, colour);
                }

                y += 10;
            }

            const auto company = companymgr::get(self->number);

            uint32_t curYear = getCurrentYear();
            uint8_t expenditureYears = std::min<uint8_t>(company->numExpenditureMonths, expenditureHistoryCapacity);

            // Paint years on top of scroll area.
            int16_t x = 132 - self->widgets[widx::scrollview].left + expenditureColumnWidth;
            for (auto i = 0; i < expenditureYears; i++)
            {
                y = 46 - self->widgets[widx::scrollview].top;

                uint16_t columnYear = curYear - (expenditureYears - i) + 1;
                uint8_t columnIndex = expenditureYears - i - 1;
                drawFinanceYear(context, x, y, columnYear, curYear);
                auto sum = drawFinanceExpenditureColumn(context, x, y, columnIndex, *company);
                drawFinanceSum(context, x, y, sum);

                x += expenditureColumnWidth;
            }
        }

        // 0x00433819
        static void onMouseUp(window* self, widget_index widgetIndex)
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
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            static loco_global<uint16_t, 0x00523376> _clickRepeatTicks;

            switch (widgetIndex)
            {
                case common::widx::company_select:
                    dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
                    break;

                case widx::loan_decrease:
                {
                    auto company = companymgr::get(self->number);
                    if (company->current_loan == 0)
                        return;

                    currency32_t stepSize{};
                    if (*_clickRepeatTicks < 100)
                        stepSize = 1000;
                    else if (*_clickRepeatTicks >= 100)
                        stepSize = 10000;
                    else if (*_clickRepeatTicks >= 200)
                        stepSize = 100000;

                    auto newLoan = std::max<currency32_t>(0, company->current_loan - stepSize);

                    gGameCommandErrorTitle = StringIds::cant_pay_back_loan;
                    game_commands::do_9(newLoan);
                    break;
                }

                case widx::loan_increase:
                {
                    currency32_t stepSize{};
                    if (*_clickRepeatTicks < 100)
                        stepSize = 1000;
                    else if (*_clickRepeatTicks >= 100)
                        stepSize = 10000;
                    else if (*_clickRepeatTicks >= 200)
                        stepSize = 100000;

                    currency32_t newLoan = companymgr::get(self->number)->current_loan + stepSize;
                    gGameCommandErrorTitle = StringIds::cant_borrow_any_more_money;
                    game_commands::do_9(newLoan);
                    break;
                }
            }
        }

        // 0x0043385D
        static void textInput(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget == common::widx::caption)
            {
                common::renameCompany(self, input);
            }
        }

        static void sub_4C8DBF(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x004C8DBF, regs);
        }

        // 0x00433868
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == common::widx::company_select)
            {
                common::switchCompany(self, itemIndex);
                sub_4C8DBF(self);
                self->invalidate();
            }
        }

        // 0x0043386F
        static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            const auto& company = companymgr::get(self->number);
            *scrollWidth = company->numExpenditureMonths * expenditureColumnWidth;
        }

        // 0x00433887
        static void tooltip(FormatArguments& args, ui::window* window, widget_index widgetIndex)
        {
            args.push(StringIds::tooltip_scroll_list);
        }

        // 0x0043399D
        static void onUpdate(window* self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self->number);
        }

        // 0x004339B7
        static void onResize(window* self)
        {
            common::enableRenameByCaption(self);
            self->setSize(windowSize);
        }

        static void initEvents()
        {
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.draw_scroll = drawScroll;
            events.on_mouse_up = onMouseUp;
            events.on_mouse_down = onMouseDown;
            events.text_input = textInput;
            events.on_dropdown = onDropdown;
            events.get_scroll_size = getScrollSize;
            events.tooltip = tooltip;
            events.on_update = onUpdate;
            events.on_resize = onResize;
        }
    }

    // 0x004345EE
    window* openFinances(company_id_t companyId)
    {
        auto window = WindowManager::bringToFront(WindowType::company, companyId);
        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
            {
                Input::toolCancel();
                window = WindowManager::bringToFront(WindowType::company, companyId);
            }
        }

        if (window == nullptr)
        {
            window = create(companyId);
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
        window->initScrollWidgets();
        window->moveInsideScreenEdges();
        finances::sub_4C8DBF(window);

        return window;
    }

    namespace CargoDelivered
    {
        const Gfx::ui_size_t windowSize = { 240, 382 };

        static widget_t widgets[] = {
            commonWidgets(240, 382, StringIds::title_company_cargo_delivered),
            widgetEnd(),
        };

        constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << common::widx::company_select);

        static window_event_list events;

        // 0x00433A22
        static void prepareDraw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            FormatArguments args{};
            args.push(company->name);

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
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            uint16_t y = self->y + 47;

            // 'Cargo delivered'
            Gfx::drawString_494B3F(
                *dpi,
                self->x + 5,
                y,
                Colour::black,
                StringIds::cargo_delivered);

            y += 10;

            uint8_t numPrinted = 0;
            const auto company = companymgr::get(self->number);
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(company->cargoDelivered)); i++)
            {
                auto cargo = ObjectManager::get<cargo_object>(i);
                if (cargo == nullptr || company->cargoDelivered[i] == 0)
                    continue;

                FormatArguments args{};
                if (company->cargoDelivered[i] == 1)
                    args.push(cargo->unit_name_singular);
                else
                    args.push(cargo->unit_name_plural);

                args.push(company->cargoDelivered[i]);

                Gfx::drawString_494B3F(
                    *dpi,
                    self->x + 10,
                    y,
                    Colour::black,
                    StringIds::black_stringid,
                    &args);

                numPrinted++;
                y += 10;
            }

            // No cargo delivered yet?
            if (numPrinted == 0)
            {
                Gfx::drawString_494B3F(
                    *dpi,
                    self->x + 10,
                    y,
                    Colour::black,
                    StringIds::cargo_delivered_none);
            }
        }

        // 0x00433BE6
        static void onMouseUp(window* self, widget_index widgetIndex)
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
        static void onMouseDown(window* self, widget_index widgetIndex)
        {
            if (widgetIndex == common::widx::company_select)
                dropdown::populateCompanySelect(self, &self->widgets[widgetIndex]);
        }

        // 0x00433C16
        static void textInput(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget == common::widx::caption)
            {
                common::renameCompany(self, input);
            }
        }

        // 0x00433C21
        static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
        {
            if (widgetIndex == common::widx::company_select)
                common::switchCompany(self, itemIndex);
        }

        // 0x00433C7D
        static void onUpdate(window* self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self->number);
        }

        // 0x00433C97
        static void onResize(window* self)
        {
            common::enableRenameByCaption(self);

            uint16_t cargoHeight = 0;
            const auto company = companymgr::get(self->number);
            for (uint8_t i = 0; i < static_cast<uint8_t>(std::size(company->cargoDelivered)); i++)
            {
                auto cargo = ObjectManager::get<cargo_object>(i);
                if (cargo == nullptr || company->cargoDelivered[i] == 0)
                    continue;

                cargoHeight += 10;
            }

            const uint16_t windowHeight = std::max<int16_t>(cargoHeight, 50) + 62;

            self->setSize({ windowSize.width, windowHeight });
        }

        static void initEvents()
        {
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.on_mouse_down = onMouseDown;
            events.text_input = textInput;
            events.on_dropdown = onDropdown;
            events.on_update = onUpdate;
            events.on_resize = onResize;
        }
    }

    namespace challenge
    {
        const Gfx::ui_size_t windowSize = { 320, 182 };

        static widget_t widgets[] = {
            commonWidgets(320, 182, StringIds::title_company_challenge),
            widgetEnd(),
        };

        constexpr uint64_t enabledWidgets = common::enabledWidgets;

        static window_event_list events;

        // 0x00433D39
        static void prepareDraw(window* self)
        {
            common::switchTabWidgets(self);

            // Set company name.
            auto company = companymgr::get(self->number);
            FormatArguments args{};
            args.push(company->name);

            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::company_select].right = self->width - 3;
            self->widgets[common::widx::company_select].left = self->width - 28;
            self->widgets[common::widx::company_select].type = widget_type::none;

            common::repositionTabs(self);
        }

        // 0x00433DEB
        static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);

            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x00433DF5, regs);
        }

        // 0x00433FFE
        static void onMouseUp(window* self, widget_index widgetIndex)
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
        static void textInput(window* self, widget_index callingWidget, char* input)
        {
            if (callingWidget == common::widx::caption)
            {
                common::renameCompany(self, input);
            }
        }

        // 0x0043402E
        static void onUpdate(window* self)
        {
            self->frame_no += 1;
            self->callPrepareDraw();
            WindowManager::invalidate(WindowType::company, self->number);
        }

        // 0x00434048
        static void onResize(window* self)
        {
            self->setSize(windowSize);
        }

        static void initEvents()
        {
            events.prepare_draw = prepareDraw;
            events.draw = draw;
            events.on_mouse_up = onMouseUp;
            events.text_input = textInput;
            events.on_update = onUpdate;
            events.on_resize = onResize;
        }
    }

    // 00434731
    window* openChallenge(company_id_t companyId)
    {
        auto window = WindowManager::bringToFront(WindowType::company, companyId);
        if (window != nullptr)
        {
            if (Input::isToolActive(window->type, window->number))
            {
                Input::toolCancel();
                window = WindowManager::bringToFront(WindowType::company, companyId);
            }
        }

        if (window == nullptr)
        {
            window = create(companyId);
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
        window->initScrollWidgets();
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
            const Gfx::ui_size_t* windowSize;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { status::widgets, widx::tab_status, &status::events, &status::enabledWidgets, &status::windowSize },
            { details::widgets, widx::tab_details, &details::events, &details::enabledWidgets, &details::windowSize },
            { colour_scheme::widgets, widx::tab_colour_scheme, &colour_scheme::events, &colour_scheme::enabledWidgets, &colour_scheme::windowSize },
            { finances::widgets, widx::tab_finances, &finances::events, &finances::enabledWidgets, &finances::windowSize },
            { CargoDelivered::widgets, widx::tab_cargo_delivered, &CargoDelivered::events, &CargoDelivered::enabledWidgets, &CargoDelivered::windowSize },
            { challenge::widgets, widx::tab_challenge, &challenge::events, &challenge::enabledWidgets, &challenge::windowSize }
        };

        static void initEvents()
        {
            status::initEvents();
            details::initEvents();
            colour_scheme::initEvents();
            finances::initEvents();
            CargoDelivered::initEvents();
            challenge::initEvents();
        }

        static void switchCompany(window* self, int16_t itemIndex)
        {
            if (itemIndex == -1)
                return;

            company_id_t companyId = dropdown::getCompanyIdFromSelection(itemIndex);

            // Try to find an open company window for this company.
            auto companyWindow = WindowManager::bringToFront(WindowType::company, companyId);
            if (companyWindow != nullptr)
                return;

            // If not, we'll turn this window into a window for the company selected.
            auto company = companymgr::get(companyId);
            if (company->name == StringIds::empty)
                return;

            self->number = companyId;
            self->owner = companyId;

            common::disableChallengeTab(self);
            self->invalidate();
        }

        static void switchTabWidgets(window* self)
        {
            self->activated_widgets = 0;

            static widget_t* widgetCollectionsByTabId[] = {
                status::widgets,
                details::widgets,
                colour_scheme::widgets,
                finances::widgets,
                CargoDelivered::widgets,
                challenge::widgets,
            };

            widget_t* newWidgets = widgetCollectionsByTabId[self->current_tab];
            if (self->widgets != newWidgets)
            {
                self->widgets = newWidgets;
                // self->initScrollWidgets();
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
            if (Input::isToolActive(self->type, self->number))
                Input::toolCancel();

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
            self->invalidate();
            self->setSize(*tabInfo.windowSize);
            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();

            if (tabInfo.widgetIndex == widx::tab_finances)
                finances::sub_4C8DBF(self);
        }

        // 0x0043252E
        static void renameCompanyPrompt(window* self, widget_index widgetIndex)
        {
            auto company = companymgr::get(self->number);
            textinput::openTextinput(self, StringIds::title_name_company, StringIds::prompt_enter_new_company_name, company->name, widgetIndex, nullptr);
        }

        // 0x0043254F
        static void renameCompany(window* self, char* input)
        {
            if (strlen(input) == 0)
                return;

            gGameCommandErrorTitle = StringIds::cannot_rename_this_company;

            uint32_t* buffer = (uint32_t*)input;
            game_commands::do_30(self->number, 1, buffer[0], buffer[1], buffer[2]);
            game_commands::do_30(0, 2, buffer[3], buffer[4], buffer[5]);
            game_commands::do_30(0, 0, buffer[6], buffer[7], buffer[8]);
        }

        static void drawCompanySelect(const window* const self, Gfx::drawpixelinfo_t* const dpi)
        {
            const auto company = companymgr::get(self->number);
            const auto competitor = ObjectManager::get<competitor_object>(company->competitor_id);

            // Draw company owner face.
            const uint32_t image = Gfx::recolour(competitor->images[company->owner_emotion], company->mainColours.primary);
            const uint16_t x = self->x + self->widgets[common::widx::company_select].left + 1;
            const uint16_t y = self->y + self->widgets[common::widx::company_select].top + 1;
            Gfx::drawImage(dpi, x, y, image);
        }

        // 0x00434413
        static void drawTabs(window* self, Gfx::drawpixelinfo_t* dpi)
        {
            auto skin = ObjectManager::get<interface_skin_object>();

            // Status tab
            {
                const uint32_t imageId = skin->img + InterfaceSkin::ImageIds::tab_company;
                widget::draw_tab(self, dpi, imageId, widx::tab_status);
            }

            // Details tab
            {
                const uint32_t imageId = Gfx::recolour(skin->img + InterfaceSkin::ImageIds::tab_company_details, self->colours[0]);
                widget::draw_tab(self, dpi, imageId, widx::tab_details);
            }

            // Colour scheme tab
            {
                static const uint32_t colourSchemeTabImageIds[] = {
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame0,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame1,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame2,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame3,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame4,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame5,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame6,
                    InterfaceSkin::ImageIds::tab_colour_scheme_frame7,
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
                    InterfaceSkin::ImageIds::tab_finances_frame0,
                    InterfaceSkin::ImageIds::tab_finances_frame1,
                    InterfaceSkin::ImageIds::tab_finances_frame2,
                    InterfaceSkin::ImageIds::tab_finances_frame3,
                    InterfaceSkin::ImageIds::tab_finances_frame4,
                    InterfaceSkin::ImageIds::tab_finances_frame5,
                    InterfaceSkin::ImageIds::tab_finances_frame6,
                    InterfaceSkin::ImageIds::tab_finances_frame7,
                    InterfaceSkin::ImageIds::tab_finances_frame8,
                    InterfaceSkin::ImageIds::tab_finances_frame9,
                    InterfaceSkin::ImageIds::tab_finances_frame10,
                    InterfaceSkin::ImageIds::tab_finances_frame11,
                    InterfaceSkin::ImageIds::tab_finances_frame12,
                    InterfaceSkin::ImageIds::tab_finances_frame13,
                    InterfaceSkin::ImageIds::tab_finances_frame14,
                    InterfaceSkin::ImageIds::tab_finances_frame15,
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
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame0,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame1,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame2,
                    InterfaceSkin::ImageIds::tab_cargo_delivered_frame3,
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
                    InterfaceSkin::ImageIds::tab_cup_frame0,
                    InterfaceSkin::ImageIds::tab_cup_frame1,
                    InterfaceSkin::ImageIds::tab_cup_frame2,
                    InterfaceSkin::ImageIds::tab_cup_frame3,
                    InterfaceSkin::ImageIds::tab_cup_frame4,
                    InterfaceSkin::ImageIds::tab_cup_frame5,
                    InterfaceSkin::ImageIds::tab_cup_frame6,
                    InterfaceSkin::ImageIds::tab_cup_frame7,
                    InterfaceSkin::ImageIds::tab_cup_frame8,
                    InterfaceSkin::ImageIds::tab_cup_frame9,
                    InterfaceSkin::ImageIds::tab_cup_frame10,
                    InterfaceSkin::ImageIds::tab_cup_frame11,
                    InterfaceSkin::ImageIds::tab_cup_frame12,
                    InterfaceSkin::ImageIds::tab_cup_frame13,
                    InterfaceSkin::ImageIds::tab_cup_frame14,
                    InterfaceSkin::ImageIds::tab_cup_frame15,
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
                if (self->isDisabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }
    }
}
