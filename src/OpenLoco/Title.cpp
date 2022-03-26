#include "Title.h"
#include "Audio/Audio.h"
#include "CompanyManager.h"
#include "Core/Variant.hpp"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "Gui.h"
#include "Interop/Interop.hpp"
#include "Intro.h"
#include "Map/TileManager.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "Ui/WindowManager.h"

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

    using MoveStep = Map::TilePos2;

    struct RotateStep
    {
    };

    struct ResetStep
    {
    };

    using TitleStep = stdx::variant<WaitStep, ReloadStep, MoveStep, RotateStep, ResetStep>;
    using TitleSequence = std::vector<TitleStep>;

    // Helper type for using stdx::visit on TitleStep
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

    static loco_global<uint16_t, 0x0050C19A> _50C19A;
    static loco_global<uint16_t, 0x00525F62> _525F62;

    static void sub_473A95(int32_t eax);

    // 0x00474874
    // ?load selected objects?
    static void sub_474874()
    {
        call(0x00474874); // editorLoadSelectedObjects
    }

    // 0x00473B91
    // object flags free 0
    static void sub_473B91()
    {
        call(0x00473B91); // editorObjectFlagsFree0
    }

    // 0x004284C8
    void sub_4284C8()
    {
        call(0x004284C8);
    }

    // 0x004442C4
    static void loadTitle()
    {
        Scenario::sub_46115C();
        if (Intro::state() == Intro::State::none)
        {
            auto backupWord = _525F62;
            auto titlePath = Environment::getPath(Environment::PathId::title);
            clearScreenFlag(ScreenFlags::networked);
            S5::load(titlePath, S5::LoadFlags::titleSequence);

            CompanyManager::setControllingId(CompanyId(0));
            CompanyManager::setSecondaryPlayerId(CompanyId::null);
            if (!isNetworked())
            {
                CompanyManager::setSecondaryPlayerId(CompanyId(1));
                if (!isNetworkHost())
                {
                    CompanyManager::setControllingId(CompanyId(1));
                    CompanyManager::setSecondaryPlayerId(CompanyId(0));
                }
            }

            _525F62 = backupWord;
        }
    }

    static void reload()
    {
        loadTitle();
        resetScreenAge();
        _50C19A = 55000;
        update();
    }

    // 0x00444357
    static void reset()
    {
        _sequenceIterator = _titleSequence.begin();
        _waitCounter = 0;
        reload();
    }

    // 0x0046AD7D
    void start()
    {
        CompanyManager::setUpdatingCompanyId(CompanyManager::getControllingId());
        if (isPaused())
        {
            GameCommands::togglePause(1);
        }

        auto currentScreenFlags = getScreenFlags();
        clearScreenFlag(ScreenFlags::networked);
        Ui::WindowManager::closeAllFloatingWindows();
        setAllScreenFlags(currentScreenFlags);
        setScreenFlag(ScreenFlags::title);
        setGameSpeed(GameSpeed::Normal);
        ObjectManager::unloadAll();
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
            if (_sequenceIterator >= _titleSequence.end())
                return;

            auto& command = *_sequenceIterator++;
            stdx::visit(overloaded{
                            [](WaitStep step) {
                                // This loop slightly deviates from the original, subtract 1 tick to make up for it.
                                _waitCounter = step.duration - 1;
                            },
                            [](ReloadStep step) {
                                reload();
                            },
                            [](MoveStep step) {
                                if (Game::hasFlags(1u << 0))
                                {
                                    auto pos = Map::Pos2(step) + Map::Pos2(16, 16);
                                    auto height = Map::TileManager::getHeight(pos);
                                    auto main = Ui::WindowManager::getMainWindow();
                                    if (main != nullptr)
                                    {
                                        auto pos3d = Map::Pos3(pos.x, pos.y, height.landHeight);
                                        main->viewportCentreOnTile(pos3d);
                                        main->flags &= ~Ui::WindowFlags::scrollingToLocation;
                                        main->viewportsUpdatePosition();
                                    }
                                }
                            },
                            [](RotateStep step) {
                                if (Game::hasFlags(1u << 0))
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

    void registerHooks()
    {
        registerHook(
            0x0046AD7D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                start();
                return 0;
            });
        registerHook(
            0x004442C4,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                loadTitle();
                return 0;
            });
    }
}
