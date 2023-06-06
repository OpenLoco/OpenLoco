#pragma once

#include "Stream.hpp"
#include <cstdint>
#include <cstdio>
#include <vector>

namespace OpenLoco
{
    class MemoryStream final : public Stream
    {
        std::vector<std::byte> _data{};
        size_t _offset{};

    public:
        void ensureLength(size_t len);

        const void* data() const;

        void* data();

        size_t getLength() const noexcept override;

        size_t getPosition() const noexcept override;

        void setPosition(size_t position) override;

        void read(void* buffer, size_t len) override;

        void write(const void* buffer, size_t len) override;
    };

}
