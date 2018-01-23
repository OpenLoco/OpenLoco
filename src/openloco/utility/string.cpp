#include "string.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <codecvt>
#include <errno.h>
#include <iconv.h>
#include <iostream>
#include <locale>
#endif

namespace openloco::utility
{
    std::string to_utf8(const std::wstring_view& src)
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

    std::wstring to_utf16(const std::string_view& src)
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
