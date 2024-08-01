#include "Config.h"
#include "EditorController.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/LoadSaveQuit.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Gui.h"
#include "Input.h"
#include "Intro.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Logging.h"
#include "Map/Tile.h"
#include "MultiPlayer.h"
#include "Network/Network.h"
#include "Objects/InterfaceSkinObject.h"
#include "Objects/ObjectManager.h"
#include "SceneManager.h"
#include "Title.h"
#include "Tutorial.h"
#include "Ui.h"
#include "Ui/Dropdown.h"
#include "Ui/Widget.h"
#include "Ui/WindowManager.h"
#include "ViewportManager.h"
#include <OpenLoco/Utility/String.hpp>
#include <string_view>

namespace OpenLoco::Ui::Windows::TitleMenu
{
    static constexpr uint8_t kBtnMainSize = 74;
    static constexpr uint8_t kBtnSubHeight = 18;
    static constexpr uint16_t kWW = kBtnMainSize * 4;
    static constexpr uint16_t kWH = kBtnMainSize + kBtnSubHeight;

    static constexpr std::array<uint32_t, 32> kGlobeSpin = {
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

    static constexpr std::array<uint32_t, 32> kGlobeConstruct = {
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

    static constexpr Widget _widgets[] = {
        makeWidget({ 0, 0 }, { kBtnMainSize, kBtnMainSize }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::title_menu_new_game),
        makeWidget({ kBtnMainSize, 0 }, { kBtnMainSize, kBtnMainSize }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::title_menu_load_game),
        makeWidget({ kBtnMainSize * 2, 0 }, { kBtnMainSize, kBtnMainSize }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::title_menu_show_tutorial),
        makeWidget({ kBtnMainSize * 3, 0 }, { kBtnMainSize, kBtnMainSize }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::title_menu_scenario_editor),
        makeWidget({ kBtnMainSize * 4 - 31, kBtnMainSize - 27 }, { 31, 27 }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::title_menu_chat_tooltip),
        makeWidget({ 0, kBtnMainSize }, { kWW, kBtnSubHeight }, WidgetType::buttonWithImage, WindowColour::secondary, Widget::kContentNull, StringIds::title_multiplayer_toggle_tooltip),
        widgetEnd(),
    };

    static void sub_439112(Window* window);
    static void sub_4391CC(int16_t itemIndex);
    static void sub_43918F(const char* string);
    static void sub_4391DA();
    static void sub_4391E2();
    static void sub_43910A();
    static void showMultiplayer(Window* window);
    static void multiplayerConnect(std::string_view host);
    static void sub_46E328();
    static const WindowEventList& getEvents();

    Window* open()
    {
        auto window = OpenLoco::Ui::WindowManager::createWindow(
            WindowType::titleMenu,
            Ui::Point((Ui::width() - kWW) / 2, Ui::height() - kWH - 25),
            { kWW, kWH },
            WindowFlags::stickToFront | WindowFlags::transparent | WindowFlags::noBackground | WindowFlags::flag_6,
            getEvents());

        window->setWidgets(_widgets);
        window->enabledWidgets = (1 << Widx::scenario_list_btn) | (1 << Widx::load_game_btn) | (1 << Widx::tutorial_btn) | (1 << Widx::scenario_editor_btn) | (1 << Widx::chat_btn) | (1 << Widx::multiplayer_toggle_btn);

        window->initScrollWidgets();

        window->setColour(WindowColour::primary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->setColour(WindowColour::secondary, AdvancedColour(Colour::mutedSeaGreen).translucent());
        window->var_846 = 0;

        return window;
    }

    // 0x00438E0B
    static void prepareDraw(Ui::Window& window)
    {
        window.disabledWidgets = 0;
        window.widgets[Widx::tutorial_btn].type = Ui::WidgetType::buttonWithImage;
        window.widgets[Widx::scenario_editor_btn].type = Ui::WidgetType::buttonWithImage;

        // TODO: add widget::set_origin()
        window.widgets[Widx::scenario_list_btn].left = 0;
        window.widgets[Widx::scenario_list_btn].right = kBtnMainSize - 1;
        window.widgets[Widx::load_game_btn].left = kBtnMainSize;
        window.widgets[Widx::load_game_btn].right = kBtnMainSize * 2 - 1;
        window.widgets[Widx::tutorial_btn].left = kBtnMainSize * 2;
        window.widgets[Widx::tutorial_btn].right = kBtnMainSize * 3 - 1;
        window.widgets[Widx::scenario_editor_btn].left = kBtnMainSize * 3;
        window.widgets[Widx::scenario_editor_btn].right = kBtnMainSize * 4 - 1;
        window.widgets[Widx::chat_btn].type = Ui::WidgetType::none;

        auto& config = Config::get();
        window.widgets[Widx::multiplayer_toggle_btn].type = config.network.enabled ? WidgetType::buttonWithImage : WidgetType::none;

        if (OpenLoco::isNetworked())
        {
            window.widgets[Widx::tutorial_btn].type = Ui::WidgetType::none;
            window.widgets[Widx::scenario_editor_btn].type = Ui::WidgetType::none;

            window.widgets[Widx::scenario_list_btn].left = kBtnMainSize;
            window.widgets[Widx::scenario_list_btn].right = kBtnMainSize * 2 - 1;
            window.widgets[Widx::load_game_btn].left = kBtnMainSize * 2;
            window.widgets[Widx::load_game_btn].right = kBtnMainSize * 3 - 1;

            window.widgets[Widx::chat_btn].type = Ui::WidgetType::buttonWithImage;
            auto* skin = ObjectManager::get<InterfaceSkinObject>();
            window.widgets[Widx::chat_btn].image = skin->img + InterfaceSkin::ImageIds::phone;
        }
    }

    // 0x00438EC7
    static void draw(Ui::Window& window, Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        // Draw widgets.
        window.draw(drawingCtx);

        if (window.widgets[Widx::scenario_list_btn].type != Ui::WidgetType::none)
        {
            int16_t x = window.widgets[Widx::scenario_list_btn].left + window.x;
            int16_t y = window.widgets[Widx::scenario_list_btn].top + window.y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, Widx::scenario_list_btn))
            {
                image_id = kGlobeSpin[((window.var_846 / 2) % kGlobeSpin.size())];
            }

            drawingCtx.drawImage(x, y, image_id);
            drawingCtx.drawImage(x, y, ImageIds::title_menu_sparkle);
        }

        if (window.widgets[Widx::load_game_btn].type != Ui::WidgetType::none)
        {
            int16_t x = window.widgets[Widx::load_game_btn].left + window.x;
            int16_t y = window.widgets[Widx::load_game_btn].top + window.y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, Widx::load_game_btn))
            {
                image_id = kGlobeSpin[((window.var_846 / 2) % kGlobeSpin.size())];
            }

            drawingCtx.drawImage(x, y, image_id);
            drawingCtx.drawImage(x, y, ImageIds::title_menu_save);
        }

        if (window.widgets[Widx::tutorial_btn].type != Ui::WidgetType::none)
        {
            int16_t x = window.widgets[Widx::tutorial_btn].left + window.x;
            int16_t y = window.widgets[Widx::tutorial_btn].top + window.y;

            uint32_t image_id = ImageIds::title_menu_globe_spin_0;
            if (Input::isHovering(WindowType::titleMenu, 0, Widx::tutorial_btn))
            {
                image_id = kGlobeSpin[((window.var_846 / 2) % kGlobeSpin.size())];
            }

            drawingCtx.drawImage(x, y, image_id);

            // TODO: base lesson overlay on language
            drawingCtx.drawImage(x, y, ImageIds::title_menu_lesson_l);
        }

        if (window.widgets[Widx::scenario_editor_btn].type != Ui::WidgetType::none)
        {
            int16_t x = window.widgets[Widx::scenario_editor_btn].left + window.x;
            int16_t y = window.widgets[Widx::scenario_editor_btn].top + window.y;

            uint32_t image_id = ImageIds::title_menu_globe_construct_24;
            if (Input::isHovering(WindowType::titleMenu, 0, Widx::scenario_editor_btn))
            {
                image_id = kGlobeConstruct[((window.var_846 / 2) % kGlobeConstruct.size())];
            }

            drawingCtx.drawImage(x, y, image_id);
        }

        if (window.widgets[Widx::multiplayer_toggle_btn].type != Ui::WidgetType::none)
        {
            auto& widget = window.widgets[Widx::multiplayer_toggle_btn];
            auto point = Point(widget.top + 3 + window.y, window.width / 2 + window.x);

            StringId string = StringIds::single_player_mode;
            FormatArguments args{};

            if (OpenLoco::isNetworked())
            {
                // char[512+1]
                auto buffer = StringManager::getString(StringIds::buffer_2039);

                // TODO: ?? replace this
                char* playerName = (char*)0xF254D0;
                strcpy((char*)buffer, playerName);

                args.push(StringIds::buffer_2039);
                string = StringIds::two_player_mode_connected;
            }

            tr.drawStringCentredClipped(point, kWW - 4, Colour::black, string, args);
        }
    }

    // 0x00439094
    static void onMouseUp(Ui::Window& window, WidgetIndex_t widgetIndex)
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
                Title::stop();
                sub_43910A();
                break;
            case Widx::chat_btn:
                beginSendChatMessage(window);
                break;
            case Widx::multiplayer_toggle_btn:
                showMultiplayer(&window);
                break;
        }
    }

    // 0x004390D1
    static void onMouseDown(Ui::Window& window, WidgetIndex_t widgetIndex)
    {
        sub_46E328();
        switch (widgetIndex)
        {
            case Widx::tutorial_btn:
                sub_439112(&window);
                break;
        }
    }

    // 0x004390DD
    static void onDropdown([[maybe_unused]] Ui::Window& window, WidgetIndex_t widgetIndex, int16_t itemIndex)
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
    static void onTextInput([[maybe_unused]] Window& window, WidgetIndex_t widgetIndex, const char* input)
    {
        switch (widgetIndex)
        {
            case Widx::chat_btn:
                sub_43918F(input);
                break;
            case Widx::multiplayer_toggle_btn:
                multiplayerConnect(input);
                break;
        }
    }

    // 0x004390f8
    static Ui::CursorId onCursor([[maybe_unused]] Window& window, [[maybe_unused]] int16_t widgetIdx, [[maybe_unused]] int16_t xPos, [[maybe_unused]] int16_t yPos, Ui::CursorId fallback)
    {
        // Reset tooltip timeout to keep tooltips open.
        addr<0x0052338A, uint16_t>() = 2000;
        return fallback;
    }

    static void showMultiplayer(Window* window)
    {
        auto& cfg = Config::get().old;
        StringManager::setString(StringIds::buffer_2039, Utility::nullTerminatedView(cfg.lastHost));
        TextInput::openTextInput(window, StringIds::enter_host_address, StringIds::enter_host_address_description, StringIds::buffer_2039, Widx::multiplayer_toggle_btn, nullptr);
    }

    static void multiplayerConnect(std::string_view host)
    {
        auto& cfg = Config::get().old;
        auto szHost = std::string(host);
        Utility::strcpy_safe(cfg.lastHost, szHost.c_str());

        Network::joinServer(host);
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

    void beginSendChatMessage(Window& self)
    {
        WindowManager::close(WindowType::multiplayer);

        auto args = FormatArguments::common();
        args.push(StringIds::the_other_player);

        // TODO: convert this to a builder pattern, with chainable functions to set the different string ids and arguments
        TextInput::openTextInput(&self, StringIds::chat_title, StringIds::chat_instructions, StringIds::empty, Widx::chat_btn, &args);
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
        GameCommands::LoadSaveQuitGameArgs args{};
        args.option1 = GameCommands::LoadSaveQuitGameArgs::Options::save;
        args.option2 = LoadOrQuitMode::loadGamePrompt;
        GameCommands::doCommand(args, GameCommands::Flags::apply);
    }

    static void sub_46E328()
    {
        call(0x0046e328);
    }

    // 0x004391F9
    static void onUpdate(Window& window)
    {
        window.var_846++;

        if (Intro::isActive())
        {
            window.invalidate();
            return;
        }

        if (!MultiPlayer::hasFlag(MultiPlayer::flags::flag_8) && !MultiPlayer::hasFlag(MultiPlayer::flags::flag_9))
        {
            window.invalidate();
            return;
        }

        if (addr<0x0050C1AE, int32_t>() < 20)
        {
            window.invalidate();
            return;
        }

        window.invalidate();
    }

    static constexpr WindowEventList kEvents = {
        .onMouseUp = onMouseUp,
        .onMouseDown = onMouseDown,
        .onDropdown = onDropdown,
        .onUpdate = onUpdate,
        .textInput = onTextInput,
        .cursor = onCursor,
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
