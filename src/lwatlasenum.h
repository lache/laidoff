#pragma once

#include "lwmacro.h"

typedef struct _LWCONTEXT LWCONTEXT;

typedef enum _LW_ATLAS_ENUM {
    LAE_TWIRL_PNG,
    LAE_C2_PNG,
    LAE_TWIRL_XXX_PNG,
    LAE_BG_ROAD_PNG,

    LAE_BG_KITCHEN,
    LAE_BG_MART_IN,
    LAE_BG_MART_OUT,
    LAE_BG_ROAD,
    LAE_BG_ROOM,
    LAE_BG_ROOM_CEILING,

    LAE_P_DOHEE,
    LAE_P_DOHEE_ALPHA,
    LAE_P_MOTHER,
    LAE_P_MOTHER_ALPHA,

    LAE_U_DIALOG_BALLOON,

    LAE_BG_KITCHEN_KTX,
    LAE_BG_MART_IN_KTX,
    LAE_BG_MART_OUT_KTX,
    LAE_BG_ROAD_KTX,
    LAE_BG_ROOM_KTX,
    LAE_BG_ROOM_CEILING_KTX,

    LAE_FX_TRAIL,
    LAE_FX_TRAIL_ALPHA,

    LAE_U_GLOW,
    LAE_U_GLOW_ALPHA,

    LAE_U_ENEMY_SCOPE_KTX,
    LAE_U_ENEMY_SCOPE_ALPHA_KTX,
    LAE_U_ENEMY_TURN_KTX,
    LAE_U_ENEMY_TURN_ALPHA_KTX,
    LAE_U_PLAYER_TURN_KTX,
    LAE_U_PLAYER_TURN_ALPHA_KTX,
    LAE_U_FIST_ICON_KTX,
    LAE_U_FIST_ICON_ALPHA_KTX,

    LAE_C_BIKER_KTX,
    LAE_C_BIKER_ALPHA_KTX,
    LAE_C_MADAM_KTX,
    LAE_C_MADAM_ALPHA_KTX,
    LAE_C_SPEAKER_KTX,
    LAE_C_SPEAKER_ALPHA_KTX,
    LAE_C_TOFU_KTX,
    LAE_C_TOFU_ALPHA_KTX,
    LAE_C_TREE_KTX,
    LAE_C_TREE_ALPHA_KTX,

    LAE_3D_PLAYER_TEX_KTX,
    LAE_3D_FLOOR_TEX_KTX,
    LAE_3D_FLOOR2_TEX_KTX,
    LAE_3D_APT_TEX_MIP_KTX,
    LAE_3D_OIL_TRUCK_TEX_KTX,
    LAE_3D_ROOM_TEX_KTX,
    LAE_3D_BATTLEGROUND_FLOOR_BAKE_TEX_KTX,
    LAE_3D_BATTLEGROUND_WALL_BAKE_TEX_KTX,
    LAE_SPIRAL_KTX,
    LAE_GUNTOWER_KTX,
    LAE_TURRET_KTX,
    LAE_CROSSBOW_KTX,
    LAE_CATAPULT_KTX,
    LAE_PYRO_KTX,
    LAE_DEVIL_KTX,
    LAE_CRYSTAL_KTX,
    LAE_CIRCLE_SHADOW_KTX,
    LAE_PUCK_KTX,
    LAE_PUCK_GRAY_KTX,
    LAE_PUCK_ENEMY_KTX,
    LAE_PUCK_PLAYER_KTX,
    LAE_PUCK_FLOOR_KTX,

    LAE_BB_CATAPULT,
    LAE_BB_CROSSBOW,
    LAE_BB_GUNTOWER,
    LAE_BB_PYRO,
    LAE_BB_TURRET,

    LAE_SPLASH512512,

    LAE_BEAM_KTX,

    LAE_BUTTON_PULL,
    LAE_BUTTON_PULL_ALPHA,
    LAE_BUTTON_DASH,
    LAE_BUTTON_DASH_ALPHA,
    LAE_BUTTON_JUMP,
    LAE_BUTTON_JUMP_ALPHA,

    LAE_ARROW,
    LAE_RADIALWAVE,
    LAE_LINEARWAVE,
    LAE_JOYSTICKAREA,
    LAE_JOYSTICK,
    LAE_PHYSICS_MENU,
    LAE_UI_CAUTION_POPUP,
    LAE_UI_CAUTION_POPUP_ALPHA,
    LAE_UI_MAIN_MENU_KO,
    LAE_UI_MAIN_MENU_EN,
    LAE_UI_BACK_BUTTON,
    LAE_RESULT_TITLE_ATLAS,
    LAE_RESULT_TITLE_ATLAS_ALPHA,
    LAE_PREPARE_TITLE_ATLAS,
    LAE_PREPARE_TITLE_ATLAS_ALPHA,
    LAE_ENERGY_ICON,
    LAE_ENERGY_ICON_ALPHA,
    LAE_RANK_ICON,
    LAE_RANK_ICON_ALPHA,
    LAE_PROFILE_ICON,
    LAE_HP_STAR_0,
    LAE_HP_STAR_0_ALPHA,
    LAE_HP_STAR_1,
    LAE_HP_STAR_1_ALPHA,
    LAE_HP_STAR_2,
    LAE_HP_STAR_2_ALPHA,
    LAE_HP_STAR_3,
    LAE_HP_STAR_3_ALPHA,
    LAE_HP_STAR_4,
    LAE_HP_STAR_4_ALPHA,
    LAE_HP_STAR_5,
    LAE_HP_STAR_5_ALPHA,
    LAE_PUCK_FLOOR_COVER,
    LAE_PUCK_FLOOR_COVER_OCTAGON,
    LAE_TOWER_BASE_2_PLAYER,
    LAE_TOWER_BASE_2_TARGET,
    LAE_EXCLAMATION_MARK,
    LAE_EXCLAMATION_MARK_ALPHA,
    LAE_STOP_MARK,
    LAE_STOP_MARK_ALPHA,
    LAE_UI_BUTTON_ATLAS,
    LAE_FOOTBALL_GROUND,
    LAE_PLAYER_CAPSULE,
    LAE_WAVE,
    LAE_WAVE_ALPHA,
    LAE_TTL_TITLE,
    LAE_TTL_TITLE_ALPHA,
    LAE_WORLD_MAP,
    LAE_SLICE_TEST,
    LAE_FLAGS_MINI0,
    LAE_FLAGS_MINI0_ALPHA,
    LAE_FLAGS_MINI1,
    LAE_FLAGS_MINI1_ALPHA,
    LAE_FLAGS_MINI2,
    LAE_FLAGS_MINI2_ALPHA,
    LAE_WATER_2048_1024_AA,
    LAE_WATER_SAND_TILE,
    
    LAE_TTL_CITY_ALPHA,
    LAE_TTL_CITY,
    LAE_TTL_CONTAINER_BLUE_ALPHA,
    LAE_TTL_CONTAINER_BLUE,
    LAE_TTL_CONTAINER_GREEN_ALPHA,
    LAE_TTL_CONTAINER_GREEN,
    LAE_TTL_CONTAINER_ORANGE_ALPHA,
    LAE_TTL_CONTAINER_ORANGE,
    LAE_TTL_CONTAINER_WHITE_ALPHA,
    LAE_TTL_CONTAINER_WHITE,
    LAE_TTL_DOTS_ALPHA,
    LAE_TTL_DOTS,
    LAE_TTL_MARK_ALPHA,
    LAE_TTL_MARK,
    LAE_TTL_PORT_ALPHA,
    LAE_TTL_PORT,
    
    LAE_COUNT,

    LAE_DONTCARE,
    LAE_ZERO_FOR_BLACK,
} LW_ATLAS_ENUM;

static const char *tex_atlas_filename[] = {
    ASSETS_BASE_PATH "tex" PATH_SEPARATOR "Twirl.png",
    ASSETS_BASE_PATH "tex" PATH_SEPARATOR "atlas01.png",
    ASSETS_BASE_PATH "tex" PATH_SEPARATOR "Twirl.png",
    ASSETS_BASE_PATH "tex" PATH_SEPARATOR "bg-road.png",


    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "bg-kitchen.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "bg-mart-in.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "bg-mart-out.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "bg-road.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "bg-room.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "bg-room-ceiling.pkm",


    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "p-dohee.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "p-dohee_alpha.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "p-mother.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "p-mother_alpha.pkm",


    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "u-dialog-balloon.pkm",

    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-kitchen-mip.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-mart-in-mip.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-mart-out-mip.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-road-mip.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-room-mip.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-room-ceiling-mip.ktx",

    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "fx-trail.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "fx-trail_alpha.pkm",

    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "u-glow.pkm",
    ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "u-glow_alpha.pkm",

    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-enemy-scope-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-enemy-scope-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-enemy-turn-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-enemy-turn-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-player-turn-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-player-turn-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-fist-icon-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-fist-icon-a_alpha.ktx",


    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-biker-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-biker-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-madam-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-madam-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-speaker-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-speaker-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-tofu-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-tofu-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-tree-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "c-tree-a_alpha.ktx",

    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "3d-player-tex.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "3d-floor-tex.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "3d-floor2-tex.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "3d-apt-tex-mip.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "3d-oil-truck-tex.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "3d-room-tex.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "3d-battleground-floor-bake-tex.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "3d-battleground-wall-bake-tex.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "spiral.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "guntower.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "turret.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "crossbow.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "catapult.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "pyro.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "devil.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "crystal.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "circle-shadow.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "puck.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "puck-gray.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "puck-enemy.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "puck-player.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "puck-floor.ktx",


    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bb-catapult.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bb-crossbow.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bb-guntower.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bb-pyro.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bb-turret.ktx",

    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "splash512512.ktx",

    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "fx-beam.ktx",

    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "button-pull-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "button-pull-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "button-dash-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "button-dash-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "button-jump-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "button-jump-a_alpha.ktx",

    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "arrow.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "radialwave.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "linearwave.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "joystickarea.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "joystick.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "physics-menu.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ui-caution-popup-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ui-caution-popup-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ui-main-menu-ko.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ui-main-menu-en.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ui-back-button.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "result-title-atlas-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "result-title-atlas-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "prepare-title-atlas-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "prepare-title-atlas-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "energy-icon-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "energy-icon-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "rank-icon-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "rank-icon-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "profile-icon.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar0-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar0-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar1-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar1-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar2-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar2-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar3-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar3-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar4-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar4-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar5-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "hpstar5-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "puck-floor-cover.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "puck-floor-cover-octagon.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "tower-base-2-player.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "tower-base-2-target.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "exclamation-mark-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "exclamation-mark-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "stop-mark-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "stop-mark-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ui-button-atlas.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "football-ground.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "player-capsule.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "wave-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "wave-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-title-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-title-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "world-map.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "slice-test.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "flags-mini0-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "flags-mini0-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "flags-mini1-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "flags-mini1-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "flags-mini2-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "flags-mini2-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "water_2048x1024_aa.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "water-sand-tile.ktx",
    
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-city-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-city-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-container-blue-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-container-blue-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-container-green-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-container-green-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-container-orange-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-container-orange-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-container-white-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-container-white-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-dots-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-dots-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-mark-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-mark-a.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-port-a_alpha.ktx",
    ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "ttl-port-a.ktx",
};

#define MAX_TEX_ATLAS LAE_COUNT

LwStaticAssert(ARRAY_SIZE(tex_atlas_filename) == LAE_COUNT, "LAE_COUNT error");

#ifdef __cplusplus
extern "C" {
#endif
    void lw_load_tex(const LWCONTEXT* _pLwc, int lae);
    void lazy_tex_atlas_glBindTexture(const LWCONTEXT* _pLwc, int lae);
    void lw_load_tex_async(const LWCONTEXT* _pLwc, int lae);
    void lw_calculate_all_tex_atlas_hash(LWCONTEXT* pLwc);
#ifdef __cplusplus
}
#endif
