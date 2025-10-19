#include "ScrollView.h"
#include "Input.h"
#include "ToolTip.h"
#include "Ui.h"
#include "Widget.h"
#include "WindowManager.h"
#include <cmath>

namespace OpenLoco::Ui::ScrollView
{
    Ui::ScrollPart _currentScrollArea; // 0x00523396
    size_t _currentScrollIndex;        // 0x00523398

    // 0x004C87E1
    // regs.bp: deltaX
    static void horizontalFollow(Ui::Window& window, Ui::Widget* const widget, const WidgetIndex_t widgetIndex, const size_t scrollIndex, const int16_t deltaX)
    {
        ScrollArea& scrollArea = window.scrollAreas[scrollIndex];
        scrollArea.flags |= ScrollFlags::hscrollbarThumbPressed;

        uint16_t trackWidth = widget->width() - (kThumbSize * 2);
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarVisible))
        {
            trackWidth -= kScrollbarSize;
        }

        if (trackWidth == 0)
        {
            return;
        }
        auto contentDeltaX = deltaX * scrollArea.contentWidth / trackWidth;

        int16_t newOffset = scrollArea.contentOffsetX + contentDeltaX;

        int frameWidth = widget->width() - (kScrollbarMargin * 2);
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarVisible))
        {
            frameWidth -= kScrollbarSize;
        }

        int16_t maxOffset = scrollArea.contentWidth - frameWidth;
        maxOffset = std::max<int16_t>(maxOffset, 0);

        scrollArea.contentOffsetX = std::clamp<int16_t>(newOffset, 0, maxOffset);

        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C8898
    // regs.bp: deltaY
    static void verticalFollow(Ui::Window& window, Ui::Widget* const widget, const WidgetIndex_t widgetIndex, const size_t scrollIndex, const int16_t deltaY)
    {
        ScrollArea& scrollArea = window.scrollAreas[scrollIndex];
        scrollArea.flags |= ScrollFlags::vscrollbarThumbPressed;

        uint16_t trackHeight = widget->height() - (kThumbSize * 2);
        if (scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            trackHeight -= kScrollbarSize;
        }

        if (trackHeight == 0)
        {
            return;
        }
        auto contentDeltaY = deltaY * scrollArea.contentHeight / trackHeight;

        int16_t newOffset = scrollArea.contentOffsetY + contentDeltaY;

        int frameHeight = widget->height() - (kScrollbarMargin * 2);
        if (scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            frameHeight -= kScrollbarSize;
        }

        int16_t maxOffset = scrollArea.contentHeight - frameHeight;
        maxOffset = std::max<int16_t>(maxOffset, 0);

        scrollArea.contentOffsetY = std::clamp<int16_t>(newOffset, 0, maxOffset);

        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C8CFD
    // regs.bp: deltaX
    void horizontalDragFollow(Ui::Window& window, Ui::Widget* const widget, const WidgetIndex_t dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaX)
    {
        ScrollArea& scrollArea = window.scrollAreas[dragScrollIndex];
        if (!scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            return;
        }

        uint16_t trackWidth = widget->width() - 2 - kThumbSize - kThumbSize;
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarVisible))
        {
            trackWidth -= kScrollbarSize;
        }

        if (trackWidth == 0)
        {
            return;
        }
        auto contentDeltaX = deltaX * scrollArea.contentWidth / trackWidth;

        int16_t newOffset = scrollArea.contentOffsetX + contentDeltaX;

        int frameWidth = widget->width() - 2;
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarVisible))
        {
            frameWidth -= kScrollbarSize;
        }

        int16_t maxOffset = scrollArea.contentWidth - frameWidth;
        maxOffset = std::max<int16_t>(maxOffset, 0);

        scrollArea.contentOffsetX = std::clamp<int16_t>(newOffset, 0, maxOffset);

        ScrollView::updateThumbs(window, dragWidgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, dragWidgetIndex);
    }

    // 0x004C8E2E
    // regs.bp: deltaY
    // possible extraction of common functionality in verticalDragFollow, horizontalDragFollow, verticalFollow, horizontalFollow
    void verticalDragFollow(Ui::Window& window, Ui::Widget* const widget, const WidgetIndex_t dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaY)
    {
        ScrollArea& scrollArea = window.scrollAreas[dragScrollIndex];
        if (!scrollArea.hasFlags(ScrollFlags::vscrollbarVisible))
        {
            return;
        }

        uint16_t trackHeight = widget->height() - 2 - kThumbSize - kThumbSize;
        if (scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            trackHeight -= kScrollbarSize;
        }

        if (trackHeight == 0)
        {
            return;
        }
        auto contentDeltaY = deltaY * scrollArea.contentHeight / trackHeight;

        int16_t newOffset = scrollArea.contentOffsetY + contentDeltaY;

        int frameHeight = widget->height() - 2;
        if (scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            frameHeight -= kScrollbarSize;
        }

        int16_t maxOffset = scrollArea.contentHeight - frameHeight;
        maxOffset = std::max<int16_t>(maxOffset, 0);

        scrollArea.contentOffsetY = std::clamp<int16_t>(newOffset, 0, maxOffset);

        ScrollView::updateThumbs(window, dragWidgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, dragWidgetIndex);
    }

    // 0x004C8EF0
    // Note: Original function returns a scrollAreaOffset not an index
    GetPartResult getPart(Ui::Window& window, Ui::Widget* widget, int16_t x, int16_t y)
    {
        GetPartResult res{ { x, y }, ScrollPart::none, 0 };

        for (auto& widget2 : window.widgets)
        {
            if (widget == &widget2)
            {
                break;
            }
            if (widget2.type == WidgetType::scrollview)
            {
                res.index++;
            }
        }

        res.index = std::min<size_t>(res.index, Window::kMaxScrollAreas - 1);

        const auto& scroll = window.scrollAreas[res.index];
        auto right = widget->right + window.x;
        auto left = widget->left + window.x;
        auto top = widget->top + window.y;
        auto bottom = widget->bottom + window.y;

        const bool needsHScroll = scroll.contentWidth > widget->width();
        const bool needsVScroll = scroll.contentHeight > widget->height();

        if (needsHScroll && scroll.hasFlags(ScrollFlags::hscrollbarVisible) && y >= (bottom - kScrollbarSize))
        {
            if (x < left + kScrollbarSize)
            {
                res.area = ScrollPart::hscrollbarButtonLeft;
                return res;
            }

            // If vertical is also visible then there is a deadzone in the corner
            if (scroll.hasFlags(ScrollFlags::vscrollbarVisible))
            {
                right -= kScrollbarSize;
            }

            // Within deadzone
            if (x >= right)
            {
                res.area = ScrollPart::none;
                return res;
            }

            if (x >= right - kThumbSize)
            {
                res.area = ScrollPart::hscrollbarButtonRight;
                return res;
            }

            if (x < scroll.hThumbLeft + left)
            {
                res.area = ScrollPart::hscrollbarTrackLeft;
                return res;
            }

            if (x > scroll.hThumbRight + left)
            {
                res.area = ScrollPart::hscrollbarTrackRight;
                return res;
            }

            res.area = ScrollPart::hscrollbarThumb;
        }
        else if (needsVScroll && scroll.hasFlags(ScrollFlags::vscrollbarVisible) && x >= (right - kScrollbarSize))
        {
            if (y < top + kScrollbarSize)
            {
                res.area = ScrollPart::vscrollbarButtonTop;
                return res;
            }

            // If horizontal is also visible then there is a deadzone in the corner
            if (scroll.hasFlags(ScrollFlags::hscrollbarVisible))
            {
                bottom -= kScrollbarSize;
            }

            // Within deadzone
            if (y >= bottom)
            {
                res.area = ScrollPart::none;
                return res;
            }

            if (y >= bottom - kThumbSize)
            {
                res.area = ScrollPart::vscrollbarButtonBottom;
                return res;
            }

            if (y < scroll.vThumbTop + top)
            {
                res.area = ScrollPart::vscrollbarTrackTop;
                return res;
            }

            if (y > scroll.vThumbBottom + top)
            {
                res.area = ScrollPart::vscrollbarTrackBottom;
                return res;
            }

            res.area = ScrollPart::vscrollbarThumb;
        }
        else
        {
            res.scrollviewLoc.x -= left + 1;
            res.scrollviewLoc.y -= top + 1;
            if (res.scrollviewLoc.x < 0 || res.scrollviewLoc.y < 0)
            {
                res.area = ScrollPart::none;
                return res;
            }
            res.scrollviewLoc.x += scroll.contentOffsetX;
            res.scrollviewLoc.y += scroll.contentOffsetY;
            res.area = ScrollPart::view;
        }
        return res;
    }

    static std::pair<uint16_t, uint16_t> calculateThumbSizeAndPosition(bool otherBarIsVisible, uint16_t widgetSize, uint16_t buttonSize, int32_t contentSize, int32_t contentOffset)
    {
        uint16_t scrollbarSize = widgetSize - (buttonSize * 2);

        if (otherBarIsVisible)
        {
            widgetSize -= kScrollbarSize;
            scrollbarSize -= kScrollbarSize;
        }

        // Thumb size
        uint16_t scrollThumbSize = std::max<uint16_t>(scrollbarSize * widgetSize / (float)contentSize, kMinThumbSize);

        // Thumb position
        auto scrollableDistance = scrollbarSize - scrollThumbSize;
        uint16_t thumbPosition = scrollableDistance * (contentOffset / (float)(contentSize - widgetSize));

        auto scrollThumbStart = buttonSize + thumbPosition;
        auto scrollThumbEnd = std::min(buttonSize + thumbPosition + scrollThumbSize, widgetSize - buttonSize - 1);

        return std::make_pair(scrollThumbStart, scrollThumbEnd);
    }

    // 0x004CA1ED
    void updateThumbs(Window& window, WidgetIndex_t widgetIndex)
    {
        const auto& widget = window.widgets[widgetIndex];
        auto& scrollArea = window.scrollAreas[window.getScrollDataIndex(widgetIndex)];

        // Horizontal scrollbar
        if (scrollArea.hasFlags(ScrollFlags::hscrollbarVisible))
        {
            std::tie(scrollArea.hThumbLeft, scrollArea.hThumbRight) = calculateThumbSizeAndPosition(scrollArea.hasFlags(ScrollFlags::vscrollbarVisible), widget.width(), kScrollButtonSize.width, scrollArea.contentWidth, scrollArea.contentOffsetX);
        }

        // Vertical scrollbar
        if (scrollArea.hasFlags(ScrollFlags::vscrollbarVisible))
        {
            std::tie(scrollArea.vThumbTop, scrollArea.vThumbBottom) = calculateThumbSizeAndPosition(scrollArea.hasFlags(ScrollFlags::hscrollbarVisible), widget.height(), kScrollButtonSize.height, scrollArea.contentHeight, scrollArea.contentOffsetY);
        }
    }

    // 0x004C894F
    static void hButtonLeft(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        window.scrollAreas[scrollAreaIndex].flags |= ScrollFlags::hscrollbarLeftPressed;
        window.scrollAreas[scrollAreaIndex].contentOffsetX = std::max(window.scrollAreas[scrollAreaIndex].contentOffsetX - kButtonClickStep, 0);
        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C89AE
    static void hButtonRight(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        window.scrollAreas[scrollAreaIndex].flags |= ScrollFlags::hscrollbarRightPressed;
        int16_t trackWidth = window.widgets[widgetIndex].width() - 2;
        if (window.scrollAreas[scrollAreaIndex].hasFlags(ScrollFlags::vscrollbarVisible))
        {
            trackWidth -= kScrollbarSize;
        }
        int16_t widgetContentWidth = std::max(window.scrollAreas[scrollAreaIndex].contentWidth - trackWidth, 0);
        window.scrollAreas[scrollAreaIndex].contentOffsetX = std::min<int16_t>(window.scrollAreas[scrollAreaIndex].contentOffsetX + kButtonClickStep, widgetContentWidth);
        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C8A36
    static void hTrackLeft(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackWidth = window.widgets[widgetIndex].width() - 2;
        if (window.scrollAreas[scrollAreaIndex].hasFlags(ScrollFlags::vscrollbarVisible))
        {
            trackWidth -= kScrollbarSize;
        }
        window.scrollAreas[scrollAreaIndex].contentOffsetX = std::max(window.scrollAreas[scrollAreaIndex].contentOffsetX - trackWidth, 0);
        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C8AA6
    static void hTrackRight(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackWidth = window.widgets[widgetIndex].width() - 2;
        if (window.scrollAreas[scrollAreaIndex].hasFlags(ScrollFlags::vscrollbarVisible))
        {
            trackWidth -= kScrollbarSize;
        }
        int16_t widgetContentWidth = std::max(window.scrollAreas[scrollAreaIndex].contentWidth - trackWidth, 0);
        window.scrollAreas[scrollAreaIndex].contentOffsetX = std::min<int16_t>(window.scrollAreas[scrollAreaIndex].contentOffsetX + trackWidth, widgetContentWidth);
        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    void verticalNudgeUp(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        window.scrollAreas[scrollAreaIndex].contentOffsetY = std::max(window.scrollAreas[scrollAreaIndex].contentOffsetY - kButtonClickStep, 0);
        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C8B26
    static void vButtonTop(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        window.scrollAreas[scrollAreaIndex].flags |= ScrollFlags::vscrollbarUpPressed;
        verticalNudgeUp(window, scrollAreaIndex, widgetIndex);
    }

    void verticalNudgeDown(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackHeight = window.widgets[widgetIndex].height() - 2;
        if (window.scrollAreas[scrollAreaIndex].hasFlags(ScrollFlags::hscrollbarVisible))
        {
            trackHeight -= kScrollbarSize;
        }
        int16_t widgetContentHeight = std::max(window.scrollAreas[scrollAreaIndex].contentHeight - trackHeight, 0);
        window.scrollAreas[scrollAreaIndex].contentOffsetY = std::min<int16_t>(window.scrollAreas[scrollAreaIndex].contentOffsetY + kButtonClickStep, widgetContentHeight);
        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C8B85
    static void vButtonBottom(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        window.scrollAreas[scrollAreaIndex].flags |= ScrollFlags::vscrollbarDownPressed;
        verticalNudgeDown(window, scrollAreaIndex, widgetIndex);
    }

    // 0x004C8C0D
    static void vTrackTop(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackHeight = window.widgets[widgetIndex].height() - 2;
        if (window.scrollAreas[scrollAreaIndex].hasFlags(ScrollFlags::hscrollbarVisible))
        {
            trackHeight -= kScrollbarSize;
        }
        window.scrollAreas[scrollAreaIndex].contentOffsetY = std::max(window.scrollAreas[scrollAreaIndex].contentOffsetY - trackHeight, 0);
        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C8C7D
    static void vTrackBottom(Ui::Window& window, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackHeight = window.widgets[widgetIndex].height() - 2;
        if (window.scrollAreas[scrollAreaIndex].hasFlags(ScrollFlags::hscrollbarVisible))
        {
            trackHeight -= kScrollbarSize;
        }
        int16_t widgetContentHeight = std::max(window.scrollAreas[scrollAreaIndex].contentHeight - trackHeight, 0);
        window.scrollAreas[scrollAreaIndex].contentOffsetY = std::min<int16_t>(window.scrollAreas[scrollAreaIndex].contentOffsetY + trackHeight, widgetContentHeight);
        ScrollView::updateThumbs(window, widgetIndex);
        WindowManager::invalidateWidget(window.type, window.number, widgetIndex);
    }

    // 0x004C8689
    void scrollLeftBegin(const int16_t x, const int16_t y, Ui::Window& window, Ui::Widget* const widget, const WidgetIndex_t widgetIndex)
    {
        auto res = getPart(window, widget, x, y);

        _currentScrollArea = res.area;
        _currentScrollIndex = res.index;

        // Not implemented for any window
        // window->call_22()
        switch (res.area)
        {
            case Ui::ScrollPart::view:
                window.callScrollMouseDown(res.scrollviewLoc.x, res.scrollviewLoc.y, static_cast<uint8_t>(res.index));
                break;
            case Ui::ScrollPart::hscrollbarButtonLeft:
                hButtonLeft(window, res.index, widgetIndex);
                break;
            case Ui::ScrollPart::hscrollbarButtonRight:
                hButtonRight(window, res.index, widgetIndex);
                break;
            case Ui::ScrollPart::hscrollbarTrackLeft:
                hTrackLeft(window, res.index, widgetIndex);
                break;
            case Ui::ScrollPart::hscrollbarTrackRight:
                hTrackRight(window, res.index, widgetIndex);
                break;
            case Ui::ScrollPart::vscrollbarButtonTop:
                vButtonTop(window, res.index, widgetIndex);
                break;
            case Ui::ScrollPart::vscrollbarButtonBottom:
                vButtonBottom(window, res.index, widgetIndex);
                break;
            case Ui::ScrollPart::vscrollbarTrackTop:
                vTrackTop(window, res.index, widgetIndex);
                break;
            case Ui::ScrollPart::vscrollbarTrackBottom:
                vTrackBottom(window, res.index, widgetIndex);
                break;
            default:
                break;
        }
    }

    // Based on 0x004C8689
    void scrollModalRight(const int16_t x, const int16_t y, Ui::Window& window, Ui::Widget* const widget, [[maybe_unused]] const WidgetIndex_t widgetIndex)
    {
        auto res = getPart(window, widget, x, y);

        _currentScrollArea = res.area;
        _currentScrollIndex = res.index;

        if (res.area == Ui::ScrollPart::view)
        {
            window.callScrollMouseDown(res.scrollviewLoc.x, res.scrollviewLoc.y, static_cast<uint8_t>(res.index));
        }
    }

    // 0x004C72ED
    void clearPressedButtons(const WindowType type, const WindowNumber_t number, const WidgetIndex_t widgetIndex)
    {
        auto window = WindowManager::find(type, number);
        if (window == nullptr)
        {
            return;
        }

        constexpr ScrollFlags horizontalFlags = ScrollFlags::hscrollbarThumbPressed | ScrollFlags::hscrollbarLeftPressed | ScrollFlags::hscrollbarRightPressed;
        constexpr ScrollFlags verticalFlags = ScrollFlags::vscrollbarThumbPressed | ScrollFlags::vscrollbarUpPressed | ScrollFlags::vscrollbarDownPressed;

        window->scrollAreas[_currentScrollIndex].flags &= ~(verticalFlags | horizontalFlags);
        WindowManager::invalidateWidget(type, number, widgetIndex);
    }

    // 0x004C7236
    void scrollLeftContinue(const int16_t x, const int16_t y, Ui::Window& window, Ui::Widget* const widget, const WidgetIndex_t widgetIndex)
    {
        if (_currentScrollArea == ScrollPart::hscrollbarThumb)
        {
            auto toolTipLoc = Ui::ToolTip::getTooltipMouseLocation();
            int16_t deltaX = x - toolTipLoc.x;
            toolTipLoc.x = x;
            Ui::ToolTip::setTooltipMouseLocation(toolTipLoc);
            ScrollView::horizontalFollow(window, widget, widgetIndex, _currentScrollIndex, deltaX);
        }
        else if (_currentScrollArea == ScrollPart::vscrollbarThumb)
        {
            auto toolTipLoc = Ui::ToolTip::getTooltipMouseLocation();
            int16_t deltaY = y - toolTipLoc.y;
            toolTipLoc.y = y;
            Ui::ToolTip::setTooltipMouseLocation(toolTipLoc);
            ScrollView::verticalFollow(window, widget, widgetIndex, _currentScrollIndex, deltaY);
        }
        else
        {
            auto res = getPart(window, widget, x, y);
            if (res.area != _currentScrollArea)
            {
                clearPressedButtons(window.type, window.number, widgetIndex);
                return;
            }

            switch (res.area)
            {
                case ScrollPart::view: // 0x004C729A
                    window.callScrollMouseDrag(res.scrollviewLoc.x, res.scrollviewLoc.y, static_cast<uint8_t>(res.index));
                    break;

                case ScrollPart::hscrollbarButtonLeft:
                    hButtonLeft(window, res.index, widgetIndex);
                    break;
                case ScrollPart::hscrollbarButtonRight:
                    hButtonRight(window, res.index, widgetIndex);
                    break;
                case ScrollPart::hscrollbarTrackLeft:
                case ScrollPart::hscrollbarTrackRight:
                    break;

                case ScrollPart::vscrollbarButtonTop:
                    vButtonTop(window, res.index, widgetIndex);
                    break;
                case ScrollPart::vscrollbarButtonBottom:
                    vButtonBottom(window, res.index, widgetIndex);
                    break;
                case ScrollPart::vscrollbarTrackTop:
                case ScrollPart::vscrollbarTrackBottom:
                    break;

                default:
                    break;
            }
        }
    }
}
