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
    static void createNewsWindow(Ui::Size windowSize, Widget* widgets, AdvancedColour colour, bool isOld, uint32_t flags)
    {
        _word_525CE0 = 5;

        int16_t y = Ui::height() - _word_525CE0;

        if (getGameSpeed() != GameSpeed::Normal || isOld)
        {
            y = Ui::height() - windowSize.height;
            _word_525CE0 = windowSize.height;
        }

        int16_t x = (Ui::width() / 2) - (windowSize.width / 2);
        Ui::Point origin = { x, y };

        auto window = WindowManager::createWindow(WindowType::news, origin, windowSize, flags, &News1::events);

        window->widgets = widgets;
        window->enabledWidgets = Common::enabledWidgets;

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
    void open(MessageId messageIndex)
    {
        bool isOld = false;
        auto news = MessageManager::get(messageIndex);

        if ((news->timeActive != 0) && (getScreenAge() >= 10))
        {
            isOld = true;
        }

        MessageManager::setActiveIndex(messageIndex);

        const auto& mtd = getMessageTypeDescriptor(news->type);

        if (!isOld)
        {
            auto messageSubType = mtd.criticality;

            if (messageSubType == MessageCriticality::majorCompany)
            {
                if (news->companyId != _playerCompany)
                {
                    messageSubType = MessageCriticality::majorCompetitor;
                }
            }

            if (messageSubType == MessageCriticality::minorCompany)
            {
                if (news->companyId != _playerCompany)
                {
                    messageSubType = MessageCriticality::minorCompetitor;
                }
            }

            auto newsSettings = Config::get().newsSettings[static_cast<uint8_t>(messageSubType)];

            if (newsSettings == NewsType::none)
            {
                news->setActive(false);
                return;
            }

            if (newsSettings == NewsType::ticker)
            {
                _word_525CE0 = 0;
                int16_t x = Ui::width() - 138;
                int16_t y = Ui::height() - 25;
                Ui::Point origin = { x, y };
                uint32_t flags = WindowFlags::stickToFront | WindowFlags::viewportNoScrolling | WindowFlags::transparent | WindowFlags::flag_7;

                auto window = WindowManager::createWindow(WindowType::news, origin, Ticker::windowSize, flags, &Ticker::events);

                window->widgets = Ticker::widgets;
                window->enabledWidgets = Ticker::enabledWidgets;

                Common::initEvents();

                window->initScrollWidgets();

                auto skin = ObjectManager::get<InterfaceSkinObject>();
                window->setColour(WindowColour::primary, AdvancedColour(skin->colour_0C).translucent());

                window->var_852 = 0;

                return;
            }
        }

        if (!isOld)
        {
            Audio::SoundId soundId = Audio::SoundId::notification;

            if (news->companyId == CompanyId::null || news->companyId == _playerCompany)
            {
                soundId = mtd.sound;
            }

            if (soundId != Audio::SoundId::null)
            {
                int32_t pan = Ui::width() / 2;
                Audio::playSound(soundId, pan);
            }
        }

        if (mtd.hasFlag(MessageTypeFlags::unk1))
        {
            uint32_t flags = WindowFlags::stickToFront | WindowFlags::viewportNoScrolling | WindowFlags::transparent | WindowFlags::noBackground;

            createNewsWindow(News2::windowSize, News2::widgets, Colour::grey, isOld, flags);
        }
        else
        {
            uint32_t flags = WindowFlags::stickToFront | WindowFlags::viewportNoScrolling | WindowFlags::transparent;
            constexpr auto colour = AdvancedColour(Colour::mutedDarkRed).translucent();

            createNewsWindow(News1::windowSize, News1::widgets, colour, isOld, flags);
        }
    }

    // 0x0042AC27
    void openLastMessage()
    {
        if (MessageManager::getActiveIndex() != MessageId::null)
        {
            auto message = MessageManager::get(MessageManager::getActiveIndex());
            if (message->isActive())
            {
                // If the current active message was user selected then remove from queue of active messages
                if (message->isUserSelected())
                    message->setActive(false);
            }
        }

        MessageManager::setActiveIndex(MessageId::null);
        WindowManager::close(WindowType::news, 0);

        if (_messageCount != 0)
        {
            auto message = MessageManager::get(MessageId(_messageCount - 1));
            message->setUserSelected();
            message->timeActive++;

            NewsWindow::open(MessageId(_messageCount - 1));
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
