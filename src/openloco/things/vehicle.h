#pragma once

#include "../Audio/Audio.h"
#include "../Company.h"
#include "../Objects/vehicle_object.h"
#include "../Window.h"
#include "../ui/WindowType.h"
#include "thing.h"

namespace openloco
{
    namespace things::vehicle
    {
        constexpr auto max_vehicle_length = 176; // TODO: Units?

        uint32_t create(const uint8_t flags, const uint16_t vehicleTypeId, const uint16_t vehicleThingId);

        namespace flags_38
        {
            constexpr uint8_t unk_0 = 1 << 0;
            constexpr uint8_t unk_1 = 1 << 1;
            constexpr uint8_t unk_3 = 1 << 3;
            constexpr uint8_t unk_4 = 1 << 4;
        }
    }

    struct vehicle_head;
    struct vehicle_1;
    struct vehicle_2;
    struct vehicle_bogie;
    struct vehicle_body;
    struct vehicle_tail;

    struct vehicle_26;

    namespace flags_5f
    {
        constexpr uint8_t breakdown_pending = 1 << 1;
        constexpr uint8_t broken_down = 1 << 2;
    }

    enum class VehicleThingType : uint8_t
    {
        head = 0,
        vehicle_1,
        vehicle_2,
        bogie,
        body_start,
        body_continued,
        tail,
    };

#pragma pack(push, 1)
    struct vehicle_base : thing_base
    {
        VehicleThingType type;
        uint8_t pad_02;
        uint8_t pad_03;
        thing_id_t next_thing_id; // 0x04
        uint8_t pad_06[0x09 - 0x06];
        uint8_t var_09;
        thing_id_t id; // 0x0A
        uint16_t var_0C;
        int16_t x; // 0x0E
        int16_t y; // 0x10
        int16_t z; // 0x12
        uint8_t var_14;
        uint8_t var_15;
        int16_t sprite_left;   // 0x16
        int16_t sprite_top;    // 0x18
        int16_t sprite_right;  // 0x1A
        int16_t sprite_bottom; // 0x1C
        uint8_t sprite_yaw;    // 0x1E
        uint8_t sprite_pitch;  // 0x1F

    private:
        template<typename TType, VehicleThingType TClass>
        TType* as() const
        {
            // This can not use reinterpret_cast due to being a const member without considerable more code
            return type == TClass ? (TType*)this : nullptr;
        }

        template<typename TType>
        TType* as() const
        {
            return as<TType, TType::vehicleThingType>();
        }

    public:
        vehicle_head* asVehicleHead() const { return as<vehicle_head>(); }
        vehicle_1* asVehicle1() const { return as<vehicle_1>(); }
        vehicle_2* asVehicle2() const { return as<vehicle_2>(); }
        vehicle_bogie* asVehicleBogie() const { return as<vehicle_bogie>(); }
        vehicle_body* asVehicleBody() const
        {
            auto vehicle = as<vehicle_body, VehicleThingType::body_start>();
            if (vehicle != nullptr)
                return vehicle;
            return as<vehicle_body, VehicleThingType::body_continued>();
        }
        vehicle_26* asVehicle2Or6() const
        {
            auto vehicle = as<vehicle_26, VehicleThingType::vehicle_2>();
            if (vehicle != nullptr)
                return vehicle;
            return as<vehicle_26, VehicleThingType::tail>();
        }
        vehicle_tail* asVehicleTail() const { return as<vehicle_tail>(); }
    };

    struct vehicle : vehicle_base
    {
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint16_t var_22;    // used for name on vehicle_0 will be type as string_id
        uint8_t pad_24[0x26 - 0x24];
        thing_id_t head; // 0x26
        uint16_t var_28;
        uint8_t pad_2A[0x2C - 0x2A];
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;         // 0x39
        thing_id_t next_car_id; // 0x3A
        uint32_t var_3C;        // veh1 speed?
        uint16_t object_id;     // 0x40 not used in all vehicles **be careful**
        TransportMode mode;     // 0x42 field same in all vehicles
        uint8_t pad_43;
        int16_t var_44; // used for name on vehicle_0 will be unique (for type) number
        uint8_t pad_46;
        uint8_t pad_47[0x4A - 0x47];
        uint16_t var_4A;
        uint8_t cargo_type; // 0x4C
        uint8_t pad_4D;
        uint16_t cargo_origin; // 0x4E
        uint8_t pad_50;
        uint8_t cargo_quantity; // 0x51
        uint8_t pad_52[0x54 - 0x52];
        uint8_t pad_54;
        uint8_t pad_55;
        uint32_t var_56;
        uint8_t var_5A;
        uint8_t pad_5B[0x5D - 0x5B];
        uint8_t var_5D;
        VehicleType vehicleType; // 0x5E
        uint8_t var_5F;          // 0x5F (bit 1 = can break down)
        uint8_t pad_60[0x6A - 0x60];
        uint8_t var_6A;
        uint8_t pad_6B[0x73 - 0x6B];
        uint8_t var_73; // 0x73 (bit 0 = broken down)

        vehicle* nextVehicle();
        vehicle* nextVehicleComponent();
        vehicle_object* object() const;

        bool updateComponent();
    };
    static_assert(sizeof(vehicle) == 0x74); // Can't use offset_of change this to last field if more found

    struct vehicle_26 : vehicle_base
    {
        uint8_t pad_20[0x44 - 0x20];
        sound_object_id_t sound_id; // 0x44
        uint8_t pad_45[0x4A - 0x45];
        uint16_t var_4A;                       // sound-related flag(s)
        ui::window_number sound_window_number; // 0x4C
        ui::WindowType sound_window_type;      // 0x4E
        uint8_t pad_4F[0x56 - 0x4F];
        uint32_t var_56;
        uint8_t pad_5A[0x73 - 0x53];
        uint8_t var_73;
    };

    struct vehicle_head : vehicle_base
    {
        static constexpr auto vehicleThingType = VehicleThingType::head;
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint16_t var_22;
        uint8_t pad_24[0x26 - 0x24];
        thing_id_t head; // 0x26
        uint32_t var_28;
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles orderId * max_num_routing_steps
        uint8_t var_38;
        uint8_t pad_39;         // 0x39
        thing_id_t next_car_id; // 0x3A
        uint32_t var_3C;        // 0x3C
        uint8_t pad_40[0x2];    // 0x40
        TransportMode mode;     // 0x42 field same in all vehicles
        uint8_t pad_43;
        int16_t var_44;
        uint32_t length_of_var_4C; // 0x46
        uint16_t var_4A;
        uint16_t var_4C;     // 0x4C index into ?order? array
        uint8_t pad_4E[0x2]; // 0x4E
        uint8_t pad_50;
        uint8_t pad_51; // 0x51
        uint8_t var_52;
        uint8_t pad_53;
        int16_t var_54;
        uint8_t pad_56[0x4];
        uint8_t pad_5A;
        uint8_t pad_5B[0x5D - 0x5B];
        uint8_t var_5D;
        VehicleType vehicleType; // 0x5E
        uint8_t var_5F;          // 0x5F
        uint8_t var_60;
        uint16_t var_61;
        uint8_t pad_63[0x69 - 0x63];
        uint32_t var_69;
        uint8_t pad_6D[0x77 - 0x6D];
        uint16_t var_77; //
        uint8_t var_79;

    public:
        bool isVehicleTypeCompatible(const uint16_t vehicleTypeId);
        void sub_4BA8D4();
        void updateVehicle();
        uint16_t update();

    private:
        void sub_4BAA76();
        uint32_t getVehicleTotalLength();
    };
    static_assert(sizeof(vehicle_head) == 0x7A); // Can't use offset_of change this to last field if more found

    struct vehicle_1 : vehicle_base
    {
        static constexpr auto vehicleThingType = VehicleThingType::vehicle_1;
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint8_t pad_22[0x26 - 0x22];
        thing_id_t head; // 0x26
        uint32_t var_28;
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;         // 0x39
        thing_id_t next_car_id; // 0x3A
        uint32_t var_3C;        // 0x3C
        uint8_t pad_40[0x2];    // 0x40
        TransportMode mode;     // 0x42 field same in all vehicles
        uint8_t pad_43;
        uint16_t var_44;
        uint16_t var_46;
        uint8_t var_48;
        uint8_t pad_49[0x4E - 0x49];
        uint16_t var_4E;
        uint16_t var_50;
        uint8_t var_52;
        int32_t var_53;
    };
    static_assert(sizeof(vehicle_1) == 0x57); // Can't use offset_of change this to last field if more found

    struct vehicle_2 : vehicle_base
    {
        static constexpr auto vehicleThingType = VehicleThingType::vehicle_2;
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint8_t pad_22[0x26 - 0x22];
        thing_id_t head; // 0x26
        uint32_t var_28;
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;              // 0x39
        thing_id_t next_car_id;      // 0x3A
        uint8_t pad_3C[0x42 - 0x3C]; // 0x3C
        TransportMode mode;          // 0x42 field same in all vehicles
        uint8_t pad_43;
        sound_object_id_t soundId; // 0x44 common with tail
        uint8_t pad_45[0x48 - 0x45];
        int16_t var_48;
        uint16_t var_4A;                       // sound-related flag(s) common with tail
        ui::window_number sound_window_number; // 0x4C common with tail
        ui::WindowType sound_window_type;      // 0x4E common with tail
        uint8_t pad_4F[0x56 - 0x4F];
        uint32_t var_56;
        uint8_t var_5A;
        uint8_t var_5B;
        uint8_t pad_5C[0x5E - 0x5C];
        uint32_t var_5E;
        uint32_t refund_cost;
        uint32_t var_66;
        uint32_t var_6A;
        uint32_t var_6E;
        uint8_t var_72;
        uint8_t var_73; // 0x73 (bit 0 = broken down)
    };
    static_assert(sizeof(vehicle_2) == 0x74); // Can't use offset_of change this to last field if more found

    struct vehicle_body : vehicle_base
    {
        static constexpr auto vehicleThingType = VehicleThingType::body_continued;
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint8_t pad_22[0x24 - 0x22];
        ColourScheme colour_scheme; // 0x24
        thing_id_t head;            // 0x26
        uint8_t pad_28[0x2C - 0x28];
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t object_sprite_type; // 0x39
        thing_id_t next_car_id;     // 0x3A
        uint8_t pad_3C[0x40 - 0x3C];
        uint16_t object_id; // 0x40
        TransportMode mode; // 0x42
        uint8_t pad_43;
        int16_t var_44;
        uint8_t var_46;
        uint8_t var_47;
        uint32_t accepted_cargo_types; // 0x48
        uint8_t cargo_type;            // 0x4C
        uint8_t max_cargo;             // 0x4D
        uint8_t pad_4E[0x51 - 0x4E];
        uint8_t var_51;
        uint8_t pad_52[0x54 - 0x52];
        uint8_t body_index; // 0x54
        int8_t var_55;
        uint32_t creation_day; // 0x56
        uint8_t pad_5A[0x5E - 0x5A];
        uint8_t var_5E;
        uint8_t var_5F;

        vehicle_object* object() const;
        int32_t update();
        void secondaryAnimationUpdate();

    private:
        void sub_4AAB0B();
        void animationUpdate();
        void sub_4AC255(vehicle_bogie* back_bogie, vehicle_bogie* front_bogie);
        void steamPuffsAnimationUpdate(uint8_t num, int32_t var_05);
        void dieselExhaust1AnimationUpdate(uint8_t num, int32_t var_05);
        void dieselExhaust2AnimationUpdate(uint8_t num, int32_t var_05);
        void electricSpark1AnimationUpdate(uint8_t num, int32_t var_05);
        void electricSpark2AnimationUpdate(uint8_t num, int32_t var_05);
        void shipWakeAnimationUpdate(uint8_t num, int32_t var_05);
        uint8_t updateSpritePitchSteepSlopes(uint16_t xy_offset, int16_t z_offset);
        uint8_t updateSpritePitch(uint16_t xy_offset, int16_t z_offset);
        uint8_t updateSpriteYaw0(int16_t x_offset, int16_t y_offset);
        uint8_t updateSpriteYaw1(int16_t x_offset, int16_t y_offset);
        uint8_t updateSpriteYaw2(int16_t x_offset, int16_t y_offset);
        uint8_t updateSpriteYaw3(int16_t x_offset, int16_t y_offset);
        uint8_t updateSpriteYaw4(int16_t x_offset, int16_t y_offset);
    };
    static_assert(sizeof(vehicle_body) == 0x60); // Can't use offset_of change this to last field if more found

    struct vehicle_bogie : vehicle_base
    {
        static constexpr auto vehicleThingType = VehicleThingType::bogie;
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint8_t pad_22[0x24 - 0x22];
        ColourScheme colour_scheme; // 0x24
        thing_id_t head;            // 0x26
        uint8_t pad_28[0x2C - 0x28];
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t object_sprite_type; // 0x39
        thing_id_t next_car_id;     // 0x3A
        uint8_t pad_3C[0x40 - 0x3C];
        uint16_t object_id; // 0x40
        TransportMode mode; // 0x42 field same in all vehicles
        uint8_t pad_43;
        uint16_t var_44;
        uint8_t var_46;
        uint8_t var_47;
        uint32_t accepted_cargo_types; // 0x48 front car component front bogie only
        uint8_t cargo_type;            // 0x4C front car component front bogie only
        uint8_t max_cargo;             // 0x4D front car component front bogie only
        uint8_t pad_4E[0x51 - 0x4E];
        uint8_t var_51;
        uint8_t pad_52[0x54 - 0x52];
        uint8_t body_index; // 0x54
        uint8_t pad_55;
        uint32_t creation_day; // 0x56
        uint8_t pad_5A[0x5E - 0x5A];
        uint8_t var_5E;
        uint8_t var_5F;
        uint8_t var_60;
        uint8_t var_61;
        uint32_t refund_cost; // 0x62 front bogies only
        uint16_t reliability; // 0x66 front bogies only
        uint16_t var_68;
        uint8_t var_6A;
    };
    static_assert(sizeof(vehicle_bogie) == 0x6B); // Can't use offset_of change this to last field if more found

    struct vehicle_tail : vehicle_base
    {
        static constexpr auto vehicleThingType = VehicleThingType::tail;
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint8_t pad_22[0x26 - 0x22];
        thing_id_t head; // 0x26
        uint32_t var_28;
        uint16_t var_2C;
        uint16_t var_2E;
        int16_t tile_x;      // 0x30
        int16_t tile_y;      // 0x32
        uint8_t tile_base_z; // 0x34
        uint8_t track_type;  // 0x35 field same in all vehicles
        uint16_t var_36;     // 0x36 field same in all vehicles
        uint8_t var_38;
        uint8_t pad_39;              // 0x39
        thing_id_t next_car_id;      // 0x3A
        uint8_t pad_3C[0x42 - 0x3C]; // 0x3C
        TransportMode mode;          // 0x42 field same in all vehicles
        uint8_t pad_43;
        sound_object_id_t soundId; // 0x44
        uint8_t pad_45[0x48 - 0x45];
        int16_t var_48;
        uint16_t var_4A;                       // sound-related flag(s) common with veh_2
        ui::window_number sound_window_number; // 0x4C common with veh_2
        ui::WindowType sound_window_type;      // 0x4E common with veh_2
    };
    static_assert(sizeof(vehicle_tail) == 0x4F); // Can't use offset_of change this to last field if more found

#pragma pack(pop)
    namespace things::vehicle
    {
        struct CarComponent
        {
            vehicle_bogie* front = nullptr;
            vehicle_bogie* back = nullptr;
            vehicle_body* body = nullptr;
            CarComponent(openloco::vehicle*& component)
            {
                front = component->asVehicleBogie();
                component = component->nextVehicleComponent();
                back = component->asVehicleBogie();
                component = component->nextVehicleComponent();
                body = component->asVehicleBody();
                component = component->nextVehicleComponent();
            }
            CarComponent() = default;
        };

        struct Car : public CarComponent
        {
            class CarComponentIter
            {
            private:
                CarComponent current;
                openloco::vehicle* nextVehicleComponent = nullptr;

            public:
                CarComponentIter(const CarComponent* carComponent)
                {
                    if (carComponent == nullptr)
                    {
                        nextVehicleComponent = nullptr;
                        return;
                    }
                    current = *carComponent;
                    nextVehicleComponent = reinterpret_cast<openloco::vehicle*>(current.body)->nextVehicleComponent();
                }

                CarComponentIter& operator++()
                {
                    if (nextVehicleComponent == nullptr)
                    {
                        return *this;
                    }
                    if (nextVehicleComponent->type == VehicleThingType::tail)
                    {
                        nextVehicleComponent = nullptr;
                        return *this;
                    }
                    CarComponent next{ nextVehicleComponent };
                    if (next.body == nullptr || next.body->type == VehicleThingType::body_start)
                    {
                        nextVehicleComponent = nullptr;
                        return *this;
                    }
                    current = next;
                    return *this;
                }

                CarComponentIter operator++(int)
                {
                    CarComponentIter retval = *this;
                    ++(*this);
                    return retval;
                }

                bool operator==(CarComponentIter other) const
                {
                    return nextVehicleComponent == other.nextVehicleComponent;
                }
                bool operator!=(CarComponentIter other) const
                {
                    return !(*this == other);
                }

                constexpr CarComponent& operator*()
                {
                    return current;
                }
                // iterator traits
                using difference_type = std::ptrdiff_t;
                using value_type = CarComponent;
                using pointer = CarComponent*;
                using reference = CarComponent&;
                using iterator_category = std::forward_iterator_tag;
            };

            CarComponentIter begin() const
            {
                return CarComponentIter(this);
            }
            CarComponentIter end() const
            {
                return CarComponentIter(nullptr);
            }

            Car(openloco::vehicle*& component)
                : CarComponent(component)
            {
            }
            Car() = default;
        };

        struct Vehicle
        {
            struct Cars
            {
                Car firstCar;
                class CarIter
                {
                private:
                    Car current;
                    openloco::vehicle* nextVehicleComponent = nullptr;

                public:
                    CarIter(const Car* carComponent)
                    {
                        if (carComponent == nullptr || carComponent->body == nullptr)
                        {
                            nextVehicleComponent = nullptr;
                            return;
                        }
                        current = *carComponent;
                        nextVehicleComponent = reinterpret_cast<openloco::vehicle*>(current.body)->nextVehicleComponent();
                    }

                    CarIter& operator++()
                    {
                        if (nextVehicleComponent == nullptr)
                        {
                            return *this;
                        }
                        while (nextVehicleComponent->type != VehicleThingType::tail)
                        {
                            Car next{ nextVehicleComponent };
                            if (next.body == nullptr)
                            {
                                break;
                            }
                            if (next.body->type == VehicleThingType::body_start)
                            {
                                current = next;
                                return *this;
                            }
                        }
                        nextVehicleComponent = nullptr;
                        return *this;
                    }

                    CarIter operator++(int)
                    {
                        CarIter retval = *this;
                        ++(*this);
                        return retval;
                    }

                    bool operator==(CarIter other) const
                    {
                        return nextVehicleComponent == other.nextVehicleComponent;
                    }
                    bool operator!=(CarIter other) const
                    {
                        return !(*this == other);
                    }

                    constexpr Car& operator*()
                    {
                        return current;
                    }
                    // iterator traits
                    using difference_type = std::ptrdiff_t;
                    using value_type = Car;
                    using pointer = Car*;
                    using reference = Car&;
                    using iterator_category = std::forward_iterator_tag;
                };

                CarIter begin() const
                {
                    return CarIter(&firstCar);
                }
                CarIter end() const
                {
                    return CarIter(nullptr);
                }

                std::size_t size() const
                {
                    if (firstCar.body == nullptr)
                    {
                        return 0;
                    }
                    return std::distance(begin(), end());
                }

                bool empty() const
                {
                    if (firstCar.body == nullptr)
                    {
                        return true;
                    }
                    return false;
                }

                Cars(Car&& _firstCar)
                    : firstCar(_firstCar)
                {
                }
                Cars() = default;
            };

            vehicle_head* head;
            vehicle_1* veh1;
            vehicle_2* veh2;
            vehicle_tail* tail;
            Cars cars;

            Vehicle(vehicle_head* _head)
                : Vehicle(_head->id)
            {
            }
            Vehicle(uint16_t _head);
        };
    }
}
