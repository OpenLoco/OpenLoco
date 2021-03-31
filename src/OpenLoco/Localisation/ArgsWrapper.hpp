#pragma once

#include <cstddef>
#include <cstring>

namespace OpenLoco::StringManager
{
    class ArgsWrapper
    {
    private:
        const std::byte* args;

    public:
        ArgsWrapper(const void* newargs)
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
