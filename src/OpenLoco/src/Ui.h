#pragma once

#include "Graphics/Gfx.h"
#include "Graphics/RenderTarget.h"
#include "Location.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Engine/World.hpp>
#include <string>
#include <vector>

namespace OpenLoco::Config
{
    enum class ScreenMode;
    struct Display;
    struct Resolution;
}

namespace OpenLoco::Paint
{
    struct PaintStruct;
}

namespace OpenLoco::Ui
{
    struct Viewport;

#pragma pack(push, 1)
    struct ScreenInfo
    {
        int16_t width;
        int16_t height;
        int16_t width_2;
        int16_t height_2;
        int16_t width_3;
        int16_t height_3;
    };
    static_assert(sizeof(ScreenInfo) == 0xC);
#pragma pack(pop)

    enum class CursorId : uint8_t
    {
        pointer = 0,
        blank = 1,
        upArrow = 2,
        upDownArrow = 3,
        handPointer = 4,
        busy = 5,
        diagonalArrows = 6,
        picker = 7,
        plantTree = 8,
        placeFountain = 9,
        placeStatue = 10,
        placeBench = 11,
        crosshair = 12,
        placeTrashBin = 13,
        placeLantern = 14,
        placeFence = 15,
        placeFlowers = 16,
        placePath = 17,
        landTool = 18,
        waterTool = 19,
        placeHome = 20,
        placeVolcano = 21,
        footsteps = 22,
        brush = 23,
        placeBanner = 24,
        openHand = 25,
        dragHand = 26,
        placeTrain = 27,
        placeTrainAlt = 28,
        placeBus = 29,
        placeBusAlt = 30,
        placeTruck = 31,
        placeTruckAlt = 32,
        placeTram = 33,
        placeTramAlt = 34,
        placePlane = 35,
        placeShip = 36,
        inwardArrows = 37,
        placeTown = 38,
        placeBuilding = 39,
        placeFactory = 40,
        bulldozerTool = 41,
        placeSignal = 42,
        placeHQ = 43,
        placeStation = 44,
    };

    struct Resolution
    {
        int32_t width;
        int32_t height;
    };

    namespace ScaleFactor
    {
        const float min = 1.0f;
        const float max = 4.0f;
        const float step = 0.25f;
    };

    void* hwnd();

    int32_t width();
    int32_t height();
    bool dirtyBlocksInitialised();

    void createWindow(const Config::Display& cfg);
    void initialise();
    void initialiseCursors();
    void initialiseInput();
    void disposeInput();
    void disposeCursors();
    void setCursor(CursorId id);
    Point32 getCursorPosScaled();
    Point32 getCursorPos();
    void setCursorPosScaled(int32_t scaledX, int32_t scaledY);
    void setCursorPos(int32_t x, int32_t y);
    void hideCursor();
    void showCursor();
    void update();
    void triggerResize();
    void render();
    bool processMessages();
    bool processMessagesMini();
    void showMessageBox(const std::string& title, const std::string& message);
    Config::Resolution getResolution();
    Config::Resolution getDesktopResolution();
    bool setDisplayMode(Config::ScreenMode mode, Config::Resolution newResolution);
    bool setDisplayMode(Config::ScreenMode mode);
    void updateFullscreenResolutions();
    std::vector<Resolution> getFullscreenResolutions();
    Resolution getClosestResolution(int32_t inWidth, int32_t inHeight);
    void handleInput();
    void minimalHandleInput();
    void setWindowScaling(float newScaleFactor);
    void adjustWindowScale(float adjust_by);
    bool hasInputFocus();

    namespace ViewportInteraction
    {
        enum class InteractionItem : uint8_t
        {
            noInteraction = 0,
            surface = 1,
            industryTree = 2,
            entity = 3,
            track = 4,
            trackExtra = 5,
            signal = 6,
            trackStation = 7,
            roadStation = 8,
            airport = 9,
            dock = 10,
            water = 11,
            tree = 12,
            wall = 13,
            townLabel = 14,
            stationLabel = 15,
            road = 16,
            roadExtra = 17,
            bridge = 18,
            building = 19,
            industry = 20,
            headquarterBuilding = 21,
            buildingInfo = 22,
        };

        enum class InteractionItemFlags : uint32_t // Bridge missing?
        {
            none = 0U,
            surface = (1U << 0),
            entity = (1U << 1),
            track = (1U << 2),
            water = (1U << 3),
            tree = (1U << 4),
            roadAndTram = (1U << 5),
            roadAndTramExtra = (1U << 6),
            signal = (1U << 7),
            wall = (1U << 8),
            headquarterBuilding = (1U << 9),
            station = (1U << 11),
            townLabel = (1U << 12),
            stationLabel = (1U << 13),
            trackExtra = (1U << 14),
            building = (1U << 15),
            industry = (1U << 16),
        };
        OPENLOCO_ENABLE_ENUM_OPERATORS(InteractionItemFlags);

        struct InteractionArg
        {
            World::Pos2 pos;
            union
            {
                uint32_t value;
                void* object;
            };
            InteractionItem type;
            uint8_t modId; // used for track mods and to indicate left/right signals
            InteractionArg() = default;
            InteractionArg(const World::Pos2& _pos, uint32_t _value, InteractionItem _type, uint8_t _unkBh)
                : pos(_pos)
                , value(_value)
                , type(_type)
                , modId(_unkBh)
            {
            }
            InteractionArg(const Paint::PaintStruct& ps);
        };

        InteractionArg getItemLeft(int16_t tempX, int16_t tempY);
        InteractionArg rightOver(int16_t x, int16_t y);

        std::pair<ViewportInteraction::InteractionArg, Ui::Viewport*> getMapCoordinatesFromPos(int32_t screenX, int32_t screenY, InteractionItemFlags flags);
        std::optional<World::Pos2> getSurfaceOrWaterLocFromUi(const Point& screenCoords);
        uint8_t getQuadrantOrCentreFromPos(const World::Pos2& loc);
        uint8_t getQuadrantFromPos(const World::Pos2& loc);
        uint8_t getSideFromPos(const World::Pos2& loc);
        std::optional<std::pair<World::Pos2, Ui::Viewport*>> getSurfaceLocFromUi(const Point& screenCoords);
    }
}
