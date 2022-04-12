#pragma once

#include "Company.h"
#include "Core/Optional.hpp"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Interop/Interop.hpp"
#include "Map/Tile.h"
#include "Objects/Object.h"
#include "Types.hpp"
#include "Ui.h"
#include "Ui/WindowType.h"
#include "Viewport.hpp"
#include "ZoomLevel.hpp"
#include <algorithm>

namespace OpenLoco::Ui
{
    using WidgetIndex_t = int8_t;
    using WindowNumber_t = uint16_t;
    enum class WidgetType : uint8_t;
    struct Window;
    struct Widget;

#pragma pack(push, 1)

    struct Viewport;

    enum class WidgetType : uint8_t
    {
        none = 0,
        panel = 1,
        frame = 2,
        wt_3,
        wt_4,
        wt_5,
        wt_6,
        toolbarTab = 7,
        tab = 8,
        buttonWithImage = 9,
        buttonWithColour = 10,
        button = 11,
        wt_12,
        wt_13,
        buttonTableHeader = 14,
        wt_15,
        groupbox = 16,
        textbox = 17,
        combobox = 18,
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

    enum class WindowColour : uint8_t
    {
        primary,
        secondary,
        tertiary,
        quaternary,
        count
    };

    namespace Scrollbars
    {
        constexpr uint8_t none = 0;
        constexpr uint8_t horizontal = (1 << 0);
        constexpr uint8_t vertical = (1 << 1);
        constexpr uint8_t both = horizontal | vertical;
    }

    struct ScrollArea
    {
        uint16_t flags;         // 0x00
        int16_t contentOffsetX; // 0x02
        int16_t contentWidth;   // 0x04
        uint16_t hThumbLeft;    // 0x06
        uint16_t hThumbRight;   // 0x08
        int16_t contentOffsetY; // 0x0A
        int16_t contentHeight;  // 0x0C
        uint16_t vThumbTop;     // 0x0E
        uint16_t vThumbBottom;  // 0x10
    };

    namespace WindowFlags
    {
        constexpr uint32_t stickToBack = 1 << 0;
        constexpr uint32_t stickToFront = 1 << 1;
        constexpr uint32_t viewportNoScrolling = 1 << 2;
        constexpr uint32_t scrollingToLocation = 1 << 3;
        constexpr uint32_t transparent = 1 << 4;
        constexpr uint32_t noBackground = 1 << 5;
        constexpr uint32_t flag_6 = 1 << 6;
        constexpr uint32_t flag_7 = 1 << 7;
        constexpr uint32_t flag_8 = 1 << 8;
        constexpr uint32_t resizable = 1 << 9;
        constexpr uint32_t noAutoClose = 1 << 10;
        constexpr uint32_t flag_11 = 1 << 11;
        constexpr uint32_t flag_12 = 1 << 12;
        constexpr uint32_t openQuietly = 1 << 13;
        constexpr uint32_t notScrollView = 1 << 14;
        constexpr uint32_t flag_15 = 1 << 15;
        constexpr uint32_t flag_16 = 1 << 16;
        constexpr uint32_t whiteBorderOne = (1 << 17);
        constexpr uint32_t whiteBorderMask = WindowFlags::whiteBorderOne | (1 << 18);
        constexpr uint32_t flag_19 = 1 << 19;
        constexpr uint32_t flag_31 = 1 << 31;
    }

    struct WindowEventList
    {
        union
        {
            void* events[29];
            struct
            {
                void (*onClose)(Window*);
                void (*onMouseUp)(Window*, WidgetIndex_t);
                void (*onResize)(Window*);
                void (*event_03)(Window*, WidgetIndex_t); // mouse_over?
                void (*onMouseDown)(Window*, WidgetIndex_t);
                void (*onDropdown)(Window*, WidgetIndex_t, int16_t);
                void (*onPeriodicUpdate)(Window*);
                void (*onUpdate)(Window*);
                void (*event_08)(Window*);
                void (*event_09)(Window*);
                void (*onToolUpdate)(Window&, const WidgetIndex_t, const int16_t, const int16_t);
                void (*onToolDown)(Window&, const WidgetIndex_t, const int16_t, const int16_t);
                void (*toolDragContinue)(Window&, const WidgetIndex_t, const int16_t, const int16_t);
                void (*toolDragEnd)(Window&, const WidgetIndex_t);
                void (*onToolAbort)(Window&, const WidgetIndex_t);
                Ui::CursorId (*event_15)(Window&, const int16_t x, const int16_t y, const Ui::CursorId, bool&);
                void (*getScrollSize)(Window*, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
                void (*scrollMouseDown)(Ui::Window*, int16_t x, int16_t y, uint8_t scrollIndex);
                void (*scrollMouseDrag)(Ui::Window*, int16_t x, int16_t y, uint8_t scrollIndex);
                void (*scrollMouseOver)(Ui::Window* window, int16_t x, int16_t y, uint8_t scrollIndex);
                void (*textInput)(Window*, WidgetIndex_t, const char*);
                void (*viewportRotate)(Window*);
                uint32_t event_22;
                std::optional<FormatArguments> (*tooltip)(Window*, WidgetIndex_t);
                Ui::CursorId (*cursor)(Window*, int16_t, int16_t, int16_t, Ui::CursorId);
                void (*onMove)(Window&, const int16_t x, const int16_t y);
                void (*prepareDraw)(Window*);
                void (*draw)(Window*, Gfx::Context*);
                void (*drawScroll)(Window&, Gfx::Context&, const uint32_t scrollIndex);
            };
        };

        WindowEventList()
        {
            // Set all events to a `ret` instruction
            std::fill_n(events, 29, (void*)0x0042A034);
        }
    };

    struct SavedViewSimple
    {
        coord_t viewX;
        coord_t viewY;
        ZoomLevel zoomLevel;
        int8_t rotation;
    };

    struct SavedView
    {
        union
        {
            coord_t mapX;
            EntityId thingId;
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

        SavedView(EntityId thingId, uint16_t flags, ZoomLevel zoomLevel, int8_t rotation, coord_t surfaceZ)
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

        Map::Pos3 getPos() const
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

    struct Window
    {
        static constexpr size_t kMaxScrollAreas = 2;

        WindowEventList* eventHandlers;                    // 0x00
        Ui::Viewport* viewports[2] = { nullptr, nullptr }; // 0x04
        uint64_t enabledWidgets = 0;                       // 0x0C
        uint64_t disabledWidgets = 0;                      // 0x14
        uint64_t activatedWidgets = 0;                     // 0x1C
        uint64_t holdableWidgets = 0;                      // 0x24
        Widget* widgets;                                   // 0x2C
        int16_t x;                                         // 0x30
        int16_t y;                                         // 0x32
        uint16_t width;                                    // 0x34
        uint16_t height;                                   // 0x36
        uint16_t minWidth;                                 // 0x38
        uint16_t maxWidth;                                 // 0x3a
        uint16_t minHeight;                                // 0x3c
        uint16_t maxHeight;                                // 0x3e
        WindowNumber_t number = 0;                         // 0x40
        uint32_t flags;                                    // 0x42
        ScrollArea scrollAreas[kMaxScrollAreas];           // 0x46
        int16_t rowInfo[1000];                             // 0x6A
        uint16_t rowCount;                                 // 0x83A
        uint16_t var_83C;
        uint16_t rowHeight;    // 0x83E
        int16_t rowHover = -1; // 0x840
        int16_t var_842;       // 0x842
        uint16_t sortMode;     // 0x844;
        uint16_t var_846 = 0;
        SavedView savedView; // 0x848
        uint16_t var_850 = 0;
        uint16_t var_852 = 0;
        uint16_t var_854 = 0; // used to limit updates
        uint16_t var_856 = 0;
        uint16_t var_858 = 0;
        union
        {
            std::byte* object; // 0x85A union
            struct
            {
                int16_t var_85A;
                int16_t var_85C;
            };
            uintptr_t info;
        };
        uint8_t pad_85E[0x870 - 0x85E];
        uint16_t currentTab = 0;                  // 0x870
        uint16_t frame_no = 0;                    // 0x872
        uint16_t currentSecondaryTab = 0;         // 0x874
        ViewportConfig viewportConfigurations[2]; // 0x876
        WindowType type;                          // 0x882
        uint8_t pad_883[1];
        CompanyId owner = CompanyId::null; // 0x884
        uint8_t var_885 = 0xFF;
        AdvancedColour colours[enumValue(WindowColour::count)]; // 0x886
        int16_t var_88A;
        int16_t var_88C;

        Window(Ui::Point position, Ui::Size size);

        constexpr bool setSize(Ui::Size minSize, Ui::Size maxSize)
        {
            bool hasResized = false;

            minWidth = minSize.width;
            minHeight = minSize.height;

            maxWidth = maxSize.width;
            maxHeight = maxSize.height;

            if (width < minWidth)
            {
                width = minWidth;
                invalidate();
                hasResized = true;
            }
            else if (width > maxWidth)
            {
                width = maxWidth;
                invalidate();
                hasResized = true;
            }

            if (height < minHeight)
            {
                height = minHeight;
                invalidate();
                hasResized = true;
            }
            else if (height > maxHeight)
            {
                height = maxHeight;
                invalidate();
                hasResized = true;
            }
            return hasResized;
        }

        constexpr void setSize(Ui::Size size)
        {
            setSize(size, size);
        }

        constexpr AdvancedColour getColour(WindowColour index) const
        {
            if (index >= WindowColour::primary && index < WindowColour::count)
            {
                return colours[enumValue(index)];
            }
            return colours[enumValue(WindowColour::primary)];
        }
        constexpr void setColour(WindowColour index, AdvancedColour colour)
        {
            if (index >= WindowColour::primary && index < WindowColour::count)
            {
                colours[enumValue(index)] = colour;
            }
        }

        bool isVisible()
        {
            return true;
        }

        bool isTranslucent()
        {
            return (this->flags & WindowFlags::transparent) != 0;
        }

        bool isEnabled(int8_t widget_index);
        bool isDisabled(int8_t widget_index);
        bool isActivated(WidgetIndex_t index);
        bool isHoldable(WidgetIndex_t index);
        bool canResize();
        void capSize(int32_t minWidth, int32_t minHeight, int32_t maxWidth, int32_t maxHeight);
        void viewportsUpdatePosition();
        void invalidatePressedImageButtons();
        void invalidate();
        void updateScrollWidgets();
        void initScrollWidgets();
        int8_t getScrollDataIndex(WidgetIndex_t index);
        void setDisabledWidgetsAndInvalidate(uint32_t _disabled_widgets);
        void drawViewports(Gfx::Context* context);
        void viewportCentreMain();
        void viewportSetUndergroundFlag(bool underground, Ui::Viewport* vp);
        void viewportGetMapCoordsByCursor(int16_t* map_x, int16_t* map_y, int16_t* offset_x, int16_t* offset_y);
        void moveWindowToLocation(viewport_pos pos);
        void viewportCentreOnTile(const Map::Pos3& loc);
        void viewportCentreTileAroundCursor(int16_t map_x, int16_t map_y, int16_t offset_x, int16_t offset_y);
        void viewportFocusOnEntity(EntityId targetEntity);
        bool viewportIsFocusedOnEntity() const;
        void viewportUnfocusFromEntity();
        void viewportZoomSet(int8_t zoomLevel, bool toCursor);
        void viewportZoomIn(bool toCursor);
        void viewportZoomOut(bool toCursor);
        void viewportRotateRight();
        void viewportRotateLeft();
        void viewportRemove(const uint8_t viewportId);
        void viewportFromSavedView(const SavedViewSimple& savedView);

        bool move(int16_t dx, int16_t dy);
        void moveInsideScreenEdges();
        bool moveToCentre();
        WidgetIndex_t findWidgetAt(int16_t xPos, int16_t yPos);
        void draw(OpenLoco::Gfx::Context* context);

        void callClose();                                                                              // 0
        void callOnMouseUp(WidgetIndex_t widgetIndex);                                                 // 1
        Ui::Window* callOnResize();                                                                    // 2
        void call_3(int8_t widget_index);                                                              // 3
        void callOnMouseDown(int8_t widget_index);                                                     // 4
        void callOnDropdown(WidgetIndex_t widget_index, int16_t item_index);                           // 5
        void callOnPeriodicUpdate();                                                                   // 6
        void callUpdate();                                                                             // 7
        void call_8();                                                                                 // 8
        void call_9();                                                                                 // 9
        void callToolUpdate(int16_t widget_index, int16_t xPos, int16_t yPos);                         // 10
        void callToolDown(int16_t widget_index, int16_t xPos, int16_t yPos);                           // 11
        void callToolDragContinue(const int16_t widget_index, const int16_t xPos, const int16_t yPos); // 12
        void callToolDragEnd(const int16_t widget_index);                                              // 13
        void callToolAbort(int16_t widget_index);                                                      // 14
        Ui::CursorId call_15(int16_t xPos, int16_t yPos, Ui::CursorId fallback, bool* out);            // 15
        void callGetScrollSize(uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);   // 16
        void callScrollMouseDown(int16_t x, int16_t y, uint8_t scroll_index);                          // 17
        void callScrollMouseDrag(int16_t x, int16_t y, uint8_t scroll_index);                          // 18
        void callScrollMouseOver(int16_t x, int16_t y, uint8_t scroll_index);                          // 19
        void callTextInput(WidgetIndex_t caller, const char* buffer);                                  // 20
        void callViewportRotate();                                                                     // 21
        std::optional<FormatArguments> callTooltip(int16_t widget_index);                              // 23
        Ui::CursorId callCursor(int16_t widgetIdx, int16_t xPos, int16_t yPos, Ui::CursorId fallback); // 24
        void callOnMove(int16_t xPos, int16_t yPos);                                                   // 25
        void callPrepareDraw();                                                                        // 26
        void callDraw(Gfx::Context* context);                                                          // 27
        void callDrawScroll(Gfx::Context* context, uint32_t scrollIndex);                              // 28

        WidgetIndex_t firstActivatedWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex);
        WidgetIndex_t prevAvailableWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex);
        WidgetIndex_t nextAvailableWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex);
    };
    assert_struct_size(Window, 0x88E);

    Map::Pos2 viewportCoordToMapCoord(int16_t x, int16_t y, int16_t z, int32_t rotation);
    std::optional<Map::Pos2> screenGetMapXyWithZ(const Point& mouse, const int16_t z);
#pragma pack(pop)
}
