#include "FileStream.h"
#include <algorithm>
#include <stdexcept>

namespace OpenLoco
{
    static size_t getFileLength(FILE* fs)
    {
        auto pos = ftell(fs);
        fseek(fs, 0, SEEK_END);
        auto len = ftell(fs);
        fseek(fs, pos, SEEK_SET);
        return len;
    }

    static size_t readFile(void* buffer, size_t len, FILE* fs)
    {
#ifdef _MSC_VER
        return _fread_nolock(buffer, 1, len, fs);
#else
        return std::fread(buffer, 1, len, fs);
#endif
    }

    static size_t writeFile(const void* buffer, size_t len, FILE* fs)
    {
#ifdef _MSC_VER
        return _fwrite_nolock(buffer, 1, len, fs);
#else
        return std::fwrite(buffer, 1, len, fs);
#endif
    }

    FileStream::FileStream(const std::filesystem::path& path, StreamMode mode)
    {
        if (!open(path, mode))
        {
            // TODO: Make this work like fstream which is not throwing for failing to open the file.
            throw std::runtime_error("Failed to open '" + path.u8string() + "' for writing");
        }
    }

    FileStream::~FileStream()
    {
        close();
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
#ifdef _WIN32
            _wfopen_s(&_file, path.wstring().c_str(), L"wb");
#else
            _file = fopen(path.u8string().c_str(), "wb");
#endif
        }
        if (mode == StreamMode::read)
        {
#ifdef _WIN32
            _wfopen_s(&_file, path.wstring().c_str(), L"rb");
#else
            _file = fopen(path.u8string().c_str(), "rb");
#endif
        }

        if (_file == nullptr)
        {
            return false;
        }

        // Get the length if we are reading an existing file.
        if (mode == StreamMode::read)
        {
            _length = getFileLength(_file);
        }

        _offset = 0;
        _mode = mode;
        return true;
    }

    bool FileStream::isOpen() const noexcept
    {
        return _file != nullptr;
    }

    void FileStream::close()
    {
        if (_file == nullptr)
        {
            return;
        }
        _mode = StreamMode::none;
        _length = 0;
        _offset = 0;

        fclose(_file);
        _file = nullptr;
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
        std::fseek(_file, position, SEEK_SET);
        _offset = position;
    }

    void FileStream::read(void* buffer, size_t len)
    {
        if (_mode != StreamMode::read)
        {
            throw std::runtime_error("Invalid operation");
        }

        const auto bytesRead = readFile(buffer, len, _file);
        if (bytesRead != len)
        {
            throw std::runtime_error("Failed to read data");
        }

        _offset += bytesRead;
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

        const auto bytesWriten = writeFile(buffer, len, _file);
        if (bytesWriten != len)
        {
            throw std::runtime_error("Failed to write data");
        }

        _offset += bytesWriten;
        _length = std::max(_length, _offset);
    }
}
