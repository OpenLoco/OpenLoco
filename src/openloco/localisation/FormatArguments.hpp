#pragma once

#include "../interop/interop.hpp"

using namespace openloco::interop;

namespace openloco
{

    class FormatArguments
    {
    private:
        void* _buffer;
        void* _bufferStart;

    public:
        FormatArguments(void* buffer)
        {
            _bufferStart = buffer;
            _buffer = _bufferStart;
        }

        FormatArguments()
            : FormatArguments(&addr<0x112C826, void*>())
        {
        }

        template<typename T>
        void push(T arg)
        {
            *(T*)_buffer = arg;
            _buffer = (void*)((uint8_t*)_buffer + sizeof(T));
        }

        const void* operator&()
        {
            return _bufferStart;
        }
    };

}
