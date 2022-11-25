#include "Languages.h"
#include "Conversion.h"
#include "Core/FileSystem.hpp"
#include "Environment.h"
#include "Platform/Platform.h"
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <fstream>

namespace OpenLoco::Localisation
{
    std::vector<LanguageDescriptor> languageDescriptors;

    void enumerateLanguages()
    {
        // (Re-)initialise the languages table.
        languageDescriptors.clear();
        LanguageDescriptor undefinedLanguage = { "", "", "", LocoLanguageId::english_uk };
        languageDescriptors.emplace_back(undefinedLanguage);

        // Search the languages dir for YAML language files.
        fs::path languageDir = Environment::getPath(Environment::PathId::languageFiles);
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
            language.englishName = header["english_name"].as<std::string>();
            language.nativeName = convertUnicodeToLoco(header["native_name"].as<std::string>());
            language.locoOriginalId = (LocoLanguageId)header["loco_original_id"].as<size_t>();

            // Store it in the languages map.
            languageDescriptors.emplace_back(language);
        }

        // Sort by native name.
        std::sort(languageDescriptors.begin(), languageDescriptors.end(), [](const LanguageDescriptor& a, const LanguageDescriptor& b) -> bool {
            return a.nativeName < b.nativeName;
        });
    }

    std::vector<LanguageDescriptor>& getLanguageDescriptors()
    {
        return languageDescriptors;
    }

    const LanguageDescriptor& getDescriptorForLanguage(std::string target_locale)
    {
        for (auto& ld : languageDescriptors)
        {
            if (ld.locale == target_locale)
                return ld;
        }

        return languageDescriptors[0];
    }
}
