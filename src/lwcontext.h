#pragma once

#include <time.h> // ios struct timespec

#include "lwgl.h"
#include "laidoff.h"
#include "linmath.h"
#include "sprite_data.h"
#include "vertices.h"
#include "lwdamagetext.h"
#include "lwbattlecreature.h"
#include "lwenemy.h"
#include "lwshader.h"
#include "lwvbo.h"
#include "lwvbotype.h"
#include "lwmacro.h"
#include "lwatlasenum.h"

#define MAX_SHADER (3)
#define MAX_BAR_SPAWN_COUNT (374) // ((int)(5 * 60 * 1.0/SCROLL_SPEED))
#define MAX_BAR_COUNT (5) // bar pool size (not the maximum spawning bar count)
#define MAX_ANIM_COUNT (10)
#define FIRST_BAR_SPAWN_WAIT_TIME (3)
#define ANIM_FPS (60)
#define MAX_HEART (5)
#define COMPLETION_TARGET_SCORE_WAIT_TIME (1)
#define TODAY_PLAYING_LIMIT_COUNT_NO_LIMIT (-1)
#define MAX_TOUCHPROC_COUNT (10)
#define NORMALIZED_SCREEN_RESOLUTION_X (2.f)
#define NORMALIZED_SCREEN_RESOLUTION_Y NORMALIZED_SCREEN_RESOLUTION_X


LwStaticAssert(ARRAY_SIZE(SPRITE_DATA[0]) == LAS_COUNT, "LAS_COUNT error");


static const char* tex_font_atlas_filename[] =
{
    //ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold_0.tga",
    //ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold_1.tga",
	ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test6_0.tga",
	ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test6_1.tga",
};

#define MAX_TEX_FONT_ATLAS (ARRAY_SIZE(tex_font_atlas_filename))

static const char *tex_atlas_filename[] =
        {
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


typedef enum
{
	LPT_GRID,
	LPT_SOLID_RED,
	LPT_SOLID_GREEN,
	LPT_SOLID_BLUE,
	LPT_SOLID_GRAY,
	LPT_SOLID_BLACK,
	LPT_SOLID_YELLOW,
	LPT_SOLID_TRANSPARENT, // testing
	LPT_SOLID_WHITE_WITH_ALPHA, // testing
	LPT_DIR_PAD,
	LPT_BOTH_END_GRADIENT_HORIZONTAL,
	
	LPT_COUNT,
} LW_PROGRAMMED_TEX;
#define MAX_TEX_PROGRAMMED LPT_COUNT

LwStaticAssert(ARRAY_SIZE(tex_atlas_filename) == LAE_COUNT, "LAE_COUNT error");


typedef enum _LW_BATTLE_STATE
{
    LBS_SELECT_COMMAND,
    LBS_SELECT_TARGET,
    LBS_COMMAND_IN_PROGRESS,
} LW_BATTLE_STATE;

typedef struct
{
    float angle;
    float x;
    float dx;
    float y;
    float w;
    float h;
    float frame_interval;
    float frame_age;
    int frame;
    mat4x4 mvp;
    int enable_title_anim;
    float title_anim_amplitude;
    float title_anim_interval;
    float vel_y;
    float acc_y;
    float acc_y_touch;
    float vel_y_limit;
    float acc_y_grav;
    float impulse_y_touch;
    float target_angle;
    float angle_min;
    float angle_max;
    float rot_up_speed;
    float rot_down_speed;
    float vel_y_upside;
    float acc_y_touch_bound; // Kiwi cannot touch-accelerate if kiwi.y is greater than this value.
    int stop_anim;
    int dead;
    double last_apply_impulse_app_time;
    int sunburst;
    float y_at_finish;
} LWKIWI;

typedef struct
{
    float x;
    float y;
    float ground_level_y;
    float w;
    float h;
    // for scroll
    mat4x4 mvp1;
    mat4x4 mvp2;
} LWGROUND;

typedef struct
{
    float x;
    float y;
    float w;
    float h;
    // for scroll
    mat4x4 mvp1;
    mat4x4 mvp2;
} LWCLOUD;

typedef struct
{
    int valid;
    float x;
    float center_y;
    float y1;
    float y2;
    float w;
    float h;
    int scored;

    mat4x4 mvp1; // upper
    mat4x4 mvp2; // lower
} LWBAR;

typedef enum
{
	LGS_BATTLE,
    LGS_FIELD,
    LGS_DIALOG,
	LGS_FONT_TEST,

    LGS_INVALID,
} LW_GAME_SCENE;

typedef struct
{
    int frame;
    float x;
    float y;
    float alpha;
} LWKEYFRAME;

typedef struct _LWCONTEXT LWCONTEXT;

typedef void(*custom_render_proc)(const LWCONTEXT*, float, float, float);
typedef void(*anim_finalized_proc)(LWCONTEXT*);
typedef void(*touch_proc)(LWCONTEXT*);

typedef struct
{
    int valid;
    int las;
    float elapsed;
    float fps;
    const LWKEYFRAME* animdata;
    int animdata_length;
    //int current_animdata_index;
    mat4x4 mvp;
    float x;
    float y;
    float alpha;
    custom_render_proc custom_render_proc_callback;
    anim_finalized_proc anim_finalized_proc_callback;
    int finalized;
} LWANIM;


typedef struct
{
    int valid;
    float x, y, w, h;
    touch_proc touch_proc_callback;
} LWTOUCHPROC;

typedef struct
{
	int valid;
	float x, y;
	float w, h;
} LWBOX2DCOLLIDER;

typedef struct
{
	int valid;
	float x, y;
	float sx, sy;
	enum _LW_VBO_TYPE lvt;
	GLuint tex_id;
} LWFIELDOBJECT;

#define MAX_FIELD_OBJECT (32)
#define MAX_BOX_COLLIDER MAX_FIELD_OBJECT


typedef struct
{
	int valid;
	float x, y, z;
	float age;
	float max_age;
	float tex_coord_speed;
	float tex_coord;
} LWTRAIL;

#define MAX_TRAIL (16)

#define SET_COLOR_RGBA_FLOAT(v, r, g, b, a) \
	v[0] = r; \
	v[1] = g; \
	v[2] = b; \
	v[3] = a;

#define MAX_DAMAGE_TEXT (16)

#define MAX_BATTLE_CREATURE (4)

#define MAX_ENEMY_SLOT (5)

typedef unsigned int UnicodeChar;

typedef struct _LWFBO {
	GLuint fbo;
	GLuint depth_render_buffer;
	GLuint color_tex;
	int dirty;
} LWFBO;

typedef struct _LWCONTEXT
{
    struct GLFWwindow* window;

    int width, height;

    LWSHADER shader[MAX_SHADER];

#define VERTEX_BUFFER_COUNT LVT_COUNT
    // index 0: center anchored square
    // index 1: vbo 1
    // index 2: vbo 2
    // index 3: left top anchored square (text rendering)
    // index 4: center bottom anchored square (dialog balloon rendering)
    LWVBO vertex_buffer[VERTEX_BUFFER_COUNT];
    //GLuint index_buffer;
	GLuint vao[VERTEX_BUFFER_COUNT];

    GLuint tex_atlas[MAX_TEX_ATLAS];
	int tex_atlas_width[MAX_TEX_ATLAS];
	int tex_atlas_height[MAX_TEX_ATLAS];
    GLuint tex_font_atlas[MAX_TEX_FONT_ATLAS];
	unsigned long tex_atlas_hash[MAX_TEX_ATLAS];
	GLuint tex_programmed[MAX_TEX_PROGRAMMED];

    int rotate;
    mat4x4 mvp;

#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    double last_time;
#else
    struct timespec last_time;
#endif

    double delta_time;

    LWKIWI kiwi;
    LWGROUND ground;
    LWCLOUD cloud;
    float ground_scroll_speed;
    float cloud_scroll_speed;
    mat4x4 proj;
    float black_curtain_alpha;
    float white_curtain_alpha;
    float curtain_alpha_speed;

    LW_GAME_SCENE game_scene;
    LW_GAME_SCENE next_game_scene;
    int score;
    int target_score;
    float render_score;
    double completion_target_score_waited_time;
    float increase_render_score;
    int highscore;

    float ready_alpha;
    float ready_alpha_speed;

    LWBAR bar[MAX_BAR_COUNT];
    float bar_dist;
    float stage_playing_duration;
    float scene_duration;
    int bar_spawn_count;
    int max_bar_spawn_count;
    double last_bar_pass_time;

    LWANIM anim[MAX_ANIM_COUNT];
    LWTOUCHPROC touch_proc[MAX_TOUCHPROC_COUNT];

    int update_count;
    int render_count;

    double app_time;

    int completed;
    int current_heart;
    int max_heart;

    int tex_atlas_index;
    LWSPRITE* sprite_data;

    int selected_command_slot;

    int selected_enemy_slot;
    LWENEMY enemy[MAX_ENEMY_SLOT];
    LW_BATTLE_STATE battle_state;
	LWANIM1D command_in_progress_anim;

    void* pFnt;

    const char* dialog;
    //UnicodeChar dialog_wide[1024 * 10];
    int dialog_bytelen;
    int render_char;
    double last_render_char_incr_time;
    int dialog_line;
    int dialog_start_index;
    int dialog_next_index;
    int dialog_move_next;
	int dialog_bg_tex_index;
	int dialog_portrait_tex_index;

	int player_move_left;
	int player_move_right;
	int player_move_up;
	int player_move_down;

	float player_pos_x;
	float player_pos_y;

	LWBOX2DCOLLIDER box_collider[MAX_BOX_COLLIDER];
	LWFIELDOBJECT field_object[MAX_FIELD_OBJECT];

	float dir_pad_x;
	float dir_pad_y;
	int dir_pad_dragging;

	LWTRAIL trail[MAX_TRAIL];
	float battle_fov_deg;
	float battle_fov_deg_0;
	float battle_fov_mag_deg_0;
	float battle_cam_center_x;

	LWDAMAGETEXT damage_text[MAX_DAMAGE_TEXT];

	LWBATTLECREATURE player_creature[MAX_BATTLE_CREATURE];

	int player_turn_creature_index;

	float battle_wall_tex_v;

	mat4x4 battle_proj;
	mat4x4 battle_view;

	int font_texture_texture_mode;

	LWFBO font_fbo;
} LWCONTEXT;

#pragma pack(push, 1)
typedef struct
{
	char header[4];
	char version[2];
	short data_type;
	short extended_width; // BIG ENDIAN!
	short extended_height; // BIG ENDIAN!
	short original_width; // BIG ENDIAN!
	short original_height; // BIG ENDIAN!
} LWPKM;
#pragma pack(pop)

#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT (0.225f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_A (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.9f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_B (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.8f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_C (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.7f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_D (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.6f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.5f)
#define DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F (DEFAULT_TEXT_BLOCK_LINE_HEIGHT*0.4f)

#define DEFAULT_TEXT_BLOCK_SIZE (1.0f)
#define DEFAULT_TEXT_BLOCK_SIZE_A (DEFAULT_TEXT_BLOCK_SIZE*0.90f)
#define DEFAULT_TEXT_BLOCK_SIZE_B (DEFAULT_TEXT_BLOCK_SIZE*0.80f)
#define DEFAULT_TEXT_BLOCK_SIZE_C (DEFAULT_TEXT_BLOCK_SIZE*0.70f)
#define DEFAULT_TEXT_BLOCK_SIZE_D (DEFAULT_TEXT_BLOCK_SIZE*0.60f)
#define DEFAULT_TEXT_BLOCK_SIZE_E (DEFAULT_TEXT_BLOCK_SIZE*0.55f)
