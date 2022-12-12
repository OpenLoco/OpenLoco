#include "TrainStationObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectManager.h"
#include <cassert>

namespace OpenLoco
{
    // 0x00490A26
    void TrainStationObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        Gfx::drawImage(&rt, x - 34, y - 34, colourImage);

        auto colour = ExtColour::translucentMutedDarkRed1;
        if (!(flags & TrainStationFlags::recolourable))
        {
            colour = ExtColour::unk2E;
        }

        auto translucentImage = Gfx::recolourTranslucent(image + 1, colour);

        Gfx::drawImage(&rt, x - 34, y - 34, translucentImage);
    }

    // 0x00490A68
    void TrainStationObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, obsoleteYear);
    }

    // 0x004909F3
    bool TrainStationObject::validate() const
    {
        if (costIndex >= 32)
        {
            return false;
        }
        if (-sellCostFactor > buildCostFactor)
        {
            return false;
        }
        if (buildCostFactor <= 0)
        {
            return false;
        }
        if (drawStyle >= 1)
        {
            return false;
        }
        return numCompatible <= 7;
    }

    // 0x004908F1
    void TrainStationObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004908F1, regs);
    }

    // 0x004909A5
    void TrainStationObject::unload()
    {
        name = 0;
        image = 0;
        std::fill(std::begin(var_12), std::end(var_12), 0);
        std::fill(std::begin(mods), std::end(mods), 0);
        std::fill(&cargoOffsetBytes[0][0], &cargoOffsetBytes[0][0] + sizeof(cargoOffsetBytes) / sizeof(std::byte*), nullptr);
        std::fill(std::begin(var_6E), std::end(var_6E), 0);
    }

    std::vector<TrainStationObject::CargoOffset> TrainStationObject::getCargoOffsets(const uint8_t rotation, const uint8_t nibble) const
    {
        assert(rotation < 4 && nibble < 4);

        const auto* bytes = cargoOffsetBytes[rotation][nibble];
        uint8_t z = *reinterpret_cast<const uint8_t*>(bytes);
        bytes++;
        std::vector<CargoOffset> result;
        while (*bytes != static_cast<std::byte>(0xFF))
        {
            result.push_back({
                Map::Pos3{
                    *reinterpret_cast<const int8_t*>(bytes),
                    *reinterpret_cast<const int8_t*>(bytes + 1),
                    z,
                },
                Map::Pos3{
                    *reinterpret_cast<const int8_t*>(bytes + 2),
                    *reinterpret_cast<const int8_t*>(bytes + 3),
                    z,
                },
            });
            bytes += 4;
        }
        return result;
    }
}
