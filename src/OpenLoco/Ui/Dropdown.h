#include "../Company.h"
#include "../Graphics/Colour.h"
#include "../Localisation/StringManager.h"
#include "../Window.h"
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

namespace OpenLoco
{
    class FormatArguments;
}

namespace OpenLoco::Ui::dropdown
{
    void add(size_t index, string_id title);
    void add(size_t index, string_id title, std::initializer_list<format_arg> l);
    void add(size_t index, string_id title, format_arg l);
    void add(size_t index, string_id title, FormatArguments& fArgs);
    int16_t getHighlightedItem();
    void setItemDisabled(size_t index);
    void setHighlightedItem(size_t index);
    void setItemSelected(size_t index);

    void show(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t itemHeight, uint8_t flags);
    void show(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t flags);
    void showImage(int16_t x, int16_t y, int16_t width, int16_t height, int16_t heightOffset, colour_t colour, uint8_t columnCount, uint8_t count);
    void showBelow(window* window, widget_index widgetIndex, size_t count, uint8_t flags);
    void showBelow(window* window, widget_index widgetIndex, size_t count, int8_t itemHeight, uint8_t flags);
    void showText(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, colour_t colour, size_t count, uint8_t flags);
    void showText(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t flags);
    void showText2(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, colour_t colour, size_t count, uint8_t flags);
    void showText2(int16_t x, int16_t y, int16_t width, int16_t height, colour_t colour, size_t count, uint8_t flags);

    void populateCompanySelect(window* window, widget_t* widget);
    company_id_t getCompanyIdFromSelection(int16_t itemIndex);
    uint16_t getItemArgument(const uint8_t index, const uint8_t argument);
    uint16_t getItemsPerRow(uint8_t itemCount);
}
