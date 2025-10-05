#pragma once

#include "Ui/Window.h"
#include <cstdint>

namespace OpenLoco::Ui::ToolTip
{
    void setWindowType(WindowType wndType);
    WindowType getWindowType();

    void setWindowNumber(WindowNumber_t wndNumber);
    WindowNumber_t getWindowNumber();

    void setWidgetIndex(WidgetIndex_t widx);
    WidgetIndex_t getWidgetIndex();

    void setNotShownTicks(uint16_t ticks);
    uint16_t getNotShownTicks();

    StringId getCurrentStringId();
    void setCurrentStringId(StringId stringId);

    bool isTimeTooltip();

    Ui::Point getTooltipMouseLocation();
    void setTooltipMouseLocation(const Ui::Point& loc);

    uint16_t getTooltipTimeout();
    void setTooltipTimeout(uint16_t tooltipTimeout);

    void set_52336E(bool value);
}
