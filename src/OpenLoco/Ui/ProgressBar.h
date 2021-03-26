#pragma once

#include "../Localisation/StringManager.h"
#include <cstdint>

namespace OpenLoco::Ui::ProgressBar
{
    void begin(string_id stringId);
    void sub_4CF63B();
    void setProgress(int32_t value);
    void end();
    void registerHooks();
}
