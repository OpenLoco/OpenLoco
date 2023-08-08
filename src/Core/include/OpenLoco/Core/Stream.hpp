#pragma once

#include "Traits.hpp"
#include <cstddef>
#include <cstdint>
#include <istream>

namespace OpenLoco
{
    namespace Utility
    {
        // Obsolete methods, use new Stream APIs

        template<typename T1, typename T2, typename T3>
        std::basic_istream<T1, T2>& readData(std::basic_istream<T1, T2>& stream, T3* dst, size_t count)
        {
            return stream.read((char*)dst, static_cast<uint64_t>(count) * sizeof(T3));
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

    enum class StreamMode
    {
        none = 0,
        read,
        write,
    };

    class Stream
    {
    public:
        virtual ~Stream() = default;

        virtual size_t getLength() const noexcept = 0;
        virtual size_t getPosition() const noexcept = 0;
        virtual void setPosition(size_t) = 0;
        virtual void read(void*, size_t) = 0;
        virtual void write(const void*, size_t) = 0;

        template<typename T>
        T readValue()
        {
            static_assert(Traits::IsPrimitive<T>::value || Traits::IsPOD<T>::value, "Type must be primitive or POD");

            T tmp;
            read(&tmp, sizeof(T));
            return tmp;
        }

        template<typename T>
        void writeValue(const T& src)
        {
            static_assert(Traits::IsPrimitive<T>::value || Traits::IsPOD<T>::value, "Type must be primitive or POD");

            write(&src, sizeof(T));
        }
    };
}
