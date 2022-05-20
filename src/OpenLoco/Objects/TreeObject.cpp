#include "TreeObject.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Interop/Interop.hpp"

namespace OpenLoco
{
    // 0x004BE2A2
    void TreeObject::drawPreviewImage(Gfx::Context& context, const int16_t x, const int16_t y) const
    {
        uint32_t image = getTreeGrowthDisplayOffset() * num_rotations;
        auto rotation = (num_rotations - 1) & 2;
        image += rotation;
        image += sprites[0][season_state];

        auto colourOptions = colours;
        if (colourOptions != 0)
        {

            auto bit = Utility::bitScanReverse(colourOptions);

            auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

            image = Gfx::recolour(image, colour);
        }

        Ui::Point treePos = Ui::Point{ x, y } + Ui::Point{ 0, 48 };

        if (flags & TreeObjectFlags::hasSnowVariation)
        {
            auto snowImage = getTreeGrowthDisplayOffset() * num_rotations;
            snowImage += rotation;
            snowImage += sprites[1][season_state];

            if (colourOptions != 0)
            {
                auto bit = Utility::bitScanReverse(colourOptions);

                auto colour = (bit == -1) ? Colour::black : static_cast<Colour>(bit);

                snowImage = Gfx::recolour(snowImage, colour);
            }
            treePos.x = x + 28;
            Gfx::drawImage(&context, treePos.x, treePos.y, snowImage);
            treePos.x = 28;
        }
        Gfx::drawImage(&context, treePos.x, treePos.y, image);
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
        if (cost_index > 32)
        {
            return false;
        }

        // 230/256 = ~90%
        if (-clear_cost_factor > build_cost_factor * 230 / 256)
        {
            return false;
        }

        switch (num_rotations)
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
    void TreeObject::load(const LoadedObjectHandle& handle, stdx::span<std::byte> data)
    {
        Interop::registers regs;
        regs.esi = Interop::X86Pointer(this);
        regs.ebx = handle.id;
        regs.ecx = enumValue(handle.type);
        Interop::call(0x004BE144, regs);
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
