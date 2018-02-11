#include "window.h"
#include "console.h"
#include "graphics/colours.h"
#include "input.h"
#include "interop/interop.hpp"
#include "widget.h"
#include <cassert>
#include <cinttypes>

using namespace openloco;
using namespace openloco::interop;

namespace openloco::ui
{
    template<typename T>
    static bool is_interop_event(T e)
    {
        return (uint32_t)e < 0x004D7000;
    }

    // 0x004CA4BD
    void window::invalidate()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x004CA4BD, regs);
    }

    void window::sub_4CA17F()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call(0x004CA17F, regs);
    }

    // 0x004C9513
    int16_t window::find_widget_at(int16_t xPos, int16_t yPos)
    {
        registers regs;
        regs.ax = xPos;
        regs.bx = yPos;
        regs.esi = (int32_t)this;
        call(0x004C9513, regs);

        return regs.dx;
    }

    void window::call_update()
    {
        registers regs;
        regs.esi = (int32_t)this;
        call((uint32_t)this->event_handlers->on_update, regs);
    }

    ui::cursor_id window::call_15(int16_t xPos, int16_t yPos, ui::cursor_id fallback, bool* out)
    {
        registers regs;
        regs.ax = xPos;
        regs.bl = *out;
        regs.cx = yPos;
        regs.edi = (int32_t)fallback;
        regs.esi = (int32_t)this;
        call(this->event_handlers->event_15, regs);

        *out = regs.bl;

        return (cursor_id)regs.edi;
    }

    ui::cursor_id window::call_cursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        registers regs;
        regs.cx = xPos;
        regs.dx = yPos;
        regs.ax = widgetIdx;
        regs.ebx = -1;
        regs.edi = (int32_t) & this->widgets[widgetIdx];
        regs.esi = (int32_t)this;
        call(this->event_handlers->cursor, regs);

        if (regs.ebx == -1)
        {
            return fallback;
        }

        return (cursor_id)regs.ebx;
    }

    bool window::call_tooltip(int16_t widget_index)
    {
        registers regs;
        regs.ax = widget_index;
        regs.esi = (int32_t)this;
        call((int32_t)this->event_handlers->tooltip, regs);
        return regs.ax != (int16_t)string_ids::null;
    }

    void window::call_prepare_draw()
    {
        if (event_handlers->prepare_draw != nullptr)
        {
            if (is_interop_event(event_handlers->prepare_draw))
            {
                registers regs;
                regs.esi = (int32_t)this;
                call((int32_t)this->event_handlers->prepare_draw, regs);
            }
            else
            {
                event_handlers->prepare_draw(this);
            }
        }
    }

    void window::call_draw(gfx::drawpixelinfo_t* dpi)
    {
        if (event_handlers->draw != nullptr)
        {
            if (is_interop_event(this->event_handlers->draw))
            {
                registers regs;
                regs.esi = (int32_t)this;
                regs.edi = (int32_t)dpi;
                call((int32_t)this->event_handlers->draw, regs);
            }
            else
            {
                event_handlers->draw(this, dpi);
            }
        }
    }

    // 0x004CA4DF
    void window::draw(gfx::drawpixelinfo_t* dpi)
    {
        if ((this->flags & window_flags::flag_4) && !(this->flags & window_flags::flag_5))
        {
            gfx::fill_rect(dpi, this->x, this->y, this->x + this->width - 1, this->y + this->height - 1, 0x2000000 | 52);
        }

        uint64_t pressed_widget = 0;
        if (input::state() == input::input_state::dropdown_active || input::state() == input::input_state::widget_pressed)
        {
            if (this->type == addr<0x0052336F, window_type>() && this->number == addr<0x00523370, uint16_t>())
            {
                if (input::has_flag((input::input_flags)(1 << 0)))
                {
                    pressed_widget = 1ULL << addr<0x00523372, uint32_t>();
                }
            }
        }

        uint64_t tool_widget = 0;
        if (this->type == addr<0x00523392, window_type>() && this->number == addr<0x00523390, uint16_t>())
        {
            tool_widget = 1ULL << addr<0x00523394, uint32_t>();
        }

        uint64_t hovered_widget = 0;
        if (this->type == addr<0x005233A8, window_type>() && this->number == addr<0x005233AA, uint16_t>())
        {
            hovered_widget = 1ULL << addr<0x005233AC, uint16_t>();
        }

        int scrollviewIndex = 0;
        for (int widgetIndex = 0; widgetIndex < 64; widgetIndex++)
        {
            auto widget = &this->widgets[widgetIndex];

            if (widget->type == widget_type::end)
            {
                break;
            }

            if ((this->flags & window_flags::flag_5) == 0)
            {
                // Check if widget is outside the draw region
                if (this->x + widget->left >= dpi->x + dpi->width && this->x + widget->right < dpi->x)
                {
                    if (this->y + widget->top >= dpi->y + dpi->height && this->y + widget->bottom < dpi->y)
                    {
                        continue;
                    }
                }
            }

            uint16_t widgetFlags = 0;
            if (widget->colour == 0 && this->flags & window_flags::flag_11)
            {
                widgetFlags = 0x80;
            }

            uint8_t colour = this->colours[widget->colour];

            bool enabled = (this->enabled_widgets & (1ULL << widgetIndex)) != 0;
            bool disabled = (this->disabled_widgets & (1ULL << widgetIndex)) != 0;
            bool activated = (this->activated_widgets & (1ULL << widgetIndex)) != 0;
            activated |= (pressed_widget & (1ULL << widgetIndex)) != 0;
            activated |= (tool_widget & (1ULL << widgetIndex)) != 0;
            bool hovered = (hovered_widget & (1ULL << widgetIndex)) != 0;

            switch (widget->type)
            {
                case widget_type::none:
                case widget_type::end:
                    break;

                case widget_type::panel:
                    widget::draw_1(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::frame:
                    widget::draw_2(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::wt_3:
                    widget::draw_3(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    break;

                case widget_type::wt_4:
                    assert(false); // Unused
                    break;

                case widget_type::wt_5:
                case widget_type::wt_6:
                case widget_type::wt_7:
                case widget_type::wt_8:
                    widget::draw_5(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    break;

                case widget_type::wt_9:
                    widget::draw_9(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated, hovered);
                    break;

                case widget_type::wt_10:
                    widget::draw_10(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated, hovered);
                    break;

                case widget_type::wt_11:
                case widget_type::wt_12:
                case widget_type::wt_14:
                    if (widget->type == widget_type::wt_12)
                    {
                        assert(false); // Unused
                    }
                    widget::draw_11_a(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    widget::draw_13(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    break;

                case widget_type::wt_13:
                    widget::draw_13(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    break;

                case widget_type::wt_15:
                case widget_type::wt_16:
                    assert(false); // Unused
                    break;

                case widget_type::wt_17:
                case widget_type::wt_18:
                case widget_type::wt_19:
                    widget::draw_17(dpi, this, widget, widgetFlags, colour);
                    widget::draw_15(dpi, this, widget, widgetFlags, colour, disabled);
                    break;

                case widget_type::wt_20:
                case widget_type::wt_21:
                    assert(false); // Unused
                    break;

                case widget_type::caption_22:
                    widget::draw_22_caption(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::caption_23:
                    widget::draw_23_caption(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::caption_24:
                    widget::draw_24_caption(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::caption_25:
                    widget::draw_25_caption(dpi, this, widget, widgetFlags, colour);
                    break;

                case widget_type::scrollview:
                    widget::draw_26(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated, hovered, scrollviewIndex);
                    scrollviewIndex++;
                    break;

                case widget_type::checkbox:
                    widget::draw_27_checkbox(dpi, this, widget, widgetFlags, colour, enabled, disabled, activated);
                    widget::draw_27_label(dpi, this, widget, widgetFlags, colour, disabled);
                    break;

                case widget_type::wt_28:
                    assert(false); // Unused
                    widget::draw_27_label(dpi, this, widget, widgetFlags, colour, disabled);
                    break;

                case widget_type::wt_29:
                    assert(false); // Unused
                    widget::draw_29(dpi, this, widget);
                    break;
            }
        }

        if (this->flags & window_flags::white_border_mask)
        {
            gfx::fill_rect_inset(
                dpi,
                this->x,
                this->y,
                this->x + this->width - 1,
                this->y + this->height - 1,
                colour::white,
                0x10);
        }
    }
}
