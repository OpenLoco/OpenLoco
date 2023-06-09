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

    FileStream::FileStream(const std::filesystem::path& path, StreamMode mode)
    {
        if (!open(path, mode))
        {
            // TODO: Make this work like fstream which is not throwing for failing to open the file.
            throw std::runtime_error("Failed to open '" + path.u8string() + "' for writing");
        }
    }

    bool FileStream::open(const std::filesystem::path& path, StreamMode mode)
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

        _offset = 0;
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
        _offset = 0;
        _fstream.close();
    }

    StreamMode FileStream::getMode() const noexcept
    {
        return _mode;
    }

    size_t FileStream::getLength() const noexcept
    {
        return _length;
    }

    size_t FileStream::getPosition() const noexcept
    {
        return _offset;
    }

    void FileStream::setPosition(size_t position)
    {
        if (_mode == StreamMode::none)
        {
            throw std::runtime_error("Invalid operation");
        }
        position = std::min(_length, static_cast<size_t>(position));
        if (_mode == StreamMode::read)
            _fstream.seekg(position);
        if (_mode == StreamMode::write)
            _fstream.seekp(position);
        _offset = position;
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

        _offset += len;
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
        if (_fstream.fail())
            throw std::runtime_error("Failed to write data");

        _offset += len;
        _length = std::max(_length, _offset);
    }
}
