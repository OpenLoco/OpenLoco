#include "../GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Input.h"
#include "../Interop/Interop.hpp"
#include "../Intro.h"
#include "../Localisation/StringIds.h"
#include "../Map/Tile.h"
#include "../MultiPlayer.h"
#include "../Objects/InterfaceSkinObject.h"
#include "../Objects/ObjectManager.h"
#include "../OpenLoco.h"
#include "../Tutorial.h"
#include "../Ui.h"
#include "../Ui/Dropdown.h"
#include "../Ui/WindowManager.h"
#include "../ViewportManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::ui::windows
{
    static const uint8_t btn_main_size = 74;
    static const uint8_t btn_sub_height = 18;
    static const uint16_t ww = btn_main_size * 4;
    static const uint16_t wh = btn_main_size + btn_sub_height;

    static const std::vector<uint32_t> globe_spin = {
        ImageIds::title_menu_globe_spin_0,
        ImageIds::title_menu_globe_spin_1,
        ImageIds::title_menu_globe_spin_2,
        ImageIds::title_menu_globe_spin_3,
        ImageIds::title_menu_globe_spin_4,
        ImageIds::title_menu_globe_spin_5,
        ImageIds::title_menu_globe_spin_6,
        ImageIds::title_menu_globe_spin_7,
        ImageIds::title_menu_globe_spin_8,
        ImageIds::title_menu_globe_spin_9,
        ImageIds::title_menu_globe_spin_10,
        ImageIds::title_menu_globe_spin_11,
        ImageIds::title_menu_globe_spin_12,
        ImageIds::title_menu_globe_spin_13,
        ImageIds::title_menu_globe_spin_14,
        ImageIds::title_menu_globe_spin_15,
        ImageIds::title_menu_globe_spin_16,
        ImageIds::title_menu_globe_spin_17,
        ImageIds::title_menu_globe_spin_18,
        ImageIds::title_menu_globe_spin_19,
        ImageIds::title_menu_globe_spin_20,
        ImageIds::title_menu_globe_spin_21,
        ImageIds::title_menu_globe_spin_22,
        ImageIds::title_menu_globe_spin_23,
        ImageIds::title_menu_globe_spin_24,
        ImageIds::title_menu_globe_spin_25,
        ImageIds::title_menu_globe_spin_26,
        ImageIds::title_menu_globe_spin_27,
        ImageIds::title_menu_globe_spin_28,
        ImageIds::title_menu_globe_spin_29,
        ImageIds::title_menu_globe_spin_30,
        ImageIds::title_menu_globe_spin_31,
    };

    static const std::vector<uint32_t> globe_construct = {
        ImageIds::title_menu_globe_construct_0,
        ImageIds::title_menu_globe_construct_1,
        ImageIds::title_menu_globe_construct_2,
        ImageIds::title_menu_globe_construct_3,
        ImageIds::title_menu_globe_construct_4,
        ImageIds::title_menu_globe_construct_5,
        ImageIds::title_menu_globe_construct_6,
        ImageIds::title_menu_globe_construct_7,
        ImageIds::title_menu_globe_construct_8,
        ImageIds::title_menu_globe_construct_9,
        ImageIds::title_menu_globe_construct_10,
        ImageIds::title_menu_globe_construct_11,
        ImageIds::title_menu_globe_construct_12,
        ImageIds::title_menu_globe_construct_13,
        ImageIds::title_menu_globe_construct_14,
        ImageIds::title_menu_globe_construct_15,
        ImageIds::title_menu_globe_construct_16,
        ImageIds::title_menu_globe_construct_17,
        ImageIds::title_menu_globe_construct_18,
        ImageIds::title_menu_globe_construct_19,
        ImageIds::title_menu_globe_construct_20,
        ImageIds::title_menu_globe_construct_21,
        ImageIds::title_menu_globe_construct_22,
        ImageIds::title_menu_globe_construct_23,
        ImageIds::title_menu_globe_construct_24,
        ImageIds::title_menu_globe_construct_25,
        ImageIds::title_menu_globe_construct_26,
        ImageIds::title_menu_globe_construct_27,
        ImageIds::title_menu_globe_construct_28,
        ImageIds::title_menu_globe_construct_29,
        ImageIds::title_menu_globe_construct_30,
        ImageIds::title_menu_globe_construct_31,
    };

    namespace widx
    {
        enum
        {
            scenario_list_btn,
            load_game_btn,
            tutorial_btn,
            scenario_editor_btn,
            chat_btn,
            multiplayer_toggle_btn,
        };
    }

    static widget_t _widgets[] = {
        makeWidget({ 0, 0 }, { btn_main_size, btn_main_size }, widget_type::wt_9, 1, StringIds::null, StringIds::title_menu_new_game),
        makeWidget({ btn_main_size, 0 }, { btn_main_size, btn_main_size }, widget_type::wt_9, 1, StringIds::null, StringIds::title_menu_load_game),
        makeWidget({ btn_main_size * 2, 0 }, { btn_main_size, btn_main_size }, widget_type::wt_9, 1, StringIds::null, StringIds::title_menu_show_tutorial),
        makeWidget({ btn_main_size * 3, 0 }, { btn_main_size, btn_main_size }, widget_type::wt_9, 1, StringIds::null, StringIds::title_menu_scenario_editor),
        makeWidget({ btn_main_size * 4 - 31, btn_main_size - 27 }, { 31, 27 }, widget_type::wt_9, 1, StringIds::null, StringIds::title_menu_chat_tooltip),
        makeWidget({ 0, btn_main_size }, { ww, btn_sub_height }, widget_type::wt_9, 1, StringIds::null, StringIds::title_multiplayer_toggle_tooltip),
        widgetEnd(),
    };

    static window_event_list _events;

    static void sub_439112(window* window);
    static void sub_4391CC(int16_t itemIndex);
    static void sub_43918F(char string[512]);
    static void sub_4391DA();
    static void sub_4391E2();
    static void sub_43910A();
    static void sub_439163(ui::window* callingWindow, widget_index callingWidget);
    static void sub_439102();
    static void sub_46E328();

    static void onMouseUp(ui::window* window, widget_index widgetIndex);
    static void onMouseDown(ui::window* window, widget_index widgetIndex);
    static void onDropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex);
    static void onUpdate(window* window);
    static void onTextInput(window* window, widget_index widgetIndex, char* input);
    static ui::cursor_id onCursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback);
    static void draw(ui::window* window, Gfx::drawpixelinfo_t* dpi);
    static void prepareDraw(ui::window* window);

    // static loco_global<window_event_list[1], 0x004f9ec8> _events;

    ui::window* openTitleMenu()
    {
        _events.on_mouse_up = onMouseUp;
        _events.on_mouse_down = onMouseDown;
        _events.on_dropdown = onDropdown;
        _events.text_input = onTextInput;
        _events.cursor = onCursor;
        _events.on_update = onUpdate;
        _events.prepare_draw = prepareDraw;
        _events.draw = draw;

        auto window = OpenLoco::ui::WindowManager::createWindow(
            WindowType::titleMenu,
            Gfx::point_t((ui::width() - ww) / 2, ui::height() - wh - 25),
            { ww, wh },
            window_flags::stick_to_front | window_flags::transparent | window_flags::no_background | window_flags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabled_widgets = (1 << widx::scenario_list_btn) | (1 << widx::load_game_btn) | (1 << widx::tutorial_btn) | (1 << widx::scenario_editor_btn) | (1 << widx::chat_btn) | (1 << widx::multiplayer_toggle_btn);

        window->initScrollWidgets();

        window->colours[0] = Colour::translucent(Colour::saturated_green);
        window->colours[1] = Colour::translucent(Colour::saturated_green);
        window->var_846 = 0;

        return window;
    }

    // 0x00438E0B
    static void prepareDraw(ui::window* window)
    {
        window->disabled_widgets = 0;
        window->widgets[widx::tutorial_btn].type = ui::widget_type::wt_9;
        window->widgets[widx::scenario_editor_btn].type = ui::widget_type::wt_9;

        // TODO: add widget::set_origin()
        window->widgets[widx::scenario_list_btn].left = 0;
        window->widgets[widx::scenario_list_btn].right = btn_main_size - 1;
        window->widgets[widx::load_game_btn].left = btn_main_size;
        window->widgets[widx::load_game_btn].right = btn_main_size * 2 - 1;
        window->widgets[widx::tutorial_btn].left = btn_main_size * 2;
        window->widgets[widx::tutorial_btn].right = btn_main_size * 3 - 1;
        window->widgets[widx::scenario_editor_btn].left = btn_main_size * 3;
        window->widgets[widx::scenario_editor_btn].right = btn_main_size * 4 - 1;
        window->widgets[widx::chat_btn].type = ui::widget_type::none;

        if (OpenLoco::isNetworked())
        {
            window->widgets[widx::tutorial_btn].type = ui::widget_type::none;
            window->widgets[widx::scenario_editor_btn].type = ui::widget_type::none;

            window->widgets[widx::scenario_list_btn].left = btn_main_size;
            window->widgets[widx::scenario_list_btn].right = btn_main_size * 2 - 1;
            window->widgets[widx::load_game_btn].left = btn_main_size * 2;
            window->widgets[widx::load_game_btn].right = btn_main_size * 3 - 1;

            window->widgets[widx::chat_btn].type = ui::widget_type::wt_9;
            interface_skin_object* skin = objectmgr::get<interface_skin_object>();
            window->widgets[widx::chat_btn].image = skin->img + InterfaceSkin::ImageIds::phone;
        }
    }

    // 0x00438EC7
    static void draw(ui::window* window, Gfx::drawpixelinfo_t* dpi)
    {
        // Draw widgets.
        window->draw(dpi);

        if (window->widgets[widx::scenario_list_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::scenario_list_btn].left + window->x;
            int16_t y = window->widgets[widx::scenario_list_btn].top + window->y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, widx::scenario_list_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            OpenLoco::Gfx::drawImage(dpi, x, y, image_id);
            OpenLoco::Gfx::drawImage(dpi, x, y, ImageIds::title_menu_sparkle);
        }

        if (window->widgets[widx::load_game_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::load_game_btn].left + window->x;
            int16_t y = window->widgets[widx::load_game_btn].top + window->y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, widx::load_game_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            OpenLoco::Gfx::drawImage(dpi, x, y, image_id);
            OpenLoco::Gfx::drawImage(dpi, x, y, ImageIds::title_menu_save);
        }

        if (window->widgets[widx::tutorial_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::tutorial_btn].left + window->x;
            int16_t y = window->widgets[widx::tutorial_btn].top + window->y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, widx::tutorial_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            OpenLoco::Gfx::drawImage(dpi, x, y, image_id);

            // TODO: base lesson overlay on language
            OpenLoco::Gfx::drawImage(dpi, x, y, ImageIds::title_menu_lesson_l);
        }

        if (window->widgets[widx::scenario_editor_btn].type != ui::widget_type::none)
        {
            int16_t x = window->widgets[widx::scenario_editor_btn].left + window->x;
            int16_t y = window->widgets[widx::scenario_editor_btn].top + window->y;

            uint32_t image_id = ImageIds::title_menu_globe_construct_24;
            if (Input::isHovering(WindowType::titleMenu, 0, widx::scenario_editor_btn))
            {
                image_id = globe_construct[((window->var_846 / 2) % globe_construct.size())];
            }

            OpenLoco::Gfx::drawImage(dpi, x, y, image_id);
        }

        {
            int16_t y = window->widgets[widx::multiplayer_toggle_btn].top + 3 + window->y;
            int16_t x = window->width / 2 + window->x;

            string_id string = StringIds::single_player_mode;

            if (OpenLoco::isNetworked())
            {
                // char[512+1]
                auto buffer = StringManager::getString(StringIds::buffer_2039);

                char* playerName = (char*)0xF254D0;

                strcpy((char*)buffer, playerName);

                addr<0x112C826, string_id>() = StringIds::buffer_2039;
                string = StringIds::two_player_mode_connected;
            }

            drawStringCentredClipped(*dpi, x, y, ww - 4, Colour::black, string, (char*)0x112c826);
        }
    }

    // 0x00439094
    static void onMouseUp(ui::window* window, widget_index widgetIndex)
    {
        if (intro::isActive())
        {
            return;
        }

        sub_46E328();

        switch (widgetIndex)
        {
            case widx::scenario_list_btn:
                sub_4391DA();
                break;
            case widx::load_game_btn:
                sub_4391E2();
                break;
            case widx::scenario_editor_btn:
                sub_43910A();
                break;
            case widx::chat_btn:
                sub_439163(window, widgetIndex);
                break;
            case widx::multiplayer_toggle_btn:
                sub_439102();
                break;
        }
    }

    // 0x004390D1
    static void onMouseDown(ui::window* window, widget_index widgetIndex)
    {
        sub_46E328();
        switch (widgetIndex)
        {
            case widx::tutorial_btn:
                sub_439112(window);
                break;
        }
    }

    // 0x004390DD
    static void onDropdown(ui::window* window, widget_index widgetIndex, int16_t itemIndex)
    {
        sub_46E328();
        switch (widgetIndex)
        {
            case widx::tutorial_btn:
                sub_4391CC(itemIndex);
                break;
        }
    }

    // 0x004390ED
    static void onTextInput(window* window, widget_index widgetIndex, char* input)
    {
        switch (widgetIndex)
        {
            case widx::chat_btn:
                sub_43918F(input);
                break;
        }
    }

    // 0x004390f8
    static ui::cursor_id onCursor(window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback)
    {
        // Reset tooltip timeout to keep tooltips open.
        addr<0x0052338A, uint16_t>() = 2000;
        return fallback;
    }

    static void sub_439102()
    {
        call(0x0046e639); // window_multiplayer::open
    }

    static widget_t _editorWidgets[] = {
        makeWidget({ 0, 0 }, { 0, 0 }, widget_type::viewport, 0, 0xFFFFFFFE),
        widgetEnd(),
    };

    // 0x0043CB9F
    void editorInit()
    {
        const int32_t uiWidth = ui::width();
        const int32_t uiHeight = ui::height();

        _editorWidgets[0].bottom = uiHeight;
        _editorWidgets[0].right = uiWidth;
        auto window = WindowManager::createWindow(
            WindowType::main,
            { 0, 0 },
            Gfx::ui_size_t(uiWidth, uiHeight),
            ui::window_flags::stick_to_back,
            (ui::window_event_list*)0x004FA5F8);
        window->widgets = _editorWidgets;
        addr<0x00e3f0b8, int32_t>() = 0; // gCurrentRotation?
        OpenLoco::ui::viewportmgr::create(
            window,
            0,
            { window->x, window->y },
            { window->width, window->height },
            ZoomLevel::full,
            { (OpenLoco::Map::map_rows * OpenLoco::Map::tile_size) / 2 - 1, (OpenLoco::Map::map_rows * OpenLoco::Map::tile_size) / 2 - 1, 480 });

        addr<0x00F2533F, int8_t>() = 0; // grid lines
        addr<0x0112C2e1, int8_t>() = 0;
        addr<0x009c870E, int8_t>() = 0;
        addr<0x009c870F, int8_t>() = 2;
        addr<0x009c8710, int8_t>() = 1;

        toolbar_top::editor::open();
        toolbar_bottom::editor::open();
    }

    static void sub_43910A()
    {
        call(0x0043D7DC); // show_scenario_editor
    }

    static void sub_439112(window* window)
    {
        dropdown::add(0, StringIds::tutorial_1_title);
        dropdown::add(1, StringIds::tutorial_2_title);
        dropdown::add(2, StringIds::tutorial_3_title);

        widget_t* widget = &window->widgets[widx::tutorial_btn];
        dropdown::showText(
            window->x + widget->left,
            window->y + widget->top,
            widget->width(),
            widget->height(),
            Colour::translucent(window->colours[0]),
            3,
            0x80);
    }

    static void sub_439163(ui::window* callingWindow, widget_index callingWidget)
    {
        WindowManager::close(WindowType::multiplayer);

        addr<0x112C826 + 8, string_id>() = StringIds::the_other_player;

        // TODO: convert this to a builder pattern, with chainable functions to set the different string ids and arguments
        textinput::openTextinput(callingWindow, StringIds::chat_title, StringIds::chat_instructions, StringIds::empty, callingWidget, (void*)0x112C826);
    }

    static void sub_43918F(char string[512])
    {
        addr<0x009C68E8, string_id>() = StringIds::empty;

        for (int i = 0; i < 32; i++)
        {
            game_commands::do_71(i, &string[i * 16]);
        }
    }

    static void sub_4391CC(int16_t itemIndex)
    {
        // DROPDOWN_ITEM_UNDEFINED
        if (itemIndex == -1)
            return;

        tutorial::start(itemIndex);
    }

    static void sub_4391DA()
    {
        call(0x443868); // scenario_select::open()
    }

    static void sub_4391E2()
    {
        game_commands::do_21(0, 0);
    }

    static void sub_46E328()
    {
        call(0x0046e328);
    }

    // 0x004391F9
    static void onUpdate(window* window)
    {
        window->var_846++;

        if (intro::isActive())
        {
            window->invalidate();
            return;
        }

        if (!multiplayer::hasFlag(multiplayer::flags::flag_8) && !multiplayer::hasFlag(multiplayer::flags::flag_9))
        {
            window->invalidate();
            return;
        }

        if (addr<0x0050C1AE, int32_t>() < 20)
        {
            window->invalidate();
            return;
        }

        auto multiplayer = WindowManager::find(WindowType::multiplayer);
        if (multiplayer == nullptr)
        {
            call(0x0046e639); // window_multiplayer::open
        }

        window->invalidate();
    }
}
