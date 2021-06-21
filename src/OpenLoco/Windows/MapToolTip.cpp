#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/InterfaceSkinObject.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::MapToolTip
{
    static loco_global<string_id[20], 0x0050A018> _mapTooltipFormatArguments;
    static loco_global<CompanyId_t, 0x0050A040> _mapTooltipOwner;
    static loco_global<uint16_t, 0x00523348> _mapTooltip523348;

    static WindowEventList events;

    enum widx
    {
        text
    };

    // 0x005234F4
    Widget _widgets[] = {
        makeWidget({ 0, 0 }, { 200, 30 }, WidgetType::wt_3, WindowColour::primary),
        widgetEnd(),
    };

    static void initEvents();

    // 0x004CEEA7
    void open()
    {
        _mapTooltip523348++;
        auto cursor = Input::getMouseLocation();

        static Gfx::point_t tooltipLocation = {};
        if ((std::abs(tooltipLocation.x - cursor.x) > 5)
            || (std::abs(tooltipLocation.y - cursor.y) > 5)
            || Input::hasFlag(Input::Flags::flag5))
        {
            _mapTooltip523348 = 0;
        }

        tooltipLocation = cursor;

        if (_mapTooltip523348 < 25 || _mapTooltipFormatArguments[0] == StringIds::null || Input::hasFlag(Input::Flags::flag5) || Input::hasKeyModifier(Input::KeyModifier::control) || Input::hasKeyModifier(Input::KeyModifier::shift) || WindowManager::find(WindowType::error) != nullptr)
        {
            WindowManager::close(WindowType::mapTooltip);
            return;
        }

        auto height = 55;
        auto maxY = Ui::height() - height;
        auto y = cursor.y + 15; // Normally, we'd display the tooltip 15 lower
        if (y > maxY)
            // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
            // so we'll subtract a bit more
            y -= height + 19;

        auto width = 240;
        auto x = width <= Ui::width() ? std::clamp(cursor.x - (width / 2), 0, Ui::width() - width) : 0;

        auto* window = WindowManager::find(WindowType::mapTooltip);
        if (window != nullptr)
        {
            window->invalidate();
            window->x = x;
            window->y = y;
            window->width = width;
            window->height = height;
        }
        else
        {
            initEvents();
            auto* window = WindowManager::createWindow(WindowType::mapTooltip, { x, y }, { width, height }, WindowFlags::stick_to_front | WindowFlags::transparent | WindowFlags::no_background, &events);
            window->widgets = _widgets;
            auto* skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->colour_06);
            window->setColour(WindowColour::tertiary, skin->colour_07);
        }
    }

    void setOwner(CompanyId_t company)
    {
        _mapTooltipOwner = company;
    }

    void reset()
    {
        setOwner(CompanyId::null);
        FormatArguments::mapToolTip(StringIds::null);
    }

    // 0x004CF13B
    static void update(Window* self)
    {
        self->invalidate();
    }

    // 0x004CF010
    static void draw(Window* self, Gfx::Context* context)
    {
    }

    static void initEvents()
    {
        events.on_update = update;
        events.draw = draw;
    }
}
