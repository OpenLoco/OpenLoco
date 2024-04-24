#pragma once

#include <OpenLoco/Interop/Interop.hpp>
#include <array>
#include <sfl/small_vector.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    class FormatArgumentsBuffer
    {
        std::array<std::byte, 16> _buffer{};

    public:
        std::byte* data()
        {
            return _buffer.data();
        }

        const std::byte* data() const
        {
            return _buffer.data();
        }

        size_t capacity() const
        {
            return _buffer.size();
        }
    };

    class FormatArguments
    {
    private:
        std::byte* _buffer;
        std::byte* _bufferStart;
        size_t _length;

    public:
        FormatArguments(std::byte* buffer, size_t length)
        {
            _bufferStart = buffer;
            _buffer = _bufferStart;
            _length = length;
        }

        FormatArguments(FormatArgumentsBuffer& buffer)
        {
            _buffer = buffer.data();
            _bufferStart = _buffer;
            _length = buffer.capacity();
        }

        FormatArguments(const FormatArgumentsBuffer& buffer)
        {
            // FIXME: Create a view type for FormatArgumentsBuffer.
            _buffer = const_cast<std::byte*>(buffer.data());
            _bufferStart = _buffer;
            _length = buffer.capacity();
        }

        FormatArguments()
        {
            loco_global<std::byte[14], 0x0112C826> _commonFormatArgs;

            _bufferStart = _buffer = &*_commonFormatArgs;
            _length = std::size(_commonFormatArgs);
        }

        template<typename... T>
        static FormatArguments common(T&&... args)
        {
            loco_global<std::byte[14], 0x0112C826> _commonFormatArgs;
            FormatArguments formatter{ _commonFormatArgs.get(), std::size(_commonFormatArgs) };
            (formatter.push(args), ...);
            return formatter;
        }

        template<typename... T>
        static FormatArguments mapToolTip(T&&... args)
        {
            loco_global<std::byte[40], 0x0050A018> _mapTooltipFormatArguments;
            FormatArguments formatter{ _mapTooltipFormatArguments.get(), std::size(_mapTooltipFormatArguments) };
            (formatter.push(args), ...);
            return formatter;
        }

        // Size in bytes to skip forward the buffer
        void skip(const size_t size)
        {
            auto* const nextOffset = getNextOffset(size);

            _buffer = nextOffset;
        }

        template<typename T>
        void push(T arg)
        {
            static_assert(sizeof(T) % 2 == 0, "Tried to push an odd number of bytes onto the format args!");
            auto* nextOffset = getNextOffset(sizeof(T));

            *(T*)_buffer = arg;
            _buffer = nextOffset;
        }

        void rewind()
        {
            _buffer = _bufferStart;
        }

        const void* operator&() const
        {
            return _bufferStart;
        }

        const std::byte* getBufferStart() const
        {
            return _bufferStart;
        }

        size_t getLength() const
        {
            return _buffer - _bufferStart;
        }

    private:
        std::byte* getNextOffset(const size_t size) const
        {
            std::byte* const nextOffset = reinterpret_cast<std::byte*>(reinterpret_cast<std::byte*>(_buffer) + size);
            if (nextOffset > _bufferStart + _length)
                throw Exception::OutOfRange("FormatArguments: attempting to advance outside of buffer");
            return nextOffset;
        }
    };

    class FormatArgumentsView
    {
    private:
        const std::byte* args;

    public:
        FormatArgumentsView(const void* newargs)
            : args(reinterpret_cast<const std::byte*>(newargs)){};

        template<typename T>
        T pop()
        {
            if (args == nullptr)
                return T{};

            T value;
            std::memcpy(&value, args, sizeof(T));
            args += sizeof(T);

            return value;
        }

        template<typename T>
        void skip()
        {
            if (args == nullptr)
                return;
            args += sizeof(T);
        }

        template<typename T>
        void push()
        {
            args -= sizeof(T);
        }
    };
}
