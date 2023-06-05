#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <nonstd/span.hpp>
#include <stdexcept>
#include <vector>

namespace stdx
{
    using nonstd::span;
}

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
        read,
        write,
    };

    class Stream
    {
    public:
        virtual ~Stream() = default;

        virtual uint64_t getLength() const noexcept = 0;
        virtual uint64_t getPosition() const noexcept = 0;
        virtual void setPosition(uint64_t) = 0;
        virtual void read(void*, size_t) = 0;
        virtual void write(const void*, size_t) = 0;

        void seek(int64_t pos)
        {
            setPosition(getPosition() + pos);
        }

    protected:
        [[noreturn]] static void throwInvalidOperation(...) { throw std::runtime_error("Invalid operation"); }
    };

}
