#include "Audio/Audio.h"
#include "Config.h"
#include "Jukebox.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "SceneManager.h"
#include "Ui/Dropdown.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/DropdownWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/WindowManager.h"

#include <cassert>

namespace OpenLoco::Ui::Windows::MusicJukebox
{
    static constexpr Ui::Size kWindowSize = { 366, 109 };

    namespace Widx
    {
        enum
        {
            frame,
            caption,
            close_button,
            panel,
            currently_playing_label,
            currently_playing,
            currently_playing_btn,
            music_controls_stop,
            music_controls_play,
            music_controls_next,
            music_playlist,
            music_playlist_btn,
            edit_selection
        };
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { (uint16_t)(kWindowSize.width - 2), 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::jukebox_window_title),
        Widgets::ImageButton({ (int16_t)(kWindowSize.width - 15), 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { kWindowSize.width, 102 }, WindowColour::secondary),
        Widgets::Label({ 10, 27 }, { 145, 12 }, WindowColour::secondary, ContentAlign::left, StringIds::currently_playing),
        Widgets::dropdownWidgets({ 160, 27 }, { 196, 12 }, WindowColour::secondary, StringIds::stringid),
        Widgets::ImageButton({ 10, 42 }, { 24, 24 }, WindowColour::secondary, ImageIds::music_controls_stop, StringIds::music_controls_stop_tip),
        Widgets::ImageButton({ 34, 42 }, { 24, 24 }, WindowColour::secondary, ImageIds::music_controls_play, StringIds::music_controls_play_tip),
        Widgets::ImageButton({ 58, 42 }, { 24, 24 }, WindowColour::secondary, ImageIds::music_controls_next, StringIds::music_controls_next_tip),
        Widgets::dropdownWidgets({ 10, 69 }, { 346, 12 }, WindowColour::secondary, StringIds::stringid),
        Widgets::Button({ 183, 86 }, { 173, 12 }, WindowColour::secondary, StringIds::edit_music_selection, StringIds::edit_music_selection_tip)

    );

    static void currentlyPlayingMouseDown(const Window& self);
    static void currentlyPlayingDropdown(Window& self, int16_t itemIndex);
    static void stopMusic(Window& self);
    static void playMusic(Window& self);
    static void playNextSong(Window& self);
    static void musicPlaylistMouseDown(const Window& self);
    static void musicPlaylistDropdown(Window& self, int16_t itemIndex);

    // 0x004C0217, 0x004C0217
    static void prepareDraw(Window& self)
    {
        self.widgets[Widx::frame].right = self.width - 1;
        self.widgets[Widx::frame].bottom = self.height - 1;
        self.widgets[Widx::panel].right = self.width - 1;
        self.widgets[Widx::panel].bottom = self.height - 1;
        self.widgets[Widx::caption].right = self.width - 2;
        self.widgets[Widx::close_button].left = self.width - 15;
        self.widgets[Widx::close_button].right = self.width - 15 + 12;

        // Currently playing music track
        {
            StringId songName = StringIds::music_none;
            if (SceneManager::isPlayMode())
            {
                songName = Jukebox::getSelectedTrackTitleId();
            }
            else if (SceneManager::isTitleMode())
            {
                auto& cfg = Config::get();
                if (cfg.audio.playTitleMusic)
                {
                    songName = StringIds::music_locomotion_title;
                }
            }
            auto args = FormatArguments(self.widgets[Widx::currently_playing].textArgs);
            args.push(songName);
        }

        self.activatedWidgets = 0;
        self.disabledWidgets = 0;

        // Jukebox controls (stop/play/skip)
        if (!SceneManager::isPlayMode())
        {
            self.disabledWidgets |= (1ULL << Widx::currently_playing) | (1ULL << Widx::currently_playing_btn) | (1ULL << Widx::music_controls_play) | (1ULL << Widx::music_controls_stop) | (1ULL << Widx::music_controls_next);
        }
        else if (Jukebox::isMusicPlaying())
        {
            // Play button appears pressed
            self.activatedWidgets |= (1ULL << Widx::music_controls_play);
        }
        else
        {
            // Stop button appears pressed
            self.activatedWidgets |= (1ULL << Widx::music_controls_stop);
        }

        // Selected playlist
        {
            static constexpr StringId playlist_string_ids[] = {
                StringIds::play_only_music_from_current_era,
                StringIds::play_all_music,
                StringIds::play_custom_music_selection,
            };

            auto args = FormatArguments(self.widgets[Widx::music_playlist].textArgs);

            StringId selectedPlaylistStringId = playlist_string_ids[enumValue(Config::get().audio.playlist)];
            args.push(selectedPlaylistStringId);
        }

        // Edit custom playlist music selection button
        if (Config::get().audio.playlist != Config::MusicPlaylistType::custom)
        {
            self.disabledWidgets |= (1ULL << Widx::edit_selection);
        }
    }

    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        self.draw(drawingCtx);
    }

    static void onMouseUp(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
    {
        switch (wi)
        {
            case Widx::close_button:
                WindowManager::close(&self);
                return;

            case Widx::music_controls_stop:
                stopMusic(self);
                return;

            case Widx::music_controls_play:
                playMusic(self);
                return;

            case Widx::music_controls_next:
                playNextSong(self);
                return;

            case Widx::edit_selection:
                MusicSelection::open();
                return;
        }
    }

    static void onMouseDown(Window& self, WidgetIndex_t wi, [[maybe_unused]] const WidgetId id)
    {
        switch (wi)
        {
            case Widx::music_playlist_btn:
                musicPlaylistMouseDown(self);
                break;
            case Widx::currently_playing_btn:
                currentlyPlayingMouseDown(self);
                break;
        }
    }

    static void onDropdown(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id, int16_t itemIndex)
    {
        switch (widgetIndex)
        {
            case Widx::music_playlist_btn:
                musicPlaylistDropdown(self, itemIndex);
                break;
            case Widx::currently_playing_btn:
                currentlyPlayingDropdown(self, itemIndex);
                break;
        }
    }

    // 0x004C0875
    static void currentlyPlayingMouseDown(const Window& self)
    {
        auto tracks = Jukebox::makeSelectedPlaylist();

        auto& dropdown = self.widgets[Widx::currently_playing];
        Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), tracks.size(), 0x80);

        int index = -1;
        for (auto track : tracks)
        {
            index++;
            Dropdown::add(index, StringIds::dropdown_stringid, Jukebox::getMusicInfo(track).titleId);
            if (track == Jukebox::getCurrentTrack())
            {
                Dropdown::setItemSelected(index);
            }
        }
    }

    // 0x004C09F8
    static void currentlyPlayingDropdown(Window& self, int16_t ax)
    {
        if (ax == -1)
        {
            return;
        }

        auto track = Jukebox::makeSelectedPlaylist().at(ax);
        if (Jukebox::requestTrack(track))
        {
            self.invalidate();
        }
    }

    // 0x004C0778
    static void stopMusic(Window& self)
    {
        if (Jukebox::disableMusic())
        {
            self.invalidate();
        }
    }

    // 0x004C07A4
    static void playMusic(Window& self)
    {
        if (Jukebox::enableMusic())
        {
            self.invalidate();
        }
    }

    // 0x004C07C4
    static void playNextSong(Window& self)
    {
        if (Jukebox::skipCurrentTrack())
        {
            self.invalidate();
        }
    }

    // 0x004C07E4
    static void musicPlaylistMouseDown(const Window& self)
    {
        auto& dropdown = self.widgets[Widx::music_playlist];
        Dropdown::show(self.x + dropdown.left, self.y + dropdown.top, dropdown.width() - 4, dropdown.height(), self.getColour(WindowColour::secondary), 3, 0x80);

        Dropdown::add(0, StringIds::dropdown_stringid, StringIds::play_only_music_from_current_era);
        Dropdown::add(1, StringIds::dropdown_stringid, StringIds::play_all_music);
        Dropdown::add(2, StringIds::dropdown_stringid, StringIds::play_custom_music_selection);

        Dropdown::setItemSelected(enumValue(Config::get().audio.playlist));
    }

    // 0x004C084A
    static void musicPlaylistDropdown(Window& self, int16_t index)
    {
        if (index == -1)
        {
            return;
        }

        auto& cfg = Config::get().audio;
        cfg.playlist = Config::MusicPlaylistType(index);
        Config::write();

        self.invalidate();

        if (!SceneManager::isTitleMode()) // Prevents title music from stopping
        {
            Audio::revalidateCurrentTrack();
        }

        WindowManager::close(WindowType::musicSelection);
    }

    // 0x004C04E0, 0x004C0A37
    static void onUpdate(Window& self)
    {
        self.frameNo += 1;
        self.callPrepareDraw();
        WindowManager::invalidateWidget(self.type, self.number, self.currentTab + 4);
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    // 0x004BF823
    Window* open()
    {
        Window* window = WindowManager::bringToFront(WindowType::musicJukebox);
        if (window != nullptr)
        {
            return window;
        }

        window = WindowManager::createWindowCentred(
            WindowType::musicJukebox,
            kWindowSize,
            WindowFlags::none,
            kEvents);

        window->setWidgets(_widgets);
        window->number = 0;
        window->currentTab = 0;
        window->frameNo = 0;
        window->rowHover = -1;
        window->object = nullptr;

        auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->windowTitlebarColour);
        window->setColour(WindowColour::secondary, interface->windowOptionsColour);

        window->holdableWidgets = 0;
        window->eventHandlers = &kEvents;
        window->activatedWidgets = 0;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        return window;
    }
}
