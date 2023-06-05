#pragma once

#include "Stream.hpp"
#include <cstdint>
#include <cstdio>
#include <vector>

namespace OpenLoco
{
    class MemoryStream final : public Stream
    {
    private:
        std::vector<std::byte> _data{};
        size_t _index{};

        void ensureLength(size_t len)
        {
            if (_data.size() < len)
            {
                _data.resize(len);
            }
        }

    public:
        const void* data() const
        {
            return _data.data();
        }

        void* data()
        {
            return _data.data();
        }

        uint64_t getLength() const override
        {
            return _data.size();
        }

        uint64_t getPosition() const override
        {
            return _index;
        }

        void setPosition(uint64_t position) override
        {
            _index = static_cast<size_t>(position);
        }

        void read(void* buffer, size_t len) override
        {
            auto maxReadLen = _data.size() - _index;
            if (len > maxReadLen)
                throw std::runtime_error("Failed to read data");
            std::memcpy(buffer, reinterpret_cast<const void*>(reinterpret_cast<size_t>(_data.data()) + _index), len);
            _index += len;
        }

        void write(const void* buffer, size_t len) override
        {
            if (len != 0)
            {
                ensureLength(_index + len);
                std::memcpy(reinterpret_cast<void*>(reinterpret_cast<size_t>(_data.data()) + _index), buffer, len);
                _index += len;
            }
        }
    };

}
