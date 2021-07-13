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

using namespace OpenLoco::Interop;
using namespace OpenLoco::Input;

namespace OpenLoco::Ui::Windows::KeyboardShortcuts
{
    static const int rowHeight = 10; // CJK: 13

    static WindowEventList _events;

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 360, 238 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 358, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::keyboard_shortcuts),
        makeWidget({ 345, 2 }, { 13, 13 }, WidgetType::wt_9, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 360, 223 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 4, 19 }, { 352, 202 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical, StringIds::keyboard_shortcut_list_tip),
        makeWidget({ 4, 223 }, { 150, 12 }, WidgetType::wt_11, WindowColour::secondary, StringIds::reset_keys, StringIds::reset_keys_tip),
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
    static void drawScroll(Ui::Window* self, Gfx::Context* context, uint32_t scrollIndex);
    static void onMouseUp(Window* self, WidgetIndex_t widgetIndex);
    static void loc_4BE832(Window* self);
    static std::optional<FormatArguments> tooltip(Window*, WidgetIndex_t);
    static void getScrollSize(Ui::Window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void onScrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index);
    static void onScrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index);

    static void initEvents()
    {
        _events.on_mouse_up = onMouseUp;
        _events.get_scroll_size = getScrollSize;
        _events.scroll_mouse_down = onScrollMouseDown;
        _events.scroll_mouse_over = onScrollMouseOver;
        _events.tooltip = tooltip;
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
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
        window->enabled_widgets = (1 << Widx::close_button) | (1 << Widx::reset_keys_btn);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_10);

        window->row_count = static_cast<uint16_t>(ShortcutManager::count());
        window->row_hover = -1;

        return window;
    }

    // 0x004BE726
    static void draw(Ui::Window* self, Gfx::Context* context)
    {
        // Draw widgets.
        self->draw(context);
    }

    // 0x004BE72C
    static void drawScroll(Ui::Window* self, Gfx::Context* context, uint32_t scrollIndex)
    {
        auto colour = self->getColour(WindowColour::secondary);
        auto shade = Colour::getShade(colour, 4);
        Gfx::clearSingle(*context, shade);

        auto yPos = 0;
        for (auto i = 0; i < self->row_count; i++)
        {
            string_id format = StringIds::black_stringid;
            if (i == self->row_hover)
            {
                Gfx::drawRect(context, 0, yPos, 800, rowHeight, 0x2000030);
                format = StringIds::wcolour2_stringid;
            }

            auto modifierStringId = StringIds::empty;
            auto baseStringId = StringIds::empty;

            if (Config::get().keyboard_shortcuts[i].var_0 != 0xFF)
            {
                if (Config::get().keyboard_shortcuts[i].var_1 != 0)
                {
                    if (Config::get().keyboard_shortcuts[i].var_1 != 1)
                    {
                        modifierStringId = StringIds::keyboard_shortcut_modifier_ctrl;
                    }
                    else
                    {
                        modifierStringId = StringIds::keyboard_shortcut_modifier_shift;
                    }
                }
                baseStringId = StringIds::shortcut_key_base + Config::get().keyboard_shortcuts[i].var_0;
            }

            auto formatter = FormatArguments::common();
            formatter.push(StringIds::keyboard_shortcut_list_format);
            formatter.push(ShortcutManager::getName(static_cast<Shortcut>(i)));
            formatter.push(modifierStringId);
            formatter.push(baseStringId);

            Gfx::drawString_494B3F(*context, 0, yPos - 1, Colour::black, format, &formatter);
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
                loc_4BE832(self);
                return;
        }
    }

    // 0x004BE832
    static void loc_4BE832(Window* self)
    {
        call(0x004BE3F3);
        OpenLoco::Config::write();
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
        *scrollHeight = self->row_count * rowHeight;
    }

    // 0x004BE853
    static void onScrollMouseOver(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / rowHeight;

        if (row >= self->row_count)
            return;

        if (row != self->row_hover)
        {
            self->row_hover = row;
            self->invalidate();
        }
    }

    // 0x004BE87B
    static void onScrollMouseDown(Ui::Window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto row = y / rowHeight;

        if (row >= self->row_count)
            return;

        EditKeyboardShortcut::open(row);
    }
}
