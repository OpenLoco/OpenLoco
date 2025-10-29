#include <cstdint>

namespace OpenLoco::S5
{
#pragma pack(push, 1)
    struct TileElement
    {
    private:
        static constexpr uint8_t FLAG_GHOST = 1 << 4;
        static constexpr uint8_t FLAG_LAST = 1 << 7;

    public:
        uint8_t type;
        uint8_t flags;
        uint8_t baseZ;
        uint8_t clearZ;
        uint8_t pad_4[4];

        void setLast(bool value)
        {
            if (value)
            {
                flags |= FLAG_LAST;
            }
            else
            {
                flags &= ~FLAG_LAST;
            }
        }

        constexpr bool isGhost() const
        {
            return flags & FLAG_GHOST;
        }

        constexpr bool isLast() const
        {
            return flags & FLAG_LAST;
        }
    };
    static_assert(sizeof(TileElement) == 8);
#pragma pack(pop)
}
