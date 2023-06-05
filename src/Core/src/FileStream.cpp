#include "FileStream.h"
#include <stdexcept>

namespace OpenLoco
{

    FileStream::FileStream(const std::filesystem::path path, StreamMode mode)
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

    uint64_t FileStream::getLength() const noexcept
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

    uint64_t FileStream::getPosition() const noexcept
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

    void FileStream::setPosition(uint64_t position)
    {
        if (_reading)
            _fstream.seekg(position);
        if (_writing)
            _fstream.seekp(position);
    }

    void FileStream::read(void* buffer, size_t len)
    {
        _fstream.read(static_cast<char*>(buffer), len);
        if (_fstream.fail())
            throw std::runtime_error("Failed to read data");
    }

    void FileStream::write(const void* buffer, size_t len)
    {
        if (len != 0)
        {
            _fstream.write(static_cast<const char*>(buffer), len);
            if (_fstream.fail())
                throw std::runtime_error("Failed to write data");
        }
    }
}
