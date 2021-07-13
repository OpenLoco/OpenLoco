#include <string>
#include <vector>

namespace OpenLoco::Localisation
{
    enum class LocoLanguageId : uint8_t
    {
        english_uk,
        english_us,
        french,
        german,
        spanish,
        italian,
        dutch,
        swedish,
        japanese,
        korean,
        chinese_simplified,
        chinese_traditional,
        id_12,
        portuguese,
        blank = 254,
        end = 255
    };

    struct LanguageDescriptor
    {
        std::string locale;
        std::string english_name;
        std::string native_name;
        LocoLanguageId loco_original_id;
    };

    void enumerateLanguages();
    std::vector<LanguageDescriptor>& getLanguageDescriptors();
    const LanguageDescriptor& getDescriptorForLanguage(std::string target_locale);
}
