#include "vehicle_channel.h"
#include "../interop/interop.hpp"
#include "../openloco.h"
#include "../things/thingmgr.h"
#include "../things/vehicle.h"

using namespace openloco;
using namespace openloco::audio;
using namespace openloco::interop;

static std::tuple<sound_id, channel_attributes> sub_48A590(const vehicle* v)
{
    registers regs;
    regs.esi = (loco_ptr)v;
    call(0x0048A590, regs);
    return std::make_tuple<sound_id, channel_attributes>((sound_id)regs.eax, { regs.ecx, regs.edx, regs.ebx });
}

vehicle_channel::vehicle_channel(channel&& c)
    : _channel(std::exchange(c, {}))
{
}

vehicle_channel::vehicle_channel(vehicle_channel&& vc)
    : _channel(std::exchange(vc._channel, {}))
    , _vehicle_id(std::exchange(vc._vehicle_id, thing_id::null))
    , _sound_id(std::exchange(vc._sound_id, {}))
    , _attributes(std::exchange(vc._attributes, {}))
{
}

vehicle_channel& vehicle_channel::operator=(vehicle_channel&& other)
{
    std::swap(_channel, other._channel);
    std::swap(_vehicle_id, other._vehicle_id);
    std::swap(_sound_id, other._sound_id);
    std::swap(_attributes, other._attributes);
    return *this;
}

void vehicle_channel::begin(thing_id_t vid)
{
    auto v = thingmgr::get<vehicle>(vid);
    if (v != nullptr)
    {
        // clang-format off
        auto [sid, sa] = sub_48A590(v);
        // clang-format on
        auto loop = audio::should_sound_loop(sid);
        auto sample = audio::get_sound_sample(sid);
        if (sample != nullptr)
        {
            _vehicle_id = vid;
            _sound_id = sid;
            _attributes = sa;

            _channel.load(*sample);
            _channel.play(loop);
            _channel.set_volume(sa.volume);
            _channel.set_pan(sa.pan);
            _channel.set_frequency(sa.freq);
        }
    }
}

void vehicle_channel::update()
{
    if (!is_free())
    {
        auto v = thingmgr::get<vehicle>(_vehicle_id);
        if (v != nullptr && v->base_type == thing_base_type::vehicle && (v->type == vehicle_thing_type::vehicle_2 || v->type == vehicle_thing_type::vehicle_6) && (v->var_4A & 1))
        {
            // clang-format off
            auto [sid, sa] = sub_48A590(v);
            // clang-format on
            if (_sound_id == sid)
            {
                v->var_4A &= ~1;
                if (_attributes.volume != sa.volume)
                {
                    _channel.set_volume(sa.volume);
                }
                if (_attributes.pan != sa.pan)
                {
                    _channel.set_pan(sa.pan);
                }
                if (_attributes.freq != sa.freq)
                {
                    _channel.set_frequency(sa.freq);
                }
                _attributes = sa;
                return;
            }
        }
        stop();
    }
}

void vehicle_channel::stop()
{
    _channel.stop();
    _vehicle_id = thing_id::null;
}
