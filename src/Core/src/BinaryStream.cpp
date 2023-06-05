#include "BinaryStream.h"

namespace OpenLoco
{

    BinaryStream::BinaryStream(const void* data, size_t len)
        : _data(data)
        , _len(len)
    {
    }

    uint64_t BinaryStream::getLength() const noexcept
    {
        return _len;
    }

    uint64_t BinaryStream::getPosition() const noexcept
    {
        return _index;
    }

    void BinaryStream::setPosition(uint64_t position)
    {
        if (position > _len)
            throw std::out_of_range("Position too large");
        _index = static_cast<size_t>(position);
    }

    void BinaryStream::read(void* buffer, size_t len)
    {
        auto maxReadLen = _len - _index;
        if (len > maxReadLen)
            throw std::runtime_error("Failed to read data");
        std::memcpy(buffer, reinterpret_cast<const void*>(reinterpret_cast<size_t>(_data) + _index), len);
        _index += len;
    }

    void BinaryStream::write([[maybe_unused]] const void* buffer, [[maybe_unused]] size_t len)
    {
        throwInvalidOperation();
    }

}
