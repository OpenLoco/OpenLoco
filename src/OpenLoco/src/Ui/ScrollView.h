#pragma once

#include "Window.h"
#include <OpenLoco/Core/EnumFlags.hpp>

namespace OpenLoco::Ui::ScrollView
{
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
