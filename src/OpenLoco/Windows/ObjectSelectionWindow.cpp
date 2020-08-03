#include "../Console.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
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
    static Gfx::ui_size_t windowSize = { 600, 398 };

    static loco_global<uint16_t[32], 0x004FE250> object_entry_group_counts;

    static loco_global<char[2], 0x005045F8> _strCheckmark;
    static loco_global<uint8_t*, 0x50D144> _50D144;

    static loco_global<uint16_t[80], 0x00112C181> _112C181;
    static loco_global<int16_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> _byte_112CC04;

    static void on_close(window*);
    static void on_mouse_up(window*, widget_index);
    static void on_update(window*);
    static void get_scroll_size(window*, uint32_t, uint16_t*, uint16_t*);
    static void on_scroll_mouse_down(window*, int16_t, int16_t, uint8_t);
    static void on_scroll_mouse_over(window*, int16_t, int16_t, uint8_t);
    static void tooltip(FormatArguments& args, Ui::window* window, widget_index widgetIndex);
    static void prepare_draw(window*);
    static void draw(window*, Gfx::drawpixelinfo_t*);
    static void draw_scroll(window*, Gfx::drawpixelinfo_t*, uint32_t);

    enum widx
    {
        frame,
        caption,
        closeButton,
        panel,
        tabArea,
        advancedButton,
        scrollview,
        objectImage,
    };

    widget_t widgets[] = {
        makeWidget({ 0, 0 }, { 600, 398 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 598, 13 }, widget_type::caption_25, 0, StringIds::title_object_selection),
        makeWidget({ 585, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 65 }, { 600, 333 }, widget_type::panel, 1),
        makeWidget({ 3, 15 }, { 589, 50 }, widget_type::wt_5, 1),
        makeWidget({ 470, 20 }, { 122, 12 }, widget_type::wt_11, 0, StringIds::object_selection_advanced, StringIds::object_selection_advanced_tooltip),
        makeWidget({ 4, 68 }, { 288, 317 }, widget_type::scrollview, 1, vertical),
        makeWidget({ 391, 68 }, { 114, 114 }, widget_type::wt_9, 1),
        widgetEnd(),
    };

    static window_event_list _events;

    // 0x00473154
    static void sub_473154(window* self)
    {
        registers regs;
        regs.esi = (uintptr_t)self;
        call(0x00473154, regs);
    }

    // 0x004731EE
    static void sub_4731EE(window* self, int eax)
    {
        registers regs;
        regs.eax = eax;
        regs.esi = (uintptr_t)self;
        call(0x004731EE, regs);
    }

    //// 0x00472D3F
    //static void sub_472D3F()
    //{
    //}

    //// 0x00472C84
    //static void resetSelectedObjectCountAndSize()
    //{
    //}

    //// 0x00472C84
    //static void windowEditorObjectSelectionSelectRequiredObjects()
    //{
    //}

    //// 0x00472D19
    //static void sub_472D19()
    //{
    //}

    //// 0x004740CF
    //static void sub_4740CF()
    //{
    //}

    //// 0x00473B5A
    //static void sub_473B5A()
    //{
    //    if (!(ObjectManager::getNumInstalledObjects() & (1 << 0)))
    //    {
    //        sub_472D3F();
    //    }

    //    resetSelectedObjectCountAndSize();
    //    windowEditorObjectSelectionSelectRequiredObjects();
    //    sub_472D19();
    //    sub_4740CF();
    //    resetSelectedObjectCountAndSize();
    //}

    // 0x00472BBC
    static ObjectManager::ObjIndexPair sub_472BBC(window* self)
    {
        //registers regs;
        //regs.esi = (uintptr_t)self;
        //call(0x00472BBC, regs);
        //return { regs.bx, static_cast<ObjectManager::object_index_entry>(regs.ebp) };
        const auto objects = ObjectManager::getAvailableObjects(static_cast<object_type>(self->current_tab));
        for (auto [index, object] : objects)
        {
            if (_50D144[index] & (1 << 0))
            {
                return { static_cast<int16_t>(index), object };
            }
        }

        //for (auto [index, object] : objects)
        //{
        //    return { 0, object };
        //}

        return { -1, ObjectManager::object_index_entry{} };
    }

    // 0x00473A95
    static void sub_473A95()
    {
        registers regs;
        regs.eax = 0;
        call(0x00473A95, regs);
    }

    static void initEvents()
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
    }

    // 0x00472A20
    Ui::window* open()
    {
        auto window = WindowManager::bringToFront(WindowType::objectSelection);

        if (window != nullptr)
            return window;

        sub_473A95();

        window = WindowManager::createWindowCentred(WindowType::objectSelection, { windowSize }, 0, &_events);
        window->widgets = widgets;
        window->enabled_widgets = (1ULL << widx::closeButton) | (1ULL << widx::tabArea) | (1ULL << widx::advancedButton);
        window->initScrollWidgets();
        window->frame_no = 0;
        window->row_hover = -1;
        window->var_856 = 0;

        initEvents();

        window->object = nullptr;

        sub_473154(window);
        sub_4731EE(window, 31);
        ObjectManager::freeScenarioText();

        auto objIndex = sub_472BBC(window);
        if (objIndex.index != -1)
        {
            window->row_hover = objIndex.index;
            window->object = objIndex.object._name;
            ObjectManager::getScenarioText(*objIndex.object._header);
        }

        auto skin = ObjectManager::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0C;

        return window;
    }

    // 0x004733AC
    static void prepare_draw(Ui::window* self)
    {
        self->activated_widgets |= (1 << widx::objectImage);
        widgets[widx::closeButton].type = widget_type::wt_9;

        if (isEditorMode())
        {
            widgets[widx::closeButton].type = widget_type::none;
        }

        self->activated_widgets &= ~(1 << widx::advancedButton);

        if (self->var_856 & (1 << 0))
        {
            self->activated_widgets |= (1 << widx::advancedButton);
        }

        static string_id tabName[] = {
            StringIds::object_interface_styles,
            StringIds::object_sounds,
            StringIds::object_currency,
            StringIds::object_animation_effects,
            StringIds::object_cliffs,
            StringIds::object_water,
            StringIds::object_land,
            StringIds::object_town_names,
            StringIds::object_cargo,
            StringIds::object_walls,
            StringIds::object_signals,
            StringIds::object_level_crossing,
            StringIds::object_street_lights,
            StringIds::object_tunnels,
            StringIds::object_bridges,
            StringIds::object_track_stations,
            StringIds::object_track_extras,
            StringIds::object_tracks,
            StringIds::object_road_stations,
            StringIds::object_road_extras,
            StringIds::object_roads,
            StringIds::object_airports,
            StringIds::object_docks,
            StringIds::object_vehicles,
            StringIds::object_trees,
            StringIds::object_snow,
            StringIds::object_climate,
            StringIds::object_map_generation_data,
            StringIds::object_buildings,
            StringIds::object_scaffolding,
            StringIds::object_industries,
            StringIds::object_world_region,
            StringIds::object_company_owners,
            StringIds::object_scenario_descriptions,
        };

        auto args = FormatArguments();
        args.push(tabName[self->current_tab]);
    }

    static loco_global<uint16_t[40], 0x0112C1C5> _112C1C5;

    // 0x0047328D
    static void sub_47328D(window* self, Gfx::drawpixelinfo_t* dpi)
    {
    }

    //static void* getDescription(ObjectManager::header* header, void* rawObject)
    //{
    //    switch (header->get_type())
    //    {
    //        case object_type::interface_skin:
    //        {
    //            auto object = (interface_skin_object*)rawObject;
    //            Gfx::recolour(object->img, 11);
    //            break;
    //        }

    //        case object_type::sound:
    //            // null
    //            break;

    //        case object_type::currency: break;

    //        case object_type::steam:
    //            // null
    //            break;

    //        case object_type::rock: break;
    //        case object_type::water: break;

    //        case object_type::land:
    //        {
    //            auto object = (land_object*)rawObject;
    //            uint32_t imageId = object->image + (object->var_3 - 1) * object->var_E;
    //            break;
    //        }

    //        case object_type::town_names:
    //            // null
    //            break;

    //        case object_type::cargo:
    //            // null
    //            break;

    //        case object_type::wall: break;
    //        case object_type::track_signal: break;
    //        case object_type::level_crossing: break;
    //        case object_type::street_light: break;
    //        case object_type::tunnel: break;
    //        case object_type::bridge: break;
    //        case object_type::track_station: break;
    //        case object_type::track_extra: break;
    //        case object_type::track: break;
    //        case object_type::road_station: break;
    //        case object_type::road_extra: break;
    //        case object_type::road: break;
    //        case object_type::airport: break;
    //        case object_type::dock: break;
    //        case object_type::vehicle: break;
    //        case object_type::tree: break;
    //        case object_type::snow: break;
    //        case object_type::climate: break;
    //        case object_type::hill_shapes: break;
    //        case object_type::building: break;
    //        case object_type::scaffolding: break;
    //        case object_type::industry: break;
    //        case object_type::region: break;
    //        case object_type::competitor: break;
    //        case object_type::scenario_text: break;
    //    }
    //}

    static loco_global<uintptr_t[34], 0x004FE1C8> _paintEntryTable;
    static void drawDescription(ObjectManager::header* header, Gfx::drawpixelinfo_t* dpi, void* ebp, int x, int y)
    {

        registers regs;
        regs.edi = (uintptr_t)dpi;
        regs.ebp = (uintptr_t)ebp;
        regs.ax = 3;
        regs.cx = x;
        regs.dx = y;
        call(_paintEntryTable[(int)header->getType()], regs);
    }

    // 0x004733F5
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        Gfx::fillRectInset(dpi, self->x, self->y + 20, self->x + self->width - 1, self->y + 20 + 60, self->colours[0], 0);
        self->draw(dpi);

        sub_47328D(self, dpi);

        bool doDefault = true;
        if (self->object != nullptr)
        {
            auto var = (OpenLoco::ObjectManager::header*)self->object;
            if (var->getType() != object_type::town_names && var->getType() != object_type::climate)
            {
                doDefault = false;
            }
        }

        if (doDefault)
        {
            auto widget = widgets[widx::objectImage];
            auto colour = Colour::getShade(self->colours[1], 5);
            Gfx::drawRect(dpi, self->x + widget.left, self->y + widget.top, widget.width(), widget.height(), colour);
        }
        auto type = self->current_tab;

        auto args = FormatArguments();
        args.push(_112C1C5[type]);
        args.push(object_entry_group_counts[type]);

        Gfx::drawString_494B3F(*dpi, self->x + 3, self->y + self->height - 12, 0, 2038, &args);

        if (self->row_hover == -1)
            return;

        loco_global<void*, 0x0050D15C> _50D15C;

        if (_50D15C == (void*)-1)
            return;

        drawDescription(
            (ObjectManager::header*)self->object,
            dpi,
            _50D15C,
            widgets[widx::objectImage].mid_x() + 1 + self->x,
            widgets[widx::objectImage].mid_y() + 1 + self->y);
    }

    // 0x0047361D
    static void draw_scroll(window* self, Gfx::drawpixelinfo_t* dpi, uint32_t)
    {
        Gfx::clearSingle(*dpi, Colour::getShade(self->colours[1], 4));

        if (ObjectManager::getNumInstalledObjects() == 0)
            return;

        int y = 0;
        auto objects = ObjectManager::getAvailableObjects(static_cast<object_type>(self->current_tab));
        for (auto [i, object] : objects)
        {
            uint8_t flags = (1 << 7) | (1 << 6) | (1 << 5);
            Gfx::fillRectInset(dpi, 2, y, 11, y + 10, self->colours[1], flags);

            uint8_t textColour = ControlCodes::colour_black;

            if (object._name == self->object)
            {
                Gfx::fillRect(dpi, 0, y, self->width, y + rowHeight - 1, (1 << 25) | PaletteIndex::index_30);
                textColour = ControlCodes::window_colour_2;
            }

            if (_50D144[i] & (1 << 0))
            {
                auto x = 2;
                _currentFontSpriteBase = Font::m2;

                if (textColour != ControlCodes::window_colour_2)
                {
                    _currentFontSpriteBase = Font::m1;
                }

                textColour = Colour::opaque(self->colours[1]);

                if (_50D144[i] & ((1 << 4) | (1 << 3) | (1 << 2)))
                {
                    textColour = Colour::inset(textColour);
                }

                Gfx::drawString(dpi, x, y, textColour, _strCheckmark);
            }

            _byte_112CC04[0] = textColour;
            strncpy(&_byte_112CC04[1], object._name, 512);
            _currentFontSpriteBase = Font::medium_bold;

            Gfx::drawString(dpi, 15, y, Colour::black, _byte_112CC04);
            y += rowHeight;
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
                int dx = widgets[widx::panel].top + self->y - 26;
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
                    self->object = nullptr;
                    self->scroll_areas[0].contentOffsetY = 0;
                    ObjectManager::freeScenarioText();
                    auto objIndex = sub_472BBC(self);
                    if (objIndex.index != -1)
                    {
                        self->row_hover = objIndex.index;
                        self->object = objIndex.object._name;

                        ObjectManager::getScenarioText(*objIndex.object._header);
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
    static void tooltip(FormatArguments& args, Ui::window* window, widget_index widgetIndex)
    {
        args.push(StringIds::tooltip_object_list);
    }

    // 0x00472B54
    static ObjectManager::ObjIndexPair getObjectFromSelection(window* self, const int16_t& y)
    {
        //registers regs;
        //regs.esi = reinterpret_cast<uintptr_t>(self);
        //regs.dx = y;
        //call(0x00472B54, regs);

        //uint8_t* edi = (uint8_t*)regs.edi;

        //return std::make_tuple(regs.bx, (ObjectManager::object_index_entry)regs.ebp, *edi);

        const int16_t rowIndex = y / rowHeight;
        const auto objects = ObjectManager::getAvailableObjects(static_cast<object_type>(self->current_tab));
        for (auto [index, object] : objects)
        {
            if (rowIndex < 0 || static_cast<uint16_t>(rowIndex) >= objects.size())
            {
                return { -1, ObjectManager::object_index_entry{} };
            }
        }

        //if (isInUseCompetitor(objects[rowIndex].first))
        //{
        //    return { -1, ObjectManager::object_index_entry{} };
        //}
        return { rowIndex, objects[rowIndex].second };
    }

    // 0x0047390A
    static void on_scroll_mouse_over(Ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    {
        auto objIndex = getObjectFromSelection(self, y);

        if (objIndex.index == self->row_hover || objIndex.index == 0xFFFF)
            return;

        self->row_hover = objIndex.index;
        self->object = objIndex.object._name;
        ObjectManager::freeScenarioText();

        if (objIndex.index != -1)
        {
            ObjectManager::getScenarioText(*objIndex.object._header);
        }

        self->invalidate();
    }

    //static void window_editor_object_selection_select_object(uint16_t bx, void* ebp)
    //{
    //    registers regs;
    //    regs.bx = bx;
    //    regs.ebp = (uintptr_t)ebp;
    //    call(0x0000473D1D, regs);
    //}

    //// 0x00473948
    //[[maybe_unused]] static void on_scroll_mouse_down(Ui::window* self, int16_t x, int16_t y, uint8_t scroll_index)
    //{
    //    auto [bx, ebp, edi] = sub_472B54(self, y);
    //    if (bx == 0xFFFF)
    //        return;

    //    self->invalidate();

    //    int type = (int)ebp->get_type();
    //    if (object_entry_group_counts[type] == 1)
    //    {
    //    }

    //    bool bx2 = 0;
    //    if ((edi & 1) == 0)
    //        bx2 |= 1;

    //    bx2 |= 6;

    //    window_editor_object_selection_select_object(bx2, ebp);
    //}

    // 0x004739DD
    static void on_close(window* self)
    {
        if (!isEditorMode())
            return;

        call(0x00474821); // unload_unselected_objects
        call(0x00474874); // editor_load_selected_objects
        call(0x0047237D); // reset_loaded_objects
        ObjectManager::freeScenarioText();
        call(0x00473B91); // editor_object_flags_free_0
    }

    // 0x00473A04
    static void on_update(window* self)
    {
        WindowManager::invalidateWidget(WindowType::objectSelection, self->number, 7);
    }
}
