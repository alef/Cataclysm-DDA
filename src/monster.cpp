#include "monster.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>

#include "ascii_art.h"
#include "avatar.h"
#include "bodypart.h"
#include "cached_options.h"
#include "cata_imgui.h"
#include "catacharset.h"
#include "character.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "damage.h"
#include "debug.h"
#include "dialogue.h"
#include "dialogue_helpers.h"
#include "effect.h"
#include "effect_on_condition.h"
#include "effect_source.h"
#include "enums.h"
#include "event.h"
#include "event_bus.h"
#include "explosion.h"
#include "faction.h"
#include "field_type.h"
#include "flat_set.h"
#include "game.h"
#include "harvest.h"
#include "imgui/imgui.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "magic.h"
#include "magic_enchantment.h"
#include "make_static.h"
#include "map.h"
#include "map_iterator.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "mattack_common.h"
#include "melee.h"
#include "messages.h"
#include "mission.h"
#include "mod_manager.h"
#include "mondeath.h"
#include "mondefense.h"
#include "monfaction.h"
#include "mongroup.h"
#include "mtype.h"
#include "mutation.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "pimpl.h"
#include "pocket_type.h"
#include "point.h"
#include "projectile.h"
#include "ret_val.h"
#include "rng.h"
#include "shearing.h"
#include "sounds.h"
#include "speed_description.h"
#include "string_formatter.h"
#include "talker.h"
#include "translation.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"
#include "units.h"
#include "viewer.h"
#include "weakpoint.h"
#include "weather.h"

static const ammo_effect_str_id ammo_effect_WHIP( "WHIP" );

static const anatomy_id anatomy_default_anatomy( "default_anatomy" );

static const damage_type_id damage_bash( "bash" );
static const damage_type_id damage_bullet( "bullet" );
static const damage_type_id damage_cut( "cut" );
static const damage_type_id damage_electric( "electric" );
static const damage_type_id damage_heat( "heat" );
static const damage_type_id damage_stab( "stab" );

static const efftype_id effect_badpoison( "badpoison" );
static const efftype_id effect_beartrap( "beartrap" );
static const efftype_id effect_bleed( "bleed" );
static const efftype_id effect_blind( "blind" );
static const efftype_id effect_bouldering( "bouldering" );
static const efftype_id effect_cramped_space( "cramped_space" );
static const efftype_id effect_critter_underfed( "critter_underfed" );
static const efftype_id effect_critter_well_fed( "critter_well_fed" );
static const efftype_id effect_crushed( "crushed" );
static const efftype_id effect_deaf( "deaf" );
static const efftype_id effect_disarmed( "disarmed" );
static const efftype_id effect_docile( "docile" );
static const efftype_id effect_downed( "downed" );
static const efftype_id effect_dripping_mechanical_fluid( "dripping_mechanical_fluid" );
static const efftype_id effect_eff_monster_immune_to_telepathy( "eff_monster_immune_to_telepathy" );
static const efftype_id effect_emp( "emp" );
static const efftype_id effect_fake_common_cold( "fake_common_cold" );
static const efftype_id effect_fake_flu( "fake_flu" );
static const efftype_id effect_grabbing( "grabbing" );
static const efftype_id effect_has_bag( "has_bag" );
static const efftype_id effect_heavysnare( "heavysnare" );
static const efftype_id effect_hit_by_player( "hit_by_player" );
static const efftype_id effect_in_pit( "in_pit" );
static const efftype_id effect_leashed( "leashed" );
static const efftype_id effect_lightsnare( "lightsnare" );
static const efftype_id effect_maimed_arm( "maimed_arm" );
static const efftype_id effect_monster_armor( "monster_armor" );
static const efftype_id effect_monster_saddled( "monster_saddled" );
static const efftype_id effect_nemesis_buff( "nemesis_buff" );
static const efftype_id effect_no_sight( "no_sight" );
static const efftype_id effect_onfire( "onfire" );
static const efftype_id effect_pacified( "pacified" );
static const efftype_id effect_paralyzepoison( "paralyzepoison" );
static const efftype_id effect_pet( "pet" );
static const efftype_id effect_photophobia( "photophobia" );
static const efftype_id effect_poison( "poison" );
static const efftype_id effect_psi_stunned( "psi_stunned" );
static const efftype_id effect_ridden( "ridden" );
static const efftype_id effect_run( "run" );
static const efftype_id effect_spooked( "spooked" );
static const efftype_id effect_spooked_recent( "spooked_recent" );
static const efftype_id effect_stunned( "stunned" );
static const efftype_id effect_supercharged( "supercharged" );
static const efftype_id effect_tied( "tied" );
static const efftype_id effect_tpollen( "tpollen" );
static const efftype_id effect_venom_dmg( "venom_dmg" );
static const efftype_id effect_venom_player1( "venom_player1" );
static const efftype_id effect_venom_player2( "venom_player2" );
static const efftype_id effect_venom_weaken( "venom_weaken" );
static const efftype_id effect_webbed( "webbed" );
static const efftype_id effect_worked_on( "worked_on" );

static const emit_id emit_emit_shock_cloud( "emit_shock_cloud" );
static const emit_id emit_emit_shock_cloud_big( "emit_shock_cloud_big" );

static const flag_id json_flag_CANNOT_MOVE( "CANNOT_MOVE" );
static const flag_id json_flag_CANNOT_TAKE_DAMAGE( "CANNOT_TAKE_DAMAGE" );
static const flag_id json_flag_DISABLE_FLIGHT( "DISABLE_FLIGHT" );
static const flag_id json_flag_GRAB( "GRAB" );
static const flag_id json_flag_GRAB_FILTER( "GRAB_FILTER" );

static const itype_id itype_beartrap( "beartrap" );
static const itype_id itype_milk( "milk" );
static const itype_id itype_milk_raw( "milk_raw" );
static const itype_id itype_rope_6( "rope_6" );
static const itype_id itype_snare_trigger( "snare_trigger" );

static const json_character_flag json_flag_ANIMALDISCORD( "ANIMALDISCORD" );
static const json_character_flag json_flag_ANIMALDISCORD2( "ANIMALDISCORD2" );
static const json_character_flag json_flag_ANIMALEMPATH( "ANIMALEMPATH" );
static const json_character_flag json_flag_ANIMALEMPATH2( "ANIMALEMPATH2" );
static const json_character_flag json_flag_BIONIC_LIMB( "BIONIC_LIMB" );

static const material_id material_bone( "bone" );
static const material_id material_flesh( "flesh" );
static const material_id material_hflesh( "hflesh" );
static const material_id material_iflesh( "iflesh" );
static const material_id material_iron( "iron" );
static const material_id material_steel( "steel" );
static const material_id material_stone( "stone" );
static const material_id material_veggy( "veggy" );

static const mfaction_str_id monfaction_acid_ant( "acid_ant" );
static const mfaction_str_id monfaction_ant( "ant" );
static const mfaction_str_id monfaction_bee( "bee" );
static const mfaction_str_id monfaction_nether_player_hate( "nether_player_hate" );
static const mfaction_str_id monfaction_wasp( "wasp" );

static const species_id species_AMPHIBIAN( "AMPHIBIAN" );
static const species_id species_CYBORG( "CYBORG" );
static const species_id species_FISH( "FISH" );
static const species_id species_FUNGUS( "FUNGUS" );
static const species_id species_HORROR( "HORROR" );
static const species_id species_LEECH_PLANT( "LEECH_PLANT" );
static const species_id species_MAMMAL( "MAMMAL" );
static const species_id species_MIGO( "MIGO" );
static const species_id species_MOLLUSK( "MOLLUSK" );
static const species_id species_NETHER( "NETHER" );
static const species_id species_PLANT( "PLANT" );
static const species_id species_PSI_NULL( "PSI_NULL" );
static const species_id species_ROBOT( "ROBOT" );
static const species_id species_ZOMBIE( "ZOMBIE" );
static const species_id species_nether_player_hate( "nether_player_hate" );

static const ter_str_id ter_t_gas_pump( "t_gas_pump" );
static const ter_str_id ter_t_gas_pump_a( "t_gas_pump_a" );

static const trait_id trait_BEE( "BEE" );
static const trait_id trait_FLOWERS( "FLOWERS" );
static const trait_id trait_INATTENTIVE( "INATTENTIVE" );
static const trait_id trait_MYCUS_FRIEND( "MYCUS_FRIEND" );
static const trait_id trait_PHEROMONE_AMPHIBIAN( "PHEROMONE_AMPHIBIAN" );
static const trait_id trait_PHEROMONE_INSECT( "PHEROMONE_INSECT" );
static const trait_id trait_PHEROMONE_MAMMAL( "PHEROMONE_MAMMAL" );
static const trait_id trait_TERRIFYING( "TERRIFYING" );
static const trait_id trait_THRESH_MYCUS( "THRESH_MYCUS" );

// Limit the number of iterations for next upgrade_time calculations.
// This also sets the percentage of monsters that will never upgrade.
// The rough formula is 2^(-x), e.g. for x = 5 it's 0.03125 (~ 3%).
static constexpr int UPGRADE_MAX_ITERS = 5;

static const std::map<creature_size, translation> size_names {
    { creature_size::tiny, to_translation( "size adj", "tiny" ) },
    { creature_size::small, to_translation( "size adj", "small" ) },
    { creature_size::medium, to_translation( "size adj", "medium" ) },
    { creature_size::large, to_translation( "size adj", "large" ) },
    { creature_size::huge, to_translation( "size adj", "huge" ) },
};

static const std::map<monster_attitude, std::pair<std::string, color_id>> attitude_names {
    {monster_attitude::MATT_FRIEND, {translate_marker( "Friendly." ), def_h_white}},
    {monster_attitude::MATT_FPASSIVE, {translate_marker( "Passive." ), def_h_white}},
    {monster_attitude::MATT_FLEE, {translate_marker( "Fleeing!" ), def_c_green}},
    {monster_attitude::MATT_FOLLOW, {translate_marker( "Tracking." ), def_c_yellow}},
    {monster_attitude::MATT_IGNORE, {translate_marker( "Ignoring." ), def_c_light_gray}},
    {monster_attitude::MATT_ATTACK, {translate_marker( "Hostile!" ), def_c_red}},
    {monster_attitude::MATT_NULL, {translate_marker( "BUG: Behavior unnamed." ), def_h_red}},
};

static int compute_kill_xp( const mtype_id &mon_type )
{
    return mon_type->difficulty + mon_type->difficulty_base;
}

monster::monster()
{
    unset_dest();
    wandf = 0;
    hp = 60;
    moves = 0;
    friendly = 0;
    anger = 0;
    morale = 2;
    faction = mfaction_id( 0 );
    no_extra_death_drops = false;
    dead = false;
    death_drops = true;
    made_footstep = false;
    hallucination = false;
    ignoring = 0;
    upgrades = false;
    upgrade_time = -1;
    stomach_timer = calendar::turn;
    last_updated = calendar::turn_zero;
    biosig_timer = calendar::before_time_starts;
    udder_timer = calendar::turn;
    horde_attraction = MHA_NULL;
    aggro_character = true;
    set_anatomy( anatomy_default_anatomy );
    set_body();
}

monster::monster( const mtype_id &id ) : monster()
{
    type = &id.obj();
    moves = type->speed;
    Creature::set_speed_base( type->speed );
    hp = type->hp;
    dialogue d( get_talker_for( this ), get_talker_for( get_avatar() ) );
    for( const auto &sa : type->special_attacks ) {
        mon_special_attack &entry = special_attacks[sa.first];
        entry.cooldown = rng( 0, sa.second->cooldown.evaluate( d ) );
    }
    anger = type->agro;
    morale = type->morale;
    faction = type->default_faction;
    upgrades = type->upgrades && ( type->half_life || type->age_grow );
    reproduces = type->reproduces && type->baby_timer && !monster::has_flag( mon_flag_NO_BREED );
    if( reproduces && type->baby_timer ) {
        baby_timer.emplace( calendar::turn + *type->baby_timer );
    }
    biosignatures = type->biosignatures;
    if( monster::has_flag( mon_flag_AQUATIC ) ) {
        fish_population = dice( 1, 20 );
    }
    if( monster::has_flag( mon_flag_RIDEABLE_MECH ) ) {
        itype_id mech_bat = itype_id( type->mech_battery );
        const itype &type = *item::find_type( mech_bat );
        int max_charge = type.magazine->capacity;
        item mech_bat_item = item( mech_bat, calendar::turn_zero );
        mech_bat_item.ammo_consume( rng( 0, max_charge ), tripoint_bub_ms::zero, nullptr );
        battery_item = cata::make_value<item>( mech_bat_item );
    }
    if( monster::has_flag( mon_flag_PET_MOUNTABLE ) ) {
        if( !type->mount_items.tied.is_empty() ) {
            itype_id tied_item_id = itype_id( type->mount_items.tied );
            item tied_item_item = item( tied_item_id, calendar::turn_zero );
            add_effect( effect_leashed, 1_turns, true );
            tied_item = cata::make_value<item>( tied_item_item );
        }
        if( !type->mount_items.tack.is_empty() ) {
            itype_id tack_item_id = itype_id( type->mount_items.tack );
            item tack_item_item = item( tack_item_id, calendar::turn_zero );
            add_effect( effect_monster_saddled, 1_turns, true );
            tack_item = cata::make_value<item>( tack_item_item );
        }
        if( !type->mount_items.armor.is_empty() ) {
            itype_id armor_item_id = itype_id( type->mount_items.armor );
            item armor_item_item = item( armor_item_id, calendar::turn_zero );
            add_effect( effect_monster_armor, 1_turns, true );
            armor_item = cata::make_value<item>( armor_item_item );
        }
        if( !type->mount_items.storage.is_empty() ) {
            itype_id storage_item_id = itype_id( type->mount_items.storage );
            item storage_item_item = item( storage_item_id, calendar::turn_zero );
            add_effect( effect_has_bag, 1_turns, true );
            tack_item = cata::make_value<item>( storage_item_item );
        }
    }
    aggro_character = type->aggro_character;
}

monster::monster( const mtype_id &id, const tripoint_bub_ms &p ) : monster( id )
{
    map &here = get_map();

    set_pos_bub_only( here, p );
    unset_dest();
}

monster::monster( const monster & ) = default;
monster::monster( monster && ) noexcept( map_is_noexcept ) = default;
monster::~monster() = default;
monster &monster::operator=( const monster & ) = default;
monster &monster::operator=( monster && ) noexcept( string_is_noexcept ) = default;

void monster::on_move( const tripoint_abs_ms &old_pos )
{
    Creature::on_move( old_pos );
    if( old_pos == pos_abs() ) {
        return;
    }
    g->update_zombie_pos( *this, old_pos, pos_abs() );
    if( has_effect( effect_ridden ) && mounted_player &&
        mounted_player->pos_abs() != pos_abs() ) {
        add_msg_debug( debugmode::DF_MONSTER, "Ridden monster %s moved independently and dumped player",
                       get_name() );
        mounted_player->forced_dismount();
    }
    if( has_dest() && pos_abs() == get_dest() ) {
        unset_dest();
    }
}

void monster::poly( const mtype_id &id )
{
    double hp_percentage = static_cast<double>( hp ) / static_cast<double>( type->hp );
    if( !no_extra_death_drops ) {
        generate_inventory();
    }
    type = &id.obj();
    moves = 0;
    Creature::set_speed_base( type->speed );
    anger = type->agro;
    morale = type->morale;
    hp = static_cast<int>( hp_percentage * type->hp );
    special_attacks.clear();
    dialogue d( get_talker_for( this ), get_talker_for( get_avatar() ) );
    for( const auto &sa : type->special_attacks ) {
        mon_special_attack &entry = special_attacks[sa.first];
        entry.cooldown = sa.second->cooldown.evaluate( d );
    }
    faction = type->default_faction;
    upgrades = type->upgrades;
    reproduces = type->reproduces;
    biosignatures = type->biosignatures;
    aggro_character = type->aggro_character;
}

bool monster::can_upgrade() const
{
    return upgrades && get_option<float>( "EVOLUTION_INVERSE_MULTIPLIER" ) > 0.0;
}

void monster::gravity_check()
{
    monster::gravity_check( &get_map() );
}

void monster::gravity_check( map *here )
{
    const tripoint_bub_ms pos = here->get_bub( pos_abs() );
    if( here->is_open_air( pos ) && !flies() ) {
        here->try_fall( pos, this );
    }
}

// For master special attack.
void monster::hasten_upgrade()
{
    if( !can_upgrade() || upgrade_time < 1 ) {
        return;
    }

    const int scaled_half_life = type->half_life * get_option<float>( "EVOLUTION_INVERSE_MULTIPLIER" );
    upgrade_time -= rng( 1, scaled_half_life );
    if( upgrade_time < 0 ) {
        upgrade_time = 0;
    }
}

int monster::get_upgrade_time() const
{
    return upgrade_time;
}

// Sets time to upgrade to 0.
void monster::allow_upgrade()
{
    upgrade_time = 0;
}

// This will disable upgrades in case max iters have been reached.
// Checking for return value of -1 is necessary.
int monster::next_upgrade_time()
{
    if( type->age_grow > 0 ) {
        return type->age_grow;
    }
    const int scaled_half_life = type->half_life * get_option<float>( "EVOLUTION_INVERSE_MULTIPLIER" );
    int day = 1; // 1 day of guaranteed evolve time
    for( int i = 0; i < UPGRADE_MAX_ITERS; i++ ) {
        if( one_in( 2 ) ) {
            day += rng( 0, scaled_half_life );
            return day;
        } else {
            day += scaled_half_life;
        }
    }
    // didn't manage to upgrade, shouldn't ever then
    upgrades = false;
    return -1;
}

void monster::try_upgrade( bool pin_time )
{
    map &here = get_map();

    if( !can_upgrade() ) {
        return;
    }

    const int current_day = to_days<int>( calendar::turn - calendar::turn_zero );
    //This should only occur when a monster is created or upgraded to a new form
    if( upgrade_time < 0 ) {
        upgrade_time = next_upgrade_time();
        if( upgrade_time < 0 ) {
            return;
        }
        if( pin_time || type->age_grow > 0 ) {
            // offset by today, always true for growing creatures
            upgrade_time += current_day;
        } else {
            // offset by starting season
            // TODO: revisit this and make it simpler
            upgrade_time += to_days<int>( calendar::start_of_cataclysm - calendar::turn_zero );
        }
    }

    // Here we iterate until we either are before upgrade_time or can't upgrade any more.
    // This is so that late into game new monsters can 'catch up' with all that half-life
    // upgrades they'd get if we were simulating whole world.
    while( true ) {
        if( upgrade_time > current_day ) {
            // not yet
            return;
        }

        if( type->upgrade_into ) {
            //If we upgrade into a blacklisted monster, treat it as though we are non-upgradeable
            if( MonsterGroupManager::monster_is_blacklisted( type->upgrade_into ) ) {
                return;
            }
            poly( type->upgrade_into );
        } else {
            mtype_id new_type;
            if( type->upgrade_multi_range ) {
                bool ret_default = false;
                std::vector<MonsterGroupResult> res = MonsterGroupManager::GetResultFromGroup(
                        type->upgrade_group, nullptr, nullptr, false, &ret_default );
                if( !res.empty() && !ret_default ) {
                    // Set the type to poly the current monster (preserves inventory)
                    new_type = res.front().id;
                    res.front().pack_size--;
                    for( const MonsterGroupResult &mgr : res ) {
                        if( !mgr.id ) {
                            continue;
                        }
                        for( int i = 0; i < mgr.pack_size; i++ ) {
                            tripoint_bub_ms spawn_pos;
                            if( g->find_nearby_spawn_point( pos_bub(), mgr.id, 1, *type->upgrade_multi_range,
                                                            spawn_pos, false, false ) ) {
                                monster *spawned = g->place_critter_at( mgr.id, spawn_pos );
                                if( spawned ) {
                                    spawned->friendly = friendly;
                                }
                            }
                        }
                    }
                }
            } else {
                new_type = MonsterGroupManager::GetRandomMonsterFromGroup( type->upgrade_group );
            }
            if( !new_type.is_empty() ) {
                if( new_type ) {
                    poly( new_type );
                } else {
                    // "upgrading" to mon_null
                    if( type->upgrade_null_despawn ) {
                        g->remove_zombie( *this );
                    } else {
                        die( &here, nullptr );
                    }
                    return;
                }
            }
        }

        if( !upgrades ) {
            // upgraded into a non-upgradeable monster
            return;
        }

        const int next_upgrade = next_upgrade_time();
        if( next_upgrade < 0 ) {
            // hit never_upgrade
            return;
        }
        upgrade_time += next_upgrade;
    }
}

void monster::set_baby_timer( const time_point &time )
{
    baby_timer.emplace( time );
}

void monster::try_reproduce()
{
    if( !reproduces ) {
        return;
    }
    // This can happen if the monster type has changed (from reproducing to non-reproducing monster)
    if( !type->baby_timer ) {
        return;
    }

    bool season_spawn = false;
    bool season_match = true;

    // only 50% of animals should reproduce
    bool female = one_in( 2 );
    for( const std::string &elem : type->baby_flags ) {
        if( elem == "SUMMER" || elem == "WINTER" || elem == "SPRING" || elem == "AUTUMN" ) {
            season_spawn = true;
        }
    }

    map &here = get_map();
    // add a decreasing chance of additional spawns when "catching up" an existing animal.
    int chance = -1;
    while( true ) {
        if( !baby_timer.has_value() || *baby_timer > calendar::turn ) {
            return;
        }

        if( season_spawn ) {
            season_match = false;
            for( const std::string &elem : type->baby_flags ) {
                if( ( season_of_year( *baby_timer ) == SUMMER && elem == "SUMMER" ) ||
                    ( season_of_year( *baby_timer ) == WINTER && elem == "WINTER" ) ||
                    ( season_of_year( *baby_timer ) == SPRING && elem == "SPRING" ) ||
                    ( season_of_year( *baby_timer ) == AUTUMN && elem == "AUTUMN" ) ) {
                    season_match = true;
                }
            }
        }

        chance += 2;
        if( !has_eaten_enough() ) {
            chance += 1; //Reduce the chances but don't prevent birth if the animal is not eating.
        }
        if( season_match && female && one_in( chance ) ) {
            int spawn_cnt = rng( 1, type->baby_count );
            if( !type->baby_type.baby_monster.is_null() ) {
                here.add_spawn( type->baby_type.baby_monster, spawn_cnt, pos_bub(), friendly );
            }
            if( !type->baby_type.baby_monster_group.is_null() ) {
                std::vector<MonsterGroupResult> babies = MonsterGroupManager::GetResultFromGroup(
                            type->baby_type.baby_monster_group, &spawn_cnt,
                            nullptr, false, nullptr, true );
                for( const MonsterGroupResult &mgr : babies ) {
                    here.add_spawn( mgr.id, std::max( 1, spawn_cnt * mgr.pack_size ), pos_bub(), friendly );
                }
            }
            if( !type->baby_type.baby_egg.is_null() ) {
                const item egg( type->baby_type.baby_egg, *baby_timer );
                for( int i = 0; i < spawn_cnt; i++ ) {
                    here.add_item_or_charges( pos_bub(), egg, true );
                }
            }
            if( !type->baby_type.baby_egg_group.is_null() ) {
                here.spawn_items( pos_bub(), item_group::items_from( type->baby_type.baby_egg_group ) );
            }
        }
        *baby_timer += *type->baby_timer;
    }
}

void monster::refill_udders()
{
    if( type->starting_ammo.empty() ) {
        debugmsg( "monster %s has no starting ammo to refill udders", get_name() );
        return;
    }
    if( !has_eaten_enough() ) {
        return;
    }
    if( ammo.empty() ) {
        // legacy animals got empty ammo map, fill them up now if needed.
        ammo[type->starting_ammo.begin()->first] = type->starting_ammo.begin()->second;
    }
    auto current_milk = ammo.find( itype_milk_raw );
    if( current_milk == ammo.end() ) {
        current_milk = ammo.find( itype_milk );
        if( current_milk != ammo.end() ) {
            // take this opportunity to update milk udders to raw_milk
            ammo[itype_milk_raw] = current_milk->second;
            // Erase old key-value from map
            ammo.erase( current_milk );
        }
    }
    // if we got here, we got milk.
    if( current_milk->second == type->starting_ammo.begin()->second ) {
        // already full up
        return;
    }
    if( calendar::turn - udder_timer > 1_days ) {
        // You milk once a day.
        ammo.begin()->second = type->starting_ammo.begin()->second;
        udder_timer = calendar::turn;
    }
}

void monster::recheck_fed_status()
{
    remove_effect( effect_critter_underfed );
    remove_effect( effect_critter_well_fed );
    if( !has_flag( mon_flag_EATS ) ) {
        return;
    }
    if( has_fully_eaten() ) {
        add_effect( effect_critter_well_fed, 24_hours );
    } else if( !has_eaten_enough() ) {
        add_effect( effect_critter_underfed, 24_hours );
    }
}

void monster::set_amount_eaten( int new_amount )
{
    amount_eaten = new_amount;
    recheck_fed_status();
}

void monster::mod_amount_eaten( int amount_to_add )
{
    amount_eaten += amount_to_add;
    recheck_fed_status();
}

int monster::get_amount_eaten() const
{
    return amount_eaten;
}

int monster::get_stomach_fullness_percent() const
{
    if( type->stomach_size == 0 ) {
        return 100; // no div-by-zero
    }
    return get_amount_eaten() / type->stomach_size;
}

bool monster::has_eaten_enough() const
{
    return !has_effect( effect_critter_underfed ) &&
           ( has_effect( effect_critter_well_fed ) || has_fully_eaten() );
}


bool monster::has_fully_eaten() const
{
    return amount_eaten >= type->stomach_size;
}

void monster::digest_food()
{
    if( calendar::turn - stomach_timer > 1_days ) {
        recheck_fed_status();
        set_amount_eaten( 0 );
        stomach_timer = calendar::turn;
    }
}

void monster::try_biosignature()
{
    if( is_hallucination() ) {
        return;
    }

    if( !biosignatures ) {
        return;
    }
    if( !type->biosig_timer ) {
        return;
    }
    if( !has_eaten_enough() ) {
        return;
    }

    if( !biosig_timer ) {
        biosig_timer.emplace( calendar::turn + *type->biosig_timer );
    }
    map &here = get_map();
    int counter = 0;
    while( true ) {
        // don't catch up too much, otherwise on some scenarios,
        // we could have years worth of poop just deposited on the floor.
        if( *biosig_timer > calendar::turn || counter > 50 ) {
            return;
        }
        here.add_item_or_charges( pos_bub(), item( type->biosig_item, *biosig_timer, 1 ), true );
        *biosig_timer += *type->biosig_timer;
        counter += 1;
    }
}

void monster::spawn( const tripoint_bub_ms &p )
{
    map &here = get_map();

    set_pos_bub_only( here, p );
    unset_dest();
}

void monster::spawn( const tripoint_abs_ms &loc )
{
    set_pos_abs_only( loc );
    unset_dest();
}

std::string monster::get_name() const
{
    return name( 1 );
}

std::string monster::name( unsigned int quantity ) const
{
    if( !type ) {
        debugmsg( "monster::name empty type!" );
        return std::string();
    }
    std::string result = type->nname( quantity );
    if( !nickname.empty() ) {
        return nickname;
    }
    if( !unique_name.empty() ) {
        //~ %1$s: monster name, %2$s: unique name
        result = string_format( pgettext( "unique monster name", "%1$s: %2$s" ),
                                result, unique_name );
    }
    if( !mission_fused.empty() ) {
        //~ name when a monster fuses with a mission target
        result = string_format( pgettext( "fused mission monster", "*%s" ), result );
    }
    return result;
}

// TODO: MATERIALS put description in materials.json?
std::string monster::name_with_armor() const
{
    std::string ret;
    if( made_of( material_iflesh ) ) {
        ret = _( "carapace" );
    } else if( made_of( material_veggy ) ) {
        ret = _( "thick bark" );
    } else if( made_of( material_bone ) ) {
        ret = _( "exoskeleton" );
    } else if( made_of( material_flesh ) || made_of( material_hflesh ) ) {
        ret = _( "thick hide" );
    } else if( made_of( material_iron ) || made_of( material_steel ) ) {
        ret = _( "armor plating" );
    } else if( made_of( phase_id::LIQUID ) ) {
        ret = _( "dense jelly mass" );
    } else {
        ret = _( "armor" );
    }
    if( has_effect( effect_monster_armor ) && !inv.empty() ) {
        for( const item &armor : inv ) {
            if( armor.is_pet_armor( true ) ) {
                ret += string_format( _( "wearing %1$s" ), armor.tname( 1 ) );
                break;
            }
        }
    }

    return ret;
}

std::string monster::disp_name( bool possessive, bool capitalize_first ) const
{
    if( !possessive ) {
        return string_format( capitalize_first ? _( "The %s" ) : _( "the %s" ), name() );
    } else {
        return string_format( capitalize_first ? _( "The %s's" ) : _( "the %s's" ), name() );
    }
}

std::string monster::skin_name() const
{
    return name_with_armor();
}

void monster::get_HP_Bar( nc_color &color, std::string &text ) const
{
    std::tie( text, color ) = ::get_hp_bar( hp, type->hp, true );
}

std::pair<std::string, nc_color> monster::get_attitude() const
{
    const auto att = attitude_names.at( attitude( &get_player_character() ) );
    return {
        _( att.first ),
        all_colors.get( att.second )
    };
}

static std::pair<std::string, nc_color> hp_description( int cur_hp, int max_hp )
{
    std::string damage_info;
    nc_color col;
    if( cur_hp >= max_hp ) {
        damage_info = _( "It is uninjured." );
        col = c_green;
    } else if( cur_hp >= max_hp * 0.8 ) {
        damage_info = _( "It is lightly injured." );
        col = c_light_green;
    } else if( cur_hp >= max_hp * 0.6 ) {
        damage_info = _( "It is moderately injured." );
        col = c_yellow;
    } else if( cur_hp >= max_hp * 0.3 ) {
        damage_info = _( "It is heavily injured." );
        col = c_yellow;
    } else if( cur_hp >= max_hp * 0.1 ) {
        damage_info = _( "It is severely injured." );
        col = c_light_red;
    } else {
        damage_info = _( "It is nearly dead!" );
        col = c_red;
    }

    if( debug_mode ) {
        damage_info += "  ";
        damage_info += string_format( _( "%1$d/%2$d HP" ), cur_hp, max_hp );
    }

    return std::make_pair( damage_info, col );
}

std::string monster::speed_description( float mon_speed_rating,
                                        bool immobile,
                                        const speed_description_id &speed_desc )
{
    if( speed_desc.is_null() || !speed_desc.is_valid() ) {
        return std::string();
    }

    const avatar &ply = get_avatar();
    double player_runcost = ply.run_cost( 100 );
    if( player_runcost <= 0.00 ) {
        player_runcost = 0.01f;
    }

    // tpt = tiles per turn
    const double player_tpt = ply.get_speed() / player_runcost;
    double ratio_tpt = player_tpt == 0.00 ?
                       100.00 : mon_speed_rating / player_tpt;
    ratio_tpt *= immobile ? 0.00 : 1.00;

    for( const speed_description_value &speed_value : speed_desc->values() ) {
        if( ratio_tpt >= speed_value.value() ) {
            return random_entry( speed_value.descriptions(), translation() ).translated();
        }
    }

    return std::string();
}

int monster::print_info( const catacurses::window &w, int vStart, int vLines, int column ) const
{
    const map &here = get_map();

    const int vEnd = vStart + vLines;
    const int max_width = getmaxx( w ) - column - 1;
    std::ostringstream oss;

    oss << get_tag_from_color( c_white ) << get_origin( type->src ) << "</color>" << "\n";

    if( debug_mode ) {
        oss << colorize( type->id.str(), c_white );
    }
    oss << "\n";

    // Print health bar, monster name, then statuses on the first line.
    nc_color bar_color = c_white;
    std::string bar_str;
    get_HP_Bar( bar_color, bar_str );
    oss << get_tag_from_color( bar_color ) << bar_str << "</color>";
    oss << "<color_white>" << std::string( 5 - utf8_width( bar_str ), '.' ) << "</color> ";
    oss << get_tag_from_color( basic_symbol_color() ) << name() << "</color> ";
    oss << "<color_h_white>" << get_effect_status() << "</color>";
    vStart += fold_and_print( w, point( column, vStart ), max_width, c_white, oss.str() );

    Character &pc = get_player_character();
    bool sees_player = sees( here, pc );
    const bool player_knows = !pc.has_trait( trait_INATTENTIVE );

    // Hostility indicator on the second line.
    std::pair<std::string, nc_color> att = get_attitude();
    if( player_knows ) {
        mvwprintz( w, point( column, vStart++ ), att.second, att.first );
    }

    // Awareness indicator in the third line.
    std::string senses_str = sees_player ? _( "Can see to your current location" ) :
                             _( "Can't see to your current location" );

    if( player_knows ) {
        vStart += fold_and_print( w, point( column, vStart ), max_width, player_knows &&
                                  sees_player ? c_red : c_green,
                                  senses_str );
    }

    const std::string speed_desc = speed_description(
                                       speed_rating(),
                                       has_flag( mon_flag_IMMOBILE ) || has_flag( json_flag_CANNOT_MOVE ),
                                       type->speed_desc );
    vStart += fold_and_print( w, point( column, vStart ), max_width, c_white, speed_desc );

    // Monster description on following lines.
    std::vector<std::string> lines = foldstring( type->get_description(), max_width );
    int numlines = lines.size();
    wattron( w, c_light_gray );
    for( int i = 0; i < numlines && vStart < vEnd; i++ ) {
        mvwprintw( w, point( column, vStart++ ), lines[i] );
    }

    if( !mission_fused.empty() ) {
        // Mission monsters fused into this monster
        const std::string fused_desc = string_format( _( "Parts of %s protrude from its body." ),
                                       enumerate_as_string( mission_fused ) );
        lines = foldstring( fused_desc, max_width );
        numlines = lines.size();
        for( int i = 0; i < numlines && vStart < vEnd; i++ ) {
            mvwprintw( w, point( column, ++vStart ), lines[i] );
        }
    }
    wattroff( w, c_light_gray );

    // Riding indicator on next line after description.
    if( has_effect( effect_ridden ) && mounted_player ) {
        mvwprintz( w, point( column, ++vStart ), c_white, _( "Rider: %s" ), mounted_player->disp_name() );
    }

    // Show monster size on the last line
    if( size_bonus > 0 ) {
        mvwprintz( w, point( column, ++vStart ), c_light_gray, _( " It is %s." ),
                   size_names.at( get_size() ) );
    }

    if( get_option<bool>( "ENABLE_ASCII_ART" ) ) {
        const ascii_art_id art = type->get_picture_id();
        if( art.is_valid() ) {
            for( const std::string &line : art->picture ) {
                fold_and_print( w, point( column, ++vStart ), max_width, c_white, line );
            }
        }
    }
    return ++vStart;
}

void monster::print_info_imgui() const
{
    const map &here = get_map();

    ImGui::TextUnformatted( get_origin( type->src ).c_str() );

    if( debug_mode ) {
        ImGui::TextUnformatted( type->id.c_str() );
    }
    ImGui::NewLine();

    // Print health bar, monster name, then statuses on the first line.
    nc_color bar_color = c_white;
    std::string bar_str;
    get_HP_Bar( bar_color, bar_str );
    ImGui::TextColored( bar_color, "%s", bar_str.c_str() );
    std::string unbar_str = std::string( 5 - utf8_width( bar_str ), '.' );
    ImGui::SameLine( 0, 0 );
    ImGui::TextColored( c_white, "%s", unbar_str.c_str() );
    nc_color symbol_color = basic_symbol_color();
    ImGui::SameLine();
    ImGui::TextColored( symbol_color, "%s %s", name().c_str(),
                        get_effect_status().c_str() );

    if( debug_mode ) {
        dialogue d( get_talker_for( const_cast<monster *>( this ) ), get_talker_for( get_avatar() ) );
        for( const std::pair<const std::string, mtype_special_attack> &attack : type->special_attacks ) {
            ImGui::Text( "%s: %g", attack.first.c_str(), attack.second->cooldown.evaluate( d ) );
        }
    }

    Character &pc = get_player_character();
    bool sees_player = sees( here, pc );
    const bool player_knows = !pc.has_trait( trait_INATTENTIVE );

    // Hostility indicator on the second line.
    std::pair<std::string, nc_color> att = get_attitude();
    if( player_knows ) {
        ImGui::TextColored( att.second, "%s", att.first.c_str() );
    }

    // Awareness indicator in the third line.
    std::string senses_str = sees_player ? _( "Can see to your current location" ) :
                             _( "Can't see to your current location" );

    if( player_knows ) {
        cataimgui::draw_colored_text( senses_str, ( player_knows && sees_player ) ? c_red : c_green );
    }

    const std::string speed_desc = speed_description(
                                       speed_rating(),
                                       has_flag( mon_flag_IMMOBILE ) || has_flag( json_flag_CANNOT_MOVE ),
                                       type->speed_desc );
    cataimgui::draw_colored_text( speed_desc, c_white );

    // Monster description on following lines.
    ImGui::NewLine();
    ImGui::TextWrapped( "%s", type->get_description().c_str() );
    ImGui::NewLine();

    if( !mission_fused.empty() ) {
        // Mission monsters fused into this monster
        const std::string fused_desc = string_format( _( "Parts of %s protrude from its body." ),
                                       enumerate_as_string( mission_fused ) );
        ImGui::TextWrapped( "%s", fused_desc.c_str() );
    }

    // Riding indicator on next line after description.
    if( has_effect( effect_ridden ) && mounted_player ) {
        ImGui::Text( _( "Rider: %s" ), mounted_player->disp_name().c_str() );
    }

    // Show monster size on the last line
    if( size_bonus > 0 ) {
        ImGui::Text( _( " It is %s." ), size_names.at( get_size() ).translated().c_str() );
    }

    if( get_option<bool>( "ENABLE_ASCII_ART" ) ) {
        const ascii_art_id art = type->get_picture_id();
        if( art.is_valid() ) {
            cataimgui::PushMonoFont();
            for( const std::string &line : art->picture ) {
                cataimgui::draw_colored_text( line, c_white );
            }
            ImGui::PopFont();
        }
    }
}

std::vector<std::string> monster::extended_description() const
{
    const map &here = get_map();

    std::vector<std::string> tmp;
    // Reserve the number of elements we know we will need.
    // Likely we will need more, but it's best to leave that to
    // exponential growth.
    tmp.reserve( 12 );

    tmp.emplace_back( get_origin( type->src ) );
    tmp.emplace_back( "--" );

    if( debug_mode ) {
        tmp.emplace_back( colorize( type->id.str(), c_white ) );
    }

    nc_color bar_color = c_white;
    std::string bar_str;
    get_HP_Bar( bar_color, bar_str );
    std::string hpbar = colorize( bar_str, bar_color ) +
                        colorize( std::string( 5 - utf8_width( bar_str ), '.' ), c_white );
    Character &pc = get_player_character();
    const bool player_knows = !pc.has_trait( trait_INATTENTIVE );
    const std::pair<std::string, nc_color> att = get_attitude();

    std::string att_colored = colorize( att.first, att.second );

    tmp.emplace_back( string_format( _( "This is a %s %s. %s" ), hpbar, colorize( name(),
                                     basic_symbol_color() ), player_knows ? att_colored : std::string() ) );
    tmp.emplace_back( "--" );

    std::string difficulty_str;
    if( debug_mode ) {
        difficulty_str = _( "Difficulty " ) + std::to_string( type->difficulty );
    } else {
        if( type->difficulty < 3 ) {
            difficulty_str = _( "<color_light_gray>Minimal threat.</color>" );
        } else if( type->difficulty < 10 ) {
            difficulty_str = _( "<color_light_gray>Mildly dangerous.</color>" );
        } else if( type->difficulty < 20 ) {
            difficulty_str = _( "<color_light_red>Dangerous.</color>" );
        } else if( type->difficulty < 30 ) {
            difficulty_str = _( "<color_red>Very dangerous.</color>" );
        } else if( type->difficulty < 50 ) {
            difficulty_str = _( "<color_red>Extremely dangerous.</color>" );
        } else {
            difficulty_str = _( "<color_red>Fatally dangerous!</color>" );
        }
    }
    tmp.emplace_back( difficulty_str );

    std::string eff = get_effect_status();
    if( !eff.empty() ) {
        tmp.emplace_back( string_format( _( "<stat>It is %s.</stat>" ), eff ) );
    }

    bool sees_player = sees( here, pc );
    std::string senses_str = sees_player ? colorize( _( "Can see to your current location" ), c_red ) :
                             colorize( _( "Can't see to your current location" ), c_green );
    tmp.emplace_back( senses_str );

    const std::pair<std::string, nc_color> hp_bar = hp_description( hp, type->hp );
    tmp.emplace_back( colorize( hp_bar.first, hp_bar.second ) );

    const std::string speed_desc = speed_description(
                                       speed_rating(),
                                       has_flag( mon_flag_IMMOBILE ) || has_flag( json_flag_CANNOT_MOVE ),
                                       type->speed_desc );
    tmp.emplace_back( speed_desc );

    tmp.emplace_back( "--" );
    tmp.emplace_back( string_format( "<dark>%s</dark>", type->get_description() ) );
    tmp.emplace_back( "--" );
    if( !mission_fused.empty() ) {
        // Mission monsters fused into this monster
        const std::string fused_desc = string_format( _( "Parts of %s protrude from its body." ),
                                       enumerate_as_string( mission_fused ) );
        tmp.emplace_back( string_format( "<dark>%s</dark>", fused_desc ) );
        tmp.emplace_back( "--" );
    }

    tmp.emplace_back( string_format( _( "It is %s in size." ),
                                     size_names.at( get_size() ) ) );

    std::vector<std::string> types = type->species_descriptions();
    if( type->has_flag( mon_flag_ANIMAL ) ) {
        types.emplace_back( _( "an animal" ) );
    }
    if( !types.empty() ) {
        tmp.emplace_back( string_format( _( "It is %s." ),
                                         enumerate_as_string( types ) ) );
    }

    using flag_description = std::pair<const mon_flag_id, std::string>;
    const auto describe_flags = [this, &tmp](
                                    std::string_view format,
                                    const std::vector<flag_description> &flags_names,
    const std::string &if_empty = "" ) {
        std::string flag_descriptions = enumerate_as_string( flags_names.begin(),
        flags_names.end(), [this]( const flag_description & fd ) {
            return type->has_flag( fd.first ) ? fd.second : "";
        } );
        if( !flag_descriptions.empty() ) {
            tmp.emplace_back( string_format( format, flag_descriptions ) );
        } else if( !if_empty.empty() ) {
            tmp.emplace_back( if_empty );
        }
    };

    using property_description = std::pair<bool, std::string>;
    const auto describe_properties = [&tmp](
                                         std::string_view format,
                                         const std::vector<property_description> &property_names,
    const std::string &if_empty = "" ) {
        std::string property_descriptions = enumerate_as_string( property_names.begin(),
        property_names.end(), []( const property_description & pd ) {
            return pd.first ? pd.second : "";
        } );
        if( !property_descriptions.empty() ) {
            tmp.emplace_back( string_format( format, property_descriptions ) );
        } else if( !if_empty.empty() ) {
            tmp.emplace_back( if_empty );
        }
    };

    describe_flags( _( "It has the following senses: %s." ), {
        {mon_flag_HEARS, pgettext( "Hearing as sense", "hearing" )},
        {mon_flag_SEES, pgettext( "Sight as sense", "sight" )},
        {mon_flag_SMELLS, pgettext( "Smell as sense", "smell" )},
    }, _( "It doesn't have senses." ) );

    describe_properties( _( "It can %s." ), {
        {swims(), pgettext( "Swim as an action", "swim" )},
        {flies(), pgettext( "Fly as an action", "fly" )},
        {can_dig(), pgettext( "Dig as an action", "dig" )},
        {climbs(), pgettext( "Climb as an action", "climb" )}
    } );

    describe_flags( _( "<bad>In fight it can %s.</bad>" ), {
        {mon_flag_GRABS, pgettext( "Grab as an action", "grab" )},
        {mon_flag_VENOM, pgettext( "Poison as an action", "poison" )},
        {mon_flag_PARALYZEVENOM, pgettext( "Paralyze as an action", "paralyze" )}
    } );

    if( !type->has_flag( mon_flag_NOHEAD ) ) {
        tmp.emplace_back( _( "It has a head." ) );
    }

    if( debug_mode ) {
        tmp.emplace_back( "--" );

        tmp.emplace_back( string_format( _( "Current Speed: %1$d" ), get_speed() ) );
        tmp.emplace_back( string_format( _( "Anger: %1$d" ), anger ) );
        tmp.emplace_back( string_format( _( "Friendly: %1$d" ), friendly ) );
        tmp.emplace_back( string_format( _( "Morale: %1$d" ), morale ) );
        if( aggro_character ) {
            tmp.emplace_back( string_format( _( "<color_red>Aggressive towards characters</color>" ) ) );
        }

        const time_duration current_time = calendar::turn - calendar::turn_zero;
        tmp.emplace_back( string_format( _( "Current Time: Turn %1$d  |  Day: %2$d" ),
                                         to_turns<int>( current_time ),
                                         to_days<int>( current_time ) ) );

        tmp.emplace_back( string_format( _( "Upgrade time: %1$d (turns left %2$d) %3$s" ),
                                         upgrade_time,
                                         to_turns<int>( time_duration::from_days( upgrade_time ) - current_time ),
                                         can_upgrade() ? "" : _( "<color_red>(can't upgrade)</color>" ) ) );

        if( !special_attacks.empty() ) {
            std::string sattack_str = string_format( _( "%d special attack(s): " ), special_attacks.size() );
            for( const auto &attack : special_attacks ) {
                sattack_str += string_format( _( "%s, cooldown %d; " ), attack.first.c_str(),
                                              attack.second.cooldown );
            }
            tmp.emplace_back( sattack_str );
        }

        if( baby_timer.has_value() ) {
            tmp.emplace_back( string_format( _( "Reproduce time: %1$d (turns left %2$d) %3$s" ),
                                             to_turn<int>( baby_timer.value() ),
                                             to_turn<int>( baby_timer.value() - current_time ),
                                             reproduces ? "" : _( "<color_red>(can't reproduce)</color>" ) ) );
        }

        if( biosig_timer.has_value() ) {
            tmp.emplace_back( string_format( _( "Biosignature time: %1$d (turns left %2$d) %3$s" ),
                                             to_turn<int>( biosig_timer.value() ),
                                             to_turn<int>( biosig_timer.value()  - current_time ),
                                             biosignatures ? "" : _( "<color_red>(no biosignature)</color>" ) ) );
        }

        if( lifespan_end.has_value() ) {
            tmp.emplace_back( string_format( _( "Lifespan end time: %1$d (turns left %2$d)" ),
                                             to_turn<int>( lifespan_end.value() ),
                                             to_turn<int>( lifespan_end.value() - current_time ) ) );
        } else {
            tmp.emplace_back( "Lifespan end time: n/a <color_yellow>(indefinite)</color>" );
        }
    }

    std::vector<std::string> ret;
    ret.reserve( tmp.size() );
    for( const std::string &s : tmp ) {
        ret.emplace_back( replace_colors( s ) );
    }

    return ret;
}

const std::string &monster::symbol() const
{
    return type->sym;
}

nc_color monster::basic_symbol_color() const
{
    return type->color;
}

nc_color monster::symbol_color() const
{
    return color_with_effects();
}

bool monster::is_symbol_highlighted() const
{
    return friendly != 0;
}

nc_color monster::color_with_effects() const
{
    nc_color ret = type->color;
    if( has_effect( effect_beartrap ) || has_effect( effect_stunned ) ||
        has_effect( effect_psi_stunned ) || has_effect( effect_downed ) ||
        has_effect( effect_tied ) ||
        has_effect( effect_lightsnare ) || has_effect( effect_heavysnare ) ) {
        ret = hilite( ret );
    }
    if( has_effect( effect_pacified ) ) {
        ret = invert_color( ret );
    }
    if( has_effect( effect_onfire ) ) {
        ret = red_background( ret );
    }
    return ret;
}

bool monster::avoid_trap( const tripoint_bub_ms & /* pos */, const trap &tr ) const
{
    // The trap position is not used, monsters are to stupid to remember traps. Actually, they do
    // not even see them.
    // Traps are on the ground, digging monsters go below, fliers and climbers go above.
    if( digging() || flies() ) {
        return true;
    }

    if( type->trap_avoids.count( tr.id ) > 0 ) {
        return true;
    }

    return dice( 3, type->sk_dodge + 1 ) >= dice( 3, tr.get_avoidance() );
}

bool monster::has_flag( const mon_flag_id &f ) const
{
    return type->has_flag( f );
}

bool monster::has_flag( const flag_id &f ) const
{
    mon_flag_str_id checked( f.c_str() );
    add_msg_debug( debugmode::DF_MONSTER,
                   "Monster %s checked for flag %s", name(),
                   f.c_str() );
    if( checked.is_valid() ) {
        return has_flag( checked );
    } else {
        return has_effect_with_flag( f );
    }
}

bool monster::can_see() const
{
    return has_flag( mon_flag_SEES ) && !effect_cache[VISION_IMPAIRED];
}

bool monster::can_hear() const
{
    return has_flag( mon_flag_HEARS ) && !has_effect( effect_deaf );
}

bool monster::can_submerge() const
{
    return ( has_flag( mon_flag_NO_BREATHE ) || swims() || has_flag( mon_flag_AQUATIC ) ) &&
           !has_flag( mon_flag_ELECTRONIC );
}

bool monster::can_drown() const
{
    return !swims() && !has_flag( mon_flag_AQUATIC ) &&
           !has_flag( mon_flag_NO_BREATHE ) && !flies();
}

bool monster::can_climb() const
{
    return climbs() || flies();
}

bool monster::digging() const
{
    return digs() || ( can_dig() && underwater );
}

bool monster::can_dig() const
{
    return has_flag( mon_flag_CAN_DIG );
}

bool monster::digs() const
{
    return has_flag( mon_flag_DIGS ) || type->move_skills.dig.has_value() ;
}

int monster::get_dig_mod() const
{
    if( type->move_skills.dig.has_value() ) {
        int percentile = type->move_skills.dig.value() * ( move_skills_data::max_movemod_penalty / 10 );
        return move_skills_data::max_movemod_penalty - percentile;
    } else if( has_flag( mon_flag_DIGS ) || has_flag( mon_flag_CAN_DIG ) ) {
        return 1;
    }

    // cannot dig
    return -1;
}

int monster::dig_skill() const
{
    return type->move_skills.dig.value_or( has_flag( mon_flag_DIGS ) ||
                                           has_flag( mon_flag_CAN_DIG ) ? 9 : -1 );
}

bool monster::flies() const
{
    return has_flag( mon_flag_FLIES ) && !has_effect_with_flag( json_flag_DISABLE_FLIGHT );
}

bool monster::climbs() const
{
    return has_flag( mon_flag_CLIMBS ) || type->move_skills.climb.has_value();
}

int monster::climb_skill() const
{
    return type->move_skills.climb.value_or( has_flag( mon_flag_CLIMBS ) ? 9 : -1 );
}


int monster::get_climb_mod() const
{
    if( type->move_skills.climb.has_value() ) {
        int percentile = type->move_skills.climb.value() * ( move_skills_data::max_movemod_penalty / 10 );
        return move_skills_data::max_movemod_penalty - percentile;
    } else if( has_flag( mon_flag_CLIMBS ) ) {
        // only for backwards compatibility. In future move away from flags
        return 1;
    }

    // cannot climb
    return -1;
}

bool monster::swims() const
{
    return has_flag( mon_flag_SWIMS ) || has_flag( mon_flag_AQUATIC ) ||
           type->move_skills.swim.has_value();
}

int monster::swim_skill() const
{
    if( type->move_skills.swim.has_value() ) {
        return type->move_skills.swim.value();
    } else if( has_flag( mon_flag_SWIMS ) ) {
        // Backwardscompatibility only
        return 10;
    } else if( has_flag( mon_flag_AQUATIC ) ) {
        // arbitrary value, if a specific speed is desired, use move_skills
        return 6;
    }
    return -1;
}

int monster::get_swim_mod() const
{
    if( type->move_skills.swim.has_value() ) {
        int percentile = type->move_skills.swim.value() * ( move_skills_data::max_movemod_penalty / 10 );
        return move_skills_data::max_movemod_penalty - percentile;

    } else if( has_flag( mon_flag_SWIMS ) ) {
        // only for backwards compatibility. In future move away from flags
        // vanilla fish have fastest possible swimspeed
        return 0;
    } else if( can_submerge() ) {
        // monsters that can submerge can walk underwater. Simulated with min swimskill
        return move_skills_data::max_movemod_penalty;
    }

    // cannot swim
    return -1;
}

bool monster::can_act() const
{
    return moves > 0 &&
           ( effects->empty() ||
             ( !has_effect( effect_stunned ) && !has_effect( effect_psi_stunned ) &&
               !has_effect( effect_downed ) && !has_effect( effect_webbed ) ) );
}

int monster::sight_range( const float light_level ) const
{
    // Non-aquatic monsters can't see much when submerged
    if( !can_see() || effect_cache[VISION_IMPAIRED] ||
        ( underwater && !swims() && !has_flag( mon_flag_AQUATIC ) && !digging() ) ) {
        return 1;
    }
    static const float default_daylight = default_daylight_level();
    if( light_level == 0 ) {
        return calculate_by_enchantment( type->vision_night, enchant_vals::mod::VISION_RANGE, true );
    } else if( light_level >= default_daylight ) {
        return calculate_by_enchantment( type->vision_day, enchant_vals::mod::VISION_RANGE, true );
    }
    int range = ( light_level * type->vision_day + ( default_daylight - light_level ) *
                  type->vision_night ) / default_daylight;

    range = calculate_by_enchantment( range, enchant_vals::mod::VISION_RANGE, true );

    return range;
}

bool monster::made_of( const material_id &m ) const
{
    return type->made_of( m );
}

bool monster::made_of_any( const std::set<material_id> &ms ) const
{
    return type->made_of_any( ms );
}

bool monster::shearable() const
{
    return type->shearing.valid();
}

bool monster::made_of( phase_id p ) const
{
    return type->phase == p;
}

bool monster::is_pet() const
{
    return friendly == -1 && has_effect( effect_pet );
}

bool monster::is_pet_follow() const
{
    return is_pet() && !has_flag( mon_flag_PET_WONT_FOLLOW );
}

bool monster::has_intelligence() const
{
    return has_flag( mon_flag_PATH_AVOID_FALL ) ||
           has_flag( mon_flag_PATH_AVOID_FIRE ) ||
           has_flag( mon_flag_PATH_AVOID_DANGER ) ||
           has_flag( mon_flag_PRIORITIZE_TARGETS ) ||
           get_pathfinding_settings().avoid_sharp ||
           get_pathfinding_settings().avoid_traps;
}

std::vector<material_id> monster::get_absorb_material() const
{
    return type->absorb_material;
}

std::vector<material_id> monster::get_no_absorb_material() const
{
    return type->no_absorb_material;
}

void monster::set_patrol_route( const std::vector<point_rel_ms> &patrol_pts_rel_ms )
{
    const tripoint_abs_ms base_abs_ms = project_to<coords::ms>( pos_abs_omt() );
    for( const point_rel_ms &patrol_pt : patrol_pts_rel_ms ) {
        patrol_route.push_back( base_abs_ms + patrol_pt );
    }
    next_patrol_point = 0;
}

void monster::shift( const point_rel_sm & )
{
    // TODO: migrate this to absolute coords and get rid of shift()
    path.clear();
}

bool monster::has_dest() const
{
    return goal.has_value();
}

tripoint_abs_ms monster::get_dest() const
{
    return goal ? *goal : pos_abs();
}

void monster::set_dest( const tripoint_abs_ms &p )
{
    if( !goal || rl_dist( p, *goal ) > 2 ) {
        reset_pathfinding_cd();
    }
    goal = p;
}

void monster::unset_dest()
{
    goal = std::nullopt;
    path.clear();
}

bool monster::is_wandering() const
{
    return !has_dest() && patrol_route.empty();
}

void monster::wander_to( const tripoint_abs_ms &p, int f )
{
    wander_pos = p;
    wandf = f;
}

Creature *monster::attack_target()
{
    const map &here = get_map();

    if( !has_dest() ) {
        return nullptr;
    }

    Creature *target = get_creature_tracker().creature_at( get_dest() );
    if( target == nullptr || target == this ||
        attitude_to( *target ) == Attitude::FRIENDLY || !sees( here,  *target ) ||
        target->is_hallucination() ) {
        return nullptr;
    }

    return target;
}

void monster::witness_thievery( item *it )
{
    add_msg( m_bad, _( "%1$s saw the %2$s being stolen!" ), get_name(), it->tname() );
    std::vector<npc *> friends;
    for( npc &guy : g->all_npcs() ) {
        if( guy.get_faction() && guy.get_faction()->mon_faction == faction.id() ) {
            friends.push_back( &guy );
        }
    }
    if( friends.empty() ) {
        aggro_character = true;
        anger = 100;
        return;
    }
    std::sort( friends.begin(), friends.end(), npc::theft_witness_compare );
    friends[0]->witness_thievery( it );
}

bool monster::is_fleeing( Character &u ) const
{
    if( effect_cache[FLEEING] ) {
        return true;
    }
    if( anger >= 100 || morale >= 100 ) {
        return false;
    }
    monster_attitude att = attitude( &u );
    return att == MATT_FLEE || ( att == MATT_FOLLOW && rl_dist( pos_bub(), u.pos_bub() ) <= 4 );
}

Creature::Attitude monster::attitude_to( const Creature &other ) const
{
    const monster *m = other.is_monster() ? static_cast< const monster *>( &other ) : nullptr;
    const Character *p = other.as_character();
    if( m != nullptr ) {
        if( m == this ) {
            return Attitude::FRIENDLY;
        }

        mf_attitude faction_att = faction.obj().attitude( m->faction );
        if( ( friendly != 0 && m->friendly != 0 ) ||
            ( friendly == 0 && m->friendly == 0 && faction_att == MFA_FRIENDLY ) ) {
            // Friendly (to player) monsters are friendly to each other
            // Unfriendly monsters go by faction attitude
            return Attitude::FRIENDLY;
            // NOLINTNEXTLINE(bugprone-branch-clone)
        } else if( friendly == 0 && m->friendly == 0 && faction_att == MFA_HATE ) {
            // Stuff that hates a specific faction will always attack that faction
            return Attitude::HOSTILE;
        } else if( ( friendly == 0 && m->friendly == 0 && faction_att == MFA_NEUTRAL ) ||
                   morale < 0 || anger < 10 ) {
            // Stuff that won't attack is neutral to everything
            return Attitude::NEUTRAL;
        } else {
            return Attitude::HOSTILE;
        }
    } else if( p != nullptr ) {
        switch( attitude( p ) ) {
            case MATT_FRIEND:
                return Attitude::FRIENDLY;
            case MATT_FPASSIVE:
            case MATT_FLEE:
            case MATT_IGNORE:
            case MATT_FOLLOW:
                return Attitude::NEUTRAL;
            case MATT_ATTACK:
                return Attitude::HOSTILE;
            case MATT_NULL:
            case NUM_MONSTER_ATTITUDES:
                break;
        }
    }
    // Should not happen!, creature should be either player or monster
    return Attitude::NEUTRAL;
}

monster_attitude monster::attitude( const Character *u ) const
{
    // override for the Personal Portal Storms Mod
    // if the monster is a nether portal monster and the character is an NPC then ignore
    if( u != nullptr && faction == monfaction_nether_player_hate && u->is_npc() &&
        get_option<bool>( "PORTAL_STORM_IGNORE_NPC" ) ) {
        // portal storm creatures ignore NPCs no matter what with this mod on
        return MATT_FPASSIVE;
    }

    if( friendly != 0 ) {
        if( has_effect( effect_docile ) ) {
            return MATT_FPASSIVE;
        }
        if( u != nullptr ) {
            if( u->is_avatar() ) {
                return MATT_FRIEND;
            }
            if( u->is_npc() ) {
                // Zombies don't understand not attacking NPCs, but dogs and bots should.
                if( u->get_attitude() != NPCATT_KILL && !type->in_species( species_ZOMBIE ) ) {
                    return MATT_FRIEND;
                }
                if( u->is_hallucination() ) {
                    return MATT_IGNORE;
                }
            }
        }
    }

    if( effect_cache[FLEEING] ) {
        return MATT_FLEE;
    }

    // CHECK FOR ANGER RELATIONS **HERE**
    bool target_has_anger_relation = false;
    if( u != nullptr ) {
        for( const trait_id &mut : u->get_functioning_mutations() ) {
            const mutation_branch &branch = *mut;
            for( const std::pair<const species_id, int> &elem : branch.anger_relations ) {
                if( type->in_species( elem.first ) ) {
                    target_has_anger_relation = true;
                    break;
                }
            }
            if( target_has_anger_relation ) {
                break;
            }
        }
    }
    // ONLY IGNORE IF NEITHER AGGRO NOR ANGER-RELATION
    if( u != nullptr && !aggro_character && !u->is_monster() && !target_has_anger_relation ) {
        return MATT_IGNORE;
    }

    // Now proceed to calculate anger/morale
    int effective_anger  = anger;
    int effective_morale = morale;

    if( u != nullptr ) {
        if( faction == monfaction_bee ) {
            if( u->has_trait( trait_BEE ) ) {
                return MATT_FRIEND;
            } else if( u->has_trait( trait_FLOWERS ) ) {
                effective_anger -= 10;
            }
        }
        auto *u_fac = u->get_faction();
        if( u_fac && u_fac->mon_faction && faction == u_fac->mon_faction ) {
            return MATT_FRIEND;
        }

        if( type->in_species( species_FUNGUS ) && ( u->has_trait( trait_THRESH_MYCUS ) ||
                u->has_trait( trait_MYCUS_FRIEND ) ) ) {
            return MATT_FRIEND;
        }

        if( effective_anger >= 10 &&
            type->in_species( species_MAMMAL ) && u->has_trait( trait_PHEROMONE_MAMMAL ) ) {
            effective_anger -= 20;
        }

        if( effective_anger >= 10 &&
            type->in_species( species_AMPHIBIAN ) && u->has_trait( trait_PHEROMONE_AMPHIBIAN ) ) {
            effective_anger -= 20;
        }

        if( ( faction == monfaction_acid_ant || faction == monfaction_ant || faction == monfaction_bee ||
              faction == monfaction_wasp ) && effective_anger >= 10 && u->has_trait( trait_PHEROMONE_INSECT ) ) {
            effective_anger -= 20;
        }

        if( u->has_trait( trait_TERRIFYING ) ) {
            effective_morale -= 10;
        }

        // Animal Discord/Empathy
        // Check for Discord first so we can apply temporary effects that make animals hate you
        if( has_flag( mon_flag_ANIMAL ) ) {
            if( u->has_flag( json_flag_ANIMALDISCORD ) ) {
                if( effective_anger >= 10 ) {
                    effective_anger += 10;
                }
                if( effective_anger < 10 ) {
                    effective_morale -= 5;
                }
            } else if( u->has_flag( json_flag_ANIMALDISCORD2 ) ) {
                if( effective_anger >= 20 ) {
                    effective_anger += 20;
                }
                if( effective_anger < 20 ) {
                    effective_morale -= 5;
                }
            } else if( u->has_flag( json_flag_ANIMALEMPATH ) ) {
                effective_anger -= 10;
                if( effective_anger < 10 ) {
                    effective_morale += 55;
                }
            } else if( u->has_flag( json_flag_ANIMALEMPATH2 ) ) {
                effective_anger -= 20;
                if( effective_anger < 20 ) {
                    effective_morale += 80;
                }
            }
        }

        for( const trait_id &mut : u->get_functioning_mutations() ) {
            const mutation_branch &branch = *mut;
            if( branch.ignored_by.empty() && branch.anger_relations.empty() ) {
                continue;
            }
            for( const species_id &spe : branch.ignored_by ) {
                if( type->in_species( spe ) ) {
                    return MATT_IGNORE;
                }
            }
            for( const std::pair<const species_id, int> &elem : branch.anger_relations ) {
                if( type->in_species( elem.first ) ) {
                    effective_anger += elem.second;
                }
            }
        }
    }

    if( effective_morale < 0 ) {
        if( effective_morale + effective_anger > 0 && get_hp() > get_hp_max() / 3 ) {
            return MATT_FOLLOW;
        }
        return MATT_FLEE;
    }

    if( effective_anger <= 0 ) {
        if( get_hp() <= 0.6 * get_hp_max() ) {
            return MATT_FLEE;
        } else {
            return MATT_IGNORE;
        }
    }

    if( effective_anger < 10 ) {
        return MATT_FOLLOW;
    }

    if( has_flag( mon_flag_KEEP_DISTANCE ) &&
        rl_dist( pos_abs(), get_dest() ) < type->tracking_distance ) {
        return MATT_FLEE;
    }

    return MATT_ATTACK;
}

int monster::hp_percentage() const
{
    return get_hp( bodypart_id( "torso" ) ) * 100 / get_hp_max();
}

int monster::get_eff_per() const
{
    return std::min( type->vision_night, type->vision_day );
}

void monster::process_triggers()
{
    process_trigger( mon_trigger::STALK, [this]() {
        return anger > 0 && one_in( 5 ) ? 1 : 0;
    } );

    process_trigger( mon_trigger::BRIGHT_LIGHT, [this]() {
        int ret = 0;
        static const int dim_light = round( .75 * default_daylight_level() );
        int light = round( get_map().ambient_light_at( pos_bub() ) );
        if( light >= dim_light ) {
            ret += 10;
        }
        return ret;
    } );

    process_trigger( mon_trigger::FIRE, [this]() {
        int ret = 0;
        map &here = get_map();
        const field_type_id fd_fire = ::fd_fire; // convert to int_id once
        for( const tripoint_bub_ms &p : here.points_in_radius( pos_bub(), 3 ) ) {
            // note using `has_field_at` without bound checks,
            // as points that come from `points_in_radius` are guaranteed to be in bounds
            const int fire_intensity =
                here.has_field_at( p, false ) ? 5 * here.get_field_intensity( p, fd_fire ) : 0;
            ret += fire_intensity;
        }
        return ret;
    } );

    if( morale != type->morale && one_in( 4 ) &&
        ( ( std::abs( morale - type->morale ) > 15 ) || one_in( 2 ) ) ) {
        if( morale < type->morale ) {
            morale++;
        } else {
            morale--;
        }
    }

    if( anger != type->agro && one_in( 4 ) &&
        ( ( std::abs( anger - type->agro ) > 15 ) || one_in( 2 ) ) ) {
        if( anger < type->agro ) {
            anger++;
        } else {
            anger--;
        }
    }

    // If we got angry at characters have a chance at calming down
    if( anger == type->agro && aggro_character && !type->aggro_character && !x_in_y( anger, 100 ) ) {
        add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro reset", name() );
        aggro_character = false;
    }

    // Cap values at [-100, 100] to prevent perma-angry moose etc.
    morale = std::min( 100, std::max( -100, morale ) );
    anger  = std::min( 100, std::max( -100, anger ) );
}

// This adjusts anger/morale levels given a single trigger.
void monster::process_trigger( mon_trigger trig, int amount )
{
    if( type->has_anger_trigger( trig ) ) {
        anger += amount;
    }
    if( type->has_fear_trigger( trig ) ) {
        morale -= amount;
    }
    if( type->has_placate_trigger( trig ) ) {
        anger -= amount;
    }
}

void monster::process_trigger( mon_trigger trig, const std::function<int()> &amount_func )
{
    if( type->has_anger_trigger( trig ) ) {
        anger += amount_func();
    }
    if( type->has_fear_trigger( trig ) ) {
        morale -= amount_func();
    }
    if( type->has_placate_trigger( trig ) ) {
        anger -= amount_func();
    }
}

bool monster::is_underwater() const
{
    return underwater && can_submerge();
}

bool monster::is_on_ground() const
{
    // TODO: actually make this work
    return false;
}

bool monster::has_weapon() const
{
    return has_flag( mon_flag_WIELDED_WEAPON ) && !has_effect( effect_disarmed ) &&
           !has_effect( effect_maimed_arm ); // monsters can actually have weapons, silly
}

bool monster::is_warm() const
{
    return has_flag( mon_flag_WARM );
}

bool monster::in_species( const species_id &spec ) const
{
    return type->in_species( spec );
}

bool monster::is_elec_immune() const
{
    return is_immune_damage( damage_electric );
}

bool monster::is_immune_effect( const efftype_id &effect ) const
{
    if( effect == effect_onfire ) {
        return is_immune_damage( damage_heat ) ||
               made_of( phase_id::LIQUID ) ||
               has_flag( mon_flag_FIREY );
    }

    if( effect == effect_bleed ) {
        return ( type->bloodType() == fd_null || type->bleed_rate == 0 );
    }

    if( effect == effect_venom_dmg ||
        effect == effect_venom_player1 ||
        effect == effect_venom_player2 ) {
        return ( !made_of( material_flesh ) && !made_of( material_iflesh ) ) ||
               type->in_species( species_NETHER ) || type->in_species( species_MIGO ) ||
               type->in_species( species_LEECH_PLANT );
    }

    if( effect == effect_paralyzepoison ||
        effect == effect_badpoison ||
        effect == effect_venom_weaken ||
        effect == effect_poison ) {
        return type->in_species( species_ZOMBIE ) || type->in_species( species_NETHER ) ||
               type->in_species( species_MIGO ) ||
               !made_of_any( Creature::cmat_flesh ) || type->in_species( species_LEECH_PLANT );
    }

    if( effect == effect_tpollen ) {
        return type->in_species( species_PLANT );
    }

    if( effect == effect_stunned ) {
        return has_flag( mon_flag_STUN_IMMUNE );
    }

    if( effect == effect_downed ) {
        if( type->bodytype == "insect" || type->bodytype == "flying insect" || type->bodytype == "spider" ||
            type->bodytype == "crab" ) {
            return x_in_y( 3, 4 );
        } else {
            return type->bodytype == "snake" || type->bodytype == "blob" || type->bodytype == "fish" ||
                   has_flag( mon_flag_FLIES ) || has_flag( mon_flag_IMMOBILE ) || has_flag( json_flag_CANNOT_MOVE );
        }
    }
    for( const flag_id &flag : effect->immune_flags ) {
        if( has_flag( flag ) ) {
            return true;
        }
    }
    return false;
}

bool monster::check_immunity_data( const field_immunity_data &ft ) const
{
    for( const flag_id &flag : ft.immunity_data_flags ) {
        if( has_flag( flag ) ) {
            return true;
        }
    }
    return false;
}

bool monster::is_immune_damage( const damage_type_id &dt ) const
{
    if( !dt->mon_immune_flags.empty() ) {
        for( const std::string &mf : dt->mon_immune_flags ) {
            if( has_flag( mon_flag_id( mf ) ) ) {
                return true;
            }
        }
    }
    // FIXME: Hardcoded damage type specific immunities
    if( dt == damage_heat && ( made_of( material_steel ) || made_of( material_stone ) ) ) {
        return true;
    }
    if( dt == damage_electric && type->sp_defense == &mdefense::zapback ) {
        return true;
    }

    return false;
}

void monster::make_bleed( const effect_source &source, time_duration duration, int intensity,
                          bool permanent, bool force, bool defferred )
{
    if( type->bleed_rate == 0 ) {
        return;
    }

    duration = ( duration * type->bleed_rate ) / 100;
    if( type->in_species( species_ROBOT ) ) {
        add_effect( source, effect_dripping_mechanical_fluid, duration, bodypart_str_id::NULL_ID() );
    } else {
        add_effect( source, effect_bleed, duration, bodypart_str_id::NULL_ID(), permanent, intensity, force,
                    defferred );
    }
}

bool monster::is_dead_state() const
{
    return hp <= 0;
}

bool monster::block_hit( Creature *, bodypart_id &, damage_instance & )
{
    return false;
}

const weakpoint *monster::absorb_hit( const weakpoint_attack &attack, const bodypart_id &,
                                      damage_instance &dam, const weakpoint &wp )
{
    resistances r = resistances( *this );
    const weakpoint *wkpt = attack.accuracy == -1.0 ?
                            type->weakpoints.select_weakpoint( attack ) :
                            &wp;
    wkpt->apply_to( r );
    for( damage_unit &elem : dam.damage_units ) {
        adjust_taken_damage_by_enchantments( elem );
        add_msg_debug( debugmode::DF_MONSTER,
                       "Dam Type: %s :: Dam Amt: %.1f :: Ar Pen: %.1f :: Armor Mult: %.1f",
                       elem.type.c_str(), elem.amount, elem.res_pen, elem.res_mult );
        add_msg_debug( debugmode::DF_MONSTER,
                       "Weakpoint: %s :: Armor Mult: %.1f :: Armor Penalty: %.1f :: Resist: %.1f",
                       wkpt->id, wkpt->armor_mult.count( elem.type ) > 0 ? wkpt->armor_mult.at( elem.type ) : 0.f,
                       wkpt->armor_penalty.count( elem.type ) > 0 ? wkpt->armor_penalty.at( elem.type ) : 0.f,
                       r.get_effective_resist( elem ) );
        elem.amount -= std::min( r.get_effective_resist( elem ) +
                                 get_worn_armor_val( elem.type ), elem.amount );
        adjust_taken_damage_by_enchantments_post_absorbed( elem );
    }

    wkpt->apply_to( dam, attack.is_crit );
    return wkpt;
}

bool monster::melee_attack( Creature &target )
{
    return melee_attack( target, get_hit() );
}

bool monster::melee_attack( Creature &target, float accuracy )
{
    map &here = get_map();
    // Note: currently this method must consume move even if attack hasn't actually happen
    // otherwise infinite loop will happen
    mod_moves( -type->attack_cost );
    if( /*This happens sometimes*/ this == &target || !is_adjacent( &target, true ) ) {
        return false;
    }
    if( !sees( here, target ) && !target.is_hallucination() ) {
        debugmsg( "Z-Level view violation: %s tried to attack %s.", disp_name(), target.disp_name() );
        return false;
    }

    const int monster_hit_roll = melee::melee_hit_range( accuracy );
    int hitspread = target.deal_melee_attack( this, monster_hit_roll );
    if( type->melee_dice == 0 ) {
        // We don't hit, so just return
        return true;
    }

    Character &player_character = get_player_character();
    if( target.is_avatar() ||
        ( target.is_npc() && player_character.attitude_to( target ) == Attitude::FRIENDLY ) ) {
        // Make us a valid target for a few turns
        add_effect( effect_hit_by_player, 3_turns );
    }

    if( has_flag( mon_flag_HIT_AND_RUN ) ) {
        add_effect( effect_run, 4_turns );
    }

    const bool u_see_me = player_character.sees( here, *this );
    const bool u_see_my_spot = player_character.sees( here, this->pos_bub( here ) );
    const bool u_see_target = player_character.sees( here, target );

    damage_instance damage = !is_hallucination() ? type->melee_damage : damage_instance();
    if( !is_hallucination() && type->melee_dice > 0 ) {
        damage.add_damage( damage_bash, dice( type->melee_dice, type->melee_sides ), type->melee_dice_ap );
    }

    modify_damage_dealt_with_enchantments( damage );

    dealt_damage_instance dealt_dam;

    if( hitspread >= 0 ) {
        target.deal_melee_hit( this, hitspread, false, damage, dealt_dam );
    }

    const int total_dealt = dealt_dam.total_damage();
    if( hitspread < 0 ) {
        bool monster_missed = monster_hit_roll < 0.0;
        // Miss
        if( u_see_my_spot && !target.in_sleep_state() ) {
            if( target.is_avatar() ) {
                if( monster_missed ) {
                    add_msg( _( "%s misses you." ), u_see_me ? disp_name( false, true ) : _( "Something" ) );
                } else {
                    add_msg( _( "You dodge %s." ), u_see_me ? disp_name() : _( "something" ) );
                }
            } else if( target.is_npc() ) {
                if( monster_missed ) {
                    add_msg( _( "%1$s misses %2$s!" ),
                             u_see_me ? disp_name( false, true ) : _( "Something" ), target.disp_name() );
                } else {
                    add_msg( _( "%1$s dodges %2$s attack." ),
                             target.disp_name(), u_see_me ? name() : _( "something" ) );
                }
            }
        } else if( target.is_avatar() ) {
            add_msg( _( "You dodge an attack from an unseen source." ) );
        }
    } else if( is_hallucination() || total_dealt > 0 ) {
        // Hallucinations always produce messages but never actually deal damage
        if( u_see_my_spot ) {
            if( target.is_avatar() ) {
                sfx::play_variant_sound( "melee_attack", "monster_melee_hit",
                                         sfx::get_heard_volume( target.pos_bub() ) );
                // don't make a pain yelp if our limb is a bionic
                if( !dealt_dam.bp_hit->has_flag( json_flag_BIONIC_LIMB ) ) {
                    sfx::do_player_death_hurt( dynamic_cast<Character &>( target ), false );
                }
                //~ 1$s is attacker name, 2$s is bodypart name in accusative.
                add_msg( m_bad, _( "%1$s hits your %2$s." ), u_see_me ? disp_name( false, true ) : _( "Something" ),
                         body_part_name_accusative( dealt_dam.bp_hit ) );
            } else if( target.is_npc() ) {
                if( has_effect( effect_ridden ) && has_flag( mon_flag_RIDEABLE_MECH ) &&
                    pos_bub() == player_character.pos_bub() ) {
                    //~ %1$s: name of your mount, %2$s: target NPC name, %3$d: damage value
                    add_msg( m_good, _( "Your %1$s hits %2$s for %3$d damage!" ), name(), target.disp_name(),
                             total_dealt );
                } else {
                    //~ %1$s: attacker name, %2$s: target NPC name, %3$s: bodypart name in accusative
                    add_msg( _( "%1$s hits %2$s %3$s." ), u_see_me ? disp_name( false, true ) : _( "Something" ),
                             target.disp_name( true ),
                             body_part_name_accusative( dealt_dam.bp_hit ) );
                }
            } else {
                if( has_effect( effect_ridden ) && has_flag( mon_flag_RIDEABLE_MECH ) &&
                    pos_bub() == player_character.pos_bub() ) {
                    //~ %1$s: name of your mount, %2$s: target creature name, %3$d: damage value
                    add_msg( m_good, _( "Your %1$s hits %2$s for %3$d damage!" ), get_name(), target.disp_name(),
                             total_dealt );
                }
                if( get_option<bool>( "LOG_MONSTER_ATTACK_MONSTER" ) ) {
                    if( !u_see_me && u_see_target ) {
                        add_msg( _( "Something hits the %1$s!" ), target.disp_name() );
                    } else if( !u_see_target ) {
                        add_msg( _( "The %1$s hits something!" ), name() );
                    } else {
                        //~ %1$s: attacker name, %2$s: target creature name
                        add_msg( _( "The %1$s hits %2$s!" ), name(), target.disp_name() );
                    }
                }
            }
        } else if( target.is_avatar() ) {
            //~ %s is bodypart name in accusative.
            add_msg( m_bad, _( "Something hits your %s." ),
                     body_part_name_accusative( dealt_dam.bp_hit ) );
        }
    } else {
        // No damage dealt
        if( u_see_my_spot ) {
            if( target.is_avatar() ) {
                //~ 1$s is attacker name, 2$s is bodypart name in accusative, 3$s is armor name
                add_msg( _( "%1$s hits your %2$s, but your %3$s protects you." ), u_see_me ? disp_name( false,
                         true ) : _( "Something" ),
                         body_part_name_accusative( dealt_dam.bp_hit ), target.skin_name() );
            } else if( target.is_npc() ) {
                //~ $1s is monster name, %2$s is that monster target name,
                //~ $3s is target bodypart name in accusative, $4s is the monster target name,
                //~ 5$s is target armor name.
                add_msg( _( "%1$s hits %2$s %3$s but is stopped by %4$s %5$s." ), u_see_me ? disp_name( false,
                         true ) : _( "Something" ),
                         target.disp_name( true ),
                         body_part_name_accusative( dealt_dam.bp_hit ),
                         target.disp_name( true ),
                         target.skin_name() );
            } else if( get_option<bool>( "LOG_MONSTER_ATTACK_MONSTER" ) ) {
                //~ $1s is monster name, %2$s is that monster target name,
                //~ $3s is target armor name.
                add_msg( _( "%1$s hits %2$s but is stopped by its %3$s." ),
                         u_see_me ? disp_name( false, true ) : _( "Something" ),
                         target.disp_name(),
                         target.skin_name() );
            }
        } else if( target.is_avatar() ) {
            //~ 1$s is bodypart name in accusative, 2$s is armor name.
            add_msg( _( "Something hits your %1$s, but your %2$s protects you." ),
                     body_part_name_accusative( dealt_dam.bp_hit ), target.skin_name() );
        }
    }

    target.check_dead_state( &here );

    if( is_hallucination() ) {
        if( one_in( 7 ) ) {
            die( &here, nullptr );
        }
        return true;
    }

    if( hitspread >= 0 && !is_hallucination() ) {
        enchantment_cache->cast_hit_you( *this, target );
    }

    if( total_dealt <= 0 ) {
        return true;
    }

    // Add any on damage effects
    for( const mon_effect_data &eff : type->atk_effs ) {
        if( x_in_y( eff.chance, 100 ) ) {
            const bodypart_id affected_bp = eff.affect_hit_bp ? dealt_dam.bp_hit :  eff.bp.id();
            target.add_effect( eff.id, time_duration::from_turns( rng( eff.duration.first,
                               eff.duration.second ) ), affected_bp, eff.permanent, rng( eff.intensity.first,
                                       eff.intensity.second ) );
            target.add_msg_if_player( m_mixed, eff.message, name() );
        }
    }

    const int stab_cut = dealt_dam.type_damage( damage_cut ) + dealt_dam.type_damage( damage_stab );

    if( stab_cut > 0 && has_flag( mon_flag_VENOM ) ) {
        target.add_msg_if_player( m_bad, _( "You're envenomed!" ) );
        target.add_effect( effect_poison, 3_minutes );
    }

    if( stab_cut > 0 && has_flag( mon_flag_BADVENOM ) ) {
        target.add_msg_if_player( m_bad,
                                  _( "You feel venom flood your body, wracking you with pain…" ) );
        target.add_effect( effect_badpoison, 4_minutes );
    }

    if( stab_cut > 0 && has_flag( mon_flag_PARALYZEVENOM ) ) {
        target.add_msg_if_player( m_bad, _( "You feel venom enter your body!" ) );
        target.add_effect( effect_paralyzepoison, 10_minutes );
    }
    return true;
}

void monster::deal_projectile_attack( map *here, Creature *source, dealt_projectile_attack &attack,
                                      const double &missed_by, bool print_messages,
                                      const weakpoint_attack &wp_attack )
{
    const projectile &proj = attack.proj;
    const auto &effects = proj.proj_effects;

    // Whip has a chance to scare wildlife even if it misses
    if( effects.count( ammo_effect_WHIP ) && type->in_category( "WILDLIFE" ) && one_in( 3 ) ) {
        add_effect( effect_run, rng( 3_turns, 5_turns ) );
    }

    if( missed_by > 1.0 ) {
        // Total miss
        return;
    }

    Creature::deal_projectile_attack( here, source, attack, missed_by, print_messages, wp_attack );

    if( !is_hallucination() && attack.last_hit_critter == this ) {
        // Maybe TODO: Get difficulty from projectile speed/size/missed_by
        on_hit( here, source, bodypart_id( "torso" ), INT_MIN, &attack );
    }
}

void monster::deal_damage_handle_type( const effect_source &source, const damage_unit &du,
                                       bodypart_id bp, int &damage, int &pain )
{
    const int adjusted_damage = du.amount * du.damage_multiplier * du.unconditional_damage_mult;
    if( is_immune_damage( du.type ) ) {
        return; // immunity
    }
    // FIXME: Hardcoded damage type effects (bash, bullet)
    if( du.type == damage_bash ) {
        if( has_flag( mon_flag_PLASTIC ) ) {
            damage += du.amount / rng( 2, 4 ); // lessened effect
            pain += du.amount / 4;
            return;
        }
    }
    if( du.type == damage_bullet || du.type->edged ) {
        make_bleed( source, 1_minutes * rng( 0, adjusted_damage ) );
    }

    Creature::deal_damage_handle_type( source, du,  bp, damage, pain );
}

int monster::heal( const int delta_hp, bool overheal )
{
    const int maxhp = type->hp;
    if( delta_hp <= 0 || ( hp >= maxhp && !overheal ) ) {
        return 0;
    }

    const int old_hp = hp;
    hp += delta_hp;
    if( hp > maxhp && !overheal ) {
        hp = maxhp;
    }
    return hp - old_hp;
}

void monster::set_hp( const int hp )
{
    this->hp = hp;
}

void monster::apply_damage( Creature *source, bodypart_id /*bp*/, int dam,
                            const bool /*bypass_med*/ )
{
    if( is_dead_state() || has_flag( json_flag_CANNOT_TAKE_DAMAGE ) ) {
        return;
    }
    // Ensure we can try to get at what hit us.
    reset_pathfinding_cd();
    hp -= dam;

    cata::event e = cata::event::make<event_type::monster_takes_damage>( dam, hp < 1 );
    if( source ) {
        get_event_bus().send_with_talker( this, source, e );
    } else {
        get_event_bus().send( e );
    }

    if( hp < 1 ) {
        set_killer( source );
    } else if( dam > 0 ) {
        process_trigger( mon_trigger::HURT, 1 + static_cast<int>( dam / 3 ) );
        // Get angry at characters if hurt by one
        if( source != nullptr && !aggro_character && !source->is_monster() && !source->is_fake() ) {
            aggro_character = true;
        }
    }
}

void monster::die_in_explosion( Creature *source )
{
    map &here = get_map();

    hp = -9999; // huge to trigger explosion and prevent corpse item
    die( &here, source );
}

void monster::heal_bp( bodypart_id, int dam )
{
    heal( dam );
}

bool monster::movement_impaired()
{
    return effect_cache[MOVEMENT_IMPAIRED];
}

bool monster::move_effects( bool )
{
    // This function is relatively expensive, we want that cached
    // IMPORTANT: If adding any new effects here, make SURE to
    // add them to hardcoded_movement_impairing in effect.cpp
    if( !effect_cache[MOVEMENT_IMPAIRED] ) {
        return true;
    }

    // If the monster has the CANNOT_MOVE flag, they can't
    // perform any move effects, so skip this step
    if( has_effect_with_flag( json_flag_CANNOT_MOVE ) ) {
        return false;
    }

    map &here = get_map();
    bool u_see_me = get_player_view().sees( here, *this );
    if( has_effect( effect_tied ) ) {
        // friendly pet, will stay tied down and obey.
        if( friendly == -1 ) {
            return false;
        }
        // non-friendly monster will struggle to get free occasionally.
        // some monsters can't be tangled up with a net/bolas/lasso etc.
        bool immediate_break = type->in_species( species_FISH ) || type->in_species( species_MOLLUSK ) ||
                               type->in_species( species_ROBOT ) || type->bodytype == "snake" || type->bodytype == "blob";
        if( !immediate_break && rng( 0, 900 ) > type->melee_dice * type->melee_sides * 1.5 ) {
            if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                add_msg( _( "The %s struggles to break free of its bonds." ), name() );
            }
        } else if( immediate_break ) {
            remove_effect( effect_tied );
            if( tied_item ) {
                if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                    add_msg( _( "The %s easily slips out of its bonds." ), name() );
                }
                here.add_item_or_charges( pos_bub(), *tied_item );
                tied_item.reset();
            }
        } else {
            if( tied_item ) {
                const bool broken = rng( type->melee_dice * type->melee_sides, std::min( 10000,
                                         type->melee_dice * type->melee_sides * 250 ) ) > 800;
                if( !broken ) {
                    here.add_item_or_charges( pos_bub(), *tied_item );
                }
                tied_item.reset();
                if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                    if( broken ) {
                        add_msg( _( "The %s snaps the bindings holding it down." ), name() );
                    } else {
                        add_msg( _( "The %s breaks free of the bindings holding it down." ), name() );
                    }
                }
            }
            remove_effect( effect_tied );
        }
        return false;
    }
    if( has_effect( effect_downed ) ) {
        if( rng( 0, 40 ) > type->melee_dice * type->melee_sides * 1.5 ) {
            if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                add_msg( _( "The %s struggles to stand." ), name() );
            }
        } else {
            if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                add_msg( _( "The %s climbs to its feet!" ), name() );
            }
            remove_effect( effect_downed );
        }
        return false;
    }
    if( has_effect( effect_webbed ) ) {
        if( x_in_y( type->melee_dice * type->melee_sides, 6 * get_effect_int( effect_webbed ) ) ) {
            if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                add_msg( _( "The %s breaks free of the webs!" ), name() );
            }
            remove_effect( effect_webbed );
        }
        return false;
    }
    if( has_effect( effect_lightsnare ) ) {
        if( x_in_y( type->melee_dice * type->melee_sides, 12 ) ) {
            remove_effect( effect_lightsnare );
            if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                add_msg( _( "The %s escapes the light snare!" ), name() );
            }
        }
        return false;
    }
    if( has_effect( effect_heavysnare ) ) {
        if( type->melee_dice * type->melee_sides >= 7 ) {
            if( x_in_y( type->melee_dice * type->melee_sides, 32 ) ) {
                remove_effect( effect_heavysnare );
                here.spawn_item( pos_bub(), itype_rope_6 );
                here.spawn_item( pos_bub(), itype_snare_trigger );
                if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                    add_msg( _( "The %s escapes the heavy snare!" ), name() );
                }
            }
        }
        return false;
    }
    if( has_effect( effect_beartrap ) ) {
        if( type->melee_dice * type->melee_sides >= 18 ) {
            if( x_in_y( type->melee_dice * type->melee_sides, 200 ) ) {
                remove_effect( effect_beartrap );
                here.spawn_item( pos_bub(), itype_beartrap );
                if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                    add_msg( _( "The %s escapes the bear trap!" ), name() );
                }
            }
        }
        return false;
    }
    if( has_effect( effect_crushed ) ) {
        if( x_in_y( type->melee_dice * type->melee_sides, 100 ) ) {
            remove_effect( effect_crushed );
            if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                add_msg( _( "The %s frees itself from the rubble!" ), name() );
            }
        }
        return false;
    }
    if( has_effect( effect_worked_on ) ) {
        return false;
    }

    // If we ever get more effects that force movement on success this will need to be reworked to
    // only trigger success effects if /all/ rolls succeed
    if( has_effect( effect_in_pit ) ) {
        if( rng( 0, 40 ) > type->melee_dice * type->melee_sides ) {
            return false;
        } else {
            if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                add_msg( _( "The %s escapes the pit!" ), name() );
            }
            remove_effect( effect_in_pit );
        }
    }
    if( has_effect_with_flag( json_flag_GRAB ) ) {
        // Pretty hacky, but monsters have no stats
        map &here = get_map();
        creature_tracker &creatures = get_creature_tracker();
        const tripoint_range<tripoint_bub_ms> &surrounding = here.points_in_radius( pos_bub(), 1, 0 );
        for( const effect &grab : get_effects_with_flag( json_flag_GRAB ) ) {
            // Is our grabber around?
            monster *grabber = nullptr;
            for( const tripoint_bub_ms loc : surrounding ) {
                // In an edge case when two monsters grab each other
                // Don't consider grabbed monster to be its own grabber
                if( loc == pos_bub() ) {
                    continue;
                }
                monster *mon = creatures.creature_at<monster>( loc );
                if( mon && mon->has_effect_with_flag( json_flag_GRAB_FILTER ) ) {
                    add_msg_debug( debugmode::DF_MATTACK, "Grabber %s found", mon->name() );
                    grabber = mon;
                    break;
                }
            }

            if( grabber == nullptr ) {
                remove_effect( grab.get_id() );
                add_msg_debug( debugmode::DF_MATTACK, "Orphan grab found and removed" );
                if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                    add_msg( _( "The %s is no longer grabbed!" ), name() );
                }
                continue;
            }
            int monster = type->melee_skill + type->melee_damage.total_damage();
            int grab_str = get_effect_int( grab.get_id() );
            add_msg_debug( debugmode::DF_MONSTER, "%s attempting to break grab %s, success %d in intensity %d",
                           get_name(), grab.get_id().c_str(), monster, grabber );
            if( !x_in_y( monster, grab_str ) ) {
                return false;
            } else {
                if( u_see_me && get_option<bool>( "LOG_MONSTER_MOVE_EFFECTS" ) ) {
                    add_msg( _( "The %s breaks free from the %s's grab!" ), name(), grabber->name() );
                }
                remove_effect( grab.get_id() );
            }
        }
    }
    return true;
}

std::string monster::get_effect_status() const
{
    std::vector<std::string> effect_status;
    for( auto &elem : *effects ) {
        for( auto &_it : elem.second ) {
            if( elem.first->is_show_in_info() ) {
                effect_status.push_back( _it.second.disp_name() );
            }
        }
    }

    return enumerate_as_string( effect_status );
}

int monster::get_worn_armor_val( const damage_type_id &dt ) const
{
    if( !has_effect( effect_monster_armor ) ) {
        return 0;
    }
    if( armor_item ) {
        return armor_item->resist( dt );
    }
    return 0;
}

int monster::get_armor_type( const damage_type_id &dt, bodypart_id /*bp*/ ) const
{
    return get_worn_armor_val( dt ) + type->armor.type_resist( dt ) + get_armor_res_bonus( dt );
}

float monster::get_hit_base() const
{
    return type->melee_skill;
}

float monster::get_dodge_base() const
{
    return type->sk_dodge;
}

float monster::hit_roll() const
{
    float hit = get_hit();
    if( has_effect( effect_bouldering ) ) {
        hit /= 4;
    }

    return melee::melee_hit_range( hit );
}

bool monster::has_grab_break_tec() const
{
    return false;
}

float monster::stability_roll() const
{
    int size_bonus = 0;
    switch( type->size ) {
        case creature_size::tiny:
            size_bonus -= 7;
            break;
        case creature_size::small:
            size_bonus -= 3;
            break;
        case creature_size::large:
            size_bonus += 5;
            break;
        case creature_size::huge:
            size_bonus += 10;
            break;
        case creature_size::medium:
            break; // keep default
        case creature_size::num_sizes:
            debugmsg( "ERROR: Invalid Creature size class." );
            break;
    }

    int stability = dice( type->melee_sides, type->melee_dice ) + size_bonus;
    if( has_effect( effect_stunned ) ) {
        stability -= rng( 1, 5 );
    }
    return stability;
}

float monster::get_dodge() const
{
    if( has_effect( effect_downed ) || has_flag( json_flag_CANNOT_MOVE ) ) {
        return 0.0f;
    }

    float ret = Creature::get_dodge();
    if( has_effect( effect_lightsnare ) || has_effect( effect_heavysnare ) ||
        has_effect( effect_beartrap ) || has_effect( effect_tied ) ) {
        ret /= 2;
    }

    if( has_effect( effect_bouldering ) ) {
        ret /= 4;
    }

    return ret;
}

float monster::get_melee() const
{
    return type->melee_skill;
}

float monster::dodge_roll() const
{
    return get_dodge() * 5;
}

bool monster::can_attack_high() const
{
    return  !( ( type->size < creature_size::medium && !has_flag( mon_flag_FLIES ) &&
                 !has_flag( mon_flag_ATTACK_UPPER ) ) || has_flag( mon_flag_ATTACK_LOWER ) )  ;
}

int monster::get_grab_strength() const
{
    return type->grab_strength;
}

void monster::add_grab( bodypart_str_id bp )
{
    add_effect( effect_grabbing, 1_days, true, 1 );
    grabbed_limbs.insert( bp );
}

void monster::remove_grab( bodypart_str_id bp )
{
    grabbed_limbs.erase( bp );
    if( grabbed_limbs.empty() ) {
        add_effect( effect_grabbing, 1_days, true, 1 );
    }
}

bool monster::is_grabbing( bodypart_str_id bp )
{
    return has_effect( effect_grabbing ) && grabbed_limbs.find( bp ) != grabbed_limbs.end();
}

float monster::fall_damage_mod() const
{
    if( flies() ) {
        return 0.0f;
    }

    switch( type->size ) {
        case creature_size::tiny:
            return 0.2f;
        case creature_size::small:
            return 0.6f;
        case creature_size::medium:
            return 1.0f;
        case creature_size::large:
            return 1.4f;
        case creature_size::huge:
            return 2.0f;
        case creature_size::num_sizes:
            debugmsg( "ERROR: Invalid Creature size class." );
            return 0.0f;
    }

    return 0.0f;
}

int monster::impact( const int force, const tripoint_bub_ms &p )
{
    if( force <= 0 ) {
        return force;
    }

    const float mod = fall_damage_mod();
    int total_dealt = 0;
    if( get_map().has_flag( ter_furn_flag::TFLAG_SHARP, p ) ) {
        const int cut_damage = std::max( 0.0f, 10 * mod - get_armor_type( damage_cut,
                                         bodypart_id( "torso" ) ) );
        apply_damage( nullptr, bodypart_id( "torso" ), cut_damage );
        total_dealt += 10 * mod;
    }

    const int bash_damage = std::max( 0.0f, force * mod - get_armor_type( damage_bash,
                                      bodypart_id( "torso" ) ) );
    apply_damage( nullptr, bodypart_id( "torso" ), bash_damage );
    total_dealt += force * mod;

    add_effect( effect_downed, time_duration::from_turns( rng( 0, mod * 3 + 1 ) ) );

    return total_dealt;
}

void monster::reset_bonuses()
{
    effect_cache.reset();

    Creature::reset_bonuses();
}

void monster::reset_stats()
{
    // Nothing here yet
}

void monster::reset_special( const std::string &special_name )
{
    dialogue d( get_talker_for( this ), get_talker_for( get_avatar() ) );
    set_special( special_name, type->special_attacks.at( special_name )->cooldown.evaluate( d ) );
}

void monster::reset_special_rng( const std::string &special_name )
{
    dialogue d( get_talker_for( this ), get_talker_for( get_avatar() ) );
    set_special( special_name, rng( 0,
                                    type->special_attacks.at( special_name )->cooldown.evaluate( d ) ) );
}

void monster::set_special( const std::string &special_name, int time )
{
    special_attacks[ special_name ].cooldown = time;
}

void monster::disable_special( const std::string &special_name )
{
    special_attacks.at( special_name ).enabled = false;
}

bool monster::special_available( std::string_view special_name ) const
{
    std::map<std::string, mon_special_attack>::const_iterator iter = special_attacks.find(
                special_name );
    return iter != special_attacks.end() && iter->second.enabled && iter->second.cooldown == 0;
}

bool monster::has_special( const std::string &special_name ) const
{
    std::map<std::string, mon_special_attack>::const_iterator iter = special_attacks.find(
                special_name );
    return iter != special_attacks.end() && iter->second.enabled;
}

void monster::explode()
{
    // Handled in mondeath::normal
    // +1 to avoid overflow when evaluating -hp
    hp = INT_MIN + 1;
}

void monster::process_turn()
{
    map &here = get_map();
    if( !is_hallucination() ) {
        for( const std::pair<const emit_id, time_duration> &e : type->emit_fields ) {
            if( !calendar::once_every( e.second ) ) {
                continue;
            }
            const emit_id emid = e.first;
            if( emid == emit_emit_shock_cloud ) {
                if( has_effect( effect_emp ) ) {
                    continue; // don't emit electricity while EMPed
                } else if( has_effect( effect_supercharged ) ) {
                    here.emit_field( pos_bub(), emit_emit_shock_cloud_big );
                    continue;
                }
            }
            here.emit_field( pos_bub(), emid );
        }
    }

    // Special attack cooldowns are updated here.
    // Loop through the monster's special attacks, same as monster::move.
    for( const auto &sp_type : type->special_attacks ) {
        const std::string &special_name = sp_type.first;
        const auto local_iter = special_attacks.find( special_name );
        if( local_iter == special_attacks.end() ) {
            continue;
        }
        mon_special_attack &local_attack_data = local_iter->second;
        if( !local_attack_data.enabled ) {
            continue;
        }

        if( local_attack_data.cooldown > 0 ) {
            local_attack_data.cooldown--;
        }
    }
    creature_tracker &creatures = get_creature_tracker();
    // Persist grabs as long as there's an adjacent target.
    if( has_effect_with_flag( json_flag_GRAB_FILTER ) ) {
        bool remove = true;
        for( const tripoint_bub_ms &dest : here.points_in_radius( pos_bub(), 1, 0 ) ) {
            const Creature *const you = creatures.creature_at<Creature>( dest );
            if( you && you->has_effect_with_flag( json_flag_GRAB ) ) {
                add_msg_debug( debugmode::DF_MATTACK, "Grabbed creature %s found, persisting grab.",
                               you->get_name() );
                remove = false;
            }
        }
        if( remove ) {
            for( const effect &eff : get_effects_with_flag( json_flag_GRAB_FILTER ) ) {
                const efftype_id effid = eff.get_id();
                remove_effect( effid );
                add_msg_debug( debugmode::DF_MATTACK, "Grab filter effect %s removed.", effid.c_str() );
            }
        }
    }
    // We update electrical fields here since they act every turn.
    if( has_flag( mon_flag_ELECTRIC_FIELD ) && !is_hallucination() ) {
        if( has_effect( effect_emp ) ) {
            if( calendar::once_every( 10_turns ) ) {
                sounds::sound( pos_bub(), 5, sounds::sound_t::combat, _( "hummmmm." ), false, "humming",
                               "electric" );
            }
        } else {
            weather_manager &weather = get_weather();
            for( const tripoint_bub_ms &zap : here.points_in_radius( pos_bub(), 1 ) ) {
                const map_stack items = here.i_at( zap );
                for( const item &item : items ) {
                    if( item.made_of( phase_id::LIQUID ) && item.flammable() ) { // start a fire!
                        here.add_field( zap, fd_fire, 2, 1_minutes );
                        sounds::sound( pos_bub(), 30, sounds::sound_t::combat,  _( "fwoosh!" ), false, "fire", "ignition" );
                        break;
                    }
                }
                if( zap != pos_bub() ) {
                    explosion_handler::emp_blast( zap ); // Fries electronics due to the intensity of the field
                }
                const ter_id &t = here.ter( zap );
                if( t == ter_t_gas_pump || t == ter_t_gas_pump_a ) {
                    if( one_in( 4 ) ) {
                        explosion_handler::explosion( this, pos_bub(), 40, 0.8, true );
                        add_msg_if_player_sees( zap, m_warning, _( "The %s explodes in a fiery inferno!" ),
                                                here.tername( zap ) );
                    } else {
                        add_msg_if_player_sees( zap, m_warning, _( "Lightning from %1$s engulfs the %2$s!" ),
                                                name(), here.tername( zap ) );
                        here.add_field( zap, fd_fire, 1, 2_turns );
                    }
                }
            }
            if( weather.lightning_active && !has_effect( effect_supercharged ) &&
                here.is_outside( pos_bub() ) ) {
                weather.lightning_active = false; // only one supercharge per strike
                sounds::sound( pos_bub(), 300, sounds::sound_t::combat, _( "BOOOOOOOM!!!" ), false, "environment",
                               "thunder_near" );
                sounds::sound( pos_bub(), 20, sounds::sound_t::combat, _( "vrrrRRRUUMMMMMMMM!" ), false,
                               "explosion",
                               "default" );
                Character &player_character = get_player_character();
                if( player_character.sees( here, pos_bub( here ) ) ) {
                    add_msg( m_bad, _( "Lightning strikes the %s!" ), name() );
                    add_msg( m_bad, _( "Your vision goes white!" ) );
                    player_character.add_effect( effect_blind, rng( 1_minutes, 2_minutes ) );
                }
                add_effect( effect_supercharged, 12_hours );
            } else if( has_effect( effect_supercharged ) && calendar::once_every( 5_turns ) ) {
                sounds::sound( pos_bub(), 20, sounds::sound_t::combat, _( "VMMMMMMMMM!" ), false, "humming",
                               "electric" );
            }
        }
    }

    Creature::process_turn();
}

void monster::die( map *here, Creature *nkiller )
{
    if( dead ) {
        // We are already dead, don't die again, note that monster::dead is
        // *only* set to true in this function!
        return;
    }
    // We were carrying a creature, deposit the rider
    if( has_effect( effect_ridden ) && mounted_player ) {
        mounted_player->forced_dismount();
    }
    g->set_critter_died();
    dead = true;
    set_killer( nkiller );
    if( get_killer() != nullptr ) {
        Character *ch = get_killer()->as_character();
        if( ch == nullptr && get_killer()->get_summoner() != nullptr ) {
            ch = get_killer()->get_summoner()->as_character();
        }
        if( !is_hallucination() && ch != nullptr ) {
            cata::event e = cata::event::make<event_type::character_kills_monster>( ch->getID(), type->id,
                            compute_kill_xp( type->id ) );
            get_event_bus().send_with_talker( ch, this, e );
        }
    }
    creature_tracker &creatures = get_creature_tracker();
    if( has_effect_with_flag( json_flag_GRAB_FILTER ) ) {
        // Need to filter out which limb we were grabbing before death
        for( const tripoint_bub_ms &player_pos : here->points_in_radius( pos_bub( *here ), 1,
                0 ) ) {
            Creature *you = creatures.creature_at( here->get_abs( player_pos ) );
            if( !you || !you->has_effect_with_flag( json_flag_GRAB ) ) {
                continue;
            }
            // ...but if there are no grabbers around we can just skip to the end
            bool grabbed = false;
            for( const tripoint_bub_ms &mon_pos : here->points_in_radius( player_pos, 1, 0 ) ) {
                const monster *const mon = creatures.creature_at<monster>( here->get_abs( mon_pos ) );
                // No persisting our grabs from beyond the grave, but we also don't get to remove the effect early
                if( mon && mon->has_effect_with_flag( json_flag_GRAB_FILTER ) && mon != this ) {
                    grabbed = true;
                    break;
                }
            }
            if( !grabbed ) {
                you->add_msg_player_or_npc( m_good, _( "The last enemy holding you collapses!" ),
                                            _( "The last enemy holding <npcname> collapses!" ) );
                // A loop for safety
                for( const effect &grab : you->get_effects_with_flag( json_flag_GRAB ) ) {
                    you->remove_effect( grab.get_id() );
                }
                continue;
            }
            // Iterate through all your grabs to figure out which one this critter held
            for( const effect &grab : you->get_effects_with_flag( json_flag_GRAB ) ) {
                if( is_grabbing( grab.get_bp().id() ) ) {
                    const effect_type effid = *grab.get_effect_type();
                    you->remove_effect( effid.id, grab.get_bp() );
                }
            }
        }
    }

    // If we're a queen, make nearby groups of our type start to die out
    if( !is_hallucination() && has_flag( mon_flag_QUEEN ) ) {
        // The submap coordinates of this monster, monster groups coordinates are
        // submap coordinates.
        const tripoint_abs_sm abssub = coords::project_to<coords::sm>( pos_abs() );
        // Do it for overmap above/below too
        for( const tripoint_abs_sm &p : points_in_radius( abssub, HALF_MAPSIZE, 1 ) ) {
            for( mongroup *&mgp : overmap_buffer.groups_at( p ) ) {
                if( MonsterGroupManager::IsMonsterInGroup( mgp->type, type->id ) ) {
                    mgp->dying = true;
                }
            }
        }
    }
    mission::on_creature_death( *this );

    // Hallucinations always just disappear
    if( is_hallucination() ) {
        mdeath::disappear( *this );
        return;
    }

    add_msg_if_player_sees( *this, m_good, type->mdeath_effect.death_message.translated(), name() );

    if( type->mdeath_effect.has_effect ) {
        //Not a hallucination, go process the death effects.
        spell death_spell = type->mdeath_effect.sp.get_spell( *this );
        if( killer != nullptr && !type->mdeath_effect.sp.self &&
            // TODO: Get the death_spell stuff to work on any map.
            death_spell.is_target_in_range( *this,
                                            killer->pos_bub() ) ) { // The operation called uses reality bubble internally
            death_spell.cast_all_effects( *this,
                                          killer->pos_bub() );      // ditto, so we should feed them pos_bub().
        } else if( type->mdeath_effect.sp.self ) {
            death_spell.cast_all_effects( *this, pos_bub() );              // ditto.
        }
    }

    if( type->mdeath_effect.eoc.has_value() ) {
        //Not a hallucination, go process the death effects.
        if( type->mdeath_effect.eoc.value().is_valid() ) {
            dialogue d( killer == nullptr ? nullptr : get_talker_for( killer ), get_talker_for( *this ) );
            type->mdeath_effect.eoc.value()->activate( d );
        } else {
            debugmsg( "eoc id %s is not valid", type->mdeath_effect.eoc.value().str() );
        }
    }

    // scale overkill damage by enchantments
    if( nkiller && ( nkiller->is_npc() || nkiller->is_avatar() ) ) {
        int current_hp = get_hp();
        current_hp = nkiller->as_character()->enchantment_cache->modify_value(
                         enchant_vals::mod::OVERKILL_DAMAGE, current_hp );
        set_hp( current_hp );
    }

    item_location corpse;
    // drop a corpse, or not - this needs to happen after the spell, for e.g. revivification effects
    switch( type->mdeath_effect.corpse_type ) {
        case mdeath_type::NORMAL:
            corpse =  mdeath::normal( here, *this );
            break;
        case mdeath_type::BROKEN:
            mdeath::broken( here, *this );
            break;
        case mdeath_type::SPLATTER:
            corpse = mdeath::splatter( here, *this );
            break;
        default:
            break;
    }

    if( death_drops ) {
        // Drop items stored in optionals
        move_special_item_to_inv( tack_item );
        move_special_item_to_inv( armor_item );
        move_special_item_to_inv( storage_item );
        move_special_item_to_inv( tied_item );

        if( has_effect( effect_heavysnare ) ) {
            add_item( item( itype_rope_6, calendar::turn_zero ) );
            add_item( item( itype_snare_trigger, calendar::turn_zero ) );
        }
        if( has_effect( effect_beartrap ) ) {
            add_item( item( itype_beartrap, calendar::turn_zero ) );
        }
    }

    if( death_drops && !no_extra_death_drops ) {
        drop_items_on_death( here, corpse.get_item() );
        spawn_dissectables_on_death( corpse.get_item() );
    }
    if( death_drops && !is_hallucination() ) {
        for( const item &it : inv ) {
            if( it.has_var( "DESTROY_ITEM_ON_MON_DEATH" ) ) {
                continue;
            }
            if( corpse ) {
                corpse->force_insert_item( it, pocket_type::CONTAINER );
            } else {
                here->add_item_or_charges( pos_bub( *here ), it );
            }
        }
        for( const item &it : dissectable_inv ) {
            if( corpse ) {
                corpse->put_in( it, pocket_type::CORPSE );
            } else {
                here->add_item( pos_bub( *here ), it );
            }
        }
    }
    if( corpse ) {
        corpse->process( *here, nullptr, corpse.pos_bub( *here ) );
        if( reality_bubble().inbounds( pos_abs() ) ) {
            corpse.make_active();
        }
    }

    // Adjust anger/morale of nearby monsters, if they have the appropriate trigger and are friendly
    // Keep filtering on the dying monsters' triggers to preserve current functionality
    bool trigger = type->has_anger_trigger( mon_trigger::FRIEND_DIED ) ||
                   type->has_fear_trigger( mon_trigger::FRIEND_DIED ) ||
                   type->has_placate_trigger( mon_trigger::FRIEND_DIED );

    if( trigger ) {
        int light = g->light_level( posz() );
        map &here = get_map();
        for( monster &critter : g->all_monsters() ) {
            // Do we actually care about this faction?
            if( critter.faction->attitude( faction ) != MFA_FRIENDLY ) {
                continue;
            }

            if( here.sees( critter.pos_bub(), pos_bub(), light ) ) {
                // Anger trumps fear trumps ennui
                if( critter.type->has_anger_trigger( mon_trigger::FRIEND_DIED ) ) {
                    critter.anger += 15;
                    if( nkiller != nullptr && !nkiller->is_monster() && !nkiller->is_fake() ) {
                        // A character killed our friend
                        add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by killing a friendly %s",
                                       critter.name(), name() );
                        critter.aggro_character = true;
                    }
                } else if( critter.type->has_fear_trigger( mon_trigger::FRIEND_DIED ) ) {
                    critter.morale -= 15;
                } else if( critter.type->has_placate_trigger( mon_trigger::FRIEND_DIED ) ) {
                    critter.anger -= 15;
                }
            }
        }
    }
}

units::energy monster::use_mech_power( units::energy amt )
{
    if( is_hallucination() || !has_flag( mon_flag_RIDEABLE_MECH ) || !battery_item ) {
        return 0_kJ;
    }
    const int max_drain = battery_item->ammo_remaining( );
    const int consumption = std::min( static_cast<int>( units::to_kilojoule( amt ) ), max_drain );
    battery_item->ammo_consume( consumption, pos_bub(), nullptr );
    return units::from_kilojoule( static_cast<std::int64_t>( consumption ) );
}

int monster::mech_str_addition() const
{
    return type->mech_str_bonus;
}

bool monster::check_mech_powered() const
{
    if( is_hallucination() || !has_flag( mon_flag_RIDEABLE_MECH ) || !battery_item ) {
        return false;
    }
    if( battery_item->ammo_remaining( ) <= 0 ) {
        return false;
    }
    const itype &type = *battery_item->type;
    if( battery_item->ammo_remaining( ) <= type.magazine->capacity / 10 && one_in( 10 ) ) {
        add_msg( m_bad, _( "Your %s emits a beeping noise as its batteries start to get low." ),
                 get_name() );
    }
    return true;
}

void monster::generate_inventory( bool disableDrops )
{
    if( is_hallucination() ) {
        return;
    }
    if( type->death_drops.is_empty() ) {
        return;
    }

    std::vector<item> new_items = item_group::items_from( type->death_drops,
                                  calendar::start_of_cataclysm,
                                  spawn_flags::use_spawn_rate );

    for( item &it : new_items ) {
        if( has_flag( mon_flag_FILTHY ) ) {
            if( ( it.is_armor() || it.is_pet_armor() ) && !it.is_gun() ) {
                // handle wearable guns as a special case
                it.set_flag( STATIC( flag_id( "FILTHY" ) ) );
            }
        }
        inv.push_back( it );
    }
    no_extra_death_drops = disableDrops;
}

void monster::drop_items_on_death( map *here, item *corpse )
{
    if( is_hallucination() ) {
        return;
    }
    if( type->death_drops.is_empty() ) {
        return;
    }

    std::vector<item> new_items = item_group::items_from( type->death_drops,
                                  calendar::start_of_cataclysm,
                                  spawn_flags::use_spawn_rate );

    for( item &e : new_items ) {
        e.randomize_rot();
    }

    // for non corpses this is much simpler
    if( !corpse ) {
        for( item &it : new_items ) {
            here->add_item_or_charges( pos_bub( *here ), it );
        }
        return;
    }

    // first put "on" things that are wearable
    for( item &it : new_items ) {
        if( has_flag( mon_flag_FILTHY ) ) {
            if( ( it.is_armor() || it.is_pet_armor() ) && !it.is_gun() ) {
                // handle wearable guns as a special case
                it.set_flag( STATIC( flag_id( "FILTHY" ) ) );
            }
        }

        // add stuff that could be worn or strapped to the creature
        if( it.is_armor() ) {
            corpse->force_insert_item( it, pocket_type::CONTAINER );
        }
    }

    // then nest the rest in those "worn" items if possible
    // TODO: disable the backup, only spawn items here that actually fit in something
    for( item &it : new_items ) {
        // add stuff that could be worn or strapped to the creature
        if( !it.is_armor() ) {
            std::pair<item_location, item_pocket *> current_best;
            for( item *worn_it : corpse->all_items_top() ) {
                item_location loc;
                std::pair<item_location, item_pocket *> internal_pocket =
                    worn_it->best_pocket( it, loc, nullptr, false, true );
                if( internal_pocket.second != nullptr &&
                    ( current_best.second == nullptr ||
                      current_best.second->better_pocket( *internal_pocket.second, it ) ) ) {
                    current_best = internal_pocket;
                }
            }
            if( current_best.second != nullptr ) {
                current_best.second->insert_item( it );
            } else {
                corpse->force_insert_item( it, pocket_type::CONTAINER );
            }
        }
    }
}

void monster::spawn_dissectables_on_death( item *corpse ) const
{
    if( is_hallucination() ) {
        return;
    }
    if( type->dissect.is_empty() ) {
        return;
    }
    if( !corpse ) {
        return;
    }

    for( const harvest_entry &entry : *type->dissect ) {
        std::vector<item> dissectables = item_group::items_from( item_group_id( entry.drop ),
                                         calendar::turn,
                                         spawn_flags::use_spawn_rate );
        for( item &dissectable : dissectables ) {
            dissectable.dropped_from = entry.type;
            for( const flag_id &flg : entry.flags ) {
                dissectable.set_flag( flg );
            }
            for( const fault_id &flt : entry.faults ) {
                dissectable.set_fault( flt );
            }
            if( corpse ) {
                corpse->put_in( dissectable, pocket_type::CORPSE );
            }
        }
    }
}

void monster::process_one_effect( effect &it, bool is_new )
{
    map &here = get_map();

    // Monsters don't get trait-based reduction, but they do get effect based reduction
    bool reduced = resists_effect( it );
    const auto get_effect = [&it, is_new]( const std::string & arg, bool reduced ) {
        if( is_new ) {
            return it.get_amount( arg, reduced );
        }
        return it.get_mod( arg, reduced );
    };

    mod_speed_bonus( get_effect( "SPEED", reduced ) );
    mod_dodge_bonus( get_effect( "DODGE", reduced ) );
    mod_hit_bonus( get_effect( "HIT", reduced ) );
    mod_bash_bonus( get_effect( "BASH", reduced ) );
    mod_cut_bonus( get_effect( "CUT", reduced ) );
    mod_size_bonus( get_effect( "SIZE", reduced ) );

    int val = get_effect( "HURT", reduced );
    if( val > 0 ) {
        if( is_new || it.activated( calendar::turn, "HURT", val, reduced, 1 ) ) {
            apply_damage( it.get_source().resolve_creature(), bodypart_id( "torso" ), val );
        }
    }

    const efftype_id &id = it.get_id();
    // TODO: MATERIALS use fire resistance
    if( it.impairs_movement() ) {
        effect_cache[MOVEMENT_IMPAIRED] = true;
    } else if( id == effect_onfire ) {
        int dam = 0;
        if( made_of( material_veggy ) ) {
            dam = rng( 10, 20 );
        } else if( made_of( material_flesh ) || made_of( material_iflesh ) ) {
            dam = rng( 5, 10 );
        }

        dam -= get_armor_type( damage_heat, bodypart_id( "torso" ) );
        if( dam > 0 ) {
            apply_damage( it.get_source().resolve_creature(), bodypart_id( "torso" ), dam );
        } else {
            it.set_duration( 0_turns );
        }
    } else if( id == effect_run ) {
        effect_cache[FLEEING] = true;
    } else if( id == effect_no_sight || id == effect_blind ) {
        effect_cache[VISION_IMPAIRED] = true;
    } else if( ( id == effect_bleed || id == effect_dripping_mechanical_fluid ) &&
               x_in_y( it.get_intensity(), it.get_max_intensity() ) ) {
        // this is for balance only
        it.mod_duration( -rng( 1_turns, it.get_int_dur_factor() / 2 ) );
        bleed( here );
        if( id == effect_bleed ) {
            // monsters are simplified so they just take damage from bleeding
            apply_damage( it.get_source().resolve_creature(), bodypart_id( "torso" ), 1 );
        }
    } else if( id == effect_fake_common_cold ) {
        if( calendar::once_every( time_duration::from_seconds( rng( 30, 300 ) ) ) && one_in( 2 ) ) {
            sounds::sound( pos_bub(), 4, sounds::sound_t::speech, _( "a hacking cough." ), false, "misc",
                           "cough" );
        }

        avatar &you = get_avatar(); // No NPCs for now.
        if( rl_dist( pos_bub(), you.pos_bub() ) <= 1 ) {
            you.get_sick( false );
        }
    } else if( id == effect_fake_flu ) {
        // Need to define the two separately because it's theoretically (and realistically) possible to have both flu and cold at once, both for players and monsters.
        if( calendar::once_every( time_duration::from_seconds( rng( 30, 300 ) ) ) && one_in( 2 ) ) {
            sounds::sound( pos_bub(), 4, sounds::sound_t::speech, _( "a hacking cough." ), false, "misc",
                           "cough" );
        }

        avatar &you = get_avatar(); // No NPCs for now.
        if( rl_dist( pos_bub(), you.pos_bub() ) <= 1 ) {
            you.get_sick( true );
        }
    }

    //Process enchantments that apply to monsters.
    enchantment_cache->clear();

    for( const auto &elem : *effects ) {
        for( const enchantment_id &ench_id : elem.first->enchantments ) {
            const enchantment &ench = ench_id.obj();
            if( ench.is_active( *this ) && ench.is_monster_relevant() ) {
                enchantment_cache->force_add( ench, *this );
            }
        }
    }
    //Reset max speed
    set_speed_bonus( calculate_by_enchantment( get_speed_base(), enchant_vals::mod::SPEED,
                     true ) - get_speed_base() );
}

void monster::process_effects()
{
    map &here = get_map();

    // Monster only effects
    for( auto &elem : *effects ) {
        for( auto &_effect_it : elem.second ) {
            process_one_effect( _effect_it.second, false );
        }
    }

    // Like with player/NPCs - keep the speed above 0
    const int min_speed_bonus = -0.75 * get_speed_base();
    if( get_speed_bonus() < min_speed_bonus ) {
        set_speed_bonus( min_speed_bonus );
    }

    Character &player_character = get_player_character();
    //If this monster has the ability to heal in combat, do it now.
    int regeneration_amount = type->regenerates;
    //Apply effect-triggered regeneartion modifiers
    for( const auto &regeneration_modifier : type->regeneration_modifiers ) {
        if( has_effect( regeneration_modifier.first ) ) {
            regeneration_amount += regeneration_modifier.second;
        }
    }
    //Apply enchantment modifiers
    regeneration_amount = calculate_by_enchantment( regeneration_amount, enchant_vals::mod::REGEN_HP,
                          true );
    //Prevent negative regeneration
    if( regeneration_amount < 0 ) {
        regeneration_amount = 0;
    }
    const int healed_amount = heal( regeneration_amount );
    if( healed_amount > 0 && one_in( 2 ) ) {
        std::string healing_format_string;
        if( healed_amount >= 50 ) {
            healing_format_string = _( "The %s is visibly regenerating!" );
        } else if( healed_amount >= 10 ) {
            healing_format_string = _( "The %s seems a little healthier." );
        } else {
            healing_format_string = _( "The %s is healing slowly." );
        }
        add_msg_if_player_sees( *this, m_warning, healing_format_string, name() );
    }

    if( type->regenerates_in_dark && !g->is_in_sunlight( pos_bub() ) ) {
        const float light = here.ambient_light_at( pos_bub() );
        // Magic number 10000 was chosen so that a floodlight prevents regeneration in a range of 20 tiles
        const float dHP = 50.0 * std::exp( - light * light / 10000 );
        if( heal( static_cast<int>( dHP ) ) > 0 && one_in( 2 ) ) {
            add_msg_if_player_sees( *this, m_warning, _( "The %s uses the darkness to regenerate." ), name() );
        }
    }

    //Monster will regen morale and aggression if it is at/above max HP
    //It regens more morale and aggression if is currently fleeing.
    if( type->regen_morale && hp >= type->hp ) {
        if( is_fleeing( player_character ) ) {
            morale = type->morale;
            anger = type->agro;
        }
        if( morale < type->morale ) {
            morale += 1;
        }
        if( anger < type->agro ) {
            anger += 1;
        }
        if( morale < 0 && morale < type->morale ) {
            morale += 5;
            morale = std::min( morale, type->morale );
        }
        if( anger < 0 && anger < type->agro ) {
            anger += 5;
            anger = std::min( anger, type->agro );
        }
    }

    if( has_flag( mon_flag_CORNERED_FIGHTER ) ) {
        creature_tracker &creatures = get_creature_tracker();
        for( const tripoint_bub_ms &p : here.points_in_radius( pos_bub(), 2 ) ) {
            const monster *const mon = creatures.creature_at<monster>( p );
            const Character *const guy = creatures.creature_at<Character>( p );
            if( mon && mon != this && mon->faction->attitude( faction ) != MFA_FRIENDLY &&
                !has_effect( effect_spooked ) && morale <= 0 ) {
                if( !has_effect( effect_spooked_recent ) ) {
                    add_effect( effect_spooked, 3_turns, false );
                    add_effect( effect_spooked_recent, 9_turns, false );
                } else {
                    if( morale < type->morale ) {
                        morale = type->morale;
                        anger = type->agro;
                    }
                }
            }
            if( guy ) {
                monster_attitude att = attitude( guy );
                if( ( friendly == 0 ) && ( att == MATT_FOLLOW || att == MATT_FLEE ) &&
                    !has_effect( effect_spooked ) ) {
                    if( !has_effect( effect_spooked_recent ) ) {
                        add_effect( effect_spooked, 3_turns, false );
                        add_effect( effect_spooked_recent, 9_turns, false );
                    } else {
                        if( morale < type->morale ) {
                            morale = type->morale;
                            anger = type->agro;
                        }
                    }
                }
            }
        }
    }

    if( is_nemesis() ) {
        add_effect( effect_nemesis_buff, 1000_turns, true );
    }

    if( has_flag( mon_flag_PHOTOPHOBIC ) && here.ambient_light_at( pos_bub() ) >= 30.0f ) {
        add_msg_if_player_sees( *this, m_good, _( "The shadow withers in the light!" ), name() );
        add_effect( effect_photophobia, 5_turns, true );
    }

    // If this critter dies in sunlight, check & assess damage.
    if( has_flag( mon_flag_SUNDEATH ) && g->is_in_sunlight( pos_bub() ) ) {
        add_msg_if_player_sees( *this, m_good, _( "The %s burns horribly in the sunlight!" ), name() );
        apply_damage( nullptr, bodypart_id( "torso" ), 100 );
        if( hp < 0 ) {
            hp = 0;
        }
    }

    if( !will_be_cramped_in_vehicle_tile( here, pos_abs() ) ) {
        remove_effect( effect_cramped_space );
    }

    Creature::process_effects();
}

bool monster::make_fungus()
{
    if( is_hallucination() ) {
        return true;
    }
    if( type->in_species( species_FUNGUS ) ) { // No friendly-fungalizing ;-)
        return true;
    }
    if( !made_of( material_flesh ) && !made_of( material_hflesh ) &&
        !made_of( material_veggy ) && !made_of( material_iflesh ) &&
        !made_of( material_bone ) ) {
        // No fungalizing robots or weird stuff (mi-gos are technically fungi, blobs are goo)
        return true;
    }
    if( type->has_flag( mon_flag_NO_FUNG_DMG ) ) {
        return true; // Return true when monster is immune to fungal damage.
    }
    if( type->fungalize_into.is_empty() ) {
        return false;
    }

    const std::string old_name = name();
    poly( type->fungalize_into );

    add_msg_if_player_sees( pos_bub(), m_info, _( "The spores transform %1$s into a %2$s!" ),
                            old_name, name() );

    return true;
}

void monster::make_friendly()
{
    unset_dest();
    friendly = rng( 5, 30 ) + rng( 0, 20 );
}

void monster::make_ally( const monster &z )
{
    friendly = z.friendly;
    faction = z.faction;
}

void monster::add_item( const item &it )
{
    inv.push_back( it );
}

bool monster::is_hallucination() const
{
    return hallucination;
}

bool monster::is_electrical() const
{
    return in_species( species_ROBOT ) || has_flag( mon_flag_ELECTRIC ) || in_species( species_CYBORG );
}

bool monster::is_fae() const
{
    return has_flag( mon_flag_FAE_CREATURE );
}

bool monster::is_nether() const
{
    return in_species( species_HORROR ) || in_species( species_NETHER ) ||
           in_species( species_nether_player_hate );
}

// The logic is If PSI_NULL, no -> If HAS_MIND, yes -> if ZOMBIE, no -> if HUMAN, yes -> else, no, and monsters temporarily immune to telepathy cannot be seen
bool monster::has_mind() const
{
    return ( ( !in_species( species_PSI_NULL ) && has_flag( mon_flag_HAS_MIND ) ) ||
             ( !in_species( species_PSI_NULL ) && !in_species( species_ZOMBIE ) &&
               has_flag( mon_flag_HUMAN ) ) ) &&
           !has_effect( effect_eff_monster_immune_to_telepathy );
}

field_type_id monster::bloodType() const
{
    if( is_hallucination() ) {
        return fd_null;
    }
    return type->bloodType();
}
field_type_id monster::gibType() const
{
    if( is_hallucination() ) {
        return fd_null;
    }
    return type->gibType();
}

creature_size monster::get_size() const
{
    return static_cast<creature_size>( type->size + size_bonus );
}

units::mass monster::get_weight() const
{
    return enchantment_cache->modify_value( enchant_vals::mod::TOTAL_WEIGHT,
                                            units::operator*( type->weight, get_size() / type->size ) );
}

units::mass monster::weight_capacity() const
{
    return type->weight * type->mountable_weight_ratio;
}

units::volume monster::get_volume() const
{
    return units::operator*( type->volume, get_size() / type->size );
}

void monster::add_msg_if_npc( const std::string &msg ) const
{
    add_msg_if_player_sees( *this, replace_with_npc_name( msg ) );
}

void monster::add_msg_player_or_npc( const std::string &/*player_msg*/,
                                     const std::string &npc_msg ) const
{
    add_msg_if_player_sees( *this, replace_with_npc_name( npc_msg ) );
}

void monster::add_msg_if_npc( const game_message_params &params, const std::string &msg ) const
{
    add_msg_if_player_sees( *this, params, replace_with_npc_name( msg ) );
}

void monster::add_msg_player_or_npc( const game_message_params &params,
                                     const std::string &/*player_msg*/, const std::string &npc_msg ) const
{
    add_msg_if_player_sees( *this, params, replace_with_npc_name( npc_msg ) );
}

units::mass monster::get_carried_weight() const
{
    units::mass total_weight = 0_gram;
    if( tack_item ) {
        total_weight += tack_item->weight();
    }
    if( storage_item ) {
        total_weight += storage_item->weight();
    }
    if( armor_item ) {
        total_weight += armor_item->weight();
    }
    for( const item &it : inv ) {
        total_weight += it.weight();
    }
    return total_weight;
}

units::volume monster::get_carried_volume() const
{
    units::volume total_volume = 0_ml;
    for( const item &it : inv ) {
        total_volume += it.volume();
    }
    return total_volume;
}

void monster::move_special_item_to_inv( cata::value_ptr<item> &it )
{
    if( it ) {
        add_item( *it );
        it.reset();
    }
}

bool monster::is_dead() const
{
    return dead || is_dead_state();
}

bool monster::is_nemesis() const
{
    return has_flag( mon_flag_NEMESIS );
}

void monster::init_from_item( item &itm )
{
    if( itm.is_corpse() ) {
        set_speed_base( get_speed_base() * 0.8 );
        const int burnt_penalty = itm.burnt;
        hp = static_cast<int>( hp * 0.7 );
        if( itm.damage_level() > 0 ) {
            set_speed_base( speed_base / ( itm.damage_level() + 1 ) );
            hp /= itm.damage_level() + 1;
        }

        hp -= burnt_penalty;

        // HP can be 0 or less, in this case revive_corpse will just deactivate the corpse
        if( hp > 0 && type->has_flag( mon_flag_REVIVES_HEALTHY ) ) {
            hp = type->hp;
            set_speed_base( type->speed );
        }
        // if parent corpse is revives healthy *and* dormant, so will the monster
        const mtype *corpse_mtype = itm.get_mtype();
        if( hp > 0 && corpse_mtype->has_flag( mon_flag_REVIVES_HEALTHY ) &&
            corpse_mtype->has_flag( mon_flag_DORMANT ) ) {
            hp = type->hp;
            set_speed_base( type->speed );
        }
        upgrade_time = itm.get_var( "upgrade_time", 0 );
        for( item *it : itm.all_items_top( pocket_type::CONTAINER ) ) {
            if( it->is_armor() ) {
                it->set_flag( STATIC( flag_id( "FILTHY" ) ) );
            }
            inv.push_back( *it );
            itm.remove_item( *it );
        }
        //Move dissectables (installed bionics, etc)
        for( item *dissectable : itm.all_items_top( pocket_type::CORPSE ) ) {
            dissectable_inv.push_back( *dissectable );
            itm.remove_item( *dissectable );
        }
    } else {
        // must be a robot
        const int damfac = itm.max_damage() - std::max( 0, itm.damage() ) + 1;
        // One hp at least, everything else would be unfair (happens only to monster with *very* low hp),
        hp = std::max( 1, hp * damfac / ( itm.max_damage() + 1 ) );
    }
}

item monster::to_item() const
{
    if( type->revert_to_itype.is_empty() ) {
        return item();
    }
    // Birthday is wrong, but the item created here does not use it anyway (I hope).
    item result( type->revert_to_itype, calendar::turn );
    const int damfac = std::max( 1, ( result.max_damage() + 1 ) * hp / type->hp );
    result.set_damage( std::max( 0, ( result.max_damage() + 1 ) - damfac ) );
    return result;
}

float monster::power_rating() const
{
    // This should probably be replaced by something based on difficulty,
    // at least for smarter critters using it for evaluation.
    float ret = get_size() - 2.0f; // Zed gets 1, cat -1, hulk 3
    ret += is_ranged_attacker() ? 2.0f : 0.0f;
    // Hostile stuff gets a big boost
    // Neutral moose will still get burned if it comes close
    return ret;
}

float monster::speed_rating() const
{
    float ret = get_speed() / 100.0f;
    const auto leap = type->special_attacks.find( "leap" );
    if( leap != type->special_attacks.end() ) {
        // TODO: Make this calculate sane values here
        ret += 0.5f;
    }

    return ret;
}

void monster::on_dodge( Creature *, float, float )
{
    // Currently does nothing, later should handle faction relations
}

void monster::on_hit( map *here, Creature *source, bodypart_id,
                      float, dealt_projectile_attack const *const proj )
{
    if( is_hallucination() ) {
        return;
    }

    if( rng( 0, 100 ) <= static_cast<int>( type->def_chance ) ) {
        type->sp_defense( *this, source, proj );
    }

    // Adjust anger/morale of nearby monsters, if they have the appropriate trigger and are friendly
    // Keep filtering on the hit monsters' triggers to preserve current functionality
    bool trigger = type->has_anger_trigger( mon_trigger::FRIEND_ATTACKED ) ||
                   type->has_fear_trigger( mon_trigger::FRIEND_ATTACKED ) ||
                   type->has_placate_trigger( mon_trigger::FRIEND_ATTACKED );

    if( trigger ) {
        int light = g->light_level( posz() );
        for( monster &critter : g->all_monsters() ) {
            // Do we actually care about this faction?
            if( critter.faction->attitude( faction ) != MFA_FRIENDLY ) {
                continue;
            }

            if( here->sees( critter.pos_bub( *here ), pos_bub( *here ), light ) ) {
                // Anger trumps fear trumps ennui
                if( critter.type->has_anger_trigger( mon_trigger::FRIEND_ATTACKED ) ) {
                    critter.anger += 15;
                    if( source != nullptr && !source->is_monster() && !source->is_fake() ) {
                        // A character attacked our friend
                        add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro triggered by attacking a friendly %s",
                                       critter.name(), name() );
                        critter.aggro_character = true;
                    }
                } else if( critter.type->has_fear_trigger( mon_trigger::FRIEND_ATTACKED ) ) {
                    critter.morale -= 15;
                } else if( critter.type->has_placate_trigger( mon_trigger::FRIEND_ATTACKED ) ) {
                    critter.anger -= 15;
                }
            }
        }
    }
    if( source != nullptr ) {
        if( Character *attacker = source->as_character() ) {
            type->families.practice_hit( *attacker );
        }
    }

    enchantment_cache->cast_hit_me( *this, source );

    check_dead_state( here );
    // TODO: Faction relations
}

int monster::get_hp_max( const bodypart_id & ) const
{
    return type->hp;
}

int monster::get_hp_max() const
{
    return type->hp;
}

int monster::get_hp( const bodypart_id & ) const
{
    return hp;
}

int monster::get_hp() const
{
    return hp;
}

float monster::get_mountable_weight_ratio() const
{
    return type->mountable_weight_ratio;
}

void monster::hear_sound( const tripoint_bub_ms &source, const int vol, const int dist,
                          bool provocative )
{
    if( !can_hear() ) {
        return;
    }

    const bool goodhearing = has_flag( mon_flag_GOODHEARING );
    const int volume = goodhearing ? 2 * vol - dist : vol - dist;
    // Error is based on volume, louder sound = less error
    if( volume <= 0 ) {
        return;
    }

    int tmp_provocative = provocative || volume >= normal_roll( 30, 5 );
    // already following a more interesting sound
    if( provocative_sound && !tmp_provocative && wandf > 0 ) {
        return;
    }

    int max_error = 0;
    if( volume < 2 ) {
        max_error = 10;
    } else if( volume < 5 ) {
        max_error = 5;
    } else if( volume < 10 ) {
        max_error = 3;
    } else if( volume < 20 ) {
        max_error = 1;
    }

    tripoint_abs_ms target = get_map().get_abs( source ) + point( rng( -max_error, max_error ),
                             rng( -max_error, max_error ) );
    // target_z will require some special check due to soil muffling sounds

    const int wander_turns = volume * ( goodhearing ? 6 : 1 );
    // again, already following a more interesting sound
    if( wander_turns < wandf ) {
        return;
    }
    // only trigger this if the monster is not friendly or the source isn't the player
    if( friendly == 0 || source != get_player_character().pos_bub() ) {
        process_trigger( mon_trigger::SOUND, volume );
    }
    provocative_sound = tmp_provocative;
    if( morale >= 0 && anger >= 10 ) {
        // TODO: Add a proper check for fleeing attitude
        // but cache it nicely, because this part is called a lot
        wander_to( target, wander_turns );
    } else if( morale < 0 ) {
        // Monsters afraid of sound should not go towards sound.
        // Move towards a point on the opposite side of us from the target.
        // TODO: make the destination scale with the sound and handle
        // the case when (x,y) is the same by picking a random direction
        tripoint_abs_ms away = pos_abs() + ( pos_abs() - target );
        away.z() = posz();
        wander_to( away, wander_turns );
    }
}

monster_horde_attraction monster::get_horde_attraction()
{
    if( horde_attraction == MHA_NULL ) {
        horde_attraction = static_cast<monster_horde_attraction>( rng( 1, 5 ) );
    }
    return horde_attraction;
}

void monster::set_horde_attraction( monster_horde_attraction mha )
{
    horde_attraction = mha;
}

bool monster::will_join_horde( int size )
{
    const monster_horde_attraction mha = get_horde_attraction();
    if( this->has_flag( mon_flag_IMMOBILE ) || this->has_flag( mon_flag_NEVER_WANDER ) ||
        this->has_flag( json_flag_CANNOT_MOVE ) ) {
        return false; //immobile monsters should never join a horde. Same with Never Wander monsters.
    }
    if( mha == MHA_NEVER ) {
        return false;
    }
    if( mha == MHA_ALWAYS ) {
        return true;
    }
    if( get_map().has_flag( ter_furn_flag::TFLAG_INDOORS, pos_bub() ) &&
        ( mha == MHA_OUTDOORS || mha == MHA_OUTDOORS_AND_LARGE ) ) {
        return false;
    }
    if( size < 3 && ( mha == MHA_LARGE || mha == MHA_OUTDOORS_AND_LARGE ) ) {
        return false;
    }
    return true;
}

void monster::on_unload()
{
    last_updated = calendar::turn;
}

void monster::on_load()
{
    try_upgrade( false );
    try_reproduce();
    try_biosignature();

    if( has_flag( mon_flag_MILKABLE ) ) {
        refill_udders();
    }

    const time_duration dt = calendar::turn - last_updated;
    last_updated = calendar::turn;
    if( dt <= 0_turns ) {
        return;
    }

    if( morale != type->morale ) { // if works, will put into helper function
        int dt_left_m = to_turns<int>( dt );

        if( std::abs( morale - type->morale ) > 15 ) {
            const int adjust_by_m = std::min( ( dt_left_m / 4 ),
                                              ( std::abs( morale - type->morale ) - 15 ) );
            dt_left_m -= adjust_by_m * 4;
            if( morale < type->morale ) {
                morale += adjust_by_m;
            } else {
                morale -= adjust_by_m;
            }
        }

        // Avoiding roll_remainder - PC out of situation, monster can calm down
        if( morale < type->morale ) {
            morale += std::min( static_cast<int>( std::ceil( dt_left_m / 8.0 ) ),
                                std::abs( morale - type->morale ) );
        } else {
            morale -= std::min( ( dt_left_m / 8 ),
                                std::abs( morale - type->morale ) );
        }
    }
    if( anger != type->agro ) {
        int dt_left_a = to_turns<int>( dt );

        if( std::abs( anger - type->agro ) > 15 ) {
            const int adjust_by_a = std::min( ( dt_left_a / 4 ),
                                              ( std::abs( anger - type->agro ) - 15 ) );
            dt_left_a -= adjust_by_a * 4;
            if( anger < type->agro ) {
                anger += adjust_by_a;
            } else {
                anger -= adjust_by_a;
            }
        }

        if( anger > type->agro ) {
            anger -= std::min( static_cast<int>( std::ceil( dt_left_a / 8.0 ) ),
                               std::abs( anger - type->agro ) );
        } else {
            anger += std::min( ( dt_left_a / 8 ),
                               std::abs( anger - type->agro ) );
        }
        // If we got angry at characters have a chance at calming down
        if( aggro_character && !type->aggro_character && !x_in_y( anger, 100 ) ) {
            add_msg_debug( debugmode::DF_MONSTER, "%s's character aggro reset", name() );
            aggro_character = false;
        }
    }

    // TODO: regen_morale
    float regen = type->regenerates;
    if( regen <= 0 ) {
        if( has_flag( mon_flag_REVIVES ) && !has_flag( mon_flag_DORMANT ) ) {
            regen = 0.02f * type->hp / to_turns<int>( 1_hours );
        } else if( made_of( material_flesh ) || made_of( material_iflesh ) ||
                   made_of( material_veggy ) ) {
            // Most living stuff here
            regen = 0.005f * type->hp / to_turns<int>( 1_hours );
        }
    }
    const int heal_amount = roll_remainder( regen * to_turns<int>( dt ) );
    const int healed = heal( heal_amount );
    int healed_speed = 0;
    if( get_speed_base() < type->speed ) {
        const int old_speed = get_speed_base();
        if( hp >= type->hp ) {
            set_speed_base( type->speed );
        } else {
            const int speed_delta = std::max( healed * type->speed / type->hp, 1 );
            set_speed_base( std::min( old_speed + speed_delta, type->speed ) );
        }
        healed_speed = get_speed_base() - old_speed;
    }

    // Update special attacks' cooldown
    for( const auto &sp_type : type->special_attacks ) {
        const std::string &special_name = sp_type.first;
        const auto local_iter = special_attacks.find( special_name );
        if( local_iter == special_attacks.end() ) {
            continue;
        }
        mon_special_attack &local_attack_data = local_iter->second;
        if( !local_attack_data.enabled ) {
            continue;
        }

        if( local_attack_data.cooldown > 0 ) {
            local_attack_data.cooldown = std::max( 0, local_attack_data.cooldown - to_turns<int>( dt ) );
        }
    }

    add_msg_debug( debugmode::DF_MONSTER, "on_load() by %s, %d turns, healed %d hp, %d speed",
                   name(), to_turns<int>( dt ), healed, healed_speed );
}

const pathfinding_settings &monster::get_pathfinding_settings() const
{
    return type->path_settings;
}

std::function<bool( const tripoint_bub_ms & )> monster::get_path_avoid() const
{
    return [this]( const tripoint_bub_ms & p ) {
        map &here = get_map();
        // If we can't move there and can't bash it, don't path through it.
        if( !can_move_to( p ) && ( bash_skill() <= 0 || !here.is_bashable( p ) ) ) {
            return true;
        }

        // Avoid nearby creatures if we have the flag.
        int radius;
        if( has_flag( mon_flag_PRIORITIZE_TARGETS ) ) {
            radius = 2;
        } else if( has_flag( mon_flag_PATH_AVOID_DANGER ) ) {
            radius = 1;
        } else {
            return false;
        }
        if( rl_dist( p, pos_bub() ) <= radius && get_creature_tracker().creature_at( p ) ) {
            return true;
        }
        return false;
    };
}

std::vector<std::pair<std::string, std::string>> monster::get_overlay_ids() const
{
    std::vector<std::pair<std::string, std::string>> rval;

    // get effects
    // at this moment we share id for effect overlay for character and for monster
    if( show_creature_overlay_icons ) {
        // if at one point there would be more overlay types than one
        // someone would need to tinker pre-allocation
        rval.reserve( effects->size() );
        for( const auto &eff_pr : *effects ) {
            rval.emplace_back( "effect_" + eff_pr.first.str(), "" );
        }
    }

    return rval;
}
