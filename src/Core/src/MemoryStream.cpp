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

    size_t MemoryStream::getLength() const noexcept
    {
        return _data.size();
    }

    size_t MemoryStream::getPosition() const noexcept
    {
        return _offset;
    }

    void MemoryStream::setPosition(size_t position)
    {
        _offset = std::min(_data.size(), position);
    }

    void MemoryStream::read(void* buffer, size_t len)
    {
        auto maxReadLen = _data.size() - _offset;
        if (len > maxReadLen)
            throw std::runtime_error("Failed to read data");
        std::memcpy(buffer, _data.data() + _offset, len);
        _offset += len;
    }

    void MemoryStream::write(const void* buffer, size_t len)
    {
        if (len != 0)
        {
            ensureLength(_offset + len);
            std::memcpy(_data.data() + _offset, buffer, len);
            _offset += len;
        }
    }
}
