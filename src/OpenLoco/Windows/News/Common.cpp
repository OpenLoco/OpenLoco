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
using namespace OpenLoco::Config;

namespace OpenLoco::Ui::NewsWindow
{
    static void createNewsWindow(Gfx::ui_size_t windowSize, widget_t* widgets, uint8_t colour, bool isOld, uint32_t flags)
    {
        _word_525CE0 = 5;

        int16_t y = Ui::height() - _word_525CE0;

        if (getGameSpeed() != 0 || isOld)
        {
            y = Ui::height() - windowSize.height;
            _word_525CE0 = windowSize.height;
        }

        int16_t x = (Ui::width() / 2) - (windowSize.width / 2);
        Gfx::point_t origin = { x, y };

        auto window = WindowManager::createWindow(WindowType::news, origin, windowSize, flags, &News1::events);

        window->widgets = widgets;
        window->enabled_widgets = Common::enabledWidgets;

        Common::initEvents();

        window->initScrollWidgets();
        window->colours[0] = colour;

        _dword_525CD0 = 0xFFFFFFFF;
        _dword_525CD4 = 0xFFFFFFFF;
        _dword_525CD8 = 0xFFFFFFFF;
        _dword_525CDC = 0xFFFFFFFF;

        News1::initViewport(window);
    }

    // 0x00428F8B
    void open(uint16_t messageIndex)
    {
        bool isOld = false;
        auto news = MessageManager::get(messageIndex);

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

            auto newsSettings = Config::get().news_settings[static_cast<uint8_t>(messageSubType)];

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
                uint32_t flags = WindowFlags::stick_to_front | WindowFlags::viewport_no_scrolling | WindowFlags::transparent | WindowFlags::flag_7;

                auto window = WindowManager::createWindow(WindowType::news, origin, Ticker::windowSize, flags, &Ticker::events);

                window->widgets = Ticker::widgets;
                window->enabled_widgets = Ticker::enabledWidgets;

                Common::initEvents();

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

            if (news->companyId == CompanyId::null || news->companyId == _playerCompany)
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
            uint32_t flags = WindowFlags::stick_to_front | WindowFlags::viewport_no_scrolling | WindowFlags::transparent | WindowFlags::no_background;

            createNewsWindow(News2::windowSize, News2::widgets, Colour::grey, isOld, flags);
        }
        else
        {
            uint32_t flags = WindowFlags::stick_to_front | WindowFlags::viewport_no_scrolling | WindowFlags::transparent;
            auto colour = Colour::translucent(Colour::salmon_pink);

            createNewsWindow(News1::windowSize, News1::widgets, colour, isOld, flags);
        }
    }

    // 0x0042AC27
    void openLastMessage()
    {
        if (_activeMessageIndex != 0xFFFF)
        {
            auto message = MessageManager::get(_activeMessageIndex);
            if (message->var_C8 != 0xFFFF)
            {
                if (message->var_C8 & (1 << 15))
                    message->var_C8 = 0xFFFF;
            }
        }

        _activeMessageIndex = 0xFFFF;
        WindowManager::close(WindowType::news, 0);

        if (_messageCount != 0)
        {
            auto message = MessageManager::get(_messageCount - 1);
            message->var_C8 = (1 << 15) | (1 << 0);

            NewsWindow::open(_messageCount - 1);
        }
    }

    void close(window* self)
    {
        // Only affects the newspaper view; the ticker ignores this widget
        self->callOnMouseUp(1);
    }

    namespace Common
    {
        void initEvents()
        {
            Ticker::initEvents();
            News1::initEvents();
        }
    }
}
