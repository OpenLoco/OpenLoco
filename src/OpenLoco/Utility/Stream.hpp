#pragma once

#include <istream>

namespace OpenLoco::Utility
{
    template<typename T1, typename T2, typename T3>
    std::basic_istream<T1, T2>& readData(std::basic_istream<T1, T2>& stream, T3* dst, size_t count)
    {
        return stream.read((char*)dst, count * sizeof(T3));
    }

    template<typename T1, typename T2, typename T3>
    std::basic_istream<T1, T2>& readData(std::basic_istream<T1, T2>& stream, T3& dst)
    {
        return readData(stream, &dst, 1);
    }

    template<typename T3, typename T1, typename T2>
    T3 readValue(std::basic_istream<T1, T2>& stream)
    {
        T3 result{};
        readData(stream, result);
        return result;
    }
}
