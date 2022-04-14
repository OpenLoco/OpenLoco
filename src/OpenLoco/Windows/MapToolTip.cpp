#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/ArgsWrapper.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Ui.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::MapToolTip
{
    static loco_global<CompanyId, 0x0050A040> _mapTooltipOwner;
    static loco_global<uint16_t, 0x00523348> _mapTooltipTimeout;

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
        _mapTooltipTimeout++;
        auto cursor = Input::getMouseLocation();

        static Ui::Point tooltipLocation = {};
        if ((std::abs(tooltipLocation.x - cursor.x) > 5)
            || (std::abs(tooltipLocation.y - cursor.y) > 5)
            || Input::hasFlag(Input::Flags::flag5))
        {
            _mapTooltipTimeout = 0;
        }

        tooltipLocation = cursor;
        auto args = FormatArguments::mapToolTip();
        StringManager::ArgsWrapper argsWrap(&args);
        auto firstArg = argsWrap.pop<string_id>();
        if (_mapTooltipTimeout < 25 || firstArg == StringIds::null || Input::hasFlag(Input::Flags::flag5) || Input::hasKeyModifier(Input::KeyModifier::control) || Input::hasKeyModifier(Input::KeyModifier::shift) || WindowManager::find(WindowType::error) != nullptr)
        {
            WindowManager::close(WindowType::mapTooltip);
            return;
        }

        auto height = 55;
        auto maxY = Ui::height() - height;
        int16_t y = cursor.y + 15; // Normally, we'd display the tooltip 15 lower
        if (y > maxY)
        {
            // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
            // so we'll subtract a bit more
            y -= height + 19;
        }

        auto width = 240;
        int16_t x = width <= Ui::width() ? std::clamp(cursor.x - (width / 2), 0, Ui::width() - width) : 0;

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
            window = WindowManager::createWindow(WindowType::mapTooltip, Ui::Point(x, y), Ui::Size(width, height), WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground, &events);
            window->widgets = _widgets;
            auto* skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->colour_06);
            window->setColour(WindowColour::tertiary, skin->colour_07);
        }
    }

    void setOwner(CompanyId company)
    {
        _mapTooltipOwner = company;
    }

    uint16_t getTooltipTimeout()
    {
        return _mapTooltipTimeout;
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
        auto args = FormatArguments::mapToolTip();
        StringManager::ArgsWrapper argsWrap(&args);
        auto firstArg = argsWrap.pop<string_id>();
        if (firstArg == StringIds::null)
        {
            return;
        }

        if (_mapTooltipOwner == CompanyId::null || _mapTooltipOwner == CompanyManager::getControllingId())
        {
            Ui::Point origin(self->x + self->width / 2, self->y + self->height / 2 - 5);
            Gfx::drawStringCentredWrapped(*context, origin, self->width, Colour::black, StringIds::outlined_wcolour2_stringid, &args);
        }
        else
        {
            Ui::Point origin(self->x + self->width / 2 + 13, self->y + self->height / 2 - 5);
            auto width = Gfx::drawStringCentredWrapped(*context, origin, self->width - 28, Colour::black, StringIds::outlined_wcolour2_stringid, &args);

            auto left = self->width / 2 + self->x + 13 - width / 2 - 28;
            auto top = self->height / 2 - 13 + self->y;
            auto right = left + 25;
            auto bottom = top + 25;

            Gfx::fillRect(*context, left, top, right, bottom, enumValue(Colour::darkGreen));

            auto* company = CompanyManager::get(_mapTooltipOwner);
            auto* competitor = ObjectManager::get<CompetitorObject>(company->competitorId);
            auto imageId = Gfx::recolour(competitor->images[company->ownerEmotion], company->mainColours.primary);

            Gfx::drawImage(context, left + 1, top + 1, imageId);
        }
    }

    static void initEvents()
    {
        events.onUpdate = update;
        events.draw = draw;
    }
}
