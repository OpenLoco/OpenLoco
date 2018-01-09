#pragma once

#include "window.h"

namespace openloco::ui
{
    enum class window_type
    {
        window_39 = 39,
        text_input = 51,
        load_game = 52,
        save_game = 54,
    };
}

namespace openloco::ui::windowmgr
{
    void update();
    void resize();
    window * find(window_type type);
    window * find(window_type type, uint16_t id);
    void close(window_type type);
    void close(window_type type, uint16_t id);
    window * create_window(window_type type, int32_t x, int32_t y, int32_t width, int32_t height, int32_t flags, void * events);
    window * create_window_centred(window_type type, int32_t width, int32_t height, int32_t flags, void * events);
}

namespace openloco::ui::windows
{
    bool prompt_load_game(uint8_t al, char * path, const char * filter, const char * title);
}

namespace openloco::ui::textinput
{
    void close();
}
