#include "TreeObject.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "ObjectImageTable.h"
#include "ObjectManager.h"
#include "ObjectStringTable.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <OpenLoco/Utility/Numeric.hpp>

namespace OpenLoco
{
    // 0x004BE2A2
    void TreeObject::drawPreviewImage(Gfx::RenderTarget& rt, const int16_t x, const int16_t y) const
    {
        uint32_t image = getTreeGrowthDisplayOffset() * numRotations;
        auto rotation = (numRotations - 1) & 2;
        image += rotation;
        image += sprites[0][seasonState];

        auto colourOptions = colours;
        if (colourOptions != 0)
        {

            auto bit = Utility::bitScanReverse(colourOptions);

            auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

            image = Gfx::recolour(image, colour);
        }

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        Ui::Point treePos = Ui::Point{ x, y } + Ui::Point{ 0, 48 };

        if (hasFlags(TreeObjectFlags::hasSnowVariation))
        {
            auto snowImage = getTreeGrowthDisplayOffset() * numRotations;
            snowImage += rotation;
            snowImage += sprites[1][seasonState];

            if (colourOptions != 0)
            {
                auto bit = Utility::bitScanReverse(colourOptions);

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
            sprites[0][variant] = imageOffset;
        }

        auto numVariantImages = numRotations * growth;
        auto nextImageOffset = imageOffset;
        for (auto variant = 0; variant < 6; variant++)
        {
            if ((var_3C & (1 << variant)) == 0)
                continue;

            sprites[0][variant] = nextImageOffset;
            nextImageOffset += numVariantImages;
        }

        // 0x004BE186
        auto numPrimaryImages = nextImageOffset - imageOffset;
        nextImageOffset = imageOffset;

        /*
            TODO: here for quick verification; remove later
            0x0A -> sprites[0][0]
            0x0E -> sprites[0][1]
            0x12 -> sprites[0][2]
            0x16 -> sprites[0][3]
            0x1A -> sprites[0][4]
            0x1E -> sprites[0][5]
            0x22 -> sprites[1][0]
            0x26 -> sprites[1][1]
            0x2A -> sprites[1][2]
            0x2E -> sprites[1][3]
            0x32 -> sprites[1][4]
            0x36 -> sprites[1][5]
        */

        if ((var_3C & (1 << 5)) == 0 && (var_3C & (1 << 4)) != 0)
        {
            sprites[0][5] = sprites[0][4];
        }

        if ((var_3C & (1 << 5)) == 0 && (var_3C & (1 << 1)) != 0)
        {
            sprites[0][5] = sprites[0][1];
        }

        if ((var_3C & (1 << 4)) == 0 && (var_3C & (1 << 1)) != 0)
        {
            sprites[0][4] = sprites[0][1];
        }

        if ((var_3C & (1 << 1)) == 0 && (var_3C & (1 << 0)) != 0)
        {
            sprites[0][1] = sprites[0][0];
        }

        if ((var_3C & (1 << 0)) == 0 && (var_3C & (1 << 1)) != 0)
        {
            sprites[0][0] = sprites[0][1];
        }

        if ((flags & TreeObjectFlags::hasSnowVariation) != TreeObjectFlags::none)
        {
            for (auto variant = 0; variant < 6; variant++)
            {
                sprites[1][variant] = sprites[0][variant] + numPrimaryImages;
            }

            nextImageOffset = numPrimaryImages * 2;
        }
        else
            nextImageOffset = numPrimaryImages;

        if ((flags & TreeObjectFlags::hasShadow) != TreeObjectFlags::none)
        {
            shadowImageOffset = nextImageOffset;
        }

        /*
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004BE144, regs);
        */
    }

    // 0x004BE231
    void TreeObject::unload()
    {
        name = 0;
        for (auto& spriteSeason : sprites)
        {
            for (auto& sprite : spriteSeason)
            {
                sprite = 0;
            }
        }
        shadowImageOffset = 0;
    }
}
