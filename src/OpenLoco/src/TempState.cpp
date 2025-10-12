#include "TempState.h"
#include <cstdlib>

namespace OpenLoco
{
    bool tempInit = false;
    TempState* _tempState;

    TempState* GetTempState()
    {
        return _tempState;
    }

    void ResetTempState() {
        if (!tempInit)
        {
            _tempState = (TempState*)calloc(1, sizeof(TempState));
            tempInit = true;
        }
        else
        {
            free(_tempState);
            tempInit = false;
            ResetTempState();
        }
    }
}
