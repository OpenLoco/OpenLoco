#include "Config.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/TextRenderer.h"
#include "Input/ShortcutFormatter.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/ButtonWidget.h"
#include "Ui/Widgets/CaptionWidget.h"
#include "Ui/Widgets/FrameWidget.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/PanelWidget.h"
#include "Ui/Widgets/ScrollViewWidget.h"
#include "Ui/WindowManager.h"
#include <OpenLoco/Engine/Input/ShortcutManager.h>

using namespace OpenLoco::Input;

namespace OpenLoco::Ui::Windows::KeyboardShortcuts
{
    static constexpr int kRowHeight = 10; // CJK: 13

    static constexpr auto _widgets = makeWidgets(
        Widgets::Frame({ 0, 0 }, { 360, 238 }, WindowColour::primary),
        Widgets::Caption({ 1, 1 }, { 358, 13 }, Widgets::Caption::Style::whiteText, WindowColour::primary, StringIds::keyboard_shortcuts),
        Widgets::ImageButton({ 345, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
        Widgets::Panel({ 0, 15 }, { 360, 223 }, WindowColour::secondary),
        Widgets::ScrollView({ 4, 19 }, { 352, 202 }, WindowColour::secondary, Scrollbars::vertical, StringIds::keyboard_shortcut_list_tip),
        Widgets::Button({ 4, 223 }, { 150, 12 }, WindowColour::secondary, StringIds::reset_keys, StringIds::reset_keys_tip)

    );

    namespace Widx
    {
        enum
        {
            frame,
            caption,
            close_button,
            panel,
            list,
            reset_keys_btn,
        };
    }

    static void resetShortcuts(Window* self);
    static const WindowEventList& getEvents();

    // 0x004BE6C7
    Window* open()
    {
        Window* window;

        window = WindowManager::bringToFront(WindowType::keyboardShortcuts, 0);
        if (window != nullptr)
        {
            return window;
        }

        // 0x004BF833 (create_options_window)
        window = WindowManager::createWindowCentred(WindowType::keyboardShortcuts, { 360, 238 }, WindowFlags::none, getEvents());

        window->setWidgets(_widgets);
        window->initScrollWidgets();

        auto skin = ObjectManager::get<InterfaceSkinObject>();
        window->setColour(WindowColour::primary, skin->windowTitlebarColour);
        window->setColour(WindowColour::secondary, skin->windowOptionsColour);

        window->rowCount = static_cast<uint16_t>(ShortcutManager::getList().size());
        window->rowHover = -1;

        return window;
    }

    // 0x004BE726
    static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
    {
        // Draw widgets.
        self.draw(drawingCtx);
    }

    // 0x004BE72C
    static void drawScroll(Ui::Window& self, Gfx::DrawingContext& drawingCtx, [[maybe_unused]] const uint32_t scrollIndex)
    {
        const auto& rt = drawingCtx.currentRenderTarget();
        auto tr = Gfx::TextRenderer(drawingCtx);

        auto colour = self.getColour(WindowColour::secondary).c();
        auto shade = Colours::getShade(colour, 4);
        drawingCtx.clearSingle(shade);

        const auto& shortcutDefs = ShortcutManager::getList();
        const auto& shortcuts = Config::get().shortcuts;
        auto yPos = 0;
        for (auto i = 0; i < self.rowCount; i++)
        {
            if (yPos + kRowHeight < rt.y)
            {
                yPos += kRowHeight;
                continue;
            }
            else if (yPos > rt.y + rt.height)
            {
                break;
            }

            StringId format = StringIds::black_stringid;
            if (i == self.rowHover)
            {
                drawingCtx.drawRect(0, yPos, 800, kRowHeight, enumValue(ExtColour::unk30), Gfx::RectFlags::transparent);
                format = StringIds::wcolour2_stringid;
            }

            char buffer[ShortcutFormatter::kShortcutBufferSize]{};

            const auto& def = shortcutDefs[i];
            auto& shortcut = shortcuts.at(def.id);
            const auto binding = ShortcutFormatter::getBinding(shortcut, buffer, std::size(buffer));

            FormatArguments formatter{};
            formatter.push(StringIds::keyboard_shortcut_list_format);
            formatter.push(ShortcutManager::getName(static_cast<Shortcut>(i)));
            formatter.push(binding.modifierStringId);
            formatter.push(binding.keyStringId);
            formatter.push(binding.keyString);

            auto point = Point(0, yPos - 1);
            tr.drawStringLeft(point, Colour::black, format, formatter);
            yPos += kRowHeight;
        }
    }

    // 0x004BE821
    static void onMouseUp(Window& self, WidgetIndex_t widgetIndex, [[maybe_unused]] const WidgetId id)
    {
        switch (widgetIndex)
        {
            case Widx::close_button:
                WindowManager::close(&self);
                return;

            case Widx::reset_keys_btn:
                resetShortcuts(&self);
                return;
        }
    }

    // 0x004BE832
    static void resetShortcuts(Window* self)
    {
        Config::resetShortcuts();
        self->invalidate();
    }

    // 0x004BE844
    static std::optional<FormatArguments> tooltip(Window&, WidgetIndex_t, [[maybe_unused]] const WidgetId id)
    {
        FormatArguments args{};
        args.push(StringIds::tooltip_scroll_list);
        return args;
    }

    // 0x004BE84E
    static void getScrollSize(Ui::Window& self, [[maybe_unused]] uint32_t scrollIndex, [[maybe_unused]] int32_t& scrollWidth, int32_t& scrollHeight)
    {
        scrollHeight = self.rowCount * kRowHeight;
    }

    // 0x004BE853
    static void onScrollMouseOver(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        auto row = y / kRowHeight;

        if (row >= self.rowCount)
        {
            return;
        }

        if (row != self.rowHover)
        {
            self.rowHover = row;
            self.invalidate();
        }
    }

    // 0x004BE87B
    static void onScrollMouseDown(Ui::Window& self, [[maybe_unused]] int16_t x, int16_t y, [[maybe_unused]] uint8_t scroll_index)
    {
        auto row = y / kRowHeight;

        if (row >= self.rowCount)
        {
            return;
        }

        EditKeyboardShortcut::open(row);
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .getScrollSize = getScrollSize,
        .scrollMouseDown = onScrollMouseDown,
        .scrollMouseOver = onScrollMouseOver,
        .tooltip = tooltip,
        .draw = draw,
        .drawScroll = drawScroll,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
