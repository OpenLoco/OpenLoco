#include "../../audio/audio.h"
#include "../../graphics/colours.h"
#include "../../interop/interop.hpp"
#include "../../message.h"
#include "../../messagemgr.h"
#include "News.h"

using namespace openloco::interop;

namespace openloco::ui::NewsWindow::ticker
{
    widget_t widgets[] = {
        make_widget({ 0, 0 }, { 111, 26 }, widget_type::wt_3, 0),
        widget_end(),
    };

    window_event_list events;

    // 0x00429EA2
    static void on_mouse_up(window* self, widget_index widgetIndex)
    {
        if (widgetIndex != 0)
            return;

        if (_activeMessageIndex == 0xFFFF)
            return;

        auto news = messagemgr::get(_activeMessageIndex);
        news->var_C8 = 1;

        auto activeMessageIndex = _activeMessageIndex;
        _activeMessageIndex = 0xFFFF;

        WindowManager::close(self);
        open(activeMessageIndex);
    }

    // 0x00429FE4
    static void on_resize(window* self)
    {
        auto y = ui::height() - 25;
        auto x = ui::width() - 138;
        auto width = 111;
        auto height = 25;

        if (y != self->y || x != self->x || width != self->width || height != self->height)
        {
            self->invalidate();
            self->y = y;
            self->x = x;
            self->width = width;
            self->height = height;
            self->invalidate();
        }
    }

    // 0x00429EEB
    static void on_update(window* self)
    {
        auto window = WindowManager::findAtAlt(_cursorX2, _cursorY2);

        if (window == self)
        {
            self->var_852 = 12;
        }

        if (self->var_852 != 0)
        {
            if (!is_paused())
            {
                self->var_852--;
            }
        }

        self->invalidate();

        if (self->var_852 == 0)
        {
            if (!is_paused())
            {
                _word_525CE0 = _word_525CE0 + 2;

                if (!(_word_525CE0 & 0x8007))
                {
                    if (_activeMessageIndex != 0xFFFF)
                    {
                        auto news = messagemgr::get(_activeMessageIndex);
                        auto cx = _word_525CE0 >> 2;
                        char* newsString = news->messageString;
                        auto newsStringChar = *newsString;

                        while (true)
                        {
                            newsStringChar = *newsString;
                            if (newsStringChar == control_codes::newline)
                            {
                                newsStringChar = 32;
                                cx--;
                                if (cx < 0)
                                    break;
                            }

                            if (newsStringChar != -1)
                            {
                                cx--;
                                if (cx < 0)
                                    break;
                                newsString++;
                                if (!newsStringChar)
                                    break;
                            }
                            else
                            {
                                cx--;
                                if (cx < 0)
                                    break;
                                newsString += 3;
                            }
                        }

                        if (newsStringChar != 32)
                        {
                            if (newsStringChar != 0)
                            {
                                audio::play_sound(audio::sound_id::ticker, ui::width());
                            }
                        }
                    }
                }
            }
        }

        if (_activeMessageIndex != 0xFFFF)
            return;

        _activeMessageIndex = 0xFFFF;

        WindowManager::close(self);
    }

    static void sub_4950EF(gfx::drawpixelinfo_t* clipped, string_id buffer, uint32_t eax, uint32_t ebp, int16_t x, int16_t y)
    {
        registers regs;
        regs.bx = buffer;
        regs.eax = eax;
        regs.cx = x;
        regs.dx = y;
        regs.ebp = ebp;
        regs.edi = (int32_t)clipped;
        call(0x004950EF, regs);
    }

    // 0x00429DAA
    static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
    {
        if (self->var_852 != 0)
            return;

        if (get_pause_flags() & 4)
            return;

        auto news = messagemgr::get(_activeMessageIndex);

        auto x = self->x;
        auto y = self->y;
        auto width = self->width;
        auto height = self->height;
        gfx::drawpixelinfo_t* clipped = nullptr;

        gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height);

        if (clipped == nullptr)
            return;

        uint32_t colour = 0x14141414;

        if (!(_word_4F8BE4[news->var_00] & (1 << 1)))
        {
            colour = colour::translucent(colour::inset(colour::icy_blue));
            colour *= 0x1010101;
        }

        gfx::clear(*clipped, colour);

        char* newsString = news->messageString;
        auto buffer = const_cast<char*>(stringmgr::get_string(string_ids::buffer_2039));

        *buffer = control_codes::colour_black;
        buffer++;
        *buffer = control_codes::font_small;
        buffer++;

        auto newsStringChar = *newsString;
        auto i = 0;

        while (true)
        {
            newsStringChar = *newsString;
            if (newsStringChar == control_codes::newline)
            {
                newsStringChar = 32;
                *buffer = newsStringChar;
                buffer++;
            }

            if (newsStringChar == -1)
            {
                *buffer = newsStringChar;
                buffer++;
                newsString++;
                *buffer = *newsString;
                buffer++;
                newsString++;
                newsStringChar = *newsString;
            }

            *buffer = newsStringChar;
            buffer++;
            newsString++;
            i++;
            if (!newsStringChar)
                break;
        }

        if ((_word_525CE0 >> 2) > i)
        {
            _word_525CE0 = _word_525CE0 | (1 << 15);
        }
        uint32_t ebp = (((_word_525CE0 & ~(1 << 15)) >> 2) << 16) | 109;

        sub_4950EF(clipped, string_ids::buffer_2039, (1 << 18), ebp, 55, 0);
    }

    void initEvents()
    {
        events.on_mouse_up = on_mouse_up;
        events.on_resize = on_resize;
        events.on_update = on_update;
        events.draw = draw;
    }
}
