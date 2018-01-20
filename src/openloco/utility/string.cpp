//#include <windows.h>
#include "string.hpp"

namespace openloco::utility
{
    std::string to_utf8(const std::wstring_view &src)
    {
        return nullptr;
//        int srcLen = src.size();
//        int sizeReq = WideCharToMultiByte(CP_UTF8, 0, src.data(), srcLen, nullptr, 0, nullptr, nullptr);
//        auto result = std::string(sizeReq, 0);
//        WideCharToMultiByte(CP_UTF8, 0, src.data(), srcLen, result.data(), sizeReq, nullptr, nullptr);
//        return result;
    }

    std::wstring to_utf16(const std::string_view &src)
    {
        return nullptr;
//        int srcLen = src.size();
//        int sizeReq = MultiByteToWideChar(CP_ACP, 0, src.data(), srcLen, nullptr, 0);
//        auto result = std::wstring(sizeReq, 0);
//        MultiByteToWideChar(CP_ACP, 0, src.data(), srcLen, result.data(), sizeReq);
//        return result;
    }
}
