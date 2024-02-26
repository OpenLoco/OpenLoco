#include "Ui/ProgressBar.h"
#include "Drawing/SoftwareDrawingEngine.h"
#include "Graphics/Colour.h"
#include "Graphics/Gfx.h"
#include "Graphics/ImageIds.h"
#include "Localisation/StringIds.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "Widget.h"
#include "Window.h"
#include <OpenLoco/Interop/Interop.hpp>

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

    static constexpr Ui::Size kWindowSize = { 350, 47 };

    Widget widgets[] = {
        makeWidget({ 0, 0 }, { 350, 47 }, WidgetType::frame, WindowColour::primary),
        makeWidget({ 1, 1 }, { 348, 13 }, WidgetType::caption_25, WindowColour::primary, StringIds::buffer_1250),
        makeWidget({ 0, 15 }, { 350, 32 }, WidgetType::panel, WindowColour::secondary),
        widgetEnd(),
    };

    static std::string _captionString;
    static uint8_t _progressBarStyle = 0; // 0x005233C8
    static uint8_t _progressBarValue = 0; // 0x011370A8

    void setProgress(uint8_t value);
    static const WindowEventList& getEvents();

    // 0x004CF6E2
    Window* open(std::string_view captionString)
    {
        _captionString = captionString;
        setScreenFlag(ScreenFlags::progressBarActive);
        _progressBarValue = 0xFF;

        auto window = WindowManager::createWindowCentred(
            WindowType::progressBar,
            kWindowSize,
            WindowFlags::flag_11 | WindowFlags::stickToFront,
            getEvents());

        window->widgets = widgets;
        window->initScrollWidgets();
        window->setColour(WindowColour::primary, Colour::black);
        window->setColour(WindowColour::secondary, Colour::black);

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
        Gfx::renderAndUpdate();
    }

    // 0x004CF78A
    static void prepareDraw([[maybe_unused]] Window& self)
    {
        char* buffer = const_cast<char*>(StringManager::getString(StringIds::buffer_1250));
        strncpy(buffer, _captionString.c_str(), 256);
    }

    // 004CF7A0
    static void draw(Window& self, Gfx::RenderTarget* rt)
    {
        auto drawingCtx = Gfx::getDrawingEngine().getDrawingContext();

        self.draw(rt);

        auto clipped = Gfx::clipRenderTarget(*rt, Ui::Rect(self.x + 2, self.y + 17, self.width - 5, self.height - 19));
        if (!clipped)
            return;

        // First, draw the train track.
        drawingCtx.drawImage(&*clipped, 0, 0, ImageIds::progressbar_track);

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
            trainImage = Gfx::recolour2(style0Frames.at(currentFrame), Colour::mutedSeaGreen, Colour::mutedDarkRed);
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
            trainImage = Gfx::recolour2(style1Frames.at(currentFrame), Colour::black, Colour::mutedGrassGreen);
        }

        // Draw the train image from the right of the window,
        int16_t xPos = _progressBarValue - 255;
        drawingCtx.drawImage(&*clipped, xPos, 0, trainImage);
    }

    static constexpr WindowEventList kEvents = {
        .prepareDraw = prepareDraw,
        .draw = draw,
    };

    static const WindowEventList& getEvents()
    {
        return kEvents;
    }
}
