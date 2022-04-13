#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"
#include <algorithm>
#include <cstring>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ToolTip
{
    static loco_global<bool, 0x0052336E> _52336E;

    static loco_global<WindowType, 0x00523381> _tooltipWindowType;
    static loco_global<WindowNumber_t, 0x00523382> _tooltipWindowNumber;
    static loco_global<int16_t, 0x00523384> _tooltipWidgetIndex;

    static loco_global<uint16_t, 0x0052338C> _tooltipNotShownTicks;

    static loco_global<int32_t, 0x112C876> gCurrentFontSpriteBase;

    static char _text[512];          // 0x01136D90
    static uint16_t _lineBreakCount; // 0x01136F90

    static loco_global<int32_t, 0x01136F98> _currentTooltipStringId;

    static WindowEventList events;

    enum widx
    {
        text
    };

    // 0x005234CC
    Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 200, 32 }, WidgetType::wt_3, WindowColour::primary),
        widgetEnd(),
    };

    static void initEvents();

    void registerHooks()
    {
        registerHook(
            0x004C906B,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Ui::Windows::ToolTip::open((Ui::Window*)regs.esi, regs.edx, regs.ax, regs.bx);
                return 0;
            });
        registerHook(
            0x004C9216,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Ui::Windows::ToolTip::update((Ui::Window*)regs.esi, regs.edx, regs.di, regs.ax, regs.bx);
                return 0;
            });
    }

    static void common(const Window* window, int32_t widgetIndex, string_id stringId, int16_t cursorX, int16_t cursorY, FormatArguments& args)
    {
        StringManager::formatString(_text, stringId, &args);

        gCurrentFontSpriteBase = Font::medium_bold;
        int16_t strWidth = Gfx::getStringWidthNewLined(_text);
        strWidth = std::max<int16_t>(strWidth, 196);

        gCurrentFontSpriteBase = Font::medium_bold;

        auto [wrappedWidth, breakCount] = Gfx::wrapString(_text, strWidth + 1);
        _lineBreakCount = breakCount;

        int width = wrappedWidth + 3;
        int height = (_lineBreakCount + 1) * 10 + 4;
        _widgets[widx::text].right = width;
        _widgets[widx::text].bottom = height;

        int x, y;

        int maxY = Ui::height() - height;
        y = cursorY + 26; // Normally, we'd display the tooltip 26 lower
        if (y > maxY)
            // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
            // so we'll subtract a bit more
            y -= height + 40;
        y = std::clamp(y, 22, maxY);

        x = width <= Ui::width() ? std::clamp(cursorX - (width / 2), 0, Ui::width() - width) : 0;

        initEvents();

        auto tooltip = WindowManager::createWindow(
            WindowType::tooltip,
            Ui::Point(x, y),
            Ui::Size(width, height),
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::flag_7,
            &events);
        tooltip->widgets = _widgets;
        _tooltipNotShownTicks = 0;
    }

    // 0x004C906B
    void open(Ui::Window* window, int32_t widgetIndex, int16_t cursorX, int16_t cursorY)
    {
        WindowManager::close(WindowType::tooltip, 0);
        if (window == nullptr || widgetIndex == -1)
        {
            return;
        }

        window->callPrepareDraw();
        if (window->widgets[widgetIndex].tooltip == StringIds::null)
        {
            return;
        }

        _tooltipWindowType = window->type;
        _tooltipWindowNumber = window->number;
        _tooltipWidgetIndex = widgetIndex;

        auto toolArgs = window->callTooltip(widgetIndex);
        if (!toolArgs)
        {
            return;
        }

        auto wnd = WindowManager::find(WindowType::error, 0);
        if (wnd != nullptr)
        {
            return;
        }

        _currentTooltipStringId = -1;

        common(window, widgetIndex, window->widgets[widgetIndex].tooltip, cursorX, cursorY, *toolArgs);
    }

    // 0x004C9216
    void update(Ui::Window* window, int32_t widgetIndex, string_id stringId, int16_t cursorX, int16_t cursorY)
    {
        WindowManager::close(WindowType::tooltip, 0);

        _tooltipWindowType = window->type;
        _tooltipWindowNumber = window->number;
        _tooltipWidgetIndex = widgetIndex;

        auto toolArgs = window->callTooltip(widgetIndex);
        if (!toolArgs)
        {
            return;
        }

        auto wnd = WindowManager::find(WindowType::error, 0);
        if (wnd != nullptr)
        {
            return;
        }

        _currentTooltipStringId = stringId;

        common(window, widgetIndex, stringId, cursorX, cursorY, *toolArgs);
    }

    // 0x004C9397
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        uint16_t x = window->x;
        uint16_t y = window->y;
        uint16_t width = window->width;
        uint16_t height = window->height;

        Gfx::drawRect(*context, x + 1, y + 1, width - 2, height - 2, 0x2000000 | 45);
        Gfx::drawRect(*context, x + 1, y + 1, width - 2, height - 2, 0x2000000 | (116 + enumValue(ObjectManager::get<InterfaceSkinObject>()->colour_08)));

        Gfx::drawRect(*context, x, y + 2, 1, height - 4, 0x2000000 | 46);
        Gfx::drawRect(*context, x + width - 1, y + 2, 1, height - 4, 0x2000000 | 46);
        Gfx::drawRect(*context, x + 2, y + height - 1, width - 4, 1, 0x2000000 | 46);
        Gfx::drawRect(*context, x + 2, y, width - 4, 1, 0x2000000 | 46);

        Gfx::drawRect(*context, x + 1, y + 1, 1, 1, 0x2000000 | 46);
        Gfx::drawRect(*context, x + width - 1 - 1, y + 1, 1, 1, 0x2000000 | 46);
        Gfx::drawRect(*context, x + 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);
        Gfx::drawRect(*context, x + width - 1 - 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);

        Gfx::drawStringCentredRaw(*context, ((width + 1) / 2) + x - 1, y + 1, _lineBreakCount, Colour::black, _text);
    }

    // 0x004C94F7
    static void onClose(Ui::Window* window)
    {
        auto str337 = const_cast<char*>(StringManager::getString(StringIds::buffer_337));
        str337[0] = '\0';
    }

    // 0x004C94FF
    static void update(Ui::Window* window)
    {
        if (_52336E == false)
        {
            _tooltipNotShownTicks = 0;
        }
    }

    void set_52336E(bool value)
    {
        _52336E = value;
    }

    // 0x004C87B5
    void closeAndReset()
    {
        Ui::WindowManager::close(WindowType::tooltip, 0);
        Input::setTooltipTimeout(0);
        _tooltipWindowType = WindowType::undefined;
        _currentTooltipStringId = StringIds::null;
        set_52336E(false);
    }

    static void initEvents()
    {
        events.onClose = onClose;
        events.onUpdate = update;
        events.draw = draw;
    }
}
