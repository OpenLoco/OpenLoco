#pragma once

#include "../Core/Span.hpp"
#include "../Types.hpp"
#include "Object.h"

namespace OpenLoco
{
    namespace SoundObjectId
    {
        constexpr SoundObjectId_t null = 0xFF;
    }
#pragma pack(push, 1)
    struct SoundObject
    {
        static constexpr auto kObjectType = ObjectType::sound;

        string_id name;
        void* data;
        uint8_t var_06;
        uint8_t pad_07;
        uint32_t volume; // 0x08

        // 0x0048AFEE
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, stdx::span<std::byte> objData);
        void unload();
    };
#pragma pack(pop)
    static_assert(sizeof(SoundObject) == 0xC);
}
