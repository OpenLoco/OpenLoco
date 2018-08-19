#include <string>

namespace openloco::localisation
{
    namespace languages
    {
        enum
        {
            undefined,
            german,
            english_uk,
            english_us,
            spanish,
            french,
            italian,
            dutch,
            portuguese_br,
            count
        };
    }

    enum class loco_language_id
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
        const std::string locale;
        const std::string english_name;
        const std::string native_name;
        loco_language_id loco_original_id;
    };

    extern const language_descriptor language_descriptors[languages::count];

    const language_descriptor& getDescriptorForLanguage(std::string target_locale);
}
