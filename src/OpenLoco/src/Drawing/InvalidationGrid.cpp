#include "InvalidationGrid.h"

#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>

using namespace OpenLoco::Interop;

namespace OpenLoco::Drawing
{
    uint32_t InvalidationGrid::getRowCount() const noexcept
    {
        return _screenInvalidation->rowCount;
    }

    uint32_t InvalidationGrid::getColumnCount() const noexcept
    {
        return _screenInvalidation->columnCount;
    }

    uint32_t InvalidationGrid::getBlockWidth() const noexcept
    {
        return _screenInvalidation->blockWidth;
    }

    uint32_t InvalidationGrid::getBlockHeight() const noexcept
    {
        return _screenInvalidation->blockHeight;
    }

    void InvalidationGrid::reset(int32_t width, int32_t height, uint32_t blockWidth, uint32_t blockHeight) noexcept
    {
        _screenInvalidation->blockWidth = blockWidth;
        _screenInvalidation->blockHeight = blockHeight;
        _screenInvalidation->columnCount = (width / blockWidth) + 1;
        _screenInvalidation->rowCount = (height / blockHeight) + 1;
        _screenInvalidation->initialised = 1;
        _screenWidth = width;
        _screenHeight = height;
    }

    void InvalidationGrid::invalidate(int32_t left, int32_t top, int32_t right, int32_t bottom) noexcept
    {
        left = std::max(left, 0);
        top = std::max(top, 0);
        right = std::min(right, static_cast<int32_t>(_screenWidth));
        bottom = std::min(bottom, static_cast<int32_t>(_screenHeight));

        if (left >= right)
            return;
        if (top >= bottom)
            return;

        left /= _screenInvalidation->blockWidth;
        right /= _screenInvalidation->blockWidth;

        top /= _screenInvalidation->blockHeight;
        bottom /= _screenInvalidation->blockHeight;

        for (int16_t y = top; y <= bottom; y++)
        {
            uint32_t yOffset = y * _screenInvalidation->columnCount;
            for (int16_t x = left; x <= right; x++)
            {
                _blocks[yOffset + x] = 0xFF;
            }
        }
    }

    uint32_t InvalidationGrid::getCountRowsInvalidated(const uint32_t x, const uint32_t y) noexcept
    {
        const auto columnCount = _screenInvalidation->columnCount;
        const auto rowCount = _screenInvalidation->rowCount;

        uint32_t y2 = y;
        while (y2 < rowCount && _blocks[y2 * columnCount + x])
            y2++;
        return y2 - y;
    }

    uint32_t InvalidationGrid::getCountColumnsInvalidated(const uint32_t x, const uint32_t y) noexcept
    {
        const auto columnCount = _screenInvalidation->columnCount;

        uint32_t x2 = x;
        while (x2 < columnCount && _blocks[y * columnCount + x2])
            x2++;
        return x2 - x;
    }

    void InvalidationGrid::clearRegion(uint32_t x, uint32_t y, uint32_t cols, uint32_t rows) noexcept
    {
        const auto columnCount = _screenInvalidation->columnCount;

        for (uint32_t x2 = x; x2 < x + cols; x2++)
        {
            for (uint32_t y2 = y; y2 < y + rows; y2++)
            {
                _blocks[y2 * columnCount + x2] = 0;
            }
        }
    }

}
