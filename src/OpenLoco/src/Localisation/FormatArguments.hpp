#pragma once

#include "../Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco
{

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

        FormatArguments()
        {
            loco_global<std::byte[0x0112C83A - 0x0112C826], 0x0112C826> _commonFormatArgs;

            _bufferStart = _buffer = &*_commonFormatArgs;
            _length = std::size(_commonFormatArgs);
        }

        template<typename... T>
        static FormatArguments common(T&&... args)
        {
            loco_global<std::byte[0x0112C83A - 0x0112C826], 0x0112C826> _commonFormatArgs;
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

        const void* operator&()
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
                throw std::out_of_range("FormatArguments: attempting to advance outside of buffer");
            return nextOffset;
        }
    };
}
