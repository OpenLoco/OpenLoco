#pragma once
#include "Vehicle.h"

namespace OpenLoco::Vehicles
{
    struct VehicleBody : VehicleBase
    {
        static constexpr auto kVehicleThingType = VehicleEntityType::body_continued;
        ColourScheme colourScheme;
        uint8_t objectSpriteType;
        uint16_t objectId;
        int16_t var_44;
        uint8_t animationFrame; //  roll/animation sprite index
        uint8_t cargoFrame;     //  cargo sprite index
        VehicleCargo primaryCargo;
        uint8_t bodyIndex;
        int8_t chuffSoundIndex;
        uint32_t creationDay;
        uint32_t var_5A;
        uint8_t wheelSlipping; // timeout that counts up
        BreakdownFlags breakdownFlags;
        uint32_t refundCost;
        uint8_t breakdownTimeout; // (likely unused)

        const VehicleObject* getObject() const;
        bool update(const CarUpdateState& carState);
        void secondaryAnimationUpdate(const Vehicle& train, const CarUpdateState& carState, const int32_t unkDistance);
        void updateSegmentCrashed(const CarUpdateState& carState);
        void sub_4AAB0B(const CarUpdateState& carState, const int32_t unkDistance);
        void updateCargoSprite();
        constexpr bool hasBreakdownFlags(BreakdownFlags flagsToTest) const
        {
            return (breakdownFlags & flagsToTest) != BreakdownFlags::none;
        }
        void sub_4AC255(VehicleBogie* backBogie, VehicleBogie* frontBogie);

    private:
        void animationUpdate(const CarUpdateState& carState, const int32_t unkDistance);
        void steamPuffsAnimationUpdate(const Vehicle& train, const CarUpdateState& carState, const int32_t unkDistance, uint8_t num, int32_t var_05);
        void dieselExhaust1AnimationUpdate(const Vehicle& train, const CarUpdateState& carState, uint8_t num, int32_t var_05);
        void dieselExhaust2AnimationUpdate(const Vehicle& train, const CarUpdateState& carState, uint8_t num, int32_t var_05);
        void electricSpark1AnimationUpdate(const Vehicle& train, const CarUpdateState& carState, const int32_t unkDistance, uint8_t num, int32_t var_05);
        void electricSpark2AnimationUpdate(const Vehicle& train, const CarUpdateState& carState, const int32_t unkDistance, uint8_t num, int32_t var_05);
        void shipWakeAnimationUpdate(const Vehicle& train, uint8_t num, int32_t var_05);
        Pitch updateSpritePitchSteepSlopes(uint16_t xyOffset, int16_t zOffset);
        Pitch updateSpritePitch(uint16_t xyOffset, int16_t zOffset);
    };
    static_assert(sizeof(VehicleBody) <= sizeof(Entity));
}
