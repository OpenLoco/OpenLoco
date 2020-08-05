#include "../interop/interop.hpp"
#include "../window.h"

using namespace openloco::interop;

namespace openloco::ui::scrollview
{
    enum class scroll_part
    {
        none = -1,
        view = 0,
        hscrollbar_button_left = 1,
        hscrollbar_button_right = 2,
        hscrollbar_track_left = 3,
        hscrollbar_track_right = 4,
        hscrollbar_thumb = 5,
        vscrollbar_button_top = 6,
        vscrollbar_button_bottom = 7,
        vscrollbar_track_top = 8,
        vscrollbar_track_bottom = 9,
        vscrollbar_thumb = 10,
    };

    namespace scroll_flags
    {
        constexpr uint16_t HSCROLLBAR_VISIBLE = 1 << 0;
        constexpr uint16_t HSCROLLBAR_THUMB_PRESSED = 1 << 1;
        constexpr uint16_t HSCROLLBAR_LEFT_PRESSED = 1 << 2;
        constexpr uint16_t HSCROLLBAR_RIGHT_PRESSED = 1 << 3;
        constexpr uint16_t VSCROLLBAR_VISIBLE = 1 << 4;
        constexpr uint16_t VSCROLLBAR_THUMB_PRESSED = 1 << 5;
        constexpr uint16_t VSCROLLBAR_UP_PRESSED = 1 << 6;
        constexpr uint16_t VSCROLLBAR_DOWN_PRESSED = 1 << 7;
    }

    constexpr uint8_t SCROLLBAR_WIDTH = 10;
    constexpr uint8_t SCROLLBAR_BUTTON_CLICK_STEP = 3;

    void get_part(
        ui::window* window,
        ui::widget_t* widget,
        int16_t x,
        int16_t y,
        int16_t* output_x,
        int16_t* output_y,
        scroll_part* output_scroll_area,
        int32_t* scroll_id);
    void update_thumbs(window* window, widget_index widgetIndex);
    void scrollLeftBegin(const int16_t x, const int16_t y, ui::window* const w, ui::widget_t* const widget, const widget_index widgetIndex);
    void scrollModalRight(const int16_t x, const int16_t y, ui::window* const w, ui::widget_t* const widget, const widget_index widgetIndex);
}
