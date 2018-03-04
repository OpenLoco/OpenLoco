#pragma once

#include "graphics/gfx.h"
#include "localisation/stringmgr.h"
#include "ui.h"
#include <cstdint>

namespace openloco::ui
{
    enum class window_type : uint8_t;
    enum class widget_type : uint8_t;
    struct window;

#pragma pack(push, 1)

    struct widget_t
    {
        widget_type type; // 0x00
        uint8_t colour;   // 0x01
        int16_t left;     // 0x02
        int16_t right;    // 0x04
        int16_t top;      // 0x06
        int16_t bottom;   // 0x08
        union
        {
            uint32_t image;
            string_id text;
            int32_t content;
        };
        string_id tooltip; // 0x0E
    };

    enum class widget_type : uint8_t
    {
        none = 0,
        panel = 1,
        frame = 2,
        wt_3,
        wt_4,
        wt_5,
        wt_6,
        wt_7,
        wt_8,
        wt_9,
        wt_10,
        wt_11,
        wt_12,
        wt_13,
        wt_14,
        wt_15,
        wt_16,
        wt_17,
        wt_18,
        wt_19,
        wt_20,
        wt_21,
        caption_22,
        caption_23,
        caption_24,
        caption_25,
        scrollview = 26,
        checkbox = 27,
        wt_28,
        wt_29,
        end = 30,
    };

    struct scroll_area_t
    {
        uint16_t flags;          // 0x00
        uint16_t h_left;         // 0x02
        uint16_t h_right;        // 0x04
        uint16_t h_thumb_left;   // 0x06
        uint16_t h_thumb_right;  // 0x08
        uint16_t v_top;          // 0x0A
        uint16_t v_bottom;       // 0x0C
        uint16_t v_thumb_top;    // 0x0E
        uint16_t v_thumb_bottom; // 0x10
    };

    namespace window_flags
    {
        constexpr uint32_t flag_0 = 1 << 0;
        constexpr uint32_t flag_1 = 1 << 1;
        constexpr uint32_t flag_4 = 1 << 4;
        constexpr uint32_t flag_5 = 1 << 5;
        constexpr uint32_t flag_6 = 1 << 6;
        constexpr uint32_t flag_7 = 1 << 7;
        constexpr uint32_t resizable = 1 << 9;
        constexpr uint32_t flag_11 = 1 << 11;
        constexpr uint32_t flag_12 = 1 << 12;
        constexpr uint32_t white_border_mask = (1 << 17) | (1 << 18);
    }

    struct window_event_list
    {
        union
        {
            void* events[29];
            struct
            {
                uint32_t on_close;
                uint32_t on_mouse_up;
                uint32_t on_resize;
                uint32_t event_03;
                uint32_t on_mouse_down;
                uint32_t on_dropdown;
                uint32_t event_06;
                uint32_t on_update;
                uint32_t event_08;
                uint32_t event_09;
                uint32_t event_10;
                uint32_t event_11;
                uint32_t event_12;
                uint32_t event_13;
                uint32_t tool_abort;
                uint32_t event_15;
                uint32_t get_scroll_size;
                uint32_t scroll_mouse_down;
                uint32_t event_18;
                uint32_t event_19;
                uint32_t text_input;
                uint32_t event_21;
                uint32_t event_22;
                uint32_t tooltip;
                uint32_t cursor;
                uint32_t event_25;
                void (*prepare_draw)(window*);
                void (*draw)(window*, gfx::drawpixelinfo_t*);
                uint32_t event_28;
            };
        };

        window_event_list()
        {
            // Set all events to a `ret` instruction
            for (auto& e : events)
            {
                e = (void*)0x0042A034;
            }
        }
    };

    struct viewport
    {
        int16_t width;       // 0x00
        int16_t height;      // 0x02
        int16_t x;           // 0x04
        int16_t y;           // 0x06
        int16_t view_x;      // 0x08
        int16_t view_y;      // 0x0A
        int16_t view_width;  // 0x0C
        int16_t view_height; // 0x0E
        uint8_t zoom;        // 0x10
        uint8_t pad_11;
        uint16_t var_12; // 0x12, maybe flags
    };

    struct window
    {
        union
        {
            uint8_t pad_all[0x88E];
            struct
            {
                window_event_list* event_handlers; // 0x00
                ui::viewport* viewport;            // 0x04
                uint8_t pad_08[0x04];              // 0x08
                uint64_t enabled_widgets;          // 0x0C
                uint64_t disabled_widgets;         // 0x14
                uint64_t activated_widgets;        // 0x1C
                uint8_t pad_24[0x2C - 0x24];
                widget_t* widgets;             // 0x2C
                uint16_t x;                    // 0x30
                uint16_t y;                    // 0x32
                uint16_t width;                // 0x34
                uint16_t height;               // 0x36
                uint16_t min_width;            // 0x38
                uint16_t max_width;            // 0x3a
                uint16_t min_height;           // 0x3c
                uint16_t max_height;           // 0x3e
                uint16_t number;               // 0x40
                uint32_t flags;                // 0x42
                scroll_area_t scroll_areas[3]; // 0x46
                uint8_t pad_7C[0x83E - 0x7C];
                uint16_t var_83E;
                uint8_t pad_840[0x846 - 0x840];
                uint16_t var_846;
                uint8_t pad_848[0x854 - 0x848];
                uint16_t var_854;
                uint16_t var_856;
                uint8_t pad_858[0x85A - 0x858];
                uint16_t var_85A;
                uint8_t pad_85C[0x870 - 0x85C];
                uint16_t var_870;
                uint16_t var_872;
                uint8_t pad_874[0x876 - 0x874];
                uint16_t var_876;
                uint16_t var_878;
                uint16_t var_87A;
                uint8_t pad_87C[0x882 - 0x87C];
                window_type type; // 0x882
                uint8_t pad_883[1];
                int8_t var_884;
                uint8_t pad_885[1];
                uint8_t colours[4]; // 0x886
            };
        };

        void invalidate();
        void sub_4CA17F();
        int16_t find_widget_at(int16_t xPos, int16_t yPos);
        void draw(openloco::gfx::drawpixelinfo_t* dpi);

        ui::cursor_id call_15(int16_t xPos, int16_t yPos, ui::cursor_id fallback, bool* out);             // 15
        bool call_tooltip(int16_t widget_index);                                                          // 23
        ui::cursor_id call_cursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback); // 24
        void call_prepare_draw();                                                                         // 26
        void call_draw(gfx::drawpixelinfo_t* dpi);
    };
#pragma pack(pop)
}
