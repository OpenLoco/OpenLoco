#include "Audio/Audio.h"
#include "Config.h"
#include "Date.h"
#include "GameCommands/GameCommands.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Intro.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Message.h"
#include "MessageManager.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "Ui/Dropdown.h"
#include "Ui/ScrollView.h"
#include "Ui/ToolManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CheckboxWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/TabWidget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

namespace OpenLoco::Ui::Windows::MessageWindow
{
    namespace Common
    {
        enum widx
        {
            frame = 0,
            caption = 1,
            close_button = 2,
            panel = 3,
            tab_messages,
            tab_settings,
        };

        const uint64_t enabledWidgets = (1 << widx::close_button) | (1 << widx::tab_messages) | (1 << widx::tab_settings);

        static constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight, StringId windowCaptionId)
        {
            return makeWidgets(
                Widgets::Frame({ 0, 0 }, { frameWidth, frameHeight }, WindowColour::primary),
                makeWidget({ 1, 1 }, { frameWidth - 2, 13 }, WidgetType::caption_24, WindowColour::primary, windowCaptionId),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Panel({ 0, 41 }, { 366, 175 }, WindowColour::secondary),
                Widgets::Tab({ 3, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_recent_messages),
                Widgets::Tab({ 34, 15 }, { 31, 27 }, WindowColour::secondary, ImageIds::tab, StringIds::tooltip_message_options));
        }

        static void prepareDraw(Window& self);
        static void switchTab(Window* self, WidgetIndex_t widgetIndex);
        static void onUpdate(Window& self);
        static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx);
    }

    namespace Messages
    {
        static constexpr Ui::Size32 kMinWindowSize = { 366, 217 };
        static constexpr Ui::Size32 kMaxWindowSize = { 366, 1200 };
        static int8_t messageHeight = 39;

        enum widx
        {
            scrollview = 6,
        };

        const uint64_t enabledWidgets = Common::enabledWidgets | (1 << scrollview);

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(366, 217, StringIds::title_messages),
            makeWidget({ 3, 45 }, { 360, 146 }, WidgetType::scrollview, WindowColour::secondary, Scrollbars::vertical)

        );

        // 0x0042A6F5
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_messages:
                case Common::widx::tab_settings:
                    Common::switchTab(&self, widgetIndex);
                    break;
            }
        }

        // 0x0042A95A
        static void onResize(Window& self)
        {
            auto scrollview = self.widgets[widx::scrollview];
            auto scrollarea = self.scrollAreas[0];

            auto y = scrollarea.contentHeight - scrollview.height() - 1;
            y = std::max(0, y);

            if (y < scrollarea.contentOffsetY)
            {
                scrollarea.contentOffsetY = y;
                Ui::ScrollView::updateThumbs(&self, widx::scrollview);
                self.invalidate();
            }
        }

        // 0x0042A847
        static void event_08(Window& self)
        {
            self.flags |= WindowFlags::notScrollView;
        }

        // 0x0042A84F
        static void event_09(Window& self)
        {
            if (!self.hasFlags(WindowFlags::notScrollView))
            {
                return;
            }

            if (self.rowHover == -1)
            {
                return;
            }

            self.rowHover = -1;
            self.invalidate();
        }

        // 0x0042A871
        static void getScrollSize([[maybe_unused]] Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] uint16_t* scrollWidth, uint16_t* scrollHeight)
        {
            *scrollHeight = MessageManager::getNumMessages() * messageHeight;
        }

        // 0x0042A8B9
        static void scrollMouseDown(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scrollIndex)
        {
            auto messageIndex = y / messageHeight;

            if (messageIndex >= MessageManager::getNumMessages())
            {
                return;
            }

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

            auto message = MessageManager::get(MessageId(messageIndex));
            message->setUserSelected();
            message->timeActive++;

            NewsWindow::open(MessageId(messageIndex));

            int32_t pan = self.width / 2 + self.x;
            Audio::playSound(Audio::SoundId::clickDown, pan);
        }

        // 0x0042A87C
        static void scrollMouseOver(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scrollIndex)
        {
            self.flags &= ~(WindowFlags::notScrollView);

            auto messageIndex = y / messageHeight;
            auto messageId = 0xFFFF;

            if (messageIndex < MessageManager::getNumMessages())
            {
                messageId = messageIndex;
            }

            if (self.rowHover != messageId)
            {
                self.rowHover = messageId;
                self.invalidate();
            }
        }

        // 0x0042A70C
        static std::optional<FormatArguments> tooltip([[maybe_unused]] Ui::Window& self, [[maybe_unused]] WidgetIndex_t widgetIndex)
        {
            FormatArguments args{};
            args.push(StringIds::tooltip_scroll_message_list);
            return args;
        }

        // 0x0042A545
        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            self.widgets[widx::scrollview].right = self.width - 4;
            self.widgets[widx::scrollview].bottom = self.height - 14;
        }

        // 0x0042A5CC
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);
        }

        // 0x0042A5D7
        static void drawScroll(Ui::Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
        {
            auto colour = Colours::getShade(self.getColour(WindowColour::secondary).c(), 4);

            const auto& rt = drawingCtx.currentRenderTarget();
            auto tr = Gfx::TextRenderer(drawingCtx);

            drawingCtx.clearSingle(colour);

            auto height = 0;
            for (auto i = 0; i < MessageManager::getNumMessages(); i++)
            {
                if (height + messageHeight <= rt.y)
                {
                    height += messageHeight;
                    continue;
                }
                else if (height >= rt.y + rt.height)
                {
                    break;
                }

                auto message = MessageManager::get(MessageId(i));
                char* buffer = message->messageString;
                auto str = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

                const size_t bufferLength = 512;
                strncpy(str, buffer, bufferLength);

                auto stringId = StringIds::black_stringid;

                if (self.rowHover == i)
                {
                    drawingCtx.drawRect(0, height, self.width, 38, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                    stringId = StringIds::wcolour2_stringid;
                }

                {
                    auto args = FormatArguments();
                    args.push(StringIds::tiny_font_date);
                    args.push(message->date);

                    auto point = Point(0, height);
                    tr.drawStringLeft(point, Colour::black, stringId, args);
                }
                {
                    auto args = FormatArguments();
                    args.push(StringIds::buffer_2039);

                    auto width = self.widgets[widx::scrollview].width() - 14;
                    auto point = Point(0, height + 6);
                    tr.drawStringLeftWrapped(point, width, Colour::black, stringId, args);
                    height += messageHeight;
                }
            }
        }

        // 0x0042A7B9
        static void tabReset(Window* self)
        {
            self->minWidth = kMinWindowSize.width;
            self->minHeight = kMinWindowSize.height;
            self->maxWidth = kMaxWindowSize.width;
            self->maxHeight = kMaxWindowSize.height;
            self->width = kMinWindowSize.width;
            self->height = kMinWindowSize.height;
            self->rowHover = -1;
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onResize = onResize,
            .onUpdate = Common::onUpdate,
            .event_08 = event_08,
            .event_09 = event_09,
            .getScrollSize = getScrollSize,
            .scrollMouseDown = scrollMouseDown,
            .scrollMouseOver = scrollMouseOver,
            .tooltip = tooltip,
            .prepareDraw = prepareDraw,
            .draw = draw,
            .drawScroll = drawScroll,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    // 0x0042A3FF
    void open()
    {
        auto window = WindowManager::bringToFront(WindowType::messages);

        if (window != nullptr)
        {
            if (ToolManager::isToolActive(window->type, window->number))
            {
                ToolManager::toolCancel();
                window = WindowManager::bringToFront(WindowType::messages);
            }
        }

        if (window == nullptr)
        {
            int16_t y = 29;
            int16_t x = Ui::width() - 366;

            window = WindowManager::createWindow(
                WindowType::messages,
                { x, y },
                { 366, 217 },
                WindowFlags::flag_11,
                Messages::getEvents());

            window->enabledWidgets = Messages::enabledWidgets;
            window->number = 0;
            window->currentTab = 0;
            window->frameNo = 0;
            window->rowHover = -1;
            window->disabledWidgets = 0;

            WindowManager::sub_4CEE0B(*window);

            window->minWidth = Messages::kMinWindowSize.width;
            window->minHeight = Messages::kMinWindowSize.height;
            window->maxWidth = Messages::kMaxWindowSize.width;
            window->maxHeight = Messages::kMaxWindowSize.height;
            window->flags |= WindowFlags::resizable;

            window->owner = CompanyManager::getControllingId();
            auto skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->colour_0A);

            window->width = Messages::kMinWindowSize.width;
            window->height = Messages::kMinWindowSize.height;
        }

        window->currentTab = 0;
        window->invalidate();

        window->setWidgets(Messages::widgets);
        window->enabledWidgets = Messages::enabledWidgets;
        window->holdableWidgets = 0;
        window->eventHandlers = &Messages::getEvents();
        window->disabledWidgets = 0;

        window->callOnResize();
        window->callPrepareDraw();
        window->initScrollWidgets();

        uint16_t scrollHeight = 0;
        window->callGetScrollSize(0, nullptr, &scrollHeight);

        scrollHeight -= window->widgets[Messages::widx::scrollview].height();

        if (static_cast<int16_t>(scrollHeight) < 0)
        {
            scrollHeight = 0;
        }

        window->scrollAreas[0].contentOffsetY = scrollHeight;

        Ui::ScrollView::updateThumbs(window, Messages::widx::scrollview);
    }

    namespace Settings
    {
        static constexpr Ui::Size32 kWindowSize = { 366, 155 };

        enum widx
        {
            company_major_news = 6,
            company_major_news_dropdown,
            competitor_major_news,
            competitor_major_news_dropdown,
            company_minor_news,
            company_minor_news_dropdown,
            competitor_minor_news,
            competitor_minor_news_dropdown,
            general_news,
            general_news_dropdown,
            advice,
            advice_dropdown,
            playSoundEffects,
        };

        static constexpr uint64_t enabledWidgets = Common::enabledWidgets | (1 << widx::company_major_news) | (1 << widx::company_major_news_dropdown) | (1 << widx::competitor_major_news) | (1 << widx::competitor_major_news_dropdown) | (1 << widx::company_minor_news) | (1 << widx::company_minor_news_dropdown) | (1 << widx::competitor_minor_news) | (1 << widx::competitor_minor_news_dropdown) | (1 << widx::general_news) | (1 << widx::general_news_dropdown) | (1 << widx::advice) | (1 << widx::advice_dropdown) | (1 << widx::playSoundEffects);

        static constexpr auto widgets = makeWidgets(
            Common::makeCommonWidgets(366, 155, StringIds::title_messages),
            makeDropdownWidgets({ 236, 47 }, { 124, 12 }, WindowColour::secondary),
            makeDropdownWidgets({ 236, 62 }, { 124, 12 }, WindowColour::secondary),
            makeDropdownWidgets({ 236, 77 }, { 124, 12 }, WindowColour::secondary),
            makeDropdownWidgets({ 236, 92 }, { 124, 12 }, WindowColour::secondary),
            makeDropdownWidgets({ 236, 107 }, { 124, 12 }, WindowColour::secondary),
            makeDropdownWidgets({ 236, 122 }, { 124, 12 }, WindowColour::secondary),
            Widgets::Checkbox({ 4, 137 }, { 346, 12 }, WindowColour::secondary, StringIds::playNewsSoundEffects, StringIds::playNewsSoundEffectsTip)

        );

        // 0x0042AA84
        static void onMouseUp(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case Common::widx::close_button:
                    WindowManager::close(&self);
                    break;

                case Common::widx::tab_messages:
                case Common::widx::tab_settings:
                    Common::switchTab(&self, widgetIndex);
                    break;

                case widx::playSoundEffects:
                {
                    Config::get().audio.playNewsSounds ^= 1;
                    Config::write();
                    WindowManager::invalidateWidget(WindowType::messages, self.number, widgetIndex);
                    break;
                }
            }
        }

        // 0x0042AA9F
        static void onMouseDown(Window& self, WidgetIndex_t widgetIndex)
        {
            switch (widgetIndex)
            {
                case widx::company_major_news:
                case widx::company_major_news_dropdown:
                case widx::competitor_major_news:
                case widx::competitor_major_news_dropdown:
                case widx::company_minor_news:
                case widx::company_minor_news_dropdown:
                case widx::competitor_minor_news:
                case widx::competitor_minor_news_dropdown:
                case widx::general_news:
                case widx::general_news_dropdown:
                case widx::advice:
                case widx::advice_dropdown:
                {
                    auto widget = self.widgets[widgetIndex - 1];
                    auto xPos = widget.left + self.x;
                    auto yPos = widget.top + self.y;
                    auto width = widget.width() - 2;
                    auto height = widget.height() + 2;
                    auto flags = 1 << 7;

                    Dropdown::show(xPos, yPos, width, height, self.getColour(WindowColour::secondary), 3, flags);

                    Dropdown::add(0, StringIds::dropdown_stringid, StringIds::message_off);
                    Dropdown::add(1, StringIds::dropdown_stringid, StringIds::message_ticker);
                    Dropdown::add(2, StringIds::dropdown_stringid, StringIds::message_window);

                    auto dropdownIndex = Config::get().old.newsSettings[(widgetIndex - 7) / 2];

                    Dropdown::setItemSelected(static_cast<size_t>(dropdownIndex));
                    break;
                }
            }
        }

        // 0x0042AAAC
        static void onDropdown([[maybe_unused]] Window& self, Ui::WidgetIndex_t widgetIndex, int16_t itemIndex)
        {
            switch (widgetIndex)
            {
                case widx::company_major_news:
                case widx::company_major_news_dropdown:
                case widx::competitor_major_news:
                case widx::competitor_major_news_dropdown:
                case widx::company_minor_news:
                case widx::company_minor_news_dropdown:
                case widx::competitor_minor_news:
                case widx::competitor_minor_news_dropdown:
                case widx::general_news:
                case widx::general_news_dropdown:
                case widx::advice:
                case widx::advice_dropdown:
                {
                    if (itemIndex == -1)
                    {
                        return;
                    }

                    auto dropdownIndex = (widgetIndex - 7) / 2;

                    if (static_cast<Config::NewsType>(itemIndex) != Config::get().old.newsSettings[dropdownIndex])
                    {
                        Config::get().old.newsSettings[dropdownIndex] = static_cast<Config::NewsType>(itemIndex);
                        Config::write();
                        Gfx::invalidateScreen();
                    }
                    break;
                }
            }
        }

        static void prepareDraw(Window& self)
        {
            Common::prepareDraw(self);

            if (Config::get().audio.playNewsSounds)
            {
                self.activatedWidgets |= (1 << widx::playSoundEffects);
            }
            else
            {
                self.activatedWidgets &= ~(1 << widx::playSoundEffects);
            }
        }

        // 0x0042AA02
        static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
        {
            auto tr = Gfx::TextRenderer(drawingCtx);

            self.draw(drawingCtx);
            Common::drawTabs(&self, drawingCtx);
            auto yPos = self.widgets[widx::company_major_news].top + self.y;

            const StringId newsStringIds[] = {
                StringIds::company_major_news,
                StringIds::competitor_major_news,
                StringIds::company_minor_news,
                StringIds::competitor_minor_news,
                StringIds::general_news,
                StringIds::advice,
            };

            const StringId newsDropdownStringIds[] = {
                StringIds::message_off,
                StringIds::message_ticker,
                StringIds::message_window,
            };

            for (auto i = 0; i < 6; i++)
            {
                {
                    auto args = FormatArguments();
                    args.push(newsStringIds[i]);

                    auto point = Point(self.x + 4, yPos);
                    tr.drawStringLeft(point, Colour::black, StringIds::wcolour2_stringid, args);
                }

                {
                    auto xPos = self.widgets[widx::company_major_news].left + self.x + 1;
                    auto args = FormatArguments();
                    args.push(newsDropdownStringIds[static_cast<uint8_t>(Config::get().old.newsSettings[i])]);

                    auto point = Point(xPos, yPos);
                    tr.drawStringLeft(point, Colour::black, StringIds::black_stringid, args);
                }
                yPos += 15;
            }
        }

        // 0x0042A7E8
        static void tabReset(Window* self)
        {
            self->minWidth = kWindowSize.width;
            self->minHeight = kWindowSize.height;
            self->maxWidth = kWindowSize.width;
            self->maxHeight = kWindowSize.height;
            self->width = kWindowSize.width;
            self->height = kWindowSize.height;
        }

        static constexpr WindowEventList kEvents = {
            .onMouseUp = onMouseUp,
            .onMouseDown = onMouseDown,
            .onDropdown = onDropdown,
            .onUpdate = Common::onUpdate,
            .prepareDraw = prepareDraw,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }

    namespace Common
    {
        struct TabInformation
        {
            std::span<const Widget> widgets;
            const widx widgetIndex;
            const WindowEventList& events;
            const uint64_t enabledWidgets;
        };

        static TabInformation tabInformationByTabOffset[] = {
            { Messages::widgets, widx::tab_messages, Messages::getEvents(), Messages::enabledWidgets },
            { Settings::widgets, widx::tab_settings, Settings::getEvents(), Settings::enabledWidgets },
        };

        static void prepareDraw(Window& self)
        {
            // Activate the current tab..
            self.activatedWidgets &= ~((1ULL << tab_messages) | (1ULL << tab_settings));
            self.activatedWidgets |= (1ULL << tabInformationByTabOffset[self.currentTab].widgetIndex);

            self.widgets[Common::widx::frame].right = self.width - 1;
            self.widgets[Common::widx::frame].bottom = self.height - 1;

            self.widgets[Common::widx::panel].right = self.width - 1;
            self.widgets[Common::widx::panel].bottom = self.height - 1;

            self.widgets[Common::widx::caption].right = self.width - 2;

            self.widgets[Common::widx::close_button].left = self.width - 15;
            self.widgets[Common::widx::close_button].right = self.width - 3;
        }

        // 0x0042A716
        static void switchTab(Window* self, WidgetIndex_t widgetIndex)
        {
            if (ToolManager::isToolActive(self->type, self->number))
            {
                ToolManager::toolCancel();
            }

            self->currentTab = widgetIndex - widx::tab_messages;
            self->frameNo = 0;
            self->flags &= ~(WindowFlags::flag_16);

            self->viewportRemove(0);

            const auto& tabInfo = tabInformationByTabOffset[widgetIndex - widx::tab_messages];

            self->enabledWidgets = tabInfo.enabledWidgets;
            self->holdableWidgets = 0;
            self->eventHandlers = &tabInfo.events;
            self->activatedWidgets = 0;
            self->setWidgets(tabInfo.widgets);
            self->disabledWidgets = 0;

            self->invalidate();

            if (self->currentTab == widx::tab_messages - widx::tab_messages)
            {
                Messages::tabReset(self);
            }
            if (self->currentTab == widx::tab_settings - widx::tab_messages)
            {
                Settings::tabReset(self);
            }

            self->callOnResize();
            self->callPrepareDraw();
            self->initScrollWidgets();
            self->invalidate();
            self->moveInsideScreenEdges();
        }

        // 0x0042AB92
        static void drawTabs(Window* self, Gfx::DrawingContext& drawingCtx)
        {
            auto skin = ObjectManager::get<InterfaceSkinObject>();

            // Message Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_messages;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_messages);
            }

            // Setting Tab
            {
                uint32_t imageId = skin->img;
                imageId += InterfaceSkin::ImageIds::tab_message_settings;

                Widget::drawTab(self, drawingCtx, imageId, widx::tab_settings);
            }
        }

        // 0x0042A826 and 0x0042AB6A
        static void onUpdate(Window& self)
        {
            self.frameNo++;
            self.callPrepareDraw();
            WindowManager::invalidateWidget(WindowType::messages, self.number, self.currentTab + Common::widx::tab_messages);
        }
    }
}
