#include "Title.h"
#include "Audio/Audio.h"
#include "Config.h"
#include "Environment.h"
#include "Game.h"
#include "GameCommands/GameCommands.h"
#include "GameCommands/General/TogglePause.h"
#include "GameState.h"
#include "GameStateFlags.h"
#include "Gui.h"
#include "Intro.h"
#include "Map/TileManager.h"
#include "MessageManager.h"
#include "Objects/ObjectIndex.h"
#include "Objects/ObjectManager.h"
#include "OpenLoco.h"
#include "S5/S5.h"
#include "Scenario.h"
#include "SceneManager.h"
#include "Ui/WindowManager.h"
#include "World/CompanyManager.h"
#include <OpenLoco/Interop/Interop.hpp>

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

    using MoveStep = World::TilePos2;

    struct RotateStep
    {
    };

    struct ResetStep
    {
    };

    using TitleStep = std::variant<WaitStep, ReloadStep, MoveStep, RotateStep, ResetStep>;
    using TitleSequence = std::vector<TitleStep>;

    // Helper type for using stdx::visit on TitleStep
    template<class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };

    // Explicit deduction guide (not needed as of C++20)
    template<class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

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
    static loco_global<ObjectManager::SelectedObjectsFlags*, 0x50D144> _objectSelection;

    static std::span<ObjectManager::SelectedObjectsFlags> getSelectedObjectFlags()
    {
        return std::span<ObjectManager::SelectedObjectsFlags>(*_objectSelection, ObjectManager::getNumInstalledObjects());
    }

    // 0x004442C4
    static void loadTitle()
    {
        Scenario::sub_46115C();
        if (Intro::state() == Intro::State::none)
        {
            uint16_t backupWord = getGameState().var_014A;
            auto titlePath = Environment::getPath(Environment::PathId::title);
            clearScreenFlag(ScreenFlags::networked);
            S5::importSaveToGameState(titlePath, S5::LoadFlags::titleSequence);

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

            getGameState().var_014A = backupWord;
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
    void reset()
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
            registers regs;
            regs.bl = GameCommands::Flags::apply;
            GameCommands::togglePause(regs);
        }

        auto currentScreenFlags = getScreenFlags();
        clearScreenFlag(ScreenFlags::networked);
        Ui::WindowManager::closeAllFloatingWindows();
        setAllScreenFlags(currentScreenFlags);
        setScreenFlag(ScreenFlags::title);
        setGameSpeed(GameSpeed::Normal);
        ObjectManager::unloadAll();
        ObjectManager::prepareSelectionList(false);
        ObjectManager::loadSelectionListObjects(getSelectedObjectFlags());
        ObjectManager::freeSelectionList();
        ObjectManager::reloadAll();
        Scenario::sub_4748D4();
        Scenario::reset();
        initialiseViewports();
        MessageManager::reset();
        Gui::init();
        reset();
        Gfx::invalidateScreen();
        resetScreenAge();

        if (Config::get().audio.playTitleMusic)
        {
            Audio::playMusic(Environment::PathId::css5, Config::get().old.volume, true);
        }
    }

    void stop()
    {
        Audio::stopMusic();
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
            std::visit(overloaded{
                           []([[maybe_unused]] WaitStep step) {
                               // This loop slightly deviates from the original, subtract 1 tick to make up for it.
                               _waitCounter = step.duration - 1;
                           },
                           []([[maybe_unused]] ReloadStep step) {
                               reload();
                           },
                           [](MoveStep step) {
                               if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
                               {
                                   auto pos = World::toWorldSpace(step) + World::Pos2(16, 16);
                                   auto height = World::TileManager::getHeight(pos);
                                   auto main = Ui::WindowManager::getMainWindow();
                                   if (main != nullptr)
                                   {
                                       auto pos3d = World::Pos3(pos.x, pos.y, height.landHeight);
                                       main->viewportCentreOnTile(pos3d);
                                       main->flags &= ~Ui::WindowFlags::scrollingToLocation;
                                       main->viewportsUpdatePosition();
                                   }
                               }
                           },
                           []([[maybe_unused]] RotateStep step) {
                               if (Game::hasFlags(GameStateFlags::tileManagerLoaded))
                               {
                                   auto main = Ui::WindowManager::getMainWindow();
                                   if (main != nullptr)
                                   {
                                       main->viewportRotateRight();
                                   }
                               }
                           },
                           []([[maybe_unused]] ResetStep step) {
                               _sequenceIterator = _titleSequence.begin();
                           },
                       },
                       command);
        } while (_waitCounter == 0);
    }

    void registerHooks()
    {
        registerHook(
            0x0046AD7D,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                start();
                regs = backup;
                return 0;
            });
        registerHook(
            0x004442C4,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                loadTitle();
                regs = backup;
                return 0;
            });
    }
}
