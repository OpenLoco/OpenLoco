#include "Chart.h"
#include "Economy/Currency.h"
#include "Graphics/TextRenderer.h"
#include "Ui/Window.h"

namespace OpenLoco::Ui
{
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
            if ((gs.flags & GraphFlags::dataFrontToBack) != GraphFlags::none)
            {
                dataIndex = gs.dataStart[lineIndex];
                dataPtr = &dataPtr[gs.dataTypeSize * (gs.dataEnd - dataIndex)];
            }

            while (dataIndex < gs.dataEnd)
            {
                // Data front-to-back?
                // NB: all charts except cargo delivery
                if ((gs.flags & GraphFlags::dataFrontToBack) != GraphFlags::none)
                {
                    dataPtr -= gs.dataTypeSize;
                }

                int64_t value = graphGetValueFromPointer(dataPtr, gs.dataTypeSize);
                maxValue = std::max(maxValue, std::abs(value));
                dataIndex++;

                // Data back-to-front?
                // NB: for cargo delivery chart
                if ((gs.flags & GraphFlags::dataFrontToBack) == GraphFlags::none)
                {
                    dataPtr += gs.dataTypeSize;
                }
            }
        }

        return maxValue;
    }

    // 0x004CFA49
    static void graphDrawAxesAndLabels(const GraphSettings& gs, Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto xAxisLabelValue = gs.xAxisRange;
        if ((gs.flags & GraphFlags::dataFrontToBack) != GraphFlags::none)
        {
            xAxisLabelValue -= (gs.dataEnd - 1) * gs.xAxisStepSize;
        }

        // 0x004CFA74
        for (auto xTickPos = 0U; xTickPos < gs.dataEnd; xTickPos++)
        {
            auto remainder = xAxisLabelValue % gs.xAxisLabelIncrement;

            // Draw vertical lines for each of the data points
            {
                auto xPos = xTickPos * gs.xAxisTickIncrement + gs.left + gs.xOffset;
                auto height = gs.canvasHeight + (remainder == 0 ? 3 : 0);

                auto colour = self.getColour(WindowColour::secondary).c();
                auto paletteIndex = Colours::getShade(colour, remainder == 0 ? 6 : 4);

                drawingCtx.drawRect(xPos, gs.top, 1, height, paletteIndex, Gfx::RectFlags::none);
            }

            // No remainder means we get to draw a label on the horizontal axis, too
            if (remainder == 0)
            {
                int16_t xPos = xTickPos * gs.xAxisTickIncrement + gs.left + gs.xOffset;
                int16_t yPos = gs.canvasBottom + 5;

                auto tr = Gfx::TextRenderer(drawingCtx);
                auto formatArgs = FormatArguments{};
                formatArgs.push(gs.xLabel);
                formatArgs.push(xAxisLabelValue);

                tr.drawStringCentred({ xPos, yPos }, Colour::black, StringIds::graph_label_format, formatArgs);
            }

            xAxisLabelValue += gs.xAxisStepSize;
        }

        // 0x004CFB5C
        auto yAxisPos = 0;
        while (true)
        {
            // Draw horizontal lines for each of the vertical axis labels
            {
                auto colour = self.getColour(WindowColour::secondary).c();
                auto paletteIndex = Colours::getShade(colour, 6);

                auto xPos = gs.left + gs.xOffset - 2;
                auto width = gs.width - gs.xOffset + 3;
                auto yPos = -yAxisPos + gs.canvasHeight + gs.top;
                if ((gs.flags & GraphFlags::showNegativeValues) != GraphFlags::none)
                {
                    yPos -= gs.canvasHeight / 2;
                }

                drawingCtx.drawRect(xPos, yPos, width, 1, paletteIndex, Gfx::RectFlags::none);

                // Draw negative counterpart?
                if ((gs.flags & GraphFlags::showNegativeValues) != GraphFlags::none)
                {
                    yPos = yAxisPos + gs.top + gs.canvasHeight / 2;
                    drawingCtx.drawRect(xPos, yPos, width, 1, paletteIndex, Gfx::RectFlags::none);
                }
            }

            // Draw the value label as well
            {
                int16_t xPos = gs.left + gs.xOffset - 3;
                // int16_t width = gs.xOffset - 3; // set but not used
                int16_t yPos = -yAxisPos + gs.canvasHeight + gs.top - 5;
                if ((gs.flags & GraphFlags::showNegativeValues) != GraphFlags::none)
                {
                    yPos -= gs.canvasHeight / 2;
                }

                int64_t yAxisLabelValue = static_cast<int64_t>(yAxisPos) << gs.numValueShifts;

                auto tr = Gfx::TextRenderer(drawingCtx);
                auto formatArgs = FormatArguments{};
                formatArgs.push(gs.yLabel);
                formatArgs.push<currency48_t>(yAxisLabelValue);

                tr.drawStringRight({ xPos, yPos }, Colour::black, StringIds::graph_label_format, formatArgs);

                // Draw negative counterpart?
                if ((gs.flags & GraphFlags::showNegativeValues) != GraphFlags::none)
                {
                    yPos = yAxisPos + gs.top + gs.canvasHeight / 2 - 5;

                    formatArgs = FormatArguments{};
                    formatArgs.push(gs.yLabel);
                    formatArgs.push<currency48_t>(-yAxisLabelValue);

                    tr.drawStringRight({ xPos, yPos }, Colour::black, StringIds::graph_label_format, formatArgs);
                }
            }

            // 0x004CFD36
            yAxisPos += gs.yAxisLabelIncrement;

            auto quadrantHeight = yAxisPos;
            if ((gs.flags & GraphFlags::showNegativeValues) != GraphFlags::none)
            {
                quadrantHeight *= 2;
            }

            if (quadrantHeight >= gs.canvasHeight)
            {
                break;
            }
        }

        // 0x004CFD59 after loop, which is back in drawGraph
    }

    // 0x004CFD87
    static void drawGraphLineSegments(const GraphSettings& gs, const uint8_t lineIndex, Gfx::DrawingContext& drawingCtx, GraphPointFlags pointFlag)
    {
        auto previousPos = Ui::Point(-1, 0);

        auto dataIndex = 0U;
        std::byte* dataPtr = gs.yData[lineIndex];
        if ((gs.flags & GraphFlags::dataFrontToBack) != GraphFlags::none)
        {
            dataIndex = gs.dataStart[lineIndex];
            dataPtr = &dataPtr[gs.dataTypeSize * (gs.dataEnd - dataIndex)];
        }

        while (dataIndex < gs.dataEnd)
        {
            if ((gs.flags & GraphFlags::dataFrontToBack) != GraphFlags::none)
            {
                dataPtr -= gs.dataTypeSize;
            }

            int64_t value = graphGetValueFromPointer(dataPtr, gs.dataTypeSize);
            value >>= gs.numValueShifts;

            int16_t xPos = gs.canvasLeft + dataIndex * gs.xAxisTickIncrement;
            int16_t yPos = gs.height - value - gs.yOffset;

            if ((gs.flags & GraphFlags::showNegativeValues) != GraphFlags::none)
            {
                yPos -= gs.canvasHeight / 2;
            }

            yPos += gs.top;

            if (pointFlag == GraphPointFlags::drawLines)
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
            else if (pointFlag == GraphPointFlags::drawPoints)
            {
                auto colour = gs.lineColour[lineIndex];
                drawingCtx.drawRect(xPos, yPos, 2, 2, colour, Gfx::RectFlags::none);
            }

            dataIndex++;

            if ((gs.flags & GraphFlags::dataFrontToBack) == GraphFlags::none)
            {
                dataPtr += gs.dataTypeSize;
            }
        }
    }

    // 0x004CF824
    void drawGraph(GraphSettings& gs, Window& self, Gfx::DrawingContext& drawingCtx)
    {
        gs.canvasLeft = gs.xOffset + gs.left;
        gs.canvasHeight = gs.height - gs.yOffset;
        gs.canvasBottom = gs.top + gs.height - gs.yOffset;

        int64_t maxValue = graphGetMaxValue(gs);

        // 0x004CFA02

        auto height = gs.canvasHeight;
        if ((gs.flags & GraphFlags::showNegativeValues) != GraphFlags::none)
        {
            height >>= 1;
        }

        // We work out the number of shifts required to bring a 64bit value
        // into something that is within the height range
        gs.numValueShifts = 0;
        for (auto adjustedMaxValue = maxValue; adjustedMaxValue > height; adjustedMaxValue >>= 1)
        {
            gs.numValueShifts++;
        }

        if ((gs.flags & GraphFlags::hideAxesAndLabels) == GraphFlags::none)
        {
            graphDrawAxesAndLabels(gs, self, drawingCtx);
        }

        // 0x004CFD59
        for (auto j = 0; j < 2; j++)
        {
            auto pointFlag = GraphPointFlags(1U << j);
            if ((gs.pointFlags & pointFlag) == GraphPointFlags::none)
            {
                continue;
            }

            for (auto i = 0U; i < gs.lineCount; i++)
            {
                if ((gs.linesToExclude & (1U << i)) != 0)
                {
                    continue;
                }

                drawGraphLineSegments(gs, i, drawingCtx, pointFlag);
            }
        }
    }
}
