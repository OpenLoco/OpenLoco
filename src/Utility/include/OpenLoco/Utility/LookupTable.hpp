#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace OpenLoco::Utility
{
    // Simplified version of sfl::static_flat_map, it has constexpr issues.
    // Switch back to sfl::static_flat_map for when that is resolved.
    template<typename K, typename V, std::size_t N>
    class LookupTable
    {
        std::array<std::pair<K, V>, N> _data{};
        std::size_t _size{};

    public:
        using value_type = std::pair<K, V>;
        using const_iterator = typename std::array<value_type, N>::const_iterator;

        template<typename... Args>
        constexpr LookupTable(Args&&... args)
            : _data{ std::forward<Args>(args)... }
            , _size(sizeof...(Args))
        {
            std::sort(_data.begin(), _data.end(), [](const value_type& a, const value_type& b) {
                return a.first < b.first;
            });
        }

        constexpr const_iterator begin() const
        {
            return _data.begin();
        }

        constexpr const_iterator end() const
        {
            return _data.begin() + _size;
        }

        constexpr std::size_t size() const
        {
            return _size;
        }

        constexpr const_iterator find(const K& key) const
        {
            auto it = std::lower_bound(begin(), end(), key, [](const value_type& entry, const K& k) {
                return entry.first < k;
            });
            if (it != end() && !(key < it->first))
            {
                return it;
            }
            return end();
        }

        constexpr bool contains(const K& key) const
        {
            return find(key) != end();
        }

        constexpr const V& at(const K& key) const
        {
            auto it = find(key);
            if (it == end())
            {
                throw std::out_of_range("LookupTable::at");
            }
            return it->second;
        }
    };

    namespace Detail
    {
        template<typename K, typename V, std::size_t N, std::size_t... I>
        constexpr auto buildLookupTableImpl(std::pair<K, V> const (&arr)[N], std::index_sequence<I...>)
        {
            return LookupTable<K, V, N>{ arr[I]... };
        }
    }

    template<typename K, typename V, std::size_t N>
    constexpr auto buildLookupTable(std::pair<K, V> const (&arr)[N])
    {
        return Detail::buildLookupTableImpl(arr, std::make_index_sequence<N>{});
    }

}
