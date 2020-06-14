#include "../audio/audio.h"
#include "../companymgr.h"
#include "../config.h"
#include "../date.h"
#include "../game_commands.h"
#include "../graphics/colours.h"
#include "../graphics/gfx.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../intro.h"
#include "../localisation/string_ids.h"
#include "../message.h"
#include "../messagemgr.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../objects/vehicle_object.h"
#include "../openloco.h"
#include "../ui.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::NewsWindow
{
    static loco_global<uint8_t[122], 0x004F8B08> _byte_4F8B08;
    static loco_global<uint8_t[122], 0x004F8B09> _byte_4F8B09;
    static loco_global<uint16_t[31], 0x004F8BE4> _word_4F8BE4;
    static loco_global<uint8_t[31], 0x004F8C22> _messageTypes;
    static loco_global<audio::sound_id[31], 0x004F8C41> _messageSounds;
    static loco_global<uint8_t, 0x00508F1A> _gameSpeed;
    static loco_global<uint32_t, 0x00523338> _cursorX2;
    static loco_global<uint32_t, 0x0052333C> _cursorY2;
    static loco_global<uint32_t, 0x00525CD0> _dword_525CD0;
    static loco_global<uint32_t, 0x00525CD4> _dword_525CD4;
    static loco_global<uint32_t, 0x00525CD8> _dword_525CD8;
    static loco_global<uint32_t, 0x00525CDC> _dword_525CDC;
    static loco_global<uint8_t, 0x00525CE0> _word_525CE0;
    static loco_global<company_id_t, 0x00525E3C> _playerCompany;
    static loco_global<uint16_t, 0x005271CE> _messageCount;
    static loco_global<uint16_t, 0x005271D0> _activeMessageIndex;
    static loco_global<uint32_t, 0x011364EC> _numTrackTypeTabs;
    static loco_global<int8_t[8], 0x011364F0> _trackTypesForTab;

    namespace common
    {
        enum newsType
        {
            none = 0,
            ticker,
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

#define commonWidgets(frameWidth, frameHeight)                                                                                            \
    make_widget({ 0, 0 }, { frameWidth, frameHeight }, widget_type::wt_3, 0),                                                             \
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
            commonWidgets(360, 117),
            widget_end(),
        };

        static window_event_list events;

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
                            uint8_t itemType;

                            if (widgetIndex == common::widx::viewport1Button)
                            {
                                itemType = _byte_4F8B08[news->var_00];
                            }
                            else
                            {
                                itemType = _byte_4F8B09[news->var_00];
                            }

                            auto itemId = news->item_id;

                            switch (itemType)
                            {
                                case 0:
                                    ui::windows::industry::open(itemId);
                                    break;

                                case 1:
                                    ui::windows::station::open(itemId);
                                    break;

                                case 2:
                                    ui::windows::town::open(itemId);
                                    break;

                                case 3:
                                    ui::build_vehicle::open(itemId, 0);
                                    break;

                                case 4:
                                    ui::windows::CompanyWindow::open(itemId);
                                    break;

                                case 5:
                                case 6:
                                    break;

                                case 7:
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

        static void sub_429209(window* self)
        {
            registers regs;
            regs.esi = (int32_t)self;
            call(0x00429209, regs);
        }

        // 0x00429DA2
        static void initViewport(window* self)
        {
            sub_429209(self);
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
            commonWidgets(360, 159),
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
                    if (!(_word_525CE0 & ((1 << 15) | (1 << 2) | (1 << 1) | (1 << 0))))
                    {
                        if (_activeMessageIndex != 0xFFFF)
                        {
                            auto news = messagemgr::get(_activeMessageIndex);
                            auto cx = _word_525CE0;
                            cx >>= 2;
                            char* buffer = news->messageString;
                            while (*buffer != 0)
                            {
                                if (*buffer == control_codes::newline)
                                {
                                    *buffer = 32;
                                    cx--;
                                    if (cx < 0)
                                    {
                                        break;
                                    }
                                }

                                if (*buffer == 0xFF)
                                {
                                    cx--;
                                    if (cx < 0)
                                    {
                                        break;
                                    }

                                    *buffer++;
                                }
                                else
                                {
                                    cx--;
                                    if (cx < 0)
                                    {
                                        break;
                                    }
                                    *buffer += 3;
                                }
                            }
                            if (*buffer != 32)
                            {
                                if (*buffer != 0)
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

        static void sub_4950EF(string_id buffer, uint32_t eax, uint32_t ebp, uint16_t cx, uint16_t dx)
        {
            registers regs;
            regs.bx = buffer;
            regs.eax = eax;
            regs.cx = cx;
            regs.dx = dx;
            regs.ebp = ebp;
            call(0x004950EF, regs);
        }

        // 0x00429DAA
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            if (self->var_852 != 0)
                return;

            if (get_pause_flags() & 4)
                return;

            auto news = messagemgr::get(_activeMessageIndex);

            auto x = self->x;
            auto y = self->y;
            auto width = self->width;
            auto height = self->height;
            gfx::drawpixelinfo_t* clipped = nullptr;

            gfx::clip_drawpixelinfo(&clipped, dpi, x, y, width, height);

            if (clipped == nullptr)
                return;

            uint32_t colour = 0x14141414;

            if (!(_word_4F8BE4[news->var_00] & (1 << 1)))
            {
                colour = colour::translucent(colour::inset(colour::icy_blue));
                colour *= 0x1010101;
            }

            gfx::clear(*clipped, colour);

            char* buffer = news->messageString;
            auto str = const_cast<char*>(stringmgr::get_string(string_ids::buffer_2039));

            *str = -112;
            *str++;
            *str = control_codes::font_small;
            *str++;

            auto al = *buffer;
            auto i = 0;

            while (*buffer != 0)
            {
                if (al != control_codes::newline)
                {
                    al = 32;
                    *str = al;
                    *str++;
                }

                if (al == 0xFF)
                {
                    *str = al;
                    *str++;
                    *buffer++;
                    *str = *buffer;
                    *str++;
                    *buffer++;
                    al = *buffer;
                }

                *str = al;
                *str++;
                *buffer++;
                i++;
            }

            if ((_word_525CE0 >> 2) > i)
            {
                _word_525CE0 = _word_525CE0 | (1 << 15);
            }
            auto ebp = ((_word_525CE0 & ~(1 << 15)) << 14) | 109;

            sub_4950EF(string_ids::buffer_2039, (1 << 18), ebp, 55, 0);
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
        auto i = 0;
        auto news = messagemgr::get(messageIndex);

        if ((news->var_C8 != 0) && (getScreenAge() >= 10))
        {
            i++;
        }

        _activeMessageIndex = messageIndex;

        auto activeMessage = news->var_00;

        if (i == 0)
        {
            auto messageType = _messageTypes[activeMessage];

            if (messageType == 0 || messageType == 2)
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

        if (i == 0)
        {
            audio::sound_id soundId = audio::sound_id::notification;

            if (news->companyId == 0xFF || i == _playerCompany)
            {
                soundId = _messageSounds[activeMessage];
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

            if (_gameSpeed == 0 || i != 0)
            {
                y = ui::height() - news2::windowSize.height;
                _word_525CE0 = news2::windowSize.height;
            }

            int16_t x = (ui::width() / 2) - (news2::windowSize.width / 2);
            gfx::point_t origin = { x, y };
            uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent | window_flags::no_background;

            auto window = WindowManager::createWindow(WindowType::news, origin, news2::windowSize, flags, &news1::events);

            window->widgets = news2::widgets;

            common::initEvents();

            window->init_scroll_widgets();
            window->colours[0] = colour::grey;

            _dword_525CD0 = 0xFFFFFFFF;
            _dword_525CD4 = 0xFFFFFFFF;
            _dword_525CD8 = 0xFFFFFFFF;
            _dword_525CDC = 0xFFFFFFFF;

            news1::sub_429209(window);
        }
        else
        {
            _word_525CE0 = 5;

            int16_t y = ui::height() - _word_525CE0;

            if (_gameSpeed == 0 || i != 0)
            {
                y = ui::height() - news1::windowSize.height;
                _word_525CE0 = news1::windowSize.height;
            }

            int16_t x = (ui::width() / 2) - (news1::windowSize.width / 2);
            gfx::point_t origin = { x, y };
            uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent | window_flags::no_background;

            auto window = WindowManager::createWindow(WindowType::news, origin, news1::windowSize, flags, &news1::events);

            window->widgets = news1::widgets;

            common::initEvents();

            window->init_scroll_widgets();
            window->colours[0] = colour::translucent(colour::dark_yellow);

            _dword_525CD0 = 0xFFFFFFFF;
            _dword_525CD4 = 0xFFFFFFFF;
            _dword_525CD8 = 0xFFFFFFFF;
            _dword_525CDC = 0xFFFFFFFF;

            news1::sub_429209(window);
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
