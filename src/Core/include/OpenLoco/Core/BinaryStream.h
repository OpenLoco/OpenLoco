#pragma once

#include "Stream.hpp"
#include <cstdint>
#include <cstdio>
#include <vector>

namespace stdx
{
    using nonstd::span;
}

namespace OpenLoco
{

    class BinaryStream final : public Stream
    {
    private:
        const void* _data{};
        size_t _index{};
        size_t _len{};

    public:
        BinaryStream(const void* data, size_t len)
            : _data(data)
            , _len(len)
        {
        }

        uint64_t getLength() const noexcept override
        {
            return _len;
        }

        uint64_t getPosition() const noexcept override
        {
            return _index;
        }

        void setPosition(uint64_t position) override
        {
            if (position > _len)
                throw std::out_of_range("Position too large");
            _index = static_cast<size_t>(position);
        }

        void read(void* buffer, size_t len) override
        {
            auto maxReadLen = _len - _index;
            if (len > maxReadLen)
                throw std::runtime_error("Failed to read data");
            std::memcpy(buffer, reinterpret_cast<const void*>(reinterpret_cast<size_t>(_data) + _index), len);
            _index += len;
        }

        void write([[maybe_unused]] const void* buffer, [[maybe_unused]] size_t len) override
        {
            Stream::throwInvalidOperation();
        }
    };

}
