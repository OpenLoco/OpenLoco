#pragma once

#include <istream>

namespace openloco::utility
{
    template<typename T1, typename T2, typename T3>
    std::basic_istream<T1, T2>& read_data(std::basic_istream<T1, T2>& stream, T3* dst, size_t count)
    {
        return stream.read((char*)dst, count * sizeof(T3));
    }

    template<typename T1, typename T2, typename T3>
    std::basic_istream<T1, T2>& read_data(std::basic_istream<T1, T2>& stream, T3& dst)
    {
        return read_data(stream, &dst, 1);
    }

    template<typename T3, typename T1, typename T2>
    T3 read_value(std::basic_istream<T1, T2>& stream)
    {
        T3 result{};
        read_data(stream, result);
        return result;
    }
}
