#pragma once

#include <cstdint>

namespace openloco::stringmgr
{
    class argswrapper
    {
    private:
        void* args;

    public:
        argswrapper(void* newargs)
            : args(newargs){};

        uint8_t pop8()
        {
            if (args == nullptr)
                return 0;

            uint8_t value = *(uint8_t*)args;
            args = (uint8_t*)args + 1;
            return value;
        }

        uint16_t pop16()
        {
            if (args == nullptr)
                return 0;

            uint16_t value = *(uint16_t*)args;
            args = (uint16_t*)args + 1;
            return value;
        }

        int16_t popS16()
        {
            if (args == nullptr)
                return 0;

            uint16_t value = *(int16_t*)args;
            args = (int16_t*)args + 1;
            return value;
        }

        uint32_t pop32()
        {
            if (args == nullptr)
                return 0;

            uint32_t value = *(uint32_t*)args;
            args = (uint32_t*)args + 1;
            return value;
        }

        int32_t popS32()
        {
            if (args == nullptr)
                return 0;

            int32_t value = *(int32_t*)args;
            args = (int32_t*)args + 1;
            return value;
        }

        void skip16()
        {
            if (args != nullptr)
                args = (uint16_t*)args + 1;
        }

        void push16()
        {
            args = (uint16_t*)args - 1;
        }
    };
}
