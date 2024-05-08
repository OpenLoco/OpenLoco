#pragma once

#include <cstddef>
#include <fmt/core.h>

namespace OpenLoco
{
    struct StringBuffer
    {
        using value_type = char;

        char* buffer;
        size_t offset;
        size_t maxLen;

    public:
        StringBuffer(value_type* buffer, size_t maxLen);

        void appendData(const void* data, size_t size);

        void append(value_type chr);

        void append(const char* input);

        void append(const char* input, size_t inputLen);

        // std::back_inserter support.
        void push_back(value_type chr);

        template<typename TLocale, typename... TArgs>
        void format(TLocale&& loc, fmt::format_string<TArgs...> fmt, TArgs&&... args)
        {
            fmt::format_to(std::back_inserter(*this), loc, fmt, std::forward<TArgs>(args)...);
        }

        char* current() const;

        void nullTerminate();

        void grow(size_t numChars);
    };
}
