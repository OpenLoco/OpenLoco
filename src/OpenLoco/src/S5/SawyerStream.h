#pragma once

#include <OpenLoco/Core/FileStream.h>
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Core/MemoryStream.h>
#include <OpenLoco/Core/Span.hpp>
#include <cstdint>
#include <memory>

namespace OpenLoco
{
    enum class SawyerEncoding : uint8_t
    {
        uncompressed,
        runLengthSingle,
        runLengthMulti,
        rotate,
    };

    /**
     * Provides a more efficient implementation than std::vector for allocating and
     * pushing bytes to a buffer.
     *
     * In particular, for Windows, HeapAlloc is used for a direct memory block from the OS
     * which is not initialised (even in debug builds). It vastly reduces the time needed
     * to load / save S5 files in debug builds.
     */
    class FastBuffer
    {
    private:
        std::byte* _data{};
        size_t _len{};
        size_t _capacity{};

        static std::byte* alloc(size_t len);
        static std::byte* realloc(std::byte* ptr, size_t len);
        static void free(std::byte* ptr);

    public:
        ~FastBuffer();

        std::byte* data();
        size_t size() const;
        void resize(size_t len);
        void reserve(size_t len);
        void clear();
        void push_back(std::byte value);
        void push_back(std::byte value, size_t len);
        void push_back(const std::byte* src, size_t len);
        stdx::span<const std::byte> getSpan() const;
    };

    class SawyerStreamReader
    {
    private:
        Stream& _stream;
        FastBuffer _decodeBuffer;
        FastBuffer _decodeBuffer2;

        stdx::span<const std::byte> decode(SawyerEncoding encoding, stdx::span<const std::byte> data);
        static void decodeRunLengthSingle(FastBuffer& buffer, stdx::span<const std::byte> data);
        static void decodeRunLengthMulti(FastBuffer& buffer, stdx::span<const std::byte> data);
        static void decodeRotate(FastBuffer& buffer, stdx::span<const std::byte> data);

    public:
        SawyerStreamReader(Stream& stream);

        stdx::span<const std::byte> readChunk();
        size_t readChunk(void* data, size_t maxDataLen);
        void read(void* data, size_t dataLen);
        bool validateChecksum();
    };

    class SawyerStreamWriter
    {
    private:
        Stream& _stream;
        uint32_t _checksum{};
        FastBuffer _encodeBuffer;
        FastBuffer _encodeBuffer2;

        void writeStream(const void* data, size_t dataLen);
        stdx::span<const std::byte> encode(SawyerEncoding encoding, stdx::span<const std::byte> data);
        static void encodeRunLengthSingle(FastBuffer& buffer, stdx::span<const std::byte> data);
        static void encodeRunLengthMulti(FastBuffer& buffer, stdx::span<const std::byte> data);
        static void encodeRotate(FastBuffer& buffer, stdx::span<const std::byte> data);

    public:
        SawyerStreamWriter(Stream& stream);

        void writeChunk(SawyerEncoding chunkType, const void* data, size_t dataLen);
        void write(const void* data, size_t dataLen);
        void writeChecksum();

        template<typename T>
        void writeChunk(SawyerEncoding chunkType, const T& data)
        {
            writeChunk(chunkType, &data, sizeof(T));
        }

        template<typename T>
        void write(const T& data)
        {
            write(&data, sizeof(T));
        }
    };
}
