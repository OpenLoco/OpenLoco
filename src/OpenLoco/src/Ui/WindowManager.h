#pragma once

#include "Localisation/StringManager.h"
#include "Window.h"
#include <OpenLoco/Engine/World.hpp>
#include <cstddef>
#include <functional>
#include <string_view>

namespace OpenLoco
{
    enum class LoadOrQuitMode : uint16_t;
}

namespace OpenLoco::Gfx
{
    struct RenderTarget;
}
namespace OpenLoco::Ui
{
    struct Viewport;
    struct Window;
}
namespace OpenLoco::World
{
    struct TrackElement;
    struct RoadElement;
    struct TreeElement;
}

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
    void setWindowColours(uint8_t i, AdvancedColour colour);
    WindowType getCurrentModalType();
    void setCurrentModalType(WindowType type);
    Window* get(size_t index);
    size_t indexOf(const Window& pWindow);
    size_t count();

    void updateViewports();
    void update();
    void updateDaily();
    Window* getMainWindow();
    Viewport* getMainViewport();
    Window* find(WindowType type);
    Window* find(WindowType type, WindowNumber_t number);
    Window* findAt(int16_t x, int16_t y);
    Window* findAt(Ui::Point point);
    Window* findAtAlt(int16_t x, int16_t y);
    Window* bringToFront(Window& window);
    Window* bringToFront(WindowType type, uint16_t id = 0);
    void invalidate(WindowType type);
    void invalidate(WindowType type, WindowNumber_t number);
    void invalidateWidget(WindowType type, WindowNumber_t number, uint8_t widgetIndex);
    void invalidateAllWindowsAfterInput();
    void close(WindowType type);
    void close(WindowType type, uint16_t id);
    void close(Window* window);
    Window* createWindow(WindowType type, Ui::Size size, WindowFlags flags, const WindowEventList& events);
    Window* createWindow(WindowType type, Ui::Point origin, Ui::Size size, WindowFlags flags, const WindowEventList& events);
    Window* createWindowCentred(WindowType type, Ui::Size size, WindowFlags flags, const WindowEventList& events);
    Window* createWindow(WindowType type, Ui::Size size, WindowFlags flags, const WindowEventList& events);
    void drawSingle(Gfx::RenderTarget* rt, Window* w, int32_t left, int32_t top, int32_t right, int32_t bottom);
    void dispatchUpdateAll();
    void callEvent8OnAllWindows();
    void callEvent9OnAllWindows();
    void callViewportRotateEventOnAllWindows();
    bool callKeyUpEventBackToFront(uint32_t charCode, uint32_t keyCode);
    void relocateWindows();
    void sub_4CEE0B(const Window& self);
    void sub_4B93A5(WindowNumber_t number);
    void closeConstructionWindows();
    void closeTopmost();
    void wheelInput(int wheel);
    bool isInFront(Ui::Window* w);
    bool isInFrontAlt(Ui::Window* w);
    Ui::Window* findWindowShowing(const viewport_pos& position);
    void closeAllFloatingWindows();
    int32_t getCurrentRotation();
    void setCurrentRotation(int32_t value);

    void viewportShiftPixels(Ui::Window* window, Ui::Viewport* viewport, int16_t dX, int16_t dY);
    void viewportSetVisibility(ViewportVisibility flags);

    // 0x0052622E
    uint16_t getVehiclePreviewRotationFrame();
    void setVehiclePreviewRotationFrame(uint16_t);

    uint8_t getVehiclePreviewRotationFrameUnk1();
    uint8_t getVehiclePreviewRotationFrameUnk2();

    void render(Gfx::RenderTarget& rt, const Rect& rect);
}

namespace OpenLoco::Vehicles
{
    struct VehicleBase;
    struct Car;
}

namespace OpenLoco::Ui::Windows
{
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
        void drawVehicleOverview(Gfx::RenderTarget* rt, int16_t vehicleTypeIdx, CompanyId company, uint8_t eax, uint8_t esi, Ui::Point offset);
        void registerHooks();
    }

    namespace Cheats
    {
        Window* open();
    }

    namespace CompanyFaceSelection
    {
        void open(CompanyId id);
    }

    namespace CompanyList
    {
        void openPerformanceIndexes();
        Window* open();
    }

    namespace CompanyWindow
    {
        Window* open(CompanyId companyId);
        Window* openAndSetName();
        Window* openChallenge(CompanyId companyId);
        Window* openFinances(CompanyId companyId);
        bool rotate(Window& self);
    }

    namespace Construction
    {
        Window* openWithFlags(uint32_t flags);
        Window* openAtTrack(const Window& main, World::TrackElement* track, const World::Pos2 pos);
        Window* openAtRoad(const Window& main, World::RoadElement* track, const World::Pos2 pos);
        void setToTrackExtra(const Window& main, World::TrackElement* track, const uint8_t bh, const World::Pos2 pos);
        void setToRoadExtra(const Window& main, World::RoadElement* track, const uint8_t bh, const World::Pos2 pos);
        void sub_4A6FAC();
        bool isStationTabOpen();
        bool isOverheadTabOpen();
        bool isSignalTabOpen();
        bool rotate(Window& self);
        void removeConstructionGhosts();
        void registerHooks();
        uint16_t getLastSelectedMods();
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
        void open(StringId title, StringId message = StringIds::null);
        void openQuiet(StringId title, StringId message = StringIds::null);
        void openWithCompetitor(StringId title, StringId message, CompanyId competitorId);
        void registerHooks();
    }

    namespace Industry
    {
        Window* open(IndustryId id);
    }

    namespace IndustryList
    {
        Window* open();
        void reset();
        void removeIndustry(const IndustryId id);
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
        void showGridlines();
        void hideGridlines();
        void showDirectionArrows();
        void hideDirectionArrows();
    }

    namespace MapToolTip
    {
        void open();
        void setOwner(CompanyId company);
        uint16_t getTooltipTimeout();
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

    namespace NetworkStatus
    {
        using CloseCallback = std::function<void()>;

        Window* open(std::string_view text, CloseCallback cbClose);
        void setText(std::string_view text);
        void setText(std::string_view text, CloseCallback cbClose);
        void close();
    }

    namespace NewsWindow
    {
        void open(MessageId messageIndex);
        void openLastMessage();
        void close(Ui::Window* window);
    }

    namespace ObjectLoadError
    {
        Window* open(const std::vector<ObjectHeader>& list);
    }

    namespace ObjectSelectionWindow
    {
        Window* open();
        bool tryCloseWindow();
    }

    namespace Options
    {
        Window* open();
        Window* openMusicSettings();
        constexpr uint8_t kTabOffsetMusic = 2;
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
        bool open(browse_type type, char* path, const char* filter, StringId titleId);
    }

    namespace PromptOkCancel
    {
        bool open(StringId captionId, StringId descriptionId, FormatArguments& descriptionArgs, StringId okButtonStringId);
    }

    namespace PromptSaveWindow
    {
        Window* open(LoadOrQuitMode savePromptType);
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
        Window* open(StationId id);
        void reset();
        void showStationCatchment(StationId id);
        void sub_491BC6();
    }

    namespace StationList
    {
        Window* open(CompanyId companyId);
        Window* open(CompanyId companyId, uint8_t type);
    }

    namespace Terraform
    {
        Window* open();
        void openClearArea();
        void openAdjustLand();
        void openAdjustWater();
        void openPlantTrees();
        void openBuildWalls();
        bool rotate(Window&);
        void setAdjustLandToolSize(uint8_t size);
        void setAdjustWaterToolSize(uint8_t size);
        void setClearAreaToolSize(uint8_t size);
        void setLastPlacedTree(World::TreeElement* elTree);
    }

    namespace TextInput
    {
        void registerHooks();

        void openTextInput(Ui::Window* w, StringId title, StringId message, StringId value, int callingWidget, void* valueArgs, uint32_t inputSize = StringManager::kUserStringSize - 1);
        void sub_4CE6C9(WindowType type, WindowNumber_t number);
        void cancel();
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
        void beginSendChatMessage(Window& self);
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
        void beginSendChatMessage(Window& self);
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
        void update(Ui::Window* window, int32_t widgetIndex, StringId stringId, int16_t x, int16_t y);
        void set_52336E(bool value);
        void closeAndReset();
        bool isTimeTooltip();
    }

    namespace Town
    {
        Window* open(uint16_t townId);
    }

    namespace TownList
    {
        Window* open();
        void removeTown(TownId);
        void reset();
        bool rotate(Window& self);
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
            void scrollDrag(const Ui::Point& pos);
            void scrollDragEnd(const Ui::Point& pos);
        }
        namespace Common
        {
            int16_t sub_4B743B(uint8_t al, uint8_t ah, int16_t cx, int16_t dx, Vehicles::VehicleBase* vehicle, Gfx::RenderTarget* const pDrawpixelinfo);
        }
        bool rotate();
        bool cancelVehicleTools();
    }

    namespace VehicleList
    {
        Window* open(CompanyId companyId, VehicleType type);
        void removeTrainFromList(Window& self, EntityId head);
    }
}
