#pragma once

#include "graphics/gfx.h"
#include "localisation/stringmgr.h"
#include "types.hpp"
#include "ui.h"
#include "ui/WindowType.h"

namespace openloco::ui
{
    using widget_index = int8_t;
    using window_number = uint16_t;
    enum class widget_type : uint8_t;
    struct window;

#pragma pack(push, 1)

    struct viewport;

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

        int16_t mid_x();
        uint16_t width();
        uint16_t height();
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
        viewport = 19,
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

    enum scrollbars : uint8_t
    {
        horizontal = (1 << 0),
        vertical = (1 << 1),
        both = (1 << 0) | (1 << 1),
    };

    static constexpr widget_t make_widget(gfx::point_t origin, gfx::ui_size_t size, widget_type type, uint8_t colour, uint32_t content = 0xFFFFFFFF, string_id tooltip = string_ids::null)
    {
        widget_t out = {};
        out.left = origin.x;
        out.right = origin.x + size.width - 1;
        out.top = origin.y;
        out.bottom = origin.y + size.height - 1;
        out.type = type;
        out.colour = colour;
        out.content = content;
        out.tooltip = tooltip;

        return out;
    }

    constexpr widget_t make_remap_widget(gfx::point_t origin, gfx::ui_size_t size, widget_type type, uint8_t colour, uint32_t content = 0xFFFFFFFF, string_id tooltip = string_ids::null)
    {
        widget_t out = make_widget(origin, size, type, colour, content, tooltip);

        // TODO: implement this as a constant.
        out.content |= (1 << 29);

        return out;
    }

    constexpr widget_t make_text_widget(gfx::point_t origin, gfx::ui_size_t size, widget_type type, uint8_t colour, string_id content, string_id tooltip = string_ids::null)
    {
        widget_t out = {};
        out.left = origin.x;
        out.right = origin.x + size.width - 1;
        out.top = origin.y;
        out.bottom = origin.y + size.height - 1;
        out.type = type;
        out.colour = colour;
        out.text = content;
        out.tooltip = tooltip;

        return out;
    }

    constexpr widget_t widget_end()
    {
        widget_t out = {};
        out.type = widget_type::end;

        return out;
    }

    struct scroll_area_t
    {
        uint16_t flags;          // 0x00
        uint16_t h_left;         // 0x02
        int16_t h_right;         // 0x04
        uint16_t h_thumb_left;   // 0x06
        uint16_t h_thumb_right;  // 0x08
        uint16_t v_top;          // 0x0A
        int16_t v_bottom;        // 0x0C
        uint16_t v_thumb_top;    // 0x0E
        uint16_t v_thumb_bottom; // 0x10
    };

    namespace window_flags
    {
        constexpr uint32_t stick_to_back = 1 << 0;
        constexpr uint32_t stick_to_front = 1 << 1;
        constexpr uint32_t scrolling_to_location = 1 << 3;
        constexpr uint32_t transparent = 1 << 4;
        constexpr uint32_t no_background = 1 << 5;
        constexpr uint32_t flag_6 = 1 << 6;
        constexpr uint32_t flag_7 = 1 << 7;
        constexpr uint32_t resizable = 1 << 9;
        constexpr uint32_t flag_11 = 1 << 11;
        constexpr uint32_t flag_12 = 1 << 12;
        constexpr uint32_t flag_15 = 1 << 15;
        constexpr uint32_t flag_16 = 1 << 16;
        constexpr uint32_t white_border_mask = (1 << 17) | (1 << 18);
    }

    struct window_event_list
    {
        union
        {
            void* events[29];
            struct
            {
                void (*on_close)(window*);
                void (*on_mouse_up)(window*, widget_index);
                void (*on_resize)(window*);
                uint32_t event_03;
                void (*on_mouse_down)(window*, widget_index);
                void (*on_dropdown)(window*, widget_index, int16_t);
                uint32_t event_06;
                void (*on_update)(window*);
                uint32_t event_08;
                uint32_t event_09;
                uint32_t event_10;
                uint32_t on_tool_down;
                uint32_t event_12;
                uint32_t event_13;
                uint32_t tool_abort;
                uint32_t event_15;
                void (*get_scroll_size)(window*, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
                uint32_t scroll_mouse_down;
                uint32_t event_18;
                uint32_t scroll_mouse_over;
                void (*text_input)(window*, widget_index, char*);
                uint32_t viewport_rotate;
                uint32_t event_22;
                void (*tooltip)(window*, widget_index);
                ui::cursor_id (*cursor)(int16_t, int16_t, int16_t, ui::cursor_id);
                uint32_t on_move;
                void (*prepare_draw)(window*);
                void (*draw)(window*, gfx::drawpixelinfo_t*);
                void (*draw_scroll)(window*, gfx::drawpixelinfo_t*, uint32_t scrollIndex);
            };
        };

        window_event_list()
        {
            // Set all events to a `ret` instruction
            std::fill_n(events, 29, (void*)0x0042A034);
        }
    };

    struct viewport_pos
    {
        int16_t x{};
        int16_t y{};

        viewport_pos()
            : viewport_pos(0, 0)
        {
        }
        viewport_pos(int16_t _x, int16_t _y)
            : x(_x)
            , y(_y)
        {
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

        constexpr bool contains(const viewport_pos& vpos)
        {
            return (vpos.y >= view_y && vpos.y < view_y + view_height && vpos.x >= view_x && vpos.x < view_x + view_width);
        }

        /**
         * Maps a 2D viewport position to a UI (screen) position.
         */
        xy32 map_to_ui(const viewport_pos& vpos)
        {
            auto uiX = x + ((vpos.x - view_x) >> zoom);
            auto uiY = y + ((vpos.y - view_y) >> zoom);
            return { uiX, uiY };
        }

        static viewport_pos map_from_3d(loc16 loc, int32_t rotation);
    };

    struct viewport_config
    {
        uint16_t viewport_target_sprite; // 0x0
        uint16_t saved_view_x;           // 0x2
        uint16_t saved_view_y;           // 0x4
    };

    struct window
    {
        union
        {
            uint8_t pad_all[0x88E];
            struct
            {
                window_event_list* event_handlers; // 0x00
                ui::viewport* viewports[2];        // 0x04
                uint64_t enabled_widgets;          // 0x0C
                uint64_t disabled_widgets;         // 0x14
                uint64_t activated_widgets;        // 0x1C
                uint64_t holdable_widgets;         // 0x24
                widget_t* widgets;                 // 0x2C
                int16_t x;                         // 0x30
                int16_t y;                         // 0x32
                uint16_t width;                    // 0x34
                uint16_t height;                   // 0x36
                uint16_t min_width;                // 0x38
                uint16_t max_width;                // 0x3a
                uint16_t min_height;               // 0x3c
                uint16_t max_height;               // 0x3e
                window_number number;              // 0x40
                uint32_t flags;                    // 0x42
                scroll_area_t scroll_areas[3];     // 0x46
                uint8_t pad_7C[0x83E - 0x7C];
                uint16_t var_83E;
                uint16_t var_840;
                uint8_t pad_842[0x846 - 0x842];
                uint16_t var_846;
                uint8_t pad_848[0x854 - 0x848];
                uint16_t var_854;
                uint16_t var_856;
                uint8_t pad_858[0x85A - 0x858];
                uint16_t var_85A;
                uint8_t pad_85C[0x870 - 0x85C];
                uint16_t current_tab; // 0x870
                uint16_t frame_no;    // 0x872
                uint8_t pad_874[0x876 - 0x874];
                viewport_config viewport_configurations[2]; // 0x876
                WindowType type;                            // 0x882
                uint8_t pad_883[1];
                int8_t var_884;
                uint8_t pad_885[1];
                uint8_t colours[4]; // 0x886
                int16_t var_88A;
                int16_t var_88C;
            };
        };

        constexpr void set_size(gfx::ui_size_t size)
        {
            this->min_width = size.width;
            this->min_height = size.height;

            this->max_width = size.width;
            this->max_height = size.height;

            this->width = size.width;
            this->height = size.height;
        }

        bool is_enabled(int8_t widget_index);
        bool is_disabled(int8_t widget_index);
        bool is_activated(widget_index index);
        bool is_holdable(widget_index index);
        bool can_resize();
        void cap_size(int32_t minWidth, int32_t minHeight, int32_t maxWidth, int32_t maxHeight);
        void viewports_update_position();
        void invalidate_pressed_image_buttons();
        void invalidate();
        void update_scroll_widgets();
        void init_scroll_widgets();
        int8_t get_scroll_data_index(widget_index index);
        void viewport_get_map_coords_by_cursor(int16_t* map_x, int16_t* map_y, int16_t* offset_x, int16_t* offset_y);
        void viewport_centre_tile_around_cursor(int16_t map_x, int16_t map_y, int16_t offset_x, int16_t offset_y);
        void viewport_zoom_set(int8_t zoomLevel, bool toCursor);
        void viewport_zoom_in(bool toCursor);
        void viewport_zoom_out(bool toCursor);
        void viewport_rotate_right();
        void viewport_rotate_left();

        bool move(int16_t dx, int16_t dy);
        widget_index find_widget_at(int16_t xPos, int16_t yPos);
        void draw(openloco::gfx::drawpixelinfo_t* dpi);

        void call_close();                                                                                // 0
        void call_on_mouse_up(widget_index widgetIndex);                                                  // 1
        ui::window* call_on_resize();                                                                     // 2
        void call_3(int8_t widget_index);                                                                 // 3
        void call_on_mouse_down(int8_t widget_index);                                                     // 4
        void call_on_dropdown(widget_index widget_index, int16_t item_index);                             // 5
        void call_update();                                                                               // 7
        void call_tool_down(int16_t widget_index, int16_t xPos, int16_t yPos);                            // 11
        ui::cursor_id call_15(int16_t xPos, int16_t yPos, ui::cursor_id fallback, bool* out);             // 15
        void call_get_scroll_size(uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);   // 16
        void call_scroll_mouse_down(int16_t x, int16_t y, uint8_t scroll_index);                          // 17
        void call_scroll_mouse_over(int16_t x, int16_t y, uint8_t scroll_index);                          // 19
        void call_text_input(widget_index caller, char* buffer);                                          // 20
        void call_viewport_rotate();                                                                      // 21
        bool call_tooltip(int16_t widget_index);                                                          // 23
        ui::cursor_id call_cursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback); // 24
        void call_on_move(int16_t xPos, int16_t yPos);                                                    // 25
        void call_prepare_draw();                                                                         // 26
        void call_draw(gfx::drawpixelinfo_t* dpi);                                                        // 27
        void call_draw_scroll(gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);                           // 28
    };
#pragma pack(pop)
}
