#include "ObjectImageTable.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::ObjectManager
{
    static loco_global<uint32_t, 0x0050D154> _totalNumImages;

    // 0x0047221F
    ImageTableResult loadImageTable(stdx::span<const std::byte> data)
    {
        auto remainingData = data;
        const auto g1Header = *reinterpret_cast<const Gfx::G1Header*>(remainingData.data());
        remainingData = remainingData.subspan(sizeof(Gfx::G1Header));

        const ImageTableResult res = { _totalNumImages, sizeof(Gfx::G1Header) + g1Header.numEntries * sizeof(Gfx::G1Element32) + g1Header.totalSize };

        const auto* g32Ptr = reinterpret_cast<const Gfx::G1Element32*>(remainingData.data());
        remainingData = remainingData.subspan(sizeof(Gfx::G1Element32) * g1Header.numEntries);
        // Urgh messy...
        auto* const imageDataBegin = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(remainingData.data()));

        for (uint32_t i = 0; i < g1Header.numEntries; ++i, ++g32Ptr)
        {
            auto g1Element = Gfx::G1Element(*g32Ptr);
            g1Element.offset = imageDataBegin + g32Ptr->offset;
            if (g1Element.flags & Gfx::G1ElementFlags::duplicatePrevious)
            {
                g1Element = *Gfx::getG1Element(_totalNumImages + i - 1);
                g1Element.xOffset += g32Ptr->xOffset;
                g1Element.yOffset += g32Ptr->yOffset;
            }
            *Gfx::getG1Element(_totalNumImages + i) = g1Element;
        }
        _totalNumImages += g1Header.numEntries;
        return res;
    }

    uint32_t getTotalNumImages()
    {
        return _totalNumImages;
    }

    void setTotalNumImages(uint32_t count)
    {
        _totalNumImages = count;
    }
}
