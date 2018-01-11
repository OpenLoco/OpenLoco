#pragma once

#include <cctype>
#include <string>

namespace openloco::utility
{
    bool iequals(const std::string_view &a, const std::string_view &b)
    {
        if (a.size() != b.size())
        {
            return false;
        }
        for (size_t i = 0; i < a.size(); i++)
        {
            if (std::tolower(a[i]) != std::tolower(b[i]))
            {
                return false;
            }
        }
        return true;
    }
}
