#pragma once
#ifndef CATA_SRC_AMMO_EFFECT_H
#define CATA_SRC_AMMO_EFFECT_H

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "calendar.h"
#include "explosion.h"
#include "field_type.h"
#include "magic.h"
#include "type_id.h"

class JsonObject;
struct ammo_effect;
template <typename T> class generic_factory;

generic_factory<ammo_effect> &get_all_ammo_effects();

struct on_hit_effect {
    bool need_touch_skin;
    efftype_id effect;
    time_duration duration;
    int intensity;

    void deserialize( const JsonObject &jo );
};

struct ammo_effect {
    public:
        void load( const JsonObject &jo, std::string_view src );
        void finalize();
        void check() const;
        fake_spell spell_data;

        field_type_id aoe_field_type = fd_null.id_or( INVALID_FIELD_TYPE_ID );
        /** used during JSON loading only */
        int trigger_chance = 1;

        std::string aoe_field_type_name = "fd_null";
        int aoe_intensity_min = 0;
        int aoe_intensity_max = 0;
        int aoe_radius = 1;
        int aoe_radius_z = 0;
        int aoe_chance = 100;
        int aoe_size = 0;
        explosion_data aoe_explosion_data;
        bool aoe_check_passable = false;

        bool aoe_check_sees = false;
        int aoe_check_sees_radius = 0;
        bool do_flashbang = false;
        bool do_emp_blast = false;
        bool foamcrete_build = false;
        std::vector<effect_on_condition_id> eoc;
        bool always_cast_spell = false;

        field_type_id trail_field_type = fd_null.id_or( INVALID_FIELD_TYPE_ID );
        /** used during JSON loading only */
        std::string trail_field_type_name = "fd_null";
        int trail_intensity_min = 0;
        int trail_intensity_max = 0;
        int trail_chance = 100;

        std::vector<on_hit_effect> on_hit_effects;

        // Used by generic_factory
        ammo_effect_str_id id;
        std::vector<std::pair<ammo_effect_str_id, mod_id>> src;
        bool was_loaded = false;

        static size_t count();
};

namespace ammo_effects
{

void load( const JsonObject &jo, const std::string &src );
void finalize_all();
void check_consistency();
void reset();

const std::vector<ammo_effect> &get_all();

} // namespace ammo_effects

extern ammo_effect_id AE_NULL;

#endif // CATA_SRC_AMMO_EFFECT_H
