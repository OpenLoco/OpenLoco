#pragma once
#include <OpenLoco/Core/Span.hpp>
#include <cstdint>

namespace OpenLoco::ObjectManager
{
    struct ImageTableResult
    {
        uint32_t imageOffset;
        uint32_t tableLength;
    };

    ImageTableResult loadImageTable(stdx::span<const std::byte> data);
    uint32_t getTotalNumImages();
    void setTotalNumImages(uint32_t count);
}
