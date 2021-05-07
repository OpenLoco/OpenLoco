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

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::MusicSelection
{
    static const Gfx::ui_size_t window_size = { 360, 238 };

    static const uint8_t rowHeight = 12; // CJK: 15

    enum widx
    {
        frame,
        title,
        close,
        panel,
        scrollview,
    };

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { 360, 238 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 358, 13 }, widget_type::caption_25, 0, StringIds::music_selection_title),
        makeWidget({ 345, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { 360, 223 }, widget_type::panel, 1),
        makeWidget({ 4, 19 }, { 352, 218 }, widget_type::scrollview, 1, scrollbars::vertical, StringIds::music_selection_tooltip),
        widgetEnd(),
    };

    static window_event_list _events;

    static void draw(Ui::window* window, Gfx::Context* context);
    static void drawScroll(Ui::window* window, Gfx::Context* context, uint32_t scrollIndex);
    static void getScrollSize(Ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
    static void onMouseUp(Ui::window* window, widget_index widgetIndex);
    static void onScrollMouseDown(Ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void onScrollMouseOver(Ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
    static void onUpdate(window* window);
    static std::optional<FormatArguments> tooltip(Ui::window* window, widget_index widgetIndex);

    static void initEvents()
    {
        _events.draw = draw;
        _events.draw_scroll = drawScroll;
        _events.get_scroll_size = getScrollSize;
        _events.on_mouse_up = onMouseUp;
        _events.on_update = onUpdate;
        _events.scroll_mouse_down = onScrollMouseDown;
        _events.scroll_mouse_over = onScrollMouseOver;
        _events.tooltip = tooltip;
    }

    // 0x004C1602
    window* open()
    {
        window* window = WindowManager::bringToFront(WindowType::musicSelection, 0);
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
        window->enabled_widgets = 1 << widx::close;
        window->initScrollWidgets();

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;

        window->row_count = Audio::num_music_tracks;
        window->row_hover = -1;

        return window;
    }

    // 0x004C165D
    static void draw(Ui::window* window, Gfx::Context* context)
    {
        // Draw widgets.
        window->draw(context);
    }

    // 0x004C1663
    static void drawScroll(Ui::window* window, Gfx::Context* context, uint32_t scrollIndex)
    {
        auto shade = Colour::getShade(window->colours[1], 4);
        Gfx::clearSingle(*context, shade);

        auto config = Config::get();

        uint16_t y = 0;
        for (uint16_t i = 0; i < window->row_count; i++)
        {
            string_id text_colour_id = StringIds::black_stringid;

            // Draw hovered track
            if (i == window->row_hover)
            {
                Gfx::drawRect(context, 0, y, 800, rowHeight, 0x2000030);
                text_colour_id = StringIds::wcolour2_stringid;
            }

            // Draw checkbox.
            Gfx::fillRectInset(context, 2, y, 11, y + 10, window->colours[1], 0xE0);

            // Draw checkmark if track is enabled.
            if (config.enabled_music[i])
                Gfx::drawString_494B3F(*context, 2, y, window->colours[1], StringIds::wcolour2_stringid, (void*)&StringIds::checkmark);

            // Draw track name.
            string_id music_title_id = Audio::getMusicInfo(i)->title_id;
            Gfx::drawString_494B3F(*context, 15, y, window->colours[1], text_colour_id, (void*)&music_title_id);

            y += rowHeight;
        }
    }

    // 0x004C176C
    static void getScrollSize(Ui::window* window, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = rowHeight * Audio::num_music_tracks;
    }

    // 0x004C1757
    static void onMouseUp(Ui::window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window->type);
                break;
        }
    }

    // 0x004C1799
    static void onScrollMouseDown(Ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentTrack = y / rowHeight;
        if (currentTrack > window->row_count)
            return;

        auto& config = Config::get();

        // Toggle the track in question.
        config.enabled_music[currentTrack] = !config.enabled_music[currentTrack];

        // Are any tracks enabled?
        uint8_t anyEnabled = 0;
        for (uint8_t i = 0; i < Audio::num_music_tracks; i++)
            anyEnabled |= config.enabled_music[i];

        // Ensure at least this track is enabled.
        if (!anyEnabled)
            config.enabled_music[currentTrack] = true;

        Config::write();
        Audio::revalidateCurrentTrack();
        window->invalidate();
    }

    // 0x004C1771
    static void onScrollMouseOver(Ui::window* window, int16_t x, int16_t y, uint8_t scroll_index)
    {
        uint16_t currentTrack = y / rowHeight;
        if (currentTrack > window->row_count || currentTrack == window->row_hover)
            return;

        window->row_hover = currentTrack;
        window->invalidate();
    }

    // 0x004C17E3
    static void onUpdate(window* window)
    {
        auto optionsWindow = WindowManager::find(WindowType::options);
        if (optionsWindow == nullptr || optionsWindow->current_tab != Options::tab_offset_music)
        {
            WindowManager::close(window);
            return;
        }
    }

    // 0x004C1762
    static std::optional<FormatArguments> tooltip(Ui::window* window, widget_index widgetIndex)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }
}
