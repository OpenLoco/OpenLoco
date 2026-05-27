#pragma once
#include <cstdint>
#include <span>

namespace OpenLoco::ObjectManager
{
    struct ImageTableResult
    {
        uint32_t imageOffset;
        uint32_t tableLength;
    };

    ImageTableResult loadImageTable(std::span<const std::byte> data);
    uint32_t getTotalNumImages();
    void setTotalNumImages(uint32_t count);
}
