#include "../companymgr.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../industrymgr.h"
#include "../input.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/industry_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/track_object.h"
#include "../things/thing.h"
#include "../things/thingmgr.h"
#include "../townmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/scrollview.h"
#include "../widget.h"

using namespace openloco::interop;

namespace openloco::ui::windows::map
{
    static loco_global<int32_t, 0x00523338> _cursorX2;
    static loco_global<int32_t, 0x0052333C> _cursorY2;
    static loco_global<uint16_t[7], 0x004FDC4C> _word_4FDC4C;
    static loco_global<uint16_t[7], 0x004FDC4E> _word_4FDC4E;
    static loco_global<uint8_t[256], 0x004FDC5C> _byte_4FDC5C;
    static loco_global<uint32_t, 0x0526284> _dword_526284;
    static loco_global<gfx::ui_size_t, 0x00526288> _word_526288;
    static loco_global<uint16_t, 0x0052628C> _word_52628C;
    static loco_global<uint16_t, 0x0052628E> _word_52628E;
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;
    static loco_global<uint32_t, 0x00F253A4> _dword_F253A4;
    static loco_global<uint8_t*, 0x00F253A8> _dword_F253A8;
    static loco_global<uint16_t[6], 0x00F253BA> _word_F253BA;
    static loco_global<uint8_t[16], 0x00F253CE> _byte_F253CE;
    static loco_global<uint8_t[19], 0x00F253DF> _byte_F253DF;
    static loco_global<uint8_t[19], 0x00F253F2> _byte_F253F2;
    static loco_global<uint32_t, 0x00F2541D> _word_F2541D;
    static loco_global<uint32_t, 0x00525E28> _dword_525E28;
    static loco_global<company_id_t, 0x00525E3C> _playerCompanyId;
    static loco_global<uint8_t[companymgr::max_companies + 1], 0x009C645C> _companyColours;
    static loco_global<int16_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> _stringFormatBuffer;

    enum widx
    {
        frame = 0,
        caption,
        closeButton,
        panel,
        tabOverall,
        tabVehicles,
        tabIndustries,
        tabRoutes,
        tabOwnership,
        scrollview,
        statusBar,
    };

    const uint64_t enabledWidgets = (1 << closeButton) | (1 << tabOverall) | (1 << tabVehicles) | (1 << tabIndustries) | (1 << tabRoutes) | (1 << tabOwnership);

    widget_t widgets[] = {
        make_widget({ 0, 0 }, { 350, 272 }, widget_type::frame, 0),
        make_widget({ 1, 1 }, { 348, 13 }, widget_type::caption_25, 0),
        make_widget({ 335, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        make_widget({ 0, 41 }, { 350, 230 }, widget_type::panel, 1),
        make_widget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_overall),
        make_widget({ 34, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_vehicles),
        make_widget({ 65, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_industries),
        make_widget({ 96, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_routes),
        make_widget({ 158, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_ownership),
        make_widget({ 3, 44 }, { 240, 215 }, widget_type::scrollview, 1, horizontal | vertical),
        make_widget({ 3, 250 }, { 322, 21 }, widget_type::wt_13, 1),
        widget_end()
    };

    static window_event_list events;

    // 0x0046B8E6
    static void onClose(window* self)
    {
        _word_526288 = gfx::ui_size_t(self->width, self->height);
        _word_52628C = self->var_88A;
        _word_52628E = self->var_88C;
        _dword_526284 = self->flags | window_flags::flag_31;
        addr<0x00113E87C, int32_t>() = 3;
        free(_dword_F253A8);
        addr<0x00113E87C, int32_t>() = 0;
    }

    // 0x0046B8CF
    static void onMouseUp(window* self, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::closeButton:
                WindowManager::close(self);
                break;

            case widx::tabOverall:
            case widx::tabVehicles:
            case widx::tabIndustries:
            case widx::tabRoutes:
            case widx::tabOwnership:
            case widx::scrollview:
            {
                auto tabIndex = widgetIndex - widx::tabOverall;
                if (tabIndex == self->current_tab)
                    return;
                self->current_tab = tabIndex;
                self->frame_no = 0;
                self->var_854 = 0;
                break;
            }
        }
    }

    // 0x0046B9F7
    static void onResize(window* self)
    {
        self->flags |= window_flags::resizable;
        self->min_width = 350;
        self->max_width = 800;
        self->max_height = 800;

        gfx::ui_size_t minWindowSize = { self->min_width, self->min_height };
        gfx::ui_size_t maxWindowSize = { self->max_width, self->max_height };
        self->set_size(minWindowSize, maxWindowSize);
    }

    static void sub_46C544(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x0046C544, regs);
    }

    static void sub_46D300(window* self, int16_t x, int16_t y)
    {
        auto i = 0;

        if (!input::has_flag(input::input_flags::flag5))
        {
            int32_t cursorX = _cursorX2;
            int32_t cursorY = _cursorY2;
            auto window = WindowManager::findAt(cursorX, cursorY);

            if (window != nullptr)
            {
                if (window == self)
                {
                    cursorX -= x;
                    if (cursorX <= 100)
                    {
                        cursorY -= y;
                        if (cursorY < 60)
                        {
                            y = cursorY;

                            for (; i < 6; i++)
                            {
                                y -= 10;

                                if (y < 0)
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        uint32_t eax;

        if (y < 0)
        {
            eax = (1 << i);
        }
        else
        {
            eax = 0;
        }

        if (eax != self->var_854)
        {
            self->var_854 = eax;
            self->invalidate();
        }

        if (self->var_854 != 0)
        {
            self->invalidate();
        }
    }

    static void sub_46D406(window* self, int16_t x, int16_t y)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x0046D406, regs);
    }

    static void sub_46D520(window* self, int16_t x, int16_t y)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x0046D520, regs);
    }

    static void sub_46D66A(window* self, int16_t x, int16_t y)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x0046D66A, regs);
    }

    static void sub_46D789(window* self, int16_t x, int16_t y)
    {
        registers regs;
        regs.cx = x;
        regs.dx = y;
        regs.esi = (int32_t)self;
        call(0x0046D789, regs);
    }

    static void sub_46B69C()
    {
        registers regs;
        call(0x0046B69C, regs);
    }

    // 0x0046BA5B
    static void onUpdate(window* self)
    {
        self->frame_no++;
        self->call_prepare_draw();

        WindowManager::invalidateWidget(WindowType::map, self->number, self->current_tab + widx::tabOverall);

        _word_F2541D++;

        if (gCurrentRotation != self->var_846)
        {
            self->var_846 = gCurrentRotation;
            sub_46B69C();
        }

        auto i = 80;

        while (i > 0)
        {
            sub_46C544(self);
            i--;
        }

        self->invalidate();

        auto x = self->x + self->width - 104;
        auto y = self->y + 44;

        switch (self->current_tab + widx::tabOverall)
        {
            case widx::tabOverall:
                sub_46D300(self, x, y);
                break;

            case widx::tabVehicles:
                sub_46D406(self, x, y);
                break;

            case widx::tabIndustries:
                sub_46D520(self, x, y);
                break;

            case widx::tabRoutes:
                sub_46D66A(self, x, y);
                break;

            case widx::tabOwnership:
                sub_46D789(self, x, y);
                break;
        }
    }

    // 0x0046B9E7
    static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        self->call_prepare_draw();
        *scrollWidth = 0x300;
        *scrollHeight = 0x300;
    }

    // 0x004C6801
    static void moveMainWindowToLocation(window* self, map_pos pos)
    {
        if (self->viewport_configurations->viewport_target_sprite != 0xFFFF)
            return;

        if (self->flags & window_flags::viewport_no_scrolling)
            return;

        self->viewport_configurations->saved_view_x = pos.x;
        self->viewport_configurations->saved_view_y = pos.y;
        self->flags |= window_flags::scrolling_to_location;
    }

    // 0x004C6827
    static void sub_4C6827(window* self, int16_t x, int16_t y, uint32_t z)
    {
        auto viewport = self->viewports[0];
        if (viewport == nullptr)
            return;

        auto height = tile_element_height(x, y);
        height -= 16;

        if (z < height)
        {
            if (!(viewport->flags & viewport_flags::underground_view))
            {
                self->invalidate();
            }

            viewport->flags |= viewport_flags::underground_view;
        }
        else
        {
            if (viewport->flags & viewport_flags::underground_view)
            {
                self->invalidate();
            }

            viewport->flags &= ~viewport_flags::underground_view;
        }

        auto pos = coordinate_3d_to_2d(x, y, z, gCurrentRotation);

        moveMainWindowToLocation(self, pos);
    }

    // 0x0046B9D4
    static void sub_46B9D4(map_pos pos)
    {
        auto z = tile_element_height(pos.x, pos.y);
        auto window = WindowManager::getMainWindow();

        if (window == nullptr)
            return;

        sub_4C6827(window, pos.x, pos.y, z);
    }

    // 0x0046B97C
    static void event18(window* self, int16_t x, int16_t y, uint8_t scrollIndex)
    {
        x += 8;
        y += 8;
        x -= 384;
        x /= 2;
        y /= 2;
        auto yCopy = y;
        y -= x;
        x += yCopy;
        x *= 32;
        y *= 32;

        map_pos pos = { x, y };
        pos = rotate2DCoordinate(pos, gCurrentRotation);

        sub_46B9D4(pos);
    }

    // 0x0046B946
    static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
    {
        args.push(string_ids::tooltip_scroll_map);
    }

    // 0x0046D223
    static void leftAlignTabs(window* self, uint8_t firstTabIndex, uint8_t lastTabIndex)
    {
        auto disabledWidgets = self->disabled_widgets >> firstTabIndex;
        auto pos = self->widgets[firstTabIndex].left;
        auto tabWidth = self->widgets[firstTabIndex].right - pos;

        for (auto index = firstTabIndex; index <= lastTabIndex; index++)
        {
            self->widgets[index].type = widget_type::none;

            if (!(disabledWidgets & (1ULL << index)))
            {
                self->widgets[index].type = widget_type::wt_8;

                self->widgets[index].left = pos;
                pos += tabWidth;

                self->widgets[index].right = pos;
                pos++;
            }
        }
    }

    // 0x0046B6BF
    static void prepareDraw(window* self)
    {
        const string_id captionText[] = {
            string_ids::title_map,
            string_ids::title_map_vehicles,
            string_ids::title_map_industries,
            string_ids::title_map_routes,
            string_ids::title_map_companies,
        };

        widgets[1].text = captionText[self->current_tab];
        auto activatedWidgets = self->activated_widgets;
        activatedWidgets &= ~((1ULL << widx::statusBar) | (1ULL << widx::scrollview) | (1ULL << widx::tabOwnership) | (1ULL << widx::tabRoutes) | (1ULL << widx::tabIndustries) | (1ULL << widx::tabVehicles) | (1ULL << widx::tabOverall));

        auto currentWidget = self->current_tab + widx::tabOverall;
        activatedWidgets |= (1ULL << currentWidget);
        self->activated_widgets = activatedWidgets;

        self->widgets[widx::frame].right = self->width - 1;
        self->widgets[widx::frame].bottom = self->height - 1;
        self->widgets[widx::panel].right = self->width - 1;
        self->widgets[widx::panel].bottom = self->height + 1;

        self->widgets[widx::caption].right = self->width - 2;
        self->widgets[widx::closeButton].left = self->width - 15;
        self->widgets[widx::closeButton].right = self->width - 3;
        self->widgets[widx::scrollview].bottom = self->height - 14;
        self->widgets[widx::scrollview].right = self->width - 108;

        self->widgets[widx::statusBar].top = self->height - 12;
        self->widgets[widx::statusBar].bottom = self->height - 3;
        self->widgets[widx::statusBar].right = self->width - 14;

        auto disabledWidgets = 0;
        if (is_editor_mode())
        {
            disabledWidgets |= (1 << widx::tabVehicles) | (1 << widx::tabRoutes) | (1 << widx::tabOwnership);
        }
        self->disabled_widgets = disabledWidgets;

        leftAlignTabs(self, widx::tabOverall, widx::tabOwnership);
    }

    // 0x0046D0E0
    static void drawTabs(window* self, gfx::drawpixelinfo_t* dpi)
    {
        auto skin = objectmgr::get<interface_skin_object>();

        // tabOverall
        {
            uint32_t imageId = skin->img;
            imageId += interface_skin::image_ids::toolbar_menu_map_north;

            widget::draw_tab(self, dpi, imageId, widx::tabOverall);
        }

        // tabVehicles,
        {
            if (!(self->disabled_widgets & (1 << widx::tabVehicles)))
            {
                static const uint32_t vehicleImageIds[] = {
                    interface_skin::image_ids::vehicle_train_frame_0,
                    interface_skin::image_ids::vehicle_train_frame_1,
                    interface_skin::image_ids::vehicle_train_frame_2,
                    interface_skin::image_ids::vehicle_train_frame_3,
                    interface_skin::image_ids::vehicle_train_frame_4,
                    interface_skin::image_ids::vehicle_train_frame_5,
                    interface_skin::image_ids::vehicle_train_frame_6,
                    interface_skin::image_ids::vehicle_train_frame_7,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tabVehicles - widx::tabOverall)
                    imageId += vehicleImageIds[(self->frame_no / 2) % std::size(vehicleImageIds)];
                else
                    imageId += vehicleImageIds[0];

                auto colour = colour::black;

                if (!is_editor_mode())
                {
                    auto company = companymgr::get(_playerCompanyId);
                    colour = company->mainColours.primary;
                }

                imageId = gfx::recolour(imageId, colour);

                widget::draw_tab(self, dpi, imageId, widx::tabVehicles);
            }
        }

        // tabIndustries,
        {
            uint32_t imageId = skin->img;
            imageId += interface_skin::image_ids::toolbar_menu_industries;

            widget::draw_tab(self, dpi, imageId, widx::tabIndustries);
        }

        // tabRoutes,
        {
            if (!(self->disabled_widgets & (1 << widx::tabRoutes)))
            {
                static const uint32_t routeImageIds[] = {
                    interface_skin::image_ids::tab_routes_frame_0,
                    interface_skin::image_ids::tab_routes_frame_1,
                    interface_skin::image_ids::tab_routes_frame_2,
                    interface_skin::image_ids::tab_routes_frame_3,
                };

                uint32_t imageId = skin->img;
                if (self->current_tab == widx::tabRoutes - widx::tabOverall)
                    imageId += routeImageIds[(self->frame_no / 16) % std::size(routeImageIds)];
                else
                    imageId += routeImageIds[0];

                widget::draw_tab(self, dpi, imageId, widx::tabRoutes);
            }
        }

        // tabOwnership,
        {
            if (!(self->disabled_widgets & (1 << widx::tabOwnership)))
            {
                uint32_t imageId = skin->img;
                imageId += interface_skin::image_ids::tab_companies;

                widget::draw_tab(self, dpi, imageId, widx::tabOwnership);
            }
        }
    }

    // 0x0046D273
    static void drawGraphKeyOverall(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t y)
    {
        static uint8_t byte_4FDD5C[] = {
            65,
            125,
            12,
            17,
            186,
            100,
        };

        static string_id lineNames[] = {
            string_ids::map_key_towns,
            string_ids::map_key_industries,
            string_ids::map_key_roads,
            string_ids::map_key_railways,
            string_ids::map_key_stations,
            string_ids::map_key_vegetation,
        };

        for (auto i = 0; i < 6; i++)
        {
            auto colour = byte_4FDD5C[i];
            if (!(self->var_854 & (1 << i)) || !(_word_F2541D & (1 << 2)))
            {
                gfx::draw_rect(dpi, x, y + 3, 5, 5, colour);
            }
            auto args = FormatArguments();
            args.push(lineNames[i]);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::draw_string_494BBF(*dpi, x + 6, y, 94, colour::black, stringId, &args);

            y += 10;
        }
    }

    // 0x0046D379
    static void drawGraphKeyVehicles(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t y)
    {
        static uint8_t byte_4FDD62[] = {
            173,
            103,
            162,
            188,
            21,
            136,
        };

        static string_id lineNames[] = {
            string_ids::forbid_trains,
            string_ids::forbid_buses,
            string_ids::forbid_trucks,
            string_ids::forbid_trams,
            string_ids::forbid_aircraft,
            string_ids::forbid_ships,
        };

        for (auto i = 0; i < 6; i++)
        {
            auto colour = byte_4FDD62[i];
            if (!(self->var_854 & (1 << i)) || !(_word_F2541D & (1 << 2)))
            {
                gfx::draw_rect(dpi, x, y + 3, 5, 5, colour);
            }
            auto args = FormatArguments();
            args.push(lineNames[i]);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::draw_string_494BBF(*dpi, x + 6, y, 94, colour::black, stringId, &args);

            y += 10;
        }
    }

    // 0x0046D47F
    static void drawGraphKeyIndustries(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t y)
    {
        static uint8_t byte_4FB464[] = {
            10,
            14,
            21,
            31,
            41,
            53,
            56,
            63,
            67,
            75,
            80,
            88,
            102,
            113,
            125,
            133,
            137,
            157,
            161,
            163,
            172,
            184,
            187,
            195,
            198,
            208,
            211,
            219,
            222,
            36,
            18,
        };

        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::industry); i++)
        {
            auto colour = byte_4FB464[_byte_F253CE[i]];

            auto industry = objectmgr::get<industry_object>(i);

            if (industry == nullptr)
                return;

            if (!(self->var_854 & (1 << i)) || !(_word_F2541D & (1 << 2)))
            {
                gfx::draw_rect(dpi, x, y + 3, 5, 5, colour);
            }

            auto args = FormatArguments();
            args.push(industry->name);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::draw_string_494BBF(*dpi, x + 6, y, 94, colour::black, stringId, &args);

            y += 10;
        }
    }

    // 0x0046D5A4
    static void drawGraphKeyRoutes(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t y)
    {
        for (auto i = 0; _byte_F253DF[i] != 0xFF; i++)
        {
            auto index = _byte_F253DF[i];
            auto colour = _byte_F253F2[i];

            if (!(self->var_854 & (1 << i)) || !(_word_F2541D & (1 << 2)))
            {
                gfx::draw_rect(dpi, x, y + 3, 5, 5, colour);
            }

            auto routeType = string_ids::map_routes_aircraft;

            if (index != 0xFE)
            {
                routeType = string_ids::map_routes_ships;

                if (index != 0xFD)
                {
                    if (index & (1 << 7))
                    {
                        auto roadObj = objectmgr::get<road_object>(index & ~(1 << 7));
                        routeType = roadObj->name;
                    }
                    else
                    {
                        auto trackObj = objectmgr::get<track_object>(index & ~(1 << 7));
                        routeType = trackObj->name;
                    }
                }
            }

            auto args = FormatArguments();
            args.push(routeType);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::draw_string_494BBF(*dpi, x + 6, y, 94, colour::black, stringId, &args);

            y += 10;
        }
    }

    // 0x0046D6E1
    static void drawGraphKeyCompanies(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t y)
    {
        auto i = 0;
        for (const auto& company : companymgr::companies())
        {
            if (company.empty())
            {
                i++;
                continue;
            }

            auto colour = colour::get_shade(company.mainColours.primary, 6);

            if (!(self->var_854 & (1 << i)) || !(_word_F2541D & (1 << 2)))
            {
                gfx::draw_rect(dpi, x, y + 3, 5, 5, colour);
            }

            auto args = FormatArguments();
            args.push(company.name);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::draw_string_494BBF(*dpi, x + 6, y, 94, colour::black, stringId, &args);

            y += 10;
            i++;
        }
    }

    // 0x0046D81F
    static void formatVehicleString(window* self, FormatArguments args)
    {
        static string_id vehicleStringSingular[] = {
            string_ids::num_trains_singular,
            string_ids::num_buses_singular,
            string_ids::num_trucks_singular,
            string_ids::num_trams_singular,
            string_ids::num_aircrafts_singular,
            string_ids::num_ships_singular,
        };

        static string_id vehicleStringPlural[] = {
            string_ids::num_trains_plural,
            string_ids::num_buses_plural,
            string_ids::num_trucks_plural,
            string_ids::num_trams_plural,
            string_ids::num_aircrafts_plural,
            string_ids::num_ships_plural,
        };

        int16_t vehicleIndex = utility::bitscanforward(self->var_854);

        if (vehicleIndex == -1)
        {
            uint16_t vehicleCount = 0;

            for (auto i = 0; i < 6; i++)
            {
                vehicleCount += _word_F253BA[i];
            }

            auto stringId = string_ids::status_num_vehicles_plural;

            if (vehicleCount == 1)
            {
                stringId = string_ids::status_num_vehicles_singular;
            }

            args.push(stringId);
            args.push(vehicleCount);
        }
        else
        {
            auto vehicleCount = _word_F253BA[vehicleIndex];
            auto stringId = vehicleStringPlural[vehicleIndex];

            if (vehicleCount == 1)
            {
                stringId = vehicleStringSingular[vehicleIndex];
            }

            args.push(stringId);
            args.push(vehicleCount);
        }
    }

    // 0x0046D87C
    static void formatIndustryString(window* self, FormatArguments args)
    {
        int16_t industryIndex = utility::bitscanforward(self->var_854);

        if (industryIndex == -1)
        {
            auto industryCount = 0;
            for (const auto& industry : industrymgr::industries())
            {
                if (industry.empty())
                    continue;

                industryCount++;
            }

            auto stringId = string_ids::status_num_industries_plural;

            if (industryCount == 1)
            {
                stringId = string_ids::status_num_industries_singular;
            }

            args.push(stringId);
            args.push(industryCount);
        }
        else
        {
            auto industryCount = 0;
            for (const auto& industry : industrymgr::industries())
            {
                if (industry.empty())
                    continue;

                if (industry.object_id == industryIndex)
                {
                    industryCount++;
                }
            }

            auto industryObj = objectmgr::get<industry_object>(industryIndex);
            auto stringId = industryObj->namePlural;

            if (industryCount == 1)
            {
                stringId = industryObj->nameSingular;
            }

            auto buffer = stringmgr::get_string(string_ids::buffer_1250);
            char* ptr = (char*)buffer;

            ptr = stringmgr::format_string(ptr, stringId, &industryCount);

            *ptr = ' ';
            ptr++;
            *ptr = '(';
            ptr++;

            if (industryObj->requiresCargo())
            {
                ptr = stringmgr::format_string(ptr, string_ids::industry_require);

                ptr = industryObj->getRequiredCargoString(ptr);

                if (industryObj->producesCargo())
                {
                    ptr = stringmgr::format_string(ptr, string_ids::cargo_to_produce);

                    ptr = industryObj->getProducedCargoString(ptr);
                }
            }
            else if (industryObj->producesCargo())
            {
                ptr = stringmgr::format_string(ptr, string_ids::industry_produce);

                ptr = industryObj->getProducedCargoString(ptr);
            }

            *ptr = ')';
            ptr++;
            *ptr = '\0';

            args.push(string_ids::buffer_1250);
            args.push(industryCount);
        }
    }

    // 0x0046B779
    static void draw(window* self, gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);
        drawTabs(self, dpi);

        {
            auto x = self->x + self->width - 104;
            auto y = self->y + 44;

            switch (self->current_tab + widx::tabOverall)
            {
                case widx::tabOverall:
                    drawGraphKeyOverall(self, dpi, x, y);
                    break;

                case widx::tabVehicles:
                    drawGraphKeyVehicles(self, dpi, x, y);
                    break;

                case widx::tabIndustries:
                    drawGraphKeyIndustries(self, dpi, x, y);
                    break;

                case widx::tabRoutes:
                    drawGraphKeyRoutes(self, dpi, x, y);
                    break;

                case widx::tabOwnership:
                    drawGraphKeyCompanies(self, dpi, x, y);
                    break;
            }

            y -= self->y;
            y += 14;
            y = std::max(y, 92);

            self->min_height = y;
        }

        auto args = FormatArguments();

        switch (self->current_tab + widx::tabOverall)
        {
            case widx::tabOverall:
            case widx::tabRoutes:
            case widx::tabOwnership:
                args.push(string_ids::empty);
                break;

            case widx::tabVehicles:
                formatVehicleString(self, args);
                break;

            case widx::tabIndustries:
                formatIndustryString(self, args);
                break;
        }

        auto x = self->x + self->widgets[widx::statusBar].left - 1;
        auto y = self->y + self->widgets[widx::statusBar].top - 1;
        auto width = self->widgets[widx::statusBar].width();

        gfx::draw_string_494BBF(*dpi, x, y, width, colour::black, string_ids::black_stringid, &args);
    }

    static map_pos rotateMapCoordinate(map_pos pos, uint8_t rotation)
    {
        map_pos coordinate2D;

        switch (rotation)
        {
            default:
            case 0:
                coordinate2D = pos;
                break;
            case 1:
                coordinate2D.x = pos.y;
                coordinate2D.y = -pos.x + 0x2FFF;
                break;
            case 2:
                coordinate2D.x = -pos.x + 0x2FFF;
                coordinate2D.y = -pos.y + 0x2FFF;
                break;
            case 3:
                coordinate2D.x = -pos.y + 0x2FFF;
                coordinate2D.y = pos.x;
                break;
        }

        return coordinate2D;
    }

    // 0x0046BE6E, 0x0046C35A
    static void drawVehiclesOnMap(gfx::drawpixelinfo_t* dpi, bool isCompanyColour)
    {
        for (auto vehicle : thingmgr::VehicleList())
        {
            things::vehicle::Vehicle train(vehicle);

            if (train.head->var_38 & (1 << 4))
                continue;

            if (train.head->x == (1 << 15))
                continue;

            for (auto car : train.cars)
            {
                for (auto carComponent : car.carComponents)
                {
                    auto x = carComponent.front->x;
                    auto y = carComponent.front->y;

                    if (x == (1 << 15))
                        continue;

                    auto trainPos = rotateMapCoordinate({ x, y }, gCurrentRotation);

                    auto left = trainPos.x;
                    auto top = trainPos.y;
                    left /= 32;
                    top /= 32;
                    auto bottom = top;
                    bottom += left;
                    left = -left;
                    left += top;
                    left += 376;
                    bottom -= 8;
                    auto right = left;
                    top = bottom;

                    auto colour = palette_index::index_15;

                    if (isCompanyColour)
                    {
                        auto companyId = car.carComponents[0].front->owner;
                        colour = colour::get_shade(_companyColours[companyId], 7);

                        if (_dword_F253A4 & (1 << companyId))
                        {
                            if (!(_word_F2541D & (1 << 2)))
                            {
                                colour = _byte_4FDC5C[colour];
                            }
                        }
                    }

                    gfx::fill_rect(dpi, left, top, right, bottom, colour);
                }
            }
        }
    }

    // 0x0046BF64
    static void sub_46BF64()
    {
    }

    // 0x0046C0AE
    static void sub_46C0AE()
    {
    }

    // 0x0046BE51, 0x0046BE34
    static void drawRectOnMap(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom, uint32_t colour)
    {
        if (left > right)
        {
            std::swap(left, right);
        }

        if (top > bottom)
        {
            std::swap(top, bottom);
        }

        gfx::fill_rect(dpi, left, top, right, bottom, colour);
    }

    // 0x0046BE51
    static void drawViewOnMap(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        left /= 32;
        top /= 16;
        right /= 32;
        bottom /= 16;
        left += _word_4FDC4C[gCurrentRotation * 2];
        top += _word_4FDC4E[gCurrentRotation * 2];
        right += _word_4FDC4C[gCurrentRotation * 2];
        bottom += _word_4FDC4E[gCurrentRotation * 2];

        uint32_t colour = (1 << 24) | palette_index::index_0A;

        drawRectOnMap(dpi, left, top, right, bottom, colour);
    }

    // 0x0046BE34
    static void drawViewCornersOnMap(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t leftOffset, int16_t topOffset, int16_t rightOffset, int16_t bottomOffset)
    {
        left /= 32;
        top /= 16;
        left += _word_4FDC4C[gCurrentRotation * 2];
        top += _word_4FDC4E[gCurrentRotation * 2];
        auto right = left;
        auto bottom = top;
        left += leftOffset;
        top += topOffset;
        right += rightOffset;
        bottom += bottomOffset;

        uint32_t colour = palette_index::index_0A;

        drawRectOnMap(dpi, left, top, right, bottom, colour);
    }

    // 0x0046BAD5
    static void drawViewportPosition(gfx::drawpixelinfo_t* dpi)
    {
        auto window = WindowManager::getMainWindow();

        if (window == nullptr)
            return;

        auto viewport = window->viewports[0];

        if (viewport == nullptr)
            return;

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;
            auto right = viewport->view_x;
            auto bottom = viewport->view_y;
            right += viewport->view_width;

            drawViewOnMap(dpi, left, top, right, bottom);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;
            top += viewport->view_height;
            auto right = viewport->view_x;
            auto bottom = viewport->view_y;
            right += viewport->view_width;
            bottom += viewport->view_height;

            drawViewOnMap(dpi, left, top, right, bottom);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;
            auto right = viewport->view_x;
            auto bottom = viewport->view_y;
            bottom += viewport->view_height;

            drawViewOnMap(dpi, left, top, right, bottom);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;
            left += viewport->view_width;
            auto right = viewport->view_x;
            auto bottom = viewport->view_y;
            right += viewport->view_width;
            bottom += viewport->view_height;

            drawViewOnMap(dpi, left, top, right, bottom);
        }

        if (!(_word_F2541D & (1 << 2)))
            return;

        if (_dword_F253A4 != 0)
            return;

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;

            drawViewCornersOnMap(dpi, left, top, 0, 0, 5, 0);
        }

        {
            auto left = viewport->view_x;
            left += viewport->view_width;
            auto top = viewport->view_y;

            drawViewCornersOnMap(dpi, left, top, -5, 0, 0, 0);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;

            drawViewCornersOnMap(dpi, left, top, 0, 0, 0, 5);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;
            top += viewport->view_height;

            drawViewCornersOnMap(dpi, left, top, 0, -5, 0, 0);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;
            top += viewport->view_height;

            drawViewCornersOnMap(dpi, left, top, 0, 0, 5, 0);
        }

        {
            auto left = viewport->view_x;
            left += viewport->view_width;
            auto top = viewport->view_y;
            top += viewport->view_height;

            drawViewCornersOnMap(dpi, left, top, -5, 0, 0, 0);
        }

        {
            auto left = viewport->view_x;
            left += viewport->view_width;
            auto top = viewport->view_y;

            drawViewCornersOnMap(dpi, left, top, 0, 0, 0, 5);
        }

        {
            auto left = viewport->view_x;
            left += viewport->view_width;
            auto top = viewport->view_y;
            top += viewport->view_height;

            drawViewCornersOnMap(dpi, left, top, 0, -5, 0, 0);
        }
    }

    // 0x0046C481
    static void drawTownNames(gfx::drawpixelinfo_t* dpi)
    {
        for (const auto& town : townmgr::towns())
        {
            if (town.empty())
                continue;

            auto townPos = rotateMapCoordinate({ town.x, town.y }, gCurrentRotation);

            townPos.x /= 32;
            townPos.y /= 32;
            auto yCopy = townPos.y;
            townPos.y += townPos.x;
            townPos.x = -townPos.x;
            townPos.x += yCopy + 376;
            townPos.y -= 8;

            stringmgr::format_string(_stringFormatBuffer, town.name);
            _currentFontSpriteBase = font::small;

            auto strWidth = gfx::getStringWidth(_stringFormatBuffer);

            strWidth /= 2;

            townPos.x -= strWidth;
            townPos.y -= 3;

            _currentFontSpriteBase = font::small;
            gfx::draw_string(dpi, townPos.x, townPos.y, colour::outline(colour::bright_purple), _stringFormatBuffer);
        }
    }

    // 0x0046B806
    static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        //registers regs;
        //regs.esi = (int32_t)self;
        //regs.edi = (int32_t)dpi;
        //regs.eax = scrollIndex;

        //call(0x0046B806, regs);

        if (!(_dword_525E28 & (1 << 0)))
            return;

        gfx::clear_single(*dpi, palette_index::index_0A);

        auto element = gfx::get_g1element(0);
        auto offset = *_dword_F253A8;

        if (_word_F2541D & (1 << 2))
            offset += 0x90000;

        gfx::get_g1element(0)->offset = offset;
        gfx::get_g1element(0)->width = 0x300;
        gfx::get_g1element(0)->height = 0x300;
        gfx::get_g1element(0)->x_offset = -8;
        gfx::get_g1element(0)->y_offset = -8;
        gfx::get_g1element(0)->flags = 0;

        gfx::draw_image(dpi, 0, 0, 0);

        gfx::get_g1element(0)->offset = element->offset;
        gfx::get_g1element(0)->width = element->width;
        gfx::get_g1element(0)->height = element->height;
        gfx::get_g1element(0)->x_offset = element->x_offset;
        gfx::get_g1element(0)->y_offset = element->y_offset;
        gfx::get_g1element(0)->flags = element->flags;
        gfx::get_g1element(0)->unused = element->unused;

        switch (self->current_tab + widx::tabOverall)
        {
            case widx::tabOverall:
            case widx::tabIndustries:
                drawVehiclesOnMap(dpi, false);
                break;
            case widx::tabVehicles:
                sub_46BF64();
                break;
            case widx::tabRoutes:
                sub_46C0AE();
                break;
            case widx::tabOwnership:
                drawVehiclesOnMap(dpi, true);
        }

        drawViewportPosition(dpi);

        if (self->saved_view.mapX & (1 << 0))
        {
            drawTownNames(dpi);
        }
    }

    static void initEvents()
    {
        events.on_close = onClose;
        events.on_mouse_up = onMouseUp;
        events.on_resize = onResize;
        events.on_update = onUpdate;
        events.get_scroll_size = getScrollSize;
        events.scroll_mouse_down = event18;
        events.event_18 = (uint32_t)0x0046B97C;
        events.tooltip = tooltip;
        events.prepare_draw = prepareDraw;
        events.draw = draw;
        events.draw_scroll = drawScroll;
    }

    static void sub_46CFF0()
    {
        registers regs;
        call(0x0046CFF0, regs);
    }
    static void sub_46CED0()
    {
        registers regs;
        call(0x0046CED0, regs);
    }

    // 0x0046B490
    void open()
    {
        auto window = WindowManager::bringToFront(WindowType::map, 0);

        if (window != nullptr)
            return;

        addr<0x00113E87C, int32_t>() = 3;
        auto ptr = malloc(0x120000);
        addr<0x00113E87C, int32_t>() = 0;

        if (ptr == NULL)
            return;

        _dword_F253A8 = static_cast<uint8_t*>(ptr);
        gfx::ui_size_t size = { 350, 272 };

        if (_dword_526284 != 0)
        {
            size = _word_526288;
        }

        window = WindowManager::createWindow(WindowType::map, size, 0, &events);
        window->widgets = widgets;
        window->enabled_widgets |= enabledWidgets;
        window->init_scroll_widgets();
        window->frame_no = 0;

        if (_dword_526284 != 0)
        {
            window->var_88A = _word_52628C;
            window->var_88C = _word_52628E;
            window->flags |= (_dword_526284 & window_flags::flag_16);
        }

        initEvents();

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0F;

        window->var_846 = gCurrentRotation;

        sub_46B69C();

        centerOnViewPoint();

        window->current_tab = 0;
        window->saved_view.mapX = 1;
        window->var_854 = 0;
        window->var_856 = 0;

        sub_46CFF0();
        sub_46CED0();

        _word_F2541D = 0;
    }

    // 0x0046B5C0
    void centerOnViewPoint()
    {
        auto mainWindow = WindowManager::getMainWindow();

        if (mainWindow == nullptr)
            return;

        auto viewport = mainWindow->viewports[0];

        if (viewport == nullptr)
            return;

        auto window = WindowManager::find(WindowType::map, 0);

        if (window == nullptr)
            return;

        auto x = viewport->view_width / 2;
        auto y = viewport->view_height / 2;
        x += viewport->view_x;
        y += viewport->view_y;
        x /= 16;
        y /= 8;
        x += _word_4FDC4C[gCurrentRotation * 2];
        y += _word_4FDC4E[gCurrentRotation * 2];

        auto width = widgets[widx::scrollview].width() - 10;
        auto height = widgets[widx::scrollview].height() - 10;
        x -= width / 2;
        x = std::max(x, 0);
        y -= height / 2;
        y = std::max(y, 0);

        width = -width;
        height = -height;
        width += window->scroll_areas[0].h_right;
        height += window->scroll_areas[0].v_bottom;

        width -= x;
        if (x > width)
        {
            x += width;
            x = std::max(x, 0);
        }

        height -= y;
        if (y > height)
        {
            y += height;
            y = std::max(y, 0);
        }

        window->scroll_areas[0].h_right = x;
        window->scroll_areas[0].v_bottom = y;

        ui::scrollview::update_thumbs(window, widx::scrollview);
    }
}
