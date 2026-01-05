#include "Audio/Audio.h"
#include "Config.h"
#include "Graphics/Colour.h"
#include "Message.h"
#include "MessageManager.h"
#include "News.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "SceneManager.h"
#include "Ui/Widgets/NewsPanelWidget.h"
#include "World/CompanyManager.h"

namespace OpenLoco::Ui::Windows::NewsWindow
{
    NewsState _nState{};

    static void createNewsWindow(Ui::Size32 kWindowSize, std::span<const Widget> widgets, AdvancedColour colour, bool isOld, WindowFlags flags)
    {
        _nState.slideInHeight = 5;

        int16_t y = Ui::height() - _nState.slideInHeight;

        if (SceneManager::getGameSpeed() != GameSpeed::Normal || isOld)
        {
            y = Ui::height() - kWindowSize.height;
            _nState.slideInHeight = kWindowSize.height;
        }

        int16_t x = (Ui::width() / 2) - (kWindowSize.width / 2);

        auto window = WindowManager::createWindow(
            WindowType::news,
            { x, y },
            kWindowSize,
            flags,
            Common::getEvents());

        window->setWidgets(widgets);
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, colour);

        _nState.savedView[0].clear();
        _nState.savedView[1].clear();

        Common::initViewports(*window);
    }

    // 0x00428F8B
    void open(MessageId messageIndex)
    {
        bool isOld = false;
        auto news = MessageManager::get(messageIndex);
        if (news == nullptr)
        {
            return;
        }

        if ((news->timeActive != 0) && (SceneManager::getSceneAge() >= 10))
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
                if (news->companyId != CompanyManager::getControllingId())
                {
                    messageSubType = MessageCriticality::majorCompetitor;
                }
            }

            if (messageSubType == MessageCriticality::minorCompany)
            {
                if (news->companyId != CompanyManager::getControllingId())
                {
                    messageSubType = MessageCriticality::minorCompetitor;
                }
            }

            auto newsSettings = Config::get().newsSettings[static_cast<uint8_t>(messageSubType)];

            if (newsSettings == Config::NewsType::none)
            {
                news->setActive(false);
                return;
            }

            if (newsSettings == Config::NewsType::ticker)
            {
                _nState.numCharsToDisplay = 0;
                WindowFlags flags = WindowFlags::stickToFront | WindowFlags::viewportNoScrolling | WindowFlags::transparent | WindowFlags::ignoreInFindAt;

                auto window = WindowManager::createWindow(
                    WindowType::news,
                    { Ui::width() - 138, Ui::height() - 25 },
                    Ticker::kWindowSize,
                    flags,
                    Ticker::getEvents());

                window->setWidgets(Ticker::getWidgets());
                window->initScrollWidgets();

                auto skin = ObjectManager::get<InterfaceSkinObject>();
                window->setColour(WindowColour::primary, AdvancedColour(skin->windowColour).translucent());

                window->var_852 = 0;

                return;
            }
        }

        if (!isOld)
        {
            Audio::SoundId soundId = Audio::SoundId::notification;

            if (!Config::get().audio.playNewsSounds)
            {
                soundId = Audio::SoundId::null;
            }
            else if (news->companyId == CompanyId::null || news->companyId == CompanyManager::getControllingId())
            {
                soundId = mtd.sound;
            }

            if (soundId != Audio::SoundId::null)
            {
                int32_t pan = Ui::width() / 2;
                Audio::playSound(soundId, pan);
            }
        }

        if (mtd.hasFlag(MessageTypeFlags::isGeneralNews))
        {
            WindowFlags flags = WindowFlags::stickToFront | WindowFlags::viewportNoScrolling | WindowFlags::transparent | WindowFlags::noBackground;

            createNewsWindow(News2::kWindowSize, News2::getWidgets(), Colour::grey, isOld, flags);
        }
        else
        {
            WindowFlags flags = WindowFlags::stickToFront | WindowFlags::viewportNoScrolling | WindowFlags::transparent;
            constexpr auto colour = AdvancedColour(Colour::mutedDarkRed).translucent();

            createNewsWindow(News1::kWindowSize, News1::getWidgets(), colour, isOld, flags);
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
                {
                    message->setActive(false);
                }
            }
        }

        MessageManager::setActiveIndex(MessageId::null);
        WindowManager::close(WindowType::news, 0);

        if (MessageManager::getNumMessages() != 0)
        {
            auto message = MessageManager::get(MessageId(MessageManager::getNumMessages() - 1));
            message->setUserSelected();
            message->timeActive++;

            NewsWindow::open(MessageId(MessageManager::getNumMessages() - 1));
        }
    }

    void close(Window* self)
    {
        // Only affects the newspaper view; the ticker ignores this widget
        self->callOnMouseUp(Common::close_button, self->widgets[Common::close_button].id);
    }
}
