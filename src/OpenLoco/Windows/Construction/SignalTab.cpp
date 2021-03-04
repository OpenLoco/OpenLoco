#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/TrackObject.h"
#include "../../Objects/TrainSignalObject.h"
#include "../../Ptr.h"
#include "../../Ui/Dropdown.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Signal
{
    widget_t widgets[] = {
        commonWidgets(138, 167, StringIds::stringid_2),
        makeWidget({ 3, 45 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, StringIds::tooltip_select_signal_type),
        makeWidget({ 123, 46 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown, StringIds::tooltip_select_signal_type),
        makeWidget({ 27, 110 }, { 40, 40 }, widget_type::wt_9, 1, 0xFFFFFFFF, StringIds::tooltip_signal_both_directions),
        makeWidget({ 71, 110 }, { 40, 40 }, widget_type::wt_9, 1, 0xFFFFFFFF, StringIds::tooltip_signal_single_direction),
        widgetEnd(),
    };

    window_event_list events;

    // 0x0049E64E
    static void onMouseUp(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case Common::widx::close_button:
                WindowManager::close(self);
                break;

            case Common::widx::tab_construction:
            case Common::widx::tab_overhead:
            case Common::widx::tab_signal:
            case Common::widx::tab_station:
                Common::switchTab(self, widgetIndex);
                break;
        }
    }

    // 0x0049E669
    static void onMouseDown(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::signal_dropdown:
            {
                uint8_t signalCount = 0;
                while (_signalList[signalCount] != 0xFF)
                    signalCount++;

                auto widget = self->widgets[widx::signal];
                auto xPos = widget.left + self->x;
                auto yPos = widget.top + self->y;
                auto width = widget.width() + 2;
                auto height = widget.height();

                Dropdown::show(xPos, yPos, width, height, self->colours[1], signalCount, (1 << 7));

                for (auto signalIndex = 0; signalIndex < signalCount; signalIndex++)
                {
                    auto signal = _signalList[signalIndex];
                    if (signal == _lastSelectedSignal)
                        Dropdown::setHighlightedItem(signalIndex);

                    auto trainSignalObj = ObjectManager::get<train_signal_object>(signal);

                    Dropdown::add(signalIndex, trainSignalObj->name);
                }
                break;
            }

            case widx::both_directions:
            {
                _isSignalBothDirections = 1;
                Input::toolCancel();
                Input::toolSet(self, widgetIndex, 42);
                break;
            }

            case widx::single_direction:
            {
                _isSignalBothDirections = 0;
                Input::toolCancel();
                Input::toolSet(self, widgetIndex, 42);
                break;
            }
        }
    }

    // 0x0049E67C
    static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex != widx::signal_dropdown)
            return;

        if (itemIndex != -1)
        {
            _lastSelectedSignal = _signalList[itemIndex];
            _scenarioSignals[_trackType] = _signalList[itemIndex];
            self->invalidate();
        }
    }

    // 0x0049E76F
    static void onUpdate(window* self)
    {
        Common::onUpdate(self, (1 << 2));
    }

    // 0x0049E745
    static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = ToInt(&self);
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E745, regs);
    }

    // 0x0049E75A
    static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = ToInt(&self);
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E75A, regs);
    }

    // 0x0049E499
    static void prepareDraw(window* self)
    {
        Common::prepareDraw(self);

        auto trackObj = ObjectManager::get<track_object>(_trackType);

        auto args = FormatArguments();
        args.push(trackObj->name);

        auto trainSignalObject = ObjectManager::get<train_signal_object>(_lastSelectedSignal);

        self->widgets[widx::signal].text = trainSignalObject->name;

        Common::repositionTabs(self);
    }

    // 0x0049E501
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        Common::drawTabs(self, dpi);

        auto trainSignalObject = ObjectManager::get<train_signal_object>(_lastSelectedSignal);

        auto xPos = self->x + 3;
        auto yPos = self->y + 63;
        auto width = 130;

        {
            auto args = FormatArguments();
            args.push(trainSignalObject->var_0C);

            Gfx::drawString_495224(*dpi, xPos, yPos, width, Colour::black, StringIds::signal_black, &args);
        }

        auto imageId = trainSignalObject->image;

        xPos = self->widgets[widx::both_directions].mid_x() + self->x;
        yPos = self->widgets[widx::both_directions].bottom + self->y - 4;

        Gfx::drawImage(dpi, xPos - 8, yPos, imageId);

        Gfx::drawImage(dpi, xPos + 8, yPos, imageId + 4);

        xPos = self->widgets[widx::single_direction].mid_x() + self->x;
        yPos = self->widgets[widx::single_direction].bottom + self->y - 4;

        Gfx::drawImage(dpi, xPos, yPos, imageId);

        if (_signalCost != 0x80000000 && _signalCost != 0)
        {
            auto args = FormatArguments();
            args.push<uint32_t>(_signalCost);

            xPos = self->x + 69;
            yPos = self->widgets[widx::single_direction].bottom + self->y + 5;

            Gfx::drawStringCentred(*dpi, xPos, yPos, Colour::black, StringIds::build_cost, &args);
        }
    }

    void tabReset(window* self)
    {
        self->callOnMouseDown(Signal::widx::both_directions);
    }

    void initEvents()
    {
        events.on_close = Common::onClose;
        events.on_mouse_up = onMouseUp;
        events.on_mouse_down = onMouseDown;
        events.on_dropdown = onDropdown;
        events.on_update = onUpdate;
        events.on_tool_update = onToolUpdate;
        events.on_tool_down = onToolDown;
        events.prepare_draw = prepareDraw;
        events.draw = draw;
    }
}
