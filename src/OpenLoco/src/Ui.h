#pragma once

#include "Graphics/RenderTarget.h"
#include <string>
#include <vector>

namespace OpenLoco::Config
{
    enum class ScreenMode;
    struct Display;
    struct Resolution;
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
    CursorId getCursor();
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
}
