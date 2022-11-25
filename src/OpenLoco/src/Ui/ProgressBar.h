#pragma once

#include "Localisation/StringManager.h"
#include <cstdint>
#include <string_view>

namespace OpenLoco::Ui::ProgressBar
{
    void begin(std::string_view string);
    void begin(string_id stringId);
    void setProgress(int32_t value);
    void end();
    void registerHooks();
}
