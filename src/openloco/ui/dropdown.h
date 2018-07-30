#include "../graphics/colours.h"
#include "../localisation/stringmgr.h"
#include <cstdlib>
#include <vector>

struct format_arg
{
    uint8_t type;
    union
    {
        uint32_t u32;
        uint16_t u16;
        int32_t s32;
        void* ptr;
    };

    format_arg(uint16_t value)
    {
        type = 1;
        u32 = value;
    }

    format_arg(char* value)
    {
        type = 2;
        ptr = value;
    }

    void print()
    {
        switch (type)
        {
            case 1:
                printf("[string_id %d '%s']\n", u32, openloco::stringmgr::get_string(u32));
                break;

            case 3:
                printf("[s32 %d]\n", s32);
                break;

            case 2:
                printf("[string '%s']", ptr);
                break;
        }
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
