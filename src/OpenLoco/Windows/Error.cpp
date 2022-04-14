#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringManager.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::Error
{
    static loco_global<uint8_t, 0x00508F09> _suppressErrorSound;
    static loco_global<char[512], 0x009C64B3> _byte_9C64B3;
    static loco_global<uint16_t, 0x009C66B3> _word_9C66B3;
    static loco_global<CompanyId, 0x009C68EC> _errorCompetitorId;
    static loco_global<int32_t, 0x112C876> gCurrentFontSpriteBase;
    static loco_global<int32_t, 0x0113E72C> _cursorX;
    static loco_global<int32_t, 0x0113E730> _cursorY;

    namespace Common
    {
        static WindowEventList events;

        static void draw(Ui::Window* self, Gfx::Context* context);
        static void onPeriodicUpdate(Ui::Window* self);
        static void initEvents();
    }
    namespace Error
    {
        enum widx
        {
            frame,
        };

        Widget widgets[] = {
            makeWidget({ 0, 0 }, { 200, 42 }, WidgetType::wt_3, WindowColour::primary),
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

        Widget widgets[] = {
            makeWidget({ 0, 0 }, { 250, 70 }, WidgetType::wt_3, WindowColour::primary),
            makeWidget({ 3, 3 }, { 64, 64 }, WidgetType::wt_3, WindowColour::secondary),
            widgetEnd(),
        };
    }

    static char* formatErrorString(string_id title, string_id message, FormatArguments args, char* buffer)
    {
        char* ptr = (char*)buffer;
        ptr[0] = ControlCodes::colour_black;
        ptr++;

        if (title != StringIds::null)
        {
            ptr = StringManager::formatString(ptr, title, &args);
        }

        if (message != StringIds::null)
        {
            if (title != StringIds::null)
            {

                *ptr = ControlCodes::newline;
                ptr++;
            }
            StringManager::formatString(ptr, message, &args);
        }

        return ptr;
    }

    static void createErrorWindow(string_id title, string_id message)
    {
        WindowManager::close(WindowType::error);

        char* buffer = _byte_9C64B3;

        auto args = FormatArguments();

        buffer = formatErrorString(title, message, args, buffer);

        if (buffer != &_byte_9C64B3[0])
        {
            gCurrentFontSpriteBase = Font::medium_bold;
            int16_t strWidth;
            {
                strWidth = Gfx::getStringWidthNewLined(&_byte_9C64B3[0]);
            }

            strWidth = std::min<int16_t>(strWidth, 196);

            gCurrentFontSpriteBase = Font::medium_bold;
            {
                uint16_t breakLineCount = 0;
                std::tie(strWidth, breakLineCount) = Gfx::wrapString(&_byte_9C64B3[0], strWidth);
                _word_9C66B3 = breakLineCount;
            }

            uint16_t frameWidth = strWidth + 3;
            uint16_t frameHeight = (_word_9C66B3 + 1) * 10 + 4;
            uint16_t width = frameWidth;
            uint16_t height = frameHeight;

            if (_errorCompetitorId != CompanyId::null)
            {
                width = 250;
                height = 70;
            }

            int x, y;

            int maxY = Ui::height() - height;
            y = _cursorY + 26; // Normally, we'd display the tooltip 26 lower
            if (y > maxY)
            {
                // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
                // so we'll subtract a bit more
                y -= height + 40;
            }
            y = std::clamp(y, 22, maxY);

            x = std::clamp(_cursorX - (width / 2), 0, Ui::width() - width);

            Ui::Size windowSize = { width, height };

            auto error = WindowManager::createWindow(
                WindowType::error,
                Ui::Point(x, y),
                windowSize,
                WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::flag_7,
                &Common::events);

            Common::initEvents();

            if (_errorCompetitorId != CompanyId::null)
            {
                error->widgets = ErrorCompetitor::widgets;
            }
            else
            {
                error->widgets = Error::widgets;
            }

            error->widgets[Error::widx::frame].right = frameWidth;
            error->widgets[Error::widx::frame].bottom = frameHeight;
            error->var_846 = 0;

            if (!(_suppressErrorSound & (1 << 0)))
            {
                int32_t pan = (error->width / 2) + error->x;
                Audio::playSound(Audio::SoundId::error, pan);
            }
        }
    }

    // 0x00431A8A
    void open(string_id title, string_id message)
    {
        _errorCompetitorId = CompanyId::null;

        createErrorWindow(title, message);
    }

    // 0x00431908
    void openWithCompetitor(string_id title, string_id message, CompanyId competitorId)
    {
        _errorCompetitorId = competitorId;

        createErrorWindow(title, message);
    }

    void registerHooks()
    {
        registerHook(
            0x00431A8A,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Ui::Windows::Error::open(regs.bx, regs.dx);
                return 0;
            });

        registerHook(
            0x00431908,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                Ui::Windows::Error::openWithCompetitor(regs.bx, regs.dx, CompanyId(regs.al));
                return 0;
            });
    }

    namespace Common
    {
        // 0x00431C05
        static void draw(Ui::Window* self, Gfx::Context* context)
        {
            uint16_t x = self->x;
            uint16_t y = self->y;
            uint16_t width = self->width;
            uint16_t height = self->height;
            auto skin = ObjectManager::get<InterfaceSkinObject>()->colour_09;

            Gfx::drawRect(*context, x + 1, y + 1, width - 2, height - 2, 0x2000000 | 45);
            Gfx::drawRect(*context, x + 1, y + 1, width - 2, height - 2, 0x2000000 | (116 + enumValue(skin)));

            Gfx::drawRect(*context, x, y + 2, 1, height - 4, 0x2000000 | 46);
            Gfx::drawRect(*context, x + width - 1, y + 2, 1, height - 4, 0x2000000 | 46);
            Gfx::drawRect(*context, x + 2, y + height - 1, width - 4, 1, 0x2000000 | 46);
            Gfx::drawRect(*context, x + 2, y, width - 4, 1, 0x2000000 | 46);

            Gfx::drawRect(*context, x + 1, y + 1, 1, 1, 0x2000000 | 46);
            Gfx::drawRect(*context, x + width - 1 - 1, y + 1, 1, 1, 0x2000000 | 46);
            Gfx::drawRect(*context, x + 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);
            Gfx::drawRect(*context, x + width - 1 - 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);

            if (_errorCompetitorId == CompanyId::null)
            {
                Gfx::drawStringCentredRaw(*context, ((width + 1) / 2) + x - 1, y + 1, _word_9C66B3, Colour::black, &_byte_9C64B3[0]);
            }
            else
            {
                auto xPos = self->widgets[ErrorCompetitor::widx::innerFrame].left + self->x;
                auto yPos = self->widgets[ErrorCompetitor::widx::innerFrame].top + self->y;

                auto company = CompanyManager::get(_errorCompetitorId);
                auto companyObj = ObjectManager::get<CompetitorObject>(company->competitorId);

                auto imageId = companyObj->images[company->ownerEmotion];
                imageId = Gfx::recolour(imageId, company->mainColours.primary);
                imageId++;

                Gfx::drawImage(context, xPos, yPos, imageId);

                if (company->jailStatus != 0)
                {
                    Gfx::drawImage(context, xPos, yPos, ImageIds::owner_jailed);
                }

                Gfx::drawStringCentredRaw(*context, self->x + 156, self->y + 20, _word_9C66B3, Colour::black, &_byte_9C64B3[0]);
            }
        }

        // 0x00431E1B
        static void onPeriodicUpdate(Ui::Window* self)
        {
            self->var_846++;
            if (self->var_846 >= 7)
            {
                WindowManager::close(self);
            }
        }

        static void initEvents()
        {
            events.draw = Common::draw;
            events.onPeriodicUpdate = Common::onPeriodicUpdate;
        }
    }
}
