#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/TextInput.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/TextBoxWidget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

#include <SDL2/SDL.h>

namespace OpenLoco::Ui::Windows::TextInput
{
    static int16_t _callingWidget;
    static WindowNumber_t _callingWindowNumber;
    static WindowType _callingWindowType;

    static FormatArgumentsBuffer _formatArgs;
    static StringId _title;
    static StringId _message;

    static Ui::TextInput::InputSession inputSession;

    namespace Widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            input,
            charLimit,
            ok,
        };
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { 330, 90 }, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { 328, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary),
        Widgets::ImageButton({ 315, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { 330, 75 }, WindowColour::secondary),
        Widgets::TextBox({ 4, 58 }, { 322, 14 }, WindowColour::secondary),
        Widgets::Label({ 150, 75 }, { 100, 10 }, WindowColour::secondary, ContentAlign::right, StringIds::num_characters_left_int_int),
        Widgets::Button({ 256, 74 }, { 70, 12 }, WindowColour::secondary, StringIds::label_button_ok)

    );

    static const WindowEventList& getEvents();

    /**
     * 0x004CE523
     *
     * @param caller @<esi>
     * @param title @<ax>
     * @param message @<bx>
     * @param value @<cx>
     * @param callingWidget @<dx>
     */
    void openTextInput(Ui::Window* caller, StringId title, StringId message, StringId value, int callingWidget, FormatArgumentsView valueArgs, uint32_t inputSize)
    {
        _title = title;
        _message = message;

        _callingWindowType = caller->type;
        _callingWindowNumber = caller->number;
        _callingWidget = callingWidget;

        // Close any previous text input window
        cancel();

        auto window = WindowManager::createWindowCentred(
            WindowType::textInput,
            { 330, 90 },
            WindowFlags::stickToFront | WindowFlags::playSoundOnOpen,
            getEvents());
        window->setWidgets(_widgets);
        window->initScrollWidgets();

        auto commonArgs = FormatArguments::common();
        std::memcpy(_formatArgs.data(), commonArgs.getBufferStart(), std::min(commonArgs.getCapacity(), _formatArgs.capacity()));
        char temp[200] = {};
        StringManager::formatString(temp, value, valueArgs);

        inputSession = Ui::TextInput::InputSession(temp, inputSize);
        inputSession.calculateTextOffset(window->widgets[Widx::input].width() - 2);

        caller = WindowManager::find(_callingWindowType, _callingWindowNumber);

        window->setColour(WindowColour::primary, caller->getColour(WindowColour::primary));
        window->setColour(WindowColour::secondary, caller->getColour(WindowColour::secondary));
        window->owner = caller->owner;

        if (caller->type == WindowType::titleMenu)
        {
            const InterfaceSkinObject* interface = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::primary, interface->windowTitlebarColour);
            window->setColour(WindowColour::secondary, interface->windowColour);
            window->owner = CompanyId::null;
        }

        if (caller->type == WindowType::timeToolbar)
        {
            const InterfaceSkinObject* interface = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, interface->windowPlayerColor);
            window->owner = CompanyManager::getControllingId();
        }

        // TODO: Get the correct type and provide getter/setter.
        window->widgets[Widx::title].styleData = enumValue(Widgets::Caption::Style::whiteText);
        if (window->owner != CompanyId::null)
        {
            window->flags |= WindowFlags::lighterFrame;
            window->widgets[Widx::title].styleData = enumValue(Widgets::Caption::Style::colourText);
        }

        // Focus the textbox element
        Input::setFocus(window->type, window->number, Widx::input);
    }

    /**
     * 0x004CE6C9
     *
     * @param type @<cl>
     * @param number @<dx>
     */
    void sub_4CE6C9(WindowType type, WindowNumber_t number)
    {
        auto window = WindowManager::find(WindowType::textInput, 0);
        if (window == nullptr)
        {
            return;
        }

        if (_callingWindowNumber == number && _callingWindowType == type)
        {
            cancel();
        }
    }

    /**
     * 0x004CE6F2
     */
    void cancel()
    {
        WindowManager::close(WindowType::textInput);
    }

    /**
     * 0x004CE6FF
     */
    void sub_4CE6FF()
    {
        auto window = WindowManager::find(WindowType::textInput);
        if (window == nullptr)
        {
            return;
        }

        window = WindowManager::find(_callingWindowType, _callingWindowNumber);
        if (window == nullptr)
        {
            cancel();
        }
    }

    /**
     * 0x004CE726
     *
     * @param window @<esi>
     */
    static void prepareDraw(Ui::Window& self)
    {
        self.widgets[Widx::title].text = _title;
        memcpy(self.widgets[Widx::title].textArgs.data(), _formatArgs.data(), 16);

        const uint16_t numCharacters = static_cast<uint16_t>(inputSession.buffer.length());
        const uint16_t maxNumCharacters = inputSession.inputLenLimit;

        FormatArguments args{ self.widgets[Widx::charLimit].textArgs };
        args.push<uint16_t>(numCharacters);
        args.push<uint16_t>(maxNumCharacters);
    }

    /**
     * 0x004CE75B
     *
     * @param window @<esi>
     * @param context @<edi>
     */
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        const auto& rt = drawingCtx.currentRenderTarget();
        auto tr = Gfx::TextRenderer(drawingCtx);

        window.draw(drawingCtx);

        // FIXME: This is pretty horrible.
        // copy and shuffle the existing args
        auto args2 = FormatArguments{};
        args2.push(_message);
        args2.push(*reinterpret_cast<uint32_t*>(_formatArgs.data() + 8));
        args2.push(*reinterpret_cast<uint32_t*>(_formatArgs.data() + 12));

        Ui::Point position = Point(window.x + window.width / 2, window.y + 30);
        tr.drawStringCentredWrapped(position, window.width - 8, Colour::black, StringIds::wcolour2_stringid, args2);

        auto& inputWidget = window.widgets[Widx::input];
        auto clipped = Gfx::clipRenderTarget(rt, Ui::Rect(inputWidget.left + 1 + window.x, inputWidget.top + 1 + window.y, inputWidget.width() - 2, inputWidget.height() - 2));
        if (!clipped)
        {
            return;
        }

        drawingCtx.pushRenderTarget(*clipped);

        char* drawnBuffer = (char*)StringManager::getString(StringIds::buffer_2039);
        strcpy(drawnBuffer, inputSession.buffer.c_str());

        {
            FormatArguments args{};
            args.push(StringIds::buffer_2039);

            position = { inputSession.xOffset, 1 };
            tr.drawStringLeft(position, Colour::black, StringIds::black_stringid, args);
        }

        if ((inputSession.cursorFrame % 32) < 16)
        {
            strncpy(drawnBuffer, inputSession.buffer.c_str(), inputSession.cursorPosition);
            drawnBuffer[inputSession.cursorPosition] = '\0';

            if (Input::isFocused(window.type, window.number, Widx::input))
            {
                auto width = tr.getStringWidth(drawnBuffer);
                auto cursorPos = Point(inputSession.xOffset + width, 1);
                drawingCtx.fillRect(cursorPos.x, cursorPos.y, cursorPos.x, cursorPos.y + 9, Colours::getShade(window.getColour(WindowColour::secondary).c(), 9), Gfx::RectFlags::none);
            }
        }

        drawingCtx.popRenderTarget();
    }

    // 0x004CE8B6
    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case Widx::close:
                WindowManager::close(&window);
                break;
            case Widx::ok:
                inputSession.sanitizeInput();
                auto caller = WindowManager::find(_callingWindowType, _callingWindowNumber);
                if (caller != nullptr)
                {
                    caller->callTextInput(_callingWidget, caller->widgets[_callingWidget].id, inputSession.buffer.c_str());
                }
                WindowManager::close(&window);
                break;
        }
    }

    // 0x004CE8FA
    static void onUpdate(Ui::Window& window)
    {
        inputSession.cursorFrame++;
        if ((inputSession.cursorFrame % 16) == 0)
        {
            window.invalidate();
        }
    }

    // 0x004CE910
    static bool keyUp(Window& w, uint32_t charCode, uint32_t keyCode)
    {
        if (charCode == SDLK_RETURN)
        {
            w.callOnMouseUp(Widx::ok, w.widgets[Widx::ok].id);
            return true;
        }
        else if (charCode == SDLK_ESCAPE)
        {
            w.callOnMouseUp(Widx::close, w.widgets[Widx::close].id);
            return true;
        }
        else if (!Input::isFocused(w.type, w.number, Widx::input) || !inputSession.handleInput(charCode, keyCode))
        {
            return false;
        }

        WindowManager::invalidate(WindowType::textInput, 0);
        inputSession.cursorFrame = 0;

        int containerWidth = w.widgets[Widx::input].width() - 2;
        if (inputSession.needsReoffsetting(containerWidth))
        {
            inputSession.calculateTextOffset(containerWidth);
        }

        return true;
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onUpdate = onUpdate,
        .prepareDraw = prepareDraw,
        .draw = draw,
        .keyUp = keyUp,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
