#include "../graphics/colours.h"
#include "../localisation/stringmgr.h"

namespace openloco::ui::dropdown
{
    void add(int16_t index, string_id title, ...);
    void set_selection(int16_t index);
    void show_text(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags);
    void show_text_2(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, int8_t count, uint8_t flags);
}
