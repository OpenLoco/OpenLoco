#include "Chart.h"
#include "Economy/Currency.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Ui/Window.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui
{
    static loco_global<uint32_t, 0x0113658C> _dword_113658C;

    // TODO: replace with templated functions
    static int64_t graphGetValueFromPointer(const std::byte* dataPtr, const uint8_t dataTypeSize)
    {
        switch (dataTypeSize)
        {
            case 2:
                return *reinterpret_cast<const int16_t*>(dataPtr);

            case 4:
                return *reinterpret_cast<const int32_t*>(dataPtr);

            case 6:
                return reinterpret_cast<const currency48_t*>(dataPtr)->asInt64();

            default:
                return 0;
        }
    }

    // 0x004CF869
    static int64_t graphGetMaxValue(const GraphSettings& gs)
    {
        int64_t maxValue = 0;
        for (auto lineIndex = 0U; lineIndex < gs.lineCount; lineIndex++)
        {
            auto dataIndex = 0U;
            std::byte* dataPtr = gs.yData[lineIndex];
            if ((gs.flags & (1 << 1)) != 0)
            {
                dataIndex = gs.dataStart[lineIndex];
                dataPtr = &dataPtr[gs.dataTypeSize * (gs.dataEnd - dataIndex)];
            }

            while (dataIndex < gs.dataEnd)
            {
                // Data front-to-back?
                // NB: all charts except cargo delivery
                if ((gs.flags & (1 << 1)) != 0)
                {
                    dataPtr -= gs.dataTypeSize;
                }

                int64_t value = graphGetValueFromPointer(dataPtr, gs.dataTypeSize);
                maxValue = std::max(maxValue, std::abs(value));
                dataIndex++;

                // Data back-to-front?
                // NB: for cargo delivery chart
                if ((gs.flags & (1 << 1)) == 0)
                {
                    dataPtr += gs.dataTypeSize;
                }
            }
        }

        return maxValue;
    }

    // 0x004CFA49
    static void graphDrawAxesAndLabels(const GraphSettings& gs, Window* self, Gfx::DrawingContext& drawingCtx)
    {
        auto eax = gs.xAxisRange;
        if (gs.flags & (1 << 1))
        {
            eax -= (gs.dataEnd - 1) * gs.xAxisStepSize;
        }
        // eax expected at _common_format_args.bits+2

        // 0x004CFA74
        for (auto ecx = 0U; ecx < gs.dataEnd; ecx++)
        {
            // auto quotient = eax / gs.xAxisLabelIncrement;  // eax
            auto remainder = eax % gs.xAxisLabelIncrement; // edx

            // Draw vertical lines for each of the data points
            {
                auto xPos = ecx * gs.word_113DD80 + gs.left + gs.xOffset;
                auto height = gs.canvasHeight + (remainder > 0 ? 3 : 0);

                auto colour = self->getColour(WindowColour::secondary).c();
                auto paletteIndex = Colours::getShade(colour, remainder == 0 ? 6 : 4);

                drawingCtx.drawRect(xPos, gs.top, 1, height, paletteIndex, Gfx::RectFlags::none);
            }

            // No remainder means we get to draw a label on the horizontal axis, too
            if (remainder == 0)
            {
                int16_t xPos = ecx * gs.word_113DD80 + gs.left + gs.xOffset;
                int16_t yPos = gs.top + gs.height - gs.yOffset + 5;

                auto tr = Gfx::TextRenderer(drawingCtx);
                auto formatArgs = FormatArguments{};
                formatArgs.push(gs.xLabel);
                formatArgs.push(eax);

                tr.drawStringCentred({ xPos, yPos }, Colour::black, StringIds::graph_label_format, formatArgs);
            }

            eax += gs.xAxisStepSize;
        }

        // 0x004CFB5C
        auto edx = 0;
        while (true)
        {
            // TODO: can be moved down, around FormatArguments. Kept here for verification
            int64_t ebx_eax = static_cast<int64_t>(edx) << gs.numValueShifts;

            // Draw horizontal lines for each of the vertical axis labels
            {
                auto colour = self->getColour(WindowColour::secondary).c();
                auto paletteIndex = Colours::getShade(colour, 6);

                auto xPos = gs.left + gs.xOffset - 2;
                auto width = gs.width - gs.xOffset + 3;
                auto yPos = -edx + gs.canvasHeight + gs.top;
                if (gs.flags & (1 << 0)) // never set
                {
                    yPos -= gs.canvasHeight / 2;
                }

                drawingCtx.drawRect(xPos, yPos, width, 1, paletteIndex, Gfx::RectFlags::none);
            }

            // Draw the value label as well
            {
                int16_t xPos = gs.left + gs.xOffset - 3;
                // int16_t width = gs.xOffset - 3; // set but not used
                int16_t yPos = -edx + gs.canvasHeight + gs.top - 5;
                if (gs.flags & (1 << 0)) // never set
                {
                    yPos -= gs.canvasHeight / 2;
                }

                auto tr = Gfx::TextRenderer(drawingCtx);
                auto formatArgs = FormatArguments{};
                formatArgs.push(gs.yLabel);
                formatArgs.push<currency48_t>(ebx_eax);

                tr.drawStringRight({ xPos, yPos }, Colour::black, StringIds::graph_label_format, formatArgs);
            }

            if (gs.flags & (1 << 0)) // never set
            {
                // presumably draws negative numbers as well
            }

            // 0x004CFD36
            edx += gs.yAxisLabelIncrement;
            auto ebp = edx;
            if (gs.flags & (1 << 0))
            {
                ebp <<= 1;
            }

            if (ebp >= gs.canvasHeight)
                break;
        }

        // 0x004CFD59 after loop, which is back in drawGraph
    }

    // 0x004CFD87
    static void drawGraphLineSegments(const GraphSettings& gs, const uint8_t lineIndex, Gfx::DrawingContext& drawingCtx)
    {
        auto previousPos = Ui::Point(-1, 0);

        auto dataIndex = 0U;
        std::byte* dataPtr = gs.yData[lineIndex];
        if ((gs.flags & (1 << 1)) != 0)
        {
            dataIndex = gs.dataStart[lineIndex];
            dataPtr = &dataPtr[gs.dataTypeSize * (gs.dataEnd - dataIndex)];
        }

        while (dataIndex < gs.dataEnd)
        {
            if (gs.flags & (1 << 1))
            {
                dataPtr -= gs.dataTypeSize;
            }

            int64_t value = graphGetValueFromPointer(dataPtr, gs.dataTypeSize);
            value >>= gs.numValueShifts;

            int16_t xPos = gs.canvasLeft + dataIndex * gs.word_113DD80;
            int16_t yPos = gs.height - value - gs.yOffset;

            if (gs.flags & (1 << 0)) // unused?
            {
                yPos -= gs.canvasHeight / 2;
            }

            yPos += gs.top;

            if (_dword_113658C != 1)
            {
                auto colour = gs.lineColour[lineIndex];
                drawingCtx.drawRect(xPos, yPos, 1, 1, colour, Gfx::RectFlags::none);

                auto targetPos = Ui::Point(xPos, yPos);
                if (previousPos.x != -1)
                {
                    drawingCtx.drawLine(previousPos, targetPos, colour);
                }
                previousPos = targetPos;
            }
            else
            {
                auto colour = gs.lineColour[lineIndex];
                drawingCtx.drawRect(xPos, yPos, 2, 2, colour, Gfx::RectFlags::none);
            }

            dataIndex++;

            if (!(gs.flags & (1 << 1)))
            {
                dataPtr += gs.dataTypeSize;
            }
        }
    }

    // 0x004CF824
    void drawGraph(GraphSettings& gs, Window* self, Gfx::DrawingContext& drawingCtx)
    {
        if (Input::hasKeyModifier(Input::KeyModifier::shift))
        {
            self->invalidate();
            const auto& rt = drawingCtx.currentRenderTarget();
            registers regs;
            regs.esi = X86Pointer(self);
            regs.edi = X86Pointer(&rt);
            call(0x004CF824, regs);
            return;
        }

        gs.canvasLeft = gs.xOffset + gs.left;
        gs.canvasHeight = gs.height - gs.yOffset;

        // TODO: unused? remove?
        gs.canvasBottom = gs.top + gs.height - gs.yOffset;

        int64_t maxValue = graphGetMaxValue(gs);

        // 0x004CFA02

        auto height = gs.canvasHeight;
        if (gs.flags & (1 << 0))
        {
            // half height flag never set likely for negative values
            height >>= 1;
        }

        // We work out the number of shifts required to bring a 64bit value
        // into something that is within the height range
        gs.numValueShifts = 0;
        for (auto adjustedMaxValue = maxValue; adjustedMaxValue > height; adjustedMaxValue >>= 1)
        {
            gs.numValueShifts++;
        }

        if (!(gs.flags & (1 << 2)))
        {
            graphDrawAxesAndLabels(gs, self, drawingCtx);
        }

        // 0x004CFD59
        for (_dword_113658C = 0; _dword_113658C < 2; _dword_113658C++) // iteration/pass??
        {
            if ((gs.byte_113DD99 & (1U << _dword_113658C)) == 0)
            {
                continue;
            }

            for (auto i = 0U; i < gs.lineCount; i++)
            {
                if ((gs.linesToExclude & (1U << i)) != 0)
                {
                    continue;
                }

                drawGraphLineSegments(gs, i, drawingCtx);
            }
        }
    }
}
