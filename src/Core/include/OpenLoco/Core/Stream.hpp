#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <istream>
#include <nonstd/span.hpp>
#include <stdexcept>
#include <vector>

namespace stdx
{
    using nonstd::span;
}

namespace OpenLoco
{
    namespace Utility
    {
        // Obsolete methods, use new Stream APIs

        template<typename T1, typename T2, typename T3>
        std::basic_istream<T1, T2>& readData(std::basic_istream<T1, T2>& stream, T3* dst, size_t count)
        {
            return stream.read((char*)dst, static_cast<uint64_t>(count) * sizeof(T3));
        }

        template<typename T1, typename T2, typename T3>
        std::basic_istream<T1, T2>& readData(std::basic_istream<T1, T2>& stream, T3& dst)
        {
            return readData(stream, &dst, 1);
        }

        template<typename T3, typename T1, typename T2>
        T3 readValue(std::basic_istream<T1, T2>& stream)
        {
            T3 result{};
            readData(stream, result);
            return result;
        }
    }

    enum class StreamMode
    {
        read,
        write,
    };

    class Stream
    {
    public:
        virtual ~Stream() {}
        virtual uint64_t getLength() const { throwInvalidOperation(); }
        virtual uint64_t getPosition() const { throwInvalidOperation(); }
        virtual void setPosition(uint64_t) { throwInvalidOperation(); }
        virtual void read(void*, size_t) { throwInvalidOperation(); }
        virtual void write(const void*, size_t) { throwInvalidOperation(); }

        void seek(int64_t pos)
        {
            setPosition(getPosition() + pos);
        }

    private:
        [[noreturn]] static void throwInvalidOperation(...) { throw std::runtime_error("Invalid operation"); }
    };

    class BinaryStream final : public Stream
    {
    private:
        const void* _data{};
        size_t _index{};
        size_t _len{};

    public:
        BinaryStream(const void* data, size_t len)
            : _data(data)
            , _len(len)
        {
        }

        uint64_t getLength() const override
        {
            return _len;
        }

        uint64_t getPosition() const override
        {
            return _index;
        }

        void setPosition(uint64_t position) override
        {
            if (position > _len)
                throw std::out_of_range("Position too large");
            _index = static_cast<size_t>(position);
        }

        void read(void* buffer, size_t len) override
        {
            auto maxReadLen = _len - _index;
            if (len > maxReadLen)
                throw std::runtime_error("Failed to read data");
            std::memcpy(buffer, reinterpret_cast<const void*>(reinterpret_cast<size_t>(_data) + _index), len);
            _index += len;
        }
    };

    class FileStream final : public Stream
    {
    private:
        std::fstream _fstream;
        bool _reading{};
        bool _writing{};

    public:
        FileStream(const std::filesystem::path path, StreamMode mode)
        {
            if (mode == StreamMode::write)
            {
                _fstream.open(path, std::ios::out | std::ios::binary);
                if (!_fstream.is_open())
                {
                    throw std::runtime_error("Failed to open '" + path.u8string() + "' for writing");
                }
                _reading = true;
                _writing = true;
            }
            else
            {
                _fstream.open(path, std::ios::in | std::ios::binary);
                if (!_fstream.is_open())
                {
                    throw std::runtime_error("Failed to open '" + path.u8string() + "' for reading");
                }
                _reading = true;
            }
        }

        uint64_t getLength() const override
        {
            auto* fs = const_cast<std::fstream*>(&_fstream);
            if (_writing)
            {
                auto backup = fs->tellp();
                fs->seekp(0, std::ios_base::end);
                auto len = fs->tellp();
                fs->seekp(backup);
                return len;
            }
            else
            {
                auto backup = fs->tellg();
                fs->seekg(0, std::ios_base::end);
                auto len = fs->tellg();
                fs->seekg(backup);
                return len;
            }
        }

        uint64_t getPosition() const override
        {
            auto* fs = const_cast<std::fstream*>(&_fstream);
            if (_writing)
            {
                return static_cast<uint64_t>(fs->tellp());
            }
            else
            {
                return static_cast<uint64_t>(fs->tellg());
            }
        }

        void setPosition(uint64_t position) override
        {
            if (_reading)
                _fstream.seekg(position);
            if (_writing)
                _fstream.seekp(position);
        }

        void read(void* buffer, size_t len) override
        {
            _fstream.read(static_cast<char*>(buffer), len);
            if (_fstream.fail())
                throw std::runtime_error("Failed to read data");
        }

        void write(const void* buffer, size_t len) override
        {
            if (len != 0)
            {
                _fstream.write(static_cast<const char*>(buffer), len);
                if (_fstream.fail())
                    throw std::runtime_error("Failed to write data");
            }
        }
    };
}
