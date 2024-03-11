#pragma once

#include "Graphics/Colour.h"
#include "Localisation/StringManager.h"
#include "Window.h"
#include "World/Company.h"
#include <cstdlib>
#include <optional>

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

namespace OpenLoco::Ui::Dropdown
{
    void addSeparator(size_t index);
    void add(size_t index, StringId title);
    void add(size_t index, StringId title, std::initializer_list<format_arg> l);
    void add(size_t index, StringId title, format_arg l);
    void add(size_t index, StringId title, const FormatArguments& fArgs);
    int16_t getHighlightedItem();
    void setItemDisabled(size_t index);
    void setHighlightedItem(size_t index);
    void setItemSelected(size_t index);

    void show(int16_t x, int16_t y, int16_t width, int16_t height, AdvancedColour colour, size_t count, uint8_t itemHeight, uint8_t flags);
    void show(int16_t x, int16_t y, int16_t width, int16_t height, AdvancedColour colour, size_t count, uint8_t flags);
    void showImage(int16_t x, int16_t y, int16_t width, int16_t height, int16_t heightOffset, AdvancedColour colour, uint8_t columnCount, uint8_t count, uint8_t flags = 0);
    void showBelow(Window* window, WidgetIndex_t widgetIndex, size_t count, uint8_t flags);
    void showBelow(Window* window, WidgetIndex_t widgetIndex, size_t count, int8_t itemHeight, uint8_t flags);
    void showText(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, AdvancedColour colour, size_t count, uint8_t flags);
    void showText(int16_t x, int16_t y, int16_t width, int16_t height, AdvancedColour colour, size_t count, uint8_t flags);
    void showText2(int16_t x, int16_t y, int16_t width, int16_t height, uint8_t itemHeight, AdvancedColour colour, size_t count, uint8_t flags);
    void showText2(int16_t x, int16_t y, int16_t width, int16_t height, AdvancedColour colour, size_t count, uint8_t flags);
    void showColour(const Window* window, const Widget* widget, uint32_t availableColours, Colour selectedColour, AdvancedColour dropdownColour);

    void populateCompanySelect(Window* window, Widget* widget);
    CompanyId getCompanyIdFromSelection(int16_t itemIndex);
    uint16_t getItemArgument(const uint8_t index, const uint8_t argument);
    uint16_t getItemsPerRow(uint8_t itemCount);

    using DropdownItemId = int32_t;

    class Builder
    {
    private:
        Window* _window{};
        WidgetIndex_t _widgetIndex{};
        std::vector<std::tuple<std::optional<DropdownItemId>, StringId>> _items;
        std::optional<DropdownItemId> _highlightedId;

    public:
        Builder& below(Window& window, WidgetIndex_t widgetIndex);
        Builder& item(DropdownItemId id, StringId text);
        Builder& separator();
        Builder& highlight(DropdownItemId id);
        void show();

        template<typename T>
        Builder& item(T id, StringId text)
        {
            item(static_cast<int32_t>(id), text);
            return *this;
        }

        template<typename T>
        Builder& highlight(T id)
        {
            highlight(static_cast<int32_t>(id));
            return *this;
        }
    };

    Builder create();
    std::optional<DropdownItemId> getSelectedItem(int32_t index);

    template<typename T>
    std::optional<T> getSelectedItem(int32_t index)
    {
        auto result = getSelectedItem(index);
        if (result)
            return static_cast<T>(*result);
        return std::nullopt;
    }
}
