#include "S5.h"
#include "../Interop/Interop.hpp"

using namespace OpenLoco::Interop;

namespace OpenLoco::S5
{
    static loco_global<Options, 0x009C8714> _activeOptions;
    static loco_global<Header, 0x009CCA34> _header;
    static loco_global<Options, 0x009CCA54> _previewOptions;
    static loco_global<uint32_t, 0x009D1C9C> _saveFlags;
    static loco_global<char[512], 0x0112CE04> _savePath;

    Options& getOptions()
    {
        return _activeOptions;
    }

    Options& getPreviewOptions()
    {
        return _previewOptions;
    }

    // 0x00441C26
    bool save(const fs::path& path, SaveFlags flags)
    {
        // Copy UTF-8 path into filename buffer
        auto path8 = path.u8string();
        if (path8.size() >= std::size(_savePath))
        {
            std::fprintf(stderr, "Save path is too long: %s\n", path8.c_str());
            return false;
        }
        std::strncpy(_savePath, path8.c_str(), std::size(_savePath));

        if (flags & SaveFlags::noWindowClose)
        {
            // TODO: Remove ghost elements before saving to file

            // Skip the close construction window call
            // We have skipped the _saveFlags = eax instruction, so do this here
            _saveFlags = flags;
            return call(0x00441C3C) & (1 << 8);
        }
        else
        {
            // Normal entry
            registers regs;
            regs.eax = flags;
            return call(0x00441C26, regs) & (1 << 8);
        }
    }
}
