#include "../companymgr.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/string_ids.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../ui/WindowManager.h"
#include "../win32.h"
#include <cassert>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <shlobj.h>
#include <windows.h>

// `interface` is defined as a macro for `struct` in `windows.h`
#undef interface
#endif

using namespace openloco::interop;

namespace openloco::ui::textinput
{
    constexpr int16_t textboxPadding = 4;

    static int16_t _callingWidget;
    static window_number _callingWindowNumber;
    static loco_global<WindowType, 0x523364> _callingWindowType;

    static char _formatArgs[16];
    static string_id _title;
    static string_id _message;

    // txtutils
    static std::string _buffer;
    static int16_t _xOffset;     // 1136FA4
    static uint8_t _cursorFrame; // 0x011370A9

    size_t cursor_position;

    static loco_global<char[16], 0x0112C826> _commonFormatArgs;
    static loco_global<int32_t, 0x0112C876> _currentFontSpriteBase;

    static window_event_list _events;

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            input,
            ok,
        };
    }

    static widget_t _widgets[] = {
        make_widget({ 0, 0 }, { 330, 90 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 328, 13 }, widget_type::caption_25, 0),
        make_widget({ 315, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 15 }, { 330, 75 }, widget_type::panel, 1),
        make_widget({ 4, 58 }, { 322, 14 }, widget_type::wt_17, 1),
        make_text_widget({ 256, 74 }, { 70, 12 }, widget_type::wt_11, 1, string_ids::label_button_ok),
        widget_end(),
    };

    void register_hooks()
    {
        register_hook(
            0x004CE523,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                open_textinput((ui::window*)regs.esi, regs.ax, regs.bx, regs.cx, regs.dx, (void*)0x0112C836);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004CE6C9,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                sub_4CE6C9((WindowType)regs.cl, (window_number)regs.dx);
                regs = backup;
                return 0;
            });

        register_hook(
            0x004CE6F2,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                cancel();
                regs = backup;
                return 0;
            });
    }

    static void prepare_draw(ui::window* window);
    static void draw(ui::window* window, gfx::drawpixelinfo_t* context);
    static void on_mouse_up(ui::window* window, widget_index widgetIndex);
    static void on_update(ui::window* window);

    static bool needs_reoffsetting(int16_t containerWidth);
    static void calculate_text_offset(int16_t containerWidth);
    static void sanitize_input();

    /**
     * 0x004CE523
     *
     * @param caller @<esi>
     * @param title @<ax>
     * @param message @<bx>
     * @param value @<cx>
     * @param callingWidget @<dx>
     */
    void open_textinput(ui::window* caller, string_id title, string_id message, string_id value, int callingWidget, void* valueArgs)
    {
        _title = title;
        _message = message;

        _callingWindowType = caller->type;
        _callingWindowNumber = caller->number;
        _callingWidget = callingWidget;

        // Close any previous text input window
        cancel();

        memcpy(_formatArgs, _commonFormatArgs, 16);

        char temp[200];
        stringmgr::format_string(temp, value, valueArgs);
        _buffer = temp;

        _events.draw = draw;
        _events.prepare_draw = prepare_draw;
        _events.on_mouse_up = on_mouse_up;
        _events.on_update = on_update;

        auto window = WindowManager::createWindowCentred(
            WindowType::textInput,
            330,
            90,
            window_flags::stick_to_front | window_flags::flag_12,
            &_events);
        window->widgets = _widgets;
        window->enabled_widgets |= 1ULL << widx::close;
        window->enabled_widgets |= 1ULL << widx::ok;
        window->init_scroll_widgets();

        cursor_position = _buffer.length();
        _cursorFrame = 0;

        _xOffset = 0;
        calculate_text_offset(_widgets[widx::input].width() - 2);

        caller = WindowManager::find(_callingWindowType, _callingWindowNumber);

        window->colours[0] = caller->colours[0];
        window->colours[1] = caller->colours[1];
        window->var_884 = caller->var_884;

        if (caller->type == WindowType::titleMenu)
        {
            interface_skin_object* interface = objectmgr::get<interface_skin_object>();
            window->colours[0] = interface->colour_0B;
            window->colours[1] = interface->colour_0C;
            window->var_884 = -1;
        }

        if (caller->type == WindowType::timeToolbar)
        {
            interface_skin_object* interface = objectmgr::get<interface_skin_object>();
            window->colours[1] = interface->colour_0A;
            window->var_884 = companymgr::get_controlling_id();
        }

        _widgets[widx::title].type = widget_type::caption_25;
        if (window->var_884 != -1)
        {
            window->flags |= window_flags::flag_11;
            _widgets[widx::title].type = widget_type::caption_24;
        }
    }

    /**
     * 0x004CE6C9
     *
     * @param type @<cl>
     * @param number @<dx>
     */
    void sub_4CE6C9(WindowType type, window_number number)
    {
        auto window = WindowManager::find(WindowType::textInput, 0);
        if (window == nullptr)
            return;

        if (_callingWindowNumber == number && *_callingWindowType == type)
        {
            cancel();
        }
    }

    /**
     * 0x004CE6F2
     */
    void cancel()
    {
        WindowManager::close(WindowType::textInput);
    }

    /**
     * 0x004CE6FF
     */
    void sub_4CE6FF()
    {
        auto window = WindowManager::find(WindowType::textInput);
        if (window == nullptr)
            return;

        window = WindowManager::find(_callingWindowType, _callingWindowNumber);
        if (window == nullptr)
        {
            cancel();
        }
    }

    /**
     * 0x004CE726
     *
     * @param window @<esi>
     */
    static void prepare_draw(ui::window* window)
    {
        _widgets[widx::title].text = _title;
        memcpy(_commonFormatArgs, _formatArgs, 16);
    }

    /**
     * 0x004CE75B
     *
     * @param window @<esi>
     * @param context @<edi>
     */
    static void draw(ui::window* window, gfx::drawpixelinfo_t* context)
    {
        window->draw(context);

        *((string_id*)(&_commonFormatArgs[0])) = _message;
        memcpy(&_commonFormatArgs[2], _formatArgs, 8);

        gfx::point_t position = { (int16_t)(window->x + window->width / 2), (int16_t)(window->y + 30) };
        gfx::draw_string_centred_wrapped(context, &position, window->width - 8, 0, string_ids::wcolour2_stringid2, &_commonFormatArgs[0]);

        auto widget = &_widgets[widx::input];
        gfx::drawpixelinfo_t* clipped = nullptr;
        if (!gfx::clip_drawpixelinfo(&clipped, context, widget->left + 1 + window->x, widget->top + 1 + window->y, widget->width() - 2, widget->height() - 2))
        {
            return;
        }

        char* drawnBuffer = (char*)stringmgr::get_string(string_ids::buffer_2039);
        strcpy(drawnBuffer, _buffer.data());

        *((string_id*)(&_commonFormatArgs[0])) = string_ids::buffer_2039;

        position = { _xOffset, 1 };
        gfx::draw_string_494B3F(*clipped, &position, 0, string_ids::white_stringid2, _commonFormatArgs);

        if ((_cursorFrame % 32) >= 16)
        {
            return;
        }

        strncpy(drawnBuffer, _buffer.data(), cursor_position);
        drawnBuffer[cursor_position] = '\0';

        *((string_id*)(&_commonFormatArgs[0])) = string_ids::buffer_2039;
        position = { _xOffset, 1 };
        gfx::draw_string_494B3F(*clipped, &position, 0, string_ids::white_stringid2, _commonFormatArgs);
        gfx::fill_rect(clipped, position.x, position.y, position.x, position.y + 9, colour::get_shade(window->colours[1], 9));
    }

    // 0x004CE8B6
    static void on_mouse_up(ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window);
                break;
            case widx::ok:
                sanitize_input();
                auto caller = WindowManager::find(_callingWindowType, _callingWindowNumber);
                if (caller != nullptr)
                {
                    caller->call_text_input(_callingWidget, _buffer.data());
                }
                WindowManager::close(window);
                break;
        }
    }

    // 0x004CE8FA
    static void on_update(ui::window* window)
    {
        _cursorFrame++;
        if ((_cursorFrame % 16) == 0)
        {
            window->invalidate();
        }
    }

    void sub_4CE910(int eax, int ebx)
    {
        auto w = WindowManager::find(WindowType::textInput);
        if (w == nullptr)
        {
            return;
        }

        if ((eax >= VK_SPACE && eax < VK_F12) || (eax >= 159 && eax <= 255))
        {
            if (_buffer.length() == 199)
            {
                return;
            }

            if (cursor_position == _buffer.length())
            {
                _buffer.append(1, (char)eax);
            }
            else
            {
                _buffer.insert(cursor_position, 1, (char)eax);
            }
            cursor_position += 1;
        }
        else if (eax == VK_BACK)
        {
            if (cursor_position == 0)
            {
                // Cursor is at beginning
                return;
            }

            _buffer.erase(cursor_position - 1, 1);
            cursor_position -= 1;
        }
        else if (ebx == VK_DELETE)
        {
            if (cursor_position == _buffer.length())
            {
                return;
            }

            _buffer.erase(cursor_position, 1);
        }
        else if (eax == VK_RETURN)
        {
            w->call_on_mouse_up(widx::ok);
            return;
        }
        else if (eax == VK_ESCAPE)
        {
            w->call_on_mouse_up(widx::close);
            return;
        }
        else if (ebx == VK_HOME)
        {
            cursor_position = 0;
        }
        else if (ebx == VK_END)
        {
            cursor_position = _buffer.length();
        }
        else if (ebx == VK_LEFT)
        {
            if (cursor_position == 0)
            {
                // Cursor is at beginning
                return;
            }

            cursor_position -= 1;
        }
        else if (ebx == VK_RIGHT)
        {
            if (cursor_position == _buffer.length())
            {
                // Cursor is at end
                return;
            }

            cursor_position += 1;
        }

        WindowManager::invalidate(WindowType::textInput, 0);
        _cursorFrame = 0;

        int containerWidth = _widgets[widx::input].width() - 2;
        if (needs_reoffsetting(containerWidth))
        {
            calculate_text_offset(containerWidth);
        }
    }

    static bool needs_reoffsetting(int16_t containerWidth)
    {
        std::string cursorStr = _buffer.substr(0, cursor_position);

        _currentFontSpriteBase = font::medium_bold;
        auto stringWidth = gfx::get_string_width(_buffer.data());
        auto cursorX = gfx::get_string_width(cursorStr.data());

        int x = _xOffset + cursorX;

        if (x < textboxPadding)
            return true;

        if (x > containerWidth - textboxPadding)
            return true;

        if (_xOffset + stringWidth < containerWidth - textboxPadding)
            return true;

        return false;
    }

    /**
     * 0x004CEB67
     *
     * @param containerWidth @<edx>
     */
    static void calculate_text_offset(int16_t containerWidth)
    {
        std::string cursorStr = _buffer.substr(0, cursor_position);

        _currentFontSpriteBase = font::medium_bold;
        auto stringWidth = gfx::get_string_width(_buffer.data());
        auto cursorX = gfx::get_string_width(cursorStr.data());

        auto midX = containerWidth / 2;

        // Prefer to centre cursor
        _xOffset = -1 * (cursorX - midX);

        // Make sure that text will always be at the left edge
        int16_t minOffset = textboxPadding;
        int16_t maxOffset = textboxPadding;

        if (stringWidth + textboxPadding * 2 > containerWidth)
        {
            // Make sure that the whole textbox is filled up
            minOffset = -stringWidth + containerWidth - textboxPadding;
        }
        _xOffset = std::clamp<int16_t>(_xOffset, minOffset, maxOffset);
    }

    /**
     * 0x004CEBFB
     */
    static void sanitize_input()
    {
        _buffer.erase(
            std::remove_if(
                _buffer.begin(),
                _buffer.end(),
                [](unsigned char chr) {
                    if (chr < ' ')
                    {
                        return true;
                    }
                    else if (chr <= 'z')
                    {
                        return false;
                    }
                    else if (chr == 171)
                    {
                        return false;
                    }
                    else if (chr == 187)
                    {
                        return false;
                    }
                    else if (chr >= 191)
                    {
                        return false;
                    }

                    return true;
                }),
            _buffer.end());
    }
}
