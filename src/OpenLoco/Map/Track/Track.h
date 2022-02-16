#include "../../Types.hpp"
#include "../Map.hpp"

namespace OpenLoco::Map::Track
{
    struct TrackConnections
    {
        uint32_t size;
        uint16_t data[16];
        void push_back(uint16_t value)
        {
            if (size + 1 < std::size(data))
            {
                data[size++] = value;
                data[size] = 0xFFFF;
            }
        }
        uint16_t pop_back()
        {
            return data[--size];
        }
    };
    static_assert(sizeof(TrackConnections) == 0x24);

    void getRoadConnections(const Map::Pos3& pos, TrackConnections& data, const CompanyId company, const uint8_t roadObjectId, const uint16_t trackAndDirection);
    void getTrackConnections(const Map::Pos3& nextTrackPos, const uint8_t nextRotation, TrackConnections& data, const CompanyId company, const uint8_t trackObjectId);
    std::pair<Map::Pos3, uint8_t> getTrackConnectionEnd(const Map::Pos3& pos, const uint16_t trackAndDirection);
}
