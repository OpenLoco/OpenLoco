#pragma once

#include "thing.h"
#include "vehicle.h"
#include <cstdio>

namespace openloco::thingmgr
{
    constexpr size_t num_thing_lists = 6;
    constexpr size_t max_things = 20000;

    enum class thing_list
    {
        null,
        vehicle_head,
        misc = 3,
    };

    template<typename T>
    T* get(thing_id_t id);

    template<>
    thing_base* get(thing_id_t id);

    template<typename T>
    T* get(thing_id_t id)
    {
        return static_cast<T*>(get<thing_base>(id));
    }

    thing_id_t first_id(thing_list list);

    template<typename T>
    T* first();

    thing_base* create_thing();
    void freeThing(thing_base* const thing);

    void update_vehicles();
    void update_misc_things();

    uint16_t getListCount(const thing_list list);
    void moveSpriteToList(thing_base* const thing, const thing_list list);
    bool checkNumFreeThings(const size_t numNewThings);

    class VehicleHeadIterator
    {
    private:
        vehicle_head* head = nullptr;
        thing_id_t nextHeadId = thing_id::null;

    public:
        VehicleHeadIterator(const uint16_t _headId)
            : nextHeadId(_headId)
        {
            ++(*this);
        }

        VehicleHeadIterator& operator++()
        {
            head = get<vehicle_head>(nextHeadId);

            if (head)
            {
                nextHeadId = head->next_thing_id;
            }
            return *this;
        }

        VehicleHeadIterator operator++(int)
        {
            VehicleHeadIterator retval = *this;
            ++(*this);
            return retval;
        }
        bool operator==(VehicleHeadIterator other) const
        {
            return head == other.head;
        }
        bool operator!=(VehicleHeadIterator other) const
        {
            return !(*this == other);
        }
        vehicle_head* operator*()
        {
            return head;
        }
        // iterator traits
        using difference_type = std::ptrdiff_t;
        using value_type = vehicle_head;
        using pointer = vehicle_head*;
        using reference = vehicle_head&;
        using iterator_category = std::forward_iterator_tag;
    };

    template<typename T, thing_list list>
    class ThingList
    {
    private:
        uint16_t firstId = thing_id::null;
        using thingListIterator = T;

    public:
        ThingList()
        {
            firstId = first_id(list);
        }

        T begin()
        {
            return T(firstId);
        }
        T end()
        {
            return T(thing_id::null);
        }
    };

    using VehicleList = ThingList<VehicleHeadIterator, thing_list::vehicle_head>;
}
