#pragma once

#include "../Company.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/StringManager.h"
#include "../Map/Tile.h"
#include "../Viewport.hpp"
#include "../Window.h"
#include <cstddef>

namespace OpenLoco
{
    struct vehicle;
}

namespace OpenLoco::ui::WindowManager
{
    enum class viewport_visibility
    {
        reset,
        undergroundView,
        heightMarksOnLand,
        overgroundView,
    };

    void init();
    void registerHooks();
    WindowType getCurrentModalType();
    void setCurrentModalType(WindowType type);
    window* get(size_t index);
    size_t indexOf(window* pWindow);
    size_t count();

    void update();
    window* getMainWindow();
    window* find(WindowType type);
    window* find(WindowType type, window_number number);
    window* findAt(int16_t x, int16_t y);
    window* findAt(Gfx::point_t point);
    window* findAtAlt(int16_t x, int16_t y);
    window* bringToFront(window* window);
    window* bringToFront(WindowType type, uint16_t id = 0);
    void invalidate(WindowType type);
    void invalidate(WindowType type, window_number number);
    void invalidateWidget(WindowType type, window_number number, uint8_t widget_index);
    void invalidateAllWindowsAfterInput();
    void close(WindowType type);
    void close(WindowType type, uint16_t id);
    void close(window* window);
    window* createWindow(WindowType type, Gfx::ui_size_t size, uint32_t flags, window_event_list* events);
    window* createWindow(WindowType type, Gfx::point_t origin, Gfx::ui_size_t size, uint32_t flags, window_event_list* events);
    window* createWindowCentred(WindowType type, Gfx::ui_size_t size, uint32_t flags, window_event_list* events);
    window* createWindow(WindowType type, Gfx::ui_size_t size, uint32_t flags, window_event_list* events);
    void drawSingle(Gfx::drawpixelinfo_t* dpi, window* w, int32_t left, int32_t top, int32_t right, int32_t bottom);
    void dispatchUpdateAll();
    void callEvent8OnAllWindows();
    void callEvent9OnAllWindows();
    void callViewportRotateEventOnAllWindows();
    void relocateWindows();
    void sub_4CEE0B(window* self);
    void sub_4B93A5(window_number number);
    void closeConstructionWindows();
    void closeTopmost();
    void allWheelInput();
    bool isInFront(ui::window* w);
    bool isInFrontAlt(ui::window* w);
    ui::window* findWindowShowing(const viewport_pos& position);
    void closeAllFloatingWindows();
    int32_t getCurrentRotation();

    void viewportShiftPixels(ui::window* window, ui::viewport* viewport, int16_t dX, int16_t dY);
    void viewportSetVisibility(viewport_visibility flags);
}

namespace OpenLoco::ui::windows
{
    window* openTitleVersion();
    window* openTitleExit();
    window* openTitleMenu();
    window* openTitleLogo();
    void open_about_window();

    bool promptOkCancel(string_id okButtonStringId);

    void showError(string_id title, string_id message = string_ids::null, bool sound = true);

    void editorInit();

    void showGridlines();
    void hideGridlines();
    void showDirectionArrows();
    void hideDirectionArrows();
}

namespace OpenLoco::ui::about
{
    void open();
}

namespace OpenLoco::ui::KeyboardShortcuts
{
    window* open();
}

namespace OpenLoco::ui::EditKeyboardShortcut
{
    window* open(uint8_t shortcutIndex);
}

namespace OpenLoco::ui::AboutMusic
{
    void open();
}

namespace OpenLoco::ui::windows::construction
{
    window* openWithFlags(uint32_t flags);
    window* openAtTrack(window* main, OpenLoco::map::track_element* track, const OpenLoco::map::map_pos pos);
    window* openAtRoad(window* main, OpenLoco::map::road_element* track, const OpenLoco::map::map_pos pos);
    void setToTrackExtra(window* main, OpenLoco::map::track_element* track, const uint8_t bh, const OpenLoco::map::map_pos pos);
    void setToRoadExtra(window* main, OpenLoco::map::road_element* track, const uint8_t bh, const OpenLoco::map::map_pos pos);
    void sub_4A6FAC();
    void registerHooks();
}

namespace OpenLoco::ui::windows::industry
{
    window* open(industry_id_t id);
}

namespace OpenLoco::ui::windows::industry_list
{
    window* open();
}

namespace OpenLoco::ui::windows::LandscapeGeneration
{
    window* open();
}

namespace OpenLoco::ui::windows::LandscapeGenerationConfirm
{
    window* open(int32_t prompt_type);
}

namespace OpenLoco::ui::windows::map
{
    void open();
    void centerOnViewPoint();
}

namespace OpenLoco::ui::windows::music_selection
{
    window* open();
}

namespace OpenLoco::ui::windows::error
{
    void open(string_id title, string_id message);
    void openWithCompetitor(string_id title, string_id message, uint8_t competitorId);
    void registerHooks();
}

namespace OpenLoco::ui::options
{
    window* open();
    window* openMusicSettings();
    constexpr uint8_t tab_offset_music = 2;
}

namespace OpenLoco::ui::prompt_browse
{
    enum browse_type : uint8_t
    {
        load = 1,
        save = 2
    };
    bool open(browse_type type, char* path, const char* filter, const char* title);
    void registerHooks();
}

namespace OpenLoco::ui::windows::ScenarioOptions
{
    window* open();
}

namespace OpenLoco::ui::windows::station
{
    window* open(uint16_t id);
    void showStationCatchment(uint16_t windowNumber);
}

namespace OpenLoco::ui::windows::station_list
{
    window* open(company_id_t companyId);
    window* open(company_id_t companyId, uint8_t type);
}

namespace OpenLoco::ui::windows::terraform
{
    window* open();
    void openClearArea();
    void openAdjustLand();
    void openAdjustWater();
    void openPlantTrees();
    void openBuildWalls();
    void registerHooks();
}

namespace OpenLoco::ui::textinput
{
    void registerHooks();

    void openTextinput(ui::window* w, string_id title, string_id message, string_id value, int callingWidget, void* valueArgs);
    void sub_4CE6C9(WindowType type, window_number number);
    void cancel();
    void sub_4CE910(int eax, int ebx);
    void sub_4CE6FF();
}

namespace OpenLoco::ui::title_options
{
    window* open();
}

namespace OpenLoco::ui::windows::toolbar_top::game
{
    void open();
}

namespace OpenLoco::ui::windows::toolbar_top::editor
{
    void open();
}

namespace OpenLoco::ui::tooltip
{
    void registerHooks();
    void open(ui::window* window, int32_t widgetIndex, int16_t x, int16_t y);
    void update(ui::window* window, int32_t widgetIndex, string_id stringId, int16_t x, int16_t y);
    void set_52336E(bool value);
    void closeAndReset();
}

namespace OpenLoco::ui::windows::town
{
    window* open(uint16_t townId);
}

namespace OpenLoco::ui::windows::town_list
{
    window* open();
}

namespace OpenLoco::ui::vehicle
{
    void registerHooks();
    namespace main
    {
        window* open(const OpenLoco::vehicle* vehicle);
    }
}

namespace OpenLoco::ui::windows::vehicle_list
{
    window* open(uint16_t companyId, uint8_t type);
}

namespace OpenLoco::ui::BuildVehicle
{
    window* open(uint32_t vehicle, uint32_t flags);
    void sub_4B92A5(ui::window* window);
    void registerHooks();
}

namespace OpenLoco::ui::MessageWindow
{
    void open();
}

namespace OpenLoco::ui::NewsWindow
{
    void open(uint16_t messageIndex);
}

namespace OpenLoco::ui::windows::CompanyFaceSelection
{
    void open(company_id_t id);
}

namespace OpenLoco::ui::windows::CompanyList
{
    void openPerformanceIndexes();
    window* open();
}

namespace OpenLoco::ui::windows::CompanyWindow
{
    window* open(company_id_t companyId);
    window* openAndSetName();
    window* openChallenge(company_id_t companyId);
    window* openFinances(company_id_t companyId);
}

namespace OpenLoco::ui::windows::ObjectSelectionWindow
{
    window* open();
}

namespace OpenLoco::ui::windows::PlayerInfoPanel
{
    window* open();
}

namespace OpenLoco::ui::TimePanel
{
    window* open();
}

namespace OpenLoco::ui::windows::toolbar_bottom::editor
{
    void open();
}
