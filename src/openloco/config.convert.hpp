#pragma once

#include "config.h"

#ifdef _WIN32
// Ignore warnings generated from yaml-cpp
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4251) // 'identifier': 'object_type1' 'identifier1' needs to have dll-interface to be used by clients of 'object_type' 'identfier2'
#pragma warning(disable : 4275) // non dll-interface 'classkey' 'identifier1' used as base for dll-interface 'classkey' 'identifier2'
#pragma warning(disable : 4996) // declaration deprecated
#include <yaml-cpp/yaml.h>
#pragma warning(pop)
#else
#include <yaml-cpp/yaml.h>
#endif

#define enum_def(x, y) \
    {                  \
        x::y, #y       \
    }

namespace YAML
{
    using namespace openloco::config;

    template<typename T>
    using convert_pair_vector = std::vector<std::pair<T, const char*>>;

    template<typename T>
    struct convert_enum_base
    {
        static Node encode(const T& rhs)
        {
            for (const auto& e : convert<T>::get_entries())
            {
                if (rhs == e.first)
                {
                    return Node(e.second);
                }
            }
            return Node();
        }

        static bool decode(const Node& node, T& rhs)
        {
            if (node.IsScalar())
            {
                auto sz = node.Scalar();
                for (const auto& e : convert<T>::get_entries())
                {
                    if (e.second == sz)
                    {
                        rhs = e.first;
                        return true;
                    }
                }
            }
            return false;
        }
    };

    // screen_mode
    const convert_pair_vector<screen_mode> screen_mode_entries = {
        enum_def(screen_mode, window),
        enum_def(screen_mode, fullscreen),
        enum_def(screen_mode, fullscreen_borderless),
    };
    template<>
    struct convert<screen_mode> : convert_enum_base<screen_mode>
    {
        static const convert_pair_vector<screen_mode>& get_entries() { return screen_mode_entries; }
    };
}

#undef enum_def
