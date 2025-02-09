#include "Widget.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Ui/ScrollView.h"
#include "Window.h"
#include <OpenLoco/Interop/Interop.hpp>
#include <cassert>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui
{
    int16_t Widget::midX() const
    {
        return (this->left + this->right) / 2;
    }

    int16_t Widget::midY() const
    {
        return (this->top + this->bottom) / 2;
    }

    uint16_t Widget::width() const
    {
        return (this->right - this->left) + 1;
    }

    uint16_t Widget::height() const
    {
        return (this->bottom - this->top) + 1;
    }

    void Widget::draw(
        Gfx::DrawingContext& drawingCtx,
        Window* window,
        const uint64_t pressedWidgets,
        const uint64_t toolWidgets,
        const uint64_t hoveredWidgets,
        uint8_t scrollviewIndex)
    {
        if (hidden || type == WidgetType::empty)
        {
            // Nothing to do here.
            return;
        }

        const auto& rt = drawingCtx.currentRenderTarget();

        if (!window->hasFlags(WindowFlags::noBackground))
        {
            // Check if widget is outside the draw region
            if (window->x + left >= rt.x + rt.width && window->x + right < rt.x)
            {
                if (window->y + top >= rt.y + rt.height && window->y + bottom < rt.y)
                {
                    return;
                }
            }
        }

        Gfx::RectInsetFlags widgetFlags = Gfx::RectInsetFlags::none;
        if (windowColour == WindowColour::primary && window->hasFlags(WindowFlags::flag_11))
        {
            widgetFlags = Gfx::RectInsetFlags::colourLight;
        }

        const auto widgetIndex = this - &window->widgets[0];
        WidgetState widgetState{};

        widgetState.window = window;
        widgetState.flags = widgetFlags;
        widgetState.colour = window->getColour(windowColour);

        widgetState.enabled = enabled | ((window->enabledWidgets & (1ULL << widgetIndex)) != 0);
        widgetState.disabled = disabled | ((window->disabledWidgets & (1ULL << widgetIndex)) != 0);
        widgetState.activated = activated | ((window->activatedWidgets & (1ULL << widgetIndex)) != 0);
        widgetState.activated |= (pressedWidgets & (1ULL << widgetIndex)) != 0;
        widgetState.activated |= (toolWidgets & (1ULL << widgetIndex)) != 0;

        widgetState.hovered = (hoveredWidgets & (1ULL << widgetIndex)) != 0;
        widgetState.scrollviewIndex = scrollviewIndex;

        // With the only exception of WidgetType::empty everything else should implement it.
        assert(events.draw != nullptr);

        if (events.draw != nullptr)
        {
            events.draw(drawingCtx, *this, widgetState);
            return;
        }
    }

    // 0x004CF194
    void Widget::drawTab(Window* w, Gfx::DrawingContext& drawingCtx, uint32_t imageId, WidgetIndex_t index)
    {
        auto widget = &w->widgets[index];

        Ui::Point pos = {};
        pos.x = widget->left + w->x;
        pos.y = widget->top + w->y;

        if (w->isDisabled(index))
        {
            return; // 0x8000
        }

        bool isActivated = false;
        if (w->isActivated(index))
        {
            isActivated = true;
        }
        else if (Input::state() == Input::State::widgetPressed)
        {
            isActivated = Input::isPressed(w->type, w->number, index);
        }

        if (imageId == kContentNull)
        {
            return;
        }

        if (isActivated)
        {
            if (imageId != kContentUnk)
            {
                drawingCtx.drawImage(pos.x, pos.y, imageId);
            }
        }
        else
        {
            if (imageId != kContentUnk)
            {
                drawingCtx.drawImage(pos.x, pos.y + 1, imageId);
            }

            drawingCtx.drawImage(pos.x, pos.y, Gfx::recolourTranslucent(ImageIds::tab, ExtColour::unk33));
            drawingCtx.drawRect(pos.x, pos.y + 26, 31, 1, Colours::getShade(w->getColour(WindowColour::secondary).c(), 7), Gfx::RectFlags::none);
        }
    }

    void Widget::leftAlignTabs(Window& window, uint8_t firstTabIndex, uint8_t lastTabIndex, uint16_t tabWidth)
    {
        auto xPos = window.widgets[firstTabIndex].left;
        for (auto i = firstTabIndex; i <= lastTabIndex; i++)
        {
            if (window.isDisabled(i))
            {
                window.widgets[i].hidden = true;
            }

            else
            {
                window.widgets[i].hidden = false;
                window.widgets[i].left = xPos;
                window.widgets[i].right = xPos + tabWidth;
                xPos = window.widgets[i].right + 1;
            }
        }
    }
}
