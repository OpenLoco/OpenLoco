#include "ProgressBar.h"
#include "../Interop/Interop.hpp"
#include "../Localisation/FormatArguments.hpp"
#include "../Localisation/StringManager.h"
#include "../OpenLoco.h"
#include "WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ProgressBar
{
    static bool _isInternalWindow = false;
    static char _captionBuffer[256] = {};

    // 0x004CF5DA
    static void loadingWindowCreate(string_id captionStringId)
    {
        // TODO: platform-specific implementation?
        registers regs;
        regs.eax = captionStringId;
        call(0x004CF5DA, regs);
    }

    // 0x004CF5C5
    void begin(string_id captionStringId)
    {
        if (isInitialised())
        {
            _isInternalWindow = true;
            if (captionStringId != StringIds::null)
            {
                auto args = FormatArguments::common();
                StringManager::formatString(_captionBuffer, std::size(_captionBuffer), captionStringId, &args);
            }
            Windows::ProgressBar::open(_captionBuffer);
        }
        else
        {
            loadingWindowCreate(captionStringId);
        }
    }

    // 0x004CF631
    // TODO: platform-specific implementation?
    static void updateLoadingWindow(uint8_t value)
    {
        registers regs;
        regs.eax = value;
        call(0x004CF631, regs);
    }

    // 0x004CF63B
    void sub_4CF63B()
    {
        call(0x004CF63B);
    }

    // 0x004CF621
    // eax: value
    void setProgress(int32_t value)
    {
        if (_isInternalWindow)
        {
            Windows::ProgressBar::setProgress(value);
        }
        else
        {
            updateLoadingWindow(value);
        }
    }

    // 0x00408163
    // TODO: platform-specific implementation?
    static void destroyLoadingWindow()
    {
        call(0x00408163);
    }

    // 0x004CF60B
    void end()
    {
        if (_isInternalWindow)
        {
            Windows::ProgressBar::close();
        }
        else
        {
            destroyLoadingWindow();
        }
    }

    void registerHooks()
    {
        registerHook(
            0x004CF5C5,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                begin(regs.eax);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004CF621,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                setProgress(regs.eax);
                regs = backup;
                return 0;
            });

        registerHook(
            0x004CF60B,
            [](registers& regs) FORCE_ALIGN_ARG_POINTER -> uint8_t {
                registers backup = regs;
                end();
                regs = backup;
                return 0;
            });
    }
}
