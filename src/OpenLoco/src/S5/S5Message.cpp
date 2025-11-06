#include "S5Message.h"
#include "Message.h"
#include <ranges>

namespace OpenLoco::S5
{

    S5::Message exportMessage(OpenLoco::Message& src)
    {
        S5::Message dst{};
        dst.type = enumValue(src.type);
        std::memcpy(dst.messageString, src.messageString, sizeof(dst.messageString));
        dst.companyId = enumValue(src.companyId);
        dst.timeActive = src.timeActive;
        std::ranges::copy(src.itemSubjects, dst.itemSubjects);
        dst.date = src.date;
        return dst;
    }

}
