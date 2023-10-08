#include "ObjectStringTable.h"
#include "Config.h"
#include "Localisation/Languages.h"
#include "Localisation/StringIds.h"
#include "Localisation/StringManager.h"
#include "ObjectManager.h"

namespace OpenLoco::ObjectManager
{
    constexpr std::array<StringId, 16> kTemporaryObjectStringIds = {
        StringIds::temporary_object_load_str_0,
        StringIds::temporary_object_load_str_1,
        StringIds::temporary_object_load_str_2,
        StringIds::temporary_object_load_str_3,
        StringIds::temporary_object_load_str_4,
        StringIds::temporary_object_load_str_5,
        StringIds::temporary_object_load_str_6,
        StringIds::temporary_object_load_str_7,
        StringIds::temporary_object_load_str_8,
        StringIds::temporary_object_load_str_9,
        StringIds::temporary_object_load_str_10,
        StringIds::temporary_object_load_str_11,
        StringIds::temporary_object_load_str_12,
        StringIds::temporary_object_load_str_13,
        StringIds::temporary_object_load_str_14,
        StringIds::temporary_object_load_str_15,
    };

    constexpr std::array<uint8_t, 34> kNumStringsPerObjectType = {
        1,
        1,
        3,
        1,
        1,
        1,
        1,
        1,
        4,
        1,
        2,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        8,
        1,
        2,
        2,
    };

    // 0x00472172
    StringTableResult loadStringTable(std::span<const std::byte> data, const LoadedObjectHandle& handle, uint8_t index)
    {
        StringTableResult res;
        auto iter = data.begin();
        const char* engBackupStr = nullptr;
        const char* anyStr = nullptr;
        const char* targetStr = nullptr;
        const auto targetLang = Localisation::getDescriptorForLanguage(Config::get().language).locoOriginalId;
        for (; iter != data.end() && *iter != static_cast<std::byte>(0xFF); ++iter)
        {
            const auto lang = static_cast<Localisation::LocoLanguageId>(*iter++);
            const auto str = reinterpret_cast<const char*>(&*iter);
            if (lang == Localisation::LocoLanguageId::english_uk)
            {
                engBackupStr = str;
            }
            else if (lang == Localisation::LocoLanguageId::english_us && engBackupStr == nullptr)
            {
                engBackupStr = str;
            }
            if (lang == targetLang)
            {
                targetStr = str;
            }
            if (engBackupStr == nullptr && targetStr == nullptr)
            {
                anyStr = str;
            }
            iter += strlen(str);
        }
        iter++;
        res.tableLength = std::distance(data.begin(), iter);
        const auto* chosenStr = [=]() {
            if (targetStr != nullptr)
            {
                return targetStr;
            }
            if (engBackupStr != nullptr)
            {
                return engBackupStr;
            }
            return anyStr;
        }();

        if (isTemporaryObjectLoad())
        {
            res.str = kTemporaryObjectStringIds[index];
            StringManager::swapString(res.str, chosenStr);
            return res;
        }

        res.str = StringIds::object_strings_begin + index;
        for (auto objType = ObjectType::interfaceSkin; enumValue(objType) < enumValue(handle.type); objType = static_cast<ObjectType>(enumValue(objType) + 1))
        {
            res.str += static_cast<uint16_t>(getMaxObjects(objType)) * kNumStringsPerObjectType[enumValue(objType)];
        }
        res.str += kNumStringsPerObjectType[enumValue(handle.type)] * handle.id;

        StringManager::swapString(res.str, chosenStr);
        return res;
    }
}
