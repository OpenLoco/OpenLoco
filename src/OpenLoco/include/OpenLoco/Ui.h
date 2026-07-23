#pragma once

#include <OpenLoco/Engine/Ui/Point.hpp>
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

    struct ScreenInfo
    {
        int16_t width;
        int16_t height;
        int16_t width_2;
        int16_t height_2;
        int16_t width_3;
        int16_t height_3;
    };

    enum class CursorId : uint8_t
    {
        pointer,
        blank,
        upArrow,
        upDownArrow,
        handPointer,
        busy,
        diagonalArrows,
        picker,
        plantTree,
        placeFountain,
        placeStatue,
        placeBench,
        crosshair,
        placeTrashBin,
        placeLantern,
        placeFence,
        placeFlowers,
        placePath,
        landTool,
        waterTool,
        placeHome,
        placeVolcano,
        footsteps,
        brush,
        placeBanner,
        openHand,
        dragHand,
        placeTrain,
        placeTrainAlt,
        placeBus,
        placeBusAlt,
        placeTruck,
        placeTruckAlt,
        placeTram,
        placeTramAlt,
        placePlane,
        placeShip,
        inwardArrows,
        placeTown,
        placeBuilding,
        placeFactory,
        bulldozerTool,
        placeSignal,
        placeHQ,
        placeStation,
    };

    constexpr uint8_t kNumCursors = 45;

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
    bool isInitialized();

    void createWindow(const Config::Display& cfg);
    void initialise();
    void initialiseCursors();
    void disposeCursors();
    void setCursor(CursorId id);
    CursorId getCursor();
    Point getCursorPosScaled();
    Point getCursorPos();
    void setCursorPosScaled(int32_t scaledX, int32_t scaledY);
    void setCursorPos(int32_t x, int32_t y);
    void hideCursor();
    void showCursor();
    void update();
    void triggerResize();
    void render();
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

    void windowPositionChanged(int32_t x, int32_t y);
    void windowSizeChanged(int32_t width, int32_t height);

#if !(defined(__APPLE__) && defined(__MACH__))
    void toggleFullscreenDesktop();
#endif
}
