#include "Languages.h"
#include "../Core/FileSystem.hpp"
#include "../Environment.h"
#include "../Platform/Platform.h"
#include "../Utility/Yaml.hpp"
#include "Conversion.h"

#include <algorithm>
#include <fstream>

namespace OpenLoco::Localisation
{
    std::vector<LanguageDescriptor> language_descriptors;

    void enumerateLanguages()
    {
        // (Re-)initialise the languages table.
        language_descriptors.clear();
        LanguageDescriptor undefinedLanguage = { "", "", "", LocoLanguageId::english_uk };
        language_descriptors.emplace_back(undefinedLanguage);

        // Search the languages dir for YAML language files.
        fs::path languageDir = Environment::getPath(Environment::path_id::language_files);
        for (auto& entry : fs::directory_iterator(languageDir))
        {
            auto filename = entry.path().string();
            if (filename.substr(filename.size() - 4, 4) != ".yml")
                continue;

            std::fstream stream(entry.path());
            if (!stream.is_open())
                continue;

            // Read only the header of the file to speed up the indexing process.
            std::string headerYaml;
            for (std::string line; line != "strings:" && !stream.eof(); std::getline(stream, line))
                headerYaml += line + '\n';

            YAML::Node node = YAML::Load(headerYaml);
            if (!node.IsMap())
                continue;

            YAML::Node header = node["header"];

            // Create a language descriptor for this language file.
            LanguageDescriptor language;
            language.locale = header["locale"].as<std::string>();
            language.english_name = header["english_name"].as<std::string>();
            language.native_name = convertUnicodeToLoco(header["native_name"].as<std::string>());
            language.loco_original_id = (LocoLanguageId)header["loco_original_id"].as<size_t>();

            // Store it in the languages map.
            language_descriptors.emplace_back(language);
        }

        // Sort by native name.
        std::sort(language_descriptors.begin(), language_descriptors.end(), [](const LanguageDescriptor& a, const LanguageDescriptor& b) -> bool {
            return a.native_name < b.native_name;
        });
    }

    std::vector<LanguageDescriptor>& getLanguageDescriptors()
    {
        return language_descriptors;
    }

    const LanguageDescriptor& getDescriptorForLanguage(std::string target_locale)
    {
        for (auto& ld : language_descriptors)
        {
            if (ld.locale == target_locale)
                return ld;
        }

        return language_descriptors[0];
    }
}
