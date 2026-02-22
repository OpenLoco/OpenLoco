#include "Graphics/Colour.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Objects/CompetitorObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "Ui.h"
#include "Ui/Widget.h"
#include "Ui/Widgets/Wt3Widget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"

namespace OpenLoco::Ui::Windows::MapToolTip
{
    static CompanyId _mapTooltipOwner;  // 0x0050A040
    static uint16_t _mapTooltipTimeout; // 0x00523348

    enum widx
    {
        text
    };

    // 0x005234F4
    static constexpr auto _widgets = makeWidgets(
        Widgets::Wt3Widget({ 0, 0 }, { 200, 30 }, WindowColour::primary)

    );

    static const WindowEventList& getEvents();

    // 0x004CEEA7
    void open()
    {
        _mapTooltipTimeout++;
        auto cursor = Input::getMouseLocation();

        static Ui::Point tooltipLocation = {};
        if ((std::abs(tooltipLocation.x - cursor.x) > 5)
            || (std::abs(tooltipLocation.y - cursor.y) > 5)
            || Input::hasFlag(Input::Flags::rightMousePressed))
        {
            _mapTooltipTimeout = 0;
        }

        tooltipLocation = cursor;
        auto args = FormatArguments::mapToolTip();
        FormatArgumentsView argsWrap(args);
        auto firstArg = argsWrap.pop<StringId>();
        if (_mapTooltipTimeout < 25 || firstArg == StringIds::null || Input::hasFlag(Input::Flags::rightMousePressed) || Input::hasKeyModifier(Input::KeyModifier::control) || Input::hasKeyModifier(Input::KeyModifier::shift) || WindowManager::find(WindowType::error) != nullptr)
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
            window = WindowManager::createWindow(WindowType::mapTooltip, { x, y }, { width, height }, WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground, getEvents());
            window->setWidgets(_widgets);
            auto* skin = ObjectManager::get<InterfaceSkinObject>();
            window->setColour(WindowColour::secondary, skin->mapTooltipObjectColour);
            window->setColour(WindowColour::tertiary, skin->mapTooltipCargoColour);
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
    static void update(Window& self)
    {
        self.invalidate();
    }

    // 0x004CF010
    static void draw(Window& self, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        auto args = FormatArguments::mapToolTip();
        FormatArgumentsView argsWrap(args);
        auto firstArg = argsWrap.pop<StringId>();
        if (firstArg == StringIds::null)
        {
            return;
        }

        if (_mapTooltipOwner == CompanyId::null || _mapTooltipOwner == CompanyManager::getControllingId())
        {
            Ui::Point origin(self.x + self.width / 2, self.y + self.height / 2 - 5);
            tr.drawStringCentredWrapped(origin, self.width, Colour::black, StringIds::outlined_wcolour2_stringid, args);
        }
        else
        {
            Ui::Point origin(self.x + self.width / 2 + 13, self.y + self.height / 2 - 5);
            auto basePoint = tr.drawStringCentredWrapped(origin, self.width - 28, Colour::black, StringIds::outlined_wcolour2_stringid, args);

            auto left = basePoint.x - 28;
            auto top = self.y + self.height / 2 - 13;
            auto right = left + 25;
            auto bottom = top + 25;

            drawingCtx.fillRect(left, top, right, bottom, PaletteIndex::black0, Gfx::RectFlags::none);

            auto* company = CompanyManager::get(_mapTooltipOwner);
            auto* competitor = ObjectManager::get<CompetitorObject>(company->competitorId);
            auto imageId = Gfx::recolour(competitor->images[enumValue(company->ownerEmotion)], company->mainColours.primary);

            drawingCtx.drawImage(left + 1, top + 1, imageId);
        }
    }

    static constexpr WindowEventList kEvents = {
        .onUpdate = update,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
