#include "Audio/Audio.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Jukebox.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::MusicSelection
{
    static constexpr Ui::Size32 kWindowSize = { 360, 238 };

    static constexpr uint8_t kRowHeight = 12; // CJK: 15

    enum widx
    {
        frame,
        title,
        close,
        panel,
        scrollview,
    };

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { 360, 238 }, WindowColour::primary),
        makeWidget({ 1, 1 }, { 358, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::music_selection_title),
        Widgets::ImageButton({ 345, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { 360, 223 }, WindowColour::secondary),
        makeWidget({ 4, 19 }, { 352, 218 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical, StringIds::music_selection_tooltip)

    );

    static const WindowEventList& getEvents();

    // 0x004C1602
    Window* open()
    {
        Window* window = WindowManager::bringToFront(WindowType::musicSelection, 0);
        if (window != nullptr)
        {
            return window;
        }

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

        window->rowCount = Jukebox::kNumMusicTracks;
        window->rowHover = -1;

        return window;
    }

    // 0x004C165D
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        // Draw widgets.
        window.draw(drawingCtx);
    }

    // 0x004C1663
    static void drawScroll(Ui::Window& window, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
    {
        const auto& rt = drawingCtx.currentRenderTarget();
        auto tr = Gfx::TextRenderer(drawingCtx);

        auto shade = Colours::getShade(window.getColour(WindowColour::secondary).c(), 4);
        drawingCtx.clearSingle(shade);

        const auto& config = Config::get().old;

        uint16_t y = 0;
        for (uint16_t row = 0; row < window.rowCount; row++)
        {
            auto musicTrack = row; // id of music track on this row

            if (y + kRowHeight < rt.y)
            {
                y += kRowHeight;
                continue;
            }
            else if (y > rt.y + rt.height)
            {
                break;
            }

            StringId textColour = StringIds::black_stringid;

            // Draw hovered track
            if (row == window.rowHover)
            {
                drawingCtx.drawRect(0, y, 800, kRowHeight, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                textColour = StringIds::wcolour2_stringid;
            }

            // Draw checkbox.
            drawingCtx.fillRectInset(2, y, 11, y + 10, window.getColour(WindowColour::secondary), Gfx::RectInsetFlags::colourLight | Gfx::RectInsetFlags::fillDarker | Gfx::RectInsetFlags::borderInset);

            // Draw checkmark if track is enabled.
            if (config.enabledMusic[musicTrack])
            {
                auto point = Point(2, y);

                auto argsBuf = FormatArgumentsBuffer{};
                auto args = FormatArguments{ argsBuf };
                args.push(StringIds::checkmark);
                tr.drawStringLeft(point, window.getColour(WindowColour::secondary), StringIds::wcolour2_stringid, args);
            }

            // Draw track name.
            {
                auto point = Point(15, y);
                StringId musicTitle = Jukebox::getMusicInfo(musicTrack).titleId;

                auto argsBuf = FormatArgumentsBuffer{};
                auto args = FormatArguments{ argsBuf };
                args.push(musicTitle);
                tr.drawStringLeft(point, window.getColour(WindowColour::secondary), textColour, args);
            }

            y += kRowHeight;
        }
    }

    // 0x004C176C
    static void getScrollSize([[maybe_unused]] Ui::Window& window, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = kRowHeight * Jukebox::kNumMusicTracks;
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
        {
            return;
        }

        auto& config = Config::get().old;

        // Toggle the track in question.
        config.enabledMusic[currentTrack] = !config.enabledMusic[currentTrack];

        // Are any tracks enabled?
        uint8_t anyEnabled = 0;
        for (uint8_t i = 0; i < Jukebox::kNumMusicTracks; i++)
        {
            anyEnabled |= config.enabledMusic[i];
        }

        // Ensure at least this track is enabled.
        if (!anyEnabled)
        {
            config.enabledMusic[currentTrack] = true;
        }

        Config::write();
        Audio::revalidateCurrentTrack();
        window.invalidate();
    }

    // 0x004C1771
    static void onScrollMouseOver(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        uint16_t currentTrack = y / kRowHeight;
        if (currentTrack > window.rowCount || currentTrack == window.rowHover)
        {
            return;
        }

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
