#include <string>
#include <vector>

namespace openloco::localisation
{
    enum class loco_language_id : uint8_t
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

    struct language_descriptor
    {
        std::string locale;
        std::string english_name;
        std::string native_name;
        loco_language_id loco_original_id;
    };

    void enumerateLanguages();
    std::vector<language_descriptor>& getLanguageDescriptors();
    const language_descriptor& getDescriptorForLanguage(std::string target_locale);
}
