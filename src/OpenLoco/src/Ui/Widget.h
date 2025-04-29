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

    namespace Detail
    {
        // TODO: Move this to a more appropriate location.
        static constexpr uint64_t getHashFNV1a(const char* s, size_t)
        {
            // FNV-1a hash
            constexpr auto kPrime = 0x00000100000001B3ULL;
            constexpr auto kOffsetBasis = 0xCBF29CE484222325ULL;

            auto res = kOffsetBasis;
            for (size_t i = 0; s[i] != '\0'; i++)
            {
                res ^= s[i];
                res *= kPrime;
            }

            return res;
        }
    }

    class WidgetId
    {
    public:
        enum class ValueType : uint64_t
        {
            none = 0,
        };

    private:
        ValueType _value{};

    public:
        static constexpr auto none = ValueType::none;

        constexpr WidgetId() = default;

        constexpr WidgetId(ValueType value)
            : _value{ value }
        {
        }

        template<size_t TSize>
        constexpr WidgetId(const char (&str)[TSize])
            : _value{ static_cast<ValueType>(Detail::getHashFNV1a(str, TSize)) }
        {
        }

        constexpr WidgetId(const char* str)
            : _value{ static_cast<ValueType>(Detail::getHashFNV1a(str, 0)) }
        {
        }

        constexpr auto operator<=>(const WidgetId&) const = default;

        // This makes switch statements work.
        constexpr operator uint64_t() const
        {
            return static_cast<uint64_t>(_value);
        }
    };

    struct Window;
    enum class WindowColour : uint8_t;

    enum class ContentAlign : uint8_t
    {
        left = 0,
        center,
        right,
    };

    enum class WidgetType : uint8_t
    {
        empty = 0,
        panel,
        newsPanel,
        frame,
        wt_3,
        slider,
        wt_6,
        toolbarTab,
        tab,
        buttonWithImage,
        buttonWithColour,
        button,
        label,
        buttonTableHeader,
        groupbox,
        textbox,
        combobox,
        viewport,
        caption,
        scrollview,
        checkbox,
    };

    struct WidgetState
    {
        Window* window{};
        Gfx::RectInsetFlags flags{};
        AdvancedColour colour{};
        bool disabled{};
        bool activated{};
        bool hovered{};
        int scrollviewIndex{};
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

        constexpr Widget(WidgetId widgetId, Ui::Point32 origin, Ui::Size32 size, WidgetType widgetType, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
            : id{ widgetId }
            , content{ content }
            , left{ static_cast<int16_t>(origin.x) }
            , right{ static_cast<int16_t>(origin.x + size.width - 1) }
            , top{ static_cast<int16_t>(origin.y) }
            , bottom{ static_cast<int16_t>(origin.y + size.height - 1) }
            , tooltip{ tooltip }
            , type{ widgetType }
            , windowColour{ colour }
        {
        }

        constexpr Widget(WidgetId widgetId, Ui::Point32 origin, Ui::Size32 size, WidgetType widgetType, WindowColour colour, StringId content, StringId tooltip = StringIds::null)
            : id{ widgetId }
            , text{ content }
            , left{ static_cast<int16_t>(origin.x) }
            , right{ static_cast<int16_t>(origin.x + size.width - 1) }
            , top{ static_cast<int16_t>(origin.y) }
            , bottom{ static_cast<int16_t>(origin.y + size.height - 1) }
            , tooltip{ tooltip }
            , type{ widgetType }
            , windowColour{ colour }
        {
        }

        constexpr Widget() = default;

        WidgetId id{ WidgetId::none };
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
        ContentAlign contentAlign{ ContentAlign::left };
        WindowColour windowColour{};

        int16_t midX() const;
        int16_t midY() const;
        uint16_t width() const;
        uint16_t height() const;

        // Custom widget attributes.
        uint32_t styleData{};

        // Widget state.
        bool disabled : 1 {};
        bool activated : 1 {};
        bool hidden : 1 {};

        // TODO: Remove this once position is a member.
        Ui::Point position() const
        {
            return { static_cast<int16_t>(left), static_cast<int16_t>(top) };
        }

        // TODO: Remove this once size is a member.
        Ui::Size size() const
        {
            return { width(), height() };
        }

        // TODO: Make tabs actual widgets.
        static void drawTab(Window* w, Gfx::DrawingContext& drawingCtx, uint32_t imageId, WidgetIndex_t index);

        // typical tab width, to be used in most (all?) cases
        static constexpr uint16_t kDefaultTabWidth = 30;
        static void leftAlignTabs(Window& window, uint8_t firstTabIndex, uint8_t lastTabIndex, uint16_t tabWidth = kDefaultTabWidth);

        void draw(Gfx::DrawingContext& drawingCtx, Window* window, const uint64_t pressedWidgets, const uint64_t toolWidgets, const uint64_t hoveredWidgets, uint8_t scrollviewIndex);
    };

    constexpr Widget makeWidget(Ui::Point32 origin, Ui::Size32 size, WidgetType type, WindowColour colour, uint32_t content = Widget::kContentNull, StringId tooltip = StringIds::null)
    {
        Widget out{ WidgetId::none, origin, size, type, colour, content, tooltip };
        return out;
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
}
