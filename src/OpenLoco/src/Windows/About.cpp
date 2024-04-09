#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/WindowManager.h"
#include "Widget.h"

namespace OpenLoco::Ui::Windows::About
{
    static constexpr Ui::Size kWindowSize = { 400, 260 };

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
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { kWindowSize.width - 2, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::about_locomotion_caption),
        makeWidget({ kWindowSize.width - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        makeWidget({ 0, 15 }, { kWindowSize.width, 245 }, WidgetType::panel, WindowColour::secondary),
        makeWidget({ 100, 234 }, { kWindowSize.width / 2, 12 }, WidgetType::button, WindowColour::secondary, StringIds::music_acknowledgements_btn),
        widgetEnd(),
    };

    static const WindowEventList& getEvents();

    // 0x0043B26C
    void open()
    {
        if (WindowManager::bringToFront(WindowType::about) != nullptr)
            return;

        auto window = WindowManager::createWindowCentred(
            WindowType::about,
            kWindowSize,
            WindowFlags::none,
            getEvents());

        window->widgets = _widgets;
        window->enabledWidgets = (1 << widx::close) | (1 << widx::music_acknowledgements_btn);
        window->initScrollWidgets();

        const auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->colour_0B);
        window->setColour(WindowColour::secondary, interface->colour_10);
    }

    // 0x0043B4AF
    static void onMouseUp(Ui::Window& window, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close:
                WindowManager::close(window.type);
                break;

            case widx::music_acknowledgements_btn:
                AboutMusic::open();
                break;
        }
    }

    // 0x0043B2E4
    static void draw(Ui::Window& window, Gfx::RenderTarget* const rt)
    {
        // Draw widgets.
        window.draw(rt);

        auto point = Point(window.x + kWindowSize.width / 2, window.y + 25);

        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_69, nullptr);

        point.y += 10;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_70, nullptr);

        // Chris Sawyer logo
        drawingCtx.drawImage(rt, window.x + 92, window.y + 52, ImageIds::chris_sawyer_logo_small);

        point.y += 79;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_71, nullptr);

        point.y += 10;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_72, nullptr);

        point.y += 10;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_73, nullptr);

        point.y += 10;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_74, nullptr);

        point.y += 13;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_75, nullptr);

        point.y += 25;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_76, nullptr);

        point.y += 10;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::about_locomotion_77, nullptr);

        // Licenced to Atari
        point.y += 25;
        drawingCtx.drawStringCentred(*rt, point, Colour::black, StringIds::licenced_to_atari_inc, nullptr);
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
