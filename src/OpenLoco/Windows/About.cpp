#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

namespace OpenLoco::Ui::Windows::About
{
    constexpr Ui::Size windowSize = { 400, 260 };

    namespace widx
    {
        enum
        {
            frame,
            title,
            close,
            panel,
            music_acknowledgements_btn,
        };
    }

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, windowSize, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { windowSize.width - 2, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::about_locomotion_caption),
        makeWidget({ windowSize.width - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { windowSize.width, 245 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 100, 234 }, { windowSize.width / 2, 12 }, WidgetType::button, WindowColour::secondary, StringIds::music_acknowledgements_btn),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void initEvents();

    // 0x0043B26C
    void open()
    {
        if (WindowManager::bringToFront(WindowType::about) != nullptr)
            return;

        initEvents();

        auto window = WindowManager::createWindowCentred(
            WindowType::about,
            windowSize,
            0,
            &_events);

        window->widgets = _widgets;
        window->enabledWidgets = (1 << widx::close) | (1 << widx::music_acknowledgements_btn);
        window->initScrollWidgets();

        const auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->colour_0B);
        window->setColour(WindowColour::secondary, interface->colour_10);
    }

    // 0x0043B4AF
    static void onMouseUp(Ui::Window* const window, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window->type);
                break;

            case widx::music_acknowledgements_btn:
                AboutMusic::open();
                break;
        }
    }

    // 0x0043B2E4
    static void draw(Ui::Window* const window, Gfx::Context* const context)
    {
        // Draw widgets.
        window->draw(context);

        const int16_t x = window->x + windowSize.width / 2;
        int16_t y = window->y + 25;

        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_69, nullptr);

        y += 10;
        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_70, nullptr);

        // Chris Sawyer logo
        drawImage(context, window->x + 92, window->y + 52, ImageIds::chris_sawyer_logo_small);

        y += 79;
        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_71, nullptr);

        y += 10;
        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_72, nullptr);

        y += 10;
        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_73, nullptr);

        y += 10;
        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_74, nullptr);

        y += 13;
        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_75, nullptr);

        y += 25;
        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_76, nullptr);

        y += 10;
        drawStringCentred(*context, x, y, Colour::black, StringIds::about_locomotion_77, nullptr);

        // Licenced to Atari
        y += 25;
        drawStringCentred(*context, x, y, Colour::black, StringIds::licenced_to_atari_inc, nullptr);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.onMouseUp = onMouseUp;
    }
}
