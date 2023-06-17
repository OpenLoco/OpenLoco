#include "TreeObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Core/Numerics.hpp>
#include <OpenLoco/Interop/Interop.hpp>

namespace OpenLoco
{
    // 0x004BE2A2
    void TreeObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        uint32_t image = getTreeGrowthDisplayOffset() * numRotations;
        auto rotation = (numRotations - 1) & 2;
        image += rotation;
        image += sprites[seasonState];

        auto colourOptions = colours;
        if (colourOptions != 0)
        {

            auto bit = Numerics::bitScanReverse(colourOptions);

            auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

            image = Gfx::recolour(image, colour);
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        Ui::Point treePos = Ui::Point{ x, y } + Ui::Point{ 0, 48 };

        if (hasFlags(TreeObjectFlags::hasSnowVariation))
        {
            auto snowImage = getTreeGrowthDisplayOffset() * numRotations;
            snowImage += rotation;
            snowImage += snowSprites[seasonState];

            if (colourOptions != 0)
            {
                auto bit = Numerics::bitScanReverse(colourOptions);

                auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

                snowImage = Gfx::recolour(snowImage, colour);
            }
            treePos.x = x + 28;
            drawingCtx.drawImage(&rt, treePos.x, treePos.y, snowImage);
            treePos.x = 28;
        }
        drawingCtx.drawImage(&rt, treePos.x, treePos.y, image);
    }

    // 0x00500775
    constexpr std::array<uint8_t, 11> treeGrowth = { {
        1,
        0,
        1,
        2,
        2,
        3,
        4,
        5,
        6,
        0,
        0,
    } };

    uint8_t TreeObject::getTreeGrowthDisplayOffset() const
    {
        return treeGrowth[growth];
    }

    // 0x004BE24D
    bool TreeObject::validate() const
    {
        if (costIndex > 32)
        {
            return false;
        }

        // 230/256 = ~90%
        if (-clearCostFactor > buildCostFactor * 230 / 256)
        {
            return false;
        }

        switch (numRotations)
        {
            default:
                return false;
            case 1:
            case 2:
            case 4:
                break;
        }
        if (growth < 1 || growth > 8)
        {
            return false;
        }

        if (height < var_02)
        {
            return false;
        }

        return var_05 >= var_04;
    }

    // 0x004BE144
    void TreeObject::load(const LoadedObjectHandle& handle, [[maybe_unused]] stdx::span<const std::byte> data, ObjectManager::DependentObjects*)
    {
        auto remainingData = data.subspan(sizeof(TreeObject));

        // Load localised name string
        {
            auto strRes = ObjectManager::loadStringTable(remainingData, handle, 0);
            name = strRes.str;
            remainingData = remainingData.subspan(strRes.tableLength);
        }

        // Load image offsets
        auto imgRes = ObjectManager::loadImageTable(remainingData);
        auto imageOffset = imgRes.imageOffset;
        assert(remainingData.size() == imgRes.tableLength);

        // Initialise sprites array
        for (auto variant = 0; variant < 6; variant++)
        {
            sprites[variant] = imageOffset;
        }

        auto numVariantImages = numRotations * growth;
        auto totalImageCount = 0;
        for (auto variant = 0; variant < 6; variant++)
        {
            if ((var_3C & (1 << variant)) == 0)
                continue;

            sprites[variant] = imgRes.imageOffset + totalImageCount;
            totalImageCount += numVariantImages;
        }

        // 0x004BE186
        const auto numPrimaryImages = totalImageCount;

        if ((var_3C & (1 << 5)) == 0 && (var_3C & (1 << 4)) != 0)
        {
            sprites[5] = sprites[4];
        }

        if ((var_3C & (1 << 5)) == 0 && (var_3C & (1 << 1)) != 0)
        {
            sprites[5] = sprites[1];
        }

        if ((var_3C & (1 << 4)) == 0 && (var_3C & (1 << 1)) != 0)
        {
            sprites[4] = sprites[1];
        }

        if ((var_3C & (1 << 1)) == 0 && (var_3C & (1 << 0)) != 0)
        {
            sprites[1] = sprites[0];
        }

        if ((var_3C & (1 << 0)) == 0 && (var_3C & (1 << 1)) != 0)
        {
            sprites[0] = sprites[1];
        }

        if ((flags & TreeObjectFlags::hasSnowVariation) != TreeObjectFlags::none)
        {
            for (auto variant = 0; variant < 6; variant++)
            {
                snowSprites[variant] = sprites[variant] + numPrimaryImages;
            }

            // Snow doubles the number of images
            totalImageCount += numPrimaryImages;
        }

        if ((flags & TreeObjectFlags::hasShadow) != TreeObjectFlags::none)
        {
            shadowImageOffset = totalImageCount;

            // Shadows double the number of images (combines to a x4 with snow)
            const auto numShadowImages = totalImageCount;

            // Calculate the total just for checking purposes
            totalImageCount += numShadowImages;
        }

        // Verify we haven't overshot any lengths
        assert(imgRes.imageOffset + totalImageCount == ObjectManager::getTotalNumImages());
    }

    // 0x004BE231
    void TreeObject::unload()
    {
        name = 0;
        std::fill(std::begin(sprites), std::end(sprites), 0);
        std::fill(std::begin(snowSprites), std::end(snowSprites), 0);
        shadowImageOffset = 0;
    }
}
