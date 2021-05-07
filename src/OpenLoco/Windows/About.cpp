#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::About
{
    constexpr Gfx::ui_size_t windowSize = { 400, 260 };

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

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, windowSize, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { windowSize.width - 2, 13 }, widget_type::caption_25, 0, StringIds::about_locomotion_caption),
        makeWidget({ windowSize.width - 15, 2 }, { 13, 13 }, widget_type::wt_9, 0, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { windowSize.width, 245 }, widget_type::panel, 1),
        makeWidget({ 100, 234 }, { windowSize.width / 2, 12 }, widget_type::wt_11, 1, StringIds::music_acknowledgements_btn),
        widgetEnd(),
    };

    static window_event_list _events;

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
        window->enabled_widgets = (1 << widx::close) | (1 << widx::music_acknowledgements_btn);
        window->initScrollWidgets();

        const auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->colours[0] = interface->colour_0B;
        window->colours[1] = interface->colour_10;
    }

    // 0x0043B4AF
    static void onMouseUp(Ui::window* const window, const widget_index widgetIndex)
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
    static void draw(Ui::window* const window, Gfx::Context* const context)
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
        _events.on_mouse_up = onMouseUp;
    }
}
