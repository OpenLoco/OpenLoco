#pragma once

#include "./Types.hpp"
#include "./Window.h"
#include <cstdint>

using namespace OpenLoco::Ui;

namespace OpenLoco::ToolManager
{
    //  0x00523390
    Ui::WindowNumber_t getToolWindowNumber();
    void setToolWindowNumber(Ui::WindowNumber_t toolWindowNumber);

    // 0x00523392
    Ui::WindowType getToolWindowType();
    void setToolWindowType(Ui::WindowType toolWindowType);

    // 0x00523393
    Ui::CursorId getToolWindowCursor();
    void setToolWindowCursor(Ui::CursorId toolWindowCursor);

    // 0x00523394
    int16_t getToolWidgetIndex();
    void setToolWidgetIndex(uint16_t toolWidgetIndex);
}
