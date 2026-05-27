#pragma once

#include "Types.hpp"
#include "Window.h"
#include <cstdint>

namespace OpenLoco::ToolManager
{
    Ui::Window* toolGetActiveWindow();
    bool isToolActive(Ui::WindowType);

    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t);
    bool isToolActive(Ui::WindowType, Ui::WindowNumber_t, int16_t);
    bool toolSet(const Ui::Window& w, int16_t widgetIndex, Ui::CursorId cursorId);
    void toolCancel();
    void toolCancel(Ui::WindowType, Ui::WindowNumber_t);

    Ui::WindowNumber_t getToolWindowNumber();
    void setToolWindowNumber(Ui::WindowNumber_t toolWindowNumber);

    Ui::WindowType getToolWindowType();
    void setToolWindowType(Ui::WindowType toolWindowType);

    Ui::CursorId getToolCursor();
    void setToolCursor(Ui::CursorId toolWindowCursor);

    int16_t getToolWidgetIndex();
    void setToolWidgetIndex(uint16_t toolWidgetIndex);
}
