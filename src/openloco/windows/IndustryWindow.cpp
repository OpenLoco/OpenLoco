#include "../config.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../industrymgr.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../localisation/string_ids.h"
#include "../objects/industry_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"
#include "../viewportmgr.h"
#include "../widget.h"

using namespace openloco::interop;
using namespace openloco::game_commands;

namespace openloco::ui::windows::industry
{
    static const gfx::ui_size_t windowSize = { 223, 161 };
    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_industry,
            tab_unknown,
            tab_production,
            tab_transported,
        };

        const uint64_t enabledWidgets = (1 << widx::caption) | (1 << widx::close_button) | (1 << widx::tab_industry) | (1 << widx::tab_unknown) | (1 << widx::tab_production) | (1 << widx::tab_transported);

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 120 }, widget_type::panel, 1),                                                               \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_industry),                     \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_production_graph),            \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_production_graph),            \
        make_remap_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_statistics)

        // Defined at the bottom of this file.
        static void prepare_draw(window* self);
        static void text_input(window* self, widget_index callingWidget, char* input);
        static void update(window* self);
        static void repositionTabs(window* self);
        static void initEvents();
    }

    namespace industry
    {
        enum widx
        {
            viewport = 8,
            status_bar,
            centre_on_viewport,
            demolish_industry,
        };

        static widget_t widgets[] = {
            commonWidgets(223, 136, string_ids::title_town),
            make_widget({ 3, 44 }, { 195, 79 }, widget_type::viewport, 1, 0xFFFFFFFE),
            make_widget({ 3, 139 }, { 115, 14 }, widget_type::wt_13, 1),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            make_widget({ 198, 44 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rubbish_bin, string_ids::demolish_this_industry),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << centre_on_viewport) | (1 << demolish_industry);

        static window_event_list events;

        // 0x00455ADD
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            self->widgets[widx::viewport].right = self->width - 26;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::status_bar].top = self->height - 12;
            self->widgets[widx::status_bar].bottom = self->height - 3;
            self->widgets[widx::status_bar].right = self->width - 14;

            self->widgets[widx::demolish_industry].right = self->width - 2;
            self->widgets[widx::demolish_industry].left = self->width - 25;

            if (is_editor_mode())
            {
                self->widgets[widx::demolish_industry].type = widget_type::wt_9;
            }
            else
            {
                self->widgets[widx::demolish_industry].type = widget_type::none;
                self->widgets[widx::viewport].right += 22;
            }

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            common::repositionTabs(self);
        }

        // 0x00455C22
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x00455C22, regs);
        }

        // 0x00455C86
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.esi = (uint32_t)self;
            regs.edx = widgetIndex;
            call(0x00455C86, regs);
        }

        static void initViewport(window* self);

        // 0x00455F1A
        static void on_resize(window* self)
        {
            self->set_size(gfx::ui_size_t(192, 137), gfx::ui_size_t(600, 440));

            if (self->viewports[0] != nullptr)
            {
                uint16_t newWidth = self->width - 30;
                if (!is_editor_mode())
                    newWidth += 22;

                uint16_t newHeight = self->height - 59;

                auto& viewport = self->viewports[0];
                if (newWidth != viewport->width || newHeight != viewport->height)
                {
                    viewport->width = newWidth;
                    viewport->height = newHeight;
                    viewport->view_width = newWidth << viewport->zoom;
                    viewport->view_height = newHeight << viewport->zoom;
                    self->saved_view.clear();
                }
            }

            initViewport(self);
        }

        // 0x00456C36
        static void initViewport(window* self)
        {
            if (self->current_tab != 0)
                return;

            self->call_prepare_draw();

            // Figure out the industry's position on the map.
            auto industry = industrymgr::get(self->number);
            int16_t tileZ = openloco::map::tile_element_height(industry->x, industry->y) & 0xFFFF;

            // Compute views.
            SavedView view = {
                industry->x,
                industry->y,
                ZoomLevel::quarter,
                static_cast<int8_t>(self->viewports[0]->getRotation()),
                tileZ,
            };

            uint16_t flags = 0;
            if (self->viewports[0] != nullptr)
            {
                if (self->saved_view == view)
                    return;

                flags = self->viewports[0]->flags;
                self->viewports[0]->width = 0;
                self->viewports[0] = nullptr;
                viewportmgr::collectGarbage();
            }
            else
            {
                if ((config::get().flags & config::flags::gridlines_on_landscape) != 0)
                    flags |= viewport_flags::gridlines_on_landscape;
            }

            self->saved_view = view;

            // 0x00499B39 start
            if (self->viewports[0] == nullptr)
            {
                auto widget = &self->widgets[widx::viewport];
                auto tile = openloco::map::map_pos3({ industry->x, industry->y, tileZ });
                auto origin = gfx::point_t(widget->left + self->x + 1, widget->top + self->y + 1);
                auto size = gfx::ui_size_t(widget->width() - 2, widget->height() - 2);
                viewportmgr::create(self, 0, origin, size, self->saved_view.zoomLevel, tile);
                self->invalidate();
                self->flags |= window_flags::viewport_no_scrolling;
            }
            // 0x00499B39 end

            if (self->viewports[0] != nullptr)
            {
                self->viewports[0]->flags = flags;
                self->invalidate();
            }
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = common::update;
            events.prepare_draw = prepare_draw;
            events.text_input = common::text_input;
            events.viewport_rotate = initViewport;
        }
    }

    // 0x00456D2D
    window* open(uint16_t industryId)
    {
        auto window = WindowManager::bringToFront(WindowType::industry, industryId);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            window = WindowManager::bringToFront(WindowType::industry, industryId);
        }

        if (window == nullptr)
        {
            // 0x00456DBC start
            const uint32_t newFlags = window_flags::flag_8 | window_flags::resizable;
            window = WindowManager::createWindow(WindowType::industry, windowSize, newFlags, &industry::events);
            window->number = industryId;
            window->min_width = 192;
            window->min_height = 137;
            window->max_width = 600;
            window->max_height = 440;

            auto skin = objectmgr::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = skin->colour_0B;
                window->colours[1] = skin->colour_0C;
            }
            // 0x00456DBC end

            window->saved_view.clear();
        }

        // TODO(avgeffen): only needs to be called once.
        common::initEvents();

        window->current_tab = 0;
        window->invalidate();

        window->widgets = industry::widgets;
        window->enabled_widgets = industry::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &industry::events;
        window->activated_widgets = 0;
        //auto industry = objectmgr::get<industry_object>(window->number);
        auto disabledWidgets = 0;

        /*if (industry->var_DE[0] == -1)
            disabledWidgets |= 0x20;
        if (industry->var_DE[1] == -1)
            disabledWidgets |= 0x40;*/

        window->disabled_widgets = disabledWidgets;
        window->init_scroll_widgets();
        industry::initViewport(window);

        return window;
    }

    namespace unknown
    {
        static widget_t widgets[] = {
            commonWidgets(222, 136, string_ids::title_industry_monthly_production),
            widget_end(),
        };

        static window_event_list events;
    }

    namespace production
    {
        static widget_t widgets[] = {
            commonWidgets(222, 136, string_ids::title_industry_monthly_production),
            widget_end(),
        };

        static window_event_list events;
    }

    namespace transported
    {
        static widget_t widgets[] = {
            commonWidgets(300, 126, string_ids::title_statistics),
            widget_end(),
        };

        static window_event_list events;
    }

    namespace common
    {
        struct TabInformation
        {
            widget_t* widgets;
            const widx widgetIndex;
            window_event_list* events;
            const uint64_t* enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { industry::widgets, widx::tab_industry, &industry::events, &industry::enabledWidgets },
            { unknown::widgets, widx::tab_unknown, &unknown::events, &common::enabledWidgets },
            { production::widgets, widx::tab_production, &production::events, &common::enabledWidgets },
            { transported::widgets, widx::tab_transported, &transported::events, &common::enabledWidgets }
        };

        static void prepare_draw(window* self)
        {
            // Reset tab widgets if needed.
            std::printf("%d\n", self->current_tab);
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->init_scroll_widgets();
            }

            // Activate the current tab.
            self->activated_widgets &= ~((1 << widx::tab_industry) | (1 << widx::tab_unknown) | (1 << widx::tab_production) | (1 << widx::tab_transported));
            widx widgetIndex = tabInformationByTabOffset[self->current_tab].widgetIndex;
            self->activated_widgets |= (1ULL << widgetIndex);

            // Put industry name in place.
            auto industry = industrymgr::get(self->number);
            auto args = FormatArguments();
            args.push(industry->name);
            args.push<uint16_t>(0);
            args.push(industry->var_D5);
            // Resize common widgets.
            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
        }

        // 0x00455CBC
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.dx = callingWidget;
            regs.esi = (int32_t)self;
            regs.cl = 1;
            regs.edi = (uintptr_t)input;
            call(0x00456C36, regs);
        }

        static void update(window* self)
        {
            self->frame_no++;
            self->call_prepare_draw();
            WindowManager::invalidate(WindowType::industry, self->number);
        }

        // 0x00456A5E, 0x00456A64
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_industry].left;
            const int16_t tabWidth = self->widgets[widx::tab_industry].right - xPos;

            for (uint8_t i = widx::tab_industry; i <= widx::tab_transported; i++)
            {
                if (self->is_disabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }
        static void initEvents()
        {
            industry::initEvents();
        }
    }
}