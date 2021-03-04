#include "../../CompanyManager.h"
#include "../../Graphics/ImageIds.h"
#include "../../Input.h"
#include "../../Localisation/FormatArguments.hpp"
#include "../../Objects/ObjectManager.h"
#include "../../Objects/RoadExtraObject.h"
#include "../../Objects/RoadObject.h"
#include "../../Objects/TrackExtraObject.h"
#include "../../Objects/TrackObject.h"
#include "../../Ptr.h"
#include "../../Ui/Dropdown.h"
#include "Construction.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Map;
using namespace OpenLoco::Map::TileManager;

namespace OpenLoco::Ui::Windows::Construction::Overhead
{
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;

    widget_t widgets[] = {
        commonWidgets(138, 192, StringIds::stringid_2),
        makeWidget({ 3, 45 }, { 132, 12 }, widget_type::checkbox, 1, StringIds::empty, StringIds::tooltip_select_track_mod),
        makeWidget({ 3, 57 }, { 132, 12 }, widget_type::checkbox, 1, StringIds::empty, StringIds::tooltip_select_track_mod),
        makeWidget({ 3, 69 }, { 132, 12 }, widget_type::checkbox, 1, StringIds::empty, StringIds::tooltip_select_track_mod),
        makeWidget({ 3, 81 }, { 132, 12 }, widget_type::checkbox, 1, StringIds::empty, StringIds::tooltip_select_track_mod),
        makeWidget({ 35, 110 }, { 66, 66 }, widget_type::wt_3, 1),
        makeWidget({ 3, 95 }, { 132, 12 }, widget_type::wt_18, 1, 0xFFFFFFFF, StringIds::tooltip_select_track_to_upgrade),
        makeWidget({ 123, 96 }, { 11, 10 }, widget_type::wt_11, 1, StringIds::dropdown, StringIds::tooltip_select_track_to_upgrade),
        widgetEnd(),
    };

    window_event_list events;

    // 0x0049EBD1
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

            case widx::checkbox_1:
            case widx::checkbox_2:
            case widx::checkbox_3:
            case widx::checkbox_4:
            {
                auto checkboxIndex = widgetIndex - widx::checkbox_1;

                _lastSelectedMods = _lastSelectedMods ^ (1 << checkboxIndex);

                // TODO: & ~(1 << 7) added to prevent crashing when selecting/deselecting overhead wires for trams
                _scenarioTrackMods[_trackType & ~(1 << 7)] = _lastSelectedMods;

                self->invalidate();
                break;
            }
        }
    }

    // 0x0049EBFC
    static void onMouseDown(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::track_dropdown:
            {
                uint8_t modCount = 3;

                auto widget = self->widgets[widx::track];
                auto xPos = widget.left + self->x;
                auto yPos = widget.top + self->y;
                auto width = widget.width() + 2;
                auto height = widget.height();

                Dropdown::show(xPos, yPos, width, height, self->colours[1], modCount, (1 << 7));

                Dropdown::add(0, StringIds::single_section);
                Dropdown::add(1, StringIds::block_section);
                Dropdown::add(2, StringIds::all_connected_track);

                Dropdown::setHighlightedItem(_lastSelectedTrackModSection);
                break;
            }

            case widx::image:
            {
                Input::toolCancel();
                Input::toolSet(self, widgetIndex, 12);
                break;
            }
        }
    }

    // 0x0049EC09
    static void onDropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
    {
        if (widgetIndex != widx::track_dropdown)
            return;

        if (itemIndex != -1)
        {
            _lastSelectedTrackModSection = itemIndex;
            self->invalidate();
        }
    }

    // 0x0049ECD1
    static void onUpdate(window* self)
    {
        Common::onUpdate(self, (1 << 5));
    }

    // 0x0049EC15
    static void onToolUpdate(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = ToInt(&self);
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049EC15, regs);
    }

    // 0x0049EC20
    static void onToolDown(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
    {
        registers regs;
        regs.esi = ToInt(&self);
        regs.dx = widgetIndex;
        regs.ax = x;
        regs.bx = y;
        call(0x0049EC20, regs);
    }

    static void setCheckbox(window* self, widget_index checkboxIndex, string_id name)
    {
        auto widgetIndex = checkboxIndex + widx::checkbox_1;
        self->widgets[widgetIndex].type = widget_type::checkbox;
        self->widgets[widgetIndex].text = name;

        if (_lastSelectedMods & (1 << checkboxIndex))
            self->activated_widgets |= (1ULL << widgetIndex);
    }

    // 0x0049E7D3
    static void prepareDraw(window* self)
    {
        Common::prepareDraw(self);

        self->activated_widgets &= ~(1 << widx::checkbox_1 | 1 << widx::checkbox_2 | 1 << widx::checkbox_3 | 1 << widx::checkbox_4);

        self->widgets[widx::checkbox_1].type = widget_type::none;
        self->widgets[widx::checkbox_2].type = widget_type::none;
        self->widgets[widx::checkbox_3].type = widget_type::none;
        self->widgets[widx::checkbox_4].type = widget_type::none;

        if (_trackType & (1 << 7))
        {
            auto trackType = _trackType & ~(1 << 7);
            auto roadObj = ObjectManager::get<road_object>(trackType);

            auto args = FormatArguments();
            args.push(roadObj->name);

            for (auto i = 0; i < 2; i++)
            {
                if (_modList[i] != 0xFF)
                {
                    auto extraName = ObjectManager::get<road_extra_object>(_modList[i])->name;
                    setCheckbox(self, i, extraName);
                }
            }
        }
        else
        {
            auto trackObj = ObjectManager::get<track_object>(_trackType);

            auto args = FormatArguments();
            args.push(trackObj->name);

            for (auto i = 0; i < 4; i++)
            {
                if (_modList[i] != 0xFF)
                {
                    auto extraName = ObjectManager::get<track_extra_object>(_modList[i])->name;
                    setCheckbox(self, i, extraName);
                }
            }
        }

        //self->activated_widgets = activatedWidgets;

        self->widgets[widx::image].type = widget_type::none;
        self->widgets[widx::track].type = widget_type::none;
        self->widgets[widx::track_dropdown].type = widget_type::none;

        self->widgets[widx::image].tooltip = StringIds::null;

        if (_lastSelectedMods & 0xF)
        {
            self->widgets[widx::image].type = widget_type::wt_3;
            self->widgets[widx::track].type = widget_type::wt_18;
            self->widgets[widx::track_dropdown].type = widget_type::wt_11;

            self->widgets[widx::image].tooltip = StringIds::upgrade_track_with_mods;

            if (isNetworkHost())
            {
                if (_toolWindowType == WindowType::construction)
                    self->widgets[widx::image].tooltip = StringIds::click_track_to_upgrade;
            }
        }

        static string_id modString[] = {
            StringIds::single_section,
            StringIds::block_section,
            StringIds::all_connected_track,
        };

        self->widgets[widx::track].text = modString[_lastSelectedTrackModSection];

        Common::repositionTabs(self);
    }

    // 0x0049EA3E
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        Common::drawTabs(self, dpi);
        if (_lastSelectedMods & 0xF)
        {
            Gfx::drawpixelinfo_t* clipped = nullptr;
            auto xPos = self->x + self->widgets[widx::image].left + 1;
            auto yPos = self->y + self->widgets[widx::image].top + 1;
            auto width = self->widgets[widx::image].width();
            auto height = self->widgets[widx::image].height();

            if (Gfx::clipDrawpixelinfo(&clipped, dpi, xPos, yPos, width, height))
            {
                coord_t x = 0x2010;
                coord_t y = 0x2010;

                auto rotCoord = rotate2dCoordinate({ x, y }, gCurrentRotation);
                Gfx::point_t screenPos = { static_cast<int16_t>(rotCoord.y - rotCoord.x), static_cast<int16_t>(((rotCoord.x + rotCoord.y) >> 1) - 460) };

                screenPos.x -= (self->widgets[widx::image].width() / 2);
                screenPos.y -= ((self->widgets[widx::image].width() / 2) + 16);
                clipped->x += screenPos.x;
                clipped->y += screenPos.y;

                _dword_E0C3E0 = clipped;

                x = 0x2000;
                y = 0x2000;

                auto company = CompanyManager::get(_playerCompany);
                auto companyColour = company->mainColours.primary;
                _byte_522095 = _byte_522095 | (1 << 0);

                if (_trackType & (1 << 7))
                {
                    uint8_t trackType = _trackType & ~(1 << 7);
                    Construction::drawRoad(x, y, _lastSelectedMods, 0x1D0, trackType, 0, companyColour, gCurrentRotation);
                }
                else
                {
                    Construction::drawTrack(x, y, _lastSelectedMods, 0x1D0, _trackType, 0, companyColour, gCurrentRotation);
                }
                _byte_522095 = _byte_522095 & ~(1 << 0);
            }
        }

        auto xPos = self->x + 69;
        auto yPos = self->widgets[widx::image].bottom + self->y + 4;

        if (_modCost != 0x80000000 && _modCost != 0)
        {
            auto args = FormatArguments();
            args.push<uint32_t>(_modCost);

            Gfx::drawStringCentred(*dpi, xPos, yPos, Colour::black, StringIds::build_cost, &args);
        }
    }

    void tabReset(window* self)
    {
        self->callOnMouseDown(Overhead::widx::image);
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
