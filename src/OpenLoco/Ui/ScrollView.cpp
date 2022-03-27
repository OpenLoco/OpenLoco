#include "ScrollView.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Ui.h"
#include "../Widget.h"
#include "WindowManager.h"
#include <cmath>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ScrollView
{
    static loco_global<Ui::ScrollView::ScrollPart, 0x00523396> _currentScrollArea;
    // TODO: Convert to a scrollIndex when all scroll functions implemented
    static loco_global<uint32_t, 0x00523398> _currentScrollOffset;
    static void setCurrentScrollIndex(size_t index)
    {
        _currentScrollOffset = index * sizeof(Ui::ScrollArea);
    }
    static size_t getCurrentScrollIndex()
    {
        return _currentScrollOffset / sizeof(Ui::ScrollArea);
    }

    // 0x004C87E1
    // regs.bp: deltaX
    static void horizontalFollow(Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex, const size_t scrollIndex, const int16_t deltaX)
    {
        ScrollArea& scrollArea = w->scrollAreas[scrollIndex];
        scrollArea.flags |= ScrollFlags::hscrollbarThumbPressed;

        uint16_t trackWidth = widget->width() - 2 - thumbSize - thumbSize;
        if (scrollArea.flags & ScrollFlags::vscrollbarVisible)
        {
            trackWidth -= barWidth;
        }

        if (trackWidth == 0)
        {
            return;
        }
        auto contentDeltaX = deltaX * scrollArea.contentWidth / trackWidth;

        int16_t newOffset = scrollArea.contentOffsetX + contentDeltaX;

        int frameWidth = widget->width() - 2;
        if (scrollArea.flags & ScrollFlags::vscrollbarVisible)
        {
            frameWidth -= barWidth;
        }

        int16_t maxOffset = scrollArea.contentWidth - frameWidth;
        maxOffset = std::max<int16_t>(maxOffset, 0);

        scrollArea.contentOffsetX = std::clamp<int16_t>(newOffset, 0, maxOffset);

        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8898
    // regs.bp: deltaY
    static void verticalFollow(Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex, const size_t scrollIndex, const int16_t deltaY)
    {
        ScrollArea& scrollArea = w->scrollAreas[scrollIndex];
        scrollArea.flags |= ScrollFlags::vscrollbarThumbPressed;

        uint16_t trackHeight = widget->height() - 2 - thumbSize - thumbSize;
        if ((scrollArea.flags & ScrollFlags::hscrollbarVisible) != 0)
        {
            trackHeight -= barWidth;
        }

        if (trackHeight == 0)
        {
            return;
        }
        auto contentDeltaY = deltaY * scrollArea.contentHeight / trackHeight;

        int16_t newOffset = scrollArea.contentOffsetY + contentDeltaY;

        int frameHeight = widget->height() - 2;
        if ((scrollArea.flags & ScrollFlags::hscrollbarVisible) != 0)
        {
            frameHeight -= barWidth;
        }

        int16_t maxOffset = scrollArea.contentHeight - frameHeight;
        maxOffset = std::max<int16_t>(maxOffset, 0);

        scrollArea.contentOffsetY = std::clamp<int16_t>(newOffset, 0, maxOffset);

        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8CFD
    // regs.bp: deltaX
    void horizontalDragFollow(Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaX)
    {
        ScrollArea& scrollArea = w->scrollAreas[dragScrollIndex];
        if ((scrollArea.flags & ScrollFlags::hscrollbarVisible) == 0)
        {
            return;
        }

        uint16_t trackWidth = widget->width() - 2 - thumbSize - thumbSize;
        if (scrollArea.flags & ScrollFlags::vscrollbarVisible)
        {
            trackWidth -= barWidth;
        }

        if (trackWidth == 0)
        {
            return;
        }
        auto contentDeltaX = deltaX * scrollArea.contentWidth / trackWidth;

        int16_t newOffset = scrollArea.contentOffsetX + contentDeltaX;

        int frameWidth = widget->width() - 2;
        if (scrollArea.flags & ScrollFlags::vscrollbarVisible)
        {
            frameWidth -= barWidth;
        }

        int16_t maxOffset = scrollArea.contentWidth - frameWidth;
        maxOffset = std::max<int16_t>(maxOffset, 0);

        scrollArea.contentOffsetX = std::clamp<int16_t>(newOffset, 0, maxOffset);

        ScrollView::updateThumbs(w, dragWidgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, dragWidgetIndex);
    }

    // 0x004C8E2E
    // regs.bp: deltaY
    // possible extraction of common functionality in verticalDragFollow, horizontalDragFollow, verticalFollow, horizontalFollow
    void verticalDragFollow(Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t dragWidgetIndex, const size_t dragScrollIndex, const int16_t deltaY)
    {
        ScrollArea& scrollArea = w->scrollAreas[dragScrollIndex];
        if ((scrollArea.flags & ScrollFlags::vscrollbarVisible) == 0)
        {
            return;
        }

        uint16_t trackHeight = widget->height() - 2 - thumbSize - thumbSize;
        if ((scrollArea.flags & ScrollFlags::hscrollbarVisible) != 0)
        {
            trackHeight -= barWidth;
        }

        if (trackHeight == 0)
        {
            return;
        }
        auto contentDeltaY = deltaY * scrollArea.contentHeight / trackHeight;

        int16_t newOffset = scrollArea.contentOffsetY + contentDeltaY;

        int frameHeight = widget->height() - 2;
        if ((scrollArea.flags & ScrollFlags::hscrollbarVisible) != 0)
        {
            frameHeight -= barWidth;
        }

        int16_t maxOffset = scrollArea.contentHeight - frameHeight;
        maxOffset = std::max<int16_t>(maxOffset, 0);

        scrollArea.contentOffsetY = std::clamp<int16_t>(newOffset, 0, maxOffset);

        ScrollView::updateThumbs(w, dragWidgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, dragWidgetIndex);
    }

    // 0x004C8EF0
    // Note: Original function returns a scrollAreaOffset not an index
    GetPartResult getPart(Ui::Window* window, Ui::Widget* widget, int16_t x, int16_t y)
    {
        GetPartResult res{ { x, y }, ScrollPart::none, 0 };

        for (const auto* winWidget = window->widgets; winWidget != widget; winWidget++)
        {
            if (winWidget->type == WidgetType::scrollview)
            {
                res.index++;
            }
        }

        res.index = std::min<size_t>(res.index, Window::kMaxScrollAreas - 1);

        const auto& scroll = window->scrollAreas[res.index];
        auto right = widget->right + window->x;
        auto left = widget->left + window->x;
        auto top = widget->top + window->y;
        auto bottom = widget->bottom + window->y;

        if ((scroll.flags & ScrollFlags::hscrollbarVisible)
            && y >= (bottom - barWidth))
        {
            if (x < left + barWidth)
            {
                res.area = ScrollPart::hscrollbarButtonLeft;
                return res;
            }

            // If vertical is also visible then there is a deadzone in the corner
            if (scroll.flags & ScrollFlags::vscrollbarVisible)
            {
                right -= barWidth;
            }

            // Within deadzone
            if (x >= right)
            {
                res.area = ScrollPart::none;
                return res;
            }

            if (x >= right - thumbSize)
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
        else if ((scroll.flags & ScrollFlags::vscrollbarVisible) && x >= (right - barWidth))
        {
            if (y < top + barWidth)
            {
                res.area = ScrollPart::vscrollbarButtonTop;
                return res;
            }

            // If horizontal is also visible then there is a deadzone in the corner
            if (scroll.flags & ScrollFlags::hscrollbarVisible)
            {
                bottom -= barWidth;
            }

            // Within deadzone
            if (y >= bottom)
            {
                res.area = ScrollPart::none;
                return res;
            }

            if (y >= bottom - thumbSize)
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

    // 0x004CA1ED
    void updateThumbs(Window* window, WidgetIndex_t widgetIndex)
    {
        registers regs;

        regs.esi = X86Pointer(window);
        regs.ebx = window->getScrollDataIndex(widgetIndex) * sizeof(ScrollArea);
        regs.edi = X86Pointer(&window->widgets[widgetIndex]);
        call(0x4CA1ED, regs);
    }

    // 0x004C894F
    static void hButtonLeft(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        w->scrollAreas[scrollAreaIndex].flags |= ScrollFlags::hscrollbarLeftPressed;
        w->scrollAreas[scrollAreaIndex].contentOffsetX = std::max(w->scrollAreas[scrollAreaIndex].contentOffsetX - buttonClickStep, 0);
        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C89AE
    static void hButtonRight(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        w->scrollAreas[scrollAreaIndex].flags |= ScrollFlags::hscrollbarRightPressed;
        int16_t trackWidth = w->widgets[widgetIndex].width() - 2;
        if ((w->scrollAreas[scrollAreaIndex].flags & ScrollFlags::vscrollbarVisible) != 0)
        {
            trackWidth -= barWidth;
        }
        int16_t widgetContentWidth = std::max(w->scrollAreas[scrollAreaIndex].contentWidth - trackWidth, 0);
        w->scrollAreas[scrollAreaIndex].contentOffsetX = std::min<int16_t>(w->scrollAreas[scrollAreaIndex].contentOffsetX + buttonClickStep, widgetContentWidth);
        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8A36
    static void hTrackLeft(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackWidth = w->widgets[widgetIndex].width() - 2;
        if ((w->scrollAreas[scrollAreaIndex].flags & ScrollFlags::vscrollbarVisible) != 0)
        {
            trackWidth -= barWidth;
        }
        w->scrollAreas[scrollAreaIndex].contentOffsetX = std::max(w->scrollAreas[scrollAreaIndex].contentOffsetX - trackWidth, 0);
        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8AA6
    static void hTrackRight(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackWidth = w->widgets[widgetIndex].width() - 2;
        if ((w->scrollAreas[scrollAreaIndex].flags & ScrollFlags::vscrollbarVisible) != 0)
        {
            trackWidth -= barWidth;
        }
        int16_t widgetContentWidth = std::max(w->scrollAreas[scrollAreaIndex].contentWidth - trackWidth, 0);
        w->scrollAreas[scrollAreaIndex].contentOffsetX = std::min<int16_t>(w->scrollAreas[scrollAreaIndex].contentOffsetX + trackWidth, widgetContentWidth);
        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    void verticalNudgeUp(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        w->scrollAreas[scrollAreaIndex].contentOffsetY = std::max(w->scrollAreas[scrollAreaIndex].contentOffsetY - buttonClickStep, 0);
        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8B26
    static void vButtonTop(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        w->scrollAreas[scrollAreaIndex].flags |= ScrollFlags::vscrollbarUpPressed;
        verticalNudgeUp(w, scrollAreaIndex, widgetIndex);
    }

    void verticalNudgeDown(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackHeight = w->widgets[widgetIndex].height() - 2;
        if ((w->scrollAreas[scrollAreaIndex].flags & ScrollFlags::hscrollbarVisible) != 0)
        {
            trackHeight -= barWidth;
        }
        int16_t widgetContentHeight = std::max(w->scrollAreas[scrollAreaIndex].contentHeight - trackHeight, 0);
        w->scrollAreas[scrollAreaIndex].contentOffsetY = std::min<int16_t>(w->scrollAreas[scrollAreaIndex].contentOffsetY + buttonClickStep, widgetContentHeight);
        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8B85
    static void vButtonBottom(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        w->scrollAreas[scrollAreaIndex].flags |= ScrollFlags::vscrollbarDownPressed;
        verticalNudgeDown(w, scrollAreaIndex, widgetIndex);
    }

    // 0x004C8C0D
    static void vTrackTop(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackHeight = w->widgets[widgetIndex].height() - 2;
        if ((w->scrollAreas[scrollAreaIndex].flags & ScrollFlags::hscrollbarVisible) != 0)
        {
            trackHeight -= barWidth;
        }
        w->scrollAreas[scrollAreaIndex].contentOffsetY = std::max(w->scrollAreas[scrollAreaIndex].contentOffsetY - trackHeight, 0);
        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8C7D
    static void vTrackBottom(Ui::Window* const w, const size_t scrollAreaIndex, const WidgetIndex_t widgetIndex)
    {
        int16_t trackHeight = w->widgets[widgetIndex].height() - 2;
        if ((w->scrollAreas[scrollAreaIndex].flags & ScrollFlags::hscrollbarVisible) != 0)
        {
            trackHeight -= barWidth;
        }
        int16_t widgetContentHeight = std::max(w->scrollAreas[scrollAreaIndex].contentHeight - trackHeight, 0);
        w->scrollAreas[scrollAreaIndex].contentOffsetY = std::min<int16_t>(w->scrollAreas[scrollAreaIndex].contentOffsetY + trackHeight, widgetContentHeight);
        ScrollView::updateThumbs(w, widgetIndex);
        WindowManager::invalidateWidget(w->type, w->number, widgetIndex);
    }

    // 0x004C8689
    void scrollLeftBegin(const int16_t x, const int16_t y, Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex)
    {
        auto res = getPart(w, widget, x, y);

        _currentScrollArea = res.area;
        setCurrentScrollIndex(res.index);

        // Not implemented for any window
        // window->call_22()
        switch (res.area)
        {
            case Ui::ScrollView::ScrollPart::view:
                w->callScrollMouseDown(res.scrollviewLoc.x, res.scrollviewLoc.y, static_cast<uint8_t>(res.index));
                break;
            case Ui::ScrollView::ScrollPart::hscrollbarButtonLeft:
                hButtonLeft(w, res.index, widgetIndex);
                break;
            case Ui::ScrollView::ScrollPart::hscrollbarButtonRight:
                hButtonRight(w, res.index, widgetIndex);
                break;
            case Ui::ScrollView::ScrollPart::hscrollbarTrackLeft:
                hTrackLeft(w, res.index, widgetIndex);
                break;
            case Ui::ScrollView::ScrollPart::hscrollbarTrackRight:
                hTrackRight(w, res.index, widgetIndex);
                break;
            case Ui::ScrollView::ScrollPart::vscrollbarButtonTop:
                vButtonTop(w, res.index, widgetIndex);
                break;
            case Ui::ScrollView::ScrollPart::vscrollbarButtonBottom:
                vButtonBottom(w, res.index, widgetIndex);
                break;
            case Ui::ScrollView::ScrollPart::vscrollbarTrackTop:
                vTrackTop(w, res.index, widgetIndex);
                break;
            case Ui::ScrollView::ScrollPart::vscrollbarTrackBottom:
                vTrackBottom(w, res.index, widgetIndex);
                break;
            default:
                break;
        }
    }

    // Based on 0x004C8689
    void scrollModalRight(const int16_t x, const int16_t y, Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex)
    {
        auto res = getPart(w, widget, x, y);

        _currentScrollArea = res.area;
        setCurrentScrollIndex(res.index);

        if (res.area == Ui::ScrollView::ScrollPart::view)
        {
            w->callScrollMouseDown(res.scrollviewLoc.x, res.scrollviewLoc.y, static_cast<uint8_t>(res.index));
        }
    }

    // 0x004C72ED
    void clearPressedButtons(const WindowType type, const WindowNumber_t number, const WidgetIndex_t widgetIndex)
    {
        auto window = WindowManager::find(type, number);
        if (window == nullptr)
            return;

        auto scrollAreaIndex = getCurrentScrollIndex();

        constexpr uint16_t horizontalFlags = ScrollFlags::hscrollbarThumbPressed | ScrollFlags::hscrollbarLeftPressed | ScrollFlags::hscrollbarRightPressed;
        constexpr uint16_t verticalFlags = ScrollFlags::vscrollbarThumbPressed | ScrollFlags::vscrollbarUpPressed | ScrollFlags::vscrollbarDownPressed;

        window->scrollAreas[scrollAreaIndex].flags &= ~(verticalFlags | horizontalFlags);
        WindowManager::invalidateWidget(type, number, widgetIndex);
    }

    // 0x004C7236
    void scrollLeftContinue(const int16_t x, const int16_t y, Ui::Window* const w, Ui::Widget* const widget, const WidgetIndex_t widgetIndex)
    {
        auto scrollIndex = getCurrentScrollIndex();
        if (_currentScrollArea == ScrollView::ScrollPart::hscrollbarThumb)
        {
            auto toolTipLoc = Input::getTooltipMouseLocation();
            int16_t deltaX = x - toolTipLoc.x;
            toolTipLoc.x = x;
            Input::setTooltipMouseLocation(toolTipLoc);
            ScrollView::horizontalFollow(w, widget, widgetIndex, scrollIndex, deltaX);
        }
        else if (_currentScrollArea == ScrollView::ScrollPart::vscrollbarThumb)
        {
            auto toolTipLoc = Input::getTooltipMouseLocation();
            int16_t deltaY = y - toolTipLoc.y;
            toolTipLoc.y = y;
            Input::setTooltipMouseLocation(toolTipLoc);
            ScrollView::verticalFollow(w, widget, widgetIndex, scrollIndex, deltaY);
        }
        else
        {
            auto res = getPart(w, widget, x, y);
            if (res.area != _currentScrollArea)
            {
                clearPressedButtons(w->type, w->number, widgetIndex);
                return;
            }

            switch (res.area)
            {
                case ScrollPart::view: // 0x004C729A
                    w->callScrollMouseDrag(res.scrollviewLoc.x, res.scrollviewLoc.y, static_cast<uint8_t>(res.index));
                    break;

                case ScrollPart::hscrollbarButtonLeft:
                    hButtonLeft(w, res.index, widgetIndex);
                    break;
                case ScrollPart::hscrollbarButtonRight:
                    hButtonRight(w, res.index, widgetIndex);
                    break;
                case ScrollPart::hscrollbarTrackLeft:
                case ScrollPart::hscrollbarTrackRight:
                    break;

                case ScrollPart::vscrollbarButtonTop:
                    vButtonTop(w, res.index, widgetIndex);
                    break;
                case ScrollPart::vscrollbarButtonBottom:
                    vButtonBottom(w, res.index, widgetIndex);
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
