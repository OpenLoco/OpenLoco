#pragma once

#include "Localisation/StringManager.h"
#include <cstdint>

namespace OpenLoco::ProgressBar
{
    void begin(string_id stringId, int32_t edx);
    void setProgress(int32_t value);
    void end();
}
