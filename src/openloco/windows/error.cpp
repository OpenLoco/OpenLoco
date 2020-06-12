#include "../audio/audio.h"
#include "../companymgr.h"
#include "../graphics/colours.h"
#include "../graphics/image_ids.h"
#include "../interop/interop.hpp"
#include "../localisation/FormatArguments.hpp"
#include "../localisation/stringmgr.h"
#include "../objects/competitor_object.h"
#include "../objects/interface_skin_object.h"
#include "../objects/objectmgr.h"
#include "../openloco.h"
#include "../ui/WindowManager.h"

using namespace openloco::interop;

namespace openloco::ui::windows::error
{
    static loco_global<uint8_t, 0x00508F09> _suppressErrorSound;
    static loco_global<ui::widget_t[1], 0x00508F1C> _widgets;
    static loco_global<char[512], 0x009C64B4> _byte_9C64B3;
    static loco_global<uint16_t, 0x009C66B3> _word_9C66B3;
    static loco_global<uint8_t, 0x009C68EC> _byte_9C68EC;
    static loco_global<int32_t, 0x112C876> gCurrentFontSpriteBase;
    static loco_global<int32_t, 0x0113E72C> _cursorX;
    static loco_global<int32_t, 0x0113E730> _cursorY;

    namespace common
    {
        static window_event_list events;

        static void draw(ui::window* self, gfx::drawpixelinfo_t* dpi);
        static void on_periodic_update(ui::window* self);
        static void init_events();
    }
    namespace error
    {
        enum widx
        {
            frame,
        };

        widget_t widgets[] = {
            make_widget({ 0, 0 }, { 200, 42 }, widget_type::wt_3, 0),
            widget_end(),
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
            make_widget({ 0, 0 }, { 250, 70 }, widget_type::wt_3, 0),
            make_widget({ 3, 3 }, { 64, 64 }, widget_type::wt_3, 1),
            widget_end(),
        };
    }

    static char* formatErrorString(string_id title, string_id message, char* buffer)
    {
        char* ptr = (char*)buffer;
        ptr[0] = -112;
        ptr++;

        if (title != string_ids::null)
        {
            auto args = FormatArguments();

            ptr = stringmgr::format_string(ptr, title, &args);
        }

        if (message != string_ids::null)
        {
            if (title != string_ids::null)
            {

                *ptr = control_codes::newline;
                ptr++;
            }

            auto args = FormatArguments();

            stringmgr::format_string(ptr, message, &args);
        }
        return ptr;
    }

    // 0x00431A8A
    void open(string_id title, string_id message)
    {
        _byte_9C68EC = 0xFF;

        WindowManager::close(WindowType::error);

        char* buffer = _byte_9C64B3;

        buffer = formatErrorString(title, message, buffer);

        if (buffer[0] != _byte_9C64B3[0])
        {
            gCurrentFontSpriteBase = font::medium_bold;
            int16_t strWidth;
            {
                strWidth = gfx::get_string_width_new_lined(&_byte_9C64B3[0]);
            }

            strWidth = std::min<int16_t>(strWidth, 196);

            gCurrentFontSpriteBase = font::medium_bold;
            {
                uint16_t breakLineCount = 0;
                strWidth = gfx::wrap_string(&_byte_9C64B3[0], strWidth, &breakLineCount);
                _word_9C66B3 = breakLineCount;
            }

            int width = strWidth + 3;
            int height = (_word_9C66B3 + 1) * 10 + 4;

            int x, y;

            int maxY = ui::height() - height;
            y = _cursorY + 26; // Normally, we'd display the tooltip 26 lower
            if (y > maxY)
                // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
                // so we'll subtract a bit more
                y -= height + 40;
            y = std::clamp(y, 22, maxY);

            x = std::clamp(_cursorX - (width / 2), 0, ui::width() - width);

            auto error = WindowManager::createWindow(
                WindowType::error,
                gfx::point_t(x, y),
                gfx::ui_size_t(width, height),
                window_flags::stick_to_front | window_flags::transparent | window_flags::flag_7,
                &common::events);

            common::init_events();

            error->widgets = error::widgets;
            error->widgets[error::widx::frame].right = width;
            error->widgets[error::widx::frame].bottom = height;
            error->var_846 = 0;

            if (!(_suppressErrorSound & (1 << 0)))
            {
                int32_t pan = (error->width / 2) + error->x;
                audio::play_sound(audio::sound_id::error, pan);
            }
        }
    }

    // 0x00431908
    void open_with_competitor(string_id title, string_id message, uint8_t competitorId)
    {
        _byte_9C68EC = competitorId;

        WindowManager::close(WindowType::error);

        char* buffer = _byte_9C64B3;

        buffer = formatErrorString(title, message, buffer);

        if (buffer[0] != _byte_9C64B3[0])
        {
            gCurrentFontSpriteBase = font::medium_bold;
            int16_t strWidth;
            {
                strWidth = gfx::get_string_width_new_lined(&_byte_9C64B3[0]);
            }

            strWidth = std::min<int16_t>(strWidth, 178);

            gCurrentFontSpriteBase = font::medium_bold;
            {
                uint16_t breakLineCount = 0;
                strWidth = gfx::wrap_string(&_byte_9C64B3[0], strWidth, &breakLineCount);
                _word_9C66B3 = breakLineCount;
            }

            int width = strWidth + 3;
            int height = (_word_9C66B3 + 1) * 10 + 4;

            int x, y;

            int maxY = ui::height() - 70;
            y = _cursorY + 26; // Normally, we'd display the tooltip 26 lower
            if (y > maxY)
                // If y is too large, the tooltip could be forced below the cursor if we'd just clamped y,
                // so we'll subtract a bit more
                y -= 70 + 40;
            y = std::clamp(y, 22, maxY);

            x = std::clamp(_cursorX - (250 / 2), 0, ui::width() - 250);

            auto error = WindowManager::createWindow(
                WindowType::error,
                gfx::point_t(x, y),
                gfx::ui_size_t(250, 70),
                window_flags::stick_to_front | window_flags::transparent | window_flags::flag_7,
                &common::events);

            common::init_events();

            error->widgets = errorCompetitor::widgets;
            error->widgets[error::widx::frame].right = width;
            error->widgets[error::widx::frame].bottom = height;
            error->var_846 = 0;

            if (!(_suppressErrorSound & (1 << 0)))
            {
                int32_t pan = (error->width / 2) + error->x;
                audio::play_sound(audio::sound_id::error, pan);
            }
        }
    }

    void registerHooks()
    {
        register_hook(
            0x00431A8A,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                ui::windows::error::open(regs.bx, regs.dx);
                return 0;
            });

        register_hook(
            0x00431908,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                ui::windows::error::open_with_competitor(regs.bx, regs.dx, regs.al);
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

            gfx::draw_rect(dpi, x + 1, y + 1, width - 2, height - 2, 0x2000000 | 45);
            gfx::draw_rect(dpi, x + 1, y + 1, width - 2, height - 2, 0x2000000 | (116 + objectmgr::get<interface_skin_object>()->colour_09));

            gfx::draw_rect(dpi, x, y + 2, 1, height - 4, 0x2000000 | 46);
            gfx::draw_rect(dpi, x + width - 1, y + 2, 1, height - 4, 0x2000000 | 46);
            gfx::draw_rect(dpi, x + 2, y + height - 1, width - 4, 1, 0x2000000 | 46);
            gfx::draw_rect(dpi, x + 2, y, width - 4, 1, 0x2000000 | 46);

            gfx::draw_rect(dpi, x + 1, y + 1, 1, 1, 0x2000000 | 46);
            gfx::draw_rect(dpi, x + width - 1 - 1, y + 1, 1, 1, 0x2000000 | 46);
            gfx::draw_rect(dpi, x + 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);
            gfx::draw_rect(dpi, x + width - 1 - 1, y + height - 1 - 1, 1, 1, 0x2000000 | 46);

            if (_byte_9C68EC == 0xFF)
            {
                gfx::draw_string_centred_raw(*dpi, ((width + 1) / 2) + x - 1, y + 1, _word_9C66B3, colour::black, &_byte_9C64B3[0]);
            }
            else
            {
                auto xPos = self->widgets[errorCompetitor::widx::innerFrame].left + self->x;
                auto yPos = self->widgets[errorCompetitor::widx::innerFrame].top + self->y;

                auto company = companymgr::get(_byte_9C68EC);
                auto companyObj = objectmgr::get<competitor_object>(company->id());

                auto imageId = companyObj->images[company->owner_emotion];
                imageId = gfx::recolour(imageId, company->mainColours.primary);
                imageId++;

                gfx::draw_image(dpi, xPos, yPos, imageId);

                if (company->jail_status != 0)
                {
                    gfx::draw_image(dpi, xPos, yPos, image_ids::owner_jailed);
                }

                gfx::draw_string_centred_raw(*dpi, self->x + 156, self->y + 20, _word_9C66B3, colour::black, &_byte_9C64B3[0]);
            }
        }

        // 0x00431E1B
        static void on_periodic_update(ui::window* self)
        {
            self->var_846++;
            if (self->var_846 >= 7)
            {
                WindowManager::close(self);
            }
        }

        static void init_events()
        {
            events.draw = common::draw;
            events.on_periodic_update = common::on_periodic_update;
        }
    }
}
