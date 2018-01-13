#pragma once

#include <cctype>
#include <string>

namespace openloco::utility
{
    std::string to_utf8(const std::wstring_view &src);
    std::wstring to_utf16(const std::string_view &src);

    static inline bool iequals(const std::string_view &a, const std::string_view &b)
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
