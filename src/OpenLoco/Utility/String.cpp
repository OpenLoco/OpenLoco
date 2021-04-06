#include "String.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <codecvt>
#include <errno.h>
#include <iconv.h>
#include <iostream>
#include <locale>
#endif
#include <charconv>

namespace OpenLoco::Utility
{
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
                unsigned long n1 = 0;
                unsigned long n2 = 0;

                [[maybe_unused]] auto [ptr1, ec1] = std::from_chars(s1.data(), s1.data() + s1.size(), n1);
                [[maybe_unused]] auto [ptr2, ec2] = std::from_chars(s2.data(), s2.data() + s2.size(), n2);
                if (n1 > n2)
                    return 1;
                else if (n1 < n2)
                    return -1;
                s1 = ptr1;
                s2 = ptr2;
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
