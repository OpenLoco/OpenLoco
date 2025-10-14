#pragma once

#include <OpenLoco/Core/Exception.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

#if defined(__i386__) || defined(_M_IX86)
#define assert_struct_size(x, y) static_assert(sizeof(x) == (y), "Improper struct size")
#else
#define assert_struct_size(x, y)
#endif

constexpr int32_t kDefaultRegValue = 0xCCCCCCCC;

namespace OpenLoco::Interop
{
    template<typename T = void>
    class X86Pointer
    {
    private:
        uintptr_t _ptr;

    public:
        X86Pointer(const T* x)
        {
            _ptr = reinterpret_cast<uintptr_t>(x);
        }

        X86Pointer(const uint32_t ptr)
        {
            _ptr = ptr;
        }

        operator uint32_t() const
        {
            return (uint32_t)_ptr;
        }

        operator T*() const
        {
            return reinterpret_cast<T*>(_ptr);
        };
    };

#pragma pack(push, 1)
    /**
     * x86 register structure, only used for easy interop to Locomotion code.
     */
    struct registers
    {
        union
        {
            int32_t eax{ kDefaultRegValue };
            int16_t ax;
            struct
            {
                int8_t al;
                int8_t ah;
            };
        };
        union
        {
            int32_t ebx{ kDefaultRegValue };
            int16_t bx;
            struct
            {
                int8_t bl;
                int8_t bh;
            };
        };
        union
        {
            int32_t ecx{ kDefaultRegValue };
            int16_t cx;
            struct
            {
                int8_t cl;
                int8_t ch;
            };
        };
        union
        {
            int32_t edx{ kDefaultRegValue };
            int16_t dx;
            struct
            {
                int8_t dl;
                int8_t dh;
            };
        };
        union
        {
            int32_t esi{ kDefaultRegValue };
            int16_t si;
        };
        union
        {
            int32_t edi{ kDefaultRegValue };
            int16_t di;
        };
        union
        {
            int32_t ebp{ kDefaultRegValue };
            int16_t bp;
        };
    };
    assert_struct_size(registers, 7 * 4);
#pragma pack(pop)

#ifndef USE_MMAP
    constexpr uintptr_t kGoodPlaceForDataSegment = 0x008A4000;
#else
#if defined(PLATFORM_32BIT)
    constexpr uintptr_t kGoodPlaceForDataSegment = 0x09000000;
#elif defined(PLATFORM_64BIT)
    constexpr uintptr_t kGoodPlaceForDataSegment = 0x200000000;
#else
#error "Unknown platform"
#endif
#endif

    template<uint32_t TAddress, typename T>
    constexpr T& addr()
    {
        constexpr auto ptrAddr = kGoodPlaceForDataSegment - 0x008A4000 + TAddress;
        // We use std::launder to prevent the compiler from doing optimizations that lead to undefined behavior.
        return *std::launder(reinterpret_cast<T*>(ptrAddr));
    }

    struct GlobalStore
    {
    public:
        static GlobalStore& getInstance();

        static void addAddressRange(uint32_t begin, uint32_t size);

    private:
        GlobalStore() = default;

        bool isAddressInRange(uint32_t address, uint32_t size) const;

        std::vector<std::pair<uint32_t, uint32_t>> addressRanges; // Pairs of (begin, size)

        static GlobalStore gStoreInstance;
    };

    template<typename T, uintptr_t TAddress>
    struct loco_global
    {
    public:
        typedef T type;
        typedef type* pointer;
        typedef type& reference;
        typedef const type& const_reference;

    private:
        pointer _Myptr;

    public:
        loco_global()
        {
            _Myptr = &(addr<TAddress, T>());
            GlobalStore::addAddressRange(TAddress, sizeof(T));
        }

        loco_global(const loco_global<T, TAddress>&) = delete; // Do not copy construct a loco global

        operator reference()
        {
            return addr<TAddress, T>();
        }

        loco_global& operator=(const_reference v)
        {
            addr<TAddress, T>() = v;
            return *this;
        }

        loco_global& operator+=(const_reference v)
        {
            addr<TAddress, T>() += v;
            return *this;
        }

        loco_global& operator|=(const_reference v)
        {
            addr<TAddress, T>() |= v;
            return *this;
        }

        loco_global& operator&=(const_reference v)
        {
            addr<TAddress, T>() &= v;
            return *this;
        }

        loco_global& operator^=(const_reference v)
        {
            addr<TAddress, T>() ^= v;
            return *this;
        }

        loco_global& operator-=(const_reference v)
        {
            addr<TAddress, T>() -= v;
            return *this;
        }

        loco_global& operator++()
        {
            addr<TAddress, T>()++;
            return *this;
        }

        T operator++(int)
        {
            reference ref = addr<TAddress, T>();
            T temp = ref;
            ref++;
            return temp;
        }

        loco_global& operator--()
        {
            addr<TAddress, T>()--;
            return *this;
        }

        T operator--(int)
        {
            reference ref = addr<TAddress, T>();
            T temp = ref;
            ref--;
            return temp;
        }

        reference operator*()
        {
            return addr<TAddress, T>();
        }

        pointer operator->()
        {
            return &(addr<TAddress, T>());
        }

        constexpr size_t size() const
        {
            return sizeof(T);
        }
    };

    template<typename T>
    struct loco_global_iterator
    {
    private:
        T* _ptr;

    public:
        loco_global_iterator(T* p)
            : _ptr(p)
        {
        }
        loco_global_iterator& operator++()
        {
            ++_ptr;
            return *this;
        }
        loco_global_iterator operator++(int)
        {
            auto temp = *this;
            ++_ptr;
            return temp;
        }
        loco_global_iterator& operator--()
        {
            --_ptr;
            return *this;
        }
        loco_global_iterator operator--(int)
        {
            auto temp = *this;
            --_ptr;
            return temp;
        }
        bool operator==(const loco_global_iterator& rhs) const
        {
            return _ptr == rhs._ptr;
        }
        T& operator*()
        {
            return *_ptr;
        }

        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = const T*;
        using reference = const T&;
        using iterator_category = std::forward_iterator_tag;
    };

    template<typename T, size_t TCount, uintptr_t TAddress>
    struct loco_global<T[TCount], TAddress>
    {
    public:
        typedef T type;
        typedef type* pointer;
        typedef type& reference;
        typedef const type& const_reference;
        typedef loco_global_iterator<T> iterator;
        static constexpr auto address = TAddress;
        static constexpr auto endAddress = TAddress + TCount * sizeof(type);

    private:
        pointer _Myfirst;
        pointer _Mylast;

    public:
        loco_global()
        {
            _Myfirst = get();
            _Mylast = _Myfirst + TCount;
            GlobalStore::addAddressRange(TAddress, TCount * sizeof(T));
        }

        operator pointer()
        {
            return get();
        }

        pointer get() const
        {
            return reinterpret_cast<pointer>(&addr<TAddress, type>());
        }

        reference operator[](int idx)
        {
            assert(idx >= 0 && static_cast<size_t>(idx) < size());
#ifndef NDEBUG
            if (idx < 0 || static_cast<size_t>(idx) >= size())
            {
                throw Exception::OutOfRange("loco_global: bounds check violation!");
            }
#endif
            return get()[idx];
        }

        constexpr size_t size() const
        {
            return TCount;
        }

        iterator begin() const
        {
            return iterator(&addr<TAddress, T>());
        }

        iterator end() const
        {
            const pointer ptrEnd = (&addr<TAddress, T>()) + TCount;
            return iterator(ptrEnd);
        }
    };
}
