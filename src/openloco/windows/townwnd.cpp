#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../input.h"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::town
{
    static const gfx::ui_size_t windowSize = { 223, 161 };

    static loco_global<uint16_t[10], 0x0112C826> commonFormatArgs;

    namespace common
    {
        enum widx
        {
            frame,
            caption,
            close_button,
            panel,
            tab_town,
            tab_population,
            tab_company_ratings,
        };

        const uint64_t enabledWidgets = (1 << close_button) | (1 << widx::tab_town) | (1 << widx::tab_population) | (1 << widx::tab_company_ratings);

        // 0x00498E9B
        void sub_498E9B(window* w)
        {
            w->enabled_widgets |= (1 << 1);
#ifdef _DISABLE_TOWN_RENAME_
            if (is_editor_mode())
            {
                w->enabled_widgets &= ~(1 << 1);
            }
#endif
        }

#define commonWidgets(frameWidth, frameHeight, windowCaptionId)                                                                           \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::frame, 0),                                                            \
        make_widget({ 1, 1 }, { frameWidth - 2, 13 }, widget_type::caption_25, 0, windowCaptionId),                                       \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 0, 41 }, { frameWidth, 120 }, widget_type::panel, 1),                                                               \
        make_remap_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_town),                         \
        make_remap_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_population_graph),            \
        make_remap_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_8, 1, image_ids::tab, string_ids::tooltip_town_ratings_each_company)

        // Defined at the bottom of this file.
        static void prepare_draw(window* self);
        static void repositionTabs(window* self);
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi);
        static void initEvents();
    }

    namespace town
    {
        enum widx
        {
            viewport = 7,
            unk_8,
            centre_on_viewport,
            expand_town,
            demolish_town,
        };

        static widget_t widgets[] = {
            commonWidgets(223, 161, string_ids::title_town),
            make_widget({ 3, 44 }, { 195, 104 }, widget_type::viewport, 1, 0xFFFFFFFE),
            make_widget({ 3, 139 }, { 195, 21 }, widget_type::wt_13, 1),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            make_widget({ 198, 44 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::town_expand, string_ids::expand_this_town),
            make_widget({ 198, 68 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rubbish_bin, string_ids::demolish_this_town),
            widget_end(),
        };

        const uint64_t enabledWidgets = common::enabledWidgets | (1 << centre_on_viewport) | (1 << expand_town) | (1 << demolish_town);

        static window_event_list events;

        // 0x00498EAF
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            self->widgets[widx::viewport].right = self->width - 26;
            self->widgets[widx::viewport].bottom = self->height - 14;

            self->widgets[widx::unk_8].top = self->height - 12;
            self->widgets[widx::unk_8].bottom = self->height - 3;
            self->widgets[widx::unk_8].right = self->width - 14;

            self->widgets[widx::expand_town].right = self->width - 2;
            self->widgets[widx::expand_town].left = self->width - 25;

            self->widgets[widx::demolish_town].right = self->width - 2;
            self->widgets[widx::demolish_town].left = self->width - 25;

            if (is_editor_mode())
            {
                self->widgets[widx::expand_town].type = widget_type::wt_9;
                self->widgets[widx::demolish_town].type = widget_type::wt_9;
            }
            else
            {
                self->widgets[widx::expand_town].type = widget_type::none;
                self->widgets[widx::demolish_town].type = widget_type::none;
                self->widgets[widx::viewport].right += 22;
            }

            self->widgets[widx::centre_on_viewport].right = self->widgets[widx::viewport].right - 1;
            self->widgets[widx::centre_on_viewport].bottom = self->widgets[widx::viewport].bottom - 1;
            self->widgets[widx::centre_on_viewport].left = self->widgets[widx::viewport].right - 24;
            self->widgets[widx::centre_on_viewport].top = self->widgets[widx::viewport].bottom - 24;

            common::repositionTabs(self);
        }

        // 0x00498FFE
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            self->drawViewports(dpi);
            widget::sub_4CF487(dpi, self, (widget_index)widx::viewport);

            static string_id townSizeLabelIds[] = {
                string_ids::town_size_hamlet,
                string_ids::town_size_village,
                string_ids::town_size_town,
                string_ids::town_size_city,
                string_ids::town_size_metropolis,
            };

            auto town = townmgr::get(self->number);
            commonFormatArgs[0] = townSizeLabelIds[(uint8_t)town->size];
            commonFormatArgs[1] = town->population;

            const uint16_t xPos = self->x + self->widgets[widx::unk_8].left;
            const uint16_t yPos = self->y + self->widgets[widx::unk_8].top;
            const uint16_t width = self->widgets[widx::unk_8].right - self->widgets[widx::unk_8].left;
            gfx::draw_string_494BBF(*dpi, xPos, yPos, width, colour::black, string_ids::status_town_population, &*commonFormatArgs);
        }

        // 0x00499079
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x00499079, regs);
        }

        // 0x00499287
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x00499287, regs);
        }

        // 0x0049938B
        static void update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x0049938B, regs);
        }

        // 0x004993A5
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x004993A5, regs);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = update;
            events.prepare_draw = prepare_draw;
            events.text_input = text_input;
        }

        // 0x00499A87
        static void initViewport(window* self)
        {
            if (self->current_tab != 0)
                return;

            self->call_prepare_draw();

            auto town = townmgr::get(self->number);

            registers regs;
            regs.ax = town->x;
            regs.cx = town->y;
            regs.esi = (int32_t)self;
            call(0x00499AB2, regs);
        }
    }

    // 0x00499B7E
    window* open(uint16_t townId)
    {
        auto window = WindowManager::bringToFront(WindowType::town, townId);
        if (window != nullptr)
        {
            if (input::is_tool_active(window->type, window->number))
                input::cancel_tool();

            window = WindowManager::bringToFront(WindowType::town, townId);
        }

        if (window == nullptr)
        {
            // 0x00499C0D start
            window = WindowManager::createWindowCentred(WindowType::town, windowSize, 0, &town::events);
            window->widgets = town::widgets;
            window->enabled_widgets = town::enabledWidgets;
            window->number = townId;
            window->current_tab = 0;
            window->frame_no = 0;
            window->disabled_widgets = 0;
            window->min_width = 192;
            window->min_height = 161;
            window->max_width = 600;
            window->max_height = 440;
            window->flags |= window_flags::resizable;

            auto skin = objectmgr::get<interface_skin_object>();
            if (skin != nullptr)
            {
                window->colours[0] = skin->colour_0B;
                window->colours[1] = skin->colour_0C;
            }
            // 0x00499C0D end

            window->var_848 = -1;
        }

        // TODO(avgeffen): only needs to be called once.
        common::initEvents();

        window->current_tab = 0;
        window->invalidate();

        window->widgets = town::widgets;
        window->enabled_widgets = town::enabledWidgets;
        window->holdable_widgets = 0;
        window->event_handlers = &town::events;
        window->activated_widgets = 0;
        window->disabled_widgets = 0;
        window->init_scroll_widgets();
        town::initViewport(window);

        return window;
    }

    namespace population
    {
        static widget_t widgets[] = {
            commonWidgets(223, 161, string_ids::title_town_population),
            widget_end(),
        };

        static window_event_list events;

        // 0x00499469
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00499469, regs);
        }

        // 0x004994F9
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x004994F9, regs);
        }

        // 0x004996AC
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x004996AC, regs);
        }

        // 0x004996D1
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x004996D1, regs);
        }

        // 0x004996DC
        static void update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x004996DC, regs);
        }

        // 0x004996F6
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x004996F6, regs);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = update;
            events.prepare_draw = prepare_draw;
            events.text_input = text_input;
        }
    }

    namespace company_ratings
    {
        static widget_t widgets[] = {
            commonWidgets(340, 208, string_ids::title_town_local_authority),
            widget_end(),
        };

        static window_event_list events;

        // 0x00499761
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00499761, regs);
        }

        // 0x004997F1
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x004997F1, regs);
        }

        // 0x004998E7
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            registers regs;
            regs.edx = widgetIndex;
            regs.esi = (int32_t)self;
            call(0x004998E7, regs);
        }

        // 0x0049990C
        static void text_input(window* self, widget_index callingWidget, char* input)
        {
            registers regs;
            regs.cl = 1;
            regs.dx = callingWidget;
            regs.edi = (uintptr_t)input;
            regs.esi = (int32_t)self;
            call(0x0049990C, regs);
        }

        // 0x00499917
        static void update(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00499917, regs);
        }

        // 0x00499936
        static void on_resize(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00499936, regs);
        }

        static void initEvents()
        {
            events.draw = draw;
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = update;
            events.prepare_draw = prepare_draw;
            events.text_input = text_input;
        }
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
            { town::widgets, widx::tab_town, &town::events, &town::enabledWidgets },
            { population::widgets, widx::tab_population, &population::events, &common::enabledWidgets },
            { company_ratings::widgets, widx::tab_company_ratings, &company_ratings::events, &common::enabledWidgets }
        };

        static void prepare_draw(window* self)
        {
            // Reset tab widgets if needed.
            auto tabWidgets = tabInformationByTabOffset[self->current_tab].widgets;
            if (self->widgets != tabWidgets)
            {
                self->widgets = tabWidgets;
                self->init_scroll_widgets();
            }

            // Activate the current tab.
            self->activated_widgets &= ~((1 << widx::tab_town) | (1 << widx::tab_population) | (1 << widx::tab_company_ratings));
            widx widgetIndex = tabInformationByTabOffset[self->current_tab].widgetIndex;
            self->activated_widgets |= (1ULL << widgetIndex);

            // Put town name in place.
            commonFormatArgs[0] = townmgr::get(self->number)->name;

            // Resize common widgets.
            self->widgets[common::widx::frame].right = self->width - 1;
            self->widgets[common::widx::frame].bottom = self->height - 1;

            self->widgets[common::widx::caption].right = self->width - 2;

            self->widgets[common::widx::close_button].left = self->width - 15;
            self->widgets[common::widx::close_button].right = self->width - 3;

            self->widgets[common::widx::panel].right = self->width - 1;
            self->widgets[common::widx::panel].bottom = self->height - 1;
        }

        // 0x004999A7, 0x004999AD
        static void repositionTabs(window* self)
        {
            int16_t xPos = self->widgets[widx::tab_town].left;
            const int16_t tabWidth = self->widgets[widx::tab_town].right - xPos;

            for (uint8_t i = widx::tab_town; i <= widx::tab_company_ratings; i++)
            {
                if (self->is_disabled(i))
                    continue;

                self->widgets[i].left = xPos;
                self->widgets[i].right = xPos + tabWidth;
                xPos = self->widgets[i].right + 1;
            }
        }

        // 0x004999E1
        static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
        {
            auto skin = objectmgr::get<interface_skin_object>();

            // Town tab
            {
                const uint32_t imageId = skin->img + interface_skin::image_ids::toolbar_menu_towns;
                widget::draw_tab(self, dpi, imageId, widx::tab_town);
            }

            // Population tab
            {
                static const uint32_t populationTabImageIds[] = {
                    interface_skin::image_ids::tab_population_frame0,
                    interface_skin::image_ids::tab_population_frame1,
                    interface_skin::image_ids::tab_population_frame2,
                    interface_skin::image_ids::tab_population_frame3,
                    interface_skin::image_ids::tab_population_frame4,
                    interface_skin::image_ids::tab_population_frame5,
                    interface_skin::image_ids::tab_population_frame6,
                    interface_skin::image_ids::tab_population_frame7,
                };

                uint32_t imageId = gfx::recolour(skin->img, self->colours[1]);
                if (self->current_tab == widx::tab_population - widx::tab_town)
                    imageId += populationTabImageIds[(self->frame_no / 4) % std::size(populationTabImageIds)];
                else
                    imageId += populationTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_population);
            }

            // Company ratings tab
            {
                static const uint32_t ratingsTabImageIds[] = {
                    interface_skin::image_ids::tab_ratings_frame0,
                    interface_skin::image_ids::tab_ratings_frame1,
                    interface_skin::image_ids::tab_ratings_frame2,
                    interface_skin::image_ids::tab_ratings_frame3,
                    interface_skin::image_ids::tab_ratings_frame4,
                    interface_skin::image_ids::tab_ratings_frame5,
                    interface_skin::image_ids::tab_ratings_frame6,
                    interface_skin::image_ids::tab_ratings_frame7,
                    interface_skin::image_ids::tab_ratings_frame8,
                    interface_skin::image_ids::tab_ratings_frame9,
                    interface_skin::image_ids::tab_ratings_frame10,
                    interface_skin::image_ids::tab_ratings_frame11,
                    interface_skin::image_ids::tab_ratings_frame12,
                    interface_skin::image_ids::tab_ratings_frame13,
                    interface_skin::image_ids::tab_ratings_frame14,
                    interface_skin::image_ids::tab_ratings_frame15,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tab_company_ratings - widx::tab_town)
                    imageId += ratingsTabImageIds[(self->frame_no / 4) % std::size(ratingsTabImageIds)];
                else
                    imageId += ratingsTabImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tab_company_ratings);
            }
        }

        static void initEvents()
        {
            town::initEvents();
            population::initEvents();
            company_ratings::initEvents();
        }
    }
}
