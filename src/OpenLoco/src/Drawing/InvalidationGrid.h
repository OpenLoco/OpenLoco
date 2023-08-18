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
                uint32_t index = 0;
                for (uint32_t row = 0; row < rowCount; row++)
                {
                    if (_blocks[index + column] != 0)
                    {
                        uint32_t columnStart = column;
                        uint32_t rowStart = row;
                        uint32_t index2 = index;

                        while (true)
                        {
                            rowStart++;
                            index2 += columnCount;
                            if (rowStart >= rowCount || _blocks[index2 + columnStart] == 0)
                                break;
                        }

                        rowStart--;
                        index2 -= columnCount;

                        clearBlocks(index, column, columnStart, index2);

                        const auto left = column * blockWidth;
                        const auto top = row * blockHeight;
                        const auto right = (columnStart + 1) * blockWidth;
                        const auto bottom = (rowStart + 1) * blockHeight;

                        if (left < _screenWidth && top < _screenHeight)
                        {
                            func(left, top, std::min(right, _screenWidth), std::min(bottom, _screenHeight));
                        }
                    }
                    index += columnCount;
                }
            }
        }

    private:
        void clearBlocks(uint32_t index, uint32_t column, uint32_t columnStart, uint32_t index2) noexcept
        {
            const auto columnCount = _screenInvalidation->columnCount;
            do
            {
                uint32_t tempColumn = column;
                do
                {
                    _blocks[index + tempColumn] = 0;
                    tempColumn++;
                } while (tempColumn <= columnStart);

                index += columnCount;
            } while (index <= index2);
        }
    };

} // namespace OpenRCT2
