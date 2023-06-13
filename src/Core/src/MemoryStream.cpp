#include "MemoryStream.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace OpenLoco
{
    MemoryStream::~MemoryStream()
    {
        if (_data != nullptr)
        {
            free(_data);
        }
    }

    void MemoryStream::reserve(size_t len)
    {
        if (len == 0 || len < _capacity)
        {
            return;
        }

        auto* newData = static_cast<std::byte*>(std::realloc(_data, len));
        if (newData == nullptr)
        {
            throw std::bad_alloc();
        }

        _data = newData;
        _capacity = len;
    }

    void MemoryStream::resize(size_t len)
    {
        if (len == 0)
        {
            clear();
            return;
        }

        if (len < _capacity)
        {
            _length = len;
            return;
        }

        reserve(len);
        _length = len;
    }

    void MemoryStream::clear()
    {
        _offset = 0;
        _length = 0;
    }

    const std::byte* MemoryStream::data() const
    {
        return _data;
    }

    std::byte* MemoryStream::data()
    {
        return _data;
    }

    size_t MemoryStream::getLength() const noexcept
    {
        return _length;
    }

    size_t MemoryStream::getPosition() const noexcept
    {
        return _offset;
    }

    void MemoryStream::setPosition(size_t position)
    {
        _offset = std::min(_length, position);
    }

    void MemoryStream::read(void* buffer, size_t len)
    {
        const auto maxReadLength = std::min(len, _length - _offset);
        if (len > maxReadLength)
        {
            throw std::runtime_error("Failed to read data");
        }

        std::memcpy(buffer, _data + _offset, len);
        _offset += maxReadLength;
    }

    // TODO: Move this somewhere more sensible.
    template<typename T>
    static constexpr T alignTo(T val, T align) noexcept
    {
        if (val % align == 0)
        {
            return val;
        }
        return val + (align - (val % align));
    }

    void MemoryStream::write(const void* buffer, size_t len)
    {
        const auto spaceLeft = _capacity - _offset;

        // Check if we have to expand the current buffer.
        if (len > spaceLeft)
        {
            constexpr auto growthFactor = 2.0f;
            constexpr auto pageSize = 0x1000U;

            const auto newCapacity = _capacity + len;
            const auto finalCapacity = alignTo(static_cast<std::size_t>(newCapacity * growthFactor), pageSize);

            auto* newData = static_cast<std::byte*>(std::realloc(_data, finalCapacity));
            if (newData == nullptr)
            {
                throw std::bad_alloc();
            }

            _data = newData;
            _capacity = finalCapacity;
        }

        // Copy the data into the buffer.
        std::memcpy(_data + _offset, buffer, len);

        const auto spaceAvailable = _length - _offset;
        if (len > spaceAvailable)
        {
            _length += len - spaceAvailable;
        }

        _offset += len;
    }
}
