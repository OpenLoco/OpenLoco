#include "ProgressBar.h"
#include "Localisation/FormatArguments.hpp"
#include "Localisation/Formatting.h"
#include "Localisation/StringManager.h"
#include "SceneManager.h"
#include "WindowManager.h"
#include <OpenLoco/Interop/Interop.hpp>

using namespace OpenLoco::Interop;

namespace OpenLoco::Ui::ProgressBar
{
    static bool _isInternalWindow = false;

    void begin(std::string_view caption)
    {
        _isInternalWindow = true;
        Windows::ProgressBar::open(caption);
    }

    // 0x004CF5C5
    void begin(StringId captionStringId)
    {
        char _captionBuffer[256] = {};
        if (captionStringId != StringIds::null)
        {
            auto args = FormatArguments::common();
            StringManager::formatString(_captionBuffer, std::size(_captionBuffer), captionStringId, args);
        }
        begin(_captionBuffer);
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
            // Used to be the native progress bar
            assert(false);
        }
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
            // Used to be the native progress bar
            assert(false);
        }
    }
}
