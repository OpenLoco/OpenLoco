#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/LabelWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"

namespace OpenLoco::Ui::Windows::About
{
    static constexpr Ui::Size kWindowSize = { 400, 260 };

    namespace Widx
    {
        constexpr auto close = WidgetId("close");
        constexpr auto music_acknowledgements_btn = WidgetId("music_acknowledgements_btn");
    }

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::about_locomotion_caption),
        Widgets::ImageButton(Widx::close, { kWindowSize.width - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { kWindowSize.width, 245 }, WindowColour::secondary),
        Widgets::Button(Widx::music_acknowledgements_btn, { 100, 234 }, { kWindowSize.width / 2, 12 }, WindowColour::secondary, StringIds::music_acknowledgements_btn),
        Widgets::Label({ 10, 25 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_69),
        Widgets::Label({ 10, 35 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_70),
        Widgets::Label({ 10, 114 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_71),
        Widgets::Label({ 10, 124 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_72),
        Widgets::Label({ 10, 134 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_73),
        Widgets::Label({ 10, 144 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_74),
        Widgets::Label({ 10, 157 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_75),
        Widgets::Label({ 10, 182 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_76),
        Widgets::Label({ 10, 192 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::about_locomotion_77),
        Widgets::Label({ 10, 217 }, { kWindowSize.width - 20, 10 }, WindowColour::secondary, ContentAlign::center, StringIds::licenced_to_atari_inc)

    );

    static const WindowEventList& getEvents();

    // 0x0043B26C
    void open()
    {
        if (WindowManager::bringToFront(WindowType::about) != nullptr)
        {
            return;
        }

        auto window = WindowManager::createWindowCentred(
            WindowType::about,
            kWindowSize,
            WindowFlags::none,
            getEvents());

        window->setWidgets(_widgets);
        window->initScrollWidgets();

        const auto interface = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, interface->windowTitlebarColour);
        window->setColour(WindowColour::secondary, interface->windowOptionsColour);
    }

    // 0x0043B4AF
    static void onMouseUp(Ui::Window& window, [[maybe_unused]] const WidgetIndex_t widgetIndex, const WidgetId id)
    {
        switch (id)
        {
            case Widx::close:
                WindowManager::close(window.type);
                break;

            case Widx::music_acknowledgements_btn:
                AboutMusic::open();
                break;
        }
    }

    // 0x0043B2E4
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        // Draw widgets.
        window.draw(drawingCtx);

        // Chris Sawyer logo
        drawingCtx.drawImage(window.x + 92, window.y + 52, ImageIds::chris_sawyer_logo_small);
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
