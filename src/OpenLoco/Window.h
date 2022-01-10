#pragma once

#include "Company.h"
#include "Core/Optional.hpp"
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
        buttonGridSort = 14,
        wt_15,
        groupbox = 16,
        textbox = 17,
        dropdown = 18,
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

	namespace WindowFlags
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
		constexpr uint32_t openQuietly = 1 << 13;
		constexpr uint32_t not_scroll_view = 1 << 14;
		constexpr uint32_t flag_15 = 1 << 15;
		constexpr uint32_t flag_16 = 1 << 16;
		constexpr uint32_t white_border_one = (1 << 17);
		constexpr uint32_t white_border_mask = WindowFlags::white_border_one | (1 << 18);
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
				void (*on_close)(Window*);
				void (*on_mouse_up)(Window*, WidgetIndex_t);
				void (*on_resize)(Window*);
				void (*event_03)(Window*, WidgetIndex_t); // mouse_over?
				void (*on_mouse_down)(Window*, WidgetIndex_t);
				void (*on_dropdown)(Window*, WidgetIndex_t, int16_t);
				void (*on_periodic_update)(Window*);
				void (*on_update)(Window*);
				void (*event_08)(Window*);
				void (*event_09)(Window*);
				void (*on_tool_update)(Window&, const WidgetIndex_t, const int16_t, const int16_t);
				void (*on_tool_down)(Window&, const WidgetIndex_t, const int16_t, const int16_t);
				void (*toolDragContinue)(Window&, const WidgetIndex_t, const int16_t, const int16_t);
				void (*toolDragEnd)(Window&, const WidgetIndex_t);
				void (*on_tool_abort)(Window&, const WidgetIndex_t);
				Ui::CursorId(*event_15)(Window&, const int16_t x, const int16_t y, const Ui::CursorId, bool&);
				void (*get_scroll_size)(Window*, uint32_t scrollIndex, uint16_t* scrollWidth, uint16_t* scrollHeight);
				void (*scroll_mouse_down)(Ui::Window*, int16_t x, int16_t y, uint8_t scroll_index);
				void (*scroll_mouse_drag)(Ui::Window*, int16_t x, int16_t y, uint8_t scroll_index);
				void (*scroll_mouse_over)(Ui::Window* window, int16_t x, int16_t y, uint8_t scroll_index);
				void (*text_input)(Window*, WidgetIndex_t, const char*);
				void (*viewport_rotate)(Window*);
				uint32_t event_22;
				std::optional<FormatArguments>(*tooltip)(Window*, WidgetIndex_t);
				Ui::CursorId(*cursor)(Window*, int16_t, int16_t, int16_t, Ui::CursorId);
				void (*on_move)(Window&, const int16_t x, const int16_t y);
				void (*prepare_draw)(Window*);
				void (*draw)(Window*, Gfx::Context*);
				void (*draw_scroll)(Window&, Gfx::Context&, const uint32_t scrollIndex);
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
			, surfaceZ(surfaceZ) {};

		SavedView(EntityId thingId, uint16_t flags, ZoomLevel zoomLevel, int8_t rotation, coord_t surfaceZ)
			: thingId(thingId)
			, flags(flags)
			, zoomLevel(zoomLevel)
			, rotation(rotation)
			, surfaceZ(surfaceZ) {};

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
		WindowEventList* event_handlers;                   // 0x00
		Ui::Viewport* viewports[2] = { nullptr, nullptr }; // 0x04
		uint64_t enabled_widgets = 0;                      // 0x0C
		uint64_t disabled_widgets = 0;                     // 0x14
		uint64_t activated_widgets = 0;                    // 0x1C
		uint64_t holdable_widgets = 0;                     // 0x24
		Widget* widgets;                                   // 0x2C
		int16_t x;                                         // 0x30
		int16_t y;                                         // 0x32
		uint16_t width;                                    // 0x34
		uint16_t height;                                   // 0x36
		uint16_t min_width;                                // 0x38
		uint16_t max_width;                                // 0x3a
		uint16_t min_height;                               // 0x3c
		uint16_t max_height;                               // 0x3e
		WindowNumber_t number = 0;                         // 0x40
		uint32_t flags;                                    // 0x42
		ScrollArea scroll_areas[2];                        // 0x46
		int16_t row_info[1000];                            // 0x6A
		uint16_t row_count;                                // 0x83A
		uint16_t var_83C;
		uint16_t row_height;    // 0x83E
		int16_t row_hover = -1; // 0x840
		int16_t var_842;        // 0x842
		uint16_t sort_mode;     // 0x844;
		uint16_t var_846 = 0;
		SavedView saved_view; // 0x848
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
		uint16_t current_tab = 0;                  // 0x870
		uint16_t frame_no = 0;                     // 0x872
		uint16_t current_secondary_tab = 0;        // 0x874
		ViewportConfig viewport_configurations[2]; // 0x876
		WindowType type;                           // 0x882
		uint8_t pad_883[1];
		CompanyId owner = CompanyId::null; // 0x884
		uint8_t var_885 = 0xFF;
		uint8_t colours[static_cast<uint8_t>(WindowColour::count)]; // 0x886
		int16_t var_88A;
		int16_t var_88C;

		Window(Ui::Point position, Ui::Size size);

		constexpr bool setSize(Ui::Size minSize, Ui::Size maxSize)
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

		constexpr void setSize(Ui::Size size)
		{
			setSize(size, size);
		}

		constexpr uint8_t getColour(WindowColour index) const
		{
			assert(index < WindowColour::count);
			return colours[static_cast<uint8_t>(index)];
		}
		constexpr void setColour(WindowColour index, Colour_t colour)
		{
			assert(index < WindowColour::count);
			colours[static_cast<uint8_t>(index)] = colour;
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
