#pragma once

#include "../../audio/audio.h"
#include "../../config.h"
#include "../../graphics/colours.h"
#include "../../interop/interop.hpp"
#include "../../message.h"
#include "../../messagemgr.h"
#include "../../objects/interface_skin_object.h"
#include "../../objects/objectmgr.h"
#include "News.h"

using namespace openloco::interop;

namespace openloco::ui::NewsWindow
{
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
        void initEvents()
        {
            ticker::initEvents();
            news1::initEvents();
        }
    }
}
