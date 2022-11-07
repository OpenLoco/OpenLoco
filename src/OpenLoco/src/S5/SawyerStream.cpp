#include "SawyerStream.h"
#include "../Utility/Numeric.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <memory>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace OpenLoco;

constexpr const char* exceptionReadError = "Failed to read data from stream";
constexpr const char* exceptionWriteError = "Failed to write data to stream";
constexpr const char* exceptionInvalidRLE = "Invalid RLE run";
constexpr const char* exceptionUnknownEncoding = "Unknown encoding";

uint8_t* FastBuffer::alloc(size_t len)
{
#ifdef _WIN32
    return reinterpret_cast<uint8_t*>(HeapAlloc(GetProcessHeap(), 0, len));
#else
    return reinterpret_cast<uint8_t*>(std::malloc(len));
#endif
}

uint8_t* FastBuffer::realloc(uint8_t* ptr, size_t len)
{
#ifdef _WIN32
    return reinterpret_cast<uint8_t*>(HeapReAlloc(GetProcessHeap(), 0, ptr, len));
#else
    return reinterpret_cast<uint8_t*>(std::realloc(ptr, len));
#endif
}

void FastBuffer::free(uint8_t* ptr)
{
#ifdef _WIN32
    HeapFree(GetProcessHeap(), 0, ptr);
#else
    std::free(ptr);
#endif
}

FastBuffer::~FastBuffer()
{
    if (_data != nullptr)
    {
        free(_data);
        _data = 0;
        _len = 0;
        _capacity = 0;
    }
}

uint8_t* FastBuffer::data()
{
    return _data;
}

size_t FastBuffer::size() const
{
    return _len;
}

void FastBuffer::resize(size_t len)
{
    reserve(len);
    _len = len;
}

void FastBuffer::reserve(size_t len)
{
    if (_capacity < len)
    {
        do
        {
            _capacity = std::max<size_t>(256, _capacity * 2);
        } while (_capacity < len);
        auto newData = _data == nullptr ? alloc(_capacity) : realloc(_data, _capacity);
        if (newData == nullptr)
        {
            throw std::bad_alloc();
        }
        else
        {
            _data = reinterpret_cast<uint8_t*>(newData);
        }
    }
}

void FastBuffer::clear()
{
    _len = 0;
}

void FastBuffer::push_back(uint8_t value)
{
    reserve(_len + 1);
    _data[_len++] = value;
}

void FastBuffer::push_back(uint8_t value, size_t len)
{
    reserve(_len + len);
    std::memset(&_data[_len], value, len);
    _len += len;
}

void FastBuffer::push_back(const uint8_t* src, size_t len)
{
    reserve(_len + len);
    std::memcpy(&_data[_len], src, len);
    _len += len;
}

stdx::span<uint8_t const> FastBuffer::getSpan() const
{
    return stdx::span<uint8_t const>(_data, _len);
}

SawyerStreamReader::SawyerStreamReader(Stream& stream)
{
    _stream = &stream;
}

SawyerStreamReader::SawyerStreamReader(const fs::path& path)
{
    _fstream = std::make_unique<FileStream>(path, StreamFlags::read);
    _stream = _fstream.get();
}

stdx::span<uint8_t const> SawyerStreamReader::readChunk()
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
        _stream->read(data, dataLen);
    }
    catch (...)
    {
        throw std::runtime_error(exceptionReadError);
    }
}

bool SawyerStreamReader::validateChecksum()
{
    auto valid = false;
    auto backupPos = _stream->getPosition();
    auto fileLength = static_cast<uint32_t>(_stream->getLength());
    if (fileLength >= 4)
    {
        // Read checksum
        uint32_t checksum;
        _stream->seek(fileLength - 4);
        _stream->read(&checksum, sizeof(checksum));

        // Calculate checksum
        uint32_t actualChecksum = 0;
        _stream->setPosition(0);
        uint8_t buffer[2048];
        for (uint32_t i = 0; i < fileLength - 4; i += sizeof(buffer))
        {
            auto readLength = std::min<size_t>(sizeof(buffer), fileLength - 4 - i);
            _stream->read(buffer, readLength);
            for (size_t j = 0; j < readLength; j++)
            {
                actualChecksum += buffer[j];
            }
        }

        valid = checksum == actualChecksum;
    }

    // Restore position
    _stream->setPosition(backupPos);

    return valid;
}

void SawyerStreamReader::close()
{
    _fstream = {};
    _stream = nullptr;
}

stdx::span<uint8_t const> SawyerStreamReader::decode(SawyerEncoding encoding, stdx::span<uint8_t const> data)
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
            _decodeBuffer.reserve(_decodeBuffer2.size());
            decodeRunLengthMulti(_decodeBuffer, _decodeBuffer2.getSpan());
            return _decodeBuffer.getSpan();
        case SawyerEncoding::rotate:
            _decodeBuffer2.clear();
            _decodeBuffer2.reserve(data.size());
            decodeRotate(_decodeBuffer2, data);
            return _decodeBuffer2.getSpan();
        default:
            throw std::runtime_error(exceptionUnknownEncoding);
    }
}

void SawyerStreamReader::decodeRunLengthSingle(FastBuffer& buffer, stdx::span<uint8_t const> data)
{
    for (size_t i = 0; i < data.size(); i++)
    {
        uint8_t rleCodeByte = data[i];
        if (rleCodeByte & 128)
        {
            i++;
            if (i >= data.size())
            {
                throw std::runtime_error(exceptionInvalidRLE);
            }

            auto copyLen = static_cast<size_t>(257 - rleCodeByte);
            auto copyByte = data[i];
            buffer.push_back(copyByte, copyLen);
        }
        else
        {
            if (i + 1 >= data.size() || i + 1 + rleCodeByte + 1 > data.size())
            {
                throw std::runtime_error(exceptionInvalidRLE);
            }

            auto copyLen = static_cast<size_t>(rleCodeByte + 1);
            buffer.push_back(&data[i + 1], copyLen);
            i += rleCodeByte + 1;
        }
    }
}

void SawyerStreamReader::decodeRunLengthMulti(FastBuffer& buffer, stdx::span<uint8_t const> data)
{
    for (size_t i = 0; i < data.size(); i++)
    {
        if (data[i] == 0xFF)
        {
            i++;
            if (i >= data.size())
            {
                throw std::runtime_error(exceptionInvalidRLE);
            }
            buffer.push_back(data[i]);
        }
        else
        {
            auto offset = static_cast<int32_t>(data[i] >> 3) - 32;
            assert(offset < 0);
            if (static_cast<size_t>(-offset) > buffer.size())
            {
                throw std::runtime_error(exceptionInvalidRLE);
            }
            auto copySrc = buffer.data() + buffer.size() + offset;
            auto copyLen = static_cast<size_t>((data[i] & 7) + 1);

            // Copy it to temp buffer first as we can't copy buffer to itself due to potential
            // realloc in between reserve and push
            uint8_t copyBuffer[32];
            assert(copyLen <= sizeof(copyBuffer));
            std::memcpy(copyBuffer, copySrc, copyLen);
            buffer.push_back(copyBuffer, copyLen);
        }
    }
}

void SawyerStreamReader::decodeRotate(FastBuffer& buffer, stdx::span<uint8_t const> data)
{
    uint8_t code = 1;
    for (size_t i = 0; i < data.size(); i++)
    {
        buffer.push_back(OpenLoco::Utility::ror(data[i], code));
        code = (code + 2) & 7;
    }
}

SawyerStreamWriter::SawyerStreamWriter(Stream& stream)
{
    _stream = &stream;
}

SawyerStreamWriter::SawyerStreamWriter(const fs::path& path)
{
    _fstream = std::make_unique<FileStream>(path, StreamFlags::write);
    _stream = _fstream.get();
}

void SawyerStreamWriter::writeChunk(SawyerEncoding chunkType, const void* data, size_t dataLen)
{
    auto encodedData = encode(chunkType, stdx::span(reinterpret_cast<const uint8_t*>(data), dataLen));
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
        _stream->write(data, dataLen);
    }
    catch (...)
    {
        throw std::runtime_error(exceptionWriteError);
    }
}

void SawyerStreamWriter::close()
{
    _fstream = {};
    _stream = nullptr;
}

stdx::span<uint8_t const> SawyerStreamWriter::encode(SawyerEncoding encoding, stdx::span<uint8_t const> data)
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
            _encodeBuffer2.reserve(_encodeBuffer.size());
            encodeRunLengthSingle(_encodeBuffer2, _encodeBuffer.getSpan());
            return _encodeBuffer2.getSpan();
        case SawyerEncoding::rotate:
            _encodeBuffer.clear();
            _encodeBuffer.reserve(data.size());
            encodeRotate(_encodeBuffer, data);
            return _encodeBuffer.getSpan();
        default:
            throw std::runtime_error(exceptionUnknownEncoding);
    }
}

void SawyerStreamWriter::encodeRunLengthSingle(FastBuffer& buffer, stdx::span<uint8_t const> data)
{
    auto src = data.data();
    auto srcEnd = src + data.size();
    auto srcNormStart = src;
    uint8_t count = 0;

    while (src < srcEnd - 1)
    {
        if ((count != 0 && src[0] == src[1]) || count > 125)
        {
            buffer.push_back(count - 1);
            buffer.push_back(srcNormStart, count);
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
            buffer.push_back(257 - count);
            buffer.push_back(src[0]);
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
        buffer.push_back(count - 1);
        buffer.push_back(srcNormStart, count);
    }
}

void SawyerStreamWriter::encodeRunLengthMulti(FastBuffer& buffer, stdx::span<uint8_t const> data)
{
    auto src = data.data();
    auto srcLen = data.size();
    if (srcLen == 0)
        return;

    // Need to emit at least one byte, otherwise there is nothing to repeat
    buffer.push_back(255);
    buffer.push_back(src[0]);

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
            buffer.push_back(255);
            buffer.push_back(src[i]);
            i++;
        }
        else
        {
            buffer.push_back(static_cast<uint8_t>((bestRepeatCount - 1) | ((32 - (i - bestRepeatIndex)) << 3)));
            i += bestRepeatCount;
        }
    }
}

void SawyerStreamWriter::encodeRotate(FastBuffer& buffer, stdx::span<uint8_t const> data)
{
    uint8_t code = 1;
    for (size_t i = 0; i < data.size(); i++)
    {
        buffer.push_back(Utility::rol(data[i], code));
        code = (code + 2) & 7;
    }
}
