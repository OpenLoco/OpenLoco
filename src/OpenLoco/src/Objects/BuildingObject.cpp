#include "BuildingObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/Numeric.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco
{
    // 0x0042DE40
    void BuildingObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto bit = Utility::bitScanReverse(colours);

        const auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

        drawBuilding(&rt, 1, x, y + 40, colour);
    }

    // 0x0042DB95
    void BuildingObject::drawBuilding(Gfx::RenderTarget* clipped, uint8_t buildingRotation, int16_t x, int16_t y, Colour colour) const
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = enumValue(colour);
        regs.eax = buildingRotation;
        regs.edi = X86Pointer(clipped);
        regs.ebp = X86Pointer(this);
        call(0x0042DB95, regs);
    }

    // 0x0042DE82
    void BuildingObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, obsoleteYear);
    }

    // 0x0042DE1E
    bool BuildingObject::validate() const
    {
        if ((var_06 == 0) || (var_06 > 63))
        {
            return false;
        }
        return (numVariations != 0) && (numVariations <= 31);
    }

    // 0x0042DBE8
    void BuildingObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects* dependencies)
    {
        auto remainingData = data.subspan(sizeof(BuildingObject));

        {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
            name = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        }

        // LOAD BUILDING PARTS Start
        // Load Part Heights
        varationHeights = reinterpret_cast<const uint8_t*>(remainingData.data());
        remainingData = remainingData.subspan(var_06 * sizeof(uint8_t));

        // Load Part Animations (probably)
        var_0C = reinterpret_cast<const uint16_t*>(remainingData.data());
        remainingData = remainingData.subspan(var_06 * sizeof(uint16_t));

        // Load Parts
        for (auto i = 0; i < numVariations; ++i)
        {
            auto& part = variationsArr10[i];
            part = reinterpret_cast<const uint8_t*>(remainingData.data());
            while (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                remainingData = remainingData.subspan(1);
            }
            remainingData = remainingData.subspan(1);
        }
        // LOAD BUILDING PARTS End

        // Load Produced Cargo
        for (auto& cargo : producedCargoType)
        {
            cargo = 0xFF;
            if (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                ObjectHeader cargoHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());
                if (dependencies != nullptr)
                {
                    dependencies->required.push_back(cargoHeader);
                }
                auto res = ObjectManager::findObjectHandle(cargoHeader);
                if (res.has_value())
                {
                    cargo = res->id;
                }
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load ??? Cargo
        for (auto& unkObj : var_A4)
        {
            unkObj = 0xFF; // This is a cargo
            if (*remainingData.data() != static_cast<std::byte>(0xFF))
            {
                ObjectHeader cargoHeader = *reinterpret_cast<const ObjectHeader*>(remainingData.data());

                if (dependencies != nullptr)
                {
                    dependencies->required.push_back(cargoHeader);
                }
                auto res = ObjectManager::findObjectHandle(cargoHeader);
                if (res.has_value())
                {
                    unkObj = res->id;
                }
            }
            remainingData = remainingData.subspan(sizeof(ObjectHeader));
        }

        // Load ???
        for (auto i = 0; i < var_AD; ++i)
        {
            var_AE[i] = reinterpret_cast<const uint8_t*>(remainingData.data());
            const auto size = *reinterpret_cast<const uint16_t*>(var_AE[i]);
            remainingData = remainingData.subspan(sizeof(uint16_t) + size);
        }

        // Load Image Offsets
        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);
    }

    // 0x0042DDC4
    void BuildingObject::unload()
    {
        name = 0;
        image = 0;
        varationHeights = nullptr;
        var_0C = 0;
        std::fill(std::begin(variationsArr10), std::end(variationsArr10), nullptr);
        std::fill(std::begin(producedCargoType), std::end(producedCargoType), 0);
        std::fill(std::begin(var_A4), std::end(var_A4), 0);
        std::fill(std::begin(var_AE), std::end(var_AE), nullptr);
    }
}
