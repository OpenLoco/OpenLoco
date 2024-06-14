#pragma once

#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>
#include <cstdint>

namespace OpenLoco::Gfx
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
        static inline Interop::loco_global<ScreenInvalidationData, 0x0050B8A0> _screenInvalidation;
        static inline Interop::loco_global<uint8_t[7500], 0x00E025C4> _blocks;
        uint32_t _screenWidth{};
        uint32_t _screenHeight{};

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
            const auto blockWidth = _screenInvalidation->blockWidth;
            const auto blockHeight = _screenInvalidation->blockHeight;

            for (uint32_t column = 0; column < columnCount; column++)
            {
                for (uint32_t row = 0; row < rowCount; row++)
                {
                    const auto rowStartOffset = row * columnCount;
                    if (_blocks[rowStartOffset + column] != 0)
                    {
                        uint32_t rowEndOffset = rowStartOffset;
                        uint32_t numRowsDirty = 0;

                        // Count amount of dirty rows at current column.
                        while (true)
                        {
                            if (row + numRowsDirty + 1 >= rowCount || _blocks[rowEndOffset + column + columnCount] == 0)
                                break;

                            numRowsDirty++;
                            rowEndOffset += columnCount;
                        }

                        // Clear rows at the current column.
                        for (auto rowOffset = rowStartOffset; rowOffset <= rowEndOffset; rowOffset += columnCount)
                        {
                            _blocks[rowOffset + column] = 0;
                        }

                        // Convert to pixel coordinates.
                        const auto left = column * blockWidth;
                        const auto top = row * blockHeight;
                        const auto right = (column + 1) * blockWidth;
                        const auto bottom = (row + numRowsDirty + 1) * blockHeight;

                        if (left < _screenWidth && top < _screenHeight)
                        {
                            func(left, top, std::min(right, _screenWidth), std::min(bottom, _screenHeight));
                        }
                    }
                }
            }
        }
    };

} // namespace OpenRCT2
