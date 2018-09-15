#include "../graphics/colours.h"
#include "../localisation/stringmgr.h"
#include <cstdlib>
#include <vector>

enum format_arg_type
{
    u16,
    ptr,
};

struct format_arg
{

    format_arg_type type;
    union
    {
        uint16_t u16;
        uintptr_t ptr;
    };

    format_arg(uint16_t value)
    {
        type = format_arg_type::u16;
        u16 = value;
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
    void set_selection(int16_t index);

    void show(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags);
    void show_text(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags);
    void show_text_2(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags);
}
