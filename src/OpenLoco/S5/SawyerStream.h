#pragma once

#include "../Core/FileSystem.hpp"
#include "../Core/Span.hpp"
#include "../Utility/Stream.hpp"
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
        uint8_t* _data{};
        size_t _len{};
        size_t _capacity{};

        static uint8_t* alloc(size_t len);
        static uint8_t* realloc(uint8_t* ptr, size_t len);
        static void free(uint8_t* ptr);

    public:
        ~FastBuffer();

        uint8_t* data();
        size_t size() const;
        void resize(size_t len);
        void reserve(size_t len);
        void clear();
        void push_back(uint8_t value);
        void push_back(uint8_t value, size_t len);
        void push_back(const uint8_t* src, size_t len);
        stdx::span<uint8_t const> getSpan() const;
    };

    class SawyerStreamReader
    {
    private:
        Stream* _stream;
        std::unique_ptr<FileStream> _fstream;
        FastBuffer _decodeBuffer;
        FastBuffer _decodeBuffer2;

        stdx::span<uint8_t const> decode(SawyerEncoding encoding, stdx::span<uint8_t const> data);
        static void decodeRunLengthSingle(FastBuffer& buffer, stdx::span<uint8_t const> data);
        static void decodeRunLengthMulti(FastBuffer& buffer, stdx::span<uint8_t const> data);
        static void decodeRotate(FastBuffer& buffer, stdx::span<uint8_t const> data);

    public:
        SawyerStreamReader(Stream& stream);
        SawyerStreamReader(const fs::path& path);

        stdx::span<uint8_t const> readChunk();
        size_t readChunk(void* data, size_t maxDataLen);
        void read(void* data, size_t dataLen);
        bool validateChecksum();
        void close();
    };

    class SawyerStreamWriter
    {
    private:
        Stream* _stream;
        std::unique_ptr<FileStream> _fstream;
        uint32_t _checksum{};
        FastBuffer _encodeBuffer;
        FastBuffer _encodeBuffer2;

        void writeStream(const void* data, size_t dataLen);
        stdx::span<uint8_t const> encode(SawyerEncoding encoding, stdx::span<uint8_t const> data);
        static void encodeRunLengthSingle(FastBuffer& buffer, stdx::span<uint8_t const> data);
        static void encodeRunLengthMulti(FastBuffer& buffer, stdx::span<uint8_t const> data);
        static void encodeRotate(FastBuffer& buffer, stdx::span<uint8_t const> data);

    public:
        SawyerStreamWriter(Stream& stream);
        SawyerStreamWriter(const fs::path& path);

        void writeChunk(SawyerEncoding chunkType, const void* data, size_t dataLen);
        void write(const void* data, size_t dataLen);
        void writeChecksum();
        void close();

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
