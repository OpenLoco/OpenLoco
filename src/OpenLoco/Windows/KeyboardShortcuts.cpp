#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input/ShortcutManager.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"
#include <SDL2/SDL.h>
#include <unordered_map>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Input;

namespace OpenLoco::Ui::Windows::KeyboardShortcuts
{
    static const int rowHeight = 10; // CJK: 13

    static WindowEventList _events;

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 360, 238 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 358, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::keyboard_shortcuts),
        makeWidget({ 345, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 360, 223 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 4, 19 }, { 352, 202 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical, StringIds::keyboard_shortcut_list_tip),
        makeWidget({ 4, 223 }, { 150, 12 }, WidgetType::button, WindowColour::secondary, StringIds::reset_keys, StringIds::reset_keys_tip),
        widgetEnd(),
    };

    namespace Widx
    {
        enum
        {
            frame,
            caption,
            close_button,
            panel,
            list,
            reset_keys_btn,
        };
    }

    static void draw(Ui::Window* self, Gfx::Context* context);
    static void drawScroll(Ui::Window& self, Gfx::Context& context, const uint32_t scrollIndex);
    static void onMouseUp(Window* self, WidgetIndex_t widgetIndex);
    static void resetShortcuts(Window* self);
    static std::optional<FormatArguments> tooltip(Window*, WidgetIndex_t);
    static void getScrollSize(Ui::Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void onScrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index);
    static void onScrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index);

    static void initEvents()
    {
        _events.onMouseUp = onMouseUp;
        _events.getScrollSize = getScrollSize;
        _events.scrollMouseDown = onScrollMouseDown;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.draw = draw;
        _events.drawScroll = drawScroll;
    }

    // 0x004BE6C7
    Window* open()
    {
        Window* window;

        window = WindowManager::bringToFront(WindowType::keyboardShortcuts, 0);
        if (window != nullptr)
            return window;

        initEvents();

        // 0x004BF833 (create_options_window)
        window = WindowManager::createWindowCentred(WindowType::keyboardShortcuts, { 360, 238 }, 0, &_events);

        window->widgets = _widgets;
        window->enabledWidgets = (1 << Widx::close_button) | (1 << Widx::reset_keys_btn);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_10);

        window->rowCount = static_cast<uint16_t>(ShortcutManager::kCount);
        window->rowHover = -1;

        return window;
    }

    // 0x004BE726
    static void draw(Ui::Window* self, Gfx::Context* context)
    {
        // Draw widgets.
        self->draw(context);
    }

    static void getBindingString(uint32_t keyCode, char* buffer, const size_t bufferLength)
    {
        static const std::unordered_map<uint32_t, string_id> keysToString = { {
            { SDLK_BACKSPACE, StringIds::keyboard_backspace },
            { SDLK_TAB, StringIds::keyboard_tab },
            { SDLK_RETURN, StringIds::keyboard_return },
            { SDLK_PAUSE, StringIds::keyboard_pause },
            { SDLK_CAPSLOCK, StringIds::keyboard_caps },
            { SDLK_ESCAPE, StringIds::keyboard_escape },
            { SDLK_SPACE, StringIds::keyboard_spacebar },
            { SDLK_PAGEUP, StringIds::keyboard_pageup },
            { SDLK_PAGEDOWN, StringIds::keyboard_pagedown },
            { SDLK_END, StringIds::keyboard_end },
            { SDLK_HOME, StringIds::keyboard_home },
            { SDLK_LEFT, StringIds::keyboard_left },
            { SDLK_UP, StringIds::keyboard_up },
            { SDLK_RIGHT, StringIds::keyboard_right },
            { SDLK_DOWN, StringIds::keyboard_down },
            { SDLK_INSERT, StringIds::keyboard_insert },
            { SDLK_DELETE, StringIds::keyboard_delete },
            { SDLK_KP_1, StringIds::keyboard_numpad_1 },
            { SDLK_KP_2, StringIds::keyboard_numpad_2 },
            { SDLK_KP_3, StringIds::keyboard_numpad_3 },
            { SDLK_KP_4, StringIds::keyboard_numpad_4 },
            { SDLK_KP_5, StringIds::keyboard_numpad_5 },
            { SDLK_KP_6, StringIds::keyboard_numpad_6 },
            { SDLK_KP_7, StringIds::keyboard_numpad_7 },
            { SDLK_KP_8, StringIds::keyboard_numpad_8 },
            { SDLK_KP_9, StringIds::keyboard_numpad_9 },
            { SDLK_KP_0, StringIds::keyboard_numpad_0 },
            { SDLK_KP_DIVIDE, StringIds::keyboard_numpad_divide },
            { SDLK_KP_ENTER, StringIds::keyboard_numpad_enter },
            { SDLK_KP_MINUS, StringIds::keyboard_numpad_minus },
            { SDLK_KP_MULTIPLY, StringIds::keyboard_numpad_multiply },
            { SDLK_KP_PERIOD, StringIds::keyboard_numpad_period },
            { SDLK_KP_PLUS, StringIds::keyboard_numpad_plus },
            { SDLK_NUMLOCKCLEAR, StringIds::keyboard_numlock },
            { SDLK_SCROLLLOCK, StringIds::keyboard_scroll },
        } };

        auto match = keysToString.find(keyCode);
        if (match != keysToString.end())
        {
            StringManager::formatString(buffer, match->second);
        }
        else
        {
            const char* sdlBuffer = SDL_GetKeyName(keyCode);
            strncpy(buffer, sdlBuffer, bufferLength - 1);
        }
    }

    // 0x004BE72C
    static void drawScroll(Ui::Window& self, Gfx::Context& context, const uint32_t scrollIndex)
    {
        auto colour = self.getColour(WindowColour::secondary).c();
        auto shade = Colours::getShade(colour, 4);
        Gfx::clearSingle(context, shade);

        const auto& shortcuts = Config::getNew().shortcuts;
        auto yPos = 0;
        for (auto i = 0; i < self.rowCount; i++)
        {
            string_id format = StringIds::black_stringid;
            if (i == self.rowHover)
            {
                Gfx::drawRect(context, 0, yPos, 800, rowHeight, 0x2000030);
                format = StringIds::wcolour2_stringid;
            }

            auto modifierStringId = StringIds::empty;
            auto baseStringId = StringIds::empty;
            char buffer[128]{};

            if (shortcuts[i].keyCode != 0xFFFFFFFF)
            {
                if (shortcuts[i].modifiers == 1)
                    modifierStringId = StringIds::keyboard_shortcut_modifier_shift;
                else if (shortcuts[i].modifiers != 0)
                    modifierStringId = StringIds::keyboard_shortcut_modifier_ctrl;

                baseStringId = StringIds::stringptr;
                getBindingString(shortcuts[i].keyCode, buffer, std::size(buffer));
            }

            auto formatter = FormatArguments::common();
            formatter.push(StringIds::keyboard_shortcut_list_format);
            formatter.push(ShortcutManager::getName(static_cast<Shortcut>(i)));
            formatter.push(modifierStringId);
            formatter.push(baseStringId);
            formatter.push(buffer);

            Gfx::drawString_494B3F(context, 0, yPos - 1, Colour::black, format, &formatter);
            yPos += rowHeight;
        }
    }

    // 0x004BE821
    static void onMouseUp(Window* self, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::close_button:
                WindowManager::close(self);
                return;

            case Widx::reset_keys_btn:
                resetShortcuts(self);
                return;
        }
    }

    // 0x004BE832
    static void resetShortcuts(Window* self)
    {
        Config::resetShortcuts();
        self->invalidate();
    }

    // 0x004BE844
    static std::optional<FormatArguments> tooltip(Window*, WidgetIndex_t)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }

    // 0x004BE84E
    static void getScrollSize(Ui::Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = self->rowCount * rowHeight;
    }

    // 0x004BE853
    static void onScrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / rowHeight;

        if (row >= self->rowCount)
            return;

        if (row != self->rowHover)
        {
            self->rowHover = row;
            self->invalidate();
        }
    }

    // 0x004BE87B
    static void onScrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / rowHeight;

        if (row >= self->rowCount)
            return;

        EditKeyboardShortcut::open(row);
    }
}
