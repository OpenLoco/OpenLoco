#include "Random.h"
#include "GameState.h"

namespace OpenLoco
{
    Core::Prng& gPrng1()
    {
        return getGameState().rng;
    }

    Core::Prng& gPrng2()
    {
        return getGameState().unkRng;
    }
}
