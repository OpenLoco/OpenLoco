#include "../Config.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Input/Shortcut.h"
#include "../Input/ShortcutManager.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;
using namespace OpenLoco::Input;

namespace OpenLoco::Ui::Windows::EditKeyboardShortcut
{
    constexpr Ui::Size windowSize = { 280, 72 };

    static WindowEventList events;
    static loco_global<uint8_t, 0x011364A4> _editingShortcutIndex;

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, windowSize, WidgetType::frame, WindowColour::primary, 0xFFFFFFFF),                                                          // 0,
        makeWidget({ 1, 1 }, { windowSize.width - 2, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::change_keyboard_shortcut),          // 1,
        makeWidget({ 265, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), // 2,
        makeWidget({ 0, 15 }, { windowSize.width, 57 }, WidgetType::panel, WindowColour::secondary, 0xFFFFFFFF),                                         // 3,
        widgetEnd(),
    };

    static void initEvents();

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

        // TODO: only needs to be called once
        initEvents();

        auto window = WindowManager::createWindow(WindowType::editKeyboardShortcut, windowSize, 0, &events);

        window->widgets = _widgets;
        window->enabledWidgets = 1 << Widx::close;
        window->initScrollWidgets();

        const auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->colour_0B);
        window->setColour(WindowColour::secondary, skin->colour_10);

        return window;
    }

    // 0x004BE8DF
    static void draw(Ui::Window* const self, Gfx::Context* const ctx)
    {
        self->draw(ctx);

        FormatArguments args{};
        args.push(ShortcutManager::getName(static_cast<Shortcut>(*_editingShortcutIndex)));
        auto point = Ui::Point(self->x + 140, self->y + 32);
        Gfx::drawStringCentredWrapped(*ctx, point, 272, Colour::black, StringIds::change_keyboard_shortcut_desc, &args);
    }

    // 0x004BE821
    static void onMouseUp(Window* const self, const WidgetIndex_t widgetIndex)
    {
        switch (widgetIndex)
        {
            case Widx::close:
                WindowManager::close(self);
                return;
        }
    }

    static void initEvents()
    {
        events.onMouseUp = onMouseUp;
        events.draw = draw;
    }
}
