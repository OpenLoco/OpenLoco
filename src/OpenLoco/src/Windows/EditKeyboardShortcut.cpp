#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Input/Shortcuts.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Engine/Input/ShortcutManager.h>
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;
using namespace OpenLoco::Input;

namespace OpenLoco::Ui::Windows::EditKeyboardShortcut
{
    static constexpr Ui::Size kWindowSize = { 280, 72 };

    static loco_global<uint8_t, 0x011364A4> _editingShortcutIndex;

    static constexpr Widget _widgets[] = {
        makeWidget({ 0, 0 }, kWindowSize, WidgetType::frame, WindowColour::primary),                                                                     // 0,
        makeWidget({ 1, 1 }, { kWindowSize.width - 2, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::change_keyboard_shortcut),         // 1,
        makeWidget({ 265, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), // 2,
        makeWidget({ 0, 15 }, { kWindowSize.width, 57 }, WidgetType::panel, WindowColour::secondary),                                                    // 3,
        widgetEnd(),
    };

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
    static void draw(Ui::Window& self, Gfx::RenderTarget* const rt)
    {
        auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        self.draw(rt);

        FormatArguments args{};
        args.push(ShortcutManager::getName(static_cast<Shortcut>(*_editingShortcutIndex)));
        auto point = Ui::Point(self.x + 140, self.y + 32);
        drawingCtx.drawStringCentredWrapped(*rt, point, 272, Colour::black, StringIds::change_keyboard_shortcut_desc, args);
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
