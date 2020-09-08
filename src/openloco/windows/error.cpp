#include "../Audio/Audio.h"
#include "../CompanyManager.h"
#include "../Graphics/Colour.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringManager.h"
#include "../Objects/CompetitorObject.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/objectmgr.h"
#include "../OpenLoco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::error
{
    static loco_global<uint8_t, 0x00508F09> _suppressErrorSound;
    static loco_global<ui::widget_t[1], 0x00508F1C> _widgets;
    static loco_global<char[512], 0x009C64B3> _byte_9C64B3;
    static loco_global<uint16_t, 0x009C66B3> _word_9C66B3;
    static loco_global<uint8_t, 0x009C68EC> _errorCompetitorId;
    static loco_global<int32_t, 0x112C876> gCurrentFontSpriteBase;
    static loco_global<int32_t, 0x0113E72C> _cursorX;
    static loco_global<int32_t, 0x0113E730> _cursorY;

    namespace common
    {
        static window_event_list events;

        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi);
        static void onPeriodicUpdate(ui::window* self);
        static void initEvents();
    }
    namespace error
    {
        enum widx
        {
            frame,
        };

        widget_t widgets[] = {
            makeWidget({ 0, 0 }, { 200, 42 }, widget_type::wt_3, 0),
            widgetEnd(),
        };
    }

    namespace errorCompetitor
    {
        enum widx
        {
            frame,
            innerFrame,
        };

        widget_t widgets[] = {
            makeWidget({ 0, 0 }, { 250, 70 }, widget_type::wt_3, 0),
            makeWidget({ 3, 3 }, { 64, 64 }, widget_type::wt_3, 1),
            widgetEnd(),
        };
    }

    static char* formatErrorString(string_id title, string_id message, FormatArguments args, char* buffer)
    {
        char* ptr = (char*)buffer;
        ptr[0] = control_codes::colour_black;
        ptr++;

        if (title != string_ids::null)
        {
            ptr = stringmgr::formatString(ptr, title, &args);
        }

        if (message != string_ids::null)
        {
            if (title != string_ids::null)
            {

                *ptr = control_codes::newline;
                ptr++;
            }
            stringmgr::formatString(ptr, message, &args);
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
            gCurrentFontSpriteBase = font::medium_bold;
            int16_t strWidth;
            {
                strWidth = gfx::getStringWidthNewLined(&_byte_9C64B3[0]);
            }

            strWidth = std::min<int16_t>(strWidth, 196);

            gCurrentFontSpriteBase = font::medium_bold;
            {
                uint16_t breakLineCount = 0;
                std::tie(strWidth, breakLineCount) = gfx::wrapString(&_byte_9C64B3[0], strWidth);
                _word_9C66B3 = breakLineCount;
            }

            uint16_t frameWidth = strWidth + 3;
            uint16_t frameHeight = (_word_9C66B3 + 1) * 10 + 4;
            uint16_t width = frameWidth;
            uint16_t height = frameHeight;

            if (_errorCompetitorId != 0xFF)
            {
                width = 250;
                height = 70;
            }

            int x, y;

            int maxY = ui::height() - height;
            y = _cursorY + 26; // Normally, we'd display the tooltip 26 lower
            if (y > maxY)
            {
                // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
                // so we'll subtract a bit more
                y -= height + 40;
            }
            y = std::clamp(y, 22, maxY);

            x = std::clamp(_cursorX - (width / 2), 0, ui::width() - width);

            gfx::ui_size_t windowSize = { width, height };

            auto error = WindowManager::createWindow(
                WindowType::error,
                gfx::point_t(x, y),
                windowSize,
                window_flags::stick_to_front | window_flags::transparent | window_flags::flag_7,
                &common::events);

            common::initEvents();

            if (_errorCompetitorId != 0xFF)
            {
                error->widgets = errorCompetitor::widgets;
            }
            else
            {
                error->widgets = error::widgets;
            }

            error->widgets[error::widx::frame].right = frameWidth;
            error->widgets[error::widx::frame].bottom = frameHeight;
            error->var_846 = 0;

            if (!(_suppressErrorSound & (1 << 0)))
            {
                int32_t pan = (error->width / 2) + error->x;
                audio::playSound(audio::sound_id::error, pan);
            }
        }
    }

    // 0x00431A8A
    void open(string_id title, string_id message)
    {
        _errorCompetitorId = 0xFF;

        createErrorWindow(title, message);
    }

    // 0x00431908
    void openWithCompetitor(string_id title, string_id message, uint8_t competitorId)
    {
        _errorCompetitorId = competitorId;

        createErrorWindow(title, message);
    }

    void registerHooks()
    {
        registerHook(
            0x00431A8A,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                ui::windows::error::open(regs.bx, regs.dx);
                return 0;
            });

        registerHook(
            0x00431908,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                ui::windows::error::openWithCompetitor(regs.bx, regs.dx, regs.al);
                return 0;
            });
    }

    namespace common
    {
        // 0x00431C05
        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi)
        {
            uint16_t x = self->x;
            uint16_t y = self->y;
            uint16_t width = self->width;
            uint16_t height = self->height;
            auto skin = objectmgr::get<interface_skin_object>()->colour_09;

            gfx::drawRect(dpi, x + 1, y + 1, width - 2, height - 2, 0x2000000 | 45);
            gfx::drawRect(dpi, x + 1, y + 1, width - 2, height - 2, 0x2000000 | (116 + skin));

            gfx::drawRect(dpi, x, y + 2, 1, height - 4, 0x2000000 | 46);
            gfx::drawRect(dpi, x + width - 1, y + 2, 1, height - 4, 0x2000000 | 46);
            gfx::drawRect(dpi, x + 2, y + height - 1, width - 4, 1, 0x2000000 | 46);
            gfx::drawRect(dpi, x + 2, y, width - 4, 1, 0x2000000 | 46);

            gfx::drawRect(dpi, x + 1, y + 1, 1, 1, 0x2000000 | 46);
            gfx::drawRect(dpi, x + width - 1 - 1, y + 1, 1, 1, 0x2000000 | 46);
            gfx::drawRect(dpi, x + 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);
            gfx::drawRect(dpi, x + width - 1 - 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);

            if (_errorCompetitorId == 0xFF)
            {
                gfx::drawStringCentredRaw(*dpi, ((width + 1) / 2) + x - 1, y + 1, _word_9C66B3, colour::black, &_byte_9C64B3[0]);
            }
            else
            {
                auto xPos = self->widgets[errorCompetitor::widx::innerFrame].left + self->x;
                auto yPos = self->widgets[errorCompetitor::widx::innerFrame].top + self->y;

                auto company = companymgr::get(_errorCompetitorId);
                auto companyObj = objectmgr::get<competitor_object>(company->id());

                auto imageId = companyObj->images[company->owner_emotion];
                imageId = gfx::recolour(imageId, company->mainColours.primary);
                imageId++;

                gfx::drawImage(dpi, xPos, yPos, imageId);

                if (company->jail_status != 0)
                {
                    gfx::drawImage(dpi, xPos, yPos, image_ids::owner_jailed);
                }

                gfx::drawStringCentredRaw(*dpi, self->x + 156, self->y + 20, _word_9C66B3, colour::black, &_byte_9C64B3[0]);
            }
        }

        // 0x00431E1B
        static void onPeriodicUpdate(ui::window* self)
        {
            self->var_846++;
            if (self->var_846 >= 7)
            {
                WindowManager::close(self);
            }
        }

        static void initEvents()
        {
            events.draw = common::draw;
            events.on_periodic_update = common::onPeriodicUpdate;
        }
    }
}
