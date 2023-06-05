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
        BinaryStream(const void* data, size_t len);

        uint64_t getLength() const noexcept override;

        uint64_t getPosition() const noexcept override;

        void setPosition(uint64_t position) override;

        void read(void* buffer, size_t len) override;

        void write(const void* buffer, size_t len) override;
    };

}
