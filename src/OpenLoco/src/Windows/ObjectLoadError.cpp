#include "Config.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/WindowManager.h"
#include "Widget.h"

namespace OpenLoco::Ui::Windows::ObjectLoadError
{
    static constexpr Ui::Size kWindowSize = { 360, 238 };

    static constexpr uint8_t kRowHeight = 12; // CJK: 15

    static std::vector<ObjectHeader> _loadErrorObjectsList;

    enum Widx
    {
        frame,
        title,
        close,
        panel,
        nameHeader,
        checksumHeader,
        scrollview,
    };

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 360, 238 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 358, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::objectErrorWindowTitle),
        makeWidget({ 345, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 360, 223 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 4, 43 }, { 176, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tableHeaderObjectId),
        makeWidget({ 180, 43 }, { 176, 12 }, WidgetType::buttonTableHeader, WindowColour::secondary, StringIds::tableHeaderObjectChecksum),
        makeWidget({ 4, 57 }, { 352, 176 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void draw(Ui::Window& window, Gfx::RenderTarget* rt);
    static void drawScroll(Ui::Window& window, Gfx::RenderTarget& rt, const uint32_t scrollIndex);
    static void getScrollSize(Ui::Window& window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex);
    static void onScrollMouseOver(Ui::Window& window, int16_t x, int16_t y, uint8_t scroll_index);
    static std::optional<FormatArguments> tooltip(Window& self, WidgetIndex_t widgetIndex);

    static void initEvents()
    {
        _events.draw = draw;
        _events.drawScroll = drawScroll;
        _events.getScrollSize = getScrollSize;
        _events.onMouseUp = onMouseUp;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.tooltip = tooltip;
    }

    Window* open(std::vector<ObjectHeader> list)
    {
        _loadErrorObjectsList = list;
        std::sort(_loadErrorObjectsList.begin(), _loadErrorObjectsList.end());

        Window* window = WindowManager::bringToFront(WindowType::objectLoadError);
        if (window != nullptr)
        {
            WindowManager::invalidate(WindowType::objectLoadError);
            return window;
        }

        window = WindowManager::createWindow(
            WindowType::objectLoadError,
            kWindowSize,
            WindowFlags::none,
            &_events);

        // TODO: only needs to be called once.
        initEvents();

        window->widgets = _widgets;
        window->enabledWidgets = 1 << Widx::close;
        window->initScrollWidgets();

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->colour_0B);
        window->setColour(WindowColour::secondary, interface->colour_10);

        window->rowCount = _loadErrorObjectsList.size();
        window->rowHover = -1;

        return window;
    }

    static void draw(Ui::Window& self, Gfx::RenderTarget* rt)
    {
        // Draw widgets
        self.draw(rt);

        // Draw explanatory text
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringLeftWrapped(*rt, self.x + 3, self.y + 19, self.width - 6, self.getColour(WindowColour::secondary), StringIds::objectErrorExplanation);
    }

    // 0x004C1663
    static void drawScroll(Ui::Window& window, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto shade = Colours::getShade(window.getColour(WindowColour::secondary).c(), 4);
        drawingCtx.clearSingle(rt, shade);

        // Acquire string buffer
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

        // Prepare format arguments to reuse
        FormatArguments args;
        args.push(StringIds::buffer_2039);

        uint16_t y = 0;
        for (uint16_t i = 0; i < window.rowCount; i++)
        {
            string_id text_colour_id = StringIds::black_stringid;

            // Draw hover rectangle
            if (i == window.rowHover)
            {
                drawingCtx.drawRect(rt, 0, y, 800, kRowHeight, enumValue(ExtColour::unk30), Drawing::RectFlags::transparent);
                text_colour_id = StringIds::wcolour2_stringid;
            }

            auto& header = _loadErrorObjectsList[i];

            // Copy object name to buffer
            std::memcpy(buffer, header.name, 8);
            buffer[8] = '\0';

            // Draw object name
            drawingCtx.drawStringLeft(rt, 1, y, window.getColour(WindowColour::secondary), text_colour_id, &args);

            // Copy object checksum to buffer
            std::stringstream ss;
            ss << std::uppercase << std::setfill('0') << std::hex << std::setw(8) << header.checksum;
            const auto checksum = ss.str();
            std::memcpy(buffer, checksum.c_str(), 8);
            buffer[8] = '\0';

            // Draw object checksum
            drawingCtx.drawStringLeft(rt, window.widgets[Widx::checksumHeader].left - 4, y, window.getColour(WindowColour::secondary), text_colour_id, &args);

            y += kRowHeight;
        }
    }

    static void getScrollSize(Ui::Window& window, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = kRowHeight * window.rowCount;
    }

    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::close:
                WindowManager::close(window.type);
                break;
        }
    }

    static void onScrollMouseOver(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        uint16_t currentTrack = y / kRowHeight;
        if (currentTrack > window.rowCount || currentTrack == window.rowHover)
            return;

        window.rowHover = currentTrack;
        window.invalidate();
    }

    static std::optional<FormatArguments> tooltip([[maybe_unused]] Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_object_list);
        return args;
    }
}
