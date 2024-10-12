#pragma once

#include "Graphics/Colour.h"
#include "Types.hpp"
#include <cstddef>

namespace OpenLoco::Gfx
{
    class DrawingContext;
}

namespace OpenLoco::Ui
{
    struct Window;

#pragma pack(push, 1)
    struct GraphSettings
    {
        uint16_t left;                 // 0x0113DC7A
        uint16_t top;                  // 0x0113DC7C
        uint16_t width;                // 0x0113DC7E
        uint16_t height;               // 0x0113DC80
        uint16_t yOffset;              // 0x0113DC82
        uint16_t xOffset;              // 0x0113DC84
        uint32_t yAxisLabelIncrement;  // 0x0113DC86
        uint16_t lineCount;            // 0x0113DC8A
        std::byte* yData[32];          // 0x0113DC8C
        uint32_t dataTypeSize;         // 0x0113DD0C
        uint16_t dataStart[32];        // 0x0113DD10
        uint32_t linesToExclude;       // 0x0113DD50
        PaletteIndex_t lineColour[32]; // 0x0113DD54
        uint16_t dataEnd;              // 0x0113DD74
        StringId xLabel;               // 0x0113DD76
        uint32_t xAxisRange;           // 0x0113DD78
        uint32_t xAxisStepSize;        // 0x0113DD7C
        uint16_t word_113DD80;         // 0x0113DD80 -- graphXAxisIncrement?
        uint16_t xAxisLabelIncrement;  // 0x0113DD82
        StringId yLabel;               // 0x0113DD84
        uint32_t dword_113DD86;        // 0x0113DD86 -- always 0
        uint32_t yAxisStepSize;        // 0x0113DD8A
        uint32_t flags;                // 0x0113DD8E
        uint16_t canvasLeft;           // 0x0113DD92
        uint16_t canvasBottom;         // 0x0113DD94
        uint16_t canvasHeight;         // 0x0113DD96
        uint8_t numValueShifts;        // 0x0113DD98 -- factors of two
        uint8_t byte_113DD99;          // 0x0113DD99
        uint16_t itemId[32];           // 0x0113DD9A
    };
#pragma pack(pop)

    static_assert(sizeof(GraphSettings) == 0x0113DD9A + sizeof(GraphSettings::itemId) - 0x0113DC7A);

    void drawGraph(GraphSettings& gs, Window* self, Gfx::DrawingContext& drawingCtx);
}
