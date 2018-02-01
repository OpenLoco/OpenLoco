#include "station.h"
#include "companymgr.h"
#include "industrymgr.h"
#include "interop/interop.hpp"
#include "localisation/string_ids.h"
#include "map/tilemgr.h"
#include "messagemgr.h"
#include "objects/building_object.h"
#include "objects/cargo_object.h"
#include "objects/industry_object.h"
#include "objects/objectmgr.h"
#include "objects/road_station_object.h"
#include "openloco.h"
#include "ui/WindowManager.h"
#include "viewportmgr.h"
#include <algorithm>
#include <cassert>

using namespace openloco::interop;
using namespace openloco::map;
using namespace openloco::ui;

namespace openloco
{
    constexpr uint8_t min_cargo_rating = 0;
    constexpr uint8_t max_cargo_rating = 200;

    struct CargoSearchState
    {
    private:
        inline static loco_global<uint8_t[map_size], 0x00F00484> _map;
        inline static loco_global<uint32_t, 0x0112C68C> _filter;
        inline static loco_global<uint32_t[max_cargo_stats], 0x0112C690> _score;
        inline static loco_global<uint32_t, 0x0112C710> _dword_112C710;
        inline static loco_global<uint8_t[max_cargo_stats], 0x0112C7D2> _industry;
        inline static loco_global<uint8_t, 0x0112C7F2> _byte_112C7F2;

    public:
        bool mapHas2(tile_coord_t x, tile_coord_t y) const
        {
            return (_map[y * map_columns + x] & (1 << 1)) != 0;
        }

        void mapRemove2(tile_coord_t x, tile_coord_t y)
        {
            _map[y * map_columns + x] &= ~(1 << 1);
        }

        uint32_t filter() const
        {
            return _filter;
        }

        void filter(uint32_t value)
        {
            _filter = value;
        }

        void resetScores()
        {
            std::fill_n(_score.get(), max_cargo_stats, 0);
        }

        uint32_t score(int cargo)
        {
            return _score[cargo];
        }

        void addScore(int cargo, int32_t value)
        {
            _score[cargo] += value;
        }

        uint32_t dword_112C710() const
        {
            return _dword_112C710;
        }

        void dword_112C710(uint32_t value)
        {
            _dword_112C710 = value;
        }

        void byte_112C7F2(uint8_t value)
        {
            _byte_112C7F2 = value;
        }

        void resetIndustryMap()
        {
            std::fill_n(_industry.get(), max_cargo_stats, 0xFF);
        }

        industry_id_t getIndustry(int cargo) const
        {
            return _industry[cargo];
        }

        void setIndustry(int cargo, industry_id_t id)
        {
            _industry[cargo] = id;
        }
    };

    static void sub_491BF5(map_pos pos);

    station_id_t station::id() const
    {
        // TODO check if this is stored in station structure
        //      otherwise add it when possible
        static loco_global<station[1024], 0x005E6EDC> _stations;
        auto index = (size_t)(this - _stations);
        if (index > 1024)
        {
            index = station_id::null;
        }
        return (station_id_t)index;
    }

    // 0x0048B23E
    void station::update()
    {
        update_cargo_acceptance();
    }

    // 0x00492640
    void station::update_cargo_acceptance()
    {
        CargoSearchState cargoSearchState;
        uint32_t currentAcceptedCargo = calcAcceptedCargo(cargoSearchState);
        uint32_t originallyAcceptedCargo = 0;
        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            auto& cs = cargo_stats[cargoId];
            cs.industry_id = cargoSearchState.getIndustry(cargoId);
            if (cs.is_accepted())
            {
                originallyAcceptedCargo |= (1 << cargoId);
            }

            bool isNowAccepted = (currentAcceptedCargo & (1 << cargoId)) != 0;
            cs.is_accepted(isNowAccepted);
        }

        if (originallyAcceptedCargo != currentAcceptedCargo)
        {
            if (owner == companymgr::get_controlling_id())
            {
                alert_cargo_acceptance_change(originallyAcceptedCargo, currentAcceptedCargo);
            }
            invalidate_window();
        }
    }

    // 0x00492683
    void station::alert_cargo_acceptance_change(uint32_t oldCargoAcc, uint32_t newCargoAcc)
    {
        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            bool acceptedBefore = (oldCargoAcc & (1 << cargoId)) != 0;
            bool acceptedNow = (newCargoAcc & (1 << cargoId)) != 0;
            if (acceptedBefore && !acceptedNow)
            {
                messagemgr::post(
                    messageType::cargoNoLongerAccepted,
                    owner,
                    id(),
                    cargoId);
            }
            else if (!acceptedBefore && acceptedNow)
            {
                messagemgr::post(
                    messageType::cargoNowAccepted,
                    owner,
                    id(),
                    cargoId);
            }
        }
    }

    uint32_t station::calcAcceptedCargo(CargoSearchState& cargoSearchState)
    {
        return calcAcceptedCargo(cargoSearchState, map_pos(-1, -1), 0);
    }

    // 0x00491FE0
    // WARNING: this may be called with station (ebp) = -1
    uint32_t station::calcAcceptedCargo(CargoSearchState& cargoSearchState, map_pos location, uint32_t ebx)
    {
        cargoSearchState.byte_112C7F2(0);
        cargoSearchState.filter(0);
        if (location.x != -1)
        {
            cargoSearchState.filter(ebx);
        }

        cargoSearchState.resetIndustryMap();

        setStationCatchmentDisplay(true);

        if (location.x != -1)
        {
            sub_491BF5(location);
        }

        cargoSearchState.resetScores();
        cargoSearchState.dword_112C710(0);
        if (this != (station*)0xFFFFFFFF)
        {
            for (uint16_t i = 0; i < var_1CE; i++)
            {
                auto pos = var_1D0[i];
                auto baseZ = pos.z / 4;
                auto tile = tilemgr::get(pos);
                for (auto& el : tile)
                {
                    auto stationEl = el.as_station();
                    if (stationEl != nullptr && stationEl->base_z() != baseZ && !(stationEl->flags() & (1 << 5)))
                    {
                        cargoSearchState.byte_112C7F2(0);
                        if (stationEl->unk_5b() == 1)
                        {
                            auto obj = objectmgr::get<road_station_object>(stationEl->object_id());
                            if (obj->flags & (1 << 1))
                            {
                                cargoSearchState.filter(cargoSearchState.filter() | (1 << obj->var_2C));
                            }
                            else if (obj->flags & (1 << 2))
                            {
                                cargoSearchState.filter(cargoSearchState.filter() | ~(1 << obj->var_2C));
                            }
                        }
                        else
                        {
                            cargoSearchState.filter(~0);
                        }
                        break;
                    }
                }
            }
        }

        if (cargoSearchState.filter() == 0)
        {
            cargoSearchState.filter(~0);
        }

        for (tile_coord_t ty = 0; ty < map_columns; ty++)
        {
            for (tile_coord_t tx = 0; tx < map_rows; tx++)
            {
                if (cargoSearchState.mapHas2(tx, ty))
                {
                    auto pos = map_pos(tx * tile_size, ty * tile_size);
                    auto tile = tilemgr::get(pos);
                    for (auto& el : tile)
                    {
                        if (!el.is_flag_4())
                        {
                            switch (el.type())
                            {
                                case element_type::industry:
                                {
                                    auto industryEl = el.as_industry();
                                    auto industry = industryEl->industry();
                                    if (industry != nullptr && industry->under_construction == 0xFF)
                                    {
                                        auto obj = industry->object();
                                        if (obj != nullptr)
                                        {
                                            for (auto cargoId : obj->required_cargo_type)
                                            {
                                                if (cargoId != 0xFF && (cargoSearchState.filter() & (1 << cargoId)))
                                                {
                                                    cargoSearchState.addScore(cargoId, 8);
                                                    cargoSearchState.setIndustry(cargoId, industry->id());
                                                }
                                            }

                                            for (auto cargoId : obj->produced_cargo_type)
                                            {
                                                if (cargoId != 0xFF && (cargoSearchState.filter() & (1 << cargoId)))
                                                {
                                                    cargoSearchState.dword_112C710(cargoSearchState.dword_112C710() | (1 << cargoId));
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                                case element_type::building:
                                {
                                    auto buildingEl = el.as_building();
                                    auto obj = buildingEl->object();
                                    if (obj != nullptr)
                                    {
                                        for (int i = 0; i < 2; i++)
                                        {
                                            if (obj->var_A2[i] != 0xFF && (cargoSearchState.filter() & (1 << obj->var_A2[i])))
                                            {
                                                cargoSearchState.addScore(obj->var_A2[i], obj->var_A6[i]);
                                                if (obj->var_A0[i] != 0)
                                                {
                                                    cargoSearchState.dword_112C710(cargoSearchState.dword_112C710() | (1 << obj->var_A2[i]));
                                                }
                                            }
                                        }

                                        for (int i = 0; i < 2; i++)
                                        {
                                            if (obj->var_A4[i] != 0xFF && (cargoSearchState.filter() & (1 << obj->var_A4[i])))
                                            {
                                                cargoSearchState.addScore(obj->var_A4[i], obj->var_A8[i]);
                                            }
                                        }

                                        if (obj->flags & (1 << 0))
                                        {
                                            static coord_t word_4F9296[] = {
                                                0,
                                                0,
                                                32,
                                                32,
                                            };

                                            static coord_t word_4F9298[] = {
                                                0,
                                                32,
                                                32,
                                                0,
                                            };

                                            auto rotation = buildingEl->var_5b();
                                            tile_coord_t x = (pos.x - word_4F9296[rotation]) / tile_size;
                                            tile_coord_t y = (pos.y - word_4F9298[rotation]) / tile_size;
                                            cargoSearchState.mapRemove2(x + 0, y + 0);
                                            cargoSearchState.mapRemove2(x + 0, y + 1);
                                            cargoSearchState.mapRemove2(x + 1, y + 0);
                                            cargoSearchState.mapRemove2(x + 1, y + 1);
                                        }
                                    }
                                    break;
                                }
                                default:
                                    continue;
                            }
                        }
                    }
                }
            }
        }

        uint32_t acceptedCargos = 0;
        for (uint8_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            if (cargoSearchState.score(cargoId) >= 8)
            {
                acceptedCargos |= (1 << cargoId);
            }
        }
        return acceptedCargos;
    }

    // 0x00491D70
    void station::setStationCatchmentDisplay(bool hideCatchment)
    {
        registers regs;
        regs.dx = (int16_t)hideCatchment;
        regs.ebp = (int32_t)this;
        call(0x00491D70, regs);
    }

    // 0x0048F7D1
    void station::sub_48F7D1()
    {
        registers regs;
        regs.ebx = id();
        call(0x0048F7D1, regs);
    }

    // 0x00492A98
    void station::getStatusString(const char* buffer)
    {
        char* ptr = (char*)buffer;
        *ptr = '\0';

        for (uint32_t cargoId = 0; cargoId < max_cargo_stats; cargoId++)
        {
            auto& stats = cargo_stats[cargoId];

            if (stats.quantity == 0)
                continue;

            if (*buffer != '\0')
                ptr = stringmgr::format_string(ptr, string_ids::waiting_cargo_separator);

            loco_global<uint32_t, 0x112C826> _common_format_args;
            *_common_format_args = stats.quantity;

            auto cargo = objectmgr::get<cargo_object>(cargoId);
            string_id unit_name = stats.quantity == 1 ? cargo->unit_name_singular : cargo->unit_name_plural;
            ptr = stringmgr::format_string(ptr, unit_name, &*_common_format_args);
        }

        string_id suffix = *buffer == '\0' ? string_ids::nothing_waiting : string_ids::waiting;
        ptr = stringmgr::format_string(ptr, suffix);
    }

    // 0x00492793
    bool station::update_cargo()
    {
        bool atLeastOneGoodRating = false;
        bool quantityUpdated = false;

        var_3B0 = std::min(var_3B0 + 1, 255);
        var_3B1 = std::min(var_3B1 + 1, 255);

        auto& rng = gprng();
        for (uint32_t i = 0; i < max_cargo_stats; i++)
        {
            auto& cargo = cargo_stats[i];
            if (!cargo.empty())
            {
                if (cargo.quantity != 0 && cargo.origin != id())
                {
                    cargo.enroute_age = std::min(cargo.enroute_age + 1, 255);
                }
                cargo.age = std::min(cargo.age + 1, 255);

                auto targetRating = calculate_cargo_rating(cargo);
                // Limit to +/- 2 minimum change
                auto ratingDelta = std::clamp(targetRating - cargo.rating, -2, 2);
                cargo.rating += ratingDelta;

                if (cargo.rating <= 50)
                {
                    // Rating < 25%, decrease cargo
                    if (cargo.quantity >= 400)
                    {
                        cargo.quantity -= rng.rand_next(1, 32);
                        quantityUpdated = true;
                    }
                    else if (cargo.quantity >= 200)
                    {
                        cargo.quantity -= rng.rand_next(1, 8);
                        quantityUpdated = true;
                    }
                }
                if (cargo.rating >= 100)
                {
                    atLeastOneGoodRating = true;
                }
                if (cargo.rating <= 100 && cargo.quantity != 0)
                {
                    if (cargo.rating <= rng.rand_next(0, 127))
                    {
                        cargo.quantity = std::max(0, cargo.quantity - rng.rand_next(1, 4));
                        quantityUpdated = true;
                    }
                }
            }
        }

        sub_4929DB();

        auto w = WindowManager::find(WindowType::station, id());
        if (w != nullptr && (w->current_tab == 2 || w->current_tab == 1 || quantityUpdated))
        {
            w->invalidate();
        }

        return atLeastOneGoodRating;
    }

    // 0x004927F6
    int32_t station::calculate_cargo_rating(const station_cargo_stats& cargo) const
    {
        int32_t rating = 0;

        // Bonus if cargo is fresh
        if (cargo.age <= 45)
        {
            rating += 40;
            if (cargo.age <= 30)
            {
                rating += 45;
                if (cargo.age <= 15)
                {
                    rating += 45;
                    if (cargo.age <= 7)
                    {
                        rating += 35;
                    }
                }
            }
        }

        // Penalty if lots of cargo waiting
        rating -= 130;
        if (cargo.quantity <= 1000)
        {
            rating += 30;
            if (cargo.quantity <= 500)
            {
                rating += 30;
                if (cargo.quantity <= 300)
                {
                    rating += 30;
                    if (cargo.quantity <= 200)
                    {
                        rating += 20;
                        if (cargo.quantity <= 100)
                        {
                            rating += 20;
                        }
                    }
                }
            }
        }

        if ((flags & (station_flags::flag_7 | station_flags::flag_8)) == 0 && !is_player_company(owner))
        {
            rating = 120;
        }

        int32_t unk3 = std::min<uint8_t>(cargo.var_36, 250);
        if (unk3 < 35)
        {
            rating += unk3 / 4;
        }

        if (cargo.var_38 < 4)
        {
            rating += 10;
            if (cargo.var_38 < 2)
            {
                rating += 10;
                if (cargo.var_38 < 1)
                {
                    rating += 13;
                }
            }
        }

        return std::clamp<int32_t>(rating, min_cargo_rating, max_cargo_rating);
    }

    void station::sub_4929DB()
    {
        registers regs;
        regs.ebp = (int32_t)this;
        call(0x004929DB, regs);
    }

    // 0x004CBA2D
    void station::invalidate()
    {
        ui::viewportmgr::invalidate(this);
    }

    void station::invalidate_window()
    {
        WindowManager::invalidate(WindowType::station, id());
    }

    static void sub_491BF5(map_pos pos)
    {
        registers regs;
        regs.ax = pos.x;
        regs.cx = pos.y;
        call(0x00491BF5, regs);
    }
}
