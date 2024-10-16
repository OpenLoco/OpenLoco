#pragma once

#include "Graphics/Colour.h"
#include "Types.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <cstddef>

namespace OpenLoco::Gfx
{
    class DrawingContext;
}

namespace OpenLoco::Ui
{
    struct Window;

    enum class GraphFlags : uint8_t
    {
        none = 0U,
        showNegativeValues = 1U << 0,
        dataFrontToBack = 1U << 1,
        hideAxesAndLabels = 1U << 2,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(GraphFlags);

    enum class GraphPointFlags : uint8_t
    {
        none = 0U,
        drawLines = 1U << 0,
        drawPoints = 1U << 1,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(GraphPointFlags);

    constexpr auto kMaxLines = 32;

    struct GraphSettings
    {
        uint16_t left;                        // 0x0113DC7A
        uint16_t top;                         // 0x0113DC7C
        uint16_t width;                       // 0x0113DC7E
        uint16_t height;                      // 0x0113DC80
        uint16_t yOffset;                     // 0x0113DC82
        uint16_t xOffset;                     // 0x0113DC84
        uint32_t yAxisLabelIncrement;         // 0x0113DC86
        uint16_t lineCount;                   // 0x0113DC8A
        std::byte* yData[kMaxLines];          // 0x0113DC8C
        uint32_t dataTypeSize;                // 0x0113DD0C
        uint16_t dataStart[kMaxLines];        // 0x0113DD10
        uint32_t linesToExclude;              // 0x0113DD50
        PaletteIndex_t lineColour[kMaxLines]; // 0x0113DD54
        uint16_t dataEnd;                     // 0x0113DD74
        StringId xLabel;                      // 0x0113DD76
        uint32_t xAxisRange;                  // 0x0113DD78
        uint32_t xAxisStepSize;               // 0x0113DD7C
        uint16_t xAxisTickIncrement;          // 0x0113DD80
        uint16_t xAxisLabelIncrement;         // 0x0113DD82
        StringId yLabel;                      // 0x0113DD84
        uint32_t dword_113DD86;               // 0x0113DD86 -- always 0
        uint32_t yAxisStepSize;               // 0x0113DD8A
        GraphFlags flags;                     // 0x0113DD8E
        uint16_t canvasLeft;                  // 0x0113DD92
        uint16_t canvasBottom;                // 0x0113DD94
        uint16_t canvasHeight;                // 0x0113DD96
        uint8_t numValueShifts;               // 0x0113DD98 -- factors of two
        GraphPointFlags pointFlags;           // 0x0113DD99
        uint16_t itemId[kMaxLines];           // 0x0113DD9A
    };

    void drawGraph(GraphSettings& gs, Window* self, Gfx::DrawingContext& drawingCtx);
}
