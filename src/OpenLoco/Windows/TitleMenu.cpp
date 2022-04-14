#include "../EditorController.h"
#include "../GameCommands/GameCommands.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Gui.h"
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
#include "../Widget.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::TitleMenu
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

    namespace Widx
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

    static Widget _widgets[] = {
        makeWidget({ 0, 0 }, { btn_main_size, btn_main_size }, WidgetType::buttonWithImage, WindowColour::secondary, StringIds::null, StringIds::title_menu_new_game),
        makeWidget({ btn_main_size, 0 }, { btn_main_size, btn_main_size }, WidgetType::buttonWithImage, WindowColour::secondary, StringIds::null, StringIds::title_menu_load_game),
        makeWidget({ btn_main_size * 2, 0 }, { btn_main_size, btn_main_size }, WidgetType::buttonWithImage, WindowColour::secondary, StringIds::null, StringIds::title_menu_show_tutorial),
        makeWidget({ btn_main_size * 3, 0 }, { btn_main_size, btn_main_size }, WidgetType::buttonWithImage, WindowColour::secondary, StringIds::null, StringIds::title_menu_scenario_editor),
        makeWidget({ btn_main_size * 4 - 31, btn_main_size - 27 }, { 31, 27 }, WidgetType::buttonWithImage, WindowColour::secondary, StringIds::null, StringIds::title_menu_chat_tooltip),
        makeWidget({ 0, btn_main_size }, { ww, btn_sub_height }, WidgetType::none, WindowColour::secondary, StringIds::null, StringIds::title_multiplayer_toggle_tooltip),
        widgetEnd(),
    };

    static WindowEventList _events;

    static void sub_439112(Window* window);
    static void sub_4391CC(int16_t itemIndex);
    static void sub_43918F(const char* string);
    static void sub_4391DA();
    static void sub_4391E2();
    static void sub_43910A();
    static void sub_439163(Ui::Window* callingWindow, WidgetIndex_t callingWidget);
    static void sub_439102();
    static void sub_46E328();

    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex);
    static void onMouseDown(Ui::Window* window, WidgetIndex_t widgetIndex);
    static void onDropdown(Ui::Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex);
    static void onUpdate(Window* window);
    static void onTextInput(Window* window, WidgetIndex_t widgetIndex, const char* input);
    static Ui::CursorId onCursor(Window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback);
    static void draw(Ui::Window* window, Gfx::Context* context);
    static void prepareDraw(Ui::Window* window);

    // static loco_global<WindowEventList[1], 0x004f9ec8> _events;

    Window* open()
    {
        _events.onMouseUp = onMouseUp;
        _events.onMouseDown = onMouseDown;
        _events.onDropdown = onDropdown;
        _events.textInput = onTextInput;
        _events.cursor = onCursor;
        _events.onUpdate = onUpdate;
        _events.prepareDraw = prepareDraw;
        _events.draw = draw;

        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::titleMenu,
            Ui::Point((Ui::width() - ww) / 2, Ui::height() - wh - 25),
            { ww, wh },
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            &_events);

        window->widgets = _widgets;
        window->enabledWidgets = (1 << Widx::scenario_list_btn) | (1 << Widx::load_game_btn) | (1 << Widx::tutorial_btn) | (1 << Widx::scenario_editor_btn) | (1 << Widx::chat_btn);

        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->var_846 = 0;

        return window;
    }

    // 0x00438E0B
    static void prepareDraw(Ui::Window* window)
    {
        window->disabledWidgets = 0;
        window->widgets[Widx::tutorial_btn].type = Ui::WidgetType::buttonWithImage;
        window->widgets[Widx::scenario_editor_btn].type = Ui::WidgetType::buttonWithImage;

        // TODO: add widget::set_origin()
        window->widgets[Widx::scenario_list_btn].left = 0;
        window->widgets[Widx::scenario_list_btn].right = btn_main_size - 1;
        window->widgets[Widx::load_game_btn].left = btn_main_size;
        window->widgets[Widx::load_game_btn].right = btn_main_size * 2 - 1;
        window->widgets[Widx::tutorial_btn].left = btn_main_size * 2;
        window->widgets[Widx::tutorial_btn].right = btn_main_size * 3 - 1;
        window->widgets[Widx::scenario_editor_btn].left = btn_main_size * 3;
        window->widgets[Widx::scenario_editor_btn].right = btn_main_size * 4 - 1;
        window->widgets[Widx::chat_btn].type = Ui::WidgetType::none;

        if (OpenLoco::isNetworked())
        {
            window->widgets[Widx::tutorial_btn].type = Ui::WidgetType::none;
            window->widgets[Widx::scenario_editor_btn].type = Ui::WidgetType::none;

            window->widgets[Widx::scenario_list_btn].left = btn_main_size;
            window->widgets[Widx::scenario_list_btn].right = btn_main_size * 2 - 1;
            window->widgets[Widx::load_game_btn].left = btn_main_size * 2;
            window->widgets[Widx::load_game_btn].right = btn_main_size * 3 - 1;

            window->widgets[Widx::chat_btn].type = Ui::WidgetType::buttonWithImage;
            auto* skin = ObjectManager::get<InterfaceSkinObject>();
            window->widgets[Widx::chat_btn].image = skin->img + InterfaceSkin::ImageIds::phone;
        }
    }

    // 0x00438EC7
    static void draw(Ui::Window* window, Gfx::Context* context)
    {
        // Draw widgets.
        window->draw(context);

        if (window->widgets[Widx::scenario_list_btn].type != Ui::WidgetType::none)
        {
            int16_t x = window->widgets[Widx::scenario_list_btn].left + window->x;
            int16_t y = window->widgets[Widx::scenario_list_btn].top + window->y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, Widx::scenario_list_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            OpenLoco::Gfx::drawImage(context, x, y, image_id);
            OpenLoco::Gfx::drawImage(context, x, y, ImageIds::title_menu_sparkle);
        }

        if (window->widgets[Widx::load_game_btn].type != Ui::WidgetType::none)
        {
            int16_t x = window->widgets[Widx::load_game_btn].left + window->x;
            int16_t y = window->widgets[Widx::load_game_btn].top + window->y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, Widx::load_game_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            OpenLoco::Gfx::drawImage(context, x, y, image_id);
            OpenLoco::Gfx::drawImage(context, x, y, ImageIds::title_menu_save);
        }

        if (window->widgets[Widx::tutorial_btn].type != Ui::WidgetType::none)
        {
            int16_t x = window->widgets[Widx::tutorial_btn].left + window->x;
            int16_t y = window->widgets[Widx::tutorial_btn].top + window->y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, Widx::tutorial_btn))
            {
                image_id = globe_spin[((window->var_846 / 2) % globe_spin.size())];
            }

            OpenLoco::Gfx::drawImage(context, x, y, image_id);

            // TODO: base lesson overlay on language
            OpenLoco::Gfx::drawImage(context, x, y, ImageIds::title_menu_lesson_l);
        }

        if (window->widgets[Widx::scenario_editor_btn].type != Ui::WidgetType::none)
        {
            int16_t x = window->widgets[Widx::scenario_editor_btn].left + window->x;
            int16_t y = window->widgets[Widx::scenario_editor_btn].top + window->y;

            uint32_t image_id = ImageIds::title_menu_globe_construct_24;
            if (Input::isHovering(WindowType::titleMenu, 0, Widx::scenario_editor_btn))
            {
                image_id = globe_construct[((window->var_846 / 2) % globe_construct.size())];
            }

            OpenLoco::Gfx::drawImage(context, x, y, image_id);
        }

        if (window->widgets[Widx::multiplayer_toggle_btn].type != Ui::WidgetType::none)
        {
            int16_t y = window->widgets[Widx::multiplayer_toggle_btn].top + 3 + window->y;
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

            drawStringCentredClipped(*context, x, y, ww - 4, Colour::black, string, (char*)0x112c826);
        }
    }

    // 0x00439094
    static void onMouseUp(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        if (Intro::isActive())
        {
            return;
        }

        sub_46E328();

        switch (widgetIndex)
        {
            case Widx::scenario_list_btn:
                sub_4391DA();
                break;
            case Widx::load_game_btn:
                sub_4391E2();
                break;
            case Widx::scenario_editor_btn:
                sub_43910A();
                break;
            case Widx::chat_btn:
                sub_439163(window, widgetIndex);
                break;
            case Widx::multiplayer_toggle_btn:
                sub_439102();
                break;
        }
    }

    // 0x004390D1
    static void onMouseDown(Ui::Window* window, WidgetIndex_t widgetIndex)
    {
        sub_46E328();
        switch (widgetIndex)
        {
            case Widx::tutorial_btn:
                sub_439112(window);
                break;
        }
    }

    // 0x004390DD
    static void onDropdown(Ui::Window* window, WidgetIndex_t widgetIndex, int16_t itemIndex)
    {
        sub_46E328();
        switch (widgetIndex)
        {
            case Widx::tutorial_btn:
                sub_4391CC(itemIndex);
                break;
        }
    }

    // 0x004390ED
    static void onTextInput(Window* window, WidgetIndex_t widgetIndex, const char* input)
    {
        switch (widgetIndex)
        {
            case Widx::chat_btn:
                sub_43918F(input);
                break;
        }
    }

    // 0x004390f8
    static Ui::CursorId onCursor(Window* window, int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback)
    {
        // Reset tooltip timeout to keep tooltips open.
        addr<0x0052338A, uint16_t>() = 2000;
        return fallback;
    }

    static void sub_439102()
    {
        call(0x0046e639); // window_multiplayer::open
    }

    // 0x0043CB9F
    void editorInit()
    {
        Main::open();

        addr<0x00F2533F, int8_t>() = 0; // grid lines
        addr<0x0112C2e1, int8_t>() = 0;
        addr<0x009c870E, int8_t>() = 0;
        addr<0x009c870F, int8_t>() = 2;
        addr<0x009c8710, int8_t>() = 1;

        ToolbarTop::Editor::open();
        ToolbarBottom::Editor::open();
        Gui::resize();
    }

    static void sub_43910A()
    {
        EditorController::init();
    }

    static void sub_439112(Window* window)
    {
        Dropdown::add(0, StringIds::tutorial_1_title);
        Dropdown::add(1, StringIds::tutorial_2_title);
        Dropdown::add(2, StringIds::tutorial_3_title);

        Widget* widget = &window->widgets[Widx::tutorial_btn];
        Dropdown::showText(
            window->x + widget->left,
            window->y + widget->top,
            widget->width(),
            widget->height(),
            window->getColour(WindowColour::primary).translucent(),
            3,
            0x80);
    }

    static void sub_439163(Ui::Window* callingWindow, WidgetIndex_t callingWidget)
    {
        WindowManager::close(WindowType::multiplayer);

        addr<0x112C826 + 8, string_id>() = StringIds::the_other_player;

        // TODO: convert this to a builder pattern, with chainable functions to set the different string ids and arguments
        TextInput::openTextInput(callingWindow, StringIds::chat_title, StringIds::chat_instructions, StringIds::empty, callingWidget, (void*)0x112C826);
    }

    static void sub_43918F(const char* string)
    {
        // Identical to processChatMessage
        GameCommands::setErrorTitle(StringIds::empty);

        for (int i = 0; i < 32; i++)
        {
            GameCommands::do_71(i, &string[i * 16]);
        }
    }

    static void sub_4391CC(int16_t itemIndex)
    {
        // DROPDOWN_ITEM_UNDEFINED
        if (itemIndex == -1)
            return;

        OpenLoco::Tutorial::start(itemIndex);
    }

    static void sub_4391DA()
    {
        ScenarioSelect::open();
    }

    static void sub_4391E2()
    {
        GameCommands::do_21(0, 0);
    }

    static void sub_46E328()
    {
        call(0x0046e328);
    }

    // 0x004391F9
    static void onUpdate(Window* window)
    {
        window->var_846++;

        if (Intro::isActive())
        {
            window->invalidate();
            return;
        }

        if (!MultiPlayer::hasFlag(MultiPlayer::flags::flag_8) && !MultiPlayer::hasFlag(MultiPlayer::flags::flag_9))
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
