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

    void getPart(
        Ui::window* window,
        Ui::widget_t* widget,
        int16_t x,
        int16_t y,
        int16_t* output_x,
        int16_t* output_y,
        ScrollPart* output_scroll_area,
        size_t* scrollIndex);
    void updateThumbs(window* window, widget_index widgetIndex);
    void scrollLeftBegin(const int16_t x, const int16_t y, Ui::window* const w, Ui::widget_t* const widget, const widget_index widgetIndex);
    void scrollLeftContinue(const int16_t x, const int16_t y, Ui::window* const w, Ui::widget_t* const widget, const widget_index widgetIndex);
    void scrollModalRight(const int16_t x, const int16_t y, Ui::window* const w, Ui::widget_t* const widget, const widget_index widgetIndex);
    void clearPressedButtons(const WindowType type, const window_number number, const widget_index widgetIndex);
    void horizontalDragFollow(Ui::window* const w, Ui::widget_t* const widget, const widget_index dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaX);
    void verticalDragFollow(Ui::window* const w, Ui::widget_t* const widget, const widget_index dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaY);
    void verticalNudgeUp(Ui::window* const w, const size_t scrollAreaIndex, const widget_index widgetIndex);
    void verticalNudgeDown(Ui::window* const w, const size_t scrollAreaIndex, const widget_index widgetIndex);
}
