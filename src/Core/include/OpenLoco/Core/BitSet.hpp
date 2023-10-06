#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>

// Note this is identical to OpenRCT2's BitSet only with style changes to match codebase

namespace OpenLoco
{
    namespace Detail
    {
        namespace BitSet
        {
            static constexpr size_t kBitsPerByte = std::numeric_limits<std::underlying_type_t<std::byte>>::digits;

            template<size_t TNumBits>
            static constexpr size_t byteAlignBits()
            {
                const auto reminder = TNumBits % kBitsPerByte;
                if constexpr (reminder == 0u)
                {
                    return TNumBits;
                }
                else
                {
                    return TNumBits + (kBitsPerByte - (TNumBits % kBitsPerByte));
                }
            }

            static_assert(byteAlignBits<1>() == 8);
            static_assert(byteAlignBits<4>() == 8);
            static_assert(byteAlignBits<8>() == 8);
            static_assert(byteAlignBits<9>() == 16);
            static_assert(byteAlignBits<9>() == 16);
            static_assert(byteAlignBits<17>() == 24);
            static_assert(byteAlignBits<24>() == 24);
            static_assert(byteAlignBits<31>() == 32);

            // Returns the amount of bytes required for a single block.
            template<size_t TNumBits>
            static constexpr size_t computeBlockSize()
            {
                constexpr size_t numBits = byteAlignBits<TNumBits>();
                if constexpr (numBits >= std::numeric_limits<uintptr_t>::digits)
                {
                    return sizeof(uintptr_t);
                }
                else
                {
                    const auto numBytes = numBits / kBitsPerByte;
                    auto mask = 1u;
                    while (mask < numBytes)
                    {
                        mask <<= 1u;
                    }
                    return mask;
                }
            }

            template<size_t TNumBits, size_t TBlockSizeBytes>
            static constexpr size_t computeBlockCount()
            {
                size_t numBits = TNumBits;
                size_t numBlocks = 0;
                while (numBits > 0)
                {
                    numBlocks++;
                    numBits -= std::min(TBlockSizeBytes * kBitsPerByte, numBits);
                }
                return numBlocks;
            }

            static_assert(computeBlockSize<1>() == sizeof(uint8_t));
            static_assert(computeBlockSize<4>() == sizeof(uint8_t));
            static_assert(computeBlockSize<8>() == sizeof(uint8_t));
            static_assert(computeBlockSize<9>() == sizeof(uint16_t));
            static_assert(computeBlockSize<14>() == sizeof(uint16_t));
            static_assert(computeBlockSize<16>() == sizeof(uint16_t));
            static_assert(computeBlockSize<18>() == sizeof(uint32_t));
            static_assert(computeBlockSize<31>() == sizeof(uint32_t));
            static_assert(computeBlockSize<33>() == sizeof(uintptr_t));

            // TODO: Replace with std::popcount when C++20 is enabled.
            template<typename T>
            static constexpr size_t popcount(const T val)
            {
                size_t res = 0;
                auto x = static_cast<std::make_unsigned_t<T>>(val);
                while (x > 0u)
                {
                    if (x & 1u)
                        res++;
                    x >>= 1u;
                }
                return res;
            }

            template<size_t TByteSize>
            struct StorageBlockType;

            template<>
            struct StorageBlockType<1>
            {
                using value_type = uint8_t;
            };

            template<>
            struct StorageBlockType<2>
            {
                using value_type = uint16_t;
            };

            template<>
            struct StorageBlockType<4>
            {
                using value_type = uint32_t;
            };

            template<>
            struct StorageBlockType<8>
            {
                using value_type = uint64_t;
            };

            template<size_t TBitSize>
            struct storage_block_type_aligned
            {
                using value_type = typename StorageBlockType<computeBlockSize<TBitSize>()>::value_type;
            };
        } // namespace BitSet
    }     // namespace Detail

    template<size_t TBitSize>
    class BitSet
    {
        static constexpr size_t kByteAlignedBitSize = Detail::BitSet::byteAlignBits<TBitSize>();

        using StorageBlockType = typename Detail::BitSet::storage_block_type_aligned<kByteAlignedBitSize>::value_type;

        static constexpr size_t kBlockByteSize = sizeof(StorageBlockType);
        static constexpr size_t kBlockBitSize = kBlockByteSize * Detail::BitSet::kBitsPerByte;
        static constexpr size_t kBlockCount = Detail::BitSet::computeBlockCount<kByteAlignedBitSize, kBlockByteSize>();
        static constexpr size_t kCapacityBits = kBlockCount * kBlockBitSize;

        static constexpr StorageBlockType kBlockValueZero = StorageBlockType{ 0u };
        static constexpr StorageBlockType kBlockValueOne = StorageBlockType{ 1u };
        static constexpr StorageBlockType kBlockValueMask = static_cast<StorageBlockType>(~kBlockValueZero);

        static constexpr bool kRequiresTrim = TBitSize != kCapacityBits;

    public:
        using BlockType = StorageBlockType;
        using Storage = std::array<BlockType, kBlockCount>;

        // Proxy object to access the bits as single value.
        template<typename T>
        class reference_base
        {
            T& _storage;
            const size_t _blockIndex;
            const size_t _blockOffset;

        public:
            constexpr reference_base(T& data, size_t blockIndex, size_t blockOffset) noexcept
                : _storage(data)
                , _blockIndex(blockIndex)
                , _blockOffset(blockOffset)
            {
            }

            constexpr reference_base& operator=(const bool value) noexcept
            {
                if (!value)
                    _storage[_blockIndex] &= ~(kBlockValueOne << _blockOffset);
                else
                    _storage[_blockIndex] |= (kBlockValueOne << _blockOffset);
                return *this;
            }

            constexpr reference_base& operator=(const reference_base& value) noexcept
            {
                return reference_base::operator=(value.value());
            }

            constexpr bool value() const noexcept
            {
                return (_storage[_blockIndex] & (kBlockValueOne << _blockOffset)) != kBlockValueZero;
            }

            constexpr operator bool() const noexcept
            {
                return value();
            }
        };

        using reference = reference_base<Storage>;
        using const_reference = reference_base<const Storage>;

        template<typename T, typename TValue>
        class iterator_base
        {
            T* _bitset{};
            size_t _pos{};

        public:
            constexpr iterator_base() = default;

            constexpr iterator_base(T* bset, size_t pos)
                : _bitset(bset)
                , _pos(pos)
            {
            }

            constexpr auto operator*() const
            {
                const auto blockIndex = computeBlockIndex(_pos);
                const auto blockOffset = computeBlockOffset(_pos);
                return TValue(_bitset->data(), blockIndex, blockOffset);
            }

            constexpr bool operator==(iterator_base other) const
            {
                return _bitset == other._bitset && _pos == other._pos;
            }

            constexpr iterator_base& operator++()
            {
                _pos++;
                return *this;
            }

            constexpr iterator_base operator++(int)
            {
                iterator_base res = *this;
                ++(*this);
                return res;
            }

            constexpr iterator_base& operator--()
            {
                _pos--;
                return *this;
            }

            constexpr iterator_base operator--(int)
            {
                iterator_base res = *this;
                --(*this);
                return res;
            }

        public:
            using difference_type = std::size_t;
            using value_type = TValue;
            using pointer = const TValue*;
            using reference = const TValue&;
            using iterator_category = std::forward_iterator_tag;
        };

        using iterator = iterator_base<BitSet, reference>;
        using const_iterator = iterator_base<const BitSet, const_reference>;

    private:
        Storage _data{};

    public:
        constexpr BitSet() = default;

        constexpr BitSet(const StorageBlockType val)
            : _data{ val }
        {
        }

        constexpr BitSet(const std::initializer_list<size_t>& indices)
        {
            for (auto idx : indices)
            {
                set(idx, true);
            }
        }

        constexpr size_t size() const noexcept
        {
            return TBitSize;
        }

        constexpr size_t count() const noexcept
        {
            size_t numBits = 0;
            for (auto& data : _data)
            {
                numBits += Detail::BitSet::popcount(data);
            }
            return numBits;
        }

        constexpr size_t capacity() const noexcept
        {
            return kCapacityBits;
        }

        constexpr Storage& data() noexcept
        {
            return _data;
        }

        constexpr const Storage& data() const noexcept
        {
            return _data;
        }

        constexpr BitSet& set(size_t index, bool value) noexcept
        {
            const auto blockIndex = computeBlockIndex(index);
            const auto blockOffset = computeBlockOffset(index);
            if (!value)
                _data[blockIndex] &= ~(kBlockValueOne << blockOffset);
            else
                _data[blockIndex] |= (kBlockValueOne << blockOffset);
            return *this;
        }

        constexpr bool get(size_t index) const noexcept
        {
            const auto blockIndex = computeBlockIndex(index);
            const auto blockOffset = computeBlockOffset(index);
            return (_data[blockIndex] & (kBlockValueOne << blockOffset)) != kBlockValueZero;
        }

        constexpr bool operator[](const size_t index) const noexcept
        {
            const auto blockIndex = computeBlockIndex(index);
            const auto blockOffset = computeBlockOffset(index);
            const_reference ref(_data, blockIndex, blockOffset);
            return ref.value();
        }

        constexpr reference operator[](const size_t index) noexcept
        {
            const auto blockIndex = computeBlockIndex(index);
            const auto blockOffset = computeBlockOffset(index);
            return reference(_data, blockIndex, blockOffset);
        }

        constexpr BitSet& flip() noexcept
        {
            for (auto& data : _data)
            {
                data ^= kBlockValueMask;
            }
            if constexpr (kRequiresTrim)
            {
                trim();
            }
            return *this;
        }

        constexpr BitSet& reset() noexcept
        {
            std::fill(_data.begin(), _data.end(), kBlockValueZero);
            if constexpr (kRequiresTrim)
            {
                trim();
            }
            return *this;
        }

        constexpr const_iterator begin() const noexcept
        {
            return const_iterator(this, 0);
        }

        constexpr const_iterator end() const noexcept
        {
            return const_iterator(this, size());
        }

        constexpr iterator begin() noexcept
        {
            return iterator(this, 0);
        }

        constexpr iterator end() noexcept
        {
            return iterator(this, size());
        }

        template<class TChar = char, class TTraits = std::char_traits<TChar>, class TAlloc = std::allocator<TChar>>
        std::basic_string<TChar, TTraits, TAlloc> to_string(TChar zero = TChar{ '0' }, TChar one = TChar{ '1' }) const
        {
            std::basic_string<TChar, TTraits, TAlloc> res;
            res.resize(TBitSize);
            for (size_t i = 0; i < TBitSize; ++i)
            {
                // Printed as little-endian.
                res[TBitSize - i - 1] = get(i) ? one : zero;
            }
            return res;
        }

        constexpr BitSet operator^(const BitSet& other) const noexcept
        {
            BitSet res = *this;
            applyOp<std::bit_xor<BlockType>>(res, other, std::make_index_sequence<kBlockCount>{});
            return res;
        }

        constexpr BitSet& operator^=(const BitSet& other) noexcept
        {
            *this = *this ^ other;
            return *this;
        }

        constexpr BitSet operator|(const BitSet& other) const noexcept
        {
            BitSet res = *this;
            applyOp<std::bit_or<BlockType>>(res, other, std::make_index_sequence<kBlockCount>{});
            return res;
        }

        constexpr BitSet& operator|=(const BitSet& other) noexcept
        {
            *this = *this | other;
            return *this;
        }

        constexpr BitSet operator&(const BitSet& other) const noexcept
        {
            BitSet res = *this;
            applyOp<std::bit_and<BlockType>>(res, other, std::make_index_sequence<kBlockCount>{});
            return res;
        }

        constexpr BitSet& operator&=(const BitSet& other) noexcept
        {
            *this = *this & other;
            return *this;
        }

        constexpr BitSet operator~() const noexcept
        {
            BitSet res = *this;
            for (size_t i = 0; i < _data.size(); i++)
            {
                res._data[i] = ~res._data[i];
            }
            if constexpr (kRequiresTrim)
            {
                res.trim();
            }
            return res;
        }

        constexpr bool operator<(const BitSet& other) const noexcept
        {
            return std::lexicographical_compare(
                _data.begin(), _data.end(), other._data.begin(), other._data.end(), std::less<StorageBlockType>{});
        }

        constexpr bool operator<=(const BitSet& other) const noexcept
        {
            return std::lexicographical_compare(
                _data.begin(), _data.end(), other._data.begin(), other._data.end(), std::less_equal<StorageBlockType>{});
        }

        constexpr bool operator>(const BitSet& other) const noexcept
        {
            return std::lexicographical_compare(
                _data.begin(), _data.end(), other._data.begin(), other._data.end(), std::greater<StorageBlockType>{});
        }

        constexpr bool operator>=(const BitSet& other) const noexcept
        {
            return std::lexicographical_compare(
                _data.begin(), _data.end(), other._data.begin(), other._data.end(), std::greater_equal<StorageBlockType>{});
        }

    private:
        template<typename TOperator, size_t... TIndex>
        void applyOp(BitSet& dst, const BitSet& src, std::index_sequence<TIndex...>) const
        {
            TOperator op{};
            ((dst._data[TIndex] = op(dst._data[TIndex], src._data[TIndex])), ...);
            if constexpr (kRequiresTrim)
            {
                dst.trim();
            }
        }

        static constexpr size_t computeBlockIndex(size_t idx) noexcept
        {
            if constexpr (kBlockCount == 1)
            {
                return 0;
            }
            else
            {
                return idx / kBlockBitSize;
            }
        }

        static constexpr size_t computeBlockOffset(size_t idx) noexcept
        {
            if constexpr (kBlockCount == 1)
            {
                return idx;
            }
            else
            {
                return idx % kBlockBitSize;
            }
        }

        // Some operations require to trim of the excess.
        constexpr void trim() noexcept
        {
            const auto byteIdx = TBitSize / kBlockBitSize;
            const auto bitIdx = TBitSize % kBlockBitSize;
            if constexpr (bitIdx == 0)
                return;

            auto trimMask = kBlockValueMask;
            trimMask <<= (kBlockBitSize - bitIdx);
            trimMask >>= (kBlockBitSize - bitIdx);

            _data[byteIdx] &= trimMask;
        }
    };

} // namespace OpenLoco
