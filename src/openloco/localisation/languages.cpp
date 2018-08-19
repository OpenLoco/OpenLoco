#include "languages.h"

namespace openloco::localisation
{
    // clang-format off
    const language_descriptor language_descriptors[languages::count] =
    {
        { "",       "",               "",                 loco_language_id::english_uk },
        { "de-DE", "German",          "Deutsch",          loco_language_id::german },
        { "en-GB", "English (UK)",    "English (UK)",     loco_language_id::english_uk },
        { "en-US", "English (US)",    "English (US)",     loco_language_id::english_us },
        { "es-ES", "Spanish",         u8"Español",        loco_language_id::spanish },
        { "fr-FR", "French",          u8"Français",       loco_language_id::french },
        { "it-IT", "Italian",         "Italiano",         loco_language_id::italian },
        { "nl-NL", "Dutch",           "Nederlands",       loco_language_id::english_uk },
        { "pt-BR", "Portuguese (BR)", u8"Português (BR)", loco_language_id::english_uk },
    };
    // clang-format on

    const language_descriptor& getDescriptorForLanguage(std::string target_locale)
    {
        for (auto& ld : language_descriptors)
        {
            if (ld.locale == target_locale)
                return ld;
        }

        return language_descriptors[languages::undefined];
    }
}
