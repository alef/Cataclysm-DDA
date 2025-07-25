#include <algorithm>
#include <array>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "colony.h"
#include "construction.h"
#include "coordinates.h"
#include "debug.h"
#include "field.h"
#include "flexbuffer_json.h"
#include "item.h"
#include "json_loader.h"
#include "map_scale_constants.h"
#include "point.h"
#include "string_formatter.h"
#include "submap.h"
#include "trap.h"
#include "type_id.h"
#include "vehicle.h"

static const construction_str_id construction_constr_ground_cable( "constr_ground_cable" );
static const construction_str_id construction_constr_rack_coat( "constr_rack_coat" );

static const field_type_str_id field_fd_acid( "fd_acid" );
static const field_type_str_id field_fd_electricity( "fd_electricity" );
static const field_type_str_id field_fd_laser( "fd_laser" );
static const field_type_str_id field_fd_nuke_gas( "fd_nuke_gas" );
static const field_type_str_id field_fd_smoke( "fd_smoke" );
static const field_type_str_id field_fd_web( "fd_web" );
static const field_type_str_id field_test_fd_migration_new_id( "test_fd_migration_new_id" );

static const furn_str_id furn_f_bookcase( "f_bookcase" );
static const furn_str_id furn_f_coffin_c( "f_coffin_c" );
static const furn_str_id furn_f_crate_o( "f_crate_o" );
static const furn_str_id furn_f_dresser( "f_dresser" );
static const furn_str_id furn_f_gas_tank( "f_gas_tank" );
static const furn_str_id furn_test_f_migration_new_id( "test_f_migration_new_id" );

static const itype_id itype_bat_nerf( "bat_nerf" );
static const itype_id itype_bottle_plastic( "bottle_plastic" );
static const itype_id itype_foodperson_mask( "foodperson_mask" );
static const itype_id itype_foon( "foon" );
static const itype_id itype_jackhammer( "jackhammer" );
static const itype_id itype_machete( "machete" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_floor_blue( "t_floor_blue" );
static const ter_str_id ter_t_floor_green( "t_floor_green" );
static const ter_str_id ter_t_floor_red( "t_floor_red" );
static const ter_str_id ter_t_rock_floor( "t_rock_floor" );
static const ter_str_id ter_test_t_migration_new_id( "test_t_migration_new_id" );

static const trap_str_id tr_beartrap( "tr_beartrap" );
static const trap_str_id tr_bubblewrap( "tr_bubblewrap" );
static const trap_str_id tr_funnel( "tr_funnel" );
static const trap_str_id tr_landmine( "tr_landmine" );
static const trap_str_id tr_rollmat( "tr_rollmat" );

// NOLINTNEXTLINE(cata-static-declarations)
extern const int savegame_version;

static const point_sm_ms &corner_ne = point_sm_ms::zero;
static const point_sm_ms corner_nw( SEEX - 1, 0 );
static const point_sm_ms corner_se( 0, SEEY - 1 );
static const point_sm_ms corner_sw( SEEX - 1, SEEY - 1 );
static const point_sm_ms random_pt( 4, 7 );

static std::string submap_empty_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_terrain_rle_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [\n"
    "    \"t_floor_red\",\n"
    "    [ \"t_dirt\", 10 ],\n"
    "    \"t_floor_green\",\n"
    "    [ \"t_dirt\", 60 ],\n"
    "    [ \"t_rock_floor\", 60 ],\n"
    "    \"t_floor_blue\",\n"
    "    [ \"t_rock_floor\", 10 ],\n"
    "    \"t_floor\"\n"
    "  ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_terrain_with_invalid_ter_ids_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [\n"
    "    [ \"t_this_ter_id_does_not_exist\", 143 ],\n"
    "    \"t_rock_floor\""
    "  ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_furniture_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [\n"
    "    [ 0, 0, \"f_bookcase\" ],\n"
    "    [ 0, 11, \"f_crate_o\" ],\n"
    "    [ 11, 0, \"f_coffin_c\" ],\n"
    "    [ 11, 11, \"f_dresser\" ],\n"
    "    [ 4, 7, \"f_gas_tank\" ]\n"
    "  ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_trap_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [\n"
    "    [ 0, 0, \"tr_bubblewrap\" ],\n"
    "    [ 0, 11, \"tr_funnel\" ],\n"
    "    [ 11, 0, \"tr_rollmat\" ],\n"
    "    [ 11, 11, \"tr_beartrap\" ],\n"
    "    [ 4, 7, \"tr_landmine\" ]\n"
    "  ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_rad_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [\n"
    "    1, 1,\n"
    "    0, 10,\n"
    "    2, 1,\n"
    "    0, 76,\n"
    "    5, 1,\n"
    "    0, 43,\n"
    "    3, 1,\n"
    "    0, 10,\n"
    "    4, 1\n"
    "  ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_item_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [\n"
    "    0, 0, [\n"
    "      {\n"
    "       \"typeid\": \"foodperson_mask\",\n"
    "       \"bday\": 39316373,\n"
    "       \"item_vars\": { \"magazine_converted\": \"1\" },\n"
    "       \"item_tags\": [ \"OUTER\", \"SUN_GLASSES\" ],\n"
    "       \"relic_data\": null,\n"
    "       \"contents\": {\n"
    "         \"contents\": [\n"
    "            { \"pocket_type\": 2, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      }\n"
    "    ],\n"
    "    0, 11, [\n"
    "      {\n"
    "        \"typeid\": \"bat_nerf\",\n"
    "        \"bday\": 39316373,\n"
    "        \"item_tags\": [ \"FRAGILE_MELEE\" ],\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      }\n"
    "    ],\n"
    "    11, 0, [\n"
    "      {\n"
    "        \"typeid\": \"machete\",\n"
    "        \"bday\": 39316373,\n"
    "        \"item_vars\": { \"magazine_converted\": \"1\" },\n"
    "        \"item_tags\": [ \"DURABLE_MELEE\", \"SHEATH_SWORD\" ],\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      },\n"
    "      {\n"
    "        \"typeid\": \"foon\",\n"
    "        \"bday\": 39316373,\n"
    "        \"owner\": \"your_followers\",\n"
    "        \"item_tags\": [ \"STAB\", \"SHEATH_KNIFE\" ],\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      }\n"
    "    ],\n"
    "    11, 11, [\n"
    "      {\n"
    "        \"typeid\": \"bottle_plastic\",\n"
    "        \"bday\": 39316373,\n"
    "        \"owner\": \"your_followers\",\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 0, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      }\n"
    "    ],\n"
    "    4, 7, [\n"
    "      {\n"
    "        \"typeid\": \"jackhammer\",\n"
    "        \"bday\": 39316417,\n"
    "        \"item_tags\": [ \"DIG_TOOL\", \"POWERED\", \"STAB\" ],\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            {\n"
    "              \"pocket_type\": 1,\n"
    "              \"contents\": [\n"
    "                {\n"
    "                  \"typeid\": \"gasoline\",\n"
    "                  \"charges\": 400,\n"
    "                  \"bday\": 39316417,\n"
    "                  \"relic_data\": null,\n"
    "                  \"contents\": {\n"
    "                    \"contents\": [\n"
    "                      { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "                      { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "                      { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "                    ]\n"
    "                  }\n"
    "                }\n"
    "              ],\n"
    "              \"_sealed\": false\n"
    "            },\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      }\n"
    "    ]\n"
    "  ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_field_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [\n"
    "    0, 0, [ \"fd_laser\", 1, 1997 ],\n"
    "    0, 11, [ \"fd_acid\", 2, 2003 ],\n"
    "    11, 0, [ \"fd_web\", 3, 2077 ],\n"
    "    11, 0, [ \"fd_smoke\", 4, 3004 ],\n"
    "    11, 11, [ \"fd_electricity\", 5, 1482 ],\n"
    "    4, 7, [ \"fd_nuke_gas\", 6, 1615 ]\n"
    "  ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_graffiti_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"graffiti\": [\n"
    "    [ 0, 0, \"a\" ],\n"
    "    [ 0, 11, \"b\" ],\n"
    "    [ 11, 0, \"c\" ],\n"
    "    [ 11, 11, \"d\" ],\n"
    "    [ 4, 7, \"e\" ]\n"
    "  ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_spawns_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [\n"
    "    [ \"mon_pheasant\", 1, 0, 0, -1, -1, false, \"NONE\" ],\n"
    "    [ \"mon_mininuke_hack\", 2, 0, 11, -1, -1, true, \"Tim\" ],\n"
    "    [ \"mon_fish_eel\", 3, 11, 0, -1, -1, false, \"Bob\" ],\n"
    "    [ \"mon_zombie_fungus\", 4, 11, 11, -1, -1, false, \"Hopper\" ],\n"
    "    [ \"mon_plague_vector\", 5, 4, 7, -1, -1, true, \"Alice\" ]\n"
    "  ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_vehicle_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [\n"
    "    {\n"
    "      \"type\": \"welding_cart\",\n"
    "      \"posx\": 9,\n"
    "      \"posy\": 2,\n"
    "      \"om_id\": 0,\n"
    "      \"faceDir\": 270,\n"
    "      \"moveDir\": 270,\n"
    "      \"turn_dir\": -90,\n"
    "      \"velocity\": 0,\n"
    "      \"falling\": false,\n"
    "      \"floating\": false,\n"
    "      \"flying\": false,\n"
    "      \"cruise_velocity\": 0,\n"
    "      \"vertical_velocity\": 0,\n"
    "      \"engine_on\": false,\n"
    "      \"tracking_on\": false,\n"
    "      \"skidding\": false,\n"
    "      \"of_turn_carry\": 0.0,\n"
    "      \"name\": \"Welding Cart\",\n"
    "      \"owner\": \"your_followers\",\n"
    "      \"old_owner\": \"NULL\",\n"
    "      \"theft_time\": null,\n"
    "      \"parts\": [\n"
    "        {\n"
    "          \"id\": \"xlframe\",\n"
    "          \"variant\": \"vertical_2\",\n"
    "          \"base\": {\n"
    "            \"typeid\": \"xlframe\",\n"
    "            \"item_tags\": [ \"VEHICLE\" ],\n"
    "            \"relic_data\": null,\n"
    "            \"contents\": {\n"
    "              \"contents\": [\n"
    "                { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "                { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "                { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "              ]\n"
    "            }\n"
    "          },\n"
    "          \"mount_dx\": 0,\n"
    "          \"mount_dy\": 0,\n"
    "          \"open\": false,\n"
    "          \"direction\": 0,\n"
    "          \"blood\": 0,\n"
    "          \"enabled\": false,\n"
    "          \"flags\": 0,\n"
    "          \"passenger_id\": -1,\n"
    "          \"crew_id\": -1,\n"
    "          \"items\": [  ],\n"
    "          \"ammo_pref\": \"null\"\n"
    "        },\n"
    "        {\n"
    "          \"id\": \"wheel_caster_large\",\n"
    "          \"base\": {\n"
    "            \"typeid\": \"wheel_caster_large\",\n"
    "            \"damaged\": 3372,\n"
    "            \"item_tags\": [ \"VEHICLE\" ],\n"
    "            \"relic_data\": null,\n"
    "            \"contents\": {\n"
    "              \"contents\": [\n"
    "                { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "                { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "                { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "              ]\n"
    "            }\n"
    "          },\n"
    "          \"mount_dx\": 0,\n"
    "          \"mount_dy\": 0,\n"
    "          \"open\": false,\n"
    "          \"direction\": 0,\n"
    "          \"blood\": 0,\n"
    "          \"enabled\": false,\n"
    "          \"flags\": 0,\n"
    "          \"passenger_id\": -1,\n"
    "          \"crew_id\": -1,\n"
    "          \"items\": [  ],\n"
    "          \"ammo_pref\": \"null\"\n"
    "        },\n"
    "        {\n"
    "          \"id\": \"small_storage_battery\",\n"
    "          \"base\": {\n"
    "            \"typeid\": \"small_storage_battery\",\n"
    "            \"damaged\": 3000,\n"
    "            \"item_tags\": [ \"VEHICLE\" ],\n"
    "            \"relic_data\": null,\n"
    "            \"contents\": {\n"
    "              \"contents\": [\n"
    "                {\n"
    "                  \"pocket_type\": 1,\n"
    "                  \"contents\": [\n"
    "                    {\n"
    "                      \"typeid\": \"battery\",\n"
    "                      \"charges\": 316,\n"
    "                      \"bday\": 39317353,\n"
    "                      \"item_tags\": [ \"IRREMOVABLE\", \"NO_DROP\" ],\n"
    "                      \"relic_data\": null,\n"
    "                      \"contents\": {\n"
    "                        \"contents\": [\n"
    "                          { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "                          { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "                          { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "                        ]\n"
    "                      }\n"
    "                    }\n"
    "                  ],\n"
    "                  \"_sealed\": false\n"
    "                },\n"
    "                { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "                { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "                { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "              ]\n"
    "            }\n"
    "          },\n"
    "          \"mount_dx\": 0,\n"
    "          \"mount_dy\": 0,\n"
    "          \"open\": false,\n"
    "          \"direction\": 0,\n"
    "          \"blood\": 0,\n"
    "          \"enabled\": false,\n"
    "          \"flags\": 0,\n"
    "          \"passenger_id\": -1,\n"
    "          \"crew_id\": -1,\n"
    "          \"items\": [  ],\n"
    "          \"ammo_pref\": \"null\"\n"
    "        },\n"
    "        {\n"
    "          \"id\": \"veh_tools_workshop\",\n"
    "          \"base\": {\n"
    "            \"typeid\": \"veh_tools_workshop\",\n"
    "            \"damaged\": 2000,\n"
    "            \"item_tags\": [ \"VEHICLE\" ],\n"
    "            \"relic_data\": null,\n"
    "            \"contents\": {\n"
    "              \"contents\": [\n"
    "                { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "                { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "                { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "              ]\n"
    "            }\n"
    "          },\n"
    "          \"mount_dx\": 0,\n"
    "          \"mount_dy\": 0,\n"
    "          \"open\": false,\n"
    "          \"direction\": 0,\n"
    "          \"blood\": 0,\n"
    "          \"enabled\": false,\n"
    "          \"flags\": 0,\n"
    "          \"passenger_id\": -1,\n"
    "          \"crew_id\": -1,\n"
    "          \"items\": [\n"
    "            {\n"
    "              \"typeid\": \"goggles_welding\",\n"
    "              \"bday\": 39317353,\n"
    "              \"relic_data\": null,\n"
    "              \"contents\": {\n"
    "                \"contents\": [\n"
    "                  { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "                  { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "                  { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "                ]\n"
    "              }\n"
    "            }\n"
    "          ],\n"
    "          \"ammo_pref\": \"null\"\n"
    "        }\n"
    "      ],\n"
    "      \"tags\": [  ],\n"
    "      \"labels\": [  ],\n"
    "      \"zones\": [  ],\n"
    "      \"other_tow_point\": [ 0, 0, 0 ],\n"
    "      \"is_locked\": false,\n"
    "      \"is_alarm_on\": false,\n"
    "      \"camera_on\": false,\n"
    "      \"last_update_turn\": 39317413,\n"
    "      \"pivot\": [ 0, 0 ],\n"
    "      \"is_following\": false,\n"
    "      \"is_patrolling\": false,\n"
    "      \"autodrive_local_target\": [ 0, 0, 0 ],\n"
    "      \"airworthy\": true,\n"
    "      \"summon_time_limit\": null,\n"
    "      \"magic\": false,\n"
    "      \"smart_controller\": null\n"
    "    }\n"
    "  ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_construction_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [\n"
    "    3,\n"
    "    2,\n"
    "    0,\n"
    "    123334,\n"
    "    \"constr_ground_cable\",\n"
    "    [\n"
    "      {\n"
    "        \"typeid\": \"cable\",\n"
    "        \"charges\": 4,\n"
    "        \"bday\": 39319475,\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      }\n"
    "    ],\n"
    "    3,\n"
    "    3,\n"
    "    0,\n"
    "    4934,\n"
    "    \"constr_rack_coat\",\n"
    "    [\n"
    "      {\n"
    "        \"typeid\": \"2x4\",\n"
    "        \"bday\": 39316447,\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      },\n"
    "      {\n"
    "        \"typeid\": \"2x4\",\n"
    "        \"bday\": 39316447,\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      },\n"
    "      {\n"
    "        \"typeid\": \"2x4\",\n"
    "        \"bday\": 39316972,\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      },\n"
    "      {\n"
    "        \"typeid\": \"nail\",\n"
    "        \"charges\": 8,\n"
    "        \"bday\": 39316972,\n"
    "        \"relic_data\": null,\n"
    "        \"contents\": {\n"
    "          \"contents\": [\n"
    "            { \"pocket_type\": 4, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 3, \"contents\": [  ], \"_sealed\": false },\n"
    "            { \"pocket_type\": 6, \"contents\": [  ], \"_sealed\": false }\n"
    "          ]\n"
    "        }\n"
    "      }\n"
    "    ]\n"
    "  ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_computer_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [\n"
    "    [ 0, 1 ],\n"
    "    {\n"
    "      \"name\": \"Bionic Vault\",\n"
    "      \"mission\": -1,\n"
    "      \"security\": 3,\n"
    "      \"alerts\": 0,\n"
    "      \"next_attempt\": -1,\n"
    "      \"options\": [\n"
    "        { \"name\": \"MANIFEST\", \"action\": \"list_bionics\", \"security\": 0 },\n"
    "        { \"name\": \"UNLOCK ENTRANCE\", \"action\": \"unlock_disarm\", \"security\": 7 }\n"
    "      ],\n"
    "      \"failures\": [ { \"action\": \"damage\" }, { \"action\": \"secubots\" }, { \"action\": \"shutdown\" } ],\n"
    "      \"access_denied\": \"ERROR!  Access denied!  Unauthorized access will be met with lethal force!\"\n"
    "    },\n"
    "    [ 3, 5 ],\n"
    "    {\n"
    "      \"name\": \"Bionic Vault\",\n"
    "      \"mission\": -1,\n"
    "      \"security\": 3,\n"
    "      \"alerts\": 0,\n"
    "      \"next_attempt\": -1,\n"
    "      \"options\": [\n"
    "        { \"name\": \"MANIFEST\", \"action\": \"list_bionics\", \"security\": 0 },\n"
    "        { \"name\": \"UNLOCK ENTRANCE\", \"action\": \"unlock_disarm\", \"security\": 7 }\n"
    "      ],\n"
    "      \"failures\": [ { \"action\": \"damage\" }, { \"action\": \"secubots\" }, { \"action\": \"shutdown\" } ],\n"
    "      \"access_denied\": \"ERROR!  Access denied!  Unauthorized access will be met with lethal force!\"\n"
    "    }\n"
    "  ]\n"
    "}\n"
);
static std::string submap_cosmetic_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [\n"
    "    [ 0, 11, \"f_sign\" ],\n"
    "    [ 11, 11, \"f_sign\" ]\n"
    "  ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [\n"
    "    [ 0, 0, \"GRAFFITI\",  \"This is written text.\" ],\n"
    "    [ 0, 11, \"SIGNAGE\",  \"Subway Map: illegible city name stop\" ],\n"
    "    [ 11, 0, \"GRAFFITI\",  \"I <3 Dr. Hylke van der Schaaf.\" ],\n"
    "    [ 11, 11, \"SIGNAGE\",  \"This is a sign\" ],\n"
    "    [ 4, 7, \"GRAFFITI\",  \"Santina is a heteronormative bully!\" ]\n"
    "  ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_ter_furn_pre_migration_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"test_t_migration_old_id\", 1 ], [ \"t_dirt\", 10 ], [ \"test_t_migration_old_id\", 1 ], [ \"t_dirt\", 132 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [\n"
    "    [ 0, 0, \"f_bookcase\" ],\n"
    "    [ 0, 11, \"test_f_migration_old_id\" ],\n"
    "    [ 11, 11, \"test_f_migration_old_id\" ]\n"
    "  ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);
static std::string submap_fd_pre_migration_ss(
    "{\n"
    "  \"version\": 32,\n"
    "  \"coordinates\": [ 0, 0, 0 ],\n"
    "  \"turn_last_touched\": 0,\n"
    "  \"temperature\": 0,\n"
    "  \"terrain\": [ [ \"t_dirt\", 144 ] ],\n"
    "  \"radiation\": [ 0, 144 ],\n"
    "  \"furniture\": [ ],\n"
    "  \"items\": [ ],\n"
    "  \"traps\": [ ],\n"
    "  \"fields\": [ 0, 0, [ \"test_fd_migration_old_id\", 1, 1997 ] ],\n"
    "  \"cosmetics\": [ ],\n"
    "  \"spawns\": [ ],\n"
    "  \"vehicles\": [ ],\n"
    "  \"partial_constructions\": [ ],\n"
    "  \"computers\": [ ]\n"
    "}\n"
);

static_assert( SEEX == 12, "Reminder to update submap tests when SEEX changes." );
static_assert( SEEY == 12, "Reminder to update submap tests when SEEY changes." );

static JsonValue submap_empty = json_loader::from_string( submap_empty_ss );
static JsonValue submap_terrain_rle = json_loader::from_string( submap_terrain_rle_ss );
static JsonValue submap_terrain_with_invalid_ter_ids = json_loader::from_string(
            submap_terrain_with_invalid_ter_ids_ss );
static JsonValue submap_furniture = json_loader::from_string( submap_furniture_ss );
static JsonValue submap_trap = json_loader::from_string( submap_trap_ss );
static JsonValue submap_rad = json_loader::from_string( submap_rad_ss );
static JsonValue submap_item = json_loader::from_string( submap_item_ss );
static JsonValue submap_field = json_loader::from_string( submap_field_ss );
static JsonValue submap_graffiti = json_loader::from_string( submap_graffiti_ss );
static JsonValue submap_spawns = json_loader::from_string( submap_spawns_ss );
static JsonValue submap_vehicle = json_loader::from_string( submap_vehicle_ss );
static JsonValue submap_construction = json_loader::from_string( submap_construction_ss );
static JsonValue submap_computer = json_loader::from_string( submap_computer_ss );
static JsonValue submap_cosmetic = json_loader::from_string( submap_cosmetic_ss );
static JsonValue submap_ter_furn_pre_migration = json_loader::from_string(
            submap_ter_furn_pre_migration_ss );
static JsonValue submap_fd_pre_migration = json_loader::from_string( submap_fd_pre_migration_ss );

static void load_from_jsin( submap &sm, const JsonValue &jsin )
{
    // Ensure that the JSON is up to date for our savegame version
    REQUIRE( savegame_version == 36 );
    int version = 0;
    JsonObject sm_json = jsin.get_object();
    if( sm_json.has_member( "version" ) ) {
        version = sm_json.get_int( "version" );
    }
    for( JsonMember member : sm_json ) {
        std::string name = member.name();
        if( name != "version" ) {
            sm.load( member, name, version );
        }
    }
}

struct submap_checks {
    bool terrain = true;
    bool furniture = true;
    bool traps = true;
    bool radiation = true;
    bool items = true;
    bool fields = true;
    bool cosmetics = true;
    bool spawns = true;
    bool vehicles = true;
    bool construction = true;
    bool computers = true;
};

//static const submap_checks dont_care;

static bool is_normal_submap( const submap &sm, submap_checks checks = {} )
{
    const bool terrain = checks.terrain;
    const bool furniture = checks.furniture;
    const bool traps = checks.traps;
    const bool radiation = checks.radiation;
    const bool items = checks.items;
    const bool fields = checks.fields;
    const bool cosmetics = checks.cosmetics;
    const bool spawns = checks.spawns;
    const bool vehicles = checks.vehicles;
    const bool construction = checks.construction;
    const bool computers = checks.computers;

    // For every point on the submap
    for( int y = 0; y < SEEY; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            if( terrain && sm.get_ter( { x, y } ) != ter_t_dirt ) {
                return false;
            }
            if( furniture && sm.get_furn( { x, y } ) != furn_str_id::NULL_ID() ) {
                return false;
            }
            if( traps && sm.get_trap( {x, y} ) != tr_null ) {
                return false;
            }
            if( radiation && sm.get_radiation( { x, y } ) != 0 ) {
                return false;
            }
            if( items && !sm.get_items( {x, y} ).empty() ) {
                return false;
            }
            if( fields && sm.get_field( { x, y } ).field_count() != 0 ) {
                return false;
            }
            if( computers && sm.has_computer( {x, y} ) ) {
                return false;
            }
        }
    }

    // Can be found without checking every point
    if( cosmetics && !sm.cosmetics.empty() ) {
        return false;
    }
    if( spawns && !sm.spawns.empty() ) {
        return false;
    }
    if( vehicles && !sm.vehicles.empty() ) {
        return false;
    }
    if( construction && !sm.partial_constructions.empty() ) {
        return false;
    }

    return true;
}

TEST_CASE( "submap_empty_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_empty );
    REQUIRE( is_normal_submap( sm ) );
}

TEST_CASE( "submap_terrain_rle_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_terrain_rle );
    submap_checks checks;
    checks.terrain = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const ter_id ter_nw = sm.get_ter( corner_nw );
    const ter_id ter_ne = sm.get_ter( corner_ne );
    const ter_id ter_sw = sm.get_ter( corner_sw );
    const ter_id ter_se = sm.get_ter( corner_se );

    // We placed a unique terrain in each of the corners. Check that those are correct
    INFO( string_format( "nw: %s", ter_nw.id().str() ) );
    INFO( string_format( "ne: %s", ter_ne.id().str() ) );
    INFO( string_format( "sw: %s", ter_sw.id().str() ) );
    INFO( string_format( "se: %s", ter_se.id().str() ) );
    // Require to prevent the lower CHECK from being spammy
    REQUIRE( ter_nw == ter_t_floor_green );
    REQUIRE( ter_ne == ter_t_floor_red );
    REQUIRE( ter_sw == ter_t_floor );
    REQUIRE( ter_se == ter_t_floor_blue );

    // And for the rest of the map, half of it is t_dirt, the other half t_rock_floor
    for( int x = 1; x < SEEX - 2; ++x ) {
        CHECK( sm.get_ter( { x, 0 } ) == ter_t_dirt );
    }
    for( int y = 1; y < SEEY / 2; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            CHECK( sm.get_ter( { x, y } ) == ter_t_dirt );
        }
    }
    for( int y = SEEY / 2; y < SEEY - 1; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            CHECK( sm.get_ter( { x, y } ) == ter_t_rock_floor );
        }

    }
    for( int x = 1; x < SEEX - 2; ++x ) {
        CHECK( sm.get_ter( { x, SEEY - 1 } ) == ter_t_rock_floor );
    }
}

TEST_CASE( "submap_terrain_load_invalid_ter_ids_as_t_dirt", "[submap][load]" )
{
    submap sm;
    const std::string error = capture_debugmsg_during( [&sm]() {
        load_from_jsin( sm, submap_terrain_with_invalid_ter_ids );
    } );
    submap_checks checks;
    checks.terrain = false;
    REQUIRE( is_normal_submap( sm, checks ) );
    REQUIRE( error == "invalid ter_str_id 't_this_ter_id_does_not_exist'" );

    //capture_debugmsg_during
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            CAPTURE( x, y );
            // expect t_rock_floor patch in a corner
            const ter_id expected = ( ( x == 11 ) && ( y == 11 ) ) ? ter_t_rock_floor : ter_t_dirt;
            CHECK( sm.get_ter( {x, y} ) == expected );
        }
    }
}

TEST_CASE( "submap_furniture_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_furniture );
    submap_checks checks;
    checks.furniture = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const furn_id furn_nw = sm.get_furn( corner_nw );
    const furn_id furn_ne = sm.get_furn( corner_ne );
    const furn_id furn_sw = sm.get_furn( corner_sw );
    const furn_id furn_se = sm.get_furn( corner_se );
    const furn_id furn_ra = sm.get_furn( random_pt );

    // We placed a unique furniture in a couple pf place. Check that those are correct
    INFO( string_format( "nw: %s", furn_nw.id().str() ) );
    INFO( string_format( "ne: %s", furn_ne.id().str() ) );
    INFO( string_format( "sw: %s", furn_sw.id().str() ) );
    INFO( string_format( "se: %s", furn_se.id().str() ) );
    INFO( string_format( "ra: %s", furn_ra.id().str() ) );
    // Require to prevent the lower CHECK from being spammy
    REQUIRE( furn_nw == furn_f_coffin_c );
    REQUIRE( furn_ne == furn_f_bookcase );
    REQUIRE( furn_sw == furn_f_dresser );
    REQUIRE( furn_se == furn_f_crate_o );
    REQUIRE( furn_ra == furn_f_gas_tank );

    // Also, check we have no other furniture
    for( int x = 0; x < SEEX; ++x ) {
        for( int y = 0; y < SEEY; ++y ) {
            point_sm_ms tested{ x, y };
            if( tested == corner_nw || tested == corner_ne || tested == corner_sw || tested == corner_se ||
                tested == random_pt ) {
                continue;
            }
            CHECK( sm.get_furn( tested ) == furn_str_id::NULL_ID() );
        }
    }
}

TEST_CASE( "submap_trap_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_trap );
    submap_checks checks;
    checks.traps = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const trap_id trap_nw = sm.get_trap( corner_nw );
    const trap_id trap_ne = sm.get_trap( corner_ne );
    const trap_id trap_sw = sm.get_trap( corner_sw );
    const trap_id trap_se = sm.get_trap( corner_se );
    const trap_id trap_ra = sm.get_trap( random_pt );

    // We placed a unique trap in a couple of places. Check that those are correct
    INFO( string_format( "nw: %s", trap_nw.id().str() ) );
    INFO( string_format( "ne: %s", trap_ne.id().str() ) );
    INFO( string_format( "sw: %s", trap_sw.id().str() ) );
    INFO( string_format( "se: %s", trap_se.id().str() ) );
    INFO( string_format( "ra: %s", trap_ra.id().str() ) );
    // Require to prevent the lower CHECK from being spammy
    REQUIRE( trap_nw == tr_rollmat );
    REQUIRE( trap_ne == tr_bubblewrap );
    REQUIRE( trap_sw == tr_beartrap );
    REQUIRE( trap_se == tr_funnel );
    REQUIRE( trap_ra == tr_landmine );

    // Also, check we have no other traps
    for( int x = 0; x < SEEX; ++x ) {
        for( int y = 0; y < SEEY; ++y ) {
            point_sm_ms tested{ x, y };
            if( tested == corner_nw || tested == corner_ne || tested == corner_sw || tested == corner_se ||
                tested == random_pt ) {
                continue;
            }
            CHECK( sm.get_trap( tested ) == tr_null );
        }
    }
}

TEST_CASE( "submap_rad_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_rad );
    submap_checks checks;
    checks.radiation = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const int rad_nw = sm.get_radiation( corner_nw );
    const int rad_ne = sm.get_radiation( corner_ne );
    const int rad_sw = sm.get_radiation( corner_sw );
    const int rad_se = sm.get_radiation( corner_se );
    const int rad_ra = sm.get_radiation( random_pt );

    // We placed a unique rad level in a couple of places. Check that those are correct
    INFO( string_format( "nw: %d", rad_nw ) );
    INFO( string_format( "ne: %d", rad_ne ) );
    INFO( string_format( "sw: %d", rad_sw ) );
    INFO( string_format( "se: %d", rad_se ) );
    INFO( string_format( "ra: %d", rad_ra ) );
    // Require to prevent the lower CHECK from being spammy
    REQUIRE( rad_nw == 2 );
    REQUIRE( rad_ne == 1 );
    REQUIRE( rad_sw == 4 );
    REQUIRE( rad_se == 3 );
    REQUIRE( rad_ra == 5 );

    std::array<int, SEEX> rads;
    // Also, check we have no other radiation
    INFO( "Below is the radiation on the row above and the current row.  Unknown values are -1" );
    for( int y = 0; y < SEEY; ++y ) {
        INFO( string_format( "%2d: %2d%2d%2d%2d%2d%2d%2d%2d%2d%2d%2d%2d", y - 1, rads[0], rads[1], rads[2],
                             rads[3], rads[4], rads[5], rads[6], rads[7], rads[8], rads[9], rads[10], rads[11] ) );
        for( int &rad : rads ) {
            rad = -1;
        }
        for( int x = 0; x < SEEX; ++x ) {
            point_sm_ms tested{ x, y };
            if( tested == corner_nw || tested == corner_ne || tested == corner_sw || tested == corner_se ||
                tested == random_pt ) {
                rads[x] = sm.get_radiation( tested );
                continue;
            }
            int fetched = sm.get_radiation( tested );
            rads[x] = fetched;
            INFO( string_format( "%2d: %2d%2d%2d%2d%2d%2d%2d%2d%2d%2d%2d%2d", y, rads[0], rads[1], rads[2],
                                 rads[3], rads[4], rads[5], rads[6], rads[7], rads[8], rads[9], rads[10], rads[11] ) );
            CHECK( fetched == 0 );
        }
    }
}

TEST_CASE( "submap_item_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_item );
    submap_checks checks;
    checks.items = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const cata::colony<item> &citem_nw = sm.get_items( corner_nw );
    const cata::colony<item> &citem_ne = sm.get_items( corner_ne );
    const cata::colony<item> &citem_sw = sm.get_items( corner_sw );
    const cata::colony<item> &citem_se = sm.get_items( corner_se );
    const cata::colony<item> &citem_ra = sm.get_items( random_pt );
    std::vector<itype_id> item_nw;
    std::vector<itype_id> item_ne;
    std::vector<itype_id> item_sw;
    std::vector<itype_id> item_se;
    std::vector<itype_id> item_ra;
    // We're just checking the ids, checking more gets quite complex
    // Only one of these needs to be a loop, I'm just doing all of them for simplicity/extensiblity
    for( const item &it : citem_nw ) {
        item_nw.push_back( it.typeId() );
    }
    for( const item &it : citem_ne ) {
        item_ne.push_back( it.typeId() );
    }
    for( const item &it : citem_sw ) {
        item_sw.push_back( it.typeId() );
    }
    for( const item &it : citem_se ) {
        item_se.push_back( it.typeId() );
    }
    for( const item &it : citem_ra ) {
        item_ra.push_back( it.typeId() );
    }

    // We placed a unique item in a couple of places. Check that those are correct
    INFO( string_format( "nw: %d %s %s", item_nw.size(), item_nw[0].str(), item_nw[1].str() ) );
    INFO( string_format( "ne: %d %s", item_ne.size(), item_ne[0].str() ) );
    INFO( string_format( "sw: %d %s", item_sw.size(), item_sw[0].str() ) );
    INFO( string_format( "se: %d %s", item_se.size(), item_se[0].str() ) );
    INFO( string_format( "ra: %d %s", item_ra.size(), item_ra[0].str() ) );
    // Require to prevent the lower CHECK from being spammy
    REQUIRE( item_nw[0] == itype_machete );
    REQUIRE( item_nw[1] == itype_foon );
    REQUIRE( item_ne[0] == itype_foodperson_mask );
    REQUIRE( item_sw[0] == itype_bottle_plastic );
    REQUIRE( item_se[0] == itype_bat_nerf );
    REQUIRE( item_ra[0] == itype_jackhammer );

    // Also, check we have no other items
    for( int y = 0; y < SEEY; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            point_sm_ms tested{ x, y };
            if( tested == corner_nw || tested == corner_ne || tested == corner_sw || tested == corner_se ||
                tested == random_pt ) {
                continue;
            }
            CHECK( sm.get_items( tested ).empty() );
        }
    }
}

TEST_CASE( "submap_field_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_field );
    submap_checks checks;
    checks.fields = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const field &field_nw = sm.get_field( corner_nw );
    const field &field_ne = sm.get_field( corner_ne );
    const field &field_sw = sm.get_field( corner_sw );
    const field &field_se = sm.get_field( corner_se );
    const field &field_ra = sm.get_field( random_pt );
    const field_entry *fd_nw = field_nw.find_field( field_fd_web );
    const field_entry *fd_ne = field_ne.find_field( field_fd_laser );
    const field_entry *fd_sw = field_sw.find_field( field_fd_electricity );
    const field_entry *fd_se = field_se.find_field( field_fd_acid );
    const field_entry *fd_ra = field_ra.find_field( field_fd_nuke_gas );
    const field_entry *fd_ow = field_nw.find_field( field_fd_smoke );
    // No nullptrs for me
    REQUIRE( fd_nw != nullptr );
    REQUIRE( fd_ow != nullptr );
    REQUIRE( fd_ne != nullptr );
    REQUIRE( fd_sw != nullptr );
    REQUIRE( fd_se != nullptr );
    REQUIRE( fd_ra != nullptr );

    // We placed a unique item in a couple of places. Check that those are correct
    INFO( string_format( "nw: %d %s %d %d %s %d %d", field_nw.field_count(),
                         fd_nw->get_field_type().id().str(), fd_nw->get_field_intensity(),
                         to_turns<int>( fd_nw->get_field_age() ), fd_ow->get_field_type().id().str(),
                         fd_ow->get_field_intensity(),
                         to_turns<int>( fd_ow->get_field_age() ) ) );
    INFO( string_format( "ne: %d %s %d %d", field_ne.field_count(),
                         fd_ne->get_field_type().id().str(), fd_ne->get_field_intensity(),
                         to_turns<int>( fd_ne->get_field_age() ) ) );
    INFO( string_format( "sw: %d %s %d %d", field_sw.field_count(),
                         fd_sw->get_field_type().id().str(), fd_sw->get_field_intensity(),
                         to_turns<int>( fd_sw->get_field_age() ) ) );
    INFO( string_format( "se: %d %s %d %d", field_se.field_count(),
                         fd_se->get_field_type().id().str(), fd_se->get_field_intensity(),
                         to_turns<int>( fd_se->get_field_age() ) ) );
    INFO( string_format( "ra: %d %s %d %d", field_ra.field_count(),
                         fd_ra->get_field_type().id().str(), fd_ra->get_field_intensity(),
                         to_turns<int>( fd_ra->get_field_age() ) ) );
    // Require to prevent the lower CHECK from being spammy
    REQUIRE( fd_nw->get_field_intensity() == 3 );
    REQUIRE( fd_nw->get_field_age() == 2077_seconds );
    REQUIRE( fd_ow->get_field_intensity() == 4 );
    REQUIRE( fd_ow->get_field_age() == 3004_seconds );
    REQUIRE( fd_ne->get_field_intensity() == 1 );
    REQUIRE( fd_ne->get_field_age() == 1997_seconds );
    REQUIRE( fd_sw->get_field_intensity() == 5 );
    REQUIRE( fd_sw->get_field_age() == 1482_seconds );
    REQUIRE( fd_se->get_field_intensity() == 2 );
    REQUIRE( fd_se->get_field_age() == 2003_seconds );
    REQUIRE( fd_ra->get_field_intensity() == 6 );
    REQUIRE( fd_ra->get_field_age() == 1615_seconds );

    // Also, check we have no other fields
    for( int y = 0; y < SEEY; ++y ) {
        for( int x = 0; x < SEEX; ++x ) {
            point_sm_ms tested{ x, y };
            if( tested == corner_nw || tested == corner_ne || tested == corner_sw || tested == corner_se ||
                tested == random_pt ) {
                continue;
            }
            CHECK( sm.get_field( tested ).field_count() == 0 );
        }
    }
}

TEST_CASE( "submap_graffiti_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_graffiti );
    submap_checks checks;
    checks.cosmetics = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const std::string &g_nw = sm.get_graffiti( corner_nw );
    const std::string &g_ne = sm.get_graffiti( corner_ne );
    const std::string &g_sw = sm.get_graffiti( corner_sw );
    const std::string &g_se = sm.get_graffiti( corner_se );
    const std::string &g_ra = sm.get_graffiti( random_pt );

    // We placed a unique graffiti in a couple of places. Check that those are correct
    INFO( string_format( "nw: %s", g_nw ) );
    INFO( string_format( "ne: %s", g_ne ) );
    INFO( string_format( "sw: %s", g_sw ) );
    INFO( string_format( "se: %s", g_se ) );
    INFO( string_format( "ra: %s", g_ra ) );
    // Require to prevent the lower CHECK from being spammy
    REQUIRE( g_nw == "c" );
    REQUIRE( g_ne == "a" );
    REQUIRE( g_sw == "d" );
    REQUIRE( g_se == "b" );
    REQUIRE( g_ra == "e" );

    // Also, check we have no other graffiti
    REQUIRE( sm.cosmetics.size() == 5 );
}

TEST_CASE( "submap_cosmetics_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_cosmetic );
    submap_checks checks;
    checks.cosmetics = false;
    // Signs require furniture
    checks.furniture = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const std::string &g_nw = sm.get_graffiti( corner_nw );
    const std::string &g_ne = sm.get_graffiti( corner_ne );
    const std::string &g_sw = sm.get_signage( corner_sw );
    const std::string &g_se = sm.get_signage( corner_se );
    const std::string &g_ra = sm.get_graffiti( random_pt );

    // We placed a unique graffiti in a couple of places. Check that those are correct
    INFO( string_format( "nw: %s", g_nw ) );
    INFO( string_format( "ne: %s", g_ne ) );
    INFO( string_format( "sw: %s", g_sw ) );
    INFO( string_format( "se: %s", g_se ) );
    INFO( string_format( "ra: %s", g_ra ) );
    // Require to prevent the lower CHECK from being spammy
    REQUIRE( g_nw == "I <3 Dr. Hylke van der Schaaf." );
    REQUIRE( g_ne == "This is written text." );
    REQUIRE( g_sw == "This is a sign" );
    REQUIRE( g_se == "Subway Map: illegible city name stop" );
    REQUIRE( g_ra == "Santina is a heteronormative bully!" );

    // Also, check we have no other cosmetics
    REQUIRE( sm.cosmetics.size() == 5 );
}

TEST_CASE( "submap_spawns_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_spawns );
    submap_checks checks;
    checks.spawns = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const spawn_point &nw = *std::find_if( sm.spawns.begin(),
    sm.spawns.end(), []( const spawn_point & spawn ) {
        return spawn.pos == corner_nw;
    } );
    const spawn_point &ne = *std::find_if( sm.spawns.begin(),
    sm.spawns.end(), []( const spawn_point & spawn ) {
        return spawn.pos == corner_ne;
    } );
    const spawn_point &sw = *std::find_if( sm.spawns.begin(),
    sm.spawns.end(), []( const spawn_point & spawn ) {
        return spawn.pos == corner_sw;
    } );
    const spawn_point &se = *std::find_if( sm.spawns.begin(),
    sm.spawns.end(), []( const spawn_point & spawn ) {
        return spawn.pos == corner_se;
    } );
    const spawn_point &ra = *std::find_if( sm.spawns.begin(),
    sm.spawns.end(), []( const spawn_point & spawn ) {
        return spawn.pos == random_pt;
    } );

    // We placed a unique spawn in a couple of places. Check that those are correct
    INFO( string_format( "nw: [%d, %d] %d %s %s %s", nw.pos.x(), nw.pos.y(), nw.count, nw.type.str(),
                         nw.friendly ? "friendly" : "hostile", nw.name.value_or( "NONE" ) ) );
    INFO( string_format( "ne: [%d, %d] %d %s %s %s", ne.pos.x(), ne.pos.y(), ne.count, ne.type.str(),
                         ne.friendly ? "friendly" : "hostile", ne.name.value_or( "NONE" ) ) );
    INFO( string_format( "sw: [%d, %d] %d %s %s %s", sw.pos.x(), sw.pos.y(), sw.count, sw.type.str(),
                         sw.friendly ? "friendly" : "hostile", sw.name.value_or( "NONE" ) ) );
    INFO( string_format( "se: [%d, %d] %d %s %s %s", se.pos.x(), se.pos.y(), se.count, se.type.str(),
                         se.friendly ? "friendly" : "hostile", se.name.value_or( "NONE" ) ) );
    INFO( string_format( "ra: [%d, %d] %d %s %s %s", ra.pos.x(), ra.pos.y(), ra.count, ra.type.str(),
                         ra.friendly ? "friendly" : "hostile", ra.name.value_or( "NONE" ) ) );
    // Require to prevent the lower CHECK from being spammy
    CHECK( nw.count == 3 );
    CHECK( nw.type.str() == "mon_fish_eel" );
    CHECK( !nw.friendly );
    CHECK( nw.name.value_or( "NONE" ) == "Bob" );
    CHECK( ne.count == 1 );
    CHECK( ne.type.str() == "mon_pheasant" );
    CHECK( !ne.friendly );
    CHECK( ne.name.value_or( "NONE" ) == "NONE" );
    CHECK( sw.count == 4 );
    CHECK( sw.type.str() == "mon_zombie_fungus" );
    CHECK( !sw.friendly );
    CHECK( sw.name.value_or( "NONE" ) == "Hopper" );
    CHECK( se.count == 2 );
    CHECK( se.type.str() == "mon_mininuke_hack" );
    CHECK( se.friendly );
    CHECK( se.name.value_or( "NONE" ) == "Tim" );
    CHECK( ra.count == 5 );
    CHECK( ra.type.str() == "mon_plague_vector" );
    CHECK( ra.friendly );
    CHECK( ra.name.value_or( "NONE" ) == "Alice" );

    // Also, check we have no other spawns
    CHECK( sm.spawns.size() == 5 );
}

TEST_CASE( "submap_vehicle_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_vehicle );
    submap_checks checks;
    checks.vehicles = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    REQUIRE( sm.vehicles.size() == 1 );
    // There's a lot we can test here, but that's getting into vehicle testing, not submap
    CHECK( sm.vehicles[0]->disp_name() == "the Welding Cart" );
}

TEST_CASE( "submap_construction_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_construction );
    submap_checks checks;
    checks.construction = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    REQUIRE( sm.partial_constructions.size() == 2 );
    REQUIRE( sm.partial_constructions.find( { 3, 2, 0 } ) != sm.partial_constructions.end() );
    const partial_con &con1 = sm.partial_constructions[ {3, 2, 0}];
    CHECK( con1.counter == 123334 );
    CHECK( con1.components.size() == 1 );
    CHECK( con1.id.id() == construction_constr_ground_cable );
    REQUIRE( sm.partial_constructions.find( { 3, 3, 0 } ) != sm.partial_constructions.end() );
    const partial_con &con2 = sm.partial_constructions[ {3, 3, 0}];
    CHECK( con2.counter == 4934 );
    CHECK( con2.components.size() == 4 );
    CHECK( con2.id.id() == construction_constr_rack_coat );
}

TEST_CASE( "submap_computer_load", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_computer );
    submap_checks checks;
    checks.computers = false;

    REQUIRE( is_normal_submap( sm, checks ) );
    // Just check there are computers in the right place
    // Checking more is complicated
    REQUIRE( sm.has_computer( point_sm_ms( point::south ) ) );
    REQUIRE( sm.has_computer( {3, 5} ) );
}

TEST_CASE( "submap_ter_furn_migration", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_ter_furn_pre_migration );
    submap_checks checks;
    checks.terrain = false;
    checks.furniture = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const ter_id ter_nw = sm.get_ter( corner_nw );
    const ter_id ter_ne = sm.get_ter( corner_ne );
    const ter_id ter_sw = sm.get_ter( corner_sw );
    const ter_id ter_se = sm.get_ter( corner_se );

    const furn_id furn_nw = sm.get_furn( corner_nw );
    const furn_id furn_ne = sm.get_furn( corner_ne );
    const furn_id furn_sw = sm.get_furn( corner_sw );
    const furn_id furn_se = sm.get_furn( corner_se );

    // North corners should have migrated from test_t_migration_old_id to test_t_migration_new_id
    INFO( string_format( "ter nw: %s", ter_nw.id().str() ) );
    INFO( string_format( "ter ne: %s", ter_ne.id().str() ) );
    REQUIRE( ter_nw == ter_test_t_migration_new_id );
    REQUIRE( ter_ne == ter_test_t_migration_new_id );
    // West one could still have a f_bookcase as it may override the test_f_migration_new_id placed by migration, as the load order for the json members isn't defined
    INFO( string_format( "furn nw: %s", furn_nw.id().str() ) );
    REQUIRE( ( furn_nw == furn_f_bookcase || furn_nw == furn_test_f_migration_new_id ) );
    // East one should now have test_f_migration_new_id as per the migration
    INFO( string_format( "furn ne: %s", furn_ne.id().str() ) );
    REQUIRE( furn_ne == furn_test_f_migration_new_id );
    // South corners should still have t_dirt
    INFO( string_format( "ter sw: %s", ter_sw.id().str() ) );
    INFO( string_format( "ter se: %s", ter_se.id().str() ) );
    REQUIRE( ter_sw == ter_t_dirt );
    REQUIRE( ter_se == ter_t_dirt );
    // But should have migrated test_f_migration_old_id to test_f_migration_new_id
    INFO( string_format( "furn sw: %s", furn_sw.id().str() ) );
    INFO( string_format( "furn se: %s", furn_se.id().str() ) );
    REQUIRE( furn_sw == furn_test_f_migration_new_id );
    REQUIRE( furn_se == furn_test_f_migration_new_id );
}

TEST_CASE( "submap_fd_migration", "[submap][load]" )
{
    submap sm;
    load_from_jsin( sm, submap_fd_pre_migration );
    submap_checks checks;
    checks.fields = false;

    REQUIRE( is_normal_submap( sm, checks ) );

    const field field_ne = sm.get_field( corner_ne );

    // North east corner should have migrated from test_fd_migration_old_id to test_fd_migration_new_id
    std::string fields_list;
    bool found_field_new_id = false;
    int total_fields = 0;
    auto it = field_ne.begin();
    while( it != field_ne.end() ) {
        const field_type_id &type = it->first;
        fields_list += type.id().str() + " ";
        found_field_new_id |= type == field_test_fd_migration_new_id;
        total_fields++;
        it++;
    }
    INFO( string_format( "%d fields found: %s", total_fields, fields_list ) );
    REQUIRE( ( found_field_new_id && total_fields == 1 ) );
}
