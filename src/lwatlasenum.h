#pragma once

typedef enum _LW_ATLAS_ENUM
{
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
    
    LAE_COUNT,
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

	ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-kitchen.ktx",
	ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-mart-in.ktx",
	ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-mart-out.ktx",
	ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-road.ktx",
	ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-room.ktx",
	ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "bg-room-ceiling.ktx",

	ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "fx-trail.pkm",
	ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "fx-trail_alpha.pkm",

	ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "u-glow.pkm",
	ASSETS_BASE_PATH "pkm" PATH_SEPARATOR "u-glow_alpha.pkm",

	ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-enemy-scope-a.ktx",
	ASSETS_BASE_PATH "ktx" PATH_SEPARATOR "u-enemy-scope-a_alpha.ktx",
};

#define MAX_TEX_ATLAS (ARRAY_SIZE(tex_atlas_filename))
LwStaticAssert(ARRAY_SIZE(tex_atlas_filename) == LAE_COUNT, "LAE_COUNT error");
