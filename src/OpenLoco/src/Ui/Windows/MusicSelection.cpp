#include "Audio/Audio.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Jukebox.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TableHeaderWidget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::MusicSelection
{
    static constexpr auto kColumnNameWidth = 261;
    static constexpr auto kColumnYearsWidth = 59;

    static constexpr Ui::Size32 kWindowSize = { 20 + kColumnNameWidth + kColumnYearsWidth + 20, 249 };

    static constexpr uint8_t kRowHeight = 12; // CJK: 15

    // TODO: make this an attribute of the Music Selection window object rather than static
    static std::vector<Jukebox::MusicId> playlist;

    enum widx
    {
        frame,
        title,
        close,
        panel,
        sort_name,
        sort_years,
        scrollview,
    };

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { kWindowSize.width, kWindowSize.height }, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::music_selection_title),
        Widgets::ImageButton({ kWindowSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { kWindowSize.width, kWindowSize.height - 15 }, WindowColour::secondary),
        Widgets::TableHeader({ 20, 17 }, { kColumnNameWidth, 12 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_sort_by_name),
        Widgets::TableHeader({ 20 + kColumnNameWidth, 17 }, { kColumnYearsWidth, 12 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_sort_by_music_years),
        Widgets::ScrollView({ 4, 30 }, { kWindowSize.width - 8, kWindowSize.height - 1 - 30 }, WindowColour::secondary, Scrollbars::vertical, StringIds::music_selection_tooltip)

    );

    static const WindowEventList& getEvents();

    static void setSortMode(Ui::Window& window, Jukebox::MusicSortMode newSortMode)
    {
        window.sortMode = static_cast<uint16_t>(newSortMode);
        playlist = Jukebox::makeAllMusicPlaylist(Jukebox::MusicSortMode(window.sortMode));
        window.invalidate();
    }

    // Cycles window.sortMode: (MusicSortMode::original or other) → order1 → order2 → MusicSortMode::original
    static void cycleSortMode(Ui::Window& window, Jukebox::MusicSortMode order1, Jukebox::MusicSortMode order2)
    {
        auto oldSortMode = Jukebox::MusicSortMode(window.sortMode);
        auto newSortMode = order1;

        if (oldSortMode == order2)
        {
            newSortMode = Jukebox::MusicSortMode::original;
        }
        else if (oldSortMode == order1)
        {
            newSortMode = order2;
        }
        setSortMode(window, newSortMode);
    }

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
        window->initScrollWidgets();

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->windowTitlebarColour);
        window->setColour(WindowColour::secondary, interface->windowOptionsColour);

        window->rowCount = Jukebox::kNumMusicTracks;
        window->rowHover = -1;
        window->sortMode = static_cast<uint16_t>(Jukebox::MusicSortMode::original);

        playlist = Jukebox::makeAllMusicPlaylist(Jukebox::MusicSortMode(window->sortMode));

        return window;
    }

    static void prepareDraw(Ui::Window& window)
    {
        auto sortMode = Jukebox::MusicSortMode(window.sortMode);

        // Set header button captions
        window.widgets[widx::sort_name].text = StringIds::table_header_name;
        window.widgets[widx::sort_years].text = StringIds::table_header_years;
        switch (sortMode)
        {
            case Jukebox::MusicSortMode::original:
                break;
            case Jukebox::MusicSortMode::name:
                window.widgets[widx::sort_name].text = StringIds::table_header_name_desc;
                break;
            case Jukebox::MusicSortMode::nameReverse:
                window.widgets[widx::sort_name].text = StringIds::table_header_name_asc;
                break;
            case Jukebox::MusicSortMode::yearsAscending:
                window.widgets[widx::sort_years].text = StringIds::table_header_years_asc;
                break;
            case Jukebox::MusicSortMode::yearsDescending:
                window.widgets[widx::sort_years].text = StringIds::table_header_years_desc;
                break;
        }
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

        const auto& config = Config::get().audio;

        uint16_t y = 0;
        for (uint16_t row = 0; row < window.rowCount; row++)
        {
            auto musicTrack = playlist[row]; // id of music track on this row
            const auto& musicInfo = Jukebox::getMusicInfo(musicTrack);

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

            // Draw hightlight on hovered row
            if (row == window.rowHover)
            {
                drawingCtx.drawRect(0, y, 800, kRowHeight, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                textColour = StringIds::wcolour2_stringid;
            }

            // Draw checkbox.
            drawingCtx.fillRectInset(2, y, 11, y + 10, window.getColour(WindowColour::secondary), Gfx::RectInsetFlags::colourLight | Gfx::RectInsetFlags::fillDarker | Gfx::RectInsetFlags::borderInset);

            // Draw checkmark if track is enabled.
            if (config.customJukebox[musicTrack])
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
                StringId musicTitle = musicInfo.titleId;

                auto argsBuf = FormatArgumentsBuffer{};
                auto args = FormatArguments{ argsBuf };
                args.push(musicTitle);
                tr.drawStringLeft(point, window.getColour(WindowColour::secondary), textColour, args);
            }

            // Draw track years.
            {
                auto point = Point(15 + kColumnNameWidth, y);
                auto argsBuf = FormatArgumentsBuffer{};
                auto args = FormatArguments{ argsBuf };
                bool hasStart = musicInfo.startYear != Jukebox::kNoStartYear;
                bool hasEnd = musicInfo.endYear != Jukebox::kNoEndYear;
                if (hasStart && hasEnd)
                {
                    args.push(StringIds::year_range);
                    args.push(musicInfo.startYear);
                    args.push(musicInfo.endYear);
                    tr.drawStringLeft(point, window.getColour(WindowColour::secondary), textColour, args);
                }
                else if (hasStart)
                {
                    args.push(StringIds::year_range_no_end);
                    args.push(musicInfo.startYear);
                    tr.drawStringLeft(point, window.getColour(WindowColour::secondary), textColour, args);
                }
                else if (hasEnd)
                {
                    args.push(StringIds::year_range_no_start);
                    args.push(musicInfo.endYear);
                    point.x += kColumnYearsWidth;
                    tr.drawStringRight(point, window.getColour(WindowColour::secondary), textColour, args);
                }
                else
                {
                    args.push(StringIds::year_range_no_start_no_end);
                    tr.drawStringLeft(point, window.getColour(WindowColour::secondary), textColour, args);
                }
            }

            y += kRowHeight;
        }
    }

    // 0x004C176C
    static void getScrollSize([[maybe_unused]] Ui::Window& window, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
    {
        scrollHeight = kRowHeight * Jukebox::kNumMusicTracks;
    }

    // 0x004C1757
    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window.type);
                break;

            case sort_name:
                cycleSortMode(window, Jukebox::MusicSortMode::name, Jukebox::MusicSortMode::nameReverse);
                break;
            case sort_years:
                cycleSortMode(window, Jukebox::MusicSortMode::yearsDescending, Jukebox::MusicSortMode::yearsAscending);
                break;
        }
    }

    // 0x004C1799
    static void onScrollMouseDown(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        uint16_t currentRow = y / kRowHeight;
        Jukebox::MusicId currentTrack = playlist[currentRow];
        if (currentTrack > Jukebox::kNumMusicTracks)
        {
            return;
        }

        auto& config = Config::get().audio;

        // Toggle the track in question.
        config.customJukebox[currentTrack] ^= true;

        // Are any tracks enabled?
        bool anyEnabled = false;
        for (uint8_t i = 0; i < Jukebox::kNumMusicTracks; i++)
        {
            anyEnabled |= config.customJukebox[i];
        }

        // Ensure at least this track is enabled.
        if (!anyEnabled)
        {
            config.customJukebox[currentTrack] = true;
        }

        Config::write();
        Audio::revalidateCurrentTrack();
        window.invalidate();
    }

    // 0x004C1771
    static void onScrollMouseOver(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        uint16_t currentRow = y / kRowHeight;
        if (currentRow > window.rowCount || currentRow == window.rowHover)
        {
            return;
        }

        window.rowHover = currentRow;
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
    static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& window, [[maybe_unused]] WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
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
        .prepareDraw = prepareDraw,
        .draw = draw,
        .drawScroll = drawScroll,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
