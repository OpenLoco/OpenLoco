#include "../CompanyManager.h"
#include "../IndustryManager.h"
#include "../Input.h"
#include "../StationManager.h"
#include "../TownManager.h"
#include "../Types.hpp"
#include "../Widget.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../objects/industry_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/road_object.h"
#include "../objects/track_object.h"
#include "../things/thing.h"
#include "../things/thingmgr.h"
#include "../ui/WindowManager.h"
#include "../ui/scrollview.h"

using namespace openloco::interop;
using namespace openloco::ui::WindowManager;

namespace openloco::ui::windows::map
{
    static loco_global<int32_t, 0x00523338> _cursorX2;
    static loco_global<int32_t, 0x0052333C> _cursorY2;
    static std::array<int16_t, 4> _4FDC4C = {
        {
            376,
            760,
            376,
            -8,
        }
    };
    static std::array<int16_t, 4> _4FDC4E = {
        {
            0,
            384,
            768,
            384,
        }
    };
    static loco_global<uint8_t[256], 0x004FDC5C> _byte_4FDC5C;
    static loco_global<uint32_t, 0x0526284> _lastMapWindowFlags;
    static loco_global<gfx::ui_size_t, 0x00526288> _lastMapWindowSize;
    static loco_global<uint16_t, 0x0052628C> _lastMapWindowVar88A;
    static loco_global<uint16_t, 0x0052628E> _lastMapWindowVar88C;
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;
    static loco_global<uint32_t, 0x00F253A4> _dword_F253A4;
    static loco_global<uint8_t*, 0x00F253A8> _dword_F253A8;
    static std::array<uint16_t, 6> _vehicleTypeCounts = {
        {
            0,
            0,
            0,
            0,
            0,
            0,
        }
    };
    static loco_global<uint8_t[16], 0x00F253CE> _byte_F253CE;
    static loco_global<uint8_t[19], 0x00F253DF> _byte_F253DF;
    static loco_global<uint8_t[19], 0x00F253F2> _routeColours;
    static loco_global<uint32_t, 0x00525E28> _dword_525E28;
    static loco_global<company_id_t, 0x00525E3C> _playerCompanyId;
    constexpr uint32_t max_orders = 256000;
    static loco_global<uint8_t[max_orders], 0x00987C5C> _dword_987C5C; // ?orders? ?routing related?
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
        makeWidget({ 0, 0 }, { 350, 272 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 348, 13 }, widget_type::caption_25, 0),
        makeWidget({ 335, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window),
        makeWidget({ 0, 41 }, { 350, 230 }, widget_type::panel, 1),
        makeRemapWidget({ 3, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_overall),
        makeRemapWidget({ 34, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_vehicles),
        makeRemapWidget({ 65, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_industries),
        makeRemapWidget({ 96, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_routes),
        makeRemapWidget({ 158, 15 }, { 31, 27 }, widget_type::wt_6, 1, image_ids::tab, string_ids::tab_map_ownership),
        makeWidget({ 3, 44 }, { 240, 215 }, widget_type::scrollview, 1, horizontal | vertical),
        makeWidget({ 3, 250 }, { 322, 21 }, widget_type::wt_13, 1),
        widgetEnd()
    };

    static window_event_list events;

    static map_pos mapWindowPosToLocation(xy32 pos)
    {
        pos.x = ((pos.x + 8) - map_columns) / 2;
        pos.y = ((pos.y + 8)) / 2;
        map_pos location = { static_cast<coord_t>(pos.y - pos.x), static_cast<coord_t>(pos.x + pos.y) };
        location.x *= tile_size;
        location.y *= tile_size;

        switch (getCurrentRotation())
        {
            case 0:
                return location;
            case 1:
                return { static_cast<coord_t>(map_width - 1 - location.y), location.x };
            case 2:
                return { static_cast<coord_t>(map_width - 1 - location.x), static_cast<coord_t>(map_height - 1 - location.y) };
            case 3:
                return { location.y, static_cast<coord_t>(map_height - 1 - location.x) };
        }

        return { 0, 0 }; // unreachable
    }

    static xy32 locationToMapWindowPos(map_pos pos)
    {
        int32_t x = pos.x;
        int32_t y = pos.y;

        switch (getCurrentRotation())
        {
            case 3:
                std::swap(x, y);
                x = map_width - 1 - x;
                break;
            case 2:
                x = map_width - 1 - x;
                y = map_height - 1 - y;
                break;
            case 1:
                std::swap(x, y);
                y = map_height - 1 - y;
                break;
            case 0:
                break;
        }

        x /= tile_size;
        y /= tile_size;

        return { static_cast<int32_t>(-x + y + map_columns - 8), static_cast<int32_t>(x + y - 8) };
    }

    // 0x0046B8E6
    static void onClose(window* self)
    {
        _lastMapWindowSize = gfx::ui_size_t(self->width, self->height);
        _lastMapWindowVar88A = self->var_88A;
        _lastMapWindowVar88C = self->var_88C;
        _lastMapWindowFlags = self->flags | window_flags::flag_31;

        free(_dword_F253A8);
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
        self->setSize(minWindowSize, maxWindowSize);
    }

    // 0x0046C544
    static void sub_46C544(window* self)
    {
        registers regs;
        regs.esi = (int32_t)self;
        call(0x0046C544, regs);
    }

    // 0x0046D34D based on
    static void setHoverItem(window* self, int16_t y, int index)
    {
        uint32_t itemHover;

        if (y < 0)
        {
            itemHover = (1 << index);
        }
        else
        {
            itemHover = 0;
        }

        if (itemHover != self->var_854)
        {
            self->var_854 = itemHover;
            self->invalidate();
        }

        if (self->var_854 != 0)
        {
            self->invalidate();
        }
    }

    static uint8_t legendWidth = 100;
    static uint8_t legendItemHeight = 10;
    static const uint8_t overallGraphKeySize = 6;

    static std::array<size_t, 5> legendLengths = {
        {
            overallGraphKeySize,
            std::size(_vehicleTypeCounts),
            objectmgr::get_max_objects(object_type::industry),
            0,
            companymgr::max_companies,
        }
    };

    static void setHoverItemTab(window* self, int16_t legendLeft, int16_t legendBottom)
    {
        uint8_t i = 0;
        int16_t y = 0;
        if (!input::hasFlag(input::input_flags::flag5))
        {
            uint32_t cursorX = _cursorX2;
            uint32_t cursorY = _cursorY2;
            auto window = WindowManager::findAt(cursorX, cursorY);

            if (window == self)
            {
                cursorX -= legendLeft;
                if (cursorX <= legendWidth)
                {
                    cursorY -= legendBottom;
                    if (self->current_tab == (widx::tabRoutes - widx::tabOverall))
                    {
                        y = cursorY;

                        for (; _byte_F253DF[i] != 0xFF; i++)
                        {
                            y -= legendItemHeight;

                            if (y < 0)
                            {
                                break;
                            }
                        }
                    }
                    else
                    {
                        if (cursorY < legendLengths[self->current_tab] * legendItemHeight)
                        {
                            y = cursorY;

                            for (; i < legendLengths[self->current_tab]; i++)
                            {
                                if (self->current_tab == (widx::tabIndustries - widx::tabOverall))
                                {
                                    auto industryObj = objectmgr::get<industry_object>(i);

                                    if (industryObj == nullptr)
                                        continue;
                                }
                                else if (self->current_tab == (widx::tabOwnership - widx::tabOverall))
                                {
                                    auto company = companymgr::get(i);

                                    if (company->empty())
                                        continue;
                                }

                                y -= legendItemHeight;

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

        setHoverItem(self, y, i);
    }

    // 0x0046B69C
    static void clearMap()
    {
        std::fill(static_cast<uint8_t*>(_dword_F253A8), _dword_F253A8 + 0x120000, palette_index::index_0A);
    }

    // 0x00F2541D
    static uint16_t mapFrameNumber = 0;

    // 0x0046BA5B
    static void onUpdate(window* self)
    {
        self->frame_no++;
        self->callPrepareDraw();

        WindowManager::invalidateWidget(WindowType::map, self->number, self->current_tab + widx::tabOverall);

        mapFrameNumber++;

        if (getCurrentRotation() != self->var_846)
        {
            self->var_846 = getCurrentRotation();
            clearMap();
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

        setHoverItemTab(self, x, y);
    }

    // 0x0046B9E7
    static void getScrollSize(window* self, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight)
    {
        self->callPrepareDraw();
        *scrollWidth = map_columns * 2;
        *scrollHeight = map_rows * 2;
    }

    // 0x0046B9D4
    static void moveMainViewToMapView(map_pos pos)
    {
        auto z = tileElementHeight(pos.x, pos.y).landHeight;
        auto window = WindowManager::getMainWindow();

        if (window == nullptr)
            return;

        window->viewportCentreOnTile({ static_cast<coord_t>(pos.x), static_cast<coord_t>(pos.y), static_cast<coord_t>(z) });
    }

    // 0x0046B97C
    static void scrollMouseDown(window* self, int16_t x, int16_t y, uint8_t scrollIndex)
    {
        auto pos = mapWindowPosToLocation({ x, y });

        moveMainViewToMapView(pos);
    }

    // 0x0046B946
    static void tooltip(FormatArguments& args, window* self, widget_index widgetIndex)
    {
        args.push(string_ids::tooltip_scroll_map);
    }

    // 0x0046D223
    static void leftAlignTabs(window* self, uint8_t firstTabIndex, uint8_t lastTabIndex)
    {
        auto disabledWidgets = self->disabled_widgets;
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

        widgets[widx::caption].text = captionText[self->current_tab];
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

        if (isEditorMode())
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

                if (!isEditorMode())
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
    static void drawGraphKeyOverall(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t* y)
    {
        static const palette_index_t overallColours[] = {
            palette_index::index_41,
            palette_index::index_7D,
            palette_index::index_0C,
            palette_index::index_11,
            palette_index::index_BA,
            palette_index::index_64,
        };

        static const string_id lineNames[] = {
            string_ids::map_key_towns,
            string_ids::map_key_industries,
            string_ids::map_key_roads,
            string_ids::map_key_railways,
            string_ids::map_key_stations,
            string_ids::map_key_vegetation,
        };

        for (auto i = 0; i < overallGraphKeySize; i++)
        {
            auto colour = overallColours[i];
            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                gfx::drawRect(dpi, x, *y + 3, 5, 5, colour);
            }
            auto args = FormatArguments();
            args.push(lineNames[i]);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::drawString_494BBF(*dpi, x + 6, *y, 94, colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x004FDD62
    static const palette_index_t vehicleTypeColours[] = {
        palette_index::index_AD,
        palette_index::index_67,
        palette_index::index_A2,
        palette_index::index_BC,
        palette_index::index_15,
        palette_index::index_B8, // changed from 136 to make ships more viewable on the map
    };

    // 0x0046D379
    static void drawGraphKeyVehicles(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t* y)
    {
        static const string_id lineNames[] = {
            string_ids::forbid_trains,
            string_ids::forbid_buses,
            string_ids::forbid_trucks,
            string_ids::forbid_trams,
            string_ids::forbid_aircraft,
            string_ids::forbid_ships,
        };

        for (uint8_t i = 0; i < std::size(_vehicleTypeCounts); i++)
        {
            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                auto colour = vehicleTypeColours[i];

                gfx::drawRect(dpi, x, *y + 3, 5, 5, colour);
            }
            auto args = FormatArguments();
            args.push(lineNames[i]);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::drawString_494BBF(*dpi, x + 6, *y, 94, colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x0046D47F
    static void drawGraphKeyIndustries(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t* y)
    {
        static const palette_index_t industryColours[] = {
            palette_index::index_0A,
            palette_index::index_0E,
            palette_index::index_15,
            palette_index::index_1F,
            palette_index::index_29,
            palette_index::index_35,
            palette_index::index_38,
            palette_index::index_3F,
            palette_index::index_43,
            palette_index::index_4B,
            palette_index::index_50,
            palette_index::index_58,
            palette_index::index_66,
            palette_index::index_71,
            palette_index::index_7D,
            palette_index::index_85,
            palette_index::index_89,
            palette_index::index_9D,
            palette_index::index_A1,
            palette_index::index_A3,
            palette_index::index_AC,
            palette_index::index_B8,
            palette_index::index_BB,
            palette_index::index_C3,
            palette_index::index_C6,
            palette_index::index_D0,
            palette_index::index_D3,
            palette_index::index_DB,
            palette_index::index_DE,
            palette_index::index_24,
            palette_index::index_12,
        };

        for (uint8_t i = 0; i < objectmgr::get_max_objects(object_type::industry); i++)
        {
            auto industry = objectmgr::get<industry_object>(i);

            if (industry == nullptr)
                continue;

            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                auto colour = industryColours[_byte_F253CE[i]];

                gfx::drawRect(dpi, x, *y + 3, 5, 5, colour);
            }

            auto args = FormatArguments();
            args.push(industry->name);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << i))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::drawString_494BBF(*dpi, x + 6, *y, 94, colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x0046D5A4
    static void drawGraphKeyRoutes(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t* y)
    {
        for (auto i = 0; _byte_F253DF[i] != 0xFF; i++)
        {
            auto index = _byte_F253DF[i];
            auto colour = _routeColours[i];

            if (!(self->var_854 & (1 << i)) || !(mapFrameNumber & (1 << 2)))
            {
                gfx::drawRect(dpi, x, *y + 3, 5, 5, colour);
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
                        auto trackObj = objectmgr::get<track_object>(index);
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

            gfx::drawString_494BBF(*dpi, x + 6, *y, 94, colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x0046D6E1
    static void drawGraphKeyCompanies(window* self, gfx::drawpixelinfo_t* dpi, uint16_t x, uint16_t* y)
    {
        for (const auto& company : companymgr::companies())
        {
            if (company.empty())
            {
                continue;
            }

            auto index = company.id();
            auto colour = colour::getShade(company.mainColours.primary, 6);

            if (!(self->var_854 & (1 << index)) || !(mapFrameNumber & (1 << 2)))
            {
                gfx::drawRect(dpi, x, *y + 3, 5, 5, colour);
            }

            auto args = FormatArguments();
            args.push(company.name);

            auto stringId = string_ids::small_black_string;

            if (self->var_854 & (1 << index))
            {
                stringId = string_ids::small_white_string;
            }

            gfx::drawString_494BBF(*dpi, x + 6, *y, 94, colour::black, stringId, &args);

            *y += 10;
        }
    }

    // 0x0046D81F
    static void formatVehicleString(window* self, FormatArguments args)
    {
        static const string_id vehicleStringSingular[] = {
            string_ids::num_trains_singular,
            string_ids::num_buses_singular,
            string_ids::num_trucks_singular,
            string_ids::num_trams_singular,
            string_ids::num_aircrafts_singular,
            string_ids::num_ships_singular,
        };

        static const string_id vehicleStringPlural[] = {
            string_ids::num_trains_plural,
            string_ids::num_buses_plural,
            string_ids::num_trucks_plural,
            string_ids::num_trams_plural,
            string_ids::num_aircrafts_plural,
            string_ids::num_ships_plural,
        };

        int16_t vehicleIndex = utility::bitScanForward(self->var_854);
        uint16_t totalVehicleCount = 0;
        auto stringId = string_ids::status_num_vehicles_plural;

        if (vehicleIndex == -1)
        {
            for (auto i = 0; i < 6; i++)
            {
                totalVehicleCount += _vehicleTypeCounts[i];
            }

            if (totalVehicleCount == 1)
            {
                stringId = string_ids::status_num_vehicles_singular;
            }
        }
        else
        {
            totalVehicleCount = _vehicleTypeCounts[vehicleIndex];
            stringId = vehicleStringPlural[vehicleIndex];

            if (totalVehicleCount == 1)
            {
                stringId = vehicleStringSingular[vehicleIndex];
            }
        }

        args.push(stringId);
        args.push(totalVehicleCount);
    }

    // 0x0046D87C
    static void formatIndustryString(window* self, FormatArguments args)
    {
        int16_t industryIndex = utility::bitScanForward(self->var_854);

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

            auto buffer = stringmgr::getString(string_ids::buffer_1250);
            char* ptr = const_cast<char*>(buffer);

            ptr = stringmgr::formatString(ptr, stringId, &industryCount);

            *ptr++ = ' ';
            *ptr++ = '(';

            if (industryObj->requiresCargo())
            {
                ptr = stringmgr::formatString(ptr, string_ids::industry_require);

                ptr = industryObj->getRequiredCargoString(ptr);

                if (industryObj->producesCargo())
                {
                    ptr = stringmgr::formatString(ptr, string_ids::cargo_to_produce);

                    ptr = industryObj->getProducedCargoString(ptr);
                }
            }
            else if (industryObj->producesCargo())
            {
                ptr = stringmgr::formatString(ptr, string_ids::industry_produce);

                ptr = industryObj->getProducedCargoString(ptr);
            }

            *ptr++ = ')';
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
            uint16_t y = self->y + 44;

            switch (self->current_tab + widx::tabOverall)
            {
                case widx::tabOverall:
                    drawGraphKeyOverall(self, dpi, x, &y);
                    break;

                case widx::tabVehicles:
                    drawGraphKeyVehicles(self, dpi, x, &y);
                    break;

                case widx::tabIndustries:
                    drawGraphKeyIndustries(self, dpi, x, &y);
                    break;

                case widx::tabRoutes:
                    drawGraphKeyRoutes(self, dpi, x, &y);
                    break;

                case widx::tabOwnership:
                    drawGraphKeyCompanies(self, dpi, x, &y);
                    break;
            }

            y -= self->y;
            y += 14;
            y = std::max(y, static_cast<uint16_t>(92));

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

        gfx::drawString_494BBF(*dpi, x, y, width, colour::black, string_ids::black_stringid, &args);
    }

    // 0x0046BF0F based on
    static void drawVehicleOnMap(gfx::drawpixelinfo_t* dpi, vehicle_base* vehicle, uint8_t colour)
    {
        if (vehicle->x == location::null)
            return;

        auto trainPos = locationToMapWindowPos({ vehicle->x, vehicle->y });

        gfx::fillRect(dpi, trainPos.x, trainPos.y, trainPos.x, trainPos.y, colour);
    }

    // 0x0046C294
    static std::pair<xy32, xy32> drawRouteLine(gfx::drawpixelinfo_t* dpi, xy32 startPos, xy32 endPos, map_pos stationPos, uint8_t colour)
    {
        auto newStartPos = locationToMapWindowPos({ stationPos.x, stationPos.y });

        if (endPos.x != location::null)
        {
            gfx::drawLine(dpi, endPos.x, endPos.y, newStartPos.x, newStartPos.y, colour);
        }

        endPos = newStartPos;

        if (startPos.x == location::null)
        {
            startPos = newStartPos;
        }

        return std::make_pair(startPos, endPos);
    }

    static std::optional<uint8_t> getRouteColour(things::vehicle::Vehicle train)
    {
        uint8_t colour;
        if (train.head->vehicleType == VehicleType::plane)
        {
            colour = 211;
            auto index = utility::bitScanForward(_dword_F253A4);
            if (index != -1)
            {
                if (_byte_F253DF[index] == 0xFE)
                {
                    if (mapFrameNumber & (1 << 2))
                    {
                        colour = _byte_4FDC5C[colour];
                    }
                }
            }
        }
        else if (train.head->vehicleType == VehicleType::ship)
        {
            colour = 139;
            auto index = utility::bitScanForward(_dword_F253A4);
            if (index != -1)
            {
                if (_byte_F253DF[index] == 0xFD)
                {
                    if (mapFrameNumber & (1 << 2))
                    {
                        colour = _byte_4FDC5C[colour];
                    }
                }
            }
        }
        else
        {
            return std::nullopt;
        }

        return colour;
    }

    // 0x0046C18D
    static void drawRoutesOnMap(gfx::drawpixelinfo_t* dpi, things::vehicle::Vehicle train)
    {
        auto colour = getRouteColour(train);

        if (!colour)
            return;

        static const uint8_t byte_4FE088[] = {
            0,
            11,
            11,
            3,
            4,
            4,
            0,
            0,
        };

        static const uint8_t dword_4FE070[] = {
            1,
            2,
            2,
            6,
            1,
            1,
        };

        xy32 startPos = { location::null, 0 };
        xy32 endPos = { location::null, 0 };
        auto index = train.head->length_of_var_4C;
        auto lastOrder = _dword_987C5C[index] & 0x7;

        while (lastOrder != 0)
        {
            if (byte_4FE088[lastOrder] & (1 << 3))
            {
                auto order = _dword_987C5C[index] & 0xC0;
                order <<= 2;
                order |= _dword_987C5C[index + 1];

                auto station = stationmgr::get(order);
                map_pos stationPos = { station->x, station->y };

                auto routePos = drawRouteLine(dpi, startPos, endPos, stationPos, *colour);
                startPos = routePos.first;
                endPos = routePos.second;
            }

            index += dword_4FE070[lastOrder];
            lastOrder = _dword_987C5C[index] & 0x7;
        }

        if (startPos.x == location::null || endPos.x == location::null)
            return;

        gfx::drawLine(dpi, startPos.x, startPos.y, endPos.x, endPos.y, *colour);
    }

    // 0x0046C426
    static uint8_t getVehicleColour(widget_index widgetIndex, things::vehicle::Vehicle train, things::vehicle::Car car)
    {
        auto colour = palette_index::index_15;

        if (widgetIndex == widx::tabOwnership || widgetIndex == widx::tabVehicles)
        {
            uint8_t index = car.front->owner;
            colour = colour::getShade(_companyColours[index], 7);

            if (widgetIndex == widx::tabVehicles)
            {
                index = static_cast<uint8_t>(train.head->vehicleType);
                colour = vehicleTypeColours[index];
            }

            if (_dword_F253A4 & (1 << index))
            {
                if (!(mapFrameNumber & (1 << 2)))
                {
                    colour = _byte_4FDC5C[colour];
                }
            }
        }

        return colour;
    }

    // 0x0046BFAD
    static void countVehiclesOnMap()
    {
        for (auto i = 0; i < 6; i++)
        {
            _vehicleTypeCounts[i] = 0;
        }

        for (auto vehicle : thingmgr::VehicleList())
        {
            things::vehicle::Vehicle train(vehicle);

            if (train.head->var_38 & (1 << 4))
                continue;

            if (train.head->x == location::null)
                continue;

            auto vehicleType = train.head->vehicleType;
            _vehicleTypeCounts[static_cast<uint8_t>(vehicleType)] = _vehicleTypeCounts[static_cast<uint8_t>(vehicleType)] + 1;
        }
    }

    // 0x0046BE6E, 0x0046C35A
    static void drawVehiclesOnMap(gfx::drawpixelinfo_t* dpi, widget_index widgetIndex)
    {
        for (auto vehicle : thingmgr::VehicleList())
        {
            things::vehicle::Vehicle train(vehicle);

            if (train.head->var_38 & (1 << 4))
                continue;

            if (train.head->x == location::null)
                continue;

            for (auto& car : train.cars)
            {
                auto colour = getVehicleColour(widgetIndex, train, car);

                for (auto& carComponent : car)
                {
                    drawVehicleOnMap(dpi, carComponent.front, colour);
                    drawVehicleOnMap(dpi, carComponent.back, colour);
                    drawVehicleOnMap(dpi, carComponent.body, colour);
                }
            }

            if (widgetIndex == widx::tabRoutes)
            {
                drawRoutesOnMap(dpi, train);
            }
        }
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

        gfx::fillRect(dpi, left, top, right, bottom, colour);
    }

    // 0x0046BE51
    static void drawViewOnMap(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t right, int16_t bottom)
    {
        left /= 32;
        top /= 16;
        right /= 32;
        bottom /= 16;
        left += _4FDC4C[getCurrentRotation()];
        top += _4FDC4E[getCurrentRotation()];
        right += _4FDC4C[getCurrentRotation()];
        bottom += _4FDC4E[getCurrentRotation()];

        uint32_t colour = (1 << 24) | palette_index::index_0A;

        drawRectOnMap(dpi, left, top, right, bottom, colour);
    }

    // 0x0046BE34
    static void drawViewCornersOnMap(gfx::drawpixelinfo_t* dpi, int16_t left, int16_t top, int16_t leftOffset, int16_t topOffset, int16_t rightOffset, int16_t bottomOffset)
    {
        left /= 32;
        top /= 16;
        left += _4FDC4C[getCurrentRotation()];
        top += _4FDC4E[getCurrentRotation()];
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

        if (!(mapFrameNumber & (1 << 2)))
            return;

        if (_dword_F253A4 != 0)
            return;

        uint8_t cornerSize = 5;

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;

            drawViewCornersOnMap(dpi, left, top, 0, 0, cornerSize, 0);
        }

        {
            auto left = viewport->view_x;
            left += viewport->view_width;
            auto top = viewport->view_y;

            drawViewCornersOnMap(dpi, left, top, -cornerSize, 0, 0, 0);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;

            drawViewCornersOnMap(dpi, left, top, 0, 0, 0, cornerSize);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;
            top += viewport->view_height;

            drawViewCornersOnMap(dpi, left, top, 0, -cornerSize, 0, 0);
        }

        {
            auto left = viewport->view_x;
            auto top = viewport->view_y;
            top += viewport->view_height;

            drawViewCornersOnMap(dpi, left, top, 0, 0, cornerSize, 0);
        }

        {
            auto left = viewport->view_x;
            left += viewport->view_width;
            auto top = viewport->view_y;
            top += viewport->view_height;

            drawViewCornersOnMap(dpi, left, top, -cornerSize, 0, 0, 0);
        }

        {
            auto left = viewport->view_x;
            left += viewport->view_width;
            auto top = viewport->view_y;

            drawViewCornersOnMap(dpi, left, top, 0, 0, 0, cornerSize);
        }

        {
            auto left = viewport->view_x;
            left += viewport->view_width;
            auto top = viewport->view_y;
            top += viewport->view_height;

            drawViewCornersOnMap(dpi, left, top, 0, -cornerSize, 0, 0);
        }
    }

    // 0x0046C481
    static void drawTownNames(gfx::drawpixelinfo_t* dpi)
    {
        for (const auto& town : townmgr::towns())
        {
            if (town.empty())
                continue;

            auto townPos = locationToMapWindowPos({ town.x, town.y });

            stringmgr::formatString(_stringFormatBuffer, town.name);
            _currentFontSpriteBase = font::small;

            auto strWidth = gfx::getStringWidth(_stringFormatBuffer);

            strWidth /= 2;

            townPos.x -= strWidth;
            townPos.y -= 3;

            _currentFontSpriteBase = font::small;
            gfx::drawString(dpi, townPos.x, townPos.y, colour::outline(colour::bright_purple), _stringFormatBuffer);
        }
    }

    // 0x0046B806
    static void drawScroll(window* self, gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex)
    {
        if (!(_dword_525E28 & (1 << 0)))
            return;

        gfx::clearSingle(*dpi, palette_index::index_0A);

        auto element = gfx::getG1Element(0);
        auto backupElement = *element;
        auto offset = *_dword_F253A8;

        if (mapFrameNumber & (1 << 2))
            offset += 0x90000;

        gfx::getG1Element(0)->offset = offset;
        gfx::getG1Element(0)->width = map_columns * 2;
        gfx::getG1Element(0)->height = map_rows * 2;
        gfx::getG1Element(0)->x_offset = -8;
        gfx::getG1Element(0)->y_offset = -8;
        gfx::getG1Element(0)->flags = 0;

        gfx::drawImage(dpi, 0, 0, 0);

        *element = backupElement;

        if (self->current_tab + widx::tabOverall == widx::tabVehicles)
        {
            countVehiclesOnMap();
        }

        drawVehiclesOnMap(dpi, self->current_tab + widx::tabOverall);

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
        events.scroll_mouse_down = scrollMouseDown;
        events.scroll_mouse_drag = scrollMouseDown;
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

        auto ptr = malloc(map_size * 8);

        if (ptr == NULL)
            return;

        _dword_F253A8 = static_cast<uint8_t*>(ptr);
        gfx::ui_size_t size = { 350, 272 };

        if (_lastMapWindowFlags != 0)
        {
            size = _lastMapWindowSize;
        }

        window = WindowManager::createWindow(WindowType::map, size, 0, &events);
        window->widgets = widgets;
        window->enabled_widgets |= enabledWidgets;

        initEvents();

        window->initScrollWidgets();
        window->frame_no = 0;

        if (_lastMapWindowFlags != 0)
        {
            window->var_88A = _lastMapWindowVar88A;
            window->var_88C = _lastMapWindowVar88C;
            window->flags |= (_lastMapWindowFlags & window_flags::flag_16);
        }

        auto skin = objectmgr::get<interface_skin_object>();
        window->colours[0] = skin->colour_0B;
        window->colours[1] = skin->colour_0F;

        window->var_846 = getCurrentRotation();

        clearMap();

        centerOnViewPoint();

        window->current_tab = 0;
        window->saved_view.mapX = 1;
        window->var_854 = 0;
        window->var_856 = 0;

        sub_46CFF0();
        sub_46CED0();

        mapFrameNumber = 0;
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
        x /= 32;
        y /= 16;
        x += _4FDC4C[getCurrentRotation()];
        y += _4FDC4E[getCurrentRotation()];

        auto width = widgets[widx::scrollview].width() - 10;
        auto height = widgets[widx::scrollview].height() - 10;
        x -= width / 2;
        x = std::max(x, 0);
        y -= height / 2;
        y = std::max(y, 0);

        width = -width;
        height = -height;
        width += window->scroll_areas[0].contentWidth;
        height += window->scroll_areas[0].contentHeight;

        width -= x;
        if (width < 0)
        {
            x += width;
            x = std::max(x, 0);
        }

        height -= y;
        if (height < 0)
        {
            y += height;
            y = std::max(y, 0);
        }

        window->scroll_areas[0].contentOffsetX = x;
        window->scroll_areas[0].contentOffsetY = y;

        ui::scrollview::updateThumbs(window, widx::scrollview);
    }
}
