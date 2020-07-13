#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::map
{
    static loco_global<int32_t, 0x00523338> _cursorX2;
    static loco_global<int32_t, 0x0052333C> _cursorY2;
    static loco_global<uint32_t, 0x0526284> _dword_526284;
    static loco_global<uint16_t, 0x00526288> _word_526288;
    static loco_global<uint16_t, 0x0052628A> _word_52628A;
    static loco_global<uint16_t, 0x0052628C> _word_52628C;
    static loco_global<uint16_t, 0x0052628E> _word_52628E;
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;
    static loco_global<uint32_t, 0x00F253A8> _dword_F253A8;
    static loco_global<uint32_t, 0x00F2541D> _word_F2541D;
    static loco_global<uint16_t[2], 0x0110015E> _unk_110015E;

    enum widx
    {
        frame = 0,
        caption,
        closeButton,
        panel,
        tabOverall,
        tabVehicles,
        tabIndustries,
        tabRoutes,
        tabOwnership,
        scrollview,
        statusBar,
    };

    const uint64_t enabledWidgets = (1 << closeButton) | (1 << tabOverall) | (1 << tabVehicles) | (1 << tabIndustries) | (1 << tabRoutes) | (1 << tabOwnership);

    widget_t widgets[] = {
        make_widget({ 0, 0 }, { 350, 272 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 348, 13 }, widget_type::caption_25, 0),
        make_widget({ 335, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 350, 230 }, widget_type::panel, 1),
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_overall),
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_vehicles),
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_industries),
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_routes),
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_ownership),
        make_widget({ 3, 44 }, { 240, 215 }, widget_type::scrollview, 1, horizontal | vertical),
        make_widget({ 3, 250 }, { 322, 21 }, widget_type::wt_13, 1),
        widget_end()
    };

    static window_event_list events;

    // 0x0046B8E6
    static void onClose(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;

        call(0x0046B8E6, regs);
    }

    // 0x0046B8CF
    static void onMouseUp(window* self, widget_index widgetIndex)
    {
        registers regs;
        regs.esi = (int32_t)self;
        regs.edx = widgetIndex;

        call(0x0046B8CF, regs);
    }

    // 0x0046B9F7
    static void onResize(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;

        call(0x0046B9F7, regs);
    }

    static void sub_46C544(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x0046C544, regs);
    }

    static void sub_46D300(window* self, int16_t x, int16_t y)
    {
        //registers regs;
        //regs.cx = x;
        //regs.dx = y;
        //regs.esi = (int32_t)self;
        //call(0x0046D300, regs);
        auto i = 0;

        if (!input::has_flag(input::input_flags::flag5))
            int32_t cursorX = _cursorX2;
            int32_t cursorY = _cursorY2;
            auto window = WindowManager::findAt(cursorX, cursorY);

            if (window != nullptr)
            {
                if (window == self)
                {
                    cursorX -= x;
                    if (cursorX <= 100)
                    {
                        cursorY -= y;
                        if (cursorY < 60)
                        {
                            y = cursorY;

                            for (; i < 6; i++)
                            {
                                y -= 10;

                                if (y < 0)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        uint32_t eax;

        if (y < 0)
        {
            eax |= (1 << i);
        }
        else
        {
            eax = 0;
        }

        if (eax != self->var_854)
        {
            self->var_854 = eax;
            self->invalidate();
        }

        if (self->var_854 != 0)
        {
            self->invalidate();
        }
    }

    static void sub_46D406(window* self, int16_t x, int16_t y)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x0046D406, regs);
    }

    static void sub_46D520(window* self, int16_t x, int16_t y)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x0046D520, regs);
    }

    static void sub_46D66A(window* self, int16_t x, int16_t y)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x0046D66A, regs);
    }

    static void sub_46D789(window* self, int16_t x, int16_t y)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x0046D789, regs);
    }

    static void sub_46B69C()
    {
        registers regs;
        call(0x0046B69C, regs);
    }

    // 0x0046BA5B
    static void onUpdate(window* self)
    {
        //registers regs;
        //regs.esi = (int32_t)self;

        //call(0x0046BA5B, regs);

        self->frame_no++;
        self->call_prepare_draw();

        WindowManager::invalidateWidget(WindowType::map, self->number, self->current_tab + tabOverall);

        _word_F2541D++;

        if (gCurrentRotation != self->var_846)
        {
            self->var_846 = gCurrentRotation;
            sub_46B69C();
        }

        auto i = 80;

        while (i > 0)
        {
            sub_46C544(self);
            i--;
        }

        self->invalidate();

        auto x = self->x + self->width - 104;
        auto y = self->y + 44;

        switch (self->current_tab + tabOverall)
        {
            case tabOverall:
                sub_46D300(self, x, y);
                break;

            case tabVehicles:
                sub_46D406(self, x, y);
                break;

            case tabIndustries:
                sub_46D520(self, x, y);
                break;

            case tabRoutes:
                sub_46D66A(self, x, y);
                break;

            case tabOwnership:
                sub_46D789(self, x, y);
                break;
        }
    }

    // 0x0046B9E7
    static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        self->call_prepare_draw();
        *scrollWidth = 0x300;
        *scrollHeight = 0x300;
    }

    // 0x0046B97C
    static void event18(window* self, int16_t x, int16_t y, uint8_t scrollIndex)
    {
        registers regs;
        regs.esi = (int32_t)self;
        regs.ax = scrollIndex;
        regs.cx = x;
        regs.dx = y;

        call(0x0046B97C, regs);
    }

    // 0x0046B946
    static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
    {
        args.push(string_ids::tooltip_scroll_map);
    }

    // 0x0046D223
    static void leftAlignTabs(window* self, uint8_t firstTabIndex, uint8_t lastTabIndex)
    {
        auto disabledWidgets = self->disabled_widgets;
        disabledWidgets >>= firstTabIndex;
        auto pos = self->widgets[firstTabIndex].left;
        auto tabWidth = self->widgets[firstTabIndex].right - pos;

        for (auto index = firstTabIndex; index <= lastTabIndex; index++)
        {
            self->widgets[index].type = widget_type::none;
            if (!(disabledWidgets & (1ULL << index)))
            {
                self->widgets[index].type = widget_type::wt_8;
                self->widgets[index].left = pos;
                pos += tabWidth;
                self->widgets[index].right = pos;
                pos++;
            }
        }
    }

    // 0x0046B6BF
    static void prepareDraw(window* self)
    {
        //registers regs;
        //regs.esi = (int32_t)self;

        //call(0x0046B6BF, regs);

        const string_id captionText[] = {
            string_ids::title_map,
            string_ids::title_map_vehicles,
            string_ids::title_map_industries,
            string_ids::title_map_routes,
            string_ids::title_map_companies,
        };

        widgets[1].text = captionText[self->current_tab];
        auto activatedWidgets = self->activated_widgets;
        activatedWidgets &= ~((1ULL << statusBar) | (1ULL << scrollview) | (1ULL << tabOwnership) | (1ULL << tabRoutes) | (1ULL << tabIndustries) | (1ULL << tabVehicles) | (1ULL << tabOverall));

        auto currentWidget = self->current_tab + tabOverall;
        activatedWidgets |= (1ULL << currentWidget);
        self->activated_widgets = activatedWidgets;

        self->widgets[frame].right = self->x - 1;
        self->widgets[frame].bottom = self->y - 1;
        self->widgets[panel].right = self->x - 1;
        self->widgets[panel].bottom = self->y + 1;

        self->widgets[caption].right = self->x - 2;
        self->widgets[closeButton].left = self->x - 15;
        self->widgets[closeButton].right = self->x - 3;
        self->widgets[scrollview].bottom = self->y - 14;
        self->widgets[scrollview].right = self->x - 108;

        self->widgets[statusBar].top = self->y - 12;
        self->widgets[statusBar].bottom = self->y - 3;
        self->widgets[statusBar].right = self->x - 14;

        auto disabledWidgets = 0;
        if (is_editor_mode())
        {
            disabledWidgets |= (1 << tabVehicles) | (1 << tabRoutes) | (1 << tabOwnership);
        }
        self->disabled_widgets = disabledWidgets;

        leftAlignTabs(self, tabOverall, tabOwnership);
    }

    // 0x0046B779
    static void draw(window* self, gfx::drawpixelinfo_t* dpi)
    {
        registers regs;
        regs.esi = (int32_t)self;
        regs.edi = (int32_t)dpi;

        call(0x0046B779, regs);
    }

    // 0x0046B806
    static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        registers regs;
        regs.esi = (int32_t)self;
        regs.edi = (int32_t)dpi;
        regs.eax = scrollIndex;

        call(0x0046B806, regs);
    }

    static void initEvents()
    {
        events.on_close = onClose;
        events.on_mouse_up = onMouseUp;
        events.on_resize = onResize;
        events.on_update = onUpdate;
        events.get_scroll_size = getScrollSize;
        events.scroll_mouse_down = event18;
        events.scroll_mouse_over = event18;
        events.tooltip = tooltip;
        events.prepare_draw = prepareDraw;
        events.draw = draw;
        events.draw_scroll = drawScroll;
    }

    static uint32_t sub_406BF7()
    {
        registers regs;
        call(0x00406BF7, regs);

        return regs.eax;
    }

    static void sub_46CFF0()
    {
        registers regs;
        call(0x0046CFF0, regs);
    }
    static void sub_46CED0()
    {
        registers regs;
        call(0x0046CED0, regs);
    }

    // 0x0046B490
    void open()
    {
        //call(0x0046B490);

        auto window = WindowManager::bringToFront(WindowType::map, 0);

        if (window != nullptr)
            return;

        addr<0x00113E87C, int32_t>() = 3;
        auto eax = sub_406BF7();
        addr<0x00113E87C, int32_t>() = 0;
        if (!eax)
            return;

        _dword_F253A8 = eax;
        gfx::ui_size_t size = { _unk_110015E[0], _unk_110015E[1] };

        if (_dword_526284 != 0)
        {
            size = { _word_52628A, _word_526288 };
        }

        window = WindowManager::createWindow(WindowType::map, size, 0, &events);
        window->widgets = widgets;
        window->enabled_widgets |= enabledWidgets;
        window->init_scroll_widgets();
        window->frame_no = 0;

        if (_dword_526284 != 0)
        {
            window->var_88A = _word_52628C;
            window->var_88C = _word_52628E;
            window->flags = _dword_526284 & window_flags::flag_16;
        }

        initEvents();

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0F;

        window->var_846 = gCurrentRotation;

        sub_46B69C();

        center_on_view_point();

        window->current_tab = 0;
        window->saved_view.mapX = 1;
        window->var_854 = 0;
        window->var_856 = 0;

        sub_46CFF0();
        sub_46CED0();

        _word_F2541D = 0;
    }

    // 0x0046B5C0
    void center_on_view_point()
    {
        call(0x0046B5C0);
    }
}
