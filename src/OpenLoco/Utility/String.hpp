#pragma once

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>

namespace OpenLoco::Utility
{
    int32_t strlogicalcmp(std::string_view s1, std::string_view s2);
    std::string toUtf8(const std::wstring_view& src);
    std::wstring toUtf16(const std::string_view& src);

    inline bool iequals(const std::string_view& a, const std::string_view& b)
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

    inline bool equals(const std::string_view& a, const std::string_view& b, bool ignoreCase = false)
    {
        return ignoreCase ? iequals(a, b) : a == b;
    }

    inline bool startsWith(std::string_view s, std::string_view value, bool ignoreCase = false)
    {
        if (s.size() >= value.size())
        {
            auto substr = s.substr(0, value.size());
            return equals(substr, value, ignoreCase);
        }
        return false;
    }

    inline bool endsWith(std::string_view s, std::string_view value, bool ignoreCase = false)
    {
        if (s.size() >= value.size())
        {
            auto substr = s.substr(s.size() - value.size());
            return equals(substr, value, ignoreCase);
        }
        return false;
    }

    inline size_t strlcpy(char* dest, const char* src, size_t size)
    {
        size_t srcLen = std::strlen(src);

        if (srcLen < size)
        {
            std::memcpy(dest, src, srcLen + 1);
        }
        else
        {
            std::memcpy(dest, src, size);
            dest[size - 1] = '\0';
        }

        return srcLen;
    }

    inline size_t strlcat(char* dest, const char* src, size_t size)
    {
        size_t srcLen = std::strlen(src);

        if (size == 0)
        {
            return srcLen;
        }

        // this lambda is essentially a reimplementation of strnlen, which isn't standard
        size_t destLen = [=] {
            auto destLen = reinterpret_cast<const char*>(std::memchr(dest, '\0', size));
            if (destLen != nullptr)
            {
                return static_cast<size_t>(destLen - dest);
            }
            else
            {
                return size;
            }
        }();

        if (destLen < size)
        {
            size_t copyCount = std::min<size_t>((size - destLen) - 1, srcLen);
            char* copyPtr = (dest + destLen);

            std::memcpy(copyPtr, src, copyCount);
            copyPtr[copyCount] = '\0';
        }

        return (destLen + srcLen);
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

    template<size_t N>
    std::string_view nullTerminatedView(const char (&src)[N])
    {
        for (size_t i = 0; i < N; i++)
        {
            if (src[i] == '\0')
            {
                return std::string_view(src, N);
            }
        }
        return std::string_view(src, N);
    }
}
