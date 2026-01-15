#pragma once
#include "Stream.hpp"
#include <type_traits>

namespace OpenLoco
{
    struct DataSerilizer
    {
        DataSerilizer(Stream& stream)
            : _stream(stream)
        {
        }

        template<typename T>
            requires std::is_integral_v<T>
        void encode(const T& src)
        {
            _stream.writeValue(src);
        }
        template<typename T>
            requires std::is_array_v<T> && std::is_integral_v<std::remove_extent_t<T>>
        void encode(const T& src)
        {
            _stream.write(&src, sizeof(src));
        }
        template<typename T>
        void encode(const T& src);

        template<typename T>
            requires std::is_integral_v<T>
        void decode(T& dest)
        {
            dest = _stream.readValue<T>();
        }
        template<typename T>
            requires std::is_array_v<T> && std::is_integral_v<std::remove_extent_t<T>>
        void decode(T& dest)
        {
            _stream.read(&dest, sizeof(dest));
        }
        template<typename T>
        void decode(T& dest);

    private:
        Stream& _stream;
    };

    template<typename T>
    struct DataSerialization
    {
        static void encode(const T& src, DataSerilizer& ds) = delete;
        static void decode(T& dest, DataSerilizer& ds) = delete;
    };

    template<typename T>
    void DataSerilizer::encode(const T& src)
    {
        DataSerialization<T>::encode(src, *this);
    }

    template<typename T>
    void DataSerilizer::decode(T& dest)
    {
        DataSerialization<T>::decode(dest, *this);
    }
}
