#pragma once

#include "config.h"
#include "utility/yaml.hpp"

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

    // resolution_t
    template<>
    struct convert<resolution_t>
    {
        static Node encode(const resolution_t& rhs)
        {
            Node node;
            node["width"] = rhs.width;
            node["height"] = rhs.height;
            return node;
        }

        static bool decode(const Node& node, resolution_t& rhs)
        {
            if (node.IsMap())
            {
                rhs.width = node["width"].as<int32_t>();
                rhs.height = node["height"].as<int32_t>();
                return true;
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
