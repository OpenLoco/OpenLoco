#include "languages.h"
#include "../environment.h"
#include "../platform/platform.h"
#include "../utility/yaml.hpp"
#include "conversion.h"

#ifdef _OPENLOCO_USE_BOOST_FS_
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include <algorithm>
#include <fstream>

namespace openloco::localisation
{
    std::vector<language_descriptor> language_descriptors;

    void enumerateLanguages()
    {
        // (Re-)initialise the languages table.
        language_descriptors.clear();
        language_descriptor undefinedLanguage = { "", "", "", loco_language_id::english_uk };
        language_descriptors.emplace_back(undefinedLanguage);

        // Search the languages dir for YAML language files.
        fs::path languageDir = environment::get_path(environment::path_id::language_files);
        for (auto& entry : fs::directory_iterator(languageDir))
        {
            auto filename = entry.path().string();
            if (filename.substr(filename.size() - 4, 4) != ".yml")
                continue;

            std::fstream stream(filename);
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
            language_descriptor language;
            language.locale = header["locale"].as<std::string>();
            language.english_name = header["english_name"].as<std::string>();
            language.native_name = convertUnicodeToLoco(header["native_name"].as<std::string>());
            language.loco_original_id = (loco_language_id)header["loco_original_id"].as<size_t>();

            // Store it in the languages map.
            language_descriptors.emplace_back(language);
        }

        // Sort by native name.
        std::sort(language_descriptors.begin(), language_descriptors.end(), [](const language_descriptor& a, const language_descriptor& b) -> bool {
            return a.native_name < b.native_name;
        });
    }

    std::vector<language_descriptor>& getLanguageDescriptors()
    {
        return language_descriptors;
    }

    const language_descriptor& getDescriptorForLanguage(std::string target_locale)
    {
        for (auto& ld : language_descriptors)
        {
            if (ld.locale == target_locale)
                return ld;
        }

        return language_descriptors[0];
    }
}
