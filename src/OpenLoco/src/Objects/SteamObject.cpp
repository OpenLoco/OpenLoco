#include "SteamObject.h"
#include "Interop/Interop.hpp"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <cassert>

namespace OpenLoco
{
    // 0x00440CAD
    void SteamObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(SteamObject));

        auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
        name = strRes.str;
        remainingData = remainingData.subspan(strRes.tableLength);

        frameInfoType0 = reinterpret_cast<const ImageAndHeight*>(remainingData.data());
        while (*remainingData.data() != static_cast<std::byte>(0xFF))
        {
            totalNumFramesType0++;
            remainingData = remainingData.subspan(sizeof(ImageAndHeight));
        }
        remainingData = remainingData.subspan(1);

        frameInfoType1 = reinterpret_cast<const ImageAndHeight*>(remainingData.data());
        while (*remainingData.data() != static_cast<std::byte>(0xFF))
        {
            totalNumFramesType1++;
            remainingData = remainingData.subspan(sizeof(ImageAndHeight));
        }
        remainingData = remainingData.subspan(1);

        for (uint8_t i = 0; i < numSoundEffects; ++i)
        {
            auto& soundId = soundEffects[i];

            ObjectHeader soundHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
            if (dependencies != nullptr)
            {
                dependencies->required.push_back(soundHeader);
            }
            auto res = ObjectManager::findObjectHandle(soundHeader);
            if (res.has_value())
            {
                soundId = res->id;
            }

            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        baseImageId = imgRes.imageOffset;

        // auto imageExtents = getMaxImageExtent(baseImageId, numImages);
        // var_05 = imageExtents.width
        // var_06 = imageExtents.heightNegative
        // var_07 = imageExtents.heightPositive

        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x00440D8B
    void SteamObject::unload()
    {
        name = 0;
        baseImageId = 0;
        totalNumFramesType0 = 0;
        totalNumFramesType1 = 0;
        frameInfoType0 = nullptr;
        frameInfoType1 = nullptr;
        var_05 = 0;
        var_06 = 0;
        var_07 = 0;

        // Unsure of var_1F size 9th position might be a terminator
        std::fill_n(std::begin(soundEffects), 8, 0);
    }
}
