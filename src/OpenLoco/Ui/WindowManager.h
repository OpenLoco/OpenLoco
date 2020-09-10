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

namespace OpenLoco::Ui::WindowManager
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
    bool isInFront(Ui::window* w);
    bool isInFrontAlt(Ui::window* w);
    Ui::window* findWindowShowing(const viewport_pos& position);
    void closeAllFloatingWindows();
    int32_t getCurrentRotation();

    void viewportShiftPixels(Ui::window* window, Ui::viewport* viewport, int16_t dX, int16_t dY);
    void viewportSetVisibility(viewport_visibility flags);
}

namespace OpenLoco::Ui::Windows::Main
{
    void open();
}

namespace OpenLoco::Ui::Windows
{
    window* openTitleVersion();
    window* openTitleExit();
    window* openTitleMenu();
    window* openTitleLogo();

    bool promptOkCancel(string_id okButtonStringId);

    void showError(string_id title, string_id message = StringIds::null, bool sound = true);

    void editorInit();

    void showGridlines();
    void hideGridlines();
    void showDirectionArrows();
    void hideDirectionArrows();
}

namespace OpenLoco::Ui::About
{
    void open();
}

namespace OpenLoco::Ui::KeyboardShortcuts
{
    window* open();
}

namespace OpenLoco::Ui::EditKeyboardShortcut
{
    window* open(uint8_t shortcutIndex);
}

namespace OpenLoco::Ui::AboutMusic
{
    void open();
}

namespace OpenLoco::Ui::Windows::Construction
{
    window* openWithFlags(uint32_t flags);
    window* openAtTrack(window* main, OpenLoco::Map::track_element* track, const OpenLoco::Map::map_pos pos);
    window* openAtRoad(window* main, OpenLoco::Map::road_element* track, const OpenLoco::Map::map_pos pos);
    void setToTrackExtra(window* main, OpenLoco::Map::track_element* track, const uint8_t bh, const OpenLoco::Map::map_pos pos);
    void setToRoadExtra(window* main, OpenLoco::Map::road_element* track, const uint8_t bh, const OpenLoco::Map::map_pos pos);
    void sub_4A6FAC();
    void registerHooks();
}

namespace OpenLoco::Ui::Windows::Industry
{
    window* open(industry_id_t id);
}

namespace OpenLoco::Ui::Windows::IndustryList
{
    window* open();
}

namespace OpenLoco::Ui::Windows::LandscapeGeneration
{
    window* open();
}

namespace OpenLoco::Ui::Windows::LandscapeGenerationConfirm
{
    window* open(int32_t prompt_type);
}

namespace OpenLoco::Ui::Windows::Map
{
    void open();
    void centerOnViewPoint();
}

namespace OpenLoco::Ui::Windows::MusicSelection
{
    window* open();
}

namespace OpenLoco::Ui::Windows::Error
{
    void open(string_id title, string_id message);
    void openWithCompetitor(string_id title, string_id message, uint8_t competitorId);
    void registerHooks();
}

namespace OpenLoco::Ui::Options
{
    window* open();
    window* openMusicSettings();
    constexpr uint8_t tab_offset_music = 2;
}

namespace OpenLoco::Ui::PromptBrowse
{
    enum browse_type : uint8_t
    {
        load = 1,
        save = 2
    };
    bool open(browse_type type, char* path, const char* filter, const char* title);
    void registerHooks();
}

namespace OpenLoco::Ui::Windows::ScenarioOptions
{
    window* open();
}

namespace OpenLoco::Ui::Windows::Station
{
    window* open(uint16_t id);
    void showStationCatchment(uint16_t windowNumber);
}

namespace OpenLoco::Ui::Windows::StationList
{
    window* open(company_id_t companyId);
    window* open(company_id_t companyId, uint8_t type);
}

namespace OpenLoco::Ui::Windows::Terraform
{
    window* open();
    void openClearArea();
    void openAdjustLand();
    void openAdjustWater();
    void openPlantTrees();
    void openBuildWalls();
    void registerHooks();
}

namespace OpenLoco::Ui::TextInput
{
    void registerHooks();

    void openTextInput(Ui::window* w, string_id title, string_id message, string_id value, int callingWidget, void* valueArgs);
    void sub_4CE6C9(WindowType type, window_number number);
    void cancel();
    void sub_4CE910(int eax, int ebx);
    void calculateTextOffset(int16_t containerWidth);
    void sub_4CE6FF();
}

namespace OpenLoco::Ui::TitleOptions
{
    window* open();
}

namespace OpenLoco::Ui::Windows::ToolbarTop::Game
{
    void open();
}

namespace OpenLoco::Ui::Windows::ToolbarTop::Editor
{
    void open();
}

namespace OpenLoco::Ui::ToolTip
{
    void registerHooks();
    void open(Ui::window* window, int32_t widgetIndex, int16_t x, int16_t y);
    void update(Ui::window* window, int32_t widgetIndex, string_id stringId, int16_t x, int16_t y);
    void set_52336E(bool value);
    void closeAndReset();
}

namespace OpenLoco::Ui::Windows::Town
{
    window* open(uint16_t townId);
}

namespace OpenLoco::Ui::Windows::TownList
{
    window* open();
}

namespace OpenLoco
{
    struct vehicle;
}

namespace OpenLoco::Things::Vehicle
{
    struct Car;
}

namespace OpenLoco::Ui::Windows::DragVehiclePart
{
    void open(OpenLoco::Things::Vehicle::Car& car);
}

namespace OpenLoco::Ui::Vehicle
{
    void registerHooks();
    namespace Main
    {
        window* open(const OpenLoco::vehicle* vehicle);
        window* openDetails(const OpenLoco::vehicle* vehicle);
    }
}

namespace OpenLoco::Ui::Windows::VehicleList
{
    window* open(uint16_t companyId, uint8_t type);
}

namespace OpenLoco::Ui::BuildVehicle
{
    window* open(uint32_t vehicle, uint32_t flags);
    void sub_4B92A5(Ui::window* window);
    void registerHooks();
}

namespace OpenLoco::Ui::MessageWindow
{
    void open();
}

namespace OpenLoco::Ui::NewsWindow
{
    void open(uint16_t messageIndex);
    void openLastMessage();
    void close(Ui::window* window);
}

namespace OpenLoco::Ui::Windows::CompanyFaceSelection
{
    void open(company_id_t id);
}

namespace OpenLoco::Ui::Windows::CompanyList
{
    void openPerformanceIndexes();
    window* open();
}

namespace OpenLoco::Ui::Windows::CompanyWindow
{
    window* open(company_id_t companyId);
    window* openAndSetName();
    window* openChallenge(company_id_t companyId);
    window* openFinances(company_id_t companyId);
}

namespace OpenLoco::Ui::Windows::ObjectSelectionWindow
{
    window* open();
}

namespace OpenLoco::Ui::Windows::PlayerInfoPanel
{
    window* open();
}

namespace OpenLoco::Ui::Windows::ScenarioSelect
{
    window* open();
}

namespace OpenLoco::Ui::TimePanel
{
    window* open();
}

namespace OpenLoco::Ui::Windows::ToolbarBottom::Editor
{
    void open();
}
