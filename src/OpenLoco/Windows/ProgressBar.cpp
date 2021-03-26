#include "../Ui/ProgressBar.h"
#include "../Graphics/Colour.h"
#include "../Graphics/Gfx.h"
#include "../Graphics/ImageIds.h"
#include "../Interop/Interop.hpp"
#include "../OpenLoco.h"
#include "../Ui/WindowManager.h"
#include "../Window.h"

#include <array>
#include <string>
#include <string_view>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::Windows::ProgressBar
{
    enum Widx
    {
        frame,
        caption,
        panel,
    };

    constexpr Gfx::ui_size_t windowSize = { 350, 47 };

    widget_t widgets[] = {
        makeWidget({ 0, 0 }, { 350, 47 }, widget_type::frame, 0),
        makeWidget({ 1, 1 }, { 348, 13 }, widget_type::caption_25, 0, StringIds::buffer_1250),
        makeWidget({ 0, 15 }, { 350, 32 }, widget_type::panel, 1),
        widgetEnd(),
    };

    static window_event_list _events;

    static std::string _captionString;
    static uint8_t _progressBarStyle = 0; // 0x005233C8
    static uint8_t _progressBarValue = 0; // 0x011370A8

    void setProgress(uint8_t value);
    static void initEvents();

    // 0x004CF6E2
    window* open(std::string_view captionString)
    {
        _captionString = captionString;
        setScreenFlag(ScreenFlags::progressBarActive);
        _progressBarValue = 0xFF;

        initEvents();

        auto window = WindowManager::createWindowCentred(
            WindowType::progressBar,
            windowSize,
            WindowFlags::flag_11 | WindowFlags::stick_to_front,
            &_events);

        window->widgets = widgets;
        window->initScrollWidgets();
        window->colours[0] = Colour::black;
        window->colours[1] = Colour::black;

        setProgress(0);
        return window;
    }

    // 0x004CF74E
    void close()
    {
        clearScreenFlag(ScreenFlags::progressBarActive);
        WindowManager::close(WindowType::progressBar);
        Gfx::invalidateScreen();
        _progressBarStyle ^= 1;
    }

    // 0x004CF76D
    void setProgress(uint8_t value)
    {
        if (_progressBarValue == value)
            return;

        _progressBarValue = value;
        WindowManager::invalidate(WindowType::progressBar);
        Ui::ProgressBar::sub_4CF63B();
    }

    // 0x004CF78A
    static void prepareDraw(window* self)
    {
        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
        strncpy(buffer, _captionString.c_str(), 256);
    }

    // 004CF7A0
    static void draw(window* self, Gfx::drawpixelinfo_t* dpi)
    {
        self->draw(dpi);

        Gfx::drawpixelinfo_t* clipped = nullptr;
        if (!Gfx::clipDrawpixelinfo(&clipped, dpi, Gfx::point_t(self->x + 2, self->y + 17), Gfx::ui_size_t(self->width - 5, self->height - 19)))
            return;

        // First, draw the train track.
        Gfx::drawImage(clipped, 0, 0, ImageIds::progressbar_track);

        // What train image to use depends on the progress bar style.
        uint32_t trainImage;
        if (_progressBarStyle == 0)
        {
            static std::array<uint32_t, 4> style0Frames = {
                ImageIds::progressbar_style0_frame0,
                ImageIds::progressbar_style0_frame1,
                ImageIds::progressbar_style0_frame2,
                ImageIds::progressbar_style0_frame3,
            };

            auto currentFrame = (_progressBarValue / 4) % std::size(style0Frames);
            trainImage = Gfx::recolour2(style0Frames.at(currentFrame), Colour::saturated_green, Colour::salmon_pink);
        }
        else
        {
            static std::array<uint32_t, 4> style1Frames = {
                ImageIds::progressbar_style1_frame0,
                ImageIds::progressbar_style1_frame1,
                ImageIds::progressbar_style1_frame2,
                ImageIds::progressbar_style1_frame3,
            };

            auto currentFrame = (_progressBarValue / 4) % std::size(style1Frames);
            trainImage = Gfx::recolour2(style1Frames.at(currentFrame), Colour::black, Colour::dark_green);
        }

        // Draw the train image from the right of the window,
        int16_t xPos = _progressBarValue - 255;
        Gfx::drawImage(clipped, xPos, 0, trainImage);
    }

    static void initEvents()
    {
        _events.draw = draw;
        _events.prepare_draw = prepareDraw;
    }
}
