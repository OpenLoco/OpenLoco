#include "Title.h"
#include "Audio/Audio.h"
#include "CompanyManager.h"
#include "Gui.h"
#include "Interop/Interop.hpp"
#include "Map/TileManager.h"
#include "OpenLoco.h"
#include "Scenario.h"
#include "Ui/WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Title
{
    static loco_global<uint16_t*, 0x009DA3CC> _currentTitleCommand;
    static loco_global<uint16_t, 0x009DA3D0> _waitCounter;

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
        _currentTitleCommand = (uint16_t*)0x4FB1F3;
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

        if (_waitCounter != 0)
        {
            _waitCounter = _waitCounter - 1;
            return;
        }

        do
        {
            uint16_t cmd = *(*_currentTitleCommand);
            _currentTitleCommand++;

            switch (cmd)
            {
                case 0: // wait(duration)
                {
                    uint16_t arg = *(*_currentTitleCommand);
                    _currentTitleCommand++;
                    _waitCounter = arg / 4;
                    break;
                }

                case 1: // reload
                {
                    uint16_t arg;
                    do
                    {
                        arg = *(*_currentTitleCommand);
                        _currentTitleCommand++;
                    } while (arg != 0);

                    loadTitle();
                    Gfx::invalidateScreen();
                    resetScreenAge();
                    addr<0x50C19A, uint16_t>() = 55000;
                    break;
                }

                case 2: // move(x, y)
                {
                    uint16_t argA = *(*_currentTitleCommand);
                    _currentTitleCommand++;
                    uint16_t argB = *(*_currentTitleCommand);
                    _currentTitleCommand++;

                    if (addr<0x00525E28, uint32_t>() & 1)
                    {
                        auto pos = Map::map_pos(argA * 32 + 16, argB * 32 + 16);
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

                    break;
                }

                case 3: // rotate()
                {
                    if (addr<0x00525E28, uint32_t>() & 1)
                    {
                        auto main = Ui::WindowManager::getMainWindow();
                        if (main != nullptr)
                        {
                            main->viewportRotateRight();
                        }
                    }

                    break;
                }

                case 4: // reset
                {
                    _currentTitleCommand = (uint16_t*)0x4FB1F3;
                    break;
                }
            }
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
