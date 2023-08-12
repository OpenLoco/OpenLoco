#pragma once

#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>
#include <cstdint>

namespace OpenLoco::Drawing
{
#pragma pack(push, 1)
    struct ScreenInvalidationData
    {
        uint16_t blockWidth;
        uint16_t blockHeight;
        uint32_t columnCount;
        uint32_t rowCount;
        // TODO: Unused, remove when interop is no longer required.
        int8_t columnShift;
        int8_t rowShift;
        int8_t initialised;
    };
    static_assert(sizeof(ScreenInvalidationData) == 0xF);
#pragma pack(pop)

    class InvalidationGrid
    {
        // TODO: Make non-static once interop is no longer required.
        static Interop::loco_global<ScreenInvalidationData, 0x0050B8A0> _screenInvalidation;
        static Interop::loco_global<uint8_t[7500], 0x00E025C4> _blocks;
        int32_t _screenWidth{};
        int32_t _screenHeight{};

    public:
        uint32_t getRowCount() const noexcept;

        uint32_t getColumnCount() const noexcept;

        uint32_t getBlockWidth() const noexcept;

        uint32_t getBlockHeight() const noexcept;

        void reset(int32_t width, int32_t height, uint32_t blockWidth, uint32_t blockHeight) noexcept;

        void invalidate(int32_t left, int32_t top, int32_t right, int32_t bottom) noexcept;

        template<typename F>
        void traverseDirtyCells(F&& func)
        {
            const auto columnCount = _screenInvalidation->columnCount;
            const auto rowCount = _screenInvalidation->rowCount;

            for (uint32_t x = 0; x < columnCount; x++)
            {
                for (uint32_t y = 0; y < rowCount; y++)
                {
                    if (_blocks[y * columnCount + x])
                    {
                        const auto cols = getCountColumnsInvalidated(x, y);
                        const auto rows = getCountRowsInvalidated(x, y);

                        // Draw the region.
                        func(x, y, cols, rows);

                        // Unset rows and cols
                        clearRegion(x, y, cols, rows);
                    }
                }
            }
        }

    private:
        uint32_t getCountRowsInvalidated(const uint32_t x, const uint32_t y) noexcept;

        uint32_t getCountColumnsInvalidated(const uint32_t x, const uint32_t y) noexcept;

        void clearRegion(uint32_t x, uint32_t y, uint32_t cols, uint32_t rows) noexcept;
    };

} // namespace OpenRCT2
