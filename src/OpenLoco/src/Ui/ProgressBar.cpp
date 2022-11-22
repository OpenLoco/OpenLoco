#include "ProgressBar.h"
#include "Interop/Interop.hpp"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/StringManager.h"
#include "SceneManager.h"
#include "WindowManager.h"

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ProgressBar
{
    static bool _isInternalWindow = false;

    // 0x004CF5DA
    static void loadingWindowCreate(std::string_view caption)
    {
        // TODO: platform-specific implementation?
        static loco_global<char[512], 0x0112CC04> _stringFormatBuffer;
        memset(_stringFormatBuffer, 0, 512);
        caption.copy(_stringFormatBuffer, caption.size());

        registers regs;
        call(0x004CF5EE, regs);
    }

    void begin(std::string_view caption)
    {
        if (isInitialised())
        {
            _isInternalWindow = true;
            Windows::ProgressBar::open(caption);
        }
        else
        {
            loadingWindowCreate(caption);
        }
    }

    // 0x004CF5C5
    void begin(string_id captionStringId)
    {
        char _captionBuffer[256] = {};
        if (captionStringId != StringIds::null)
        {
            auto args = FormatArguments::common();
            StringManager::formatString(_captionBuffer, std::size(_captionBuffer), captionStringId, &args);
        }
        begin(_captionBuffer);
    }

    // 0x004CF631
    // TODO: platform-specific implementation?
    static void updateLoadingWindow(uint8_t value)
    {
        registers regs;
        regs.eax = value;
        call(0x004CF631, regs);
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
