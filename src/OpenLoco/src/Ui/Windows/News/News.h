#pragma once

#include "Graphics/Gfx.h"
#include "Ui/Widgets/ImageButtonWidget.h"
#include "Ui/Widgets/NewsPanelWidget.h"
#include "Ui/Widgets/ViewportWidget.h"
#include "Ui/WindowManager.h"
#include "World/Company.h"

namespace OpenLoco::Ui::Windows::NewsWindow
{
    struct NewsState
    {
        SavedView savedView[2];
        uint16_t slideInHeight;
        uint16_t numCharsToDisplay;
    };

    enum class SubjectType : int8_t
    {
        companyFace = -2,
        vehicleImage = -3,
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

        constexpr auto makeCommonWidgets(int32_t frameWidth, int32_t frameHeight)
        {
            return makeWidgets(
                Widgets::NewsPanel({ 0, 0 }, { frameWidth, frameHeight }, Widgets::NewsPanel::Style::old, WindowColour::primary),
                Widgets::ImageButton({ frameWidth - 15, 2 }, { 13, 13 }, WindowColour::primary, ImageIds::close_button, StringIds::tooltip_close_window),
                Widgets::Viewport({ 2, frameHeight - 73 }, { 168, 64 }, WindowColour::primary, Widget::kContentUnk),
                Widgets::Viewport({ 180, frameHeight - 73 }, { 168, 64 }, WindowColour::primary, Widget::kContentUnk),
                Widgets::ImageButton({ 2, frameHeight - 75 }, { 180, 75 }, WindowColour::primary),
                Widgets::ImageButton({ 2, frameHeight - 75 }, { 180, 75 }, WindowColour::primary));
        }
    }

    namespace News1
    {
        static constexpr Ui::Size32 kWindowSize = { 360, 117 };

        std::span<const Widget> getWidgets();

        void initViewports(Window& self);
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
