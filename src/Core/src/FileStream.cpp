#include "FileStream.h"
#include <algorithm>
#include <stdexcept>

namespace OpenLoco
{
    static size_t getFileLength(std::fstream& fs)
    {
        auto backup = fs.tellg();
        fs.seekg(0, std::ios_base::end);
        auto len = fs.tellg();
        fs.seekg(backup);
        return len;
    }

    FileStream::FileStream(const std::filesystem::path path, StreamMode mode)
    {
        open(path, mode);
    }

    bool FileStream::open(const std::filesystem::path path, StreamMode mode)
    {
        if (mode == StreamMode::none)
        {
            throw std::invalid_argument("Invalid mode argument");
        }

        close();

        if (mode == StreamMode::write)
        {
            _fstream.open(path, std::ios::out | std::ios::binary);
        }
        if (mode == StreamMode::read)
        {
            _fstream.open(path, std::ios::in | std::ios::binary);
        }

        if (!_fstream.is_open())
        {
            return false;
        }

        // Get the length if we are reading an existing file.
        if (mode == StreamMode::read)
        {
            _length = getFileLength(_fstream);
        }

        _mode = mode;
        return true;
    }

    bool FileStream::isOpen() const noexcept
    {
        return _fstream.is_open();
    }

    void FileStream::close()
    {
        _mode = StreamMode::none;
        _length = 0;
        _fstream.close();
    }

    StreamMode FileStream::getMode() const noexcept
    {
        return _mode;
    }

    uint64_t FileStream::getLength() const noexcept
    {
        return static_cast<uint64_t>(_length);
    }

    uint64_t FileStream::getPosition() const noexcept
    {
        auto& fs = const_cast<std::fstream&>(_fstream);
        if (_mode == StreamMode::write)
        {
            return static_cast<uint64_t>(fs.tellp());
        }
        if (_mode == StreamMode::read)
        {
            return static_cast<uint64_t>(fs.tellg());
        }
        // File not open.
        return 0;
    }

    void FileStream::setPosition(uint64_t position)
    {
        if (_mode == StreamMode::none)
        {
            throw std::runtime_error("Invalid operation");
        }
        if (_mode == StreamMode::read)
            _fstream.seekg(position);
        if (_mode == StreamMode::write)
            _fstream.seekp(position);
    }

    void FileStream::read(void* buffer, size_t len)
    {
        if (_mode != StreamMode::read)
        {
            throw std::runtime_error("Invalid operation");
        }
        _fstream.read(static_cast<char*>(buffer), len);
        if (_fstream.fail())
            throw std::runtime_error("Failed to read data");
    }

    void FileStream::write(const void* buffer, size_t len)
    {
        if (_mode != StreamMode::write)
        {
            throw std::runtime_error("Invalid operation");
        }
        if (len == 0)
        {
            return;
        }

        _fstream.write(static_cast<const char*>(buffer), len);
        _length = std::max(_length, static_cast<size_t>(_fstream.tellp()));

        if (_fstream.fail())
            throw std::runtime_error("Failed to write data");
    }
}
