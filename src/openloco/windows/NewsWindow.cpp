#include "../audio/audio.h"
#include "../companymgr.h"
#include "../config.h"
#include "../date.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../industrymgr.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../message.h"
#include "../messagemgr.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/vehicle_object.h"
#include "../openloco.h"
#include "../stationmgr.h"
#include "../things/thingmgr.h"
#include "../townmgr.h"
#include "../ui.h"
#include "../ui/WindowManager.h"
#include "../viewportmgr.h"

using namespace openloco::interop;

namespace openloco::ui::NewsWindow
{
    static loco_global<uint8_t[31][4], 0x004F8B08> _byte_4F8B08;
    static loco_global<uint8_t[31][4], 0x004F8B09> _byte_4F8B09;
    static loco_global<uint16_t[31], 0x004F8BE4> _word_4F8BE4;
    static loco_global<uint8_t[31], 0x004F8C22> _messageTypes;
    static loco_global<uint8_t[31], 0x004F8C41> _messageSounds;
    static loco_global<uint8_t, 0x00508F1A> _gameSpeed;
    static loco_global<uint8_t[3], 0x005215B5> _unk_5215B5;
    static loco_global<uint32_t, 0x00523338> _cursorX2;
    static loco_global<uint32_t, 0x0052333C> _cursorY2;
    static loco_global<uint32_t, 0x00525CD0> _dword_525CD0;
    static loco_global<uint32_t, 0x00525CD4> _dword_525CD4;
    static loco_global<uint32_t, 0x00525CD8> _dword_525CD8;
    static loco_global<uint32_t, 0x00525CDC> _dword_525CDC;
    static loco_global<uint16_t, 0x00525CE0> _word_525CE0;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;
    static loco_global<uint16_t, 0x005271CE> _messageCount;
    static loco_global<uint16_t, 0x005271D0> _activeMessageIndex;
    static loco_global<int32_t, 0x00E3F0B8> gCurrentRotation;
    static loco_global<int16_t, 0x112C876> _currentFontSpriteBase;
    static loco_global<char[512], 0x0112CC04> byte_112CC04;
    static loco_global<uint32_t, 0x011364EC> _numTrackTypeTabs;
    static loco_global<int8_t[8], 0x011364F0> _trackTypesForTab;

    namespace common
    {
        enum newsType
        {
            none = 0,
            ticker,
            newsWindow,
        };

        enum newsItem
        {
            majorCompany,
            majorCompetitor,
            minorCompany,
            minorCompetitor,
            general,
            advice,
        };

        enum widx
        {
            frame,
            close_button,
            viewport1,
            viewport2,
            viewport1Button,
            viewport2Button,
        };

        const uint64_t enabledWidgets = (1 << close_button) | (1 << viewport1Button) | (1 << viewport2Button);

#define commonWidgets(frameWidth, frameHeight, frameType)                                                                                 \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, frameType, 0),                                                                     \
        make_widget({ frameWidth - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, image_ids::close_button, string_ids::tooltip_close_window), \
        make_widget({ 2, frameHeight - 73 }, { 168, 64 }, widget_type::viewport, 0, 0xFFFFFFFE),                                          \
        make_widget({ 180, frameHeight - 73 }, { 168, 64 }, widget_type::viewport, 0, 0xFFFFFFFE),                                        \
        make_widget({ 2, frameHeight - 75 }, { 180, 75 }, widget_type::wt_9, 0),                                                          \
        make_widget({ 2, frameHeight - 75 }, { 180, 75 }, widget_type::wt_9, 0)

        static void initEvents();
    }

    namespace news1
    {
        static const gfx::ui_size_t windowSize = { 360, 117 };

        widget_t widgets[] = {
            commonWidgets(360, 117, widget_type::wt_3),
            widget_end(),
        };

        static window_event_list events;

        enum newsItems
        {
            industry,
            station,
            town,
            vehicle,
            company,
            vehicleTab = 7,
        };

        // 0x00429BB7
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            switch (widgetIndex)
            {
                case common::widx::close_button:
                {
                    if (_activeMessageIndex != 0xFFFF)
                    {
                        auto news = messagemgr::get(_activeMessageIndex);
                        news->var_C8 = 0xFFFF;
                        _activeMessageIndex = 0xFFFF;
                    }
                    WindowManager::close(self);
                    break;
                }

                case common::widx::viewport1Button:
                case common::widx::viewport2Button:
                {
                    if (_activeMessageIndex != 0xFFFF)
                    {
                        auto news = messagemgr::get(_activeMessageIndex);

                        if (_word_4F8BE4[news->var_00] & (1 << 2))
                        {
                            uint32_t itemType;
                            uint16_t itemId;
                            if (widgetIndex == common::widx::viewport1Button)
                            {
                                itemType = _byte_4F8B08[news->var_00][0];
                                itemId = news->item_id_1;
                            }
                            else
                            {
                                itemType = _byte_4F8B09[news->var_00][0];
                                itemId = news->item_id_2;
                            }

                            switch (itemType)
                            {
                                case newsItems::industry:
                                    ui::windows::industry::open(itemId);
                                    break;

                                case newsItems::station:
                                    ui::windows::station::open(itemId);
                                    break;

                                case newsItems::town:
                                    ui::windows::town::open(itemId);
                                    break;

                                case newsItems::vehicle:
                                {
                                    registers regs;
                                    auto vehicle = thingmgr::get<openloco::vehicle>(itemId);

                                    regs.edx = (int32_t)vehicle;
                                    call(0x004B6033, regs);
                                    break;
                                }

                                case newsItems::company:
                                    ui::windows::CompanyWindow::open(itemId);
                                    break;

                                case 5:
                                case 6:
                                    break;

                                case newsItems::vehicleTab:
                                    auto vehicleObj = objectmgr::get<vehicle_object>(itemId);
                                    auto window = ui::build_vehicle::open(static_cast<uint32_t>(vehicleObj->type), 0x80000000);
                                    window->row_hover = itemId;
                                    if (vehicleObj->mode == TransportMode::rail || vehicleObj->mode == TransportMode::road)
                                    {
                                        if (vehicleObj->track_type != 0xFF)
                                        {
                                            uint8_t i = 0;
                                            while (i < _numTrackTypeTabs)
                                            {
                                                if (vehicleObj->track_type == _trackTypesForTab[i])
                                                {
                                                    window->current_secondary_tab = i;
                                                    break;
                                                }
                                            }
                                        }
                                    }

                                    auto rowHover = window->row_hover;

                                    ui::build_vehicle::sub_4B92A5(window);

                                    window->row_hover = rowHover;
                                    break;
                            }
                        }
                    }
                }
            }
        }

        // 0x00429D2C
        static void on_update(window* self)
        {
            uint16_t height = _word_525CE0 + 4;

            _word_525CE0 = std::min(height, self->height);

            height = ui::height() - _word_525CE0 - self->y;
            auto width = (ui::width() / 2) - 180 - self->x;

            if (width != 0 || height != 0)
            {
                self->invalidate();
                self->height = height;
                self->width = width;

                auto viewport1 = self->viewports[0];

                if (viewport1 != nullptr)
                {
                    viewport1->x += width;
                    viewport1->y += height;
                }

                auto viewport2 = self->viewports[1];

                if (viewport2 != nullptr)
                {
                    viewport2->x += width;
                    viewport2->y += height;
                }

                self->invalidate();
            }
        }

        // 0x00429209
        static void initViewport(window* self)
        {
            map::map_pos3 pos;
            pos.x = -1;
            pos.y = -1;
            pos.z = -1;
            int8_t rotation = -1;
            ZoomLevel zoomLevel = (ZoomLevel)-1;
            auto news = messagemgr::get(_activeMessageIndex);
            uint16_t thingId = 0xFFFF;
            bool isThing = false;
            bool selectable = false;

            if (_activeMessageIndex != 0xFFFF)
            {
                if (_word_4F8BE4[news->var_00] & (1 << 2))
                {
                    auto itemType = _byte_4F8B08[news->var_00][0];

                    if (news->item_id_1 != 0xFFFF)
                    {
                        switch (itemType)
                        {
                            case newsItems::industry:
                            {
                                auto industry = industrymgr::get(news->item_id_1);

                                pos.x = industry->x;
                                pos.y = industry->y;
                                pos.z = tile_element_height(pos.x, pos.y);
                                rotation = gCurrentRotation;
                                zoomLevel = ZoomLevel::half;
                                selectable = true;
                                break;
                            }

                            case newsItems::station:
                            {
                                auto station = stationmgr::get(news->item_id_1);

                                pos.x = station->x;
                                pos.y = station->y;
                                pos.z = station->z;
                                rotation = gCurrentRotation;
                                zoomLevel = ZoomLevel::full;
                                selectable = true;
                                break;
                            }

                            case newsItems::town:
                            {
                                auto town = townmgr::get(news->item_id_1);

                                pos.x = town->x;
                                pos.y = town->y;
                                pos.z = tile_element_height(pos.x, pos.y);
                                rotation = gCurrentRotation;
                                zoomLevel = ZoomLevel::half;
                                selectable = true;
                                break;
                            }

                            case newsItems::vehicle:
                            {
                                auto vehicle = thingmgr::get<openloco::vehicle>(news->item_id_1);

                                vehicle = vehicle->next_car()->next_car();
                                thingId = vehicle->id;
                                vehicle = vehicle->next_car();

                                if (vehicle->type != vehicle_thing_type::vehicle_6)
                                {
                                    thingId = vehicle->id;
                                    if (vehicle->type == vehicle_thing_type::vehicle_bogie)
                                    {
                                        vehicle = vehicle->next_car();
                                        thingId = vehicle->next_car_id;
                                    }
                                }
                                isThing = true;
                                zoomLevel = ZoomLevel::full;
                                rotation = gCurrentRotation;
                                selectable = true;
                                break;
                            }

                            case newsItems::company:
                                zoomLevel = (ZoomLevel)-2;
                                selectable = true;
                                break;

                            case 5:
                                pos.x = news->item_id_1; // possible union?
                                pos.y = news->item_id_2;
                                pos.z = tile_element_height(pos.x, pos.y);
                                zoomLevel = ZoomLevel::full;
                                rotation = gCurrentRotation;
                                selectable = true;
                                break;

                            case 6:
                                break;

                            case newsItems::vehicleTab:
                                zoomLevel = (ZoomLevel)-3;
                                selectable = true;
                                break;
                        }
                    }
                }
            }

            self->widgets[common::widx::viewport1].type = widget_type::none;
            self->widgets[common::widx::viewport1Button].type = widget_type::none;

            if (pos.x != -1 && pos.y != -1)
            {
                self->widgets[common::widx::viewport1].type = widget_type::viewport;
            }

            if (selectable)
            {
                self->widgets[common::widx::viewport1Button].type = widget_type::wt_9;
            }

            uint32_t ecx = pos.z << 16 | rotation << 8 | (uint8_t)zoomLevel;
            uint32_t edx = pos.y << 16 | pos.x | 1 << 30;

            if (isThing)
            {
                ecx = rotation << 8 | (uint8_t)zoomLevel;
                edx = thingId | 1 << 31;
            }

            if (_dword_525CD0 != ecx || _dword_525CD4 != edx)
            {
                _dword_525CD0 = ecx;
                _dword_525CD4 = edx;
                auto viewport = self->viewports[0];
                if (viewport != nullptr)
                {
                    viewport = nullptr;
                    self->invalidate();
                }

                self->widgets[common::widx::viewport1].left = 6;
                self->widgets[common::widx::viewport1].right = 353;
                self->widgets[common::widx::viewport1Button].left = 4;
                self->widgets[common::widx::viewport1Button].right = 355;

                if (_word_4F8BE4[news->var_00] & (1 << 3))
                {
                    self->widgets[common::widx::viewport1].left = 6;
                    self->widgets[common::widx::viewport1].right = 173;
                    self->widgets[common::widx::viewport1Button].left = 4;
                    self->widgets[common::widx::viewport1Button].right = 175;
                }

                if (edx != 0xFFFFFFFF)
                {
                    gfx::point_t origin = { self->widgets[common::widx::viewport1].left + 1 + self->x, self->widgets[common::widx::viewport1].top + 1 + self->y };
                    gfx::ui_size_t viewportSize = { self->widgets[common::widx::viewport1].width(), 62 };

                    if (_word_4F8BE4[news->var_00] & (1 << 1))
                    {
                        origin = { self->widgets[common::widx::viewport1].left + self->x, self->widgets[common::widx::viewport1].top + self->y };
                        viewportSize = { self->widgets[common::widx::viewport1].width() + 2U, 64 };
                    }

                    if (isThing)
                    {
                        viewportmgr::create(self, 0, origin, viewportSize, zoomLevel, thingId);
                    }
                    else
                    {
                        viewportmgr::create(self, 0, origin, viewportSize, zoomLevel, pos);
                    }
                    self->invalidate();
                }
            }

            pos.x = -1;
            pos.y = -1;
            pos.z = -1;
            rotation = -1;
            zoomLevel = ZoomLevel::full;
            thingId = 0xFFFF;
            isThing = false;
            selectable = false;

            if (_activeMessageIndex != 0xFFFF)
            {
                if (_word_4F8BE4[news->var_00] & (1 << 2))
                {
                    auto itemType = _byte_4F8B09[news->var_00][0];

                    if (news->item_id_2 != 0xFFFF)
                    {
                        switch (itemType)
                        {
                            case newsItems::industry:
                            {
                                auto industry = industrymgr::get(news->item_id_2);

                                pos.x = industry->x;
                                pos.y = industry->y;
                                pos.z = tile_element_height(pos.x, pos.y);
                                rotation = gCurrentRotation;
                                zoomLevel = ZoomLevel::half;
                                selectable = true;
                                break;
                            }

                            case newsItems::station:
                            {
                                auto station = stationmgr::get(news->item_id_2);

                                pos.x = station->x;
                                pos.y = station->y;
                                pos.z = station->z;
                                rotation = gCurrentRotation;
                                zoomLevel = ZoomLevel::full;
                                selectable = true;
                                break;
                            }

                            case newsItems::town:
                            {
                                auto town = townmgr::get(news->item_id_2);

                                pos.x = town->x;
                                pos.y = town->y;
                                pos.z = tile_element_height(pos.x, pos.y);
                                rotation = gCurrentRotation;
                                zoomLevel = ZoomLevel::half;
                                selectable = true;
                                break;
                            }

                            case newsItems::vehicle:
                            {
                                auto vehicle = thingmgr::get<openloco::vehicle>(news->item_id_2);

                                vehicle = vehicle->next_car()->next_car();
                                thingId = vehicle->id;
                                vehicle = vehicle->next_car();

                                if (vehicle->type != vehicle_thing_type::vehicle_6)
                                {
                                    thingId = vehicle->id;
                                    if (vehicle->type == vehicle_thing_type::vehicle_bogie)
                                    {
                                        vehicle = vehicle->next_car();
                                        thingId = vehicle->next_car_id;
                                    }
                                }
                                isThing = true;
                                zoomLevel = ZoomLevel::full;
                                rotation = gCurrentRotation;
                                selectable = true;
                                break;
                            }

                            case newsItems::company:
                                zoomLevel = (ZoomLevel)-2;
                                selectable = true;
                                break;

                            case 5:
                                pos.x = news->item_id_1; // possible union?
                                pos.y = news->item_id_2;
                                pos.z = tile_element_height(pos.x, pos.y);
                                zoomLevel = ZoomLevel::full;
                                rotation = gCurrentRotation;
                                selectable = true;
                                break;

                            case 6:
                                break;

                            case newsItems::vehicleTab:
                                zoomLevel = (ZoomLevel)-3;
                                selectable = true;
                                break;
                        }
                    }
                }
            }

            self->widgets[common::widx::viewport2].type = widget_type::none;
            self->widgets[common::widx::viewport2Button].type = widget_type::none;

            if (pos.x != -1 && pos.y != -1)
            {
                self->widgets[common::widx::viewport2].type = widget_type::viewport;
            }

            if (selectable)
            {
                self->widgets[common::widx::viewport2Button].type = widget_type::wt_9;
            }

            ecx = pos.z << 16 | rotation << 8 | (uint8_t)zoomLevel;
            edx = pos.y << 16 | pos.x | 1 << 30;

            if (isThing)
            {
                ecx = rotation << 8 | (uint8_t)zoomLevel;
                edx = thingId | 1 << 31;
            }

            if (_dword_525CD8 != ecx || _dword_525CDC != edx)
            {
                _dword_525CD8 = ecx;
                _dword_525CDC = edx;
                auto viewport = self->viewports[0];
                if (viewport != nullptr)
                {
                    viewport = nullptr;
                    self->invalidate();
                }

                self->widgets[common::widx::viewport2].left = 186;
                self->widgets[common::widx::viewport2].right = 353;
                self->widgets[common::widx::viewport2Button].left = 184;
                self->widgets[common::widx::viewport2Button].right = 355;

                if (edx != 0xFFFFFFFF)
                {
                    gfx::point_t origin = { self->widgets[common::widx::viewport2].left + 1 + self->x, self->widgets[common::widx::viewport1].top + 1 + self->y };
                    gfx::ui_size_t viewportSize = { self->widgets[common::widx::viewport2].width(), 62 };

                    if (_word_4F8BE4[news->var_00] & (1 << 1))
                    {
                        origin = { self->widgets[common::widx::viewport2].left + self->x, self->widgets[common::widx::viewport1].top + self->y };
                        viewportSize = { self->widgets[common::widx::viewport2].width() + 2U, 64 };
                    }

                    viewportmgr::create(self, 1, origin, viewportSize, zoomLevel, pos);
                    self->invalidate();
                }
            }
        }

        // 0x00429739
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x00429739, regs);
        }

        static void initEvents()
        {
            events.on_mouse_up = on_mouse_up;
            events.on_resize = initViewport;
            events.on_update = on_update;
            events.viewport_rotate = initViewport;
            events.draw = draw;
        }
    }

    namespace news2
    {
        static const gfx::ui_size_t windowSize = { 360, 159 };

        widget_t widgets[] = {
            commonWidgets(360, 159, widget_type::wt_5),
            widget_end(),
        };
    }

    namespace ticker
    {
        static const gfx::ui_size_t windowSize = { 111, 26 };

        enum widx
        {
            frame,
        };
        const uint64_t enabledWidgets = (1 << widx::frame);

        widget_t widgets[] = {
            make_widget({ 0, 0 }, { 111, 26 }, widget_type::wt_3, 0),
            widget_end(),
        };

        static window_event_list events;

        // 0x00429EA2
        static void on_mouse_up(window* self, widget_index widgetIndex)
        {
            if (widgetIndex != 0)
                return;

            if (_activeMessageIndex == 0xFFFF)
                return;

            auto news = messagemgr::get(_activeMessageIndex);
            news->var_C8 = 1;

            auto activeMessageIndex = _activeMessageIndex;
            _activeMessageIndex = 0xFFFF;

            WindowManager::close(self);
            open(activeMessageIndex);
        }

        // 0x00429FE4
        static void on_resize(window* self)
        {
            auto y = ui::height() - 25;
            auto x = ui::width() - 138;
            auto width = 111;
            auto height = 25;

            if (y != self->y || x != self->x || width != self->width || height != self->height)
            {
                self->invalidate();
                self->y = y;
                self->x = x;
                self->width = width;
                self->height = height;
                self->invalidate();
            }
        }

        // 0x00429EEB
        static void on_update(window* self)
        {
            //registers regs;
            //regs.esi = (int32_t)self;
            //call(0x00429EEB, regs);

            auto window = WindowManager::findAtAlt(_cursorX2, _cursorY2);

            if (window == self)
            {
                self->var_852 = 12;
            }

            if (self->var_852 != 0)
            {
                if (!is_paused())
                {
                    self->var_852--;
                }
            }

            self->invalidate();

            if (self->var_852 == 0)
            {
                if (!is_paused())
                {
                    _word_525CE0 = _word_525CE0 + 2;
                    
                    if (!(_word_525CE0 & 0x8007))
                    {
                        if (_activeMessageIndex != 0xFFFF)
                        {
                            auto news = messagemgr::get(_activeMessageIndex);
                            auto cx = _word_525CE0 >> 2;
                            char* buffer = news->messageString;
                            auto al = *buffer;

                            while (true)
                            {
                                al = *buffer;
                                if (al == control_codes::newline)
                                {
                                    al = 32;
                                    cx--;
                                    if (cx < 0)
                                        break;
                                }

                                if (al != 0xFF)
                                {
                                    cx--;
                                    if (cx < 0)
                                        break;
                                    buffer++;
                                    if (!al)
                                        break;
                                }
                                else
                                {
                                    cx--;
                                    if (cx < 0)
                                        break;
                                    buffer += 3;
                                }
                            }

                            if (al != 32)
                            {
                                if (al != 0)
                                {
                                    audio::play_sound(audio::sound_id::ticker, ui::width());
                                }
                            }
                        }
                    }
                }
            }

            if (_activeMessageIndex != 0xFFFF)
                return;

            _activeMessageIndex = 0xFFFF;

            WindowManager::close(self);
        }

        //static void sub_4950EF(gfx::drawpixelinfo_t* clipped, string_id buffer, uint32_t eax, uint32_t ebp, int16_t x, int16_t y)
        //{
        //    registers regs;
        //    regs.bx = buffer;
        //    regs.eax = eax;
        //    regs.cx = x;
        //    regs.dx = y;
        //    regs.ebp = ebp;
        //    regs.edi = (int32_t)clipped;
        //    call(0x004950EF, regs);

        //    //_currentFontSpriteBase = font::medium_bold;
        //    //gfx::draw_string(clipped, clipped->x, clipped->y, colour::black, _unk_5215B5);
        //    //stringmgr::format_string(byte_112CC04, buffer);

        //    //registers regs;
        //    //regs.esi =
        //    //call(0x0049544E, regs);
        //}

        // 0x00429DAA
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            registers regs;
            regs.esi = (int32_t)self;
            regs.edi = (int32_t)dpi;
            call(0x00429DAA, regs);

            //if (self->var_852 != 0)
            //    return;

            //if (get_pause_flags() & 4)
            //    return;

            //auto news = messagemgr::get(_activeMessageIndex);

            //auto x = self->x;
            //auto y = self->y;
            //auto width = self->width;
            //auto height = self->height;
            //gfx::drawpixelinfo_t* clipped = nullptr;

            //gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height);

            //if (clipped == nullptr)
            //    return;

            //uint32_t colour = 0x14141414;

            //if (!(_word_4F8BE4[news->var_00] & (1 << 1)))
            //{
            //    colour = colour::translucent(colour::inset(colour::icy_blue));
            //    colour *= 0x1010101;
            //}

            //gfx::clear(*clipped, colour);

            //char* buffer = news->messageString;
            //auto str = const_cast<char*>(stringmgr::get_string(string_ids::buffer_2039));

            //strncpy(str, buffer, 512);

            ////*str = -112;
            ////*str++;
            ////*str = control_codes::font_small;
            ////*str++;

            ////auto al = *buffer;
            //auto i = 0;

            ////while (*buffer != 0)
            ////{
            ////    if (al != control_codes::newline)
            ////    {
            ////        al = 32;
            ////        *str = al;
            ////        *str++;
            ////    }

            ////    if (al == 0xFF)
            ////    {
            ////        *str = al;
            ////        *str++;
            ////        *buffer++;
            ////        *str = *buffer;
            ////        *str++;
            ////        *buffer++;
            ////        al = *buffer;
            ////    }

            ////    *str = al;
            ////    *str++;
            ////    *buffer++;
            ////    i++;
            ////}

            //if ((_word_525CE0 >> 2) > i)
            //{
            //    _word_525CE0 = _word_525CE0 | (1 << 15);
            //}
            //uint32_t ebp = (((_word_525CE0 & ~(1 << 15)) >> 2) << 14) | 109;

            //sub_4950EF(clipped, string_ids::buffer_2039, (1 << 18), ebp, 55, 0);
        }

        static void initEvents()
        {
            events.on_mouse_up = on_mouse_up;
            events.on_resize = on_resize;
            events.on_update = on_update;
            events.draw = draw;
        }
    }

    // 0x00428F8B
    void open(uint16_t messageIndex)
    {
        auto companyId = 0;
        auto news = messagemgr::get(messageIndex);

        if ((news->var_C8 != 0) && (getScreenAge() >= 10))
        {
            companyId++;
        }

        _activeMessageIndex = messageIndex;

        auto activeMessage = news->var_00;

        if (companyId == 0)
        {
            auto messageType = _messageTypes[activeMessage];

            if (messageType == common::newsItem::majorCompany || messageType == common::newsItem::minorCompany)
            {
                if (news->companyId != _playerCompany)
                {
                    messageType++;
                }
            }

            auto newsSettings = config::get().news_settings[messageType];

            if (newsSettings == common::newsType::none)
            {
                news->var_C8 = 0xFFFF;
                return;
            }

            if (newsSettings == common::newsType::ticker)
            {
                _word_525CE0 = 0;
                int16_t x = ui::width() - 138;
                int16_t y = ui::height() - 25;
                gfx::point_t origin = { x, y };
                uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent | window_flags::flag_7;

                auto window = WindowManager::createWindow(WindowType::news, origin, ticker::windowSize, flags, &ticker::events);

                window->widgets = ticker::widgets;
                window->enabled_widgets = ticker::enabledWidgets;

                common::initEvents();

                window->init_scroll_widgets();

                auto skin = objectmgr::get<interface_skin_object>();
                window->colours[0] = colour::translucent(skin->colour_0C);

                window->var_854 = 0;

                return;
            }
        }

        if (companyId == 0)
        {
            audio::sound_id soundId = audio::sound_id::notification;

            if (news->companyId == company_id::null || companyId == _playerCompany)
            {
                soundId = static_cast<audio::sound_id>(_messageSounds[activeMessage]);
            }

            if (soundId != audio::sound_id::null)
            {
                int32_t pan = ui::width() / 2;
                audio::play_sound(soundId, pan);
            }
        }

        if (_word_4F8BE4[activeMessage] & (1 << 1))
        {
            _word_525CE0 = 5;

            int16_t y = ui::height() - _word_525CE0;

            if (_gameSpeed != 0 || companyId != 0)
            {
                y = ui::height() - news2::windowSize.height;
                _word_525CE0 = news2::windowSize.height;
            }

            int16_t x = (ui::width() / 2) - (news2::windowSize.width / 2);
            gfx::point_t origin = { x, y };
            uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent | window_flags::no_background;

            auto window = WindowManager::createWindow(WindowType::news, origin, news2::windowSize, flags, &news1::events);

            window->widgets = news2::widgets;
            window->enabled_widgets = common::enabledWidgets;

            common::initEvents();

            window->init_scroll_widgets();
            window->colours[0] = colour::grey;

            _dword_525CD0 = 0xFFFFFFFF;
            _dword_525CD4 = 0xFFFFFFFF;
            _dword_525CD8 = 0xFFFFFFFF;
            _dword_525CDC = 0xFFFFFFFF;

            news1::initViewport(window);
        }
        else
        {
            _word_525CE0 = 5;

            int16_t y = ui::height() - _word_525CE0;

            if (_gameSpeed != 0 || companyId != 0)
            {
                y = ui::height() - news1::windowSize.height;
                _word_525CE0 = news1::windowSize.height;
            }

            int16_t x = (ui::width() / 2) - (news1::windowSize.width / 2);
            gfx::point_t origin = { x, y };
            uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent;

            auto window = WindowManager::createWindow(WindowType::news, origin, news1::windowSize, flags, &news1::events);

            window->widgets = news1::widgets;
            window->enabled_widgets = common::enabledWidgets;

            common::initEvents();

            window->init_scroll_widgets();
            window->colours[0] = colour::translucent(colour::dark_yellow);

            _dword_525CD0 = 0xFFFFFFFF;
            _dword_525CD4 = 0xFFFFFFFF;
            _dword_525CD8 = 0xFFFFFFFF;
            _dword_525CDC = 0xFFFFFFFF;

            news1::initViewport(window);
        }
    }

    namespace common
    {
        static void initEvents()
        {
            ticker::initEvents();
            news1::initEvents();
        }
    }
}
