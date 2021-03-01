#include "Title.h"
#include "Audio/Audio.h"
#include "CompanyManager.h"
#include "Gui.h"
#include "Interop/Interop.hpp"
#include "Map/TileManager.h"
#include "OpenLoco.h"
#include "Scenario.h"
#include "Ui/WindowManager.h"

#include <variant>
#include <vector>

using namespace OpenLoco::Interop;

namespace OpenLoco::Title
{
    struct WaitStep
    {
        uint16_t duration;
    };

    struct ReloadStep
    {
        uint16_t arg;
    };

    using MoveStep = Map::TilePos;

    struct RotateStep
    {
    };

    struct ResetStep
    {
    };

    using TitleStep = std::variant<WaitStep, ReloadStep, MoveStep, RotateStep, ResetStep>;
    using TitleSequence = std::vector<TitleStep>;

    // Helper type for using std::visit on TitleStep
    template<class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };

    // Explicit deduction guide (not needed as of C++20)
    template<class... Ts>
    overloaded(Ts...)->overloaded<Ts...>;

    static const TitleSequence _titleSequence = {
        MoveStep{ 231, 160 },
        WaitStep{ 368 },
        MoveStep{ 93, 59 },
        WaitStep{ 370 },
        RotateStep{},
        MoveStep{ 314, 230 },
        WaitStep{ 370 },
        MoveStep{ 83, 314 },
        WaitStep{ 370 },
        RotateStep{},
        RotateStep{},
        MoveStep{ 224, 253 },
        WaitStep{ 375 },
        MoveStep{ 210, 322 },
        WaitStep{ 375 },
        MoveStep{ 125, 338 },
        WaitStep{ 375 },
        MoveStep{ 72, 138 },
        WaitStep{ 375 },
        RotateStep{},
        MoveStep{ 178, 158 },
        WaitStep{ 375 },
        MoveStep{ 203, 316 },
        WaitStep{ 375 },
        ReloadStep{ 0 },
        ResetStep{},
    };

    static TitleSequence::const_iterator _sequenceIterator;
    static uint16_t _waitCounter;

    static void sub_473A95(int32_t eax);

    void registerHooks()
    {
        registerHook(
            0x0046AD7D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                start();
                return 0;
            });
    }

    // 0x00472031
    // ?unload all objects?
    static void sub_472031()
    {
        call(0x00472031);
    }

    // 0x00474874
    // ?load selected objects?
    static void sub_474874()
    {
        call(0x00474874);
    }

    // 0x00473B91
    // object flags free 0
    static void sub_473B91()
    {
        call(0x00473B91);
    }

    // 0x004284C8
    void sub_4284C8()
    {
        call(0x004284C8);
    }

    static void loadTitle()
    {
        call(0x004442C4);
    }

    // 0x00444357
    static void reset()
    {
        _sequenceIterator = _titleSequence.begin();
        _waitCounter = 0;
        loadTitle();
        resetScreenAge();
        addr<0x50C19A, uint16_t>() = 55000;
        update();
    }

    // 0x0046AD7D
    void start()
    {
        CompanyManager::updatingCompanyId(CompanyManager::getControllingId());
        if (isPaused())
        {
            togglePause(true);
        }

        auto currentScreenFlags = getScreenFlags();
        clearScreenFlag(ScreenFlags::networked);
        Ui::WindowManager::closeAllFloatingWindows();
        setAllScreenFlags(currentScreenFlags);
        setScreenFlag(ScreenFlags::title);
        setGameSpeed(0);
        sub_472031();
        sub_473A95(1);
        sub_474874();
        sub_473B91();
        ObjectManager::reloadAll();
        Scenario::sub_4748D4();
        Scenario::reset();
        initialiseViewports();
        sub_4284C8();
        Gui::init();
        reset();
        Gfx::invalidateScreen();
        resetScreenAge();

        Audio::playTitleScreenMusic();
    }

    // 0x00444387
    void update()
    {
        if (!isTitleMode())
            return;

        resetScreenAge();

        if (_waitCounter > 0)
        {
            _waitCounter -= 1;
            return;
        }

        do
        {
            auto& command = *_sequenceIterator++;
            std::visit(overloaded{
                           [](WaitStep step) {
                               _waitCounter = step.duration / 4;
                           },
                           [](ReloadStep step) {
                               loadTitle();
                               Gfx::invalidateScreen();
                               resetScreenAge();
                               addr<0x50C19A, uint16_t>() = 55000;
                           },
                           [](MoveStep step) {
                               if (addr<0x00525E28, uint32_t>() & 1)
                               {
                                   auto pos = Map::map_pos(step) + Map::map_pos(16, 16);
                                   auto height = Map::TileManager::getHeight(pos);
                                   auto main = Ui::WindowManager::getMainWindow();
                                   if (main != nullptr)
                                   {
                                       auto pos3d = Map::map_pos3(pos, height.landHeight);
                                       main->viewportCentreOnTile(pos3d);
                                       main->flags &= ~Ui::WindowFlags::scrolling_to_location;
                                       main->viewportsUpdatePosition();
                                   }
                               }
                           },
                           [](RotateStep step) {
                               if (addr<0x00525E28, uint32_t>() & 1)
                               {
                                   auto main = Ui::WindowManager::getMainWindow();
                                   if (main != nullptr)
                                   {
                                       main->viewportRotateRight();
                                   }
                               }
                           },
                           [](ResetStep step) {
                               _sequenceIterator = _titleSequence.begin();
                           },
                       },
                       command);
        } while (_waitCounter == 0);
    }

    // 0x00473A95
    static void sub_473A95(int32_t eax)
    {
        registers regs;
        regs.eax = eax;
        call(0x00473A95, regs);
    }
}
