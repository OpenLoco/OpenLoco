#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>
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

    static char _text[512];          // 0x01136D90
    static uint16_t _lineBreakCount; // 0x01136F90

    static loco_global<int32_t, 0x01136F98> _currentTooltipStringId;

    enum widx
    {
        text
    };

    // 0x005234CC
    static constexpr auto _widgets = makeWidgets(
        Widgets::Wt3Widget({ 0, 0 }, { 200, 32 }, WindowColour::primary)

    );

    void registerHooks()
    {
        registerHook(
            0x004C906B,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Ui::Windows::ToolTip::open((Ui::Window*)regs.esi, regs.edx, regs.ax, regs.bx);
                regs = backup;
                return 0;
            });
        registerHook(
            0x004C9216,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Ui::Windows::ToolTip::update((Ui::Window*)regs.esi, regs.edx, regs.di, regs.ax, regs.bx);
                regs = backup;
                return 0;
            });
    }

    static const WindowEventList& getEvents();

    static void common([[maybe_unused]] const Window* window, [[maybe_unused]] int32_t widgetIndex, StringId stringId, int16_t cursorX, int16_t cursorY, FormatArguments& args)
    {
        StringManager::formatString(_text, stringId, args);

        const auto font = Gfx::Font::medium_bold;
        int16_t strWidth = Gfx::TextRenderer::getStringWidthNewLined(font, _text);
        strWidth = std::min<int16_t>(strWidth, 196);

        auto [wrappedWidth, breakCount] = Gfx::TextRenderer::wrapString(font, _text, strWidth + 1);
        _lineBreakCount = breakCount;

        int width = wrappedWidth + 3;
        int height = (_lineBreakCount + 1) * 10 + 4;
        int x, y;

        int maxY = Ui::height() - height;
        y = cursorY + 26; // Normally, we'd display the tooltip 26 lower
        if (y > maxY)
        {
            // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
            // so we'll subtract a bit more
            y -= height + 40;
        }
        y = std::clamp(y, 22, maxY);

        x = width <= Ui::width() ? std::clamp(cursorX - (width / 2), 0, Ui::width() - width) : 0;

        auto tooltip = WindowManager::createWindow(
            WindowType::tooltip,
            { x, y },
            { width, height },
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::flag_7,
            getEvents());
        tooltip->setWidgets(_widgets);
        tooltip->widgets[widx::text].right = width;
        tooltip->widgets[widx::text].bottom = height;

        _tooltipNotShownTicks = 0;
    }

    // 0x004C906B
    void open(Ui::Window* window, int32_t widgetIndex, int16_t cursorX, int16_t cursorY)
    {
        WindowManager::close(WindowType::tooltip, 0);
        if (window == nullptr || widgetIndex == kWidgetIndexNull)
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

        auto toolArgs = window->callTooltip(widgetIndex, window->widgets[widgetIndex].id);
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
    void update(Ui::Window* window, int32_t widgetIndex, StringId stringId, int16_t cursorX, int16_t cursorY)
    {
        WindowManager::close(WindowType::tooltip, 0);

        _tooltipWindowType = window->type;
        _tooltipWindowNumber = window->number;
        _tooltipWidgetIndex = widgetIndex;

        auto toolArgs = window->callTooltip(widgetIndex, window->widgets[widgetIndex].id);
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
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        uint16_t width = window.width;
        uint16_t height = window.height;

        drawingCtx.drawRect(1, 1, width - 2, height - 2, enumValue(ExtColour::unk2D), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(1, 1, width - 2, height - 2, (enumValue(ExtColour::unk74) + enumValue(ObjectManager::get<InterfaceSkinObject>()->tooltipColour)), Gfx::RectFlags::transparent);

        drawingCtx.drawRect(0, 2, 1, height - 4, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(0 + width - 1, 2, 1, height - 4, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(0 + 2, height - 1, width - 4, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(0 + 2, 0, width - 4, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);

        drawingCtx.drawRect(0 + 1, 1, 1, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(0 + width - 1 - 1, 1, 1, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(0 + 1, height - 1 - 1, 1, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(0 + width - 1 - 1, height - 1 - 1, 1, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);

        auto point = Point(((width + 1) / 2) - 1, 1);
        tr.drawStringCentredRaw(point, _lineBreakCount, Colour::black, _text);
    }

    // 0x004C94F7
    static void onClose([[maybe_unused]] Ui::Window& window)
    {
        auto str337 = const_cast<char*>(StringManager::getString(StringIds::buffer_337));
        str337[0] = '\0';
    }

    // 0x004C94FF
    static void update([[maybe_unused]] Ui::Window& window)
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

    // 0x00439BB1
    bool isTimeTooltip()
    {
        return _tooltipWindowType == WindowType::timeToolbar && _tooltipWidgetIndex == 3;
    }

    static constexpr WindowEventList kEvents = {
        .onClose = onClose,
        .onUpdate = update,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
