#pragma once
#include "Graphics/Gfx.h"
#include "Ui/WindowManager.h"
#include "World/Company.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::NewsWindow
{
    static loco_global<uint32_t, 0x00523338> _cursorX2;
    static loco_global<uint32_t, 0x0052333C> _cursorY2;
    static loco_global<uint32_t, 0x00525CD0> _dword_525CD0;
    static loco_global<uint32_t, 0x00525CD4> _dword_525CD4;
    static loco_global<uint32_t, 0x00525CD8> _dword_525CD8;
    static loco_global<uint32_t, 0x00525CDC> _dword_525CDC;
    static loco_global<uint16_t, 0x00525CE0> _word_525CE0;
    static loco_global<uint16_t, 0x005271CE> _messageCount;
    static loco_global<char[512], 0x0112CC04> _byte_112CC04;
    static loco_global<uint32_t, 0x011364EC> _numTrackTypeTabs;
    static loco_global<int8_t[8], 0x011364F0> _trackTypesForTab;

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

#define commonWidgets(frameWidth, frameHeight, frameType)                                                                                                            \
    makeWidget({ 0, 0 }, { frameWidth, frameHeight }, frameType, WindowColour::primary),                                                                             \
        makeWidget({ frameWidth - 15, 2 }, { 13, 13 }, WidgetType::buttonWithImage, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window), \
        makeWidget({ 2, frameHeight - 73 }, { 168, 64 }, WidgetType::viewport, WindowColour::primary, Widget::kContentUnk),                                          \
        makeWidget({ 180, frameHeight - 73 }, { 168, 64 }, WidgetType::viewport, WindowColour::primary, Widget::kContentUnk),                                        \
        makeWidget({ 2, frameHeight - 75 }, { 180, 75 }, WidgetType::buttonWithImage, WindowColour::primary),                                                        \
        makeWidget({ 2, frameHeight - 75 }, { 180, 75 }, WidgetType::buttonWithImage, WindowColour::primary)

        void initEvents();
    }

    namespace News1
    {
        static constexpr Ui::Size kWindowSize = { 360, 117 };

        extern Widget widgets[7];

        extern WindowEventList events;

        void initEvents();
        void initViewport(Window& self);
    }

    namespace News2
    {
        static constexpr Ui::Size kWindowSize = { 360, 159 };

        extern Widget widgets[7];
    }

    namespace Ticker
    {
        static constexpr Ui::Size kWindowSize = { 111, 26 };

        enum widx
        {
            frame,
        };
        constexpr uint64_t enabledWidgets = (1 << widx::frame);

        extern Widget widgets[2];

        extern WindowEventList events;

        void initEvents();
    }
}
