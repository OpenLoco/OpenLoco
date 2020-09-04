#pragma once

#include "company.h"
#include "core/Optional.hpp"
#include "graphics/gfx.h"
#include "interop/interop.hpp"
#include "localisation/string_ids.h"
#include "localisation/stringmgr.h"
#include "map/tile.h"
#include "types.hpp"
#include "ui.h"
#include "ui/WindowType.h"
#include "viewport.hpp"
#include <algorithm>

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

        int16_t mid_x() const;
        int16_t mid_y() const;
        uint16_t width() const;
        uint16_t height() const;
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
        none = 0,
        horizontal = (1 << 0),
        vertical = (1 << 1),
        both = (1 << 0) | (1 << 1),
    };

    static constexpr widget_t makeWidget(gfx::point_t origin, gfx::ui_size_t size, widget_type type, uint8_t colour, uint32_t content = 0xFFFFFFFF, string_id tooltip = string_ids::null)
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

    constexpr widget_t makeRemapWidget(gfx::point_t origin, gfx::ui_size_t size, widget_type type, uint8_t colour, uint32_t content = 0xFFFFFFFF, string_id tooltip = string_ids::null)
    {
        widget_t out = makeWidget(origin, size, type, colour, content, tooltip);

        // TODO: implement this as a constant.
        out.content |= (1 << 29);

        return out;
    }

#define makeStepperWidgets(...)                 \
    makeWidget(__VA_ARGS__),                    \
        makeStepperDecreaseWidget(__VA_ARGS__), \
        makeStepperIncreaseWidget(__VA_ARGS__)

    [[maybe_unused]] static constexpr widget_t makeStepperDecreaseWidget(gfx::point_t origin, gfx::ui_size_t size, [[maybe_unused]] widget_type type, uint8_t colour, [[maybe_unused]] uint32_t content = 0xFFFFFFFF, [[maybe_unused]] string_id tooltip = string_ids::null)
    {
        const int16_t xPos = origin.x + size.width - 26;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 13;
        const uint16_t height = size.height - 2;

        return makeWidget({ xPos, yPos }, { width, height }, widget_type::wt_11, colour, string_ids::stepper_minus);
    }

    [[maybe_unused]] static constexpr widget_t makeStepperIncreaseWidget(gfx::point_t origin, gfx::ui_size_t size, [[maybe_unused]] widget_type type, uint8_t colour, [[maybe_unused]] uint32_t content = 0xFFFFFFFF, [[maybe_unused]] string_id tooltip = string_ids::null)
    {
        const int16_t xPos = origin.x + size.width - 13;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 12;
        const uint16_t height = size.height - 2;

        return makeWidget({ xPos, yPos }, { width, height }, widget_type::wt_11, colour, string_ids::stepper_plus);
    }

    constexpr widget_t makeTextWidget(gfx::point_t origin, gfx::ui_size_t size, widget_type type, uint8_t colour, string_id content, string_id tooltip = string_ids::null)
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

    constexpr widget_t widgetEnd()
    {
        widget_t out = {};
        out.type = widget_type::end;

        return out;
    }

    struct scroll_area_t
    {
        uint16_t flags;          // 0x00
        int16_t contentOffsetX;  // 0x02
        int16_t contentWidth;    // 0x04
        uint16_t h_thumb_left;   // 0x06
        uint16_t h_thumb_right;  // 0x08
        int16_t contentOffsetY;  // 0x0A
        int16_t contentHeight;   // 0x0C
        uint16_t v_thumb_top;    // 0x0E
        uint16_t v_thumb_bottom; // 0x10
    };

    namespace window_flags
    {
        constexpr uint32_t stick_to_back = 1 << 0;
        constexpr uint32_t stick_to_front = 1 << 1;
        constexpr uint32_t viewport_no_scrolling = 1 << 2;
        constexpr uint32_t scrolling_to_location = 1 << 3;
        constexpr uint32_t transparent = 1 << 4;
        constexpr uint32_t no_background = 1 << 5;
        constexpr uint32_t flag_6 = 1 << 6;
        constexpr uint32_t flag_7 = 1 << 7;
        constexpr uint32_t flag_8 = 1 << 8;
        constexpr uint32_t resizable = 1 << 9;
        constexpr uint32_t no_auto_close = 1 << 10;
        constexpr uint32_t flag_11 = 1 << 11;
        constexpr uint32_t flag_12 = 1 << 12;
        constexpr uint32_t flag_13 = 1 << 13;
        constexpr uint32_t not_scroll_view = 1 << 14;
        constexpr uint32_t flag_15 = 1 << 15;
        constexpr uint32_t flag_16 = 1 << 16;
        constexpr uint32_t white_border_one = (1 << 17);
        constexpr uint32_t white_border_mask = window_flags::white_border_one | (1 << 18);
        constexpr uint32_t flag_19 = 1 << 19;
        constexpr uint32_t flag_31 = 1 << 31;
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
                void (*event_03)(window*, widget_index); // mouse_over?
                void (*on_mouse_down)(window*, widget_index);
                void (*on_dropdown)(window*, widget_index, int16_t);
                void (*on_periodic_update)(window*);
                void (*on_update)(window*);
                void (*event_08)(window*);
                void (*event_09)(window*);
                void (*on_tool_update)(window&, const widget_index, const int16_t, const int16_t);
                void (*on_tool_down)(window&, const widget_index, const int16_t, const int16_t);
                void (*event_12)(window&, const widget_index);
                void (*event_13)(window&, const widget_index);
                void (*on_tool_abort)(window&, const widget_index);
                uint32_t event_15;
                void (*get_scroll_size)(window*, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
                void (*scroll_mouse_down)(ui::window*, int16_t x, int16_t y, uint8_t scroll_index);
                void (*scroll_mouse_drag)(ui::window*, int16_t x, int16_t y, uint8_t scroll_index);
                void (*scroll_mouse_over)(ui::window* window, int16_t x, int16_t y, uint8_t scroll_index);
                void (*text_input)(window*, widget_index, char*);
                void (*viewport_rotate)(window*);
                uint32_t event_22;
                void (*tooltip)(FormatArguments& args, window*, widget_index);
                ui::cursor_id (*cursor)(window*, int16_t, int16_t, int16_t, ui::cursor_id);
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

    struct SavedView
    {
        union
        {
            coord_t mapX;
            thing_id_t thingId;
        };
        union
        {
            coord_t mapY;
            uint16_t flags;
        };
        ZoomLevel zoomLevel;
        int8_t rotation;
        int16_t surfaceZ;

        SavedView() = default;

        SavedView(coord_t mapX, coord_t mapY, ZoomLevel zoomLevel, int8_t rotation, coord_t surfaceZ)
            : mapX(mapX)
            , mapY(mapY)
            , zoomLevel(zoomLevel)
            , rotation(rotation)
            , surfaceZ(surfaceZ){};

        SavedView(thing_id_t thingId, uint16_t flags, ZoomLevel zoomLevel, int8_t rotation, coord_t surfaceZ)
            : thingId(thingId)
            , flags(flags)
            , zoomLevel(zoomLevel)
            , rotation(rotation)
            , surfaceZ(surfaceZ){};

        bool isEmpty() const
        {
            return mapX == -1 && mapY == -1;
        }

        bool hasUnkFlag15() const
        {
            return (flags & (1 << 14)) != 0;
        }

        bool isThingView() const
        {
            return (flags & (1 << 15)) != 0;
        }

        openloco::map::map_pos3 getPos() const
        {
            if (isThingView())
                return {};

            return { mapX, static_cast<coord_t>(mapY & 0x3FFF), surfaceZ };
        }

        void clear()
        {
            mapX = -1;
            mapY = -1;
        }

        bool operator==(const SavedView& rhs) const
        {
            return mapX == rhs.mapX && mapY == rhs.mapY && zoomLevel == rhs.zoomLevel && rotation == rhs.rotation && surfaceZ == rhs.surfaceZ;
        }

        bool operator!=(const SavedView& rhs) const
        {
            return !(*this == rhs);
        }
    };

    struct window
    {
        window_event_list* event_handlers;                 // 0x00
        ui::viewport* viewports[2] = { nullptr, nullptr }; // 0x04
        uint64_t enabled_widgets = 0;                      // 0x0C
        uint64_t disabled_widgets = 0;                     // 0x14
        uint64_t activated_widgets = 0;                    // 0x1C
        uint64_t holdable_widgets = 0;                     // 0x24
        widget_t* widgets;                                 // 0x2C
        int16_t x;                                         // 0x30
        int16_t y;                                         // 0x32
        uint16_t width;                                    // 0x34
        uint16_t height;                                   // 0x36
        uint16_t min_width;                                // 0x38
        uint16_t max_width;                                // 0x3a
        uint16_t min_height;                               // 0x3c
        uint16_t max_height;                               // 0x3e
        window_number number = 0;                          // 0x40
        uint32_t flags;                                    // 0x42
        scroll_area_t scroll_areas[2];                     // 0x46
        int16_t row_info[1000];                            // 0x6A
        uint16_t row_count;                                // 0x83A
        uint16_t var_83C;
        uint16_t row_height;    // 0x83E
        int16_t row_hover = -1; // 0x840
        uint8_t pad_842[0x844 - 0x842];
        uint16_t sort_mode; // 0x844;
        uint16_t var_846 = 0;
        SavedView saved_view; // 0x848
        uint16_t var_850 = 0;
        uint16_t var_852 = 0;
        uint16_t var_854 = 0; // used to limit updates
        uint16_t var_856 = 0;
        uint16_t var_858 = 0;
        union
        {
            char* object; // 0x85A union
            struct
            {
                int16_t var_85A;
                int16_t var_85C;
            };
        };
        uint8_t pad_85E[0x870 - 0x85E];
        uint16_t current_tab = 0;                   // 0x870
        uint16_t frame_no = 0;                      // 0x872
        uint16_t current_secondary_tab = 0;         // 0x874
        viewport_config viewport_configurations[2]; // 0x876
        WindowType type;                            // 0x882
        uint8_t pad_883[1];
        company_id_t owner = company_id::null; // 0x884
        uint8_t var_885 = 0xFF;
        uint8_t colours[4]; // 0x886
        int16_t var_88A;
        int16_t var_88C;

        window(gfx::point_t position, gfx::ui_size_t size);

        constexpr bool setSize(gfx::ui_size_t minSize, gfx::ui_size_t maxSize)
        {
            bool hasResized = false;

            min_width = minSize.width;
            min_height = minSize.height;

            max_width = maxSize.width;
            max_height = maxSize.height;

            if (width < min_width)
            {
                width = min_width;
                invalidate();
                hasResized = true;
            }
            else if (width > max_width)
            {
                width = max_width;
                invalidate();
                hasResized = true;
            }

            if (height < min_height)
            {
                height = min_height;
                invalidate();
                hasResized = true;
            }
            else if (height > max_height)
            {
                height = max_height;
                invalidate();
                hasResized = true;
            }
            return hasResized;
        }

        constexpr void setSize(gfx::ui_size_t size)
        {
            setSize(size, size);
        }

        bool isVisible()
        {
            return true;
        }

        bool isTranslucent()
        {
            return (this->flags & window_flags::transparent) != 0;
        }

        bool isEnabled(int8_t widget_index);
        bool isDisabled(int8_t widget_index);
        bool isActivated(widget_index index);
        bool isHoldable(widget_index index);
        bool canResize();
        void capSize(int32_t minWidth, int32_t minHeight, int32_t maxWidth, int32_t maxHeight);
        void viewportsUpdatePosition();
        void invalidatePressedImageButtons();
        void invalidate();
        void updateScrollWidgets();
        void initScrollWidgets();
        int8_t getScrollDataIndex(widget_index index);
        void setDisabledWidgetsAndInvalidate(uint32_t _disabled_widgets);
        void drawViewports(gfx::drawpixelinfo_t* dpi);
        void viewportSetUndergroundFlag(bool underground, ui::viewport* vp);
        void viewportGetMapCoordsByCursor(int16_t* map_x, int16_t* map_y, int16_t* offset_x, int16_t* offset_y);
        void moveWindowToLocation(viewport_pos pos);
        void viewportCentreOnTile(const map::map_pos3& loc);
        void viewportCentreTileAroundCursor(int16_t map_x, int16_t map_y, int16_t offset_x, int16_t offset_y);
        void viewportZoomSet(int8_t zoomLevel, bool toCursor);
        void viewportZoomIn(bool toCursor);
        void viewportZoomOut(bool toCursor);
        void viewportRotateRight();
        void viewportRotateLeft();

        bool move(int16_t dx, int16_t dy);
        void moveInsideScreenEdges();
        bool moveToCentre();
        widget_index findWidgetAt(int16_t xPos, int16_t yPos);
        void draw(openloco::gfx::drawpixelinfo_t* dpi);

        void callClose();                                                                                // 0
        void callOnMouseUp(widget_index widgetIndex);                                                    // 1
        ui::window* callOnResize();                                                                      // 2
        void call_3(int8_t widget_index);                                                                // 3
        void callOnMouseDown(int8_t widget_index);                                                       // 4
        void callOnDropdown(widget_index widget_index, int16_t item_index);                              // 5
        void callOnPeriodicUpdate();                                                                     // 6
        void callUpdate();                                                                               // 7
        void call_8();                                                                                   // 8
        void call_9();                                                                                   // 9
        void callToolUpdate(int16_t widget_index, int16_t xPos, int16_t yPos);                           // 10
        void callToolDown(int16_t widget_index, int16_t xPos, int16_t yPos);                             // 11
        void call_12(const int16_t widget_index);                                                        // 12
        void call_13(const int16_t widget_index);                                                        // 13
        void callToolAbort(int16_t widget_index);                                                        // 14
        ui::cursor_id call_15(int16_t xPos, int16_t yPos, ui::cursor_id fallback, bool* out);            // 15
        void callGetScrollSize(uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);     // 16
        void callScrollMouseDown(int16_t x, int16_t y, uint8_t scroll_index);                            // 17
        void callScrollMouseDrag(int16_t x, int16_t y, uint8_t scroll_index);                            // 18
        void callScrollMouseOver(int16_t x, int16_t y, uint8_t scroll_index);                            // 19
        void callTextInput(widget_index caller, char* buffer);                                           // 20
        void callViewportRotate();                                                                       // 21
        bool callTooltip(int16_t widget_index);                                                          // 23
        ui::cursor_id callCursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, ui::cursor_id fallback); // 24
        void callOnMove(int16_t xPos, int16_t yPos);                                                     // 25
        void callPrepareDraw();                                                                          // 26
        void callDraw(gfx::drawpixelinfo_t* dpi);                                                        // 27
        void callDrawScroll(gfx::drawpixelinfo_t* dpi, uint32_t scrollIndex);                            // 28
    };
    static_assert(sizeof(window) == 0x88E);

    map::map_pos viewportCoordToMapCoord(int16_t x, int16_t y, int16_t z, int32_t rotation);
    std::optional<map::map_pos> screenGetMapXyWithZ(const xy32& mouse, const int16_t z);
#pragma pack(pop)
}
