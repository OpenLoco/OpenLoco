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

namespace OpenLoco::Ui::Windows::NewsWindow
{
    static void createNewsWindow(Gfx::ui_size_t windowSize, Widget* widgets, uint8_t colour, bool isOld, uint32_t flags)
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
        window->setColour(WindowColour::primary, colour);

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
            NewsItemSubType messageSubTypes[31] = {
                NewsItemSubType::advice,
                NewsItemSubType::general,
                NewsItemSubType::advice,
                NewsItemSubType::minorCompany,
                NewsItemSubType::minorCompany,
                NewsItemSubType::minorCompany,
                NewsItemSubType::minorCompany,
                NewsItemSubType::minorCompany,
                NewsItemSubType::minorCompany,
                NewsItemSubType::advice,
                NewsItemSubType::advice,
                NewsItemSubType::majorCompany,
                NewsItemSubType::advice,
                NewsItemSubType::minorCompany,
                NewsItemSubType::minorCompany,
                NewsItemSubType::general,
                NewsItemSubType::majorCompany,
                NewsItemSubType::general,
                NewsItemSubType::general,
                NewsItemSubType::general,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompetitor,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompany,
                NewsItemSubType::majorCompetitor,
            };

            auto messageSubType = messageSubTypes[activeMessage];

            if (messageSubType == NewsItemSubType::majorCompany)
            {
                if (news->companyId != _playerCompany)
                {
                    messageSubType = NewsItemSubType::majorCompetitor;
                }
            }

            if (messageSubType == NewsItemSubType::minorCompany)
            {
                if (news->companyId != _playerCompany)
                {
                    messageSubType = NewsItemSubType::minorCompetitor;
                }
            }

            auto newsSettings = Config::get().news_settings[static_cast<uint8_t>(messageSubType)];

            if (newsSettings == NewsType::none)
            {
                news->var_C8 = 0xFFFF;
                return;
            }

            if (newsSettings == NewsType::ticker)
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

                auto skin = ObjectManager::get<InterfaceSkinObject>();
                window->setColour(WindowColour::primary, Colour::translucent(skin->colour_0C));

                window->var_852 = 0;

                return;
            }
        }

        if (!isOld)
        {
            Audio::SoundId soundId = Audio::SoundId::notification;

            if (news->companyId == CompanyId::null || news->companyId == _playerCompany)
            {
                Audio::SoundId messageSounds[31] = {
                    Audio::SoundId::notification,
                    Audio::SoundId::newsAwww,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::notification,
                    Audio::SoundId::applause2,
                    Audio::SoundId::applause2,
                    Audio::SoundId::newsOooh,
                    Audio::SoundId::applause2,
                    Audio::SoundId::newsOooh,
                    Audio::SoundId::newsOooh,
                    Audio::SoundId::newsAwww,
                    Audio::SoundId::applause2,
                    Audio::SoundId::newsAwww,
                    Audio::SoundId::newsAwww,
                    Audio::SoundId::newsAwww,
                    Audio::SoundId::newsAwww,
                    Audio::SoundId::newsAwww,
                    Audio::SoundId::applause2,
                    Audio::SoundId::notification,
                    Audio::SoundId::newsAwww,
                    Audio::SoundId::applause2,
                    Audio::SoundId::newsOooh
                };

                soundId = messageSounds[activeMessage];
            }

            if (soundId != Audio::SoundId::null)
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

    void close(Window* self)
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
