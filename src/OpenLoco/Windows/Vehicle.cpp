#include "../Things/Vehicle.h"
#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Things/ThingManager.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Vehicle
{
    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_main,
            tab_vehicle_details,
            tab_cargo,
            tab_finances,
            tab_route,
        };

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                                    \
    makeWidget({ 0, 0 }, { (frameWidth), (frameHeight) }, widget_type::frame, 0),                                                                  \
        makeWidget({ 1, 1 }, { (frameWidth)-2, 13 }, widget_type::caption_24, 0, windowCaptionId),                                                 \
        makeWidget({ (frameWidth)-15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),           \
        makeWidget({ 0, 41 }, { 265, 136 }, widget_type::panel, 1),                                                                                \
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_main),     \
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_details), \
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_cargo),   \
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_finance), \
        makeRemapWidget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_route)

        constexpr uint64_t enabledWidgets = (1 << close_button) | (1 << tab_main) | (1 << tab_vehicle_details) | (1 << tab_cargo) | (1 << tab_finances) | (1 << tab_route);

        static OpenLoco::vehicle_head* getVehicle(const window* self)
        {
            return ThingManager::get<OpenLoco::vehicle_head>(self->number);
        }

        static void textInput(window* const self, const widget_index callingWidget, char* const input);
        static void renameVehicle(window* const self, const widget_index widgetIndex);
        static void switchTab(window* const self, const widget_index widgetIndex);
        static void sub_4B5ECD(window* const self);
        static void drawTabs(window* const window, Gfx::drawpixelinfo_t* const context);
    }

    namespace finances
    {

        static widget_t widgets[] = {
            commonWidgets(636, 319, StringIds::title_company_finances),
            widget_end(),
        };

        //constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << common::widx::company_select) | (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        //const uint64_t holdableWidgets = (1 << widx::loan_decrease) | (1 << widx::loan_increase);

        static window_event_list events;

    }

    static loco_global<window_event_list, 0x005003C0> _mainEvents;
    static loco_global<window_event_list, 0x00500434> _vehicleDetailsEvents;
    static loco_global<window_event_list, 0x005004A8> _cargoEvents;
    static loco_global<window_event_list, 0x00500554> _routeEvents;

    static loco_global<int32_t, 0x0113614E> _113614E;
    static loco_global<int16_t, 0x01136156> _1136156;

    static void sub_4B28E2(window* w, int dx)
    {
        registers regs;

        regs.dx = dx;
        regs.esi = (uintptr_t)w;

        call(0x004B28E2, regs);
    }

    namespace Main
    {
        static const Gfx::ui_size_t windowSize = { 265, 177 };
        static const Gfx::ui_size_t minWindowSize = { 192, 177 };
        static const Gfx::ui_size_t maxWindowSize = { 600, 440 };

        enum widx
        {
            viewport = common::widx::tab_route + 1,
            unk_10,
            unk_11,
            stop_start,
            pickup,
            pass_signal,
            change_direction,
            centre_viewport
        };

        static widget_t widgets[] = {
            commonWidgets(265, 177, StringIds::stringid),
            make_widget({ 3, 44 }, { 237, 120 }, widget_type::viewport, 1),
            make_widget({ 3, 155 }, { 237, 21 }, widget_type::wt_13, 1),
            make_widget({ 240, 46 }, { 24, 115 }, widget_type::wt_5, 1),
            make_widget({ 240, 44 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::stop_start, StringIds::tooltip_stop_start),
            make_widget({ 240, 68 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::null, StringIds::tooltip_remove_from_track),
            make_widget({ 240, 92 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::pass_signal, StringIds::tooltip_pass_signal_at_danger),
            make_widget({ 240, 116 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::construction_right_turnaround, StringIds::tooltip_change_direction),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::null, StringIds::move_main_view_to_show_this),
            widget_end()
        };

        constexpr uint64_t enabledWidgets = common::enabledWidgets | (1 << unk_11) | (1 << stop_start) | (1 << pickup) | (1 << pass_signal) | (1 << change_direction) | (1 << centre_viewport);

        static window_event_list events;

        // 0x004B5D82
        static void resetDisabledWidgets(window* const self)
        {
            self->disabled_widgets = 0;
        }

        // 0x004B5D88
        static void createViewport(window* const self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<uint32_t>(self);
            call(0x004B5D88, regs);
        }

        static uint16_t rowHeights[vehicleTypeCount] = {
            22,
            22,
            22,
            22,
            82,
            45
        };

        static void initEvents();

        // 0x004B60DC
        static window* create(const uint16_t head)
        {
            auto* const self = WindowManager::createWindow(WindowType::vehicle, windowSize, WindowFlags::flag_11 | WindowFlags::flag_8 | WindowFlags::resizable, &events);
            self->widgets = widgets;
            self->enabled_widgets = enabledWidgets;
            self->number = head;
            const auto* vehicle = ThingManager::get<OpenLoco::vehicle_head>(head);
            self->owner = vehicle->owner;
            self->row_height = rowHeights[static_cast<uint8_t>(vehicle->vehicleType)];
            self->current_tab = 0;
            self->frame_no = 0;
            resetDisabledWidgets(self);
            self->min_width = minWindowSize.width;
            self->min_height = minWindowSize.height;
            self->max_width = maxWindowSize.width;
            self->max_height = maxWindowSize.height;
            self->var_85C = -1;
            WindowManager::close(WindowType::dragVehiclePart, 0);
            _113614E = -1;
            _1136156 = -1;

            const auto* skin = ObjectManager::get<interface_skin_object>();
            self->colours[1] = skin->colour_0A;
            return self;
        }

        // 0x004B6033
        window* open(const OpenLoco::vehicle* vehicle)
        {
            const auto head = vehicle->head;
            auto* self = WindowManager::find(WindowType::vehicle, head);
            if (self != nullptr)
            {
                if (Input::isToolActive(self->type, self->number))
                {
                    Input::toolCancel();
                }
                self = WindowManager::bringToFront(WindowType::vehicle, head);
            }
            if (self == nullptr)
            {
                // TODO: only needs to be called once
                initEvents();
                self = create(head);
                self->saved_view.clear();
            }
            self->current_tab = 0;
            self->invalidate();
            self->widgets = widgets;
            self->enabled_widgets = enabledWidgets;
            self->holdable_widgets = (1 << widx::unk_11);
            self->event_handlers = &events;
            self->activated_widgets = 0;
            resetDisabledWidgets(self);
            self->initScrollWidgets();
            createViewport(self);
            return self;
        }

        // 0x004B24D1
        static void onMouseUp(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::caption:
                    common::renameVehicle(self, widgetIndex);
                    break;
                case common::widx::tab_main:
                case common::widx::tab_vehicle_details:
                case common::widx::tab_cargo:
                case common::widx::tab_finances:
                case common::widx::tab_route:
                    common::switchTab(self, widgetIndex);
                    break;
                case widx::pickup:
                case widx::change_direction:
                case widx::centre_viewport:
                case widx::pass_signal:
                case widx::unk_11:
                    throw std::runtime_error("Not implemented");
                    break;
            }
        }

        // 0x004B30F3
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 4);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 10);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 13);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 14);
            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 15);

            if (w->isDisabled(13))
            {
                Input::toolCancel(WindowType::vehicle, w->number);
                return;
            }

            auto vehicle = common::getVehicle(w);

            if (vehicle->tile_x != -1 && (vehicle->var_38 & Things::Vehicle::Flags38::unk_4) == 0)
            {
                return;
            }

            if (!WindowManager::isInFront(w))
                return;

            if (vehicle->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, 13);
            }
        }

        // 0x004B3210
        static void onResize(window* const self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            call(0x004B3210, regs);
        }

        // 0x004B251A
        static void onMouseDown(window* const self, const widget_index widgetIndex)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            regs.edx = widgetIndex;
            call(0x004B251A, regs);
        }

        // 0x004B253A
        static void onDropdown(window* const self, const widget_index widgetIndex, const int16_t itemIndex)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            regs.edx = widgetIndex;
            regs.eax = itemIndex;
            call(0x004B253A, regs);
        }

        // 0x004B2545
        static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004B2545, regs);
        }

        // 0x004B2550
        static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x004B2550, regs);
        }

        // 0x004B255B
        static void onToolAbort(window& self, const widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            call(0x004B255B, regs);
        }

        // 0x004B32F9
        static void viewportRotate(window* const self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            call(0x004B32F9, regs);
        }

        // 0x004B31F2
        static void tooltip(FormatArguments& args, Ui::window* const self, const widget_index widgetIndex)
        {
        }

        // 0x004B1EB5
        static void prepareDraw(window* const self)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            call(0x004B1EB5, regs);
        }

        // 0x004B226D
        static void draw(window* const self, Gfx::drawpixelinfo_t* const dpi)
        {
            registers regs{};
            regs.esi = reinterpret_cast<int32_t>(self);
            regs.edi = reinterpret_cast<int32_t>(dpi);
            call(0x004B226D, regs);
        }

        static void initEvents()
        {
            events.on_mouse_up = onMouseUp;
            events.on_resize = onResize;
            events.on_mouse_down = onMouseDown;
            events.on_dropdown = onDropdown;
            events.on_update = onUpdate;
            events.on_tool_update = onToolUpdate;
            events.on_tool_abort = onToolAbort;
            events.text_input = common::textInput;
            events.viewport_rotate = viewportRotate;
            events.tooltip = tooltip;
            events.prepare_draw = prepareDraw;
            events.draw = draw;
        }
    }

    namespace VehicleDetails
    {
        // 0x004B3C45
        // "Show <vehicle> design details and options" tab in vehicle window
        static void onUpdate(window* w)
        {
            if (w->number == _1136156)
            {
                if (WindowManager::find(WindowType::dragVehiclePart) == nullptr)
                {
                    _1136156 = -1;
                    _113614E = -1;
                    w->invalidate();
                }
            }

            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 5);

            if (_1136156 == -1 && w->isActivated(11))
            {
                WindowManager::invalidateWidget(WindowType::vehicle, w->number, 11);
            }

            if (w->isDisabled(10))
            {
                Input::toolCancel(WindowType::vehicle, w->number);
                return;
            }

            auto vehicle = common::getVehicle(w);
            if (vehicle->tile_x != -1 && (vehicle->var_38 & Things::Vehicle::Flags38::unk_4) == 0)
                return;

            if (!WindowManager::isInFrontAlt(w))
                return;

            if (vehicle->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, 10);
            }
        }
    }

    namespace cargo
    {

        static widget_t _widgets[] = {
            make_widget({ 0, 0 }, { 265, 177 }, widget_type::frame, 0, 0xFFFFFFFF),                                                                // 0
            make_widget({ 1, 1 }, { 263, 13 }, widget_type::caption_24, 0, StringIds::title_vehicle_cargo),                                       // 1
            make_widget({ 250, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),                  // 2
            make_widget({ 0, 41 }, { 265, 136 }, widget_type::panel, 1, 0xFFFFFFFF),                                                               // 3
            make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_main),     // 4
            make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_details), // 5
            make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_cargo),   // 6
            make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_finance), // 7
            make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_8, 1, ImageIds::tab_vehicle_background, StringIds::tooltip_vehicle_tab_route),  // 8
            make_widget({ 240, 44 }, { 24, 24 }, widget_type::wt_9, 1, ImageIds::SPR_2386, StringIds::refit_vehicle_tip),                        // 9
            make_widget({ 3, 44 }, { 259, 120 }, widget_type::scrollview, 1, vertical),                                                            // 10
            widget_end()
        };

        static bool canRefit(OpenLoco::vehicle_head* headVehicle)
        {
            if (!isPlayerCompany(headVehicle->owner))
            {
                return false;
            }

            OpenLoco::Things::Vehicle::Vehicle train(headVehicle);

            if (train.cars.empty())
            {
                return false;
            }

            auto object = ObjectManager::get<vehicle_object>(train.cars[0].carComponents[0].front->object_id);
            return (object->flags & FlagsE0::refittable);
        }

#pragma warning(disable : 4505)
        // 004B3DDE
        static void ui__vehicle__event_3_prepare_draw(window* self)
        {
            if (self->widgets != _widgets)
            {
                self->initScrollWidgets();
            }

            self->enabled_widgets &= ~0b111110000;
            self->enabled_widgets |= self->current_tab + 4;

            auto* headVehicle = common::getVehicle(self);
            auto ax = headVehicle->var_44;
            auto cx = headVehicle->var_22;

            FormatArguments args = {};
            args.push(cx);
            args.push(ax);

            _widgets[0].right = self->width - 1;
            _widgets[0].bottom = self->height - 1;
            _widgets[3].right = self->width - 1;
            _widgets[3].bottom = self->height - 1;
            _widgets[2].left = self->width - 15;
            _widgets[2].right = self->width - 3;
            _widgets[0xA].right = self->width - 26;
            _widgets[0xA].bottom = self->height - 14;
            _widgets[9].right = self->width - 2;
            _widgets[9].left = self->width - 25;
            _widgets[9].type = widget_type::wt_9;
            if (!canRefit(headVehicle))
            {
                _widgets[9].type = widget_type::none;
                _widgets[0xA].right = self->width - 26 + 22;
            }

            common::sub_4B5ECD(self);
        }

        static void sub_4B6658(OpenLoco::vehicle_head* vehicle)
        {
            // TODO: implement
        }

        // 004B3F0D
        static void ui__vehicle__event_3_draw(Ui::window* window, Gfx::drawpixelinfo_t* context)
        {
            window->draw(context);
            common::drawTabs(window, context);

            sub_4B6658(common::getVehicle(window));

            FormatArguments args = {};
            args.push<string_id>(StringIds::buffer_1250);

            Gfx::drawString_494BBF(*context, window->x + 3, window->y + window->height - 13, window->width - 15, Colour::black, StringIds::str_1282, &args);
        }

        // 004B3F62
        static void ui__vehicle__event_3_draw_scroll(window* w, Gfx::drawpixelinfo_t* pDrawpixelinfo, uint32_t i)
        {
        }

        // 004B41BD
        static void ui__vehicle__event_3_on_mouse_up(window* w, widget_index i)
        {
        }

        // 004B41E2
        static void ui__vehicle__event_3_on_mouse_down(window* w, widget_index i)
        {
        }

        // 004B41E9
        static void ui__vehicle__event_3_on_dropdown(window* w, widget_index i, int16_t i1)
        {
        }

        // 004B41F4
        static void ui__vehicle__event_3_text_input(window* w, widget_index i, char* string)
        {
        }

        // 0x004B4339
        static void ui__vehicle__event_3_tooltip(FormatArguments& args, Ui::window* self, widget_index widgetIndex)
        {
            args.push(StringIds::tooltip_scroll_vehicle_list);

            auto vehicle = common::getVehicle(self);
            args.push(StringIds::getVehicleType(vehicle->vehicleType));
        }

        // 0x004B4360
        static void ui__vehicle__event_3_get_scroll_size(Ui::window* window, uint32_t i, uint16_t* width, uint16_t* height)
        {
            Things::Vehicle::Vehicle train(common::getVehicle(window));

            *height = 0;
            if (train.cars.empty())
            {
                return;
            }

            int rows = train.cars.size();
            *height = rows * window->row_height;
        }

        // 0x004B4404
        static void ui__vehicle__event_3_scroll_mouse_over(window* pWindow, int16_t i, int16_t i1, uint8_t i2)
        {
        }

        // 0x004B45DD
        static void ui__vehicle__event_3_8(window* w)
        {
            w->flags |= WindowFlags::not_scroll_view;
        }

        // 0x004B45E5
        static void ui__vehicle__event_3_9(window* w)
        {
            if (w->flags & WindowFlags::not_scroll_view)
            {
                if (w->row_hover != -1)
                {
                    w->row_hover = -1;
                    w->invalidate();
                }
            }
        }

        // 0x004B4607
        static void ui__vehicle__event_3_on_update(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();
            WindowManager::invalidateWidget(w->type, w->number, 6);
        }

        // 0x004B4621
        static void ui__vehicle__event_3_on_resize(window* w)
        {
            w->setSize({ 192, 142 }, { 400, 440 });
        }
    }

    namespace route_details
    {
        // 0x004B55D1
        // "Show <vehicle> route details" tab in vehicle window
        static void onUpdate(window* w)
        {
            w->frame_no += 1;
            w->callPrepareDraw();

            WindowManager::invalidateWidget(WindowType::vehicle, w->number, 8);

            if (!WindowManager::isInFront(w))
                return;

            if (common::getVehicle(w)->owner != CompanyManager::getControllingId())
                return;

            if (!Input::isToolActive(WindowType::vehicle, w->number))
            {
                sub_4B28E2(w, 13);
            }
        }
    }

    namespace common
    {

        // 0x004B26C0
        static void textInput(window* const self, const widget_index callingWidget, char* const input)
        {
            registers regs;
            regs.dx = callingWidget;
            regs.esi = reinterpret_cast<int32_t>(self);
            regs.cl = 1;
            regs.edi = reinterpret_cast<uintptr_t>(input);
            call(0x004B26C0, regs);
        }

        // 0x004B2680
        static void renameVehicle(window* self, widget_index widgetIndex)
        {
            auto vehicle = getVehicle(self);
            if (vehicle != nullptr)
            {
                FormatArguments args{};
                args.push(StringIds::getVehicleType(vehicle->vehicleType)); // 0
                args.skip(6);
                args.push(StringIds::getVehicleType(vehicle->vehicleType)); // 8
                TextInput::openTextInput(self, StringIds::title_name_vehicle, StringIds::prompt_enter_new_vehicle_name, vehicle->var_22, widgetIndex, &vehicle->var_44);
            }
        }

        // 0x004B2566
        static void switchTab(window* self, widget_index widgetIndex)
        {
            throw std::runtime_error("Not implemented");
        }

        // 0x004B5ECD
        static void sub_4B5ECD(window* const self)
        {
        }

        // 0x004B5F0D
        static void drawTabs(window* const window, Gfx::drawpixelinfo_t* const context)
        {
        }
    }

    void registerHooks()
    {
        _mainEvents->on_mouse_up = Main::onMouseUp;
        _mainEvents->on_update = Main::onUpdate;
        _vehicleDetailsEvents->on_update = VehicleDetails::onUpdate;
        _cargoEvents->on_mouse_up = cargo::ui__vehicle__event_3_on_mouse_up;
        _cargoEvents->on_resize = cargo::ui__vehicle__event_3_on_resize;
        _cargoEvents->on_mouse_down = cargo::ui__vehicle__event_3_on_mouse_down;
        _cargoEvents->draw = cargo::ui__vehicle__event_3_draw;
        _cargoEvents->draw_scroll = cargo::ui__vehicle__event_3_draw_scroll;
        _cargoEvents->prepare_draw = cargo::ui__vehicle__event_3_prepare_draw;
        _cargoEvents->on_dropdown = cargo::ui__vehicle__event_3_on_dropdown;
        _cargoEvents->text_input = cargo::ui__vehicle__event_3_text_input;
        _cargoEvents->tooltip = cargo::ui__vehicle__event_3_tooltip;
        _cargoEvents->get_scroll_size = cargo::ui__vehicle__event_3_get_scroll_size;
        _cargoEvents->scroll_mouse_over = cargo::ui__vehicle__event_3_scroll_mouse_over;
        _cargoEvents->event_08 = cargo::ui__vehicle__event_3_8;
        _cargoEvents->event_09 = cargo::ui__vehicle__event_3_9;
        _cargoEvents->on_update = cargo::ui__vehicle__event_3_on_update;
        _cargoEvents->on_resize = cargo::ui__vehicle__event_3_on_resize;
        _routeEvents->on_update = route_details::onUpdate;
    }
}
