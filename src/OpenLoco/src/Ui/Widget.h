#pragma once

#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include <array>
#include <cstdint>

namespace OpenLoco::Gfx
{
    enum class RectInsetFlags : uint8_t;

    class DrawingContext;
}

namespace OpenLoco::Ui
{
    using WidgetIndex_t = int16_t;

    constexpr WidgetIndex_t kWidgetIndexNull = -1;

    struct Window;
    enum class WindowColour : uint8_t;

    enum class ContentAlign : uint8_t
    {
        Left = 0,
        Center,
        Right,
    };

    enum class WidgetType : uint8_t
    {
        none = 0,
        panel = 1,
        frame = 2,
        wt_3,
        wt_4,
        slider,
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
    };

    struct WidgetState
    {
        Window* window;
        Gfx::RectInsetFlags flags;
        AdvancedColour colour;
        bool enabled;
        bool disabled;
        bool activated;
        bool hovered;
        int scrollviewIndex;
    };

    struct Widget;

    struct WidgetEventsList
    {
        void (*draw)(Gfx::DrawingContext&, const Widget&, const WidgetState&) = nullptr;
    };

    struct Widget
    {
        // Indicates that the imageId has a colour set and not to replace it with the window colour
        // This reuses the ImageIdFlags::translucent flag for use in widget draw
        // Flag *MUST* be removed before passing to drawingCtx.drawImage functions
        static constexpr uint32_t kImageIdColourSet = (1U << 30);
        static constexpr uint32_t kContentNull = 0xFFFFFFFFU;
        static constexpr uint32_t kContentUnk = 0xFFFFFFFEU;

        constexpr Widget(Ui::Point32 origin, Ui::Size32 size, WidgetType widgetType, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : content{ content }
            , left{ static_cast<int16_t>(origin.x) }
            , right{ static_cast<int16_t>(origin.x + size.width - 1) }
            , top{ static_cast<int16_t>(origin.y) }
            , bottom{ static_cast<int16_t>(origin.y + size.height - 1) }
            , tooltip{ tooltip }
            , type{ widgetType }
            , windowColour{ colour }
        {
        }

        constexpr Widget(Ui::Point32 origin, Ui::Size32 size, WidgetType widgetType, WindowColour colour, StringId content, StringId tooltip = StringIds::null)
            : text{ content }
            , left{ static_cast<int16_t>(origin.x) }
            , right{ static_cast<int16_t>(origin.x + size.width - 1) }
            , top{ static_cast<int16_t>(origin.y) }
            , bottom{ static_cast<int16_t>(origin.y + size.height - 1) }
            , tooltip{ tooltip }
            , type{ widgetType }
            , windowColour{ colour }
        {
        }

        constexpr Widget(WidgetType widgetType)
            : content{ kContentNull }
            , left{}
            , right{}
            , top{}
            , bottom{}
            , tooltip{ StringIds::null }
            , type{ widgetType }
            , windowColour{}
        {
        }

        constexpr Widget() = default;

        FormatArgumentsBuffer textArgs;
        WidgetEventsList events;
        union
        {
            uint32_t image{ ImageIds::null };
            StringId text;
            uint32_t content;
        };
        int16_t left{};
        int16_t right{};
        int16_t top{};
        int16_t bottom{};
        Gfx::Font font{ Gfx::Font::medium_bold };
        StringId tooltip{ StringIds::null };
        WidgetType type{};
        ContentAlign contentAlign{ ContentAlign::Left };
        WindowColour windowColour{};

        int16_t midX() const;
        int16_t midY() const;
        uint16_t width() const;
        uint16_t height() const;

        // TODO: Make tabs actual widgets.
        static void drawTab(Window* w, Gfx::DrawingContext& drawingCtx, uint32_t imageId, WidgetIndex_t index);

        // typical tab width, to be used in most (all?) cases
        static constexpr uint16_t kDefaultTabWidth = 30;
        static void leftAlignTabs(Window& window, uint8_t firstTabIndex, uint8_t lastTabIndex, uint16_t tabWidth = kDefaultTabWidth);

        void draw(Gfx::DrawingContext& drawingCtx, Window* window, const uint64_t pressedWidgets, const uint64_t toolWidgets, const uint64_t hoveredWidgets, uint8_t& scrollviewIndex);
    };

    constexpr Widget makeWidget(Ui::Point32 origin, Ui::Size32 size, WidgetType type, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        Widget out{ origin, size, type, colour, content, tooltip };
        return out;
    }

    constexpr Widget makeDropdownButtonWidget(Ui::Point32 origin, Ui::Size32 size, WindowColour colour, [[maybe_unused]] uint32_t content = Widget::kContentNull, [[maybe_unused]] StringId tooltip = StringIds::null)
    {
        const int16_t xPos = origin.x + size.width - 12;
        const int16_t yPos = origin.y + 1;
        const uint16_t width = 11;
        const uint16_t height = 10;

        return makeWidget({ xPos, yPos }, { width, height }, WidgetType::button, colour, StringIds::dropdown, tooltip);
    }

    namespace Detail
    {
        template<typename T, typename Enable = void>
        struct WidgetsCount
        {
            static constexpr size_t count = 0;
        };

        template<typename T>
        struct WidgetsCount<T, std::enable_if_t<std::is_base_of_v<Widget, T>>>
        {
            static constexpr size_t count = 1;
        };

        template<typename T, std::size_t N>
        struct WidgetsCount<std::array<T, N>, std::enable_if_t<std::is_base_of_v<Widget, T>>>
        {
            static constexpr size_t count = N;
        };

        template<typename T>
        struct IsWidgetsArray : std::false_type
        {
        };

        template<std::size_t N>
        struct IsWidgetsArray<std::array<Widget, N>> : std::true_type
        {
        };
    }

    template<typename... TArgs>
    constexpr auto makeWidgets(TArgs&&... args)
    {
        constexpr auto totalCount = [&]() {
            size_t count = 0;
            ((count += Detail::WidgetsCount<std::decay_t<decltype(args)>>::count), ...);
            return count;
        }();

        std::array<Widget, totalCount> res{};
        size_t index = 0;

        const auto append = [&](auto&& val) {
            if constexpr (Detail::IsWidgetsArray<std::decay_t<decltype(val)>>::value)
            {
                for (auto&& widget : val)
                {
                    res[index] = std::move(widget);
                    index++;
                }
            }
            else
            {
                res[index] = std::move(val);
                index++;
            }
        };

        ((append(args)), ...);

        return res;
    }

    constexpr auto makeDropdownWidgets(Ui::Point32 origin, Ui::Size32 size, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        // TODO: Make this a single widget.
        return makeWidgets(
            makeWidget(origin, size, WidgetType::combobox, colour, content, tooltip),
            makeDropdownButtonWidget(origin, size, colour));
    }
}
