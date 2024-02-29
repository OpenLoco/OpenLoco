#include "SawyerStream.h"
#include <OpenLoco/Core/Exception.hpp>
#include <algorithm>
#include <bit>
#include <cassert>
#include <cstring>
#include <memory>
#include <stdexcept>

using namespace OpenLoco;

constexpr const char* exceptionReadError = "Failed to read data from stream";
constexpr const char* exceptionWriteError = "Failed to write data to stream";
constexpr const char* exceptionInvalidRLE = "Invalid RLE run";
constexpr const char* exceptionUnknownEncoding = "Unknown encoding";

SawyerStreamReader::SawyerStreamReader(Stream& stream)
    : _stream(stream)
{
}

std::span<const std::byte> SawyerStreamReader::readChunk()
{
    SawyerEncoding encoding;
    read(&encoding, sizeof(encoding));

    uint32_t length;
    read(&length, sizeof(length));

    _decodeBuffer.resize(length);
    read(_decodeBuffer.data(), length);

    return decode(encoding, _decodeBuffer.getSpan());
}

size_t SawyerStreamReader::readChunk(void* data, size_t maxDataLen)
{
    auto chunkData = readChunk();
    std::memcpy(data, chunkData.data(), std::min(chunkData.size(), maxDataLen));
    return chunkData.size();
}

void SawyerStreamReader::read(void* data, size_t dataLen)
{
    try
    {
        _stream.read(data, dataLen);
    }
    catch (...)
    {
        throw Exception::RuntimeError(exceptionReadError);
    }
}

bool SawyerStreamReader::validateChecksum()
{
    auto valid = false;
    auto backupPos = _stream.getPosition();
    auto fileLength = static_cast<uint32_t>(_stream.getLength());
    if (fileLength >= 4)
    {
        // Read checksum
        uint32_t checksum;
        _stream.setPosition(fileLength - 4);
        _stream.read(&checksum, sizeof(checksum));

        // Calculate checksum
        uint32_t actualChecksum = 0;
        _stream.setPosition(0);
        uint8_t buffer[2048];
        for (uint32_t i = 0; i < fileLength - 4; i += sizeof(buffer))
        {
            auto readLength = std::min<size_t>(sizeof(buffer), fileLength - 4 - i);
            _stream.read(buffer, readLength);
            for (size_t j = 0; j < readLength; j++)
            {
                actualChecksum += buffer[j];
            }
        }

        valid = checksum == actualChecksum;
    }

    // Restore position
    _stream.setPosition(backupPos);

    return valid;
}

std::span<const std::byte> SawyerStreamReader::decode(SawyerEncoding encoding, std::span<const std::byte> data)
{
    switch (encoding)
    {
        case SawyerEncoding::uncompressed:
            return data;
        case SawyerEncoding::runLengthSingle:
            _decodeBuffer2.clear();
            _decodeBuffer2.reserve(data.size());
            decodeRunLengthSingle(_decodeBuffer2, data);
            return _decodeBuffer2.getSpan();
        case SawyerEncoding::runLengthMulti:
            _decodeBuffer2.clear();
            _decodeBuffer2.reserve(data.size());
            decodeRunLengthSingle(_decodeBuffer2, data);

            _decodeBuffer.clear();
            _decodeBuffer.reserve(_decodeBuffer2.getLength());
            decodeRunLengthMulti(_decodeBuffer, _decodeBuffer2.getSpan());
            return _decodeBuffer.getSpan();
        case SawyerEncoding::rotate:
            _decodeBuffer2.clear();
            _decodeBuffer2.reserve(data.size());
            decodeRotate(_decodeBuffer2, data);
            return _decodeBuffer2.getSpan();
        default:
            throw Exception::RuntimeError(exceptionUnknownEncoding);
    }
}

static void fillStream(MemoryStream& buffer, std::byte value, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        buffer.writeValue(value);
    }
}

void SawyerStreamReader::decodeRunLengthSingle(MemoryStream& buffer, std::span<const std::byte> data)
{
    for (size_t i = 0; i < data.size(); i++)
    {
        uint8_t rleCodeByte = static_cast<uint8_t>(data[i]);
        if (rleCodeByte & 128)
        {
            i++;
            if (i >= data.size())
            {
                throw Exception::RuntimeError(exceptionInvalidRLE);
            }

            auto copyLen = static_cast<size_t>(257 - rleCodeByte);
            auto copyByte = data[i];
            fillStream(buffer, copyByte, copyLen);
        }
        else
        {
            if (i + 1 >= data.size() || i + 1 + rleCodeByte + 1 > data.size())
            {
                throw Exception::RuntimeError(exceptionInvalidRLE);
            }

            auto copyLen = static_cast<size_t>(rleCodeByte + 1);
            buffer.write(&data[i + 1], copyLen);
            i += rleCodeByte + 1;
        }
    }
}

void SawyerStreamReader::decodeRunLengthMulti(MemoryStream& buffer, std::span<const std::byte> data)
{
    for (size_t i = 0; i < data.size(); i++)
    {
        if (data[i] == std::byte{ 0xFF })
        {
            i++;
            if (i >= data.size())
            {
                throw Exception::RuntimeError(exceptionInvalidRLE);
            }
            buffer.writeValue(data[i]);
        }
        else
        {
            auto offset = static_cast<int32_t>(data[i] >> 3) - 32;
            assert(offset < 0);
            if (static_cast<size_t>(-offset) > buffer.getLength())
            {
                throw Exception::RuntimeError(exceptionInvalidRLE);
            }
            auto copySrc = buffer.data() + buffer.getLength() + offset;
            auto copyLen = (static_cast<size_t>(data[i]) & 7) + 1;

            // Copy it to temp buffer first as we can't copy buffer to itself due to potential
            // realloc in between reserve and push
            std::byte copyBuffer[32];
            assert(copyLen <= sizeof(copyBuffer));
            std::memcpy(copyBuffer, copySrc, copyLen);
            buffer.write(copyBuffer, copyLen);
        }
    }
}

void SawyerStreamReader::decodeRotate(MemoryStream& buffer, std::span<const std::byte> data)
{
    uint8_t code = 1;
    for (size_t i = 0; i < data.size(); i++)
    {
        buffer.writeValue(std::rotr(static_cast<uint8_t>(data[i]), code));
        code = (code + 2) & 7;
    }
}

SawyerStreamWriter::SawyerStreamWriter(Stream& stream)
    : _stream(stream)
{
}

void SawyerStreamWriter::writeChunk(SawyerEncoding chunkType, const void* data, size_t dataLen)
{
    auto encodedData = encode(chunkType, std::span(static_cast<const std::byte*>(data), dataLen));
    write(&chunkType, sizeof(chunkType));
    write(static_cast<uint32_t>(encodedData.size()));
    write(encodedData.data(), encodedData.size());
}

void SawyerStreamWriter::write(const void* data, size_t dataLen)
{
    writeStream(data, dataLen);
    auto data8 = reinterpret_cast<const uint8_t*>(data);
    for (size_t i = 0; i < dataLen; i++)
    {
        _checksum += data8[i];
    }
}

void SawyerStreamWriter::writeChecksum()
{
    writeStream(&_checksum, sizeof(_checksum));
}

void SawyerStreamWriter::writeStream(const void* data, size_t dataLen)
{
    try
    {
        _stream.write(data, dataLen);
    }
    catch (...)
    {
        throw Exception::RuntimeError(exceptionWriteError);
    }
}

std::span<const std::byte> SawyerStreamWriter::encode(SawyerEncoding encoding, std::span<const std::byte> data)
{
    switch (encoding)
    {
        case SawyerEncoding::uncompressed:
            return data;
        case SawyerEncoding::runLengthSingle:
            _encodeBuffer.clear();
            _encodeBuffer.reserve(data.size());
            encodeRunLengthSingle(_encodeBuffer, data);
            return _encodeBuffer.getSpan();
        case SawyerEncoding::runLengthMulti:
            _encodeBuffer.clear();
            _encodeBuffer.reserve(data.size());
            encodeRunLengthMulti(_encodeBuffer, data);

            _encodeBuffer2.clear();
            _encodeBuffer2.reserve(_encodeBuffer.getLength());
            encodeRunLengthSingle(_encodeBuffer2, _encodeBuffer.getSpan());
            return _encodeBuffer2.getSpan();
        case SawyerEncoding::rotate:
            _encodeBuffer.clear();
            _encodeBuffer.reserve(data.size());
            encodeRotate(_encodeBuffer, data);
            return _encodeBuffer.getSpan();
        default:
            throw Exception::RuntimeError(exceptionUnknownEncoding);
    }
}

void SawyerStreamWriter::encodeRunLengthSingle(MemoryStream& buffer, std::span<const std::byte> data)
{
    auto src = data.data();
    auto srcEnd = src + data.size();
    auto srcNormStart = src;
    uint8_t count = 0;

    while (src < srcEnd - 1)
    {
        if ((count != 0 && src[0] == src[1]) || count > 125)
        {
            buffer.writeValue(static_cast<std::byte>(count - 1));
            buffer.write(srcNormStart, count);
            srcNormStart += count;
            count = 0;
        }
        if (src[0] == src[1])
        {
            for (; count < 125 && src + count < srcEnd; count++)
            {
                if (src[0] != src[count])
                {
                    break;
                }
            }
            buffer.writeValue(static_cast<std::byte>(257 - count));
            buffer.writeValue(src[0]);
            src += count;
            srcNormStart = src;
            count = 0;
        }
        else
        {
            count++;
            src++;
        }
    }
    if (src == srcEnd - 1)
    {
        count++;
    }
    if (count != 0)
    {
        buffer.writeValue(static_cast<std::byte>(count - 1));
        buffer.write(srcNormStart, count);
    }
}

void SawyerStreamWriter::encodeRunLengthMulti(MemoryStream& buffer, std::span<const std::byte> data)
{
    auto src = data.data();
    auto srcLen = data.size();
    if (srcLen == 0)
        return;

    // Need to emit at least one byte, otherwise there is nothing to repeat
    buffer.writeValue(std::byte{ 0xFF });
    buffer.writeValue(src[0]);

    // Iterate through remainder of the source buffer
    for (size_t i = 1; i < srcLen;)
    {
        size_t searchIndex = (i < 32) ? 0 : (i - 32);
        size_t searchEnd = i - 1;

        size_t bestRepeatIndex = 0;
        size_t bestRepeatCount = 0;
        for (size_t repeatIndex = searchIndex; repeatIndex <= searchEnd; repeatIndex++)
        {
            size_t repeatCount = 0;
            size_t maxRepeatCount = std::min(std::min(static_cast<size_t>(7), searchEnd - repeatIndex), srcLen - i - 1);
            // maxRepeatCount should not exceed srcLen
            assert(repeatIndex + maxRepeatCount < srcLen);
            assert(i + maxRepeatCount < srcLen);
            for (size_t j = 0; j <= maxRepeatCount; j++)
            {
                if (src[repeatIndex + j] == src[i + j])
                {
                    repeatCount++;
                }
                else
                {
                    break;
                }
            }
            if (repeatCount > bestRepeatCount)
            {
                bestRepeatIndex = repeatIndex;
                bestRepeatCount = repeatCount;

                // Maximum repeat count is 8
                if (repeatCount == 8)
                    break;
            }
        }

        if (bestRepeatCount == 0)
        {
            buffer.writeValue(std::byte{ 0xFF });
            buffer.writeValue(src[i]);
            i++;
        }
        else
        {
            buffer.writeValue(static_cast<std::byte>((bestRepeatCount - 1) | ((32 - (i - bestRepeatIndex)) << 3)));
            i += bestRepeatCount;
        }
    }
}

void SawyerStreamWriter::encodeRotate(MemoryStream& buffer, std::span<const std::byte> data)
{
    uint8_t code = 1;
    for (size_t i = 0; i < data.size(); i++)
    {
        buffer.writeValue(std::rotl(static_cast<uint8_t>(data[i]), code));
        code = (code + 2) & 7;
    }
}
