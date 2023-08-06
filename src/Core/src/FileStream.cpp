#include "FileStream.h"
#include <algorithm>
#include <stdexcept>
#include <stdio.h>

namespace OpenLoco
{
    static FILE* fileOpen(const std::filesystem::path& path, StreamMode mode)
    {
        if (mode == StreamMode::none)
        {
            throw std::invalid_argument("Invalid mode argument");
        }
#ifdef _WIN32
        FILE* fs;
        _wfopen_s(&fs, path.wstring().c_str(), mode == StreamMode::read ? L"rb" : L"wb");
        return fs;
#else
        return fopen(path.u8string().c_str(), mode == StreamMode::read ? "rb" : "wb");
#endif
    }

    static size_t fileTell(FILE* fs)
    {
#ifdef _MSC_VER
        return static_cast<size_t>(_ftelli64_nolock(fs));
#else
        return ftello(fs);
#endif
    }

    static void fileSeek(FILE* fs, size_t offset, int origin)
    {
#ifdef _MSC_VER
        _fseeki64_nolock(fs, offset, origin);
#else
        fseeko(fs, offset, origin);
#endif
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

    static void fileClose(FILE* fs)
    {
#ifdef _MSC_VER
        _fclose_nolock(fs);
#else
        fclose(fs);
#endif
    }

    static size_t getFileLength(FILE* fs)
    {
        const auto current = fileTell(fs);
        fileSeek(fs, 0, SEEK_END);
        const auto length = fileTell(fs);
        fileSeek(fs, current, SEEK_SET);
        return length;
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
        close();

        _file = fileOpen(path, mode);
        if (_file == nullptr)
        {
            return false;
        }

        // Increase the buffer size to 1MiB.
        std::setvbuf(_file, nullptr, _IOFBF, 1024 * 1024);

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

        fileClose(_file);
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
        fileSeek(_file, position, SEEK_SET);
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
