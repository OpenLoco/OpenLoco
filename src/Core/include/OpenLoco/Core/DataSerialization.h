#pragma once

#include "Stream.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <type_traits>

namespace OpenLoco
{
    namespace Detail
    {
        template<typename T>
            requires std::is_integral_v<T>
        constexpr T byteSwap(T value) noexcept
        {
            auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(value);
            std::reverse(bytes.begin(), bytes.end());
            return std::bit_cast<T>(bytes);
        }

        template<typename T>
            requires std::is_integral_v<T>
        constexpr T littleEndian(T value) noexcept
        {
            if constexpr (std::endian::native == std::endian::little)
            {
                return value;
            }
            else
            {
                return byteSwap(value);
            }
        }
    }

    struct DataSerilizer
    {
        DataSerilizer(Stream& stream)
            : _stream(stream)
        {
        }

        template<typename T>
            requires std::is_enum_v<T>
        void encode(const T& src)
        {
            _stream.writeValue(Detail::littleEndian(static_cast<std::underlying_type_t<T>>(src)));
        }

        template<typename T>
            requires std::is_integral_v<T>
        void encode(const T& src)
        {
            _stream.writeValue(Detail::littleEndian(src));
        }

        template<typename T>
            requires std::is_array_v<T> && std::is_integral_v<std::remove_extent_t<T>>
        void encode(const T& src)
        {
            if constexpr (std::endian::native == std::endian::little)
            {
                _stream.write(&src, sizeof(src));
            }
            else
            {
                for (const auto& element : src)
                {
                    encode(element);
                }
            }
        }

        template<typename T>
        void encode(const T& src);

        template<typename T>
            requires std::is_enum_v<T>
        void decode(T& dest)
        {
            dest = static_cast<T>(Detail::littleEndian(_stream.readValue<std::underlying_type_t<T>>()));
        }

        template<typename T>
            requires std::is_integral_v<T>
        void decode(T& dest)
        {
            dest = Detail::littleEndian(_stream.readValue<T>());
        }

        template<typename T>
            requires std::is_array_v<T> && std::is_integral_v<std::remove_extent_t<T>>
        void decode(T& dest)
        {
            if constexpr (std::endian::native == std::endian::little)
            {
                _stream.read(&dest, sizeof(dest));
            }
            else
            {
                for (auto& element : dest)
                {
                    decode(element);
                }
            }
        }

        template<typename T>
        void decode(T& dest);

        template<typename... Args>
        void encodeAll(const Args&... args)
        {
            (encode(args), ...);
        }

        template<typename... Args>
        void decodeAll(Args&... args)
        {
            (decode(args), ...);
        }

    private:
        Stream& _stream;
    };

    template<auto... Fields>
    struct FieldList
    {
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
        if constexpr (requires { typename DataSerialization<T>::Fields; })
        {
            [&]<auto... Fields>(FieldList<Fields...>) {
                (encode(src.*Fields), ...);
            }(typename DataSerialization<T>::Fields{});
        }
        else
        {
            DataSerialization<T>::encode(src, *this);
        }
    }

    template<typename T>
    void DataSerilizer::decode(T& dest)
    {
        if constexpr (requires { typename DataSerialization<T>::Fields; })
        {
            [&]<auto... Fields>(FieldList<Fields...>) {
                (decode(dest.*Fields), ...);
            }(typename DataSerialization<T>::Fields{});
        }
        else
        {
            DataSerialization<T>::decode(dest, *this);
        }
    }
}
