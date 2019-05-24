#include "scrollview.h"
#include "../interop/interop.hpp"
#include "../ui.h"
#include "WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::scrollview
{
    static loco_global<uint16_t, 0x00523396> _currentScrollArea;
    // TODO: Convert to a scrollIndex when all scroll functions implemented
    static loco_global<uint32_t, 0x00523398> _currentScrollOffset;

    // 0x004C8EF0
    void get_part(
        ui::window* window,
        ui::widget_t* widget,
        int16_t x,
        int16_t y,
        int16_t* output_x,
        int16_t* output_y,
        scroll_part* output_scroll_area,
        int32_t* scroll_id)
    {
        registers regs;
        regs.ax = x;
        regs.bx = y;
        regs.esi = (uint32_t)window;
        regs.edi = (uint32_t)widget;

        call(0x004C8EF0, regs);

        *output_x = regs.ax;
        *output_y = regs.bx;
        *output_scroll_area = (scroll_part)regs.cx;
        *scroll_id = regs.edx;
    }

    // 0x004CA1ED
    void update_thumbs(window* window, widget_index widgetIndex)
    {
        registers regs;

        regs.esi = (uintptr_t)window;
        regs.ebx = window->get_scroll_data_index(widgetIndex) * sizeof(scroll_area_t);
        regs.edi = (uintptr_t)&window->widgets[widgetIndex];
        call(0x4CA1ED, regs);
    }

    // 0x004C894F
    static void hButtonLeft(ui::window* const w, const int16_t scrollAreaIndex, const widget_index widgetIndex)
    {
        w->scroll_areas[scrollAreaIndex].flags |= scroll_flags::HSCROLLBAR_LEFT_PRESSED;
        w->scroll_areas[scrollAreaIndex].contentOffsetX = std::max(w->scroll_areas[scrollAreaIndex].contentOffsetX - SCROLLBAR_BUTTON_CLICK_STEP, 0);
        scrollview::update_thumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C89AE
    static void hButtonRight(ui::window* const w, const int16_t scrollAreaIndex, const widget_index widgetIndex)
    {
        w->scroll_areas[scrollAreaIndex].flags |= scroll_flags::HSCROLLBAR_RIGHT_PRESSED;
        int16_t widgetWidth = w->widgets[widgetIndex].width() - 2;
        if ((w->scroll_areas[scrollAreaIndex].flags & scroll_flags::VSCROLLBAR_VISIBLE) != 0)
        {
            widgetWidth -= SCROLLBAR_WIDTH + 1;
        }
        int16_t widgetContentWidth = std::max(w->scroll_areas[scrollAreaIndex].contentWidth - widgetWidth, 0);
        w->scroll_areas[scrollAreaIndex].contentOffsetX = std::min<int16_t>(w->scroll_areas[scrollAreaIndex].contentOffsetX + SCROLLBAR_BUTTON_CLICK_STEP, widgetContentWidth);
        scrollview::update_thumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8A36
    static void hTrackLeft(ui::window* const w, const int16_t scrollAreaIndex, const widget_index widgetIndex)
    {
        int16_t widgetWidth = w->widgets[widgetIndex].width() - 2;
        if ((w->scroll_areas[scrollAreaIndex].flags & scroll_flags::VSCROLLBAR_VISIBLE) != 0)
        {
            widgetWidth -= SCROLLBAR_WIDTH + 1;
        }
        w->scroll_areas[scrollAreaIndex].contentOffsetX = std::max(w->scroll_areas[scrollAreaIndex].contentOffsetX - widgetWidth, 0);
        scrollview::update_thumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8AA6
    static void hTrackRight(ui::window* const w, const int16_t scrollAreaIndex, const widget_index widgetIndex)
    {
        int16_t widgetWidth = w->widgets[widgetIndex].width() - 2;
        if ((w->scroll_areas[scrollAreaIndex].flags & scroll_flags::VSCROLLBAR_VISIBLE) != 0)
        {
            widgetWidth -= SCROLLBAR_WIDTH + 1;
        }
        int16_t widgetContentWidth = std::max(w->scroll_areas[scrollAreaIndex].contentWidth - widgetWidth, 0);
        w->scroll_areas[scrollAreaIndex].contentOffsetX = std::min<int16_t>(w->scroll_areas[scrollAreaIndex].contentOffsetX + widgetWidth, widgetContentWidth);
        scrollview::update_thumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8B26
    static void vButtonTop(ui::window* const w, const int16_t scrollAreaIndex, const widget_index widgetIndex)
    {
        w->scroll_areas[scrollAreaIndex].flags |= scroll_flags::VSCROLLBAR_UP_PRESSED;
        w->scroll_areas[scrollAreaIndex].v_top = std::max(w->scroll_areas[scrollAreaIndex].v_top - SCROLLBAR_BUTTON_CLICK_STEP, 0);
        scrollview::update_thumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8B85
    static void vButtonBottom(ui::window* const w, const int16_t scrollAreaIndex, const widget_index widgetIndex)
    {
        w->scroll_areas[scrollAreaIndex].flags |= scroll_flags::VSCROLLBAR_DOWN_PRESSED;
        int16_t widgetHeight = w->widgets[widgetIndex].height() - 2;
        if ((w->scroll_areas[scrollAreaIndex].flags & scroll_flags::HSCROLLBAR_VISIBLE) != 0)
        {
            widgetHeight -= SCROLLBAR_WIDTH + 1;
        }
        int16_t widgetContentHeight = std::max(w->scroll_areas[scrollAreaIndex].v_bottom - widgetHeight, 0);
        w->scroll_areas[scrollAreaIndex].v_top = std::min<int16_t>(w->scroll_areas[scrollAreaIndex].v_top + SCROLLBAR_BUTTON_CLICK_STEP, widgetContentHeight);
        scrollview::update_thumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8C0D
    static void vTrackTop(ui::window* const w, const int16_t scrollAreaIndex, const widget_index widgetIndex)
    {
        int16_t widgetHeight = w->widgets[widgetIndex].height() - 2;
        if ((w->scroll_areas[scrollAreaIndex].flags & scroll_flags::HSCROLLBAR_VISIBLE) != 0)
        {
            widgetHeight -= SCROLLBAR_WIDTH + 1;
        }
        w->scroll_areas[scrollAreaIndex].v_top = std::max(w->scroll_areas[scrollAreaIndex].v_top - widgetHeight, 0);
        scrollview::update_thumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8C7D
    static void vTrackBottom(ui::window* const w, const int16_t scrollAreaIndex, const widget_index widgetIndex)
    {
        int16_t widgetHeight = w->widgets[widgetIndex].height() - 2;
        if ((w->scroll_areas[scrollAreaIndex].flags & scroll_flags::HSCROLLBAR_VISIBLE) != 0)
        {
            widgetHeight -= SCROLLBAR_WIDTH + 1;
        }
        int16_t widgetContentHeight = std::max(w->scroll_areas[scrollAreaIndex].v_bottom - widgetHeight, 0);
        w->scroll_areas[scrollAreaIndex].v_top = std::min<int16_t>(w->scroll_areas[scrollAreaIndex].v_top + widgetHeight, widgetContentHeight);
        scrollview::update_thumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8689
    void scrollLeftBegin(const int16_t x, const int16_t y, ui::window* const w, ui::widget_t* const widget, const widget_index widgetIndex)
    {
        ui::scrollview::scroll_part scrollArea;
        int16_t outX, outY;
        int32_t scrollAreaOffset;

        ui::scrollview::get_part(w, widget, x, y, &outX, &outY, &scrollArea, &scrollAreaOffset);

        _currentScrollArea = (uint16_t)scrollArea;
        _currentScrollOffset = scrollAreaOffset;
        int16_t scrollAreaIndex = scrollAreaOffset / sizeof(ui::scroll_area_t);

        // Not implemented for any window
        // window->call_22()
        switch (scrollArea)
        {
            case ui::scrollview::scroll_part::view:
                w->call_scroll_mouse_down(outX, outY, scrollAreaIndex);
                break;
            case ui::scrollview::scroll_part::hscrollbar_button_left:
                hButtonLeft(w, scrollAreaIndex, widgetIndex);
                break;
            case ui::scrollview::scroll_part::hscrollbar_button_right:
                hButtonRight(w, scrollAreaIndex, widgetIndex);
                break;
            case ui::scrollview::scroll_part::hscrollbar_track_left:
                hTrackLeft(w, scrollAreaIndex, widgetIndex);
                break;
            case ui::scrollview::scroll_part::hscrollbar_track_right:
                hTrackRight(w, scrollAreaIndex, widgetIndex);
                break;
            case ui::scrollview::scroll_part::vscrollbar_button_top:
                vButtonTop(w, scrollAreaIndex, widgetIndex);
                break;
            case ui::scrollview::scroll_part::vscrollbar_button_bottom:
                vButtonBottom(w, scrollAreaIndex, widgetIndex);
                break;
            case ui::scrollview::scroll_part::vscrollbar_track_top:
                vTrackTop(w, scrollAreaIndex, widgetIndex);
                break;
            case ui::scrollview::scroll_part::vscrollbar_track_bottom:
                vTrackBottom(w, scrollAreaIndex, widgetIndex);
                break;
            default:
                break;
        }
    }

    // Based on 0x004C8689
    void scrollModalRight(const int16_t x, const int16_t y, ui::window* const w, ui::widget_t* const widget, const widget_index widgetIndex)
    {
        ui::scrollview::scroll_part scrollArea;
        int16_t outX, outY;
        int32_t scrollAreaOffset;

        ui::scrollview::get_part(w, widget, x, y, &outX, &outY, &scrollArea, &scrollAreaOffset);

        _currentScrollArea = (uint16_t)scrollArea;
        _currentScrollOffset = scrollAreaOffset;
        int16_t scrollAreaIndex = scrollAreaOffset / sizeof(ui::scroll_area_t);

        if (scrollArea == ui::scrollview::scroll_part::view)
        {
            w->call_scroll_mouse_down(outX, outY, scrollAreaIndex);
        }
    }
}
