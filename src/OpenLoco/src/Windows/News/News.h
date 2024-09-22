#pragma once

#include "Graphics/Gfx.h"
#include "Ui/WindowManager.h"
#include "World/Company.h"

namespace OpenLoco::Ui::Windows::NewsWindow
{
    struct NewsState
    {
        uint32_t dword_525CD0; // 0x00525CD0
        uint32_t dword_525CD4; // 0x00525CD4
        uint32_t dword_525CD8; // 0x00525CD8
        uint32_t dword_525CDC; // 0x00525CDC
        uint16_t word_525CE0;  // 0x00525CE0
    };

    extern NewsState _nState;

    namespace Common
    {
        enum widx
        {
            frame,
            close_button,
            viewport1,
            viewport2,
            viewport1Button,
            viewport2Button,
        };

        constexpr uint64_t enabledWidgets = (1 << close_button) | (1 << viewport1Button) | (1 << viewport2Button);

        constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight, WidgetType frameType)
        {
            return makeWidgets(
                makeWidget({ 0, 0 }, { frameWidth, frameHeight }, frameType, WindowColour::primary),
                makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                makeWidget({ 2, frameHeight - 73 }, { 168, 64 }, WidgetType::viewport, WindowColour::primary, Widget::kContentUnk),
                makeWidget({ 180, frameHeight - 73 }, { 168, 64 }, WidgetType::viewport, WindowColour::primary, Widget::kContentUnk),
                makeWidget({ 2, frameHeight - 75 }, { 180, 75 }, WidgetType::buttonWithImage, WindowColour::primary),
                makeWidget({ 2, frameHeight - 75 }, { 180, 75 }, WidgetType::buttonWithImage, WindowColour::primary));
        }
    }

    namespace News1
    {
        static constexpr Ui::Size32 kWindowSize = { 360, 117 };

        std::span<const Widget> getWidgets();

        void initViewport(Window& self);
        const WindowEventList& getEvents();
    }

    namespace News2
    {
        static constexpr Ui::Size32 kWindowSize = { 360, 159 };

        std::span<const Widget> getWidgets();
    }

    namespace Ticker
    {
        static constexpr Ui::Size32 kWindowSize = { 111, 26 };

        enum widx
        {
            frame,
        };
        constexpr uint64_t enabledWidgets = (1 << widx::frame);

        std::span<const Widget> getWidgets();

        const WindowEventList& getEvents();
    }
}
