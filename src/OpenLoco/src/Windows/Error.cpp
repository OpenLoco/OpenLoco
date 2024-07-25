#include "Audio/Audio.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Input.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"
#include "Objects/CompetitorObject.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::Error
{
    static loco_global<bool, 0x00508F09> _suppressErrorSound;

    static char _errorText[512];         // 0x009C64B3
    static uint16_t _linebreakCount;     // 0x009C66B3
    static CompanyId _errorCompetitorId; // 0x009C68EC

    static constexpr auto kMinWidth = 70;
    static constexpr auto kMaxWidth = 250;
    static constexpr auto kPadding = 4;

    namespace Common
    {
        static const WindowEventList& getEvents();
    }

    namespace Error
    {
        enum widx
        {
            frame,
        };

        static constexpr Widget widgets[] = {
            makeWidget({ 0, 0 }, { 200, 42 }, WidgetType::panel, WindowColour::primary),
            widgetEnd(),
        };
    }

    namespace ErrorCompetitor
    {
        enum widx
        {
            frame,
            innerFrame,
        };

        static constexpr Widget widgets[] = {
            makeWidget({ 0, 0 }, { 250, 70 }, WidgetType::panel, WindowColour::primary),
            makeWidget({ 3, 3 }, { 64, 64 }, WidgetType::wt_3, WindowColour::secondary),
            widgetEnd(),
        };
    }

    static char* formatErrorString(StringId title, StringId message, FormatArguments args, char* buffer)
    {
        char* ptr = buffer;
        ptr[0] = ControlCodes::Colour::white;
        ptr++;

        if (title != StringIds::null)
        {
            ptr = StringManager::formatString(ptr, title, args);
        }

        if (message != StringIds::null)
        {
            if (title != StringIds::null)
            {

                *ptr = ControlCodes::newline;
                ptr++;
            }
            StringManager::formatString(ptr, message, args);
        }

        return ptr;
    }

    static void createErrorWindow(StringId title, StringId message)
    {
        WindowManager::close(WindowType::error);

        char* buffer = _errorText;

        auto args = FormatArguments::common();

        buffer = formatErrorString(title, message, args, buffer);

        if (buffer != &_errorText[0])
        {
            auto& drawingCtx = Gfx::getDrawingEngine().getDrawingContext();
            auto tr = Gfx::TextRenderer(drawingCtx);

            // How wide is the error string?
            tr.setCurrentFont(Gfx::Font::medium_bold);
            uint16_t strWidth = tr.getStringWidthNewLined(&_errorText[0]);
            strWidth = std::clamp<uint16_t>(strWidth, kMinWidth, kMaxWidth);

            // How many linebreaks?
            {
                tr.setCurrentFont(Gfx::Font::medium_bold);
                uint16_t breakLineCount = 0;
                std::tie(strWidth, breakLineCount) = tr.wrapString(&_errorText[0], strWidth + kPadding);
                _linebreakCount = breakLineCount;
            }

            // Calculate frame size
            uint16_t width = strWidth + 2 * kPadding;
            uint16_t height = (_linebreakCount + 1) * 10 + 2 * kPadding;
            uint16_t frameWidth = width - 1;
            uint16_t frameHeight = height - 1;

            if (_errorCompetitorId != CompanyId::null)
            {
                width = kMaxWidth;
                height = 70;
            }

            // Position error message around the cursor
            auto mousePos = Input::getMouseLocation();
            Ui::Point windowPosition = mousePos + Ui::Point(-width / 2, 26);
            windowPosition.x = std::clamp<int16_t>(windowPosition.x, 0, Ui::width() - width - 40);
            windowPosition.y = std::clamp<int16_t>(windowPosition.y, 22, Ui::height() - height - 40);

            auto error = WindowManager::createWindow(
                WindowType::error,
                windowPosition,
                Ui::Size(width, height),
                WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::flag_7,
                Common::getEvents());

            if (_errorCompetitorId != CompanyId::null)
            {
                error->setWidgets(ErrorCompetitor::widgets);
            }
            else
            {
                error->setWidgets(Error::widgets);
            }

            error->setColour(WindowColour::primary, AdvancedColour(Colour::mutedDarkRed).translucent());
            error->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedDarkRed).translucent());

            error->widgets[Error::widx::frame].right = frameWidth;
            error->widgets[Error::widx::frame].bottom = frameHeight;
            error->var_846 = 0;

            if (!_suppressErrorSound)
            {
                int32_t pan = (error->width / 2) + error->x;
                Audio::playSound(Audio::SoundId::error, pan);
            }
        }
    }

    // 0x00431A8A
    void open(StringId title, StringId message)
    {
        _errorCompetitorId = CompanyId::null;

        createErrorWindow(title, message);
    }

    void openQuiet(StringId title, StringId message)
    {
        _errorCompetitorId = CompanyId::null;
        _suppressErrorSound = true;

        createErrorWindow(title, message);

        _suppressErrorSound = false;
    }

    // 0x00431908
    void openWithCompetitor(StringId title, StringId message, CompanyId competitorId)
    {
        _errorCompetitorId = competitorId;

        createErrorWindow(title, message);
    }

    void registerHooks()
    {
        registerHook(
            0x00431A8A,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Ui::Windows::Error::open(regs.bx, regs.dx);
                regs = backup;
                return 0;
            });

        registerHook(
            0x00431908,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                Ui::Windows::Error::openWithCompetitor(regs.bx, regs.dx, CompanyId(regs.al));
                regs = backup;
                return 0;
            });
    }

    namespace Common
    {
        // 0x00431C05
        static void draw(Ui::Window& self, Gfx::DrawingContext& drawingCtx)
        {
            self.draw(drawingCtx);

            auto tr = Gfx::TextRenderer(drawingCtx);
            auto colour = AdvancedColour(Colour::white).translucent(); //self.colours[0];

            if (_errorCompetitorId == CompanyId::null)
            {
                uint16_t xPos = self.x + self.width / 2;
                uint16_t yPos = self.y + kPadding;

                tr.drawStringCentredRaw(Point(xPos, yPos), _linebreakCount, colour, &_errorText[0]);
            }
            else
            {
                auto xPos = self.widgets[ErrorCompetitor::widx::innerFrame].left + self.x + kPadding;
                auto yPos = self.widgets[ErrorCompetitor::widx::innerFrame].top + self.y + kPadding;

                auto company = CompanyManager::get(_errorCompetitorId);
                auto companyObj = ObjectManager::get<CompetitorObject>(company->competitorId);

                auto imageId = companyObj->images[enumValue(company->ownerEmotion)];
                imageId = Gfx::recolour(imageId, company->mainColours.primary);
                imageId++;

                drawingCtx.drawImage(xPos, yPos, imageId);

                if (company->jailStatus != 0)
                {
                    drawingCtx.drawImage(xPos, yPos, ImageIds::owner_jailed);
                }

                auto point = Point(self.x + 156, self.y + 20);
                tr.drawStringCentredRaw(point, _linebreakCount, colour, &_errorText[0]);
            }
        }

        // 0x00431E1B
        static void onPeriodicUpdate(Ui::Window& self)
        {
            self.var_846++;
            if (self.var_846 >= 7)
            {
                WindowManager::close(&self);
            }
        }

        static constexpr WindowEventList kEvents = {
            .onPeriodicUpdate = onPeriodicUpdate,
            .draw = draw,
        };

        static const WindowEventList& getEvents()
        {
            return kEvents;
        }
    }
}
