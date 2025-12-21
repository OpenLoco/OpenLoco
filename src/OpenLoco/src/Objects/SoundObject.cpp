#include "SoundObject.h"
#include "ObjectStringTable.h"

#include <cassert>

namespace OpenLoco
{
    // 0x0048AFAF
    void SoundObject::load(const LoadedObjectHandle& handle, std::span<const std::byte> objData, ObjectManager::DependentObjects*)
    {
        auto remainingData = objData.subspan(sizeof(SoundObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        // Unknown structure size 0x10
        uint32_t numUnkStructs = *reinterpret_cast<const uint32_t*>(remainingData.data());
        remainingData = remainingData.subspan(sizeof(uint32_t));
        [[maybe_unused]] uint32_t pcmDataLength = *reinterpret_cast<const uint32_t*>(remainingData.data());
        remainingData = remainingData.subspan(sizeof(uint32_t));
        remainingData = remainingData.subspan(numUnkStructs * 16);

        // Main pcm data structure
        dataOffset = static_cast<uint32_t>(remainingData.data() - objData.data());

        assert(remainingData.size() == pcmDataLength);
    }

    // 0x0048AFE1
    void SoundObject::unload()
    {
        name = 0;
        dataOffset = 0;
    }

    const OpenLoco::SoundObjectData* SoundObject::getData() const
    {
        if (dataOffset == 0)
        {
            return nullptr;
        }
        return reinterpret_cast<const OpenLoco::SoundObjectData*>(reinterpret_cast<const uint8_t*>(this) + dataOffset);
    }

}
