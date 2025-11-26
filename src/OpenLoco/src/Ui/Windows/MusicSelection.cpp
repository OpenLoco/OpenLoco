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
#include "Ui/ScrollView.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/Widgets/TableHeaderWidget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::MusicSelection
{
    static constexpr Ui::Size32 kWindowSizeMin = { 300, 100 };
    static constexpr Ui::Size32 kWindowSizeMax = { 800, 800 };
    static constexpr Ui::Size32 kWindowSizeDefault = { 360, 238 };

    static constexpr auto kColumnYearsWidth = 75;
    static constexpr auto kStatusBarClearance = 13;
    static constexpr auto kPadding = 4;
    static constexpr uint8_t kRowHeight = 12; // CJK: 15

    // TODO: make this an attribute of the Music Selection window object rather than static
    static std::vector<Jukebox::MusicId> playlist;

    enum widx
    {
        frame,
        title,
        close,
        panel,
        sort_title,
        sort_years,
        scrollview,
        status_bar,
    };

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { kWindowSizeDefault.width, kWindowSizeDefault.height }, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSizeDefault.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::music_selection_title),
        Widgets::ImageButton({ kWindowSizeDefault.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { kWindowSizeDefault.width, kWindowSizeDefault.height - 15 }, WindowColour::secondary),
        Widgets::TableHeader({ kPadding + 1, 17 }, { kWindowSizeDefault.width - 2 * kPadding - kColumnYearsWidth - 1, 12 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_sort_by_track_title),
        Widgets::TableHeader({ kWindowSizeDefault.width - kPadding - kColumnYearsWidth, 17 }, { kColumnYearsWidth, 12 }, WindowColour::secondary, Widget::kContentNull, StringIds::tooltip_sort_by_music_years),
        Widgets::ScrollView({ kPadding, 30 }, { kWindowSizeDefault.width - 2 * kPadding, kWindowSizeDefault.height - kStatusBarClearance - 30 }, WindowColour::secondary, Scrollbars::vertical, StringIds::music_selection_tooltip),
        Widgets::Label({ kPadding, kWindowSizeDefault.height - 12 }, { kWindowSizeMin.width - kResizeHandleSize - kPadding, 11 }, WindowColour::secondary, ContentAlign::left, StringIds::black_stringid)

    );

    static const WindowEventList& getEvents();

    static void setSortMode(Ui::Window& window, Jukebox::MusicSortMode sortMode)
    {
        window.sortMode = static_cast<uint16_t>(sortMode);

        playlist = Jukebox::makeAllMusicPlaylist(sortMode);

        // Set table header button captions
        window.widgets[widx::sort_title].text = StringIds::table_header_title;
        window.widgets[widx::sort_years].text = StringIds::table_header_years;
        switch (sortMode)
        {
            case Jukebox::MusicSortMode::original:
                break;
            case Jukebox::MusicSortMode::titleAscending:
                window.widgets[widx::sort_title].text = StringIds::table_header_title_asc;
                break;
            case Jukebox::MusicSortMode::titleDescending:
                window.widgets[widx::sort_title].text = StringIds::table_header_title_desc;
                break;
            case Jukebox::MusicSortMode::yearsAscending:
                window.widgets[widx::sort_years].text = StringIds::table_header_years_asc;
                break;
            case Jukebox::MusicSortMode::yearsDescending:
                window.widgets[widx::sort_years].text = StringIds::table_header_years_desc;
                break;
        }
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

        window.invalidate();
    }

    static void updateStatusBar(Ui::Window& self, uint16_t numTracksSelected)
    {
        FormatArguments args{ self.widgets[widx::status_bar].textArgs };
        args.push(numTracksSelected == 1 ? StringIds::status_music_tracks_selected_singular : StringIds::status_music_tracks_selected_plural);
        args.push(numTracksSelected);
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
            kWindowSizeDefault,
            WindowFlags::resizable,
            getEvents());

        window->setWidgets(_widgets);
        window->initScrollWidgets();

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->windowTitlebarColour);
        window->setColour(WindowColour::secondary, interface->windowOptionsColour);

        window->rowCount = Jukebox::kNumMusicTracks;
        window->rowHover = -1;

        setSortMode(*window, Jukebox::MusicSortMode::original);

        // Set status bar text
        uint16_t numTracksSelected = std::ranges::count(Config::get().audio.customJukebox, true);
        updateStatusBar(*window, numTracksSelected);

        return window;
    }

    static void onResize(Ui::Window& self)
    {
        self.setSize(kWindowSizeMin, kWindowSizeMax);

        // Resize & reposition widgets
        self.widgets[widx::frame].right = self.width - 1;
        self.widgets[widx::frame].bottom = self.height - 1;
        self.widgets[widx::title].right = self.width - 2;
        self.widgets[widx::close].left = self.width - 15;
        self.widgets[widx::close].right = self.width - 3;
        self.widgets[widx::panel].right = self.width - 1;
        self.widgets[widx::panel].bottom = self.height - 1;

        self.widgets[widx::sort_years].right = self.width - 1 - kPadding;
        self.widgets[widx::sort_years].left = self.widgets[widx::sort_years].right - kColumnYearsWidth + 1;
        self.widgets[widx::sort_title].right = self.widgets[widx::sort_years].left - 1;
        self.widgets[widx::scrollview].right = self.width - 1 - kPadding;
        self.widgets[widx::scrollview].bottom = self.height - 1 - kStatusBarClearance;
        self.widgets[widx::status_bar].top = self.height - 12;
        self.widgets[widx::status_bar].bottom = self.height - 2;
        self.widgets[widx::status_bar].right = self.width - kResizeHandleSize - kPadding;
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
        // Horizontal offsets of columns within the scrollview widget.
        const auto columnTitleOffset = 15;
        const auto columnYearsOffset = window.widgets[widx::sort_years].left - 5;

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

            // Draw track title.
            {
                auto point = Point(columnTitleOffset, y);
                StringId musicTitle = musicInfo.titleId;

                auto argsBuf = FormatArgumentsBuffer{};
                auto args = FormatArguments{ argsBuf };
                args.push(musicTitle);
                tr.drawStringLeft(point, window.getColour(WindowColour::secondary), textColour, args);
            }

            // Draw track years.
            {
                auto point = Point(columnYearsOffset, y);
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
                    point.x += kColumnYearsWidth - ScrollView::kScrollbarSize - 7;
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

            case sort_title:
                cycleSortMode(window, Jukebox::MusicSortMode::titleAscending, Jukebox::MusicSortMode::titleDescending);
                break;
            case sort_years:
                cycleSortMode(window, Jukebox::MusicSortMode::yearsDescending, Jukebox::MusicSortMode::yearsAscending);
                break;
        }
    }

    // 0x004C1799
    static void onScrollMouseDown(Ui::Window& window, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        const uint16_t currentRow = y / kRowHeight;
        if (currentRow > window.rowCount)
        {
            return;
        }
        const Jukebox::MusicId currentTrack = playlist[currentRow];

        auto& config = Config::get().audio;

        // Toggle the track in question.
        config.customJukebox[currentTrack] ^= true;

        // Do not allow there being 0 tracks enabled
        uint16_t numTracksSelected = std::ranges::count(config.customJukebox, true);
        if (numTracksSelected == 0)
        {
            config.customJukebox[currentTrack] = true;
            numTracksSelected = 1;
        }

        updateStatusBar(window, numTracksSelected);
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
        .onResize = onResize,
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
