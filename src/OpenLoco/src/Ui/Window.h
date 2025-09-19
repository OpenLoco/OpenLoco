#pragma once

#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Map/Tile.h"
#include "Objects/Object.h"
#include "Types.hpp"
#include "Ui.h"
#include "Ui/ScrollFlags.hpp"
#include "Ui/Widget.h"
#include "Ui/WindowType.h"
#include "Viewport.hpp"
#include "World/Company.h"
#include "ZoomLevel.hpp"
#include <OpenLoco/Core/EnumFlags.hpp>
#include <OpenLoco/Interop/Interop.hpp>
#include <algorithm>
#include <optional>
#include <sfl/small_vector.hpp>

namespace OpenLoco::Gfx
{
    class DrawingContext;
}

namespace OpenLoco::Input
{
    enum class Shortcut : uint16_t;
}

namespace OpenLoco::Ui
{
    using WindowNumber_t = uint16_t;

    struct Window;
    struct Viewport;

    enum class WindowColour : uint8_t
    {
        primary,
        secondary,
        tertiary,
        quaternary,
        count
    };

    enum class WindowFlags : uint32_t
    {
        none = 0U,
        stickToBack = 1U << 0,
        stickToFront = 1U << 1,
        viewportNoScrolling = 1U << 2,
        scrollingToLocation = 1U << 3,
        transparent = 1U << 4,
        noBackground = 1U << 5,
        flag_6 = 1U << 6,
        flag_7 = 1U << 7,
        flag_8 = 1U << 8,
        resizable = 1U << 9,
        noAutoClose = 1U << 10,
        flag_11 = 1U << 11,
        flag_12 = 1U << 12,
        openQuietly = 1U << 13,
        notScrollView = 1U << 14,
        flag_15 = 1U << 15,
        flag_16 = 1U << 16,
        whiteBorderOne = 1U << 17,
        whiteBorderMask = whiteBorderOne | (1U << 18),
        flag_19 = 1U << 19,
        flag_31 = 1U << 31,
    };
    OPENLOCO_ENABLE_ENUM_OPERATORS(WindowFlags);

    struct WindowEventList
    {
        void (*onClose)(Window&) = nullptr;
        void (*onMouseUp)(Window&, WidgetIndex_t, WidgetId) = nullptr;
        void (*onResize)(Window&) = nullptr;
        void (*onMouseHover)(Window&, WidgetIndex_t, WidgetId) = nullptr;
        void (*onMouseDown)(Window&, WidgetIndex_t, WidgetId) = nullptr;
        void (*onDropdown)(Window&, WidgetIndex_t, WidgetId, int16_t) = nullptr;
        void (*onPeriodicUpdate)(Window&) = nullptr;
        void (*onUpdate)(Window&) = nullptr;
        void (*event_08)(Window&) = nullptr;
        void (*event_09)(Window&) = nullptr;
        void (*onToolUpdate)(Window&, const WidgetIndex_t, WidgetId, const int16_t, const int16_t) = nullptr;
        void (*onToolDown)(Window&, const WidgetIndex_t, WidgetId, const int16_t, const int16_t) = nullptr;
        void (*toolDrag)(Window&, const WidgetIndex_t, WidgetId, const int16_t, const int16_t) = nullptr;
        void (*toolUp)(Window&, const WidgetIndex_t, WidgetId, const int16_t, const int16_t) = nullptr;
        void (*onToolAbort)(Window&, const WidgetIndex_t, WidgetId) = nullptr;
        Ui::CursorId (*toolCursor)(Window&, const int16_t x, const int16_t y, const Ui::CursorId, bool&) = nullptr;
        void (*getScrollSize)(Window&, uint32_t scrollIndex, int32_t& scrollWidth, int32_t& scrollHeight) = nullptr;
        void (*scrollMouseDown)(Ui::Window&, int16_t x, int16_t y, uint8_t scrollIndex) = nullptr;
        void (*scrollMouseDrag)(Ui::Window&, int16_t x, int16_t y, uint8_t scrollIndex) = nullptr;
        void (*scrollMouseOver)(Ui::Window& window, int16_t x, int16_t y, uint8_t scrollIndex) = nullptr;
        void (*textInput)(Window&, WidgetIndex_t, WidgetId, const char*) = nullptr;
        void (*viewportRotate)(Window&) = nullptr;
        uint32_t event_22{};
        std::optional<FormatArguments> (*tooltip)(Window&, WidgetIndex_t, WidgetId) = nullptr;
        Ui::CursorId (*cursor)(Window&, WidgetIndex_t, WidgetId, int16_t, int16_t, Ui::CursorId) = nullptr;
        void (*onMove)(Window&, const int16_t x, const int16_t y) = nullptr;
        void (*prepareDraw)(Window&) = nullptr;
        void (*draw)(Window&, Gfx::DrawingContext&) = nullptr;
        void (*drawScroll)(Window&, Gfx::DrawingContext&, const uint32_t scrollIndex) = nullptr;
        bool (*keyUp)(Window&, uint32_t charCode, uint32_t keyCode) = nullptr;
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
        coord_t mapX{ -1 };
        coord_t mapY{ -1 };
        EntityId entityId{ EntityId::null };
        uint16_t flags{};
        ZoomLevel zoomLevel{};
        int8_t rotation{};
        int16_t surfaceZ{};

        constexpr SavedView() = default;

        constexpr SavedView(coord_t mapX, coord_t mapY, ZoomLevel zoomLevel, int8_t rotation, coord_t surfaceZ)
            : mapX(mapX)
            , mapY(mapY)
            , zoomLevel(zoomLevel)
            , rotation(rotation)
            , surfaceZ(surfaceZ) {};

        constexpr SavedView(EntityId entityId, uint16_t flags, ZoomLevel zoomLevel, int8_t rotation, coord_t surfaceZ)
            : entityId(entityId)
            , flags(flags)
            , zoomLevel(zoomLevel)
            , rotation(rotation)
            , surfaceZ(surfaceZ) {};

        constexpr bool isEmpty() const
        {
            return mapX == -1 && mapY == -1 && entityId == EntityId::null;
        }

        constexpr bool isEntityView() const
        {
            return (flags & (1 << 15)) != 0;
        }

        constexpr World::Pos3 getPos() const
        {
            if (isEntityView())
            {
                return {};
            }

            return { mapX, mapY, surfaceZ };
        }

        constexpr void clear()
        {
            mapX = -1;
            mapY = -1;
            entityId = EntityId::null;
        }

        auto operator<=>(const SavedView& other) const = default;
    };

    struct Window
    {
        static constexpr size_t kMaxScrollAreas = 2;

        sfl::small_vector<Widget, 16> widgets;
        const WindowEventList* eventHandlers;
        uint64_t disabledWidgets = 0;
        uint64_t activatedWidgets = 0;
        uint64_t holdableWidgets = 0;
        union
        {
            std::byte* object;
            struct
            {
                int16_t var_85A;
                int16_t var_85C;
            };
            uintptr_t info;
        };
        Ui::Viewport* viewports[2] = { nullptr, nullptr };
        SavedView savedView;
        int16_t expandContentCounter = 0; // Used to delay content expand when hovering over expandable scroll content
        bool showTownNames = false;       // Map window only
        WindowFlags flags;
        WindowNumber_t number = 0;
        int16_t x;
        int16_t y;
        uint16_t width;
        uint16_t height;
        uint16_t minWidth;
        uint16_t maxWidth;
        uint16_t minHeight;
        uint16_t maxHeight;
        ScrollArea scrollAreas[kMaxScrollAreas];
        int16_t rowInfo[1000];
        uint16_t rowCount;
        uint16_t var_83C;
        uint16_t rowHeight;
        int16_t rowHover = -1;
        union
        {
            int16_t orderTableIndex = -1;
            int16_t selectedTileIndex;
        };
        uint16_t sortMode;
        uint16_t var_846 = 0;
        uint16_t var_850 = 0;
        uint16_t var_852 = 0;
        uint16_t var_854 = 0; // used to limit updates
        union
        {
            uint16_t filterLevel;     // ObjectSelectionWindow
            uint16_t numTicksVisible; // TimePanel
            Input::Shortcut editingShortcutIndex; // EditKeyboardShortcut
        };
        uint16_t var_858 = 0;
        uint16_t currentTab = 0;
        uint16_t frameNo = 0;
        uint16_t currentSecondaryTab = 0;
        int16_t var_88A;
        int16_t var_88C;
        ViewportConfig viewportConfigurations[2];
        WindowType type;
        CompanyId owner = CompanyId::null;
        uint8_t var_885 = 0xFF;
        AdvancedColour colours[enumValue(WindowColour::count)];

        Window(Ui::Point32 position, Ui::Size32 size);

        // TODO: Remove this once position is a member.
        constexpr Ui::Point position() const
        {
            return { x, y };
        }

        // TODO: Remove this once size is a member.
        constexpr Ui::Size size() const
        {
            return { width, height };
        }

        void setWidgets(std::span<const Widget> newWidgets)
        {
            widgets.clear();
            widgets.insert(widgets.end(), newWidgets.begin(), newWidgets.end());
        }

        constexpr bool setSize(Ui::Size32 minSize, Ui::Size32 maxSize)
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

        constexpr void setSize(Ui::Size32 size)
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

        constexpr bool hasFlags(WindowFlags flagsToTest) const
        {
            return (flags & flagsToTest) != WindowFlags::none;
        }

        bool isVisible()
        {
            return true;
        }

        bool isTranslucent()
        {
            return this->hasFlags(WindowFlags::transparent);
        }

        bool isEnabled(WidgetIndex_t widgetIndex);
        bool isDisabled(WidgetIndex_t widgetIndex);
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
        void setDisabledWidgetsAndInvalidate(uint32_t _disabledWidgets);
        void viewportCentreMain();
        void viewportSetUndergroundFlag(bool underground, Ui::Viewport* vp);
        void viewportGetMapCoordsByCursor(int16_t* mapX, int16_t* mapY, int16_t* offsetX, int16_t* offsetY);
        void moveWindowToLocation(viewport_pos pos);
        void viewportCentreOnTile(const World::Pos3& loc);
        void viewportCentreTileAroundCursor(int16_t mapX, int16_t mapY, int16_t offsetX, int16_t offsetY);
        void viewportFocusOnEntity(EntityId targetEntity);
        bool viewportIsFocusedOnEntity(EntityId targetEntity) const;
        bool viewportIsFocusedOnAnyEntity() const;
        void viewportUnfocusFromEntity();
        void viewportZoomSet(int8_t zoomLevel, bool toCursor);
        void viewportZoomIn(bool toCursor);
        void viewportZoomOut(bool toCursor);
        void viewportRotateRight();
        void viewportRotateLeft();
        void viewportRotate(bool directionRight);
        void viewportRemove(const uint8_t viewportId);
        void viewportFromSavedView(const SavedViewSimple& savedView);

        bool move(int16_t dx, int16_t dy);
        void moveInsideScreenEdges();
        bool moveToCentre();
        WidgetIndex_t findWidgetAt(int16_t xPos, int16_t yPos);
        void draw(Gfx::DrawingContext& drawingCtx);

        void callClose();                                                                                                 // 0
        void callOnMouseUp(WidgetIndex_t widgetIndex, WidgetId id);                                                       // 1
        Ui::Window* callOnResize();                                                                                       // 2
        void callOnMouseHover(WidgetIndex_t widgetIndex, WidgetId id);                                                    // 3
        void callOnMouseDown(WidgetIndex_t widgetIndex, WidgetId id);                                                     // 4
        void callOnDropdown(WidgetIndex_t widgetIndex, WidgetId id, int16_t itemIndex);                                   // 5
        void callOnPeriodicUpdate();                                                                                      // 6
        void callUpdate();                                                                                                // 7
        void call_8();                                                                                                    // 8
        void call_9();                                                                                                    // 9
        void callToolUpdate(WidgetIndex_t widgetIndex, WidgetId id, int16_t xPos, int16_t yPos);                          // 10
        void callToolDown(WidgetIndex_t widgetIndex, WidgetId id, int16_t xPos, int16_t yPos);                            // 11
        void callToolDrag(WidgetIndex_t widgetIndex, WidgetId id, const int16_t xPos, const int16_t yPos);                // 12
        void callToolUp(WidgetIndex_t widgetIndex, WidgetId id, const int16_t xPos, const int16_t yPos);                  // 13
        void callToolAbort(WidgetIndex_t widgetIndex, WidgetId id);                                                       // 14
        Ui::CursorId callToolCursor(int16_t xPos, int16_t yPos, Ui::CursorId fallback, bool* out);                        // 15
        void callGetScrollSize(uint32_t scrollIndex, int32_t& scrollWidth, int32_t& scrollHeight);                        // 16
        void callScrollMouseDown(int16_t x, int16_t y, uint8_t scrollIndex);                                              // 17
        void callScrollMouseDrag(int16_t x, int16_t y, uint8_t scrollIndex);                                              // 18
        void callScrollMouseOver(int16_t x, int16_t y, uint8_t scrollIndex);                                              // 19
        void callTextInput(WidgetIndex_t caller, WidgetId id, const char* buffer);                                        // 20
        void callViewportRotate();                                                                                        // 21
        std::optional<FormatArguments> callTooltip(WidgetIndex_t widgetIndex, WidgetId id);                               // 23
        Ui::CursorId callCursor(WidgetIndex_t widgetIdx, WidgetId id, int16_t xPos, int16_t yPos, Ui::CursorId fallback); // 24
        void callOnMove(int16_t xPos, int16_t yPos);                                                                      // 25
        void callPrepareDraw();                                                                                           // 26
        void callDraw(Gfx::DrawingContext& ctx);                                                                          // 27
        void callDrawScroll(Gfx::DrawingContext& drawingCtx, uint32_t scrollIndex);                                       // 28
        bool callKeyUp(uint32_t charCode, uint32_t keyCode);                                                              // 29

        WidgetIndex_t firstActivatedWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex);
        WidgetIndex_t prevAvailableWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex);
        WidgetIndex_t nextAvailableWidgetInRange(WidgetIndex_t minIndex, WidgetIndex_t maxIndex);
    };

    World::Pos2 viewportCoordToMapCoord(int16_t x, int16_t y, int16_t z, int32_t rotation);
    std::optional<World::Pos2> screenGetMapXyWithZ(const Point& mouse, const int16_t z);

}
