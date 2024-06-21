#include "Intro.h"
#include "Audio/Audio.h"
#include "Graphics/Colour.h"
#include "Graphics/ImageIds.h"
#include "Graphics/RenderTarget.h"
#include "Graphics/SoftwareDrawingEngine.h"
#include "Graphics/TextRenderer.h"
#include "Gui.h"
#include "Localisation/StringIds.h"
#include "MultiPlayer.h"
#include "OpenLoco.h"
#include "Title.h"
#include "Ui.h"
#include "ViewportManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Intro
{
    static loco_global<int32_t, 0x0050C190> _50C190;
    static loco_global<State, 0x0050C195> _state;
    static loco_global<bool, 0x0050C196> _50C196;

    bool isActive()
    {
        return state() != State::none;
    }

    State state()
    {
        return *_state;
    }

    void state(State state)
    {
        _state = state;
    }

    static void updateEnd()
    {
        auto& drawingEngine = Gfx::getDrawingEngine();
        drawingEngine.getDrawingContext().clearSingle(drawingEngine.getScreenRT(), PaletteIndex::index_0A);
        if (!_50C196)
        {
            // Audio::stopIntro(); Note: There is no sound!
            _50C196 = false;
        }
        _state = State::end2;
        _50C190 = 0;
    }

    static void updateEnd2()
    {
        _state = State::none;
        Gfx::loadDefaultPalette();
        addr<0x0005252E0, int32_t>() = 0;
        Gfx::invalidateScreen();
        initialiseViewports();
        Gui::init();
        Title::reset();
    }

    static void updateNone() {}

    static void updateDisplayNoticeBegin()
    {
        Gfx::loadPalette(ImageIds::default_palette, 0);
        _state = State::displayNoticeBeginReset;
    }

    static void updateBegin()
    {
        if (MultiPlayer::hasFlag(MultiPlayer::flag_8) || MultiPlayer::hasFlag(MultiPlayer::flag_9))
        {
            updateDisplayNoticeBegin();
            return;
        }

        Gfx::loadPalette(ImageIds::atari_intro_palette, 0);

        auto& drawingEngine = Gfx::getDrawingEngine();
        auto& rt = drawingEngine.getScreenRT();
        auto& drawContext = drawingEngine.getDrawingContext();

        drawContext.clearSingle(rt, PaletteIndex::index_3F);

        const auto pos = Ui::Point(Ui::width() / 2 - 216, Ui::height() / 2 - 54);
        drawContext.drawImage(rt, pos, ImageId(ImageIds::atari_logo_intro_left));
        drawContext.drawImage(rt, pos + Ui::Point(216, 0), ImageId(ImageIds::atari_logo_intro_right));
        _50C190 = -24;
        _50C196 = false;
        _state = State::displayAtari;
    }

    static void updateDisplayAtari()
    {
        _50C190++;
        uint8_t modifier = 0;
        if (_50C190 >= 0)
        {
            modifier = _50C190;
            if (_50C190 >= 55)
            {
                modifier = 110 - _50C190;
            }
        }
        modifier = std::min(255, modifier * 8);

        Gfx::loadPalette(ImageIds::atari_intro_palette, modifier);

        if (_50C190 >= 110)
        {
            Gfx::loadPalette(ImageIds::chris_sawyer_intro_palette, 0);

            auto& drawingEngine = Gfx::getDrawingEngine();
            auto& rt = drawingEngine.getScreenRT();
            auto& drawContext = drawingEngine.getDrawingContext();

            drawContext.clearSingle(rt, PaletteIndex::index_0A);

            const auto pos = Ui::Point(Ui::width() / 2 - 320 + 70, Ui::height() / 2 - 58);
            drawContext.drawImage(rt, pos, ImageId(ImageIds::chris_sawyer_logo_intro_left));
            drawContext.drawImage(rt, pos + Ui::Point(250, 0), ImageId(ImageIds::chris_sawyer_logo_intro_right));

            _50C190 = 0;
            _state = State::displayCS;
        }
    }
    static void updateDisplayCS()
    {
        _50C190++;
        uint8_t modifier = 0;
        if (_50C190 >= 0)
        {
            modifier = _50C190;
            if (_50C190 >= 50)
            {
                modifier = 100 - _50C190;
            }
        }
        modifier = std::min(255, modifier * 8);

        Gfx::loadPalette(ImageIds::chris_sawyer_intro_palette, modifier);
        if (_50C190 >= 100)
        {
            auto& drawingEngine = Gfx::getDrawingEngine();
            auto& rt = drawingEngine.getScreenRT();
            auto& drawContext = drawingEngine.getDrawingContext();

            drawContext.clearSingle(rt, PaletteIndex::index_0A);

            _state = State::displayNoticeBegin;
        }
    }
    static void updateState4() {}
    static void updateState5() {}
    static void updateState6() {}
    static void updateState7() {}

    constexpr std::array<StringId, 5> kIntroNoticeStrings = {
        StringIds::intro_vehicle_notice_0,
        StringIds::intro_vehicle_notice_1,
        StringIds::intro_vehicle_notice_2,
        StringIds::intro_vehicle_notice_3,
        StringIds::intro_vehicle_notice_4,
    };

    static void updateDisplayNotice()
    {
        auto& drawingEngine = Gfx::getDrawingEngine();
        auto& rt = drawingEngine.getScreenRT();
        auto& drawContext = drawingEngine.getDrawingContext();
        auto tr = Gfx::TextRenderer(drawContext);

        drawContext.clearSingle(rt, PaletteIndex::index_0A);

        const auto pos = Ui::Point(Ui::width() / 2, Ui::height() / 2);

        tr.drawStringCentredWrapped(rt, pos + Ui::Point(0, -80), Ui::width(), Colour::black, StringIds::intro_notice_0);
        tr.drawStringCentredWrapped(rt, pos, Ui::width(), Colour::black, StringIds::intro_notice_1);

        auto noticePos = pos + Ui::Point(0, 80);
        for (auto& noticeId : kIntroNoticeStrings)
        {
            tr.drawStringCentred(rt, noticePos, Colour::black, noticeId);
            noticePos += Ui::Point(0, 10);
        }

        if (_50C190 == 0)
        {
            Gfx::loadPalette(ImageIds::default_palette, 0xFF);
        }
        _50C190++;

        if (MultiPlayer::hasFlag(MultiPlayer::flag_8) || MultiPlayer::hasFlag(MultiPlayer::flag_9))
        {
            if (_50C190 >= 160)
            {
                _state = State::end;
            }
        }
        else
        {
            if (_50C190 >= 1440)
            {
                _state = State::end;
            }
        }
    }
    static void updateDisplayNoticeBeginReset()
    {
        _50C190 = 0;
        _state = State::displayNotice;
        updateDisplayNotice();
    }

    constexpr std::array<void (*)(), 11> kUpdateFunctions = {
        updateNone,
        updateBegin,
        updateDisplayAtari,
        updateDisplayCS,
        updateState4,
        updateState5,
        updateState6,
        updateState7,
        updateDisplayNoticeBegin,
        updateDisplayNotice,
        updateDisplayNoticeBeginReset,
    };

    // 0x0046AE0C
    void update()
    {
        addr<0x0005252E0, int32_t>() = 1;
        if (_state == State::end)
        {
            updateEnd();
        }
        else if (_state == State::end2)
        {
            updateEnd2();
        }
        else if (enumValue(*_state) < std::size(kUpdateFunctions))
        {
            kUpdateFunctions[enumValue(*_state)]();
        }
        sub_431695(0);
    }
}
