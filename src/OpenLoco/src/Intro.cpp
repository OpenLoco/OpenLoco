#include "Intro.h"
#include "Audio/Audio.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Localisation/StringIds.h"
#include "MultiPlayer.h"
#include "OpenLoco.h"
#include "Ui.h"
#include "ViewportManager.h"

namespace OpenLoco::Intro
{
    static int32_t _introTicks; // 0x0050C190
    static State _state;        // 0x0050C195

    bool isActive()
    {
        return state() != State::none;
    }

    State state()
    {
        return _state;
    }

    void state(State state)
    {
        _state = state;
    }

    void skip()
    {
        switch (_state)
        {
            case State::displayAtari:
                _introTicks = 109;
                break;
            case State::displayCS:
                _introTicks = 99;
                break;
            case State::displayNotice:
                _state = State::end;
                break;
            default:
                break;
        }
    }

    static void tickEnd()
    {
        _state = State::none;
        _introTicks = 0;
    }

    static void tickNone() {}

    static void tickDisplayNoticeBegin()
    {
        Gfx::loadPalette(ImageIds::default_palette, 0);
        _state = State::displayNoticeBeginReset;
    }

    static void tickBegin()
    {
        if (MultiPlayer::hasFlag(MultiPlayer::flag_8) || MultiPlayer::hasFlag(MultiPlayer::flag_9))
        {
            tickDisplayNoticeBegin();
            return;
        }

        Gfx::loadPalette(ImageIds::atari_intro_palette, 0);

        _introTicks = -24;
        _state = State::displayAtari;
    }

    static void tickDisplayAtari()
    {
        _introTicks++;
        uint8_t modifier = 0;
        if (_introTicks >= 0)
        {
            modifier = _introTicks;
            if (_introTicks >= 55)
            {
                modifier = 110 - _introTicks;
            }
        }
        modifier = std::min(255, modifier * 8);

        Gfx::loadPalette(ImageIds::atari_intro_palette, modifier);

        if (_introTicks >= 110)
        {
            Gfx::loadPalette(ImageIds::chris_sawyer_intro_palette, 0);

            _introTicks = 0;
            _state = State::displayCS;
        }
    }
    static void tickDisplayCS()
    {
        _introTicks++;
        uint8_t modifier = 0;
        if (_introTicks >= 0)
        {
            modifier = _introTicks;
            if (_introTicks >= 50)
            {
                modifier = 100 - _introTicks;
            }
        }
        modifier = std::min(255, modifier * 8);

        Gfx::loadPalette(ImageIds::chris_sawyer_intro_palette, modifier);
        if (_introTicks >= 100)
        {
            _state = State::displayNoticeBegin;
        }
    }
    static void tickState4() {}
    static void tickState5() {}
    static void tickState6() {}
    static void tickState7() {}

    constexpr std::array<StringId, 5> kIntroNoticeStrings = {
        StringIds::intro_vehicle_notice_0,
        StringIds::intro_vehicle_notice_1,
        StringIds::intro_vehicle_notice_2,
        StringIds::intro_vehicle_notice_3,
        StringIds::intro_vehicle_notice_4,
    };

    static void tickDisplayNotice()
    {
        if (_introTicks == 0)
        {
            Gfx::loadPalette(ImageIds::default_palette, 0xFF);
        }
        _introTicks++;

        if (MultiPlayer::hasFlag(MultiPlayer::flag_8) || MultiPlayer::hasFlag(MultiPlayer::flag_9))
        {
            if (_introTicks >= 160)
            {
                _state = State::end;
            }
        }
        else
        {
            if (_introTicks >= 1440)
            {
                _state = State::end;
            }
        }
    }
    static void tickDisplayNoticeBeginReset()
    {
        _introTicks = 0;
        _state = State::displayNotice;
        tickDisplayNotice();
    }

    constexpr std::array<void (*)(), 11> kTickFunctions = {
        tickNone,
        tickBegin,
        tickDisplayAtari,
        tickDisplayCS,
        tickState4,
        tickState5,
        tickState6,
        tickState7,
        tickDisplayNoticeBegin,
        tickDisplayNotice,
        tickDisplayNoticeBeginReset,
    };

    // 0x0046AE0C
    void tick()
    {
        if (_state == State::end)
        {
            tickEnd();
        }
        else if (enumValue(_state) < std::size(kTickFunctions))
        {
            kTickFunctions[enumValue(_state)]();
        }
    }

    static void drawAtari(Gfx::DrawingContext& drawingCtx)
    {
        drawingCtx.clear(PaletteIndex::mutedDarkRed5); // this isn't actually mutedDarkRed5 as the atari palette is different

        const auto pos = Ui::Point(Ui::width() / 2 - 216, Ui::height() / 2 - 54);
        drawingCtx.drawImage(ZoomLevel::full, pos, ImageId(ImageIds::atari_logo_intro_left));
        drawingCtx.drawImage(ZoomLevel::full, pos + Ui::Point(216, 0), ImageId(ImageIds::atari_logo_intro_right));
    }

    static void drawChrisSawyer(Gfx::DrawingContext& drawingCtx)
    {
        drawingCtx.clear(PaletteIndex::black0);

        const auto pos = Ui::Point(Ui::width() / 2 - 320 + 70, Ui::height() / 2 - 58);
        drawingCtx.drawImage(ZoomLevel::full, pos, ImageId(ImageIds::chris_sawyer_logo_intro_left));
        drawingCtx.drawImage(ZoomLevel::full, pos + Ui::Point(250, 0), ImageId(ImageIds::chris_sawyer_logo_intro_right));
    }

    static void drawNotice(Gfx::DrawingContext& drawingCtx)
    {
        auto tr = Gfx::TextRenderer(drawingCtx);

        drawingCtx.clear(PaletteIndex::black0);

        const auto pos = Ui::Point(Ui::width() / 2, Ui::height() / 2);

        tr.drawStringCentredWrapped(pos + Ui::Point(0, -80), Ui::width(), Colour::black, StringIds::intro_notice_0);
        tr.drawStringCentredWrapped(pos, Ui::width(), Colour::black, StringIds::intro_notice_1);

        auto noticePos = pos + Ui::Point(0, 80);
        for (auto& noticeId : kIntroNoticeStrings)
        {
            tr.drawStringCentred(noticePos, Colour::black, noticeId);
            noticePos += Ui::Point(0, 10);
        }
    }

    void draw(Gfx::DrawingContext& drawingCtx)
    {
        switch (_state)
        {
            case State::displayAtari:
                drawAtari(drawingCtx);
                break;
            case State::displayCS:
                drawChrisSawyer(drawingCtx);
                break;
            case State::displayNotice:
                drawNotice(drawingCtx);
                break;
            default:
                drawingCtx.clear(PaletteIndex::black0);
                break;
        }
    }
}
