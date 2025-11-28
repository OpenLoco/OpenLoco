#include "Ui/ToolTip.h"
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

#include <algorithm>
#include <cstring>

namespace OpenLoco::Ui::ToolTip
{
    static WindowType _tooltipWindowType;
    static WindowNumber_t _tooltipWindowNumber;
    static WidgetIndex_t _tooltipWidgetIndex;
    static uint16_t _tooltipNotShownTicks;
    static StringId _currentTooltipStringId;
    static Ui::Point _tooltipCursor;
    static uint16_t _tooltipTimeout;
    static bool _52336E;

    void setWindowType(WindowType wndType)
    {
        _tooltipWindowType = wndType;
    }
    WindowType getWindowType()
    {
        return _tooltipWindowType;
    }

    void setWindowNumber(WindowNumber_t wndNumber)
    {
        _tooltipWindowNumber = wndNumber;
    }
    WindowNumber_t getWindowNumber()
    {
        return _tooltipWindowNumber;
    }

    void setWidgetIndex(WidgetIndex_t widx)
    {
        _tooltipWidgetIndex = widx;
    }
    WidgetIndex_t getWidgetIndex()
    {
        return _tooltipWidgetIndex;
    }

    void setNotShownTicks(uint16_t ticks)
    {
        _tooltipNotShownTicks = ticks;
    }
    uint16_t getNotShownTicks()
    {
        return _tooltipNotShownTicks;
    }

    StringId getCurrentStringId()
    {
        return _currentTooltipStringId;
    }
    void setCurrentStringId(StringId stringId)
    {
        _currentTooltipStringId = stringId;
    }

    // 0x00439BB1
    bool isTimeTooltip()
    {
        return _tooltipWindowType == WindowType::timeToolbar && _tooltipWidgetIndex == 3;
    }

    Ui::Point getTooltipMouseLocation()
    {
        return _tooltipCursor;
    }

    void setTooltipMouseLocation(const Ui::Point& loc)
    {
        _tooltipCursor = loc;
    }

    uint16_t getTooltipTimeout()
    {
        return _tooltipTimeout;
    }

    void setTooltipTimeout(uint16_t tooltipTimeout)
    {
        _tooltipTimeout = tooltipTimeout;
    }

    void set_52336E(bool value)
    {
        _52336E = value;
    }
}

namespace OpenLoco::Ui::Windows::ToolTip
{
    static char _text[512];          // 0x01136D90
    static uint16_t _lineBreakCount; // 0x01136F90

    enum widx
    {
        text
    };

    // 0x005234CC
    static constexpr auto _widgets = makeWidgets(
        Widgets::Wt3Widget({ 0, 0 }, { 200, 32 }, WindowColour::primary)

    );

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
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::ignoreInFindAt,
            getEvents());
        tooltip->setWidgets(_widgets);
        tooltip->widgets[widx::text].right = width;
        tooltip->widgets[widx::text].bottom = height;

        Ui::ToolTip::setNotShownTicks(0);
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

        Ui::ToolTip::setWindowType(window->type);
        Ui::ToolTip::setWindowNumber(window->number);
        Ui::ToolTip::setWidgetIndex(widgetIndex);

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

        Ui::ToolTip::setCurrentStringId(StringIds::null);

        common(window, widgetIndex, window->widgets[widgetIndex].tooltip, cursorX, cursorY, *toolArgs);
    }

    // 0x004C9216
    void update(Ui::Window* window, int32_t widgetIndex, StringId stringId, int16_t cursorX, int16_t cursorY)
    {
        WindowManager::close(WindowType::tooltip, 0);

        Ui::ToolTip::setWindowType(window->type);
        Ui::ToolTip::setWindowNumber(window->number);
        Ui::ToolTip::setWidgetIndex(widgetIndex);

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

        Ui::ToolTip::setCurrentStringId(stringId);

        common(window, widgetIndex, stringId, cursorX, cursorY, *toolArgs);
    }

    // 0x004C9397
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        uint16_t x = window.x;
        uint16_t y = window.y;
        uint16_t width = window.width;
        uint16_t height = window.height;

        drawingCtx.drawRect(x + 1, y + 1, width - 2, height - 2, enumValue(ExtColour::unk2D), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(x + 1, y + 1, width - 2, height - 2, (enumValue(ExtColour::unk74) + enumValue(ObjectManager::get<InterfaceSkinObject>()->tooltipColour)), Gfx::RectFlags::transparent);

        drawingCtx.drawRect(x, y + 2, 1, height - 4, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(x + width - 1, y + 2, 1, height - 4, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(x + 2, y + height - 1, width - 4, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(x + 2, y, width - 4, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);

        drawingCtx.drawRect(x + 1, y + 1, 1, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(x + width - 1 - 1, y + 1, 1, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(x + 1, y + height - 1 - 1, 1, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);
        drawingCtx.drawRect(x + width - 1 - 1, y + height - 1 - 1, 1, 1, enumValue(ExtColour::unk2E), Gfx::RectFlags::transparent);

        auto point = Point(((width + 1) / 2) + x - 1, y + 1);
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
        if (Ui::ToolTip::_52336E == false)
        {
            Ui::ToolTip::setNotShownTicks(0);
        }
    }

    // 0x004C87B5
    void closeAndReset()
    {
        Ui::WindowManager::close(WindowType::tooltip, 0);
        Ui::ToolTip::setTooltipTimeout(0);

        Ui::ToolTip::setWindowType(WindowType::undefined);
        Ui::ToolTip::setCurrentStringId(StringIds::null);

        Ui::ToolTip::set_52336E(false);
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
