#include "../Audio/Audio.h"
#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::MusicSelection
{
    static const Ui::Size window_size = { 360, 238 };

    static const uint8_t rowHeight = 12; // CJK: 15

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

    static WindowEventList _events;

    static void draw(Ui::Window* window, Gfx::Context* context);
    static void drawScroll(Ui::Window& window, Gfx::Context& context, const uint32_t scrollIndex);
    static void getScrollSize(Ui::Window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex);
    static void onScrollMouseDown(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void onScrollMouseOver(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void onUpdate(Window* window);
    static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex);

    static void initEvents()
    {
        _events.draw = draw;
        _events.drawScroll = drawScroll;
        _events.getScrollSize = getScrollSize;
        _events.onMouseUp = onMouseUp;
        _events.onUpdate = onUpdate;
        _events.scrollMouseDown = onScrollMouseDown;
        _events.scrollMouseOver = onScrollMouseOver;
        _events.tooltip = tooltip;
    }

    // 0x004C1602
    Window* open()
    {
        Window* window = WindowManager::bringToFront(WindowType::musicSelection, 0);
        if (window != nullptr)
            return window;

        window = WindowManager::createWindow(
            WindowType::musicSelection,
            window_size,
            0,
            &_events);

        // TODO: only needs to be called once.
        initEvents();

        window->widgets = _widgets;
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
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        // Draw widgets.
        window->draw(context);
    }

    // 0x004C1663
    static void drawScroll(Ui::Window& window, Gfx::Context& context, const uint32_t scrollIndex)
    {
        auto shade = Colours::getShade(window.getColour(WindowColour::secondary).c(), 4);
        Gfx::clearSingle(context, shade);

        const auto& config = Config::get();

        uint16_t y = 0;
        for (uint16_t i = 0; i < window.rowCount; i++)
        {
            string_id text_colour_id = StringIds::black_stringid;

            // Draw hovered track
            if (i == window.rowHover)
            {
                Gfx::drawRect(context, 0, y, 800, rowHeight, 0x2000030);
                text_colour_id = StringIds::wcolour2_stringid;
            }

            // Draw checkbox.
            Gfx::fillRectInset(context, 2, y, 11, y + 10, window.getColour(WindowColour::secondary).u8(), 0xE0);

            // Draw checkmark if track is enabled.
            if (config.enabledMusic[i])
                Gfx::drawString_494B3F(context, 2, y, window.getColour(WindowColour::secondary), StringIds::wcolour2_stringid, (void*)&StringIds::checkmark);

            // Draw track name.
            string_id music_title_id = Audio::getMusicInfo(i)->titleId;
            Gfx::drawString_494B3F(context, 15, y, window.getColour(WindowColour::secondary), text_colour_id, (void*)&music_title_id);

            y += rowHeight;
        }
    }

    // 0x004C176C
    static void getScrollSize(Ui::Window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = rowHeight * Audio::kNumMusicTracks;
    }

    // 0x004C1757
    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window->type);
                break;
        }
    }

    // 0x004C1799
    static void onScrollMouseDown(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentTrack = y / rowHeight;
        if (currentTrack > window->rowCount)
            return;

        auto& config = Config::get();

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
        window->invalidate();
    }

    // 0x004C1771
    static void onScrollMouseOver(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentTrack = y / rowHeight;
        if (currentTrack > window->rowCount || currentTrack == window->rowHover)
            return;

        window->rowHover = currentTrack;
        window->invalidate();
    }

    // 0x004C17E3
    static void onUpdate(Window* window)
    {
        auto optionsWindow = WindowManager::find(WindowType::options);
        if (optionsWindow == nullptr || optionsWindow->currentTab != Options::tab_offset_music)
        {
            WindowManager::close(window);
            return;
        }
    }

    // 0x004C1762
    static std::optional<FormatArguments> tooltip(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }
}
