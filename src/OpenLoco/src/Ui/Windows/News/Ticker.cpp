#include "Audio/Audio.h"
#include "Graphics/Colour.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Message.h"
#include "MessageManager.h"
#include "News.h"
#include "SceneManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/Wt3Widget.h"

namespace OpenLoco::Ui::Windows::NewsWindow::Ticker
{
    static constexpr auto widgets = makeWidgets(
        Widgets::Wt3Widget({ 0, 0 }, { 111, 26 }, WindowColour::primary)

    );

    std::span<const Widget> getWidgets()
    {
        return widgets;
    }

    // 0x00429EA2
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        if (widgetIndex != 0)
        {
            return;
        }

        if (MessageManager::getActiveIndex() == MessageId::null)
        {
            return;
        }

        auto news = MessageManager::get(MessageManager::getActiveIndex());
        news->setActive(true);
        news->timeActive++;

        auto activeMessageIndex = MessageManager::getActiveIndex();
        MessageManager::setActiveIndex(MessageId::null);

        WindowManager::close(&self);
        open(activeMessageIndex);
    }

    // 0x00429FE4
    static void onResize(Window& self)
    {
        auto y = Ui::height() - kWindowSize.height + 1;
        auto x = Ui::width() - kWindowSize.width - 27;
        auto height = kWindowSize.height - 1;

        if (y != self.y || x != self.x || kWindowSize.width != self.width || height != self.height)
        {
            self.invalidate();
            self.y = y;
            self.x = x;
            self.width = kWindowSize.width;
            self.height = height;
            self.invalidate();
        }
    }

    // 0x00429EEB
    static void onUpdate(Window& self)
    {
        auto cursor = Input::getMouseLocation2();
        auto window = WindowManager::findAtAlt(cursor.x, cursor.y);

        if (window == &self)
        {
            self.var_852 = 12;
        }

        if (self.var_852 != 0)
        {
            if (!SceneManager::isPaused())
            {
                self.var_852--;
            }
        }

        self.invalidate();

        if (self.var_852 == 0 && !SceneManager::isPaused())
        {
            _nState.numCharsToDisplay = _nState.numCharsToDisplay + 2;

            if (!((_nState.numCharsToDisplay & (1 << 15)) || _nState.numCharsToDisplay & 7))
            {
                if (MessageManager::getActiveIndex() != MessageId::null)
                {
                    auto news = MessageManager::get(MessageManager::getActiveIndex());
                    auto cx = _nState.numCharsToDisplay >> 2;
                    char* newsString = news->messageString;
                    auto newsStringChar = *newsString;

                    while (true)
                    {
                        newsStringChar = *newsString;
                        if (newsStringChar == ControlCodes::newline)
                        {
                            newsStringChar = ' ';
                            cx--;
                            if (cx < 0)
                            {
                                break;
                            }
                        }

                        if (newsStringChar != static_cast<char>(-1))
                        {
                            cx--;
                            if (cx < 0)
                            {
                                break;
                            }
                            newsString++;
                            if (!newsStringChar)
                            {
                                break;
                            }
                        }
                        else
                        {
                            cx--;
                            if (cx < 0)
                            {
                                break;
                            }
                            newsString += 3;
                        }
                    }

                    if (newsStringChar != ' ')
                    {
                        if (newsStringChar != 0)
                        {
                            Audio::playSound(Audio::SoundId::ticker, Ui::width());
                        }
                    }
                }
            }
        }

        if (MessageManager::getActiveIndex() != MessageId::null)
        {
            return;
        }

        MessageManager::setActiveIndex(MessageId::null);

        WindowManager::close(&self);
    }

    // 0x00429DAA
    static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        if (self.var_852 != 0)
        {
            return;
        }

        if ((SceneManager::getPauseFlags() & PauseFlags::browsePrompt) != PauseFlags::none)
        {
            return;
        }

        auto news = MessageManager::get(MessageManager::getActiveIndex());

        auto x = self.x;
        auto y = self.y;
        auto width = self.width;
        auto height = self.height;

        const auto& rt = drawingCtx.currentRenderTarget();
        auto clipped = Gfx::clipRenderTarget(rt, { x, y, width, height });

        if (!clipped)
        {
            return;
        }

        drawingCtx.pushRenderTarget(*clipped);

        auto colour = Colours::getShade(Colour::white, 5);
        const auto& mtd = getMessageTypeDescriptor(news->type);

        if (!mtd.hasFlag(MessageTypeFlags::renderNewspaper))
        {
            colour = Colours::getShade(Colour::mutedDarkRed, 5);
        }

        auto tr = Gfx::TextRenderer(drawingCtx);
        drawingCtx.clearSingle(colour);

        char* newsString = news->messageString;
        auto buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_2039));

        *buffer = ControlCodes::Colour::black;
        buffer++;
        *buffer = ControlCodes::Font::small;
        buffer++;

        auto newsStringChar = *newsString;
        auto i = 0;

        while (true)
        {
            newsStringChar = *newsString;
            if (newsStringChar == ControlCodes::newline)
            {
                newsStringChar = ' ';
                *buffer = newsStringChar;
                buffer++;
            }

            if (newsStringChar == static_cast<char>(-1))
            {
                *buffer++ = *newsString++;
                *buffer++ = *newsString++;
                newsStringChar = *newsString;
            }

            *buffer = newsStringChar;
            buffer++;
            newsString++;
            i++;
            if (!newsStringChar)
            {
                break;
            }
        }

        if ((_nState.numCharsToDisplay >> 2) > i)
        {
            _nState.numCharsToDisplay = _nState.numCharsToDisplay | (1 << 15);
        }

        auto point = Point(55, 0);
        tr.drawStringTicker(point, StringIds::buffer_2039, Colour::black, 4, ((_nState.numCharsToDisplay & ~(1 << 15)) >> 2), 109);

        drawingCtx.popRenderTarget();
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onResize = onResize,
        .onUpdate = onUpdate,
        .draw = draw,
    };

    const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
