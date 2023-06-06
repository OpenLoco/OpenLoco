#pragma once

#include "Stream.hpp"
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace OpenLoco
{
    class FileStream final : public Stream
    {
    private:
        std::fstream _fstream;
        StreamMode _mode{};
        size_t _length{};
        size_t _offset{};

    public:
        FileStream(const std::filesystem::path& path, StreamMode mode);

        bool open(const std::filesystem::path& path, StreamMode mode);

        bool isOpen() const noexcept;

        void close();

        StreamMode getMode() const noexcept;

        uint64_t getLength() const noexcept override;

        uint64_t getPosition() const noexcept override;

        void setPosition(uint64_t position) override;

        void read(void* buffer, size_t len) override;

        void write(const void* buffer, size_t len) override;
    };
}
