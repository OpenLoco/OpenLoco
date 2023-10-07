#pragma once

#include "Stream.hpp"
#include <cstdint>
#include <cstdio>
#include <span>
#include <vector>

namespace OpenLoco
{
    class MemoryStream final : public Stream
    {
        std::byte* _data{};
        size_t _offset{};
        size_t _length{};
        size_t _capacity{};

    public:
        ~MemoryStream();

        void reserve(size_t len);

        void resize(size_t len);

        void clear();

        const std::byte* data() const;

        std::byte* data();

        std::span<std::byte> getSpan();

        std::span<const std::byte> getSpan() const;

        size_t getLength() const noexcept override;

        size_t getPosition() const noexcept override;

        void setPosition(size_t position) override;

        void read(void* buffer, size_t len) override;

        void write(const void* buffer, size_t len) override;
    };
}
