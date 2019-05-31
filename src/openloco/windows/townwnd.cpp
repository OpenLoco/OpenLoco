#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::town
{
    namespace common
    {
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
        static void initEvents();
    }

    namespace town
    {
        static widget_t widgets[] = {
            commonWidgets(223, 161, string_ids::title_town),
            make_widget({ 3, 44 }, { 195, 104 }, widget_type::viewport, 1, 0xFFFFFFFE),
            make_widget({ 3, 139 }, { 195, 21 }, widget_type::wt_13, 1),
            make_widget({ 0, 0 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::null, string_ids::move_main_view_to_show_this),
            make_widget({ 198, 44 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::town_expand, string_ids::expand_this_town),
            make_widget({ 198, 68 }, { 24, 24 }, widget_type::wt_9, 1, image_ids::rubbish_bin, string_ids::demolish_this_town),
            widget_end(),
        };

        static window_event_list events;

        // 0x00498EAF
        static void prepare_draw(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00498EAF, regs);
        }

        // 0x00498FFE
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.edi = (int32_t)dpi;
            regs.esi = (int32_t)self;
            call(0x00498FFE, regs);
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
    }

    // 0x00499B7E
    // dx: townId
    // esi: {return}
    window* open(uint16_t townId)
    {
        registers regs;
        regs.dx = townId;
        call(0x00499B7E, regs);
        return (window*)regs.esi;
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
        static void initEvents()
        {
            town::initEvents();
            population::initEvents();
            company_ratings::initEvents();
        }
    }
}
