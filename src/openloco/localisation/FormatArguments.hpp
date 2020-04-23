#pragma once

#include "../compat.h"
#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco
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

        template<typename T>
        void push(T arg)
        {
            std::byte* nextOffset = (std::byte*)((std::byte*)_buffer + sizeof(T));
            if (nextOffset > _bufferStart + _length)
                throw std::out_of_range("FormatArguments::push: attempting to push outside of buffer");

            *(T*)_buffer = arg;
            _buffer = nextOffset;
        }

        const void* operator&()
        {
            return _bufferStart;
        }

        size_t getLength() const
        {
            return _buffer - _bufferStart;
        }
    };

}
