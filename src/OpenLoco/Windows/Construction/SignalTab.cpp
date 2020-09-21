#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/TrackObject.h"
#include "../../Objects/TrainSignalObject.h"
#include "../../Ui/Dropdown.h"
#include "Construction.h"

using namespace OpenLoco::interop;
using namespace OpenLoco::map;
using namespace OpenLoco::map::tilemgr;

namespace OpenLoco::ui::windows::construction::signal
{
    widget_t widgets[] = {
        commonWidgets(138, 167, string_ids::stringid_2),
        makeWidget({ 3, 45 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, string_ids::tooltip_select_signal_type),
        makeWidget({ 123, 46 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::tooltip_select_signal_type),
        makeWidget({ 27, 110 }, { 40, 40 }, widget_type::wt_9, 1, 0xFFFFFFFF, string_ids::tooltip_signal_both_directions),
        makeWidget({ 71, 110 }, { 40, 40 }, widget_type::wt_9, 1, 0xFFFFFFFF, string_ids::tooltip_signal_single_direction),
        widgetEnd(),
    };

    window_event_list events;

    // 0x0049E64E
    static void onMouseUp(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case common::widx::close_button:
                WindowManager::close(self);
                break;

            case common::widx::tab_construction:
            case common::widx::tab_overhead:
            case common::widx::tab_signal:
            case common::widx::tab_station:
                common::switchTab(self, widgetIndex);
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

                dropdown::show(xPos, yPos, width, height, self->colours[1], signalCount, (1 << 7));

                for (auto signalIndex = 0; signalIndex < signalCount; signalIndex++)
                {
                    auto signal = _signalList[signalIndex];
                    if (signal == _lastSelectedSignal)
                        dropdown::setHighlightedItem(signalIndex);

                    auto trainSignalObj = objectmgr::get<train_signal_object>(signal);

                    dropdown::add(signalIndex, trainSignalObj->name);
                }
                break;
            }

            case widx::both_directions:
            {
                _isSignalBothDirections = 1;
                input::toolCancel();
                input::toolSet(self, widgetIndex, 42);
                break;
            }

            case widx::single_direction:
            {
                _isSignalBothDirections = 0;
                input::toolCancel();
                input::toolSet(self, widgetIndex, 42);
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
        common::onUpdate(self, (1 << 2));
    }

    // 0x0049E745
    static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E745, regs);
    }

    // 0x0049E75A
    static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E75A, regs);
    }

    // 0x0049E499
    static void prepareDraw(window* self)
    {
        common::prepareDraw(self);

        auto trackObj = objectmgr::get<track_object>(_trackType);

        auto args = FormatArguments();
        args.push(trackObj->name);

        auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);

        self->widgets[widx::signal].text = trainSignalObject->name;

        common::repositionTabs(self);
    }

    // 0x0049E501
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        common::drawTabs(self, dpi);

        auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);

        auto xPos = self->x + 3;
        auto yPos = self->y + 63;
        auto width = 130;

        {
            auto args = FormatArguments();
            args.push(trainSignalObject->var_0C);

            Gfx::drawString_495224(*dpi, xPos, yPos, width, Colour::black, string_ids::signal_black, &args);
        }

        auto imageId = trainSignalObject->var_0E;

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

            Gfx::drawStringCentred(*dpi, xPos, yPos, Colour::black, string_ids::build_cost, &args);
        }
    }

    void tabReset(window* self)
    {
        self->callOnMouseDown(signal::widx::both_directions);
    }

    void initEvents()
    {
        events.on_close = common::onClose;
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
