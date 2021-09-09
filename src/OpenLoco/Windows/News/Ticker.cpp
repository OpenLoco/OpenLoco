#include "../../Audio/Audio.h"
#include "../../Graphics/Colour.h"
#include "../../Interop/Interop.hpp"
#include "../../Localisation/StringIds.h"
#include "../../Message.h"
#include "../../MessageManager.h"
#include "../../Widget.h"
#include "News.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::NewsWindow::Ticker
{
    Widget widgets[] = {
        makeWidget({ 0, 0 }, { 111, 26 }, WidgetType::wt_3, WindowColour::primary),
        widgetEnd(),
    };

    WindowEventList events;

    // 0x00429EA2
    static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
    {
        if (widgetIndex != 0)
            return;

        if (_activeMessageIndex == MessageId::null)
            return;

        auto news = MessageManager::get(_activeMessageIndex);
        news->var_C8 = 1;

        auto activeMessageIndex = _activeMessageIndex;
        _activeMessageIndex = MessageId::null;

        WindowManager::close(self);
        open(activeMessageIndex);
    }

    // 0x00429FE4
    static void onResize(Window* self)
    {
        auto y = Ui::height() - windowSize.height + 1;
        auto x = Ui::width() - windowSize.width - 27;
        auto height = windowSize.height - 1;

        if (y != self->y || x != self->x || windowSize.width != self->width || height != self->height)
        {
            self->invalidate();
            self->y = y;
            self->x = x;
            self->width = windowSize.width;
            self->height = height;
            self->invalidate();
        }
    }

    // 0x00429EEB
    static void onUpdate(Window* self)
    {
        auto window = WindowManager::findAtAlt(_cursorX2, _cursorY2);

        if (window == self)
        {
            self->var_852 = 12;
        }

        if (self->var_852 != 0)
        {
            if (!isPaused())
            {
                self->var_852--;
            }
        }

        self->invalidate();

        if (self->var_852 == 0 && !isPaused())
        {
            _word_525CE0 = _word_525CE0 + 2;

            if (!(_word_525CE0 & 0x8007))
            {
                if (_activeMessageIndex != MessageId::null)
                {
                    auto news = MessageManager::get(_activeMessageIndex);
                    auto cx = _word_525CE0 >> 2;
                    char* newsString = news->messageString;
                    auto newsStringChar = *newsString;

                    while (true)
                    {
                        newsStringChar = *newsString;
                        if (newsStringChar == ControlCodes::newline)
                        {
                            newsStringChar = ' ';
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

                    if (newsStringChar != ' ')
                    {
                        if (newsStringChar != 0)
                        {
                            Audio::playSound(Audio::SoundId::ticker, Ui::width());
                        }
                    }
                }
            }
        }

        if (_activeMessageIndex != MessageId::null)
            return;

        _activeMessageIndex = MessageId::null;

        WindowManager::close(self);
    }

    // 0x004950EF
    static void sub_4950EF(Gfx::Context* clipped, string_id buffer, uint32_t eax, uint32_t ebp, int16_t x, int16_t y)
    {
        registers regs;
        regs.bx = buffer;
        regs.eax = eax;
        regs.cx = x;
        regs.dx = y;
        regs.ebp = ebp;
        regs.edi = X86Pointer(clipped);
        call(0x004950EF, regs);
    }

    // 0x00429DAA
    static void draw(Ui::Window* self, Gfx::Context* context)
    {
        if (self->var_852 != 0)
            return;

        if (getPauseFlags() & (1 << 2))
            return;

        auto news = MessageManager::get(_activeMessageIndex);

        auto x = self->x;
        auto y = self->y;
        auto width = self->width;
        auto height = self->height;

        auto clipped = Gfx::clipContext(*context, { x, y, width, height });

        if (!clipped)
            return;

        auto colour = Colour::getShade(Colour::white, 5);

        if (!(_word_4F8BE4[news->type] & (1 << 1)))
        {
            colour = Colour::getShade(Colour::salmon_pink, 5);
        }

        Gfx::clearSingle(*clipped, colour);

        char* newsString = news->messageString;
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

        *buffer = ControlCodes::colour_black;
        buffer++;
        *buffer = ControlCodes::font_small;
        buffer++;

        auto newsStringChar = *newsString;
        auto i = 0;

        while (true)
        {
            newsStringChar = *newsString;
            if (newsStringChar == ControlCodes::newline)
            {
                newsStringChar = ' ';
                *buffer = newsStringChar;
                buffer++;
            }

            if (newsStringChar == -1)
            {
                *buffer++ = *newsString++;
                *buffer++ = *newsString++;
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

        sub_4950EF(&*clipped, StringIds::buffer_2039, (1 << 18), ebp, 55, 0);
    }

    void initEvents()
    {
        events.on_mouse_up = onMouseUp;
        events.on_resize = onResize;
        events.on_update = onUpdate;
        events.draw = draw;
    }
}
