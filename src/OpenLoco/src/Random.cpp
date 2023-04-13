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

    void recordTickStartPrng()
    {
        auto& gs = getGameState();
        gs.var_1B4 = gs.rng;
    }
}
