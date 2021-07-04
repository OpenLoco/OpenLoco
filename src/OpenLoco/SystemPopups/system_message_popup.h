#pragma once
#include <string>
#include <sstream>

namespace OpenLoco
{
    namespace platform
    {
        bool system_message_popup(const std::string& title, const std::string& message_to_display);

        bool system_message_popup(const std::string& title, std::stringstream& message_as_stream);
    }
}
