#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>

namespace openloco::utility
{
    std::string to_utf8(const std::wstring_view& src);
    std::wstring to_utf16(const std::string_view& src);

    static inline bool iequals(const std::string_view& a, const std::string_view& b)
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

    inline size_t strlcpy(char* dest, const char* src, size_t size)
    {
        size_t src_len = std::strlen(src);

        if (src_len < size)
        {
            std::memcpy(dest, src, src_len + 1);
        }
        else
        {
            std::memcpy(dest, src, size);
            dest[size - 1] = '\0';
        }

        return src_len;
    }

    inline size_t strlcat(char* dest, const char* src, size_t size)
    {
        size_t src_len = std::strlen(src);

        if (size == 0)
        {
            return src_len;
        }

        // this lambda is essentially a reimplementation of strnlen, which isn't standard
        size_t dest_len = [=] {
            auto dest_end = reinterpret_cast<const char*>(std::memchr(dest, '\0', size));
            if (dest_end != nullptr)
            {
                return static_cast<size_t>(dest_end - dest);
            }
            else
            {
                return size;
            }
        }();

        if (dest_len < size)
        {
            size_t copy_count = std::min<size_t>((size - dest_len) - 1, src_len);
            char* copy_ptr = (dest + dest_len);

            std::memcpy(copy_ptr, src, copy_count);
            copy_ptr[copy_count] = '\0';
        }

        return (dest_len + src_len);
    }

    template<size_t N>
    void strcpy_safe(char (&dest)[N], const char* src)
    {
        (void)strlcpy(dest, src, N);
    }

    template<size_t N>
    void strcat_safe(char (&dest)[N], const char* src)
    {
        (void)strlcat(dest, src, N);
    }

    template<size_t N, typename... Args>
    int sprintf_safe(char (&dest)[N], const char* fmt, Args&&... args)
    {
        return std::snprintf(dest, N, fmt, std::forward<Args>(args)...);
    }
}
