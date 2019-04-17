#include "../graphics/colours.h"
#include "../localisation/stringmgr.h"
#include "../window.h"
#include <cstdlib>
#include <vector>

enum format_arg_type
{
    u16,
    u32,
    ptr,
};

struct format_arg
{
    format_arg_type type;
    union
    {
        uint16_t u16;
        uint32_t u32;
        uintptr_t ptr;
    };

    format_arg(uint16_t value)
    {
        type = format_arg_type::u16;
        u16 = value;
    }

    format_arg(uint32_t value)
    {
        type = format_arg_type::u32;
        u32 = value;
    }

    format_arg(char* value)
    {
        type = format_arg_type::ptr;
        ptr = (uintptr_t)value;
    }

    format_arg(const char* value)
    {
        type = format_arg_type::ptr;
        ptr = (uintptr_t)value;
    }
};

namespace openloco::ui::dropdown
{
    void add(int16_t index, string_id title);
    void add(int16_t index, string_id title, std::initializer_list<format_arg> l);
    void add(int16_t index, string_id title, format_arg l);
    int16_t get_highlighted_item();
    void set_item_disabled(int16_t index);
    void set_highlighted_item(int16_t index);
    void set_item_selected(int16_t index);

    void show(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags);
    void show_below(window* window, widget_index widgetIndex, int8_t count);
    void show_below(window* window, widget_index widgetIndex, int8_t count, int8_t height);
    void show_text(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags);
    void show_text_2(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags);
}
