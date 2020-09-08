#include "../../Audio/Audio.h"
#include "../../Config.h"
#include "../../Graphics/Colour.h"
#include "../../Message.h"
#include "../../MessageManager.h"
#include "../../Interop/interop.hpp"
#include "../../objects/interface_skin_object.h"
#include "../../objects/objectmgr.h"
#include "News.h"

using namespace openloco::interop;
using namespace openloco::config;

namespace openloco::ui::NewsWindow
{
    static void createNewsWindow(gfx::ui_size_t windowSize, widget_t* widgets, uint8_t colour, bool isOld, uint32_t flags)
    {
        _word_525CE0 = 5;

        int16_t y = ui::height() - _word_525CE0;

        if (_gameSpeed != 0 || isOld)
        {
            y = ui::height() - windowSize.height;
            _word_525CE0 = windowSize.height;
        }

        int16_t x = (ui::width() / 2) - (windowSize.width / 2);
        gfx::point_t origin = { x, y };

        auto window = WindowManager::createWindow(WindowType::news, origin, windowSize, flags, &news1::events);

        window->widgets = widgets;
        window->enabled_widgets = common::enabledWidgets;

        common::initEvents();

        window->initScrollWidgets();
        window->colours[0] = colour;

        _dword_525CD0 = 0xFFFFFFFF;
        _dword_525CD4 = 0xFFFFFFFF;
        _dword_525CD8 = 0xFFFFFFFF;
        _dword_525CDC = 0xFFFFFFFF;

        news1::initViewport(window);
    }

    // 0x00428F8B
    void open(uint16_t messageIndex)
    {
        bool isOld = false;
        auto news = messagemgr::get(messageIndex);

        if ((news->var_C8 != 0) && (getScreenAge() >= 10))
        {
            isOld = true;
        }

        _activeMessageIndex = messageIndex;

        auto activeMessage = news->type;

        if (!isOld)
        {
            newsItemSubType messageSubTypes[31] = {
                newsItemSubType::advice,
                newsItemSubType::general,
                newsItemSubType::advice,
                newsItemSubType::minorCompany,
                newsItemSubType::minorCompany,
                newsItemSubType::minorCompany,
                newsItemSubType::minorCompany,
                newsItemSubType::minorCompany,
                newsItemSubType::minorCompany,
                newsItemSubType::advice,
                newsItemSubType::advice,
                newsItemSubType::majorCompany,
                newsItemSubType::advice,
                newsItemSubType::minorCompany,
                newsItemSubType::minorCompany,
                newsItemSubType::general,
                newsItemSubType::majorCompany,
                newsItemSubType::general,
                newsItemSubType::general,
                newsItemSubType::general,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompetitor,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompany,
                newsItemSubType::majorCompetitor,
            };

            auto messageSubType = messageSubTypes[activeMessage];

            if (messageSubType == newsItemSubType::majorCompany)
            {
                if (news->companyId != _playerCompany)
                {
                    messageSubType = newsItemSubType::majorCompetitor;
                }
            }

            if (messageSubType == newsItemSubType::minorCompany)
            {
                if (news->companyId != _playerCompany)
                {
                    messageSubType = newsItemSubType::minorCompetitor;
                }
            }

            auto newsSettings = config::get().news_settings[static_cast<uint8_t>(messageSubType)];

            if (newsSettings == newsType::none)
            {
                news->var_C8 = 0xFFFF;
                return;
            }

            if (newsSettings == newsType::ticker)
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

                window->initScrollWidgets();

                auto skin = objectmgr::get<interface_skin_object>();
                window->colours[0] = colour::translucent(skin->colour_0C);

                window->var_852 = 0;

                return;
            }
        }

        if (!isOld)
        {
            audio::sound_id soundId = audio::sound_id::notification;

            if (news->companyId == company_id::null || news->companyId == _playerCompany)
            {
                audio::sound_id messageSounds[31] = {
                    audio::sound_id::notification,
                    audio::sound_id::news_awww,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::notification,
                    audio::sound_id::applause_2,
                    audio::sound_id::applause_2,
                    audio::sound_id::news_oooh,
                    audio::sound_id::applause_2,
                    audio::sound_id::news_oooh,
                    audio::sound_id::news_oooh,
                    audio::sound_id::news_awww,
                    audio::sound_id::applause_2,
                    audio::sound_id::news_awww,
                    audio::sound_id::news_awww,
                    audio::sound_id::news_awww,
                    audio::sound_id::news_awww,
                    audio::sound_id::news_awww,
                    audio::sound_id::applause_2,
                    audio::sound_id::notification,
                    audio::sound_id::news_awww,
                    audio::sound_id::applause_2,
                    audio::sound_id::news_oooh
                };

                soundId = messageSounds[activeMessage];
            }

            if (soundId != audio::sound_id::null)
            {
                int32_t pan = ui::width() / 2;
                audio::playSound(soundId, pan);
            }
        }

        if (_word_4F8BE4[activeMessage] & (1 << 1))
        {
            uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent | window_flags::no_background;

            createNewsWindow(news2::windowSize, news2::widgets, colour::grey, isOld, flags);
        }
        else
        {
            uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent;
            auto colour = colour::translucent(colour::salmon_pink);

            createNewsWindow(news1::windowSize, news1::widgets, colour, isOld, flags);
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
