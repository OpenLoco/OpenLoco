#include "../companymgr.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::CompanyFaceSelection
{
    loco_global<uint8_t, 0x9C68F2> _9C68F2;
    loco_global<uint16_t, 0x112C1C1> _112C1C1;

    static const gfx::ui_size_t windowSize = { 400, 272 };
    static window_event_list events;

    enum widx
    {
        frame,
        caption,
        close_button,
        panel,
        scrollview,
        unk_5
    };

    // 0x509680
    static widget_t widgets[] = {
        make_widget({ 0, 0 }, { 400, 272 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 398, 13 }, widget_type::caption_24, 0, string_ids::company_face_selection_title),
        make_widget({ 385, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { 400, 257 }, widget_type::panel, 1),
        make_widget({ 4, 19 }, { 188, 248 }, widget_type::scrollview, 1, vertical, string_ids::tooltip_company_face_selection),
        make_widget({ 265, 23 }, { 66, 66 }, widget_type::wt_5, 1),
        widget_end(),
    };

    static void initEvents();

    void sub_435381()
    {
        registers regs;
        call(0x435381, regs);
    }

    void sub_4353F4(company_id_t id)
    {
        registers regs;
        regs.al = id;
        call(0x004353F4, regs);
    }

    void open(company_id_t id)
    {
        auto self = WindowManager::bringToFront(WindowType::companyFaceSelection, 0);

        if (self)
        {
            _9C68F2 = id;
            self->owner = id;
            sub_4353F4(id);
            self->invalidate();
        }
        else
        {
            initEvents();
            self = WindowManager::createWindow(WindowType::companyFaceSelection, windowSize, 0, &events);
            self->widgets = widgets;
            self->enabled_widgets = (1 << widx::close_button);
            self->init_scroll_widgets();
            _9C68F2 = id;
            self->owner = id;
            auto skin = objectmgr::get<interface_skin_object>();
            self->colours[1] = skin->colour_0A;
            sub_435381();
            sub_4353F4(id);
            self->row_count = _112C1C1;
            self->row_hover = -1;
            self->var_85A = -1;
            self->var_85C = -1;
        }
    }

    static void onClose(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x4352A4, regs);
    }

    // 0x435299
    static void onMouseUp(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
                WindowManager::close(self);
                break;
        }
    }

    // 0x4352BB
    static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = _112C1C1 * 10;
    }

    static void scrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.esi = (int32_t)self;
        regs.cx = x;
        regs.dx = y;
        call(0x435314, regs);
    }

    static void scrollMouseOver(window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        registers regs;
        regs.ax = scroll_index;
        regs.esi = (int32_t)self;
        regs.cx = x;
        regs.dx = y;
        call(0x004352C7, regs);
    }

    // 0x4352B1
    static void tooltip(FormatArguments& args, window* self, widget_index)
    {
        args.push(string_ids::tooltip_scroll_list);
    }

    // 0x434FE8
    static void prepareDraw(window* self)
    {
        auto company = companymgr::get(_9C68F2);

        FormatArguments args{};
        args.push(company->name);
    }

    // 0x435003
    static void draw(window* self, gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        if (self->row_hover == -1)
        {
            return;
        }

        registers regs;
        regs.esi = (int32_t)self;
        regs.edi = (int32_t)dpi;
        call(0x435003, regs);
    }

    static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        registers regs;
        regs.ax = scrollIndex;
        regs.esi = (int32_t)self;
        regs.edi = (int32_t)dpi;
        call(0x435152, regs);
    }

    static void initEvents()
    {
        events.on_close = onClose;
        events.on_mouse_up = onMouseUp;
        events.get_scroll_size = getScrollSize;
        events.scroll_mouse_down = scrollMouseDown;
        events.scroll_mouse_over = scrollMouseOver;
        events.tooltip = tooltip;
        events.prepare_draw = prepareDraw;
        events.draw = draw;
        events.draw_scroll = drawScroll;
    }
}
