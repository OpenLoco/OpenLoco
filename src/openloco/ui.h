#pragma once

#include "graphics/gfx.h"
#include <string>
#include <vector>

namespace openloco::config
{
    enum class screen_mode;
    struct display_config;
    struct resolution_t;
}

namespace openloco::ui
{

    struct screen_info_t
    {
        gfx::drawpixelinfo_t dpi;
        int16_t width;
        int16_t height;
        int16_t width_2;
        int16_t height_2;
        int16_t width_3;
        int16_t height_3;
        int16_t dirty_block_width;
        int16_t dirty_block_height;
        int32_t dirty_block_columns;
        int32_t dirty_block_rows;
        int8_t dirty_block_column_shift;
        int8_t dirty_block_row_shift;
        int8_t dirty_blocks_initialised;
    };

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

    namespace ScaleFactor
    {
        const float min = 1.0f;
        const float max = 4.0f;
        const float step = 1.0f;
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
    config::resolution_t getResolution();
    config::resolution_t getDesktopResolution();
    bool setDisplayMode(config::screen_mode mode, config::resolution_t newResolution);
    bool setDisplayMode(config::screen_mode mode);
    void updateFullscreenResolutions();
    std::vector<Resolution> getFullscreenResolutions();
    Resolution getClosestResolution(int32_t inWidth, int32_t inHeight);
    void handleInput();
    void minimalHandleInput();
    void setWindowScaling(float newScaleFactor);
    void adjust_window_scale(float adjust_by);

    namespace viewport_interaction
    {
        struct InteractionArg
        {
            coord_t x;
            coord_t y;
            union
            {
                uint32_t value;
                void* object;
            };
            uint8_t unkBh;
        };

        enum class InteractionItem : uint8_t
        {
            t_0 = 0,
            t_1 = 1,
            t_2 = 2,
            thing = 3,
            track = 4,
            trackExtra = 5,
            signal = 6,
            trackStation = 7,
            roadStation = 8,
            airport = 9,
            dock = 10,
            t_11 = 11,
            tree = 12,
            wall = 13,
            town = 14,
            station = 15,
            road = 16,
            roadExtra = 17,
            t_18 = 18, // bridge?
            building = 19,
            industry = 20,
            headquarterBuilding = 21,
        };

        InteractionItem getItemLeft(int16_t tempX, int16_t tempY, InteractionArg* arg);
        InteractionItem rightOver(int16_t x, int16_t y, InteractionArg* arg);
    }
}
