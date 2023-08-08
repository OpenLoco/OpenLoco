#pragma once

#include "Stream.hpp"
#include <cstdint>
#include <cstdio>
#include <filesystem>

namespace OpenLoco
{
    class FileStream final : public Stream
    {
    private:
        FILE* _file{};
        StreamMode _mode{};
        size_t _length{};
        size_t _offset{};

    public:
        FileStream() = default;
        FileStream(const std::filesystem::path& path, StreamMode mode);
        ~FileStream();

        bool open(const std::filesystem::path& path, StreamMode mode);

        bool isOpen() const noexcept;

        void close();

        StreamMode getMode() const noexcept;

        size_t getLength() const noexcept override;

        size_t getPosition() const noexcept override;

        void setPosition(size_t position) override;

        void read(void* buffer, size_t len) override;

        void write(const void* buffer, size_t len) override;
    };
}
