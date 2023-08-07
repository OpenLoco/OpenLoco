#include "BinaryStream.h"
#include "Exception.hpp"
#include <cstring>

namespace OpenLoco
{
    BinaryStream::BinaryStream(const void* data, size_t len)
        : _data(data)
        , _len(len)
    {
    }

    size_t BinaryStream::getLength() const noexcept
    {
        return _len;
    }

    size_t BinaryStream::getPosition() const noexcept
    {
        return _index;
    }

    void BinaryStream::setPosition(size_t position)
    {
        _index = std::min(_len, position);
    }

    void BinaryStream::read(void* buffer, size_t len)
    {
        auto maxReadLen = _len - _index;
        if (len > maxReadLen)
            throw Exception::RuntimeError("Failed to read data");
        std::memcpy(buffer, reinterpret_cast<const void*>(reinterpret_cast<size_t>(_data) + _index), len);
        _index += len;
    }

    void BinaryStream::write([[maybe_unused]] const void* buffer, [[maybe_unused]] size_t len)
    {
        throw Exception::InvalidOperation("Can not write");
    }
}
