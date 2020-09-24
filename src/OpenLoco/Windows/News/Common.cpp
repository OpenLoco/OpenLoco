#include "../../Audio/Audio.h"
#include "../../Config.h"
#include "../../Graphics/Colour.h"
#include "../../Interop/Interop.hpp"
#include "../../Message.h"
#include "../../MessageManager.h"
#include "../../Objects/InterfaceSkinObject.h"
#include "../../Objects/ObjectManager.h"
#include "News.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::config;

namespace OpenLoco::Ui::NewsWindow
{
    static void createNewsWindow(Gfx::ui_size_t windowSize, widget_t* widgets, uint8_t colour, bool isOld, uint32_t flags)
    {
        _word_525CE0 = 5;

        int16_t y = Ui::height() - _word_525CE0;

        if (_gameSpeed != 0 || isOld)
        {
            y = Ui::height() - windowSize.height;
            _word_525CE0 = windowSize.height;
        }

        int16_t x = (Ui::width() / 2) - (windowSize.width / 2);
        Gfx::point_t origin = { x, y };

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
                int16_t x = Ui::width() - 138;
                int16_t y = Ui::height() - 25;
                Gfx::point_t origin = { x, y };
                uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent | window_flags::flag_7;

                auto window = WindowManager::createWindow(WindowType::news, origin, ticker::windowSize, flags, &ticker::events);

                window->widgets = ticker::widgets;
                window->enabled_widgets = ticker::enabledWidgets;

                common::initEvents();

                window->initScrollWidgets();

                auto skin = ObjectManager::get<interface_skin_object>();
                window->colours[0] = Colour::translucent(skin->colour_0C);

                window->var_852 = 0;

                return;
            }
        }

        if (!isOld)
        {
            Audio::sound_id soundId = Audio::sound_id::notification;

            if (news->companyId == company_id::null || news->companyId == _playerCompany)
            {
                Audio::sound_id messageSounds[31] = {
                    Audio::sound_id::notification,
                    Audio::sound_id::news_awww,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::notification,
                    Audio::sound_id::applause_2,
                    Audio::sound_id::applause_2,
                    Audio::sound_id::news_oooh,
                    Audio::sound_id::applause_2,
                    Audio::sound_id::news_oooh,
                    Audio::sound_id::news_oooh,
                    Audio::sound_id::news_awww,
                    Audio::sound_id::applause_2,
                    Audio::sound_id::news_awww,
                    Audio::sound_id::news_awww,
                    Audio::sound_id::news_awww,
                    Audio::sound_id::news_awww,
                    Audio::sound_id::news_awww,
                    Audio::sound_id::applause_2,
                    Audio::sound_id::notification,
                    Audio::sound_id::news_awww,
                    Audio::sound_id::applause_2,
                    Audio::sound_id::news_oooh
                };

                soundId = messageSounds[activeMessage];
            }

            if (soundId != Audio::sound_id::null)
            {
                int32_t pan = Ui::width() / 2;
                Audio::playSound(soundId, pan);
            }
        }

        if (_word_4F8BE4[activeMessage] & (1 << 1))
        {
            uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent | window_flags::no_background;

            createNewsWindow(news2::windowSize, news2::widgets, Colour::grey, isOld, flags);
        }
        else
        {
            uint32_t flags = window_flags::stick_to_front | window_flags::viewport_no_scrolling | window_flags::transparent;
            auto colour = Colour::translucent(Colour::salmon_pink);

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
