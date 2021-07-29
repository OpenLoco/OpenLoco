#include "SubpositionData.h"
#include "Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::Map::TrackData
{
    static loco_global<const MoveInfo* [352], 0x04D9724> _4D9724; // 44 trackId's * 8 directions
    static loco_global<const MoveInfo* [80], 0x04D9CA4> _4D9CA4; // 10 roadId's * 8 directions

    const std::vector<MoveInfo> getTrackSubPositon(uint16_t trackAndDirection)
    {
        auto* moveInfoStart = _4D9724[trackAndDirection];
        auto moveInfoSize = *(reinterpret_cast<const uint16_t*>(moveInfoStart) - 1);
        std::vector<MoveInfo> moveInfoArr;
        moveInfoArr.resize(moveInfoSize);
        for (auto i = 0; i < moveInfoSize; ++i)
        {
            moveInfoArr[i] = moveInfoStart[i];
        }
        return moveInfoArr;
    }

    const std::vector<MoveInfo> getRoadSubPositon(uint16_t trackAndDirection)
    {
        auto* moveInfoStart = _4D9CA4[trackAndDirection];
        auto moveInfoSize = *(reinterpret_cast<const uint16_t*>(moveInfoStart) - 1);
        std::vector<MoveInfo> moveInfoArr;
        moveInfoArr.resize(moveInfoSize);
        for (auto i = 0; i < moveInfoSize; ++i)
        {
            moveInfoArr[i] = moveInfoStart[i];
        }
        return moveInfoArr;
    }
}
