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

namespace OpenLoco::Utility
{
    // Case-insensitive logical string comparison function
    int32_t strlogicalcmp(const char* s1, const char* s2)
    {
        for (;;)
        {
            if (*s2 == '\0')
                return *s1 != '\0';
            else if (*s1 == '\0')
                return -1;
            else if (!(isdigit(*s1) && isdigit(*s2)))
            {
                if (toupper(*s1) != toupper(*s2))
                    return toupper(*s1) - toupper(*s2);
                else
                {
                    ++s1;
                    ++s2;
                }
            }
            else
            {
                char *lim1, *lim2;
                unsigned long n1 = strtoul(s1, &lim1, 10);
                unsigned long n2 = strtoul(s2, &lim2, 10);
                if (n1 > n2)
                    return 1;
                else if (n1 < n2)
                    return -1;
                s1 = lim1;
                s2 = lim2;
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
