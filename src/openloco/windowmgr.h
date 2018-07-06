#pragma once

#include "Window.h"
#include "graphics/gfx.h"
#include "localisation/stringmgr.h"
#include "town.h"
#include <cstddef>

namespace openloco::ui
{
    enum class WindowType : uint8_t
    {
        main = 0,
        topToolbar = 1,
        playerInfoToolbar = 2,
        timeToolbar = 3,

        tooltip = 6,
        dropdown = 7,

        about = 9,
        // The Atari credits window is no longer used
        aboutAtari = 10,
        aboutMusic = 11,
        error = 12,
        construction = 13,
        saveGamePrompt = 14,
        terraform = 15,
        titleMenu = 16,
        titleExit = 17,
        scenarioSelect = 18,
        keyboardShortcuts = 19,
        editKeyboardShortcut = 20,
        map = 21,
        titleLogo = 22,
        vehicle = 23,
        station = 24,

        company = 26,
        vehicleList = 27,
        buildVehicle = 28,
        stationList = 29,

        objectSelection = 31,
        townList = 32,
        town = 33,
        industry = 34,
        industryList = 35,

        messages = 37,

        multiplayer = 39,
        options = 40,
        musicSelection = 41,
        companyFaceSelection = 42,
        landscapeGeneration = 43,

        scenarioOptions = 45,

        wt_47 = 47,
        companyList = 48,
        tutorial = 49,
        confirmDisplayModePrompt = 50,
        textInput = 51,
        fileBrowser = 52,

        confirmation = 54,
        titleVersion = 55,
        titleOptions = 56,

        undefined = 255
    };
}

namespace openloco::ui::windowmgr
{
    void register_hooks();
    WindowType current_modal_type();
    void current_modal_type(WindowType type);
    Window* get(size_t index);
    size_t num_windows();

    void update();
    Window* get_main();
    Window* find(WindowType type);
    Window* find(WindowType type, window_number number);
    Window* find_at(int16_t x, int16_t y);
    Window* find_at_alt(int16_t x, int16_t y);
    Window* bring_to_front(Window* window);
    Window* bring_to_front(WindowType type, uint16_t id);
    void invalidate(WindowType type);
    void invalidate(WindowType type, window_number number);
    void invalidate_widget(WindowType type, window_number number, uint8_t widget_index);
    void invalidate_all_windows_after_input();
    void close(WindowType type);
    void close(WindowType type, uint16_t id);
    void close(Window* window);
    Window* create_window(WindowType type, int32_t x, int32_t y, int32_t width, int32_t height, int32_t flags, window_event_list* events);
    Window* create_window_centred(WindowType type, int32_t width, int32_t height, int32_t flags, window_event_list* events);
    void draw_single(gfx::GraphicsContext* _context, Window* w, int32_t left, int32_t top, int32_t right, int32_t bottom);
    void dispatch_update_all();
    void call_event_viewport_rotate_on_all_windows();
    void relocate_windows();
    void sub_4CEE0B(Window* self);
    void sub_4B93A5(window_number number);
    void close_topmost();
    void all_wheel_input();
}

namespace openloco::windows::AboutWindow
{
    void open();
}

namespace openloco::windows::AboutMusicWindow
{
    void open();
}

namespace openloco::windows::ConfirmationWindow
{
    bool open(string_id okButtonStringId);
}

namespace openloco::windows::ConstructionWindow
{
    void onClick(ui::Window& w, uint16_t widgetIndex);
}

namespace openloco::windows::MapWindow
{
    void centerOnViewpoint();
}

namespace openloco::windows::OptionsWindow
{
    void open();
}

namespace openloco::windows::StationWindow
{
    void drawScroll2(ui::Window& w, gfx::GraphicsContext& context);
}

namespace openloco::windows::FileBrowserWindow
{
    enum class BrowseType
    {
        load = 1,
        save = 2
    };
    bool open(BrowseType type, char* path, const char* filter, const char* title);
    void registerHooks();
}

namespace openloco::windows::TextInputWindow
{
    void registerHooks();
    void open(ui::Window* w, string_id title, string_id message, string_id value, int callingWidget, void* valueArgs);
    void sub_4CE6C9(ui::WindowType type, ui::window_number number);
    void cancel();
    void sub_4CE910(int eax, int ebx);
    void sub_4CE6FF();
}

namespace openloco::windows::TitleExitWindow
{
    ui::Window* open();
}

namespace openloco::windows::TitleLogoWindow
{
    ui::Window* open();
}

namespace openloco::windows::TitleMenuWindow
{
    ui::Window* open();
}

namespace openloco::windows::TitleOptionsWindow
{
    ui::Window* open();
}

namespace openloco::windows::TitleVersionWindow
{
    ui::Window* open();
}

namespace openloco::windows::TooltipWindow
{
    void registerHooks();
    void open(ui::Window* window, int32_t widgetIndex, int16_t x, int16_t y);
    void update(ui::Window* window, int32_t widgetIndex, string_id stringId, int16_t x, int16_t y);
}

namespace openloco::windows::TownWindow
{
    ui::Window* open(town_id_t townId);
    void sub_498E9B(ui::Window* w);
}
