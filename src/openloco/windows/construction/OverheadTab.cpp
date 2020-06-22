#include "Construction.h"

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::map::tilemgr;

namespace openloco::ui::windows::construction
{
    namespace overhead
    {
        // 0x0049EBD1
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

                case widx::checkbox_1:
                case widx::checkbox_2:
                case widx::checkbox_3:
                case widx::checkbox_4:
                {
                    auto checkboxIndex = widgetIndex - 8;

                    if (_lastSelectedMods & 1 << checkboxIndex)
                        _lastSelectedMods = _lastSelectedMods & ~(1 << checkboxIndex);
                    else
                        _lastSelectedMods = _lastSelectedMods | (1 << checkboxIndex);

                    // TODO: & ~(1 << 7) added to prevent crashing when selecting/deselecting overhead wires for trams
                    _scenarioTrackMods[_trackType & ~(1 << 7)] = _lastSelectedMods;

                    self->invalidate();
                    break;
                }
            }
        }

        // 0x0049EBFC
        static void on_mouse_down(window* self, widget_index widgetIndex)
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

                    dropdown::show(xPos, yPos, width, height, self->colours[1], modCount, 0x80);

                    dropdown::add(0, string_ids::single_section);
                    dropdown::add(1, string_ids::block_section);
                    dropdown::add(2, string_ids::all_connected_track);

                    dropdown::set_highlighted_item(_lastSelectedTrackModSection);
                    break;
                }

                case widx::image:
                {
                    input::cancel_tool();
                    input::toolSet(self, widgetIndex, 12);
                    break;
                }
            }
        }

        // 0x0049EC09
        static void on_dropdown(window* self, widget_index widgetIndex, int16_t itemIndex)
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
        static void on_update(window* self)
        {
            common::on_update(self, (1 << 5));
        }

        // 0x0049EC15
        static void on_tool_update(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049EC15, regs);
        }

        // 0x0049EC20
        static void on_tool_down(window& self, const widget_index widgetIndex, const int16_t x, const int16_t y)
        {
            registers regs;
            regs.esi = (int32_t)&self;
            regs.dx = widgetIndex;
            regs.ax = x;
            regs.bx = y;
            call(0x0049EC20, regs);
        }

        // 0x0049E7D3
        static void prepare_draw(window* self)
        {
            common::prepare_draw(self);

            auto activatedWidgets = self->activated_widgets;
            activatedWidgets &= ~(1 << widx::checkbox_1 | 1 << widx::checkbox_2 | 1 << widx::checkbox_3 | 1 << widx::checkbox_4);

            self->widgets[widx::checkbox_1].type = widget_type::none;
            self->widgets[widx::checkbox_2].type = widget_type::none;
            self->widgets[widx::checkbox_3].type = widget_type::none;
            self->widgets[widx::checkbox_4].type = widget_type::none;

            if (_trackType & (1 << 7))
            {
                auto trackType = _trackType & ~(1 << 7);
                auto roadObj = objectmgr::get<road_object>(trackType);

                auto args = FormatArguments();
                args.push(roadObj->name);

                if (_modList[0] != 0xFF)
                {
                    auto roadExtraObject = objectmgr::get<road_extra_object>(_modList[0]);
                    self->widgets[widx::checkbox_1].type = widget_type::checkbox;
                    self->widgets[widx::checkbox_1].text = roadExtraObject->name;

                    if (_lastSelectedMods & 1 << 0)
                        activatedWidgets |= 1 << widx::checkbox_1;
                }

                if (_modList[1] != 0xFF)
                {
                    auto roadExtraObject = objectmgr::get<road_extra_object>(_modList[1]);
                    self->widgets[widx::checkbox_2].type = widget_type::checkbox;
                    self->widgets[widx::checkbox_2].text = roadExtraObject->name;

                    if (_lastSelectedMods & 1 << 1)
                        activatedWidgets |= 1 << widx::checkbox_2;
                }
            }
            else
            {
                auto trackObj = objectmgr::get<track_object>(_trackType);

                auto args = FormatArguments();
                args.push(trackObj->name);

                if (_modList[0] != 0xFF)
                {
                    auto trackExtraObject = objectmgr::get<track_extra_object>(_modList[0]);
                    self->widgets[widx::checkbox_1].type = widget_type::checkbox;
                    self->widgets[widx::checkbox_1].text = trackExtraObject->name;

                    if (_lastSelectedMods & 1 << 0)
                        activatedWidgets |= 1 << widx::checkbox_1;
                }

                if (_modList[1] != 0xFF)
                {
                    auto trackExtraObject = objectmgr::get<track_extra_object>(_modList[1]);
                    self->widgets[widx::checkbox_2].type = widget_type::checkbox;
                    self->widgets[widx::checkbox_2].text = trackExtraObject->name;

                    if (_lastSelectedMods & 1 << 1)
                        activatedWidgets |= 1 << widx::checkbox_2;
                }

                if (_modList[2] != 0xFF)
                {
                    auto trackExtraObject = objectmgr::get<track_extra_object>(_modList[2]);
                    self->widgets[widx::checkbox_3].type = widget_type::checkbox;
                    self->widgets[widx::checkbox_3].text = trackExtraObject->name;

                    if (_lastSelectedMods & 1 << 2)
                        activatedWidgets |= 1 << widx::checkbox_3;
                }

                if (_modList[3] != 0xFF)
                {
                    auto trackExtraObject = objectmgr::get<track_extra_object>(_modList[3]);
                    self->widgets[widx::checkbox_4].type = widget_type::checkbox;
                    self->widgets[widx::checkbox_4].text = trackExtraObject->name;

                    if (_lastSelectedMods & 1 << 3)
                        activatedWidgets |= 1 << widx::checkbox_4;
                }
            }

            self->activated_widgets = activatedWidgets;

            self->widgets[widx::image].type = widget_type::none;
            self->widgets[widx::track].type = widget_type::none;
            self->widgets[widx::track_dropdown].type = widget_type::none;

            self->widgets[widx::image].tooltip = string_ids::null;

            if (_lastSelectedMods & 0xF)
            {
                self->widgets[widx::image].type = widget_type::wt_3;
                self->widgets[widx::track].type = widget_type::wt_18;
                self->widgets[widx::track_dropdown].type = widget_type::wt_11;

                self->widgets[widx::image].tooltip = string_ids::upgrade_track_with_mods;

                if (is_unknown_3_mode())
                {
                    if (_toolWindowType == WindowType::construction)
                        self->widgets[widx::image].tooltip = string_ids::click_track_to_upgrade;
                }
            }

            static string_id modString[] = {
                string_ids::single_section,
                string_ids::block_section,
                string_ids::all_connected_track,
            };

            self->widgets[widx::track].text = modString[_lastSelectedTrackModSection];

            common::repositionTabs(self);
        }

        // 0x0049EA3E
        static void draw(window* self, gfx::drawpixelinfo_t* dpi)
        {
            self->draw(dpi);
            common::drawTabs(self, dpi);
            if (_lastSelectedMods & 0xF)
            {
                gfx::drawpixelinfo_t* clipped = nullptr;
                auto xPos = self->x + self->widgets[widx::image].left + 1;
                auto yPos = self->y + self->widgets[widx::image].top + 1;
                auto width = self->widgets[widx::image].width();
                auto height = self->widgets[widx::image].height();

                if (gfx::clip_drawpixelinfo(&clipped, dpi, xPos, yPos, width, height))
                {
                    auto x = 0x2010;
                    auto y = 0x2010;

                    switch (gCurrentRotation)
                    {
                        case 0:
                        {
                            auto bx = x;
                            x = -x + y;
                            y += bx;
                            y >>= 1;
                            y -= 460;
                            break;
                        }
                        case 1:
                        {
                            x = -x;
                            auto bx = x;
                            x -= y;
                            y += bx;
                            y >>= 1;
                            y -= 460;
                            break;
                        }
                        case 2:
                        {
                            auto bx = x;
                            x -= y;
                            y = -y;
                            y -= bx;
                            y >>= 1;
                            y -= 460;
                            break;
                        }
                        case 3:
                        {
                            auto bx = x;
                            x += y;
                            y = -y;
                            y += bx;
                            y >>= 1;
                            y -= 460;
                            break;
                        }
                    }
                    x -= (self->widgets[widx::image].width() / 2);
                    y -= ((self->widgets[widx::image].width() / 2) + 16);
                    clipped->x += x;
                    clipped->y += y;

                    _dword_E0C3E0 = (uint32_t)clipped;

                    x = 0x2000;
                    y = 0x2000;

                    auto company = companymgr::get(_playerCompany);
                    auto companyColour = company->mainColours.primary;
                    auto edi = _lastSelectedMods << 16 | 0x1D0;
                    _byte_522095 = _byte_522095 | (1 << 0);

                    if (_trackType & (1 << 7))
                    {
                        uint8_t trackType = _trackType & ~(1 << 7);
                        auto edx = (companyColour << 16) | trackType;
                        construction::drawRoad(x, y, edi, gCurrentRotation, edx);
                    }
                    else
                    {
                        auto edx = (companyColour << 16) | _trackType;
                        construction::drawTrack(x, y, edi, gCurrentRotation, edx);
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

                gfx::draw_string_centred(*dpi, xPos, yPos, colour::black, string_ids::build_cost, &args);
            }
        }

        void tabReset(window* self)
        {
            self->call_on_mouse_down(overhead::widx::image);
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
}
