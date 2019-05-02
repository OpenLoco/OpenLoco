#pragma once

#include <string>
#include <vector>

namespace openloco::config
{
    enum class screen_mode;
    struct display_config;
}

namespace openloco::ui
{
    enum class cursor_id
    {
        pointer,
        blank,
        up_arrow,
        up_down_arrow,
        hand_pointer,
        busy,
        diagonal_arrows,
    };

    struct Resolution
    {
        int32_t width;
        int32_t height;
    };

#ifdef _WIN32
    void* hwnd();
#endif
    int32_t width();
    int32_t height();
    bool dirty_blocks_initialised();

    void create_window(const config::display_config& cfg);
    void initialise();
    void initialise_cursors();
    void initialise_input();
    void dispose_input();
    void dispose_cursors();
    void set_cursor(cursor_id id);
    void get_cursor_pos(int32_t& x, int32_t& y);
    void set_cursor_pos(int32_t x, int32_t y);
    void hide_cursor();
    void show_cursor();
    void update();
    void trigger_resize();
    void render();
    bool process_messages();
    void show_message_box(const std::string& title, const std::string& message);
    void updateFullscreenResolutions();
    std::vector<Resolution> getFullscreenResolutions();
    Resolution getClosestResolution(int32_t inWidth, int32_t inHeight);
#if true || !(defined(__APPLE__) && defined(__MACH__))
    void set_screen_mode(config::screen_mode mode);
#endif
    void handleInput();
    void minimalHandleInput();

    namespace viewport_interaction
    {
        uint8_t get_item_left(int16_t x, int16_t y, void* arg);
        void right_over(int16_t x, int16_t y);
    }
}
