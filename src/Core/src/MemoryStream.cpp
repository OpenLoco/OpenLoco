#include "MemoryStream.h"
#include <cstring>
#include <stdexcept>

namespace OpenLoco
{
    void MemoryStream::ensureLength(size_t len)
    {
        if (_data.size() < len)
        {
            _data.resize(len);
        }
    }

    const void* MemoryStream::data() const
    {
        return _data.data();
    }

    void* MemoryStream::data()
    {
        return _data.data();
    }

    uint64_t MemoryStream::getLength() const noexcept
    {
        return _data.size();
    }

    uint64_t MemoryStream::getPosition() const noexcept
    {
        return _index;
    }

    void MemoryStream::setPosition(uint64_t position)
    {
        _index = std::min(_data.size(), static_cast<size_t>(position));
    }

    void MemoryStream::read(void* buffer, size_t len)
    {
        auto maxReadLen = _data.size() - _index;
        if (len > maxReadLen)
            throw std::runtime_error("Failed to read data");
        std::memcpy(buffer, reinterpret_cast<const void*>(reinterpret_cast<size_t>(_data.data()) + _index), len);
        _index += len;
    }

    void MemoryStream::write(const void* buffer, size_t len)
    {
        if (len != 0)
        {
            ensureLength(_index + len);
            std::memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(_data.data()) + _index), buffer, len);
            _index += len;
        }
    }
}
