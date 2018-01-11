#pragma once

#include "openloco.h"
#include "window.h"

namespace openloco::ui
{
    enum class window_type
    {
        window_39 = 39,
        text_input = 51,
        load_game = 52,
        prompt_ok_cancel = 54,
        undefined = 255
    };
}

namespace openloco::ui::windowmgr
{
    window_type current_modal_type();
    void current_modal_type(window_type type);

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
    enum class browse_type
    {
        load = 1,
        save = 2
    };

    bool prompt_browse(browse_type type, char * path, const char * filter, const char * title);
    bool prompt_ok_cancel(string_id okButtonStringId);
}

namespace openloco::ui::textinput
{
    void close();
}
