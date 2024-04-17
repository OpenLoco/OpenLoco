#include "Audio/Audio.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::MusicSelection
{
    static constexpr Ui::Size kWindowSize = { 360, 238 };

    static constexpr uint8_t kRowHeight = 12; // CJK: 15

    enum widx
    {
        frame,
        title,
        close,
        panel,
        scrollview,
    };

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 360, 238 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 358, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::music_selection_title),
        makeWidget({ 345, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 360, 223 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 4, 19 }, { 352, 218 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical, StringIds::music_selection_tooltip),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    // 0x004C1602
    Window* open()
    {
        Window* window = WindowManager::bringToFront(WindowType::musicSelection, 0);
        if (window != nullptr)
            return window;

        window = WindowManager::createWindow(
            WindowType::musicSelection,
            kWindowSize,
            WindowFlags::none,
            getEvents());

        window->setWidgets(_widgets);
        window->enabledWidgets = 1 << widx::close;
        window->initScrollWidgets();

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->colour_0B);
        window->setColour(WindowColour::secondary, interface->colour_10);

        window->rowCount = Audio::kNumMusicTracks;
        window->rowHover = -1;

        return window;
    }

    // 0x004C165D
    static void draw(Ui::Window& window, Gfx::RenderTarget* rt)
    {
        // Draw widgets.
        window.draw(rt);
    }

    // 0x004C1663
    static void drawScroll(Ui::Window& window, Gfx::RenderTarget& rt, [[maybe_unused]] const uint32_t scrollIndex)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        auto shade = Colours::getShade(window.getColour(WindowColour::secondary).c(), 4);
        drawingCtx.clearSingle(rt, shade);

        const auto& config = Config::get().old;

        uint16_t y = 0;
        for (uint16_t i = 0; i < window.rowCount; i++)
        {
            if (y + kRowHeight < rt.y)
            {
                y += kRowHeight;
                continue;
            }
            else if (y > rt.y + rt.height)
            {
                break;
            }

            StringId text_colour_id = StringIds::black_stringid;

            // Draw hovered track
            if (i == window.rowHover)
            {
                drawingCtx.drawRect(rt, 0, y, 800, kRowHeight, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                text_colour_id = StringIds::wcolour2_stringid;
            }

            // Draw checkbox.
            drawingCtx.fillRectInset(rt, 2, y, 11, y + 10, window.getColour(WindowColour::secondary), Gfx::RectInsetFlags::colourLight | Gfx::RectInsetFlags::fillDarker | Gfx::RectInsetFlags::borderInset);

            // Draw checkmark if track is enabled.
            if (config.enabledMusic[i])
            {
                auto point = Point(2, y);
                drawingCtx.drawStringLeft(rt, point, window.getColour(WindowColour::secondary), StringIds::wcolour2_stringid, (void*)&StringIds::checkmark);
            }

            // Draw track name.
            {
                auto point = Point(15, y);
                StringId music_title_id = Audio::getMusicInfo(i)->titleId;
                drawingCtx.drawStringLeft(rt, point, window.getColour(WindowColour::secondary), text_colour_id, (void*)&music_title_id);
            }

            y += kRowHeight;
        }
    }

    // 0x004C176C
    static void getScrollSize([[maybe_unused]] Ui::Window& window, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = kRowHeight * Audio::kNumMusicTracks;
    }

    // 0x004C1757
    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window.type);
                break;
        }
    }

    // 0x004C1799
    static void onScrollMouseDown(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        uint16_t currentTrack = y / kRowHeight;
        if (currentTrack > window.rowCount)
            return;

        auto& config = Config::get().old;

        // Toggle the track in question.
        config.enabledMusic[currentTrack] = !config.enabledMusic[currentTrack];

        // Are any tracks enabled?
        uint8_t anyEnabled = 0;
        for (uint8_t i = 0; i < Audio::kNumMusicTracks; i++)
            anyEnabled |= config.enabledMusic[i];

        // Ensure at least this track is enabled.
        if (!anyEnabled)
            config.enabledMusic[currentTrack] = true;

        Config::write();
        Audio::revalidateCurrentTrack();
        window.invalidate();
    }

    // 0x004C1771
    static void onScrollMouseOver(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        uint16_t currentTrack = y / kRowHeight;
        if (currentTrack > window.rowCount || currentTrack == window.rowHover)
            return;

        window.rowHover = currentTrack;
        window.invalidate();
    }

    // 0x004C17E3
    static void onUpdate(Window& window)
    {
        auto optionsWindow = WindowManager::find(WindowType::options);
        if (optionsWindow == nullptr || optionsWindow->currentTab != Options::kTabOffsetMusic)
        {
            WindowManager::close(&window);
            return;
        }
    }

    // 0x004C1762
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onUpdate = onUpdate,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = onScrollMouseDown,
        .scrollMouseOver = onScrollMouseOver,
        .tooltip = tooltip,
        .draw = draw,
        .drawScroll = drawScroll,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
