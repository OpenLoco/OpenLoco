#pragma once

#include "Window.h"

namespace OpenLoco::Ui::ScrollView
{
    // For horizontal scrollbars its N wide, for vertical its N tall
    static constexpr uint8_t kScrollbarSize = 11;
    static constexpr auto kScrollButtonSize = Ui::Size(kScrollbarSize, kScrollbarSize);
    static constexpr auto kScrollbarMargin = 1;
    static constexpr uint8_t kThumbSize = 10;
    static constexpr uint8_t kMinThumbSize = 20;
    static constexpr uint8_t kButtonClickStep = 3;

    struct GetPartResult
    {
        Ui::Point scrollviewLoc;
        ScrollPart area;
        size_t index;
    };
    GetPartResult getPart(Ui::Window& window, Ui::Widget* widget, int16_t x, int16_t y);
    void updateThumbs(Window& window, WidgetIndex_t widgetIndex);
    void scrollLeftBegin(const int16_t x, const int16_t y, Ui::Window& w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex);
    void scrollLeftContinue(const int16_t x, const int16_t y, Ui::Window& w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex);
    void scrollModalRight(const int16_t x, const int16_t y, Ui::Window& w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex);
    void clearPressedButtons(const WindowType type, const WindowNumber_t number, const WidgetIndex_t widgetIndex);
    void horizontalDragFollow(Ui::Window& w, Ui::Widget* const widget, const WidgetIndex_t dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaX);
    void verticalDragFollow(Ui::Window& w, Ui::Widget* const widget, const WidgetIndex_t dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaY);
    void verticalNudgeUp(Ui::Window& w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex);
    void verticalNudgeDown(Ui::Window& w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex);
}
