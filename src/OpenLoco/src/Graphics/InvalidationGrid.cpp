#include "InvalidationGrid.h"

#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>
#include <cstring>

using namespace OpenLoco::Interop;

namespace OpenLoco::Gfx
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

        // TODO: Remove this once _blocks is no longer interop wrapper.
        auto* blocks = _blocks.get();

        const auto columnSize = right - left + 1;

        for (int16_t y = top; y <= bottom; y++)
        {
            const auto yOffset = y * _screenInvalidation->columnCount;

            // Mark row by column size as invalidated.
            std::memset(blocks + yOffset + left, 0xFF, columnSize);
        }
    }

}
