#include "Languages.h"
#include "Conversion.h"
#include "Environment.h"
#include <OpenLoco/Core/FileSystem.hpp>
#include <OpenLoco/Platform/Platform.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>
#include <ranges>

namespace OpenLoco::Localisation
{
    static std::vector<LanguageDescriptor> _languageDescriptors;

    void enumerateLanguages()
    {
        // (Re-)initialise the languages table.
        _languageDescriptors.clear();
        LanguageDescriptor undefinedLanguage = { "", "", "", LocoLanguageId::english_uk };
        _languageDescriptors.emplace_back(undefinedLanguage);

        // Search the languages dir for YAML language files.
        fs::path languageDir = Environment::getPath(Environment::PathId::languageFiles);
        for (auto& entry : fs::directory_iterator(languageDir))
        {
            const auto filePath = entry.path();

            const auto fileExt = filePath.extension();
            if (fileExt != ".yml")
            {
                continue;
            }

            std::ifstream stream(filePath);
            if (!stream.is_open())
            {
                continue;
            }

            // Read only the header of the file to speed up the indexing process.
            std::string headerYaml;
            for (std::string line; line != "strings:" && !stream.eof(); std::getline(stream, line))
            {
                headerYaml += line + '\n';
            }

            YAML::Node node = YAML::Load(headerYaml);
            if (!node.IsMap())
            {
                continue;
            }

            YAML::Node header = node["header"];

            // Create a language descriptor for this language file.
            LanguageDescriptor language;
            language.locale = header["locale"].as<std::string>();
            language.englishName = header["english_name"].as<std::string>();
            language.nativeName = convertUnicodeToLoco(header["native_name"].as<std::string>());
            language.locoOriginalId = (LocoLanguageId)header["loco_original_id"].as<size_t>();

            // Store it in the languages map.
            _languageDescriptors.emplace_back(language);
        }

        // Sort by native name.
        std::ranges::sort(_languageDescriptors, {}, &LanguageDescriptor::nativeName);
    }

    std::span<const LanguageDescriptor> getLanguageDescriptors()
    {
        return _languageDescriptors;
    }

    const LanguageDescriptor& getDescriptorForLanguage(std::string_view target_locale)
    {
        const auto it = std::ranges::find(_languageDescriptors, target_locale, &LanguageDescriptor::locale);
        if (it != _languageDescriptors.end())
        {
            return *it;
        }

        return _languageDescriptors[0];
    }
}
