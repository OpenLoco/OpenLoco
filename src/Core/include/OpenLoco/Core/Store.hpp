#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <limits>
#include <sfl/segmented_vector.hpp>
#include <type_traits>
#include <vector>

namespace OpenLoco
{
    template<typename T>
    class Store
    {
    public:
        using Index = uint32_t;
        static constexpr Index kInvalidIndex = std::numeric_limits<Index>::max();

    private:
        static constexpr size_t kBitsPerWord = std::numeric_limits<size_t>::digits;

        static constexpr std::size_t kSegmentSize = 1024;
        sfl::segmented_vector<T, kSegmentSize> _slots;
        std::vector<size_t> _live;
        Index _firstFreeHint = 0;
        size_t _liveCount = 0;

        static constexpr size_t wordOf(Index i) noexcept
        {
            return i / kBitsPerWord;
        }

        static constexpr size_t maskOf(Index i) noexcept
        {
            return size_t{ 1 } << (i % kBitsPerWord);
        }

        void setLiveBit(Index i) noexcept
        {
            _live[wordOf(i)] |= maskOf(i);
        }

        void clearLiveBit(Index i) noexcept
        {
            _live[wordOf(i)] &= ~maskOf(i);
        }

        bool testLiveBit(Index i) const noexcept
        {
            return (_live[wordOf(i)] & maskOf(i)) != 0;
        }

        void ensureLiveBitsFor(size_t slotCount)
        {
            const size_t words = (slotCount + kBitsPerWord - 1) / kBitsPerWord;
            if (_live.size() < words)
            {
                _live.resize(words, 0);
            }
        }

        template<bool IsConst>
        class IteratorImpl
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = std::conditional_t<IsConst, const T*, T*>;
            using reference = std::conditional_t<IsConst, const T&, T&>;

        private:
            using StorePtr = std::conditional_t<IsConst, const Store*, Store*>;

            StorePtr _store{};
            Index _index{};

            void advanceToLive() noexcept
            {
                const auto cap = static_cast<Index>(_store->_slots.size());
                while (_index < cap && !_store->testLiveBit(_index))
                {
                    ++_index;
                }
            }

        public:
            IteratorImpl() = default;
            IteratorImpl(StorePtr store, Index index) noexcept
                : _store(store)
                , _index(index)
            {
                advanceToLive();
            }

            reference operator*() const noexcept
            {
                return _store->_slots[_index];
            }

            pointer operator->() const noexcept
            {
                return &_store->_slots[_index];
            }

            Index index() const noexcept
            {
                return _index;
            }

            IteratorImpl& operator++() noexcept
            {
                ++_index;
                advanceToLive();
                return *this;
            }

            IteratorImpl operator++(int) noexcept
            {
                IteratorImpl tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const IteratorImpl& other) const noexcept
            {
                return _index == other._index;
            }

            bool operator!=(const IteratorImpl& other) const noexcept
            {
                return !(*this == other);
            }
        };

    public:
        using iterator = IteratorImpl<false>;
        using const_iterator = IteratorImpl<true>;

        Store() = default;

        Index allocate()
        {
            const auto cap = static_cast<Index>(_slots.size());
            Index scan = _firstFreeHint;

            while (scan < cap)
            {
                const size_t word = wordOf(scan);
                const size_t bitInWord = scan % kBitsPerWord;
                size_t freeBits = ~_live[word];
                freeBits &= ~((size_t{ 1 } << bitInWord) - 1);

                if (freeBits != 0)
                {
                    const auto found = static_cast<Index>(word * kBitsPerWord + std::countr_zero(freeBits));
                    if (found < cap)
                    {
                        setLiveBit(found);
                        _slots[found] = T{};
                        _firstFreeHint = found + 1;
                        ++_liveCount;
                        return found;
                    }
                    break;
                }
                scan = static_cast<Index>((word + 1) * kBitsPerWord);
            }

            const auto i = cap;
            _slots.emplace_back();
            ensureLiveBitsFor(_slots.size());
            setLiveBit(i);
            _firstFreeHint = i + 1;
            ++_liveCount;
            return i;
        }

        void release(Index i)
        {
            assert(contains(i));
            clearLiveBit(i);
            if (i < _firstFreeHint)
            {
                _firstFreeHint = i;
            }
            --_liveCount;
        }

        T& operator[](Index i) noexcept
        {
            assert(contains(i));
            return _slots[i];
        }

        const T& operator[](Index i) const noexcept
        {
            assert(contains(i));
            return _slots[i];
        }

        bool contains(Index i) const noexcept
        {
            return i < _slots.size() && testLiveBit(i);
        }

        size_t size() const noexcept
        {
            return _liveCount;
        }

        size_t capacity() const noexcept
        {
            return _slots.size();
        }

        bool empty() const noexcept
        {
            return _liveCount == 0;
        }

        void clear() noexcept
        {
            _slots.clear();
            _live.clear();
            _firstFreeHint = 0;
            _liveCount = 0;
        }

        void reserve(size_t n)
        {
            _slots.reserve(n);
            ensureLiveBitsFor(n);
        }

        iterator begin() noexcept
        {
            return iterator(this, 0);
        }

        iterator end() noexcept
        {
            return iterator(this, static_cast<Index>(_slots.size()));
        }

        const_iterator begin() const noexcept
        {
            return const_iterator(this, 0);
        }

        const_iterator end() const noexcept
        {
            return const_iterator(this, static_cast<Index>(_slots.size()));
        }

        const_iterator cbegin() const noexcept
        {
            return begin();
        }

        const_iterator cend() const noexcept
        {
            return end();
        }
    };
}
