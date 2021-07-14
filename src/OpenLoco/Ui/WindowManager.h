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
    enum class ViewportVisibility
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
    Window* get(size_t index);
    size_t indexOf(Window* pWindow);
    size_t count();

    void updateViewports();
    void update();
    Window* getMainWindow();
    Viewport* getMainViewport();
    Window* find(WindowType type);
    Window* find(WindowType type, WindowNumber_t number);
    Window* findAt(int16_t x, int16_t y);
    Window* findAt(Gfx::point_t point);
    Window* findAtAlt(int16_t x, int16_t y);
    Window* bringToFront(Window* window);
    Window* bringToFront(WindowType type, uint16_t id = 0);
    void invalidate(WindowType type);
    void invalidate(WindowType type, WindowNumber_t number);
    void invalidateWidget(WindowType type, WindowNumber_t number, uint8_t widget_index);
    void invalidateAllWindowsAfterInput();
    void close(WindowType type);
    void close(WindowType type, uint16_t id);
    void close(Window* window);
    Window* createWindow(WindowType type, Gfx::ui_size_t size, uint32_t flags, WindowEventList* events);
    Window* createWindow(WindowType type, Gfx::point_t origin, Gfx::ui_size_t size, uint32_t flags, WindowEventList* events);
    Window* createWindowCentred(WindowType type, Gfx::ui_size_t size, uint32_t flags, WindowEventList* events);
    Window* createWindow(WindowType type, Gfx::ui_size_t size, uint32_t flags, WindowEventList* events);
    void drawSingle(Gfx::Context* context, Window* w, int32_t left, int32_t top, int32_t right, int32_t bottom);
    void dispatchUpdateAll();
    void callEvent8OnAllWindows();
    void callEvent9OnAllWindows();
    void callViewportRotateEventOnAllWindows();
    void relocateWindows();
    void sub_4CEE0B(Window* self);
    void sub_4B93A5(WindowNumber_t number);
    void closeConstructionWindows();
    void closeTopmost();
    void allWheelInput();
    bool isInFront(Ui::Window* w);
    bool isInFrontAlt(Ui::Window* w);
    Ui::Window* findWindowShowing(const viewport_pos& position);
    void closeAllFloatingWindows();
    int32_t getCurrentRotation();
    void setCurrentRotation(int32_t value);

    void viewportShiftPixels(Ui::Window* window, Ui::Viewport* viewport, int16_t dX, int16_t dY);
    void viewportSetVisibility(ViewportVisibility flags);
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
        Window* open(uint32_t vehicle, uint32_t flags);
        void sub_4B92A5(Ui::Window* window);
        void registerHooks();
    }

    namespace Cheats
    {
        Window* open();
    }

    namespace CompanyFaceSelection
    {
        void open(CompanyId_t id);
    }

    namespace CompanyList
    {
        void openPerformanceIndexes();
        Window* open();
    }

    namespace CompanyWindow
    {
        Window* open(CompanyId_t companyId);
        Window* openAndSetName();
        Window* openChallenge(CompanyId_t companyId);
        Window* openFinances(CompanyId_t companyId);
    }

    namespace Construction
    {
        Window* openWithFlags(uint32_t flags);
        Window* openAtTrack(Window* main, Map::TrackElement* track, const Map::Pos2 pos);
        Window* openAtRoad(Window* main, Map::RoadElement* track, const Map::Pos2 pos);
        void setToTrackExtra(Window* main, Map::TrackElement* track, const uint8_t bh, const Map::Pos2 pos);
        void setToRoadExtra(Window* main, Map::RoadElement* track, const uint8_t bh, const Map::Pos2 pos);
        void sub_4A6FAC();
        void rotate(Window* self);
        void registerHooks();
    }

    namespace DragVehiclePart
    {
        void open(Vehicles::Car& car);
    }

    namespace EditKeyboardShortcut
    {
        Window* open(uint8_t shortcutIndex);
    }

    namespace Error
    {
        void open(string_id title, string_id message);
        void openWithCompetitor(string_id title, string_id message, uint8_t competitorId);
        void registerHooks();
    }

    namespace Industry
    {
        Window* open(IndustryId_t id);
    }

    namespace IndustryList
    {
        Window* open();
    }

    namespace KeyboardShortcuts
    {
        Window* open();
    }

    namespace LandscapeGeneration
    {
        Window* open();
    }

    namespace LandscapeGenerationConfirm
    {
        Window* open(int32_t prompt_type);
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
        Window* open();
    }
    namespace NewsWindow
    {
        void open(uint16_t messageIndex);
        void openLastMessage();
        void close(Ui::Window* window);
    }

    namespace ObjectSelectionWindow
    {
        Window* open();
    }

    namespace Options
    {
        Window* open();
        Window* openMusicSettings();
        constexpr uint8_t tab_offset_music = 2;
    }

    namespace PlayerInfoPanel
    {
        Window* open();
        void invalidateFrame();
    }

    namespace ProgressBar
    {
        Window* open(std::string_view captionString);
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
        Window* open(uint16_t savePromptType);
    }

    namespace ScenarioOptions
    {
        Window* open();
    }

    namespace ScenarioSelect
    {
        Window* open();
    }

    namespace Station
    {
        Window* open(uint16_t id);
        void showStationCatchment(uint16_t windowNumber);
    }

    namespace StationList
    {
        Window* open(CompanyId_t companyId);
        Window* open(CompanyId_t companyId, uint8_t type);
    }

    namespace Terraform
    {
        Window* open();
        void openClearArea();
        void openAdjustLand();
        void openAdjustWater();
        void openPlantTrees();
        void openBuildWalls();
        bool rotate(Window*);
        void registerHooks();
    }

    namespace TextInput
    {
        void registerHooks();

        void openTextInput(Ui::Window* w, string_id title, string_id message, string_id value, int callingWidget, void* valueArgs);
        void sub_4CE6C9(WindowType type, WindowNumber_t number);
        void cancel();
        void handleInput(uint32_t charCode, uint32_t keyCode);
        void sub_4CE6FF();
    }

    namespace TileInspector
    {
        Window* open();
    }

    namespace TimePanel
    {
        Window* open();
        void invalidateFrame();
    }

    namespace TitleExit
    {
        Window* open();
    }

    namespace TitleLogo
    {
        Window* open();
    }

    namespace TitleMenu
    {
        Window* open();
        void editorInit();
    }

    namespace TitleOptions
    {
        Window* open();
    }

    namespace TitleVersion
    {
        Window* open();
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
        void open(Ui::Window* window, int32_t widgetIndex, int16_t x, int16_t y);
        void update(Ui::Window* window, int32_t widgetIndex, string_id stringId, int16_t x, int16_t y);
        void set_52336E(bool value);
        void closeAndReset();
    }

    namespace Town
    {
        Window* open(uint16_t townId);
    }

    namespace TownList
    {
        Window* open();
        void reset();
    }

    namespace Tutorial
    {
        Window* open();
    }

    namespace Vehicle
    {
        void registerHooks();
        namespace Main
        {
            Window* open(const Vehicles::VehicleBase* vehicle);
        }
        namespace Details
        {
            Window* open(const Vehicles::VehicleBase* vehicle);
            void scrollDrag(const Gfx::point_t& pos);
            void scrollDragEnd(const Gfx::point_t& pos);
        }
        namespace Common
        {
            int16_t sub_4B743B(uint8_t al, uint8_t ah, int16_t cx, int16_t dx, Vehicles::VehicleBase* vehicle, Gfx::Context* const pDrawpixelinfo);
        }
        bool rotate();
    }

    namespace VehicleList
    {
        Window* open(CompanyId_t companyId, VehicleType type);
    }
}
