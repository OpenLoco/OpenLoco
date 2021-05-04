#pragma once

#include "../Company.h"
#include "../Graphics/Gfx.h"
#include "../Localisation/StringManager.h"
#include "../Map/Tile.h"
#include "../Viewport.hpp"
#include "../Window.h"
#include <cstddef>

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

    void updateViewports();
    void update();
    window* getMainWindow();
    viewport* getMainViewport();
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
    void setCurrentRotation(int32_t value);

    void viewportShiftPixels(Ui::window* window, Ui::viewport* viewport, int16_t dX, int16_t dY);
    void viewportSetVisibility(viewport_visibility flags);
}

namespace OpenLoco::Vehicles
{
    struct VehicleBase;
    struct Car;
}

namespace OpenLoco::Ui::Windows
{
    void showError(string_id title, string_id message = StringIds::null, bool sound = true);

    void showGridlines();
    void hideGridlines();
    void showDirectionArrows();
    void hideDirectionArrows();

    namespace About
    {
        void open();
    }

    namespace AboutMusic
    {
        void open();
    }

    namespace BuildVehicle
    {
        window* open(uint32_t vehicle, uint32_t flags);
        void sub_4B92A5(Ui::window* window);
        void registerHooks();
    }

    namespace Cheats
    {
        window* open();
    }

    namespace CompanyFaceSelection
    {
        void open(CompanyId_t id);
    }

    namespace CompanyList
    {
        void openPerformanceIndexes();
        window* open();
    }

    namespace CompanyWindow
    {
        window* open(CompanyId_t companyId);
        window* openAndSetName();
        window* openChallenge(CompanyId_t companyId);
        window* openFinances(CompanyId_t companyId);
    }

    namespace Construction
    {
        window* openWithFlags(uint32_t flags);
        window* openAtTrack(window* main, Map::TrackElement* track, const Map::Pos2 pos);
        window* openAtRoad(window* main, Map::RoadElement* track, const Map::Pos2 pos);
        void setToTrackExtra(window* main, Map::TrackElement* track, const uint8_t bh, const Map::Pos2 pos);
        void setToRoadExtra(window* main, Map::RoadElement* track, const uint8_t bh, const Map::Pos2 pos);
        void sub_4A6FAC();
        void rotate(window* self);
        void registerHooks();
    }

    namespace DragVehiclePart
    {
        void open(Vehicles::Car& car);
    }

    namespace EditKeyboardShortcut
    {
        window* open(uint8_t shortcutIndex);
    }

    namespace Error
    {
        void open(string_id title, string_id message);
        void openWithCompetitor(string_id title, string_id message, uint8_t competitorId);
        void registerHooks();
    }

    namespace Industry
    {
        window* open(IndustryId_t id);
    }

    namespace IndustryList
    {
        window* open();
    }

    namespace KeyboardShortcuts
    {
        window* open();
    }

    namespace LandscapeGeneration
    {
        window* open();
    }

    namespace LandscapeGenerationConfirm
    {
        window* open(int32_t prompt_type);
    }

    namespace Main
    {
        void open();
    }

    namespace MapToolTip
    {
        void open();
        void setOwner(CompanyId_t company);
        void reset();
    }

    namespace MapWindow
    {
        void open();
        void centerOnViewPoint();
    }

    namespace MessageWindow
    {
        void open();
    }

    namespace MusicSelection
    {
        window* open();
    }
    namespace NewsWindow
    {
        void open(uint16_t messageIndex);
        void openLastMessage();
        void close(Ui::window* window);
    }

    namespace ObjectSelectionWindow
    {
        window* open();
    }

    namespace Options
    {
        window* open();
        window* openMusicSettings();
        constexpr uint8_t tab_offset_music = 2;
    }

    namespace PlayerInfoPanel
    {
        window* open();
        void invalidateFrame();
    }

    namespace ProgressBar
    {
        window* open(std::string_view captionString);
        void setProgress(uint8_t value);
        void close();
    }

    namespace PromptBrowse
    {
        enum browse_type : uint8_t
        {
            load = 1,
            save = 2
        };
        bool open(browse_type type, char* path, const char* filter, const char* title);
        void handleInput(uint32_t charCode, uint32_t keyCode);
        void registerHooks();
    }

    namespace PromptOkCancel
    {
        bool open(string_id okButtonStringId);
        void handleInput(uint32_t charCode, uint32_t keyCode);
    }

    namespace PromptSaveWindow
    {
        window* open(uint16_t savePromptType);
    }

    namespace ScenarioOptions
    {
        window* open();
    }

    namespace ScenarioSelect
    {
        window* open();
    }

    namespace Station
    {
        window* open(uint16_t id);
        void showStationCatchment(uint16_t windowNumber);
    }

    namespace StationList
    {
        window* open(CompanyId_t companyId);
        window* open(CompanyId_t companyId, uint8_t type);
    }

    namespace Terraform
    {
        window* open();
        void openClearArea();
        void openAdjustLand();
        void openAdjustWater();
        void openPlantTrees();
        void openBuildWalls();
        bool rotate(window*);
        void registerHooks();
    }

    namespace TextInput
    {
        void registerHooks();

        void openTextInput(Ui::window* w, string_id title, string_id message, string_id value, int callingWidget, void* valueArgs);
        void sub_4CE6C9(WindowType type, window_number number);
        void cancel();
        void handleInput(uint32_t charCode, uint32_t keyCode);
        void sub_4CE6FF();
    }

    namespace TileInspector
    {
        window* open();
    }

    namespace TimePanel
    {
        window* open();
        void invalidateFrame();
    }

    namespace TitleExit
    {
        window* open();
    }

    namespace TitleLogo
    {
        window* open();
    }

    namespace TitleMenu
    {
        window* open();
        void editorInit();
    }

    namespace TitleOptions
    {
        window* open();
    }

    namespace TitleVersion
    {
        window* open();
    }

    namespace ToolbarBottom::Editor
    {
        void open();
    }

    namespace ToolbarTop::Game
    {
        void open();
    }

    namespace ToolbarTop::Editor
    {
        void open();
    }

    namespace ToolTip
    {
        void registerHooks();
        void open(Ui::window* window, int32_t widgetIndex, int16_t x, int16_t y);
        void update(Ui::window* window, int32_t widgetIndex, string_id stringId, int16_t x, int16_t y);
        void set_52336E(bool value);
        void closeAndReset();
    }

    namespace Town
    {
        window* open(uint16_t townId);
    }

    namespace TownList
    {
        window* open();
    }

    namespace Tutorial
    {
        window* open();
    }

    namespace Vehicle
    {
        void registerHooks();
        namespace Main
        {
            window* open(const Vehicles::VehicleBase* vehicle);
        }
        namespace Details
        {
            window* open(const Vehicles::VehicleBase* vehicle);
            void scrollDrag(const Gfx::point_t& pos);
            void scrollDragEnd(const Gfx::point_t& pos);
        }
        namespace Common
        {
            int16_t sub_4B743B(uint8_t al, uint8_t ah, int16_t cx, int16_t dx, Vehicles::VehicleBase* vehicle, Gfx::drawpixelinfo_t* const pDrawpixelinfo);
        }
        bool rotate();
    }

    namespace VehicleList
    {
        window* open(CompanyId_t companyId, VehicleType type);
    }
}
