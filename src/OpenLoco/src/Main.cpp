#include "OpenLoco.h"
#include <OpenLoco/Platform/Platform.h>

int main(int argc, const char** argv)
{
    return OpenLoco::main(OpenLoco::Platform::getCmdLineVector(argc, argv));
}
