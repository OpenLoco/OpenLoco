#pragma once

#include "Object.h"
#include "Types.hpp"
#include <span>

namespace OpenLoco
{
    namespace ObjectManager
    {
        struct DependentObjects;
    }
    namespace Gfx
    {
        class DrawingContext;
    }

    // Towns growth can be influenced by the region objects cargo influence
    // the cargo influence can be further filtered so it only effects
    // certain types of town
    enum class CargoInfluenceTownFilterType : uint8_t
    {
        allTowns,
        maySnow,  // Towns above (or equal to) the summer snow line
        inDesert, // Towns located in a desert (>=100 desert surface tiles)
    };

#pragma pack(push, 1)
    struct RegionObject
    {
        static constexpr auto kObjectType = ObjectType::region;

        StringId name;
        uint32_t image; // 0x02
        uint8_t pad_06[0x8 - 0x6];
        uint8_t numCargoInflunceObjects;                          // 0x08 length of cargoInfluenceObjectIds and cargoInfluenceTownFilter
        uint8_t cargoInfluenceObjectIds[4];                       // 0x09
        CargoInfluenceTownFilterType cargoInfluenceTownFilter[4]; // 0x0D valid values 0, 1, 2
        uint8_t pad_11;

        void drawPreviewImage(Gfx::DrawingContext& drawingCtx, const int16_t x, const int16_t y) const;
        // 0x0043CB89
        bool validate() const { return true; }
        void load(const LoadedObjectHandle& handle, std::span<const std::byte> data, ObjectManager::DependentObjects* dependencies);
        void unload();
    };
#pragma pack(pop)

    static_assert(sizeof(RegionObject) == 0x12);
}
