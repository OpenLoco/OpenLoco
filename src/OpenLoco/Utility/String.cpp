#include "String.hpp"

#ifdef _WIN32
#include <windows.h>
#undef max
#else
#include <codecvt>
#include <errno.h>
#include <iconv.h>
#include <iostream>
#include <locale>
#endif
#include <limits>

namespace OpenLoco::Utility
{
    static std::string_view skipDigits(std::string_view s)
    {
        while (!s.empty() && (s[0] == '.' || s[0] == ',' || isdigit(s[0])))
        {
            s = s.substr(1);
        }
        return s;
    }

    static std::pair<int32_t, std::string_view> parseNextNumber(std::string_view s)
    {
        int32_t num = 0;
        while (!s.empty())
        {
            if (s[0] != '.' && s[0] != ',')
            {
                if (!isdigit(s[0]))
                    break;

                auto newResult = num * 10;
                if (newResult / 10 != num)
                {
                    return { std::numeric_limits<int32_t>::max(), skipDigits(s) };
                }
                num = newResult + (s[0] - '0');
                if (num < newResult)
                {
                    return { std::numeric_limits<int32_t>::max(), skipDigits(s) };
                }
            }
            s = s.substr(1);
        }
        return { num, s };
    }

    // Case-insensitive logical string comparison function
    int32_t strlogicalcmp(std::string_view s1, std::string_view s2)
    {
        for (;;)
        {
            if (s2.empty())
                return !s1.empty();
            else if (s1.empty())
                return -1;
            else if (!(isdigit(s1[0]) && isdigit(s2[0])))
            {
                if (toupper(s1[0]) != toupper(s2[0]))
                    return toupper(s1[0]) - toupper(s2[0]);
                else
                {
                    s1 = s1.substr(1);
                    s2 = s2.substr(1);
                }
            }
            else
            {
                // TODO: Replace with std::from_chars when all compilers support std::from_chars
                auto [n1, s1n] = parseNextNumber(s1);
                auto [n2, s2n] = parseNextNumber(s2);
                if (n1 > n2)
                    return 1;
                else if (n1 < n2)
                    return -1;
                s1 = s1n;
                s2 = s2n;
            }
        }
    }

    std::string toUtf8(const std::wstring_view& src)
    {
#ifdef _WIN32
        int srcLen = src.size();
        int sizeReq = WideCharToMultiByte(CP_UTF8, 0, src.data(), srcLen, nullptr, 0, nullptr, nullptr);
        auto result = std::string(sizeReq, 0);
        WideCharToMultiByte(CP_UTF8, 0, src.data(), srcLen, result.data(), sizeReq, nullptr, nullptr);
        return result;
#else
        using convert_typeX = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_typeX, wchar_t> converterX;

        std::string out = converterX.to_bytes(src.data());
        std::cout << __PRETTY_FUNCTION__ << " " << out << "\n";
        return out;
#endif
    }

    std::wstring toUtf16(const std::string_view& src)
    {
#ifdef _WIN32
        int srcLen = src.size();
        int sizeReq = MultiByteToWideChar(CP_ACP, 0, src.data(), srcLen, nullptr, 0);
        auto result = std::wstring(sizeReq, 0);
        MultiByteToWideChar(CP_ACP, 0, src.data(), srcLen, result.data(), sizeReq);
        return result;
#else
        perror("STUB!\n");
        return std::to_wstring(0);
#endif
    }
}
