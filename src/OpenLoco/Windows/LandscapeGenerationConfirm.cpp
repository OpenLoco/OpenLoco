#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/StringIds.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../Scenario.h"
#include "../Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::LandscapeGenerationConfirm
{
    static const Gfx::ui_size_t window_size = { 280, 92 };

    enum widx
    {
        panel = 0,
        caption = 1,
        close_button = 2,
        button_ok = 3,
        button_cancel = 4,
    };

    static widget_t widgets[] = {
        makeWidget({ 0, 0 }, { 280, 92 }, widget_type::panel, 0),
        makeWidget({ 1, 1 }, { 278, 13 }, widget_type::caption_22, 0),
        makeWidget({ 267, 2 }, { 11, 11 }, widget_type::wt_11, 0, StringIds::close_window_cross, StringIds::tooltip_close_window),
        makeWidget({ 20, 77 }, { 100, 12 }, widget_type::wt_11, 0, StringIds::label_ok),
        makeWidget({ 160, 77 }, { 100, 12 }, widget_type::wt_11, 0, StringIds::label_button_cancel),
        widgetEnd()
    };

    static window_event_list events;

    // 0x004C18A5
    static void draw(window* window, Gfx::drawpixelinfo_t* dpi)
    {
        window->draw(dpi);

        static loco_global<string_id, 0x0112C826> commonFormatArgs;
        string_id prompt = window->var_846 == 0 ? StringIds::prompt_confirm_generate_landscape : StringIds::prompt_confirm_random_landscape;
        *commonFormatArgs = prompt;

        auto origin = Gfx::point_t(window->x + (window->width / 2), window->y + 41);
        Gfx::drawStringCentredWrapped(dpi, &origin, window->width, Colour::black, StringIds::wcolour2_stringid, (const char*)&*commonFormatArgs);
    }

    // 0x004C18E4
    static void onMouseUp(window* window, widget_index widgetIndex)
    {
        switch (widgetIndex)
        {
            case widx::close_button:
            case widx::button_cancel:
                WindowManager::close(window);
                break;

            case widx::button_ok:
                uint32_t status = window->var_846;
                WindowManager::close(window);

                if (status == 0)
                    Scenario::generateLandscape();
                else
                    Scenario::eraseLandscape();
                break;
        }
    }

    static void init_events()
    {
        events.draw = draw;
        events.on_mouse_up = onMouseUp;
    }

    // 0x004C180C
    window* open(int32_t prompt_type)
    {
        auto window = WindowManager::bringToFront(WindowType::landscapeGenerationConfirm, 0);
        if (window == nullptr)
        {
            window = WindowManager::createWindowCentred(WindowType::landscapeGenerationConfirm, window_size, 0, &events);
            window->widgets = widgets;
            window->setVisible(widx::close_button, widx::button_ok, widx::button_cancel);
            window->initScrollWidgets();
            window->colours[0] = Colour::translucent(Colour::salmon_pink);
            window->colours[1] = Colour::translucent(Colour::salmon_pink);
            window->flags |= WindowFlags::transparent;

            // TODO(avgeffen): only needs to be called once.
            init_events();
        }

        window->var_846 = prompt_type;
        if (prompt_type == 0)
            window->widgets[widx::caption].text = StringIds::title_generate_new_landscape;
        else
            window->widgets[widx::caption].text = StringIds::title_random_landscape_option;

        return window;
    }
}
