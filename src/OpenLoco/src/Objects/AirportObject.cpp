#include "AirportObject.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"

namespace OpenLoco
{
    // 0x00490DCF
    void AirportObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        auto colourImage = Gfx::recolour(image, Colour::mutedDarkRed);

        Gfx::drawImage(&rt, x - 34, y - 34, colourImage);
    }

    // 0x00490DE7
    void AirportObject::drawDescription(Gfx::RenderTarget& rt, const int16_t x, const int16_t y, [[maybe_unused]] const int16_t width) const
    {
        Ui::Point rowPosition = { x, y };
        ObjectManager::drawGenericDescription(rt, rowPosition, designedYear, obsoleteYear);
    }

    // 0x00490DA8
    bool AirportObject::validate() const
    {
        if (costIndex > 32)
        {
            return false;
        }

        if (-sellCostFactor > buildCostFactor)
        {
            return false;
        }

        return buildCostFactor > 0;
    }

    // 0x00490CAF
    void AirportObject::load(const LoadedObjectHandle& handle, stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(AirportObject));

        {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
            name = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        }

        var_14 = reinterpret_cast<const uint8_t*>(remainingData.data());
        remainingData = remainingData.subspan(numSpriteSets * sizeof(uint8_t));
        var_18 = reinterpret_cast<const uint16_t*>(remainingData.data());
        remainingData = remainingData.subspan(numSpriteSets * sizeof(uint16_t));

        for (auto i = 0; i < numTiles; ++i)
        {
            var_1C[i] = reinterpret_cast<const uint8_t*>(remainingData.data());
            auto* ptr = var_1C[i];
            while (*ptr++ != 0xFF)
                ;
            remainingData = remainingData.subspan(ptr - var_1C[i]);
        }

        var_9C = reinterpret_cast<const uint32_t*>(remainingData.data());
        auto* ptr = reinterpret_cast<const uint8_t*>(remainingData.data());
        while (*ptr != 0xFF)
            ptr += 4;
        ptr++;
        remainingData = remainingData.subspan(ptr - reinterpret_cast<const uint8_t*>(var_9C));

        movementNodes = reinterpret_cast<const MovementNode*>(remainingData.data());
        remainingData = remainingData.subspan(numMovementNodes * sizeof(MovementNode));
        movementEdges = reinterpret_cast<const MovementEdge*>(remainingData.data());
        remainingData = remainingData.subspan(numMovementEdges * sizeof(MovementEdge));

        auto imgRes = ObjectManager::loadImageTable(remainingData);
        image = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);

        auto nextImage = image + 1;
        if (allowedPlaneTypes & (1 << 0))
        {
            nextImage += numTiles * 4;
        }
        var_0C = nextImage;
    }

    // 0x00490D66
    void AirportObject::unload()
    {
        name = 0;
        image = 0;
        var_0C = 0;

        std::fill(std::begin(var_1C), std::end(var_1C), nullptr);

        var_9C = nullptr;

        movementNodes = nullptr;
        movementEdges = nullptr;
    }
}
