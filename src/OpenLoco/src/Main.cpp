#include "OpenLoco.h"
#include <OpenLoco/Platform/Platform.h>
#include <SDL2/SDL_main.h>

int main(int argc, const char** argv)
{
    SDL_SetMainReady();
    return OpenLoco::main(OpenLoco::Platform::getCmdLineVector(argc, argv));
}
