#pragma once

#include "Audio/Audio.h"
#include "Object.h"
#include "Types.hpp"
#include <span>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
    namespace SoundObjectId
    {
        constexpr SoundObjectId_t null = 0xFF;
    }
#pragma pack(push, 1)

    struct SoundObjectData
    {
        int32_t var_00;
        int32_t offset;
        uint32_t length;
        Audio::WAVEFORMATEX pcmHeader;

        const void* pcm() const
        {
            return (void*)((uintptr_t)this + sizeof(SoundObjectData));
        }
    };
    static_assert(sizeof(SoundObjectData) == 0x1E);

    struct SoundObject
    {
        static constexpr auto kObjectType = ObjectType::sound;

        StringId name;
        const SoundObjectData* data;
        uint8_t var_06;
        uint8_t pad_07;
        uint32_t volume; // 0x08

        // 0x0048AFEE
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> objData, ObjectManager::DependentObjects*);
        void unload();
    };
#pragma pack(pop)
    static_assert(sizeof(SoundObject) == 0xC);
}
