#include "system_message_popup.h"
#include "../Ui.h"

namespace OpenLoco
{
    namespace platform
    {
        bool system_message_popup(const std::string& title, std::stringstream& message_as_stream)
        {
            return system_message_popup(title, message_as_stream.str());
        }
    }
}

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
//Win32 Error message handler

#include <Windows.h>

namespace OpenLoco
{
    namespace platform
    {
        bool system_message_popup(const std::string& title, const std::string& message_to_display)
        {
            MessageBox(nullptr, TEXT(message_to_display.c_str()), TEXT(title.c_str()), MB_OK);

            return true;
        }
    }
}

//Uncomment for additional platform implementations:

//#ifdef _WIN64
////define something for Windows (64-bit only)
//#else
////define something for Windows (32-bit only)
//#endif

//#elif __APPLE__
//#include <TargetConditionals.h>
//#if TARGET_OS_MAC
//// Other kinds of Mac OS
//#else
//#error "Unknown Apple platform"
//#endif

//#elif __linux__
//// linux

//#elif __unix__ // all unices not caught above
//// Unix

//default to SDL message box if system call not implemented
#else

namespace OpenLoco
{
    namespace platform
    {
        bool system_message_popup(const std::string& title, const std::string& message_to_display)
        {
            Ui::showMessageBox(title, message_to_display);
            return true;
        }
    }
}

#endif
