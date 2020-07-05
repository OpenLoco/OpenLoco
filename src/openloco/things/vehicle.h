#pragma once

#include "../audio/audio.h"
#include "../company.h"
#include "../objects/vehicle_object.h"
#include "../ui/WindowType.h"
#include "../window.h"
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

    enum class vehicle_thing_type : uint8_t
    {
        vehicle_0 = 0,
        vehicle_1,
        vehicle_2,
        vehicle_bogie,
        vehicle_body_start,
        vehicle_body_cont,
        vehicle_6,
    };

#pragma pack(push, 1)
    struct vehicle_base : thing_base
    {
        vehicle_thing_type type;
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
        template<typename TType, vehicle_thing_type TClass>
        TType* as() const
        {
            // This can not use reinterpret_cast due to being a const member without considerable more code
            return type == TClass ? (TType*)this : nullptr;
        }

        template<typename TType>
        TType* as() const
        {
            return as<TType, TType::VehicleThingType>();
        }

    public:
        vehicle_head* as_vehicle_head() const { return as<vehicle_head>(); }
        vehicle_1* as_vehicle_1() const { return as<vehicle_1>(); }
        vehicle_2* as_vehicle_2() const { return as<vehicle_2>(); }
        vehicle_bogie* as_vehicle_bogie() const { return as<vehicle_bogie>(); }
        vehicle_body* as_vehicle_body() const
        {
            auto vehicle = as<vehicle_body, vehicle_thing_type::vehicle_body_start>();
            if (vehicle != nullptr)
                return vehicle;
            return as<vehicle_body, vehicle_thing_type::vehicle_body_cont>();
        }
        vehicle_26* as_vehicle_2or6() const
        {
            auto vehicle = as<vehicle_26, vehicle_thing_type::vehicle_2>();
            if (vehicle != nullptr)
                return vehicle;
            return as<vehicle_26, vehicle_thing_type::vehicle_6>();
        }
        vehicle_tail* as_vehicle_tail() const { return as<vehicle_tail>(); }
    };

    struct vehicle : vehicle_base
    {
        uint8_t pad_20;
        company_id_t owner; // 0x21
        uint16_t var_22;    // used for name on vehicle_0 will be type as string_id
        uint8_t pad_24[0x28 - 0x24];
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

        vehicle* next_vehicle();
        vehicle* next_vehicle_component();
        vehicle_object* object() const;

        void update_head();

    private:
        bool update();
    };
    static_assert(sizeof(vehicle) == 0x74); // Can't use offset_of change this to last field if more found

    struct vehicle_26 : vehicle_base
    {
        uint8_t pad_20[0x40 - 0x20];
        uint16_t object_id; // 0x40
        uint8_t pad_42[0x44 - 0x42];
        uint8_t sound_id; // 0x44
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
        static constexpr auto VehicleThingType = vehicle_thing_type::vehicle_0;
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

    private:
        void sub_4BAA76();
        uint32_t getVehicleTotalLength();
    };
    static_assert(sizeof(vehicle_head) == 0x7A); // Can't use offset_of change this to last field if more found

    struct vehicle_1 : vehicle_base
    {
        static constexpr auto VehicleThingType = vehicle_thing_type::vehicle_1;
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
        static constexpr auto VehicleThingType = vehicle_thing_type::vehicle_2;
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
        uint8_t var_44;
        uint8_t pad_45[0x48 - 0x45];
        int16_t var_48;
        uint16_t var_4A;
        uint8_t pad_4C[0x56 - 0x4C];
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
        static constexpr auto VehicleThingType = vehicle_thing_type::vehicle_body_cont;
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
        void secondary_animation_update();

    private:
        void sub_4AAB0B();
        void animation_update();
        void sub_4AC255(vehicle_bogie* back_bogie, vehicle_bogie* front_bogie);
        void steam_puffs_animation_update(uint8_t num, int32_t var_05);
        void diesel_exhaust1_animation_update(uint8_t num, int32_t var_05);
        void diesel_exhaust2_animation_update(uint8_t num, int32_t var_05);
        void electric_spark1_animation_update(uint8_t num, int32_t var_05);
        void electric_spark2_animation_update(uint8_t num, int32_t var_05);
        void ship_wake_animation_update(uint8_t num, int32_t var_05);
        uint8_t update_sprite_pitch_steep_slopes(uint16_t xy_offset, int16_t z_offset);
        uint8_t update_sprite_pitch(uint16_t xy_offset, int16_t z_offset);
        uint8_t update_sprite_yaw_0(int16_t x_offset, int16_t y_offset);
        uint8_t update_sprite_yaw_1(int16_t x_offset, int16_t y_offset);
        uint8_t update_sprite_yaw_2(int16_t x_offset, int16_t y_offset);
        uint8_t update_sprite_yaw_3(int16_t x_offset, int16_t y_offset);
        uint8_t update_sprite_yaw_4(int16_t x_offset, int16_t y_offset);
    };
    static_assert(sizeof(vehicle_body) == 0x60); // Can't use offset_of change this to last field if more found

    struct vehicle_bogie : vehicle_base
    {
        static constexpr auto VehicleThingType = vehicle_thing_type::vehicle_bogie;
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
        static constexpr auto VehicleThingType = vehicle_thing_type::vehicle_6;
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
        uint8_t var_44;
        uint8_t pad_45[0x48 - 0x45];
        int16_t var_48;
        uint16_t var_4A;
    };
    static_assert(sizeof(vehicle_tail) == 0x4C); // Can't use offset_of change this to last field if more found

#pragma pack(pop)
    namespace things::vehicle
    {
        struct CarComponent
        {
            vehicle_bogie* front;
            vehicle_bogie* back;
            vehicle_body* body;
        };

        struct Car
        {
            std::vector<CarComponent> carComponents;
            Car(openloco::vehicle*& component)
            {
                for (;
                     component->type != vehicle_thing_type::vehicle_6 && component->type != vehicle_thing_type::vehicle_body_start;
                     component = component->next_vehicle_component())
                {
                    auto front = component->as_vehicle_bogie();
                    component = component->next_vehicle_component();
                    auto back = component->as_vehicle_bogie();
                    component = component->next_vehicle_component();
                    auto body = component->as_vehicle_body();
                    carComponents.push_back(CarComponent{ front, back, body });
                }
            }
        };

        struct Train
        {
            vehicle_head* head;
            vehicle_1* veh1;
            vehicle_2* veh2;
            vehicle_tail* tail;

            std::vector<Car> cars;
            Train(vehicle_head* _head)
                : Train(_head->id)
            {
            }
            Train(uint16_t _head);
        };
    }
}
