#include "Interop.hpp"
#include <OpenLoco/Diagnostics/Logging.h>

using namespace OpenLoco::Diagnostics;

namespace OpenLoco::Interop
{
    GlobalStore GlobalStore::gStoreInstance = GlobalStore();

    GlobalStore& GlobalStore::getInstance()
    {
        return gStoreInstance;
    }

    void GlobalStore::addAddressRange([[maybe_unused]] uint32_t begin, [[maybe_unused]] uint32_t size)
    {
#ifdef _LOG_GLOBAL_STORE_
        auto& gs = getInstance();
        if (gs.isAddressInRange(begin, size))
        {
            Logging::warn("Address range {:#08x}-{:#08x} overlaps with existing ranges.\n", begin, begin + size);
            return;
        }
        gs.addressRanges.emplace_back(begin, size);
#endif
    }

    bool GlobalStore::isAddressInRange(uint32_t address, uint32_t size) const
    {
        for (auto& range : addressRanges)
        {
            if (address >= range.first && address < (range.first + range.second))
            {
                return true;
            }
        }
        const auto end = address + size;
        for (auto& range : addressRanges)
        {
            if (end > range.first && end <= (range.first + range.second))
            {
                return true;
            }
        }
        return false;
    }

}
