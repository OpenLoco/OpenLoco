#include "../Interop/Interop.hpp"
#include "../Window.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ScrollView
{
    enum class ScrollPart : int16_t
    {
        none = -1,
        view = 0,
        hscrollbarButtonLeft = 1,
        hscrollbarButtonRight = 2,
        hscrollbarTrackLeft = 3,
        hscrollbarTrackRight = 4,
        hscrollbarThumb = 5,
        vscrollbarButtonTop = 6,
        vscrollbarButtonBottom = 7,
        vscrollbarTrackTop = 8,
        vscrollbarTrackBottom = 9,
        vscrollbarThumb = 10,
    };

    namespace ScrollFlags
    {
        constexpr uint16_t hscrollbarVisible = 1 << 0;
        constexpr uint16_t hscrollbarThumbPressed = 1 << 1;
        constexpr uint16_t hscrollbarLeftPressed = 1 << 2;
        constexpr uint16_t hscrollbarRightPressed = 1 << 3;
        constexpr uint16_t vscrollbarVisible = 1 << 4;
        constexpr uint16_t vscrollbarThumbPressed = 1 << 5;
        constexpr uint16_t vscrollbarUpPressed = 1 << 6;
        constexpr uint16_t vscrollbarDownPressed = 1 << 7;
    }

    constexpr uint8_t thumbSize = 10;
    constexpr uint8_t barWidth = 11;
    constexpr uint8_t buttonClickStep = 3;

    struct GetPartResult
    {
        Ui::Point scrollviewLoc;
        ScrollPart area;
        size_t index;
    };
    GetPartResult getPart(Ui::Window* window, Ui::Widget* widget, int16_t x, int16_t y);
    void updateThumbs(Window* window, WidgetIndex_t widgetIndex);
    void scrollLeftBegin(const int16_t x, const int16_t y, Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex);
    void scrollLeftContinue(const int16_t x, const int16_t y, Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex);
    void scrollModalRight(const int16_t x, const int16_t y, Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex);
    void clearPressedButtons(const WindowType type, const WindowNumber_t number, const WidgetIndex_t widgetIndex);
    void horizontalDragFollow(Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaX);
    void verticalDragFollow(Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaY);
    void verticalNudgeUp(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex);
    void verticalNudgeDown(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex);
}
