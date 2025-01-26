#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input/Shortcuts.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Engine/Input/ShortcutManager.h>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Input;

namespace OpenLoco::Ui::Windows::EditKeyboardShortcut
{
    static constexpr Ui::Size32 kWindowSize = { 280, 72 };

    static loco_global<uint8_t, 0x011364A4> _editingShortcutIndex;

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, kWindowSize, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { kWindowSize.width - 2, 13 }, CaptionVariant::whiteText, WindowColour::primary, StringIds::change_keyboard_shortcut),
        Widgets::ImageButton({ 265, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { kWindowSize.width, 57 }, WindowColour::secondary));

    static const WindowEventList& getEvents();

    namespace Widx
    {
        enum
        {
            frame,
            caption,
            close,
            panel,
        };
    }

    // 0x004BF7B9
    Window* open(const uint8_t shortcutIndex)
    {
        WindowManager::close(WindowType::editKeyboardShortcut);
        _editingShortcutIndex = shortcutIndex;

        auto window = WindowManager::createWindow(WindowType::editKeyboardShortcut, kWindowSize, WindowFlags::none, getEvents());

        window->setWidgets(_widgets);
        window->enabledWidgets = 1 << Widx::close;
        window->initScrollWidgets();

        const auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_10);

        return window;
    }

    // 0x004BE8DF
    static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        self.draw(drawingCtx);

        FormatArguments args{};
        args.push(ShortcutManager::getName(static_cast<Shortcut>(*_editingShortcutIndex)));
        auto point = Ui::Point(self.x + 140, self.y + 32);
        tr.drawStringCentredWrapped(point, 272, Colour::black, StringIds::change_keyboard_shortcut_desc, args);
    }

    // 0x004BE821
    static void onMouseUp(Window& self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::close:
                WindowManager::close(&self);
                return;
        }
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
