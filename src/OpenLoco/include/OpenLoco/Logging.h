#pragma once

#include <OpenLoco/Diagnostics/Logging.h>

namespace OpenLoco::Diagnostics::Logging
{
    void initialize(std::string_view logLevels);
    void shutdown();
}
