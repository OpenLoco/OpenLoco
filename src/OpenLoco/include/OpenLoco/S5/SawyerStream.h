#pragma once

#include <OpenLoco/Core/FileStream.h>
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Core/MemoryStream.h>
#include <cstdint>
#include <memory>
#include <span>

namespace OpenLoco
{
    enum class SawyerEncoding : uint8_t
    {
        uncompressed,
        runLengthSingle,
        runLengthMulti,
        rotate,
    };

    class SawyerStreamReader
    {
    private:
        Stream& _stream;
        MemoryStream _decodeBuffer;
        MemoryStream _decodeBuffer2;

        std::span<const std::byte> decode(SawyerEncoding encoding, std::span<const std::byte> data);
        static void decodeRunLengthSingle(MemoryStream& buffer, std::span<const std::byte> data);
        static void decodeRunLengthMulti(MemoryStream& buffer, std::span<const std::byte> data);
        static void decodeRotate(MemoryStream& buffer, std::span<const std::byte> data);

    public:
        SawyerStreamReader(Stream& stream);

        std::span<const std::byte> readChunk();
        size_t readChunk(void* data, size_t maxDataLen);
        void read(void* data, size_t dataLen);
        bool validateChecksum();
    };

    class SawyerStreamWriter
    {
    private:
        Stream& _stream;
        uint32_t _checksum{};
        MemoryStream _encodeBuffer;
        MemoryStream _encodeBuffer2;

        void writeStream(const void* data, size_t dataLen);
        std::span<const std::byte> encode(SawyerEncoding encoding, std::span<const std::byte> data);
        static void encodeRunLengthSingle(MemoryStream& buffer, std::span<const std::byte> data);
        static void encodeRunLengthMulti(MemoryStream& buffer, std::span<const std::byte> data);
        static void encodeRotate(MemoryStream& buffer, std::span<const std::byte> data);

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
