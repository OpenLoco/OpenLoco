#include "OpenLoco.h"
#include <OpenLoco/Platform/Platform.h>
#include <SDL2/SDL_main.h>

int main(int argc, const char** argv)
{
#ifdef WIN32
    // Ensures that assert dialogs allow for ignoring them (not the default behaviour for console subsystem)
    _set_error_mode(_OUT_TO_MSGBOX);
#endif
    SDL_SetMainReady();
    return OpenLoco::main(OpenLoco::Platform::getCmdLineVector(argc, argv));
}
