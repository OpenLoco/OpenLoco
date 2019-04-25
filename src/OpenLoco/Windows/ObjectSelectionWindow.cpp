#include "../Console.h"
#include "../Graphics/Colour.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/LandObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../Window.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ObjectSelectionWindow
{
    static constexpr int rowHeight = 12;

    static loco_global<uint16_t[32], 0x004FE250> object_entry_group_counts;
    static window_event_list _events;

    static loco_global<uint16_t[80], 0x00112C181> _112C181;

    static loco_global<uint8_t*, 0x50D144> _50D144;

    static loco_global<uint32_t, 0x0112A110> _installedObjectCount;

    static loco_global<ui::widget_t[8], 0x0050D164> _widgets;

    static void on_close(window*);
    static void on_mouse_up(window*, widget_index);
    static void on_update(window*);
    static void get_scroll_size(window*, uint32_t, uint16_t*, uint16_t*);
    static void on_scroll_mouse_down(window*, int16_t, int16_t, uint8_t);
    static void on_scroll_mouse_over(window*, int16_t, int16_t, uint8_t);
    static void tooltip(window*, widget_index);
    static void prepare_draw(window*);
    static void draw(window*, gfx::drawpixelinfo_t*);
    static void draw_scroll(window*, gfx::drawpixelinfo_t*, uint32_t);

    static void sub_473154(window* self)
    {
        registers regs;
        regs.esi = (uintptr_t)self;
        call(0x00473154, regs);
    }

    static void sub_4731EE(window* self, int eax)
    {
        registers regs;
        regs.eax = eax;
        regs.esi = (uintptr_t)self;
        call(0x004731EE, regs);
    }

    static std::tuple<uint16_t, objectmgr::header*> sub_472BBC(window* self)
    {
        registers regs;
        regs.esi = (uintptr_t)self;
        call(0x00472BBC, regs);
        return std::make_tuple(regs.bx, (objectmgr::header*)regs.ebp);
    }

    // 0x00472A20
    ui::window* open()
    {
        _events.on_close = on_close;
        _events.on_mouse_up = on_mouse_up;
        _events.on_update = on_update;
        _events.get_scroll_size = get_scroll_size;
        //        _events.scroll_mouse_down = on_scroll_mouse_down;
        _events.scroll_mouse_down = reinterpret_cast<void (*)(window*, int16_t, int16_t, uint8_t)>(0x00473948);
        _events.scroll_mouse_over = on_scroll_mouse_over;
        _events.tooltip = tooltip;
        _events.prepare_draw = prepare_draw;
        _events.draw = draw;
        _events.draw_scroll = draw_scroll;

        ui::window* window;

        window = WindowManager::bringToFront(WindowType::objectSelection);
        if (window != nullptr)
            return window;

        registers r1;
        r1.eax = 0;
        call(0x00473A95, r1);

        window = WindowManager::createWindowCentred(WindowType::objectSelection, { 600, 398 }, 0, &_events);
        window->widgets = _widgets;
        window->enabled_widgets = 4 | 16 | 32;
        window->init_scroll_widgets();
        window->frame_no = 0;
        window->row_hover = -1;
        window->var_856 = 0;

        window->var_85A_ptr = reinterpret_cast<void*>(-1);

        sub_473154(window);
        sub_4731EE(window, 31);
        call(0x00471B95); // free_scenario_text_from_object
        auto [bx, ebp] = sub_472BBC(window);
        if (bx != 0xFFFF)
        {
            registers r1;
            r1.esi = (uintptr_t)window;
            r1.ebp = (uintptr_t)ebp;
            call(0x0047176D, r1); // get_scenario_text_from_object
        }

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0C;

        return window;
    }

    // 0x004733AC
    static void prepare_draw(ui::window* self)
    {
        self->activated_widgets |= 1 << 7;
        _widgets[2].type = widget_type::wt_9;

        if (is_editor_mode())
        {
            _widgets[2].type = widget_type::none;
        }

        self->activated_widgets &= ~(1 << 5);

        if (self->var_856 & 1)
        {
            self->activated_widgets |= (1 << 5);
        }

        loco_global<string_id, 0x112C826> common_format_args;
        *common_format_args = 2053 + self->current_tab;
    }
    static loco_global<uint16_t[40], 0x0112C1C5> _112C1C5;

    // 0x0047328D
    static void sub_47328D(window* self, gfx::drawpixelinfo_t* dpi)
    {
    }

    static void* getDescription(objectmgr::header* header, void* rawObject)
    {
        switch (header->get_type())
        {
            case object_type::interface_skin:
            {
                auto object = (interface_skin_object*)rawObject;
                gfx::recolour(object->img, 11);
                break;
            }

            case object_type::sound:
                // null
                break;

            case object_type::currency: break;

            case object_type::steam:
                // null
                break;

            case object_type::rock: break;
            case object_type::water: break;

            case object_type::land:
            {
                auto object = (land_object*)rawObject;
                uint32_t imageId = object->image + (object->var_3 - 1) * object->var_E;
                break;
            }

            case object_type::town_names:
                // null
                break;

            case object_type::cargo:
                // null
                break;

            case object_type::wall: break;
            case object_type::track_signal: break;
            case object_type::level_crossing: break;
            case object_type::street_light: break;
            case object_type::tunnel: break;
            case object_type::bridge: break;
            case object_type::track_station: break;
            case object_type::track_extra: break;
            case object_type::track: break;
            case object_type::road_station: break;
            case object_type::road_extra: break;
            case object_type::road: break;
            case object_type::airport: break;
            case object_type::dock: break;
            case object_type::vehicle: break;
            case object_type::tree: break;
            case object_type::snow: break;
            case object_type::climate: break;
            case object_type::hill_shapes: break;
            case object_type::building: break;
            case object_type::scaffolding: break;
            case object_type::industry: break;
            case object_type::region: break;
            case object_type::competitor: break;
            case object_type::scenario_text: break;
        }
    }

    static loco_global<uintptr_t[34], 0x004FE1C8> _paintEntryTable;
    static void drawDescription(objectmgr::header* header, gfx::drawpixelinfo_t* dpi, void* ebp, int x, int y)
    {

        registers regs;
        regs.edi = (uintptr_t)dpi;
        regs.ebp = (uintptr_t)ebp;
        regs.ax = 3;
        regs.cx = x;
        regs.dx = y;
        call(_paintEntryTable[(int)header->get_type()], regs);
    }

    // 0x004733F5
    static void draw(window* self, gfx::drawpixelinfo_t* dpi)
    {
        gfx::fill_rect_inset(dpi, self->x, self->y + 20, self->x + self->width - 1, self->y + 20 + 60, self->colours[0], 0);
        self->draw(dpi);

        sub_47328D(self, dpi);

        bool doDefault = true;
        if (self->var_85A_ptr != (void*)-1)
        {
            auto var = (openloco::objectmgr::header*)self->var_85A_ptr;
            if (var->get_type() != object_type::town_names && var->get_type() != object_type::climate)
            {
                doDefault = false;
            }
        }

        if (doDefault)
        {
            gfx::draw_rect(dpi, self->x + _widgets[7].left, self->y + _widgets[7].top, _widgets[7].width(), _widgets[7].height(), colour::get_shade(self->colours[1], 5));
        }
        auto type = self->current_tab;

        loco_global<string_id, 0x112C826> common_format_args;
        addr<0x112C826 + 0, int16_t>() = _112C1C5[type];
        addr<0x112C826 + 2, int16_t>() = object_entry_group_counts[type];

        gfx::draw_string_494B3F(*dpi, self->x + 3, self->y + self->height - 12, 0, 2038, &common_format_args);

        if (self->row_hover == -1)
            return;

        loco_global<void*, 0x0050D15C> _50D15C;

        if (_50D15C == (void*)-1)
            return;

        drawDescription(
            (objectmgr::header*)self->var_85A_ptr,
            dpi,
            _50D15C,
            _widgets[7].mid_x() + 1 + self->x,
            _widgets[7].mid_y() + 1 + self->y);
    }

    static void draw_scroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t)
    {
        gfx::clear_single(*dpi, colour::get_shade(self->colours[1], 4));

        if (_installedObjectCount == 0)
            return;

        int dx = 0;
        auto objects = objectmgr::getAvailableObjects(static_cast<object_type>(self->current_tab));
        for (auto [i, object] : objects)
        {
            gfx::fill_rect_inset(dpi, 2, dx, 11, dx + 10, self->colours[1], 0xE0);

            uint8_t al = control_codes::colour_black;
            if (object._header == self->var_85A_ptr)
            {
                gfx::fill_rect(dpi, 0, dx, self->width, dx + rowHeight - 1, 0x2000030);
                al = control_codes::window_colour_2;
            }

            if (_50D144[i] & 1)
            {
                gfx::fill_rect_inset(dpi, 2, dx, 11, dx + 10, colour::bright_green, 0xE0);
                // draw checkmark
            }

            std::string buffer;
            buffer += al;
            buffer += object._name;
            gfx::draw_string(dpi, 15, dx, 0, buffer.data());
            dx += rowHeight;
        }
    }

    static loco_global<uint8_t[999], 0x004FE384> _4FE384;

    struct unk1
    {
        uint8_t var_0;
        uint8_t var_1;
    };
    static loco_global<unk1[36], 0x0112C21C> _112C21C;

    static loco_global<uint16_t, 0x0052334A> _52334A;
    static loco_global<uint16_t, 0x0052334C> _52334C;

    // 0x004737BA
    void on_mouse_up(window* self, widget_index w)
    {
        switch (w)
        {
            case 2:
                call(0x00473A13);
                break;

            case 4:
            {
                int clickedTab = -1;
                int dx = _widgets[3].top + self->y - 26;
                int cx = self->x + 3;

                for (int bx = 0; bx < 2; bx++)
                {
                    int y = dx - bx * 24;
                    int x = bx * 10 + cx;

                    for (int i = 0; _112C21C[i].var_0 != 0xFF; i++)
                    {
                        if (_112C21C[i].var_1 != bx)
                            continue;

                        if (_52334A >= x && _52334C >= y)
                        {
                            if (_52334A < x + 31 && y + 27 > _52334C)
                            {
                                clickedTab = _112C21C[i].var_0;
                                break;
                            }
                        }

                        x += 31;
                    }
                }

                if (clickedTab != -1 && self->current_tab != clickedTab)
                {
                    sub_4731EE(self, clickedTab);
                    self->row_hover = -1;
                    self->var_85A_ptr = reinterpret_cast<void*>(-1);
                    self->scroll_areas[0].v_top = 0;
                    call(0x00471B95); // free_scenario_text_from_object
                    auto [bx, ebp] = sub_472BBC(self);
                    if (bx != 0xFFFF)
                    {
                        self->row_hover = bx;
                        self->var_85A_ptr = ebp;

                        registers r1;
                        r1.esi = (uintptr_t)self;
                        r1.ebp = (uintptr_t)ebp;
                        call(0x0047176D, r1); // get_scenario_text_from_object
                    }

                    self->invalidate();
                }

                break;
            }

            case 5:
            {
                self->var_856 ^= 1;
                int eax = self->current_tab;
                sub_473154(self);

                if ((self->var_856 & 1) == 0)
                {
                    if (_4FE384[eax] & 1 << 1)
                    {
                        eax = _112C21C[0].var_0;
                    }
                }
                sub_4731EE(self, eax);
                self->invalidate();

                break;
            }
        }
    }

    // 0x004738ED
    void get_scroll_size(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        *scrollHeight = _112C181[self->current_tab] * rowHeight;
    }

    // 0x00473900
    static void tooltip(ui::window* self, widget_index widgetIndex)
    {
        loco_global<string_id, 0x112C826> common_format_args;
        *common_format_args = 2087; // TODO: string id
    }

    static std::tuple<uint16_t, objectmgr::header*, uint8_t> sub_472B54(ui::window* self, int16_t y)
    {
        registers regs;
        regs.esi = reinterpret_cast<uintptr_t>(self);
        regs.dx = y;
        call(0x00472B54, regs);

        uint8_t* edi = (uint8_t*)regs.edi;

        return std::make_tuple(regs.bx, (objectmgr::header*)regs.ebp, *edi);
    }

    // 0x0047390A
    static void on_scroll_mouse_over(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto [bx, ebp, edi] = sub_472B54(self, y);
        if (bx == self->row_hover)
            return;

        self->row_hover = bx;
        self->var_85A_ptr = ebp;
        call(0x00471B95); // free_scenario_text_from_object
        if (bx != 0xFFFF)
        {
            registers r1;
            r1.esi = (uintptr_t)self;
            r1.ebp = (uintptr_t)ebp;
            call(0x0047176D, r1); // get_scenario_text_from_object
        }
        self->invalidate();
    }

    static void window_editor_object_selection_select_object(uint16_t bx, void* ebp)
    {
        registers regs;
        regs.bx = bx;
        regs.ebp = (uintptr_t)ebp;
        call(0x0000473D1D, regs);
    }

    // 0x00473948
    [[maybe_unused]] static void on_scroll_mouse_down(ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto [bx, ebp, edi] = sub_472B54(self, y);
        if (bx == 0xFFFF)
            return;

        self->invalidate();

        int type = (int)ebp->get_type();
        if (object_entry_group_counts[type] == 1)
        {
        }

        bool bx2 = 0;
        if ((edi & 1) == 0)
            bx2 |= 1;

        bx2 |= 6;

        window_editor_object_selection_select_object(bx2, ebp);
    }

    // 0x004739DD
    static void on_close(window* self)
    {
        if (!is_editor_mode())
            return;

        call(0x00474821); // unload_unselected_objects
        call(0x00474874); // editor_load_selected_objects
        call(0x0047237D); // reset_loaded_objects
        call(0x00471B95); // free_scenario_text_from_object
        call(0x00473B91); // editor_object_flags_free_0
    }

    // 0x00473A04
    static void on_update(window* self)
    {
        WindowManager::invalidateWidget(WindowType::objectSelection, self->number, 7);
    }
}
