#include "../../graphics/image_ids.h"
#include "../../input.h"
#include "../../localisation/FormatArguments.hpp"
#include "../../objects/objectmgr.h"
#include "../../objects/track_object.h"
#include "../../objects/train_signal_object.h"
#include "../../ui/dropdown.h"
#include "Construction.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction::signal
{
    widget_t widgets[] = {
        commonWidgets(138, 167, string_ids::stringid_2),
        make_widget({ 3, 45 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, string_ids::tooltip_select_signal_type),
        make_widget({ 123, 46 }, { 11, 10 }, widget_type::wt_11, 1, string_ids::dropdown, string_ids::tooltip_select_signal_type),
        make_widget({ 27, 110 }, { 40, 40 }, widget_type::wt_9, 1, 0xFFFFFFFF, string_ids::tooltip_signal_both_directions),
        make_widget({ 71, 110 }, { 40, 40 }, widget_type::wt_9, 1, 0xFFFFFFFF, string_ids::tooltip_signal_single_direction),
        widget_end(),
    };

    window_event_list events;

    // 0x0049E64E
    static void on_mouse_up(window* self, widget_index widgetIndex)
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
    static void on_mouse_down(window* self, widget_index widgetIndex)
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
                        dropdown::set_highlighted_item(signalIndex);

                    auto trainSignalObj = objectmgr::get<train_signal_object>(signal);

                    dropdown::add(signalIndex, trainSignalObj->name);
                }
                break;
            }

            case widx::both_directions:
            {
                _isSignalBothDirections = 1;
                input::cancel_tool();
                input::toolSet(self, widgetIndex, 42);
                break;
            }

            case widx::single_direction:
            {
                _isSignalBothDirections = 0;
                input::cancel_tool();
                input::toolSet(self, widgetIndex, 42);
                break;
            }
        }
    }

    // 0x0049E67C
    static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
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
    static void on_update(window* self)
    {
        common::on_update(self, (1 << 2));
    }

    // 0x0049E745
    static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E745, regs);
    }

    // 0x0049E75A
    static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = (int32_t)&self;
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049E75A, regs);
    }

    // 0x0049E499
    static void prepare_draw(window* self)
    {
        common::prepare_draw(self);

        auto trackObj = objectmgr::get<track_object>(_trackType);

        auto args = FormatArguments();
        args.push(trackObj->name);

        auto trainSignalObject = objectmgr::get<train_signal_object>(_lastSelectedSignal);

        self->widgets[widx::signal].text = trainSignalObject->name;

        common::repositionTabs(self);
    }

    // 0x0049E501
    static void draw(window* self, gfx::drawpixelinfo_t* dpi)
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

            gfx::draw_string_495224(*dpi, xPos, yPos, width, colour::black, string_ids::signal_black, &args);
        }

        auto imageId = trainSignalObject->var_0E;

        xPos = self->widgets[widx::both_directions].mid_x() + self->x;
        yPos = self->widgets[widx::both_directions].bottom + self->y - 4;

        gfx::draw_image(dpi, xPos - 8, yPos, imageId);

        gfx::draw_image(dpi, xPos + 8, yPos, imageId + 4);

        xPos = self->widgets[widx::single_direction].mid_x() + self->x;
        yPos = self->widgets[widx::single_direction].bottom + self->y - 4;

        gfx::draw_image(dpi, xPos, yPos, imageId);

        if (_signalCost != 0x80000000 && _signalCost != 0)
        {
            auto args = FormatArguments();
            args.push<uint32_t>(_signalCost);

            xPos = self->x + 69;
            yPos = self->widgets[widx::single_direction].bottom + self->y + 5;

            gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::build_cost, &args);
        }
    }

    void tabReset(window* self)
    {
        self->call_on_mouse_down(signal::widx::both_directions);
    }

    void init_events()
    {
        events.on_close = common::on_close;
        events.on_mouse_up = on_mouse_up;
        events.on_mouse_down = on_mouse_down;
        events.on_dropdown = on_dropdown;
        events.on_update = on_update;
        events.on_tool_update = on_tool_update;
        events.on_tool_down = on_tool_down;
        events.prepare_draw = prepare_draw;
        events.draw = draw;
    }
}
