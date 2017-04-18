#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <string.h>

#include "lwgl.h"
#include "laidoff.h"
#include "lwbitmapcontext.h"
#include "constants.h"
#include "kiwi_api.h"
#include "tinyobj_loader_c.h"
#include "file.h"
#include "font.h"
#include "lwcontext.h"
#include "lwlog.h"
#include "ktx.h"
#include "script.h"
#include "unicode.h"
#include "dialog.h"
#include "tex.h"
#include "battlelogic.h"
#include "lwbattlecommand.h"
#include "lwbattlecommandresult.h"
#include "lwmacro.h"
#include "lwenemy.h"
#include "battle.h"
#include "render_font_test.h"
#include "render_admin.h"

#define LWEPSILON (1e-3)
#define INCREASE_RENDER_SCORE (20)
#define RENDER_SCORE_INITIAL (-5)
#define APPLY_IMPULSE_COOLTIME (0.1f)
#define KIWI_COLLISION_BOX_W_MULTIPLIER (0.80f)
#define KIWI_COLLISION_BOX_H_MULTIPLIER (0.80f)
#define BAR_COLLISION_BOX_W_MULTIPLIER (0.90f)
#define BAR_COLLISION_BOX_H_MULTIPLIER (0.95f)
#define KIWI_X_POS (-0.5f)
#define BOAST_WAIT_TIME (3)
#define CANNOT_BOAST_WAIT_TIME (3)
#define QUIT_WAIT_TIME (3)
#define COMPLETION_SCORE (3000)
#define SCROLL_SPEED (0.8f)

#define MAX_COMMAND_SLOT (6)

// Vertex attributes: Coordinates (3) + Normal (3) + UV (2) + S9 (2)
// See Also: LWVERTEX
const static GLsizei stride_in_bytes = (GLsizei)(sizeof(float) * (3 + 3 + 2 + 2));
LwStaticAssert(sizeof(LWVERTEX) == sizeof(float) * (3 + 3 + 2 + 2), "LWVERTEX size error");


#if LW_PLATFORM_ANDROID || LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR
double glfwGetTime() {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec + (double)now.tv_nsec / 1e9;
}

void glfwGetFramebufferSize(void *p, int *w, int *h) {
	// do nothing
}

struct GLFWwindow;
#endif

#pragma pack(push, 1)
typedef struct {
	char idlength;
	char colourmaptype;
	char datatypecode;
	short colourmaporigin;
	short colourmaplength;
	char colourmapdepth;
	short x_origin;
	short y_origin;
	short width;
	short height;
	char bitsperpixel;
	char imagedescriptor;
} TGAHEADER;
#pragma pack(pop)


void play_sound(enum LW_SOUND lws);
void stop_sound(enum LW_SOUND lws);
HRESULT init_ext_image_lib();
HRESULT init_ext_sound_lib();
void destroy_ext_sound_lib();
void create_image(const char *filename, LWBITMAPCONTEXT *pBitmapContext, int tex_atlas_index);
void release_image(LWBITMAPCONTEXT *pBitmapContext);
void lwc_render(const LWCONTEXT *pLwc);
void lwc_render_battle(const LWCONTEXT *pLwc);
void lwc_render_dialog(const LWCONTEXT *pLwc);
void lwc_render_field(const LWCONTEXT *pLwc);
void init_load_textures(LWCONTEXT *pLwc);
void load_test_font(LWCONTEXT *pLwc);
int LoadObjAndConvert(float bmin[3], float bmax[3], const char *filename);
int spawn_attack_trail(LWCONTEXT *pLwc, float x, float y, float z);
void update_attack_trail(LWCONTEXT *pLwc);
float get_battle_enemy_x_center(int enemy_slot_index);
int exec_attack(LWCONTEXT *pLwc, int enemy_slot);
int spawn_damage_text(LWCONTEXT *pLwc, float x, float y, float z, const char *text);
void update_damage_text(LWCONTEXT *pLwc);
void reset_runtime_context(LWCONTEXT* pLwc);

typedef struct {
	GLuint vb;
	int numTriangles;
} DrawObject;

extern DrawObject gDrawObject;

typedef struct _GAPDIST {
	int bar_count;
	float gap_dist;
} GAPDIST;

void set_tex_filter(int min_filter, int mag_filter) {
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
}

static void invalidate_all_touch_proc(LWCONTEXT *pLwc) {
	for (int i = 0; i < MAX_TOUCHPROC_COUNT; i++) {
		pLwc->touch_proc[i].valid = 0;
	}
}

//------------------------------------------------------------------------
unsigned short swap_bytes(unsigned short aData) {
	return (unsigned short)((aData & 0x00FF) << 8) | ((aData & 0xFF00) >> 8);
}

unsigned int swap_4_bytes(unsigned int num) {
	return ((num >> 24) & 0xff) | // move byte 3 to byte 0
		((num << 8) & 0xff0000) | // move byte 1 to byte 2
		((num >> 8) & 0xff00) | // move byte 2 to byte 1
		((num << 24) & 0xff000000); // byte 0 to byte 3
}

static void apply_touch_impulse(LWCONTEXT *pLwc) {
	if (!pLwc->kiwi.dead && !pLwc->completed) {
		const double app_time_since_last = pLwc->app_time - pLwc->kiwi.last_apply_impulse_app_time;

		if (app_time_since_last > APPLY_IMPULSE_COOLTIME) {
			pLwc->kiwi.acc_y_touch = (float)(pLwc->kiwi.impulse_y_touch / pLwc->delta_time);

			play_sound(LWS_TOUCH);

			pLwc->kiwi.last_apply_impulse_app_time = pLwc->app_time;
		}
	}
}

static void change_to_ready_state(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_DIALOG;

	pLwc->kiwi.x = KIWI_X_POS;
	pLwc->kiwi.enable_title_anim = 1;
	pLwc->ready_alpha = 1;
	pLwc->ready_alpha_speed = 2.5f;
	// pLwc->kiwi.y = 0; // controlled by title animation
	pLwc->kiwi.angle = 0;
	pLwc->kiwi.target_angle = 0;
	pLwc->increase_render_score = 0;
	pLwc->render_score = RENDER_SCORE_INITIAL;

	stop_sound(LWS_BGM);
	play_sound(LWS_START);
}

static void change_to_title_state(LWCONTEXT *pLwc) {
	invalidate_all_touch_proc(pLwc);

	pLwc->next_game_scene = LGS_FIELD;
}

static void play_gameover_anim(LWCONTEXT *pLwc, int test) {
	pLwc->completion_target_score_waited_time = 0;
	pLwc->target_score = pLwc->score;
	pLwc->render_score = RENDER_SCORE_INITIAL;

}

static void on_title_enter(LWCONTEXT *pLwc) {
	// Determine a tex atlas
	pLwc->tex_atlas_index = 0;
	pLwc->sprite_data = SPRITE_DATA[pLwc->tex_atlas_index];

	pLwc->ground_scroll_speed = SCROLL_SPEED;

	pLwc->cloud_scroll_speed = 0.4f;

	pLwc->curtain_alpha_speed = 2.5f;

	pLwc->score = 0;

	pLwc->ready_alpha = 1;

	pLwc->bar_dist = 1;

	pLwc->bar_spawn_count = 0;
	pLwc->max_bar_spawn_count = MAX_BAR_SPAWN_COUNT;
	pLwc->completed = 0;
	memset(pLwc->bar, 0, sizeof(pLwc->bar));
	memset(pLwc->anim, 0, sizeof(pLwc->anim));
	memset(pLwc->touch_proc, 0, sizeof(pLwc->touch_proc));

	pLwc->current_heart =
		request_get_today_playing_limit_count() - request_get_today_played_count();
	pLwc->max_heart = request_get_today_playing_limit_count();

	play_sound(LWS_BGM);
}

void set_texture_parameter_values(const LWCONTEXT *pLwc, float x, float y, float w, float h,
	float atlas_w, float atlas_h, int shader_index) {

	const float offset[2] = {
		x / atlas_w,
		y / atlas_h,
	};

	const float scale[2] = {
		w / atlas_w,
		h / atlas_h,
	};

	glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, offset);
	glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, scale);
}

void
set_texture_parameter(const LWCONTEXT *pLwc, LWENUM _LW_ATLAS_ENUM lae, LWENUM _LW_ATLAS_SPRITE las) {
	set_texture_parameter_values(
		pLwc,
		(float)pLwc->sprite_data[las].x,
		(float)pLwc->sprite_data[las].y,
		(float)pLwc->sprite_data[las].w,
		(float)pLwc->sprite_data[las].h,
		(float)pLwc->tex_atlas_width[lae],
		(float)pLwc->tex_atlas_height[lae],
		0
	);
}

static void
create_shader(const char *shader_name, LWSHADER *pShader, const GLchar *vst, const GLchar *fst) {
	pShader->valid = 0;

	GLenum error_enum;
	// vertex shader

	pShader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	error_enum = glGetError();

	const GLchar* vstarray[] = { vst };
	glShaderSource(pShader->vertex_shader, 1, vstarray, NULL);
	error_enum = glGetError();
	glCompileShader(pShader->vertex_shader);
	error_enum = glGetError();
	GLint isCompiled = 0;
	glGetShaderiv(pShader->vertex_shader, GL_COMPILE_STATUS, &isCompiled);
	error_enum = glGetError();
	if (isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(pShader->vertex_shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		GLchar *errorLog = (GLchar *)calloc(maxLength, sizeof(GLchar));
		glGetShaderInfoLog(pShader->vertex_shader, maxLength, &maxLength, errorLog);
		LOGE("'%s' Vertex Shader Error (length:%d): %s", shader_name, maxLength, errorLog);
		free(errorLog);

		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(pShader->vertex_shader); // Don't leak the shader.
		return;
	}

	// fragment shader

	pShader->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	error_enum = glGetError();
	const GLchar* fstarray[] = { fst };
	glShaderSource(pShader->fragment_shader, 1, fstarray, NULL);
	error_enum = glGetError();
	glCompileShader(pShader->fragment_shader);
	error_enum = glGetError();
	glGetShaderiv(pShader->fragment_shader, GL_COMPILE_STATUS, &isCompiled);
	error_enum = glGetError();
	if (isCompiled == GL_FALSE) {
		GLint maxLength = 0;
		glGetShaderiv(pShader->fragment_shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		GLchar *errorLog = (GLchar *)calloc(maxLength, sizeof(GLchar));
		glGetShaderInfoLog(pShader->fragment_shader, maxLength, &maxLength, errorLog);
		LOGE("'%s' Fragment Shader Error: %s", shader_name, errorLog);
		free(errorLog);

		// Provide the infolog in whatever manor you deem best.
		// Exit with failure.
		glDeleteShader(pShader->fragment_shader); // Don't leak the shader.
		return;
	}

	pShader->program = glCreateProgram();
	error_enum = glGetError();
	glAttachShader(pShader->program, pShader->vertex_shader);
	error_enum = glGetError();
	glAttachShader(pShader->program, pShader->fragment_shader);
	error_enum = glGetError();
	glLinkProgram(pShader->program);
	error_enum = glGetError();
	//Note the different functions here: glGetProgram* instead of glGetShader*.
	GLint isLinked = 0;
	glGetProgramiv(pShader->program, GL_LINK_STATUS, (int *)&isLinked);
	error_enum = glGetError();
	if (isLinked == GL_FALSE) {
		GLint maxLength = 0;
		glGetProgramiv(pShader->program, GL_INFO_LOG_LENGTH, &maxLength);

		//The maxLength includes the NULL character
		GLchar *errorLog = (GLchar *)calloc(maxLength, sizeof(GLchar));
		glGetProgramInfoLog(pShader->program, maxLength, &maxLength, errorLog);
		LOGE("'%s' Program Link Error: %s", shader_name, errorLog);
		free(errorLog);
		//We don't need the program anymore.
		glDeleteProgram(pShader->program);
		//Don't leak shaders either.
		glDeleteShader(pShader->vertex_shader);
		glDeleteShader(pShader->fragment_shader);

		//Use the infoLog as you see fit.

		//In this simple program, we'll just leave
		return;
	}

	glUseProgram(pShader->program);
	// Uniforms
	pShader->mvp_location = glGetUniformLocation(pShader->program, "MVP");
	error_enum = glGetError();
	pShader->vuvoffset_location = glGetUniformLocation(pShader->program, "vUvOffset");
	error_enum = glGetError();
	pShader->vuvscale_location = glGetUniformLocation(pShader->program, "vUvScale");
	error_enum = glGetError();
	pShader->vs9offset_location = glGetUniformLocation(pShader->program, "vS9Offset");
	error_enum = glGetError();
	pShader->alpha_multiplier_location = glGetUniformLocation(pShader->program, "alpha_multiplier");
	error_enum = glGetError();
	pShader->overlay_color_location = glGetUniformLocation(pShader->program, "overlay_color");
	error_enum = glGetError();
	pShader->overlay_color_ratio_location = glGetUniformLocation(pShader->program,
		"overlay_color_ratio");
	error_enum = glGetError();
	pShader->diffuse_location = glGetUniformLocation(pShader->program, "diffuse");
	error_enum = glGetError();
	pShader->alpha_only_location = glGetUniformLocation(pShader->program, "alpha_only");
	error_enum = glGetError();
	pShader->glyph_color_location = glGetUniformLocation(pShader->program, "glyph_color");
	error_enum = glGetError();
	pShader->outline_color_location = glGetUniformLocation(pShader->program, "outline_color");
	error_enum = glGetError();

	// Attribs
	pShader->vpos_location = glGetAttribLocation(pShader->program, "vPos");
	error_enum = glGetError();
	pShader->vcol_location = glGetAttribLocation(pShader->program, "vCol");
	error_enum = glGetError();
	pShader->vuv_location = glGetAttribLocation(pShader->program, "vUv");
	error_enum = glGetError();
	pShader->vs9_location = glGetAttribLocation(pShader->program, "vS9");
	error_enum = glGetError();

	pShader->valid = 1;
}

void init_gl_shaders(LWCONTEXT *pLwc) {

#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
#define GLSL_DIR_NAME "glsl"
#else
#define GLSL_DIR_NAME "glsles"
#endif

	char *default_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "default-vert.glsl");
	char *default_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "default-frag.glsl");
	char *font_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "font-frag.glsl");
	char *etc1_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "etc1-frag.glsl");

	if (!default_vert_glsl) {
		LOGE("init_gl_shaders: default-vert.glsl not loaded. Abort...");
		return;
	}

	if (!default_frag_glsl) {
		LOGE("init_gl_shaders: default-frag.glsl not loaded. Abort...");
		return;
	}

	if (!font_frag_glsl) {
		LOGE("init_gl_shaders: font-frag.glsl not loaded. Abort...");
		return;
	}

	if (!etc1_frag_glsl) {
		LOGE("init_gl_shaders: etc1-frag.glsl not loaded. Abort...");
		return;
	}

	create_shader("Default Shader", &pLwc->shader[0], default_vert_glsl, default_frag_glsl);
	create_shader("Font Shader", &pLwc->shader[1], default_vert_glsl, font_frag_glsl);
	create_shader("ETC1 with Alpha Shader", &pLwc->shader[2], default_vert_glsl, etc1_frag_glsl);

	release_string(default_vert_glsl);
	release_string(default_frag_glsl);
	release_string(font_frag_glsl);
	release_string(etc1_frag_glsl);
}

static void load_vbo(LWCONTEXT *pLwc, const char *filename, LWVBO *pVbo) {
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	size_t mesh_file_size = 0;
	char *mesh_vbo_data = create_binary_from_file(filename, &mesh_file_size);
	glBufferData(GL_ARRAY_BUFFER, mesh_file_size, mesh_vbo_data, GL_STATIC_DRAW);
	release_binary(mesh_vbo_data);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	pVbo->vertex_buffer = vbo;
	pVbo->vertex_count = (int)(mesh_file_size / stride_in_bytes);
}

static void init_vbo(LWCONTEXT *pLwc) {
	// LVT_BATTLE_BOWL_OUTER
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Bowl_Outer.vbo",
		&pLwc->vertex_buffer[LVT_BATTLE_BOWL_OUTER]);

	// LVT_BATTLE_BOWL_INNER
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Bowl_Inner.vbo",
		&pLwc->vertex_buffer[LVT_BATTLE_BOWL_INNER]);

	// LVT_ENEMY_SCOPE
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "EnemyScope.vbo",
		&pLwc->vertex_buffer[LVT_ENEMY_SCOPE]);

	// LVT_LEFT_TOP_ANCHORED_SQUARE ~ LVT_RIGHT_BOTTOM_ANCHORED_SQUARE
	// 9 anchored squares...
	const float anchored_square_offset[][2] = {
		{ +1, -1 }, { +0, -1 }, { -1, -1 },
		{ +1, +0 }, { +0, +0 }, { -1, +0 },
		{ +1, +1 }, { +0, +1 }, { -1, +1 },
	};
	for (int i = 0; i < ARRAY_SIZE(anchored_square_offset); i++) {
		LWVERTEX square_vertices[VERTEX_COUNT_PER_ARRAY];
		memcpy(square_vertices, full_square_vertices, sizeof(full_square_vertices));
		for (int r = 0; r < VERTEX_COUNT_PER_ARRAY; r++) {
			square_vertices[r].x += anchored_square_offset[i][0];
			square_vertices[r].y += anchored_square_offset[i][1];
		}

		const LW_VBO_TYPE lvt = LVT_LEFT_TOP_ANCHORED_SQUARE + i;

		glGenBuffers(1, &pLwc->vertex_buffer[lvt].vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(LWVERTEX) * VERTEX_COUNT_PER_ARRAY,
			square_vertices, GL_STATIC_DRAW);
		pLwc->vertex_buffer[lvt].vertex_count = VERTEX_COUNT_PER_ARRAY;
	}

	// LVT_PLAYER
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Player.vbo",
		&pLwc->vertex_buffer[LVT_PLAYER]);

	// LVT_CUBE_WALL
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "CubeWall.vbo",
		&pLwc->vertex_buffer[LVT_CUBE_WALL]);

	// LVT_HOME
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Home.vbo",
		&pLwc->vertex_buffer[LVT_HOME]);

	// LVT_TRAIL
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Trail.vbo",
		&pLwc->vertex_buffer[LVT_TRAIL]);
}

void set_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
	GLenum error_enum;
	glEnableVertexAttribArray(pLwc->shader[shader_index].vpos_location);
	error_enum = glGetError();
	glVertexAttribPointer(pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
		stride_in_bytes, (void *)0);
	error_enum = glGetError();
	glEnableVertexAttribArray(pLwc->shader[shader_index].vcol_location);
	error_enum = glGetError();
	glVertexAttribPointer(pLwc->shader[shader_index].vcol_location, 3, GL_FLOAT, GL_FALSE,
		stride_in_bytes, (void *)(sizeof(float) * 3));
	error_enum = glGetError();
	glEnableVertexAttribArray(pLwc->shader[shader_index].vuv_location);
	error_enum = glGetError();
	glVertexAttribPointer(pLwc->shader[shader_index].vuv_location, 2, GL_FLOAT, GL_FALSE,
		stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
	error_enum = glGetError();
	glEnableVertexAttribArray(pLwc->shader[shader_index].vs9_location);
	error_enum = glGetError();
	glVertexAttribPointer(pLwc->shader[shader_index].vs9_location, 2, GL_FLOAT, GL_FALSE,
		stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2)));
	error_enum = glGetError();
}

static void init_vao(LWCONTEXT *pLwc, int shader_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	GLenum error_enum = glGetError();

	glGenVertexArrays(VERTEX_BUFFER_COUNT, pLwc->vao);
	error_enum = glGetError();

	for (int i = 0; i < VERTEX_BUFFER_COUNT; i++) {
		glBindVertexArray(pLwc->vao[i]);
		error_enum = glGetError();
		glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[i].vertex_buffer);
		error_enum = glGetError();
		set_vertex_attrib_pointer(pLwc, shader_index);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	error_enum = glGetError();
	glBindVertexArray(0);
	error_enum = glGetError();
#endif
}

static void init_gl_context(LWCONTEXT *pLwc) {
	init_gl_shaders(pLwc);

	init_vbo(pLwc);

	init_vao(pLwc, 0);

	// Enable culling (CCW is default)
	glEnable(GL_CULL_FACE);

	//glEnable(GL_ALPHA_TEST);
	//glCullFace(GL_CW);
	//glDisable(GL_CULL_FACE);

	// Alpha component should be 1 in RPI platform.
	glClearColor(90 / 255.f, 173 / 255.f, 255 / 255.f, 1);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_ONE, GL_ONE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glDepthMask(GL_TRUE);
	mat4x4_identity(pLwc->mvp);
}

static void reset_dir_pad_position(LWCONTEXT *pLwc) {
	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	float dir_pad_center_x = 0;
	float dir_pad_center_y = 0;
	get_dir_pad_center(aspect_ratio, &dir_pad_center_x, &dir_pad_center_y);

	pLwc->dir_pad_x = dir_pad_center_x;
	pLwc->dir_pad_y = dir_pad_center_y;
}

void set_creature_data(LWBATTLECREATURE* c, const char* name, int lv, int hp, int max_hp, int mp, int max_mp, int turn_token) {
	c->valid = 1;
	strcpy(c->name, name);
	c->lv = lv;
	c->hp = hp;
	c->max_hp = max_hp;
	c->mp = mp;
	c->max_mp = max_hp;
	c->selected_r = 0.5f;
	c->selected_g = 0.5f;
	c->selected_b = 0.5f;
	c->selected_a = 0.5f;
	c->turn_token = turn_token;
}

void reset_runtime_context(LWCONTEXT* pLwc) {

	pLwc->game_scene = LGS_BATTLE;
	pLwc->font_texture_texture_mode = 0;

	pLwc->battle_fov_deg_0 = 49.134f;
	pLwc->battle_fov_mag_deg_0 = 35.134f;
	pLwc->battle_fov_deg = pLwc->battle_fov_deg_0;
	pLwc->selected_enemy_slot = -1;

	// Fill enemy slots with test enemies
	pLwc->enemy[0] = ENEMY_DATA_LIST[0];
	pLwc->enemy[1] = ENEMY_DATA_LIST[1];
	pLwc->enemy[2] = ENEMY_DATA_LIST[2];
	pLwc->enemy[3] = ENEMY_DATA_LIST[3];
	pLwc->enemy[4] = ENEMY_DATA_LIST[4];

	memset(pLwc->player_creature, 0, sizeof(pLwc->player_creature));

	set_creature_data(
		&pLwc->player_creature[0],
		u8"주인공",
		1,
		50,
		50,
		30,
		30,
		1
	);
	pLwc->player_creature[0].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player_creature[0].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player_creature[0].skill[1] = &SKILL_DATA_LIST[1];
	pLwc->player_creature[0].skill[2] = &SKILL_DATA_LIST[2];
	pLwc->player_creature[0].skill[3] = &SKILL_DATA_LIST[3];
	pLwc->player_creature[0].skill[4] = &SKILL_DATA_LIST[4];
	pLwc->player_creature[0].skill[5] = &SKILL_DATA_LIST[5];

	set_creature_data(
		&pLwc->player_creature[1],
		u8"극작가",
		2,
		25,
		50,
		30,
		30,
		2
	);
	pLwc->player_creature[1].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player_creature[1].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player_creature[1].skill[1] = &SKILL_DATA_LIST[2];

	set_creature_data(
		&pLwc->player_creature[2],
		u8"손톱깎이",
		3,
		46,
		46,
		15,
		55,
		3
	);
	pLwc->player_creature[2].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player_creature[2].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player_creature[2].skill[1] = &SKILL_DATA_LIST[3];

	set_creature_data(
		&pLwc->player_creature[3],
		u8"대문호",
		8,
		105,
		105,
		0,
		20,
		4
	);
	pLwc->player_creature[3].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player_creature[3].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player_creature[3].skill[1] = &SKILL_DATA_LIST[4];

	for (int i = 0; i < MAX_BATTLE_CREATURE; i++) {
		pLwc->player_creature[i].selected = 0;
	}

	pLwc->player_creature[0].selected = 1;

	pLwc->player_turn_creature_index = 0;

	pLwc->battle_state = LBS_SELECT_COMMAND;

	pLwc->selected_command_slot = 0;

	pLwc->font_fbo.dirty = 1;

	pLwc->admin_button_command[0].name = LWU("필드");
	pLwc->admin_button_command[0].command_handler = change_to_field;
	pLwc->admin_button_command[1].name = LWU("대화");
	pLwc->admin_button_command[1].command_handler = change_to_dialog;
	pLwc->admin_button_command[2].name = LWU("전투");
	pLwc->admin_button_command[2].command_handler = change_to_battle;
	pLwc->admin_button_command[3].name = LWU("글꼴");
	pLwc->admin_button_command[3].command_handler = change_to_font_test;
	pLwc->admin_button_command[4].name = LWU("전투리셋");
	pLwc->admin_button_command[4].command_handler = reset_runtime_context;
	pLwc->admin_button_command[5].name = LWU("글꼴디버그");
	pLwc->admin_button_command[5].command_handler = toggle_font_texture_test_mode;
}

void delete_font_fbo(LWCONTEXT* pLwc) {
	if (pLwc->font_fbo.fbo) {
		glDeleteFramebuffers(1, &pLwc->font_fbo.fbo);
		pLwc->font_fbo.fbo = 0;
	}

	if (pLwc->font_fbo.depth_render_buffer) {
		glDeleteRenderbuffers(1, &pLwc->font_fbo.depth_render_buffer);
		pLwc->font_fbo.depth_render_buffer = 0;
	}

	if (pLwc->font_fbo.color_tex) {
		glDeleteTextures(1, &pLwc->font_fbo.color_tex);
		pLwc->font_fbo.color_tex = 0;
	}
}

void init_font_fbo(LWCONTEXT* pLwc) {

	// Delete GL resources before init

	delete_font_fbo(pLwc);

	// Start init

	pLwc->font_fbo.width = 1024;
	pLwc->font_fbo.height = 1024;

	glGenFramebuffers(1, &pLwc->font_fbo.fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, pLwc->font_fbo.fbo);

	glGenRenderbuffers(1, &pLwc->font_fbo.depth_render_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, pLwc->font_fbo.depth_render_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, pLwc->width, pLwc->height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_COMPONENT, GL_RENDERBUFFER, pLwc->font_fbo.depth_render_buffer);

	glGenTextures(1, &pLwc->font_fbo.color_tex);
	glBindTexture(GL_TEXTURE_2D, pLwc->font_fbo.color_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pLwc->font_fbo.width, pLwc->font_fbo.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pLwc->font_fbo.color_tex, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		LOGE("init_font_fbo: glCheckFramebufferStatus failed. return = %d", status);
		delete_font_fbo(pLwc);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	pLwc->font_fbo.dirty = 1;
}

void init_lwc_runtime_data(LWCONTEXT *pLwc) {

	init_font_fbo(pLwc);

	reset_runtime_context(pLwc);

	pLwc->highscore = request_get_highscore();

	//pLwc->pFnt = load_fnt(ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold.fnt");
	pLwc->pFnt = load_fnt(ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test6.fnt");

	//const BMF_CHAR *bc = font_binary_search_char(pLwc->pFnt, 100);

	pLwc->dialog = create_string_from_file(ASSETS_BASE_PATH "d" PATH_SEPARATOR "d1.txt");
	if (pLwc->dialog) {
		pLwc->dialog_bytelen = (int)strlen(pLwc->dialog);
	} else {
		LOGE("dialog loading failed.");
	}


	/*
	pLwc->dialog_total_len = u8_toucs(pLwc->dialog, ARRAY_SIZE(pLwc->dialog),
		pLwc->dialog, dialog_utf8_len);
	 */

	init_lua(pLwc);

	spawn_field_object(pLwc, 0, 5, 2, 2, LVT_HOME, pLwc->tex_programmed[LPT_SOLID_GREEN], 1, 1);

	spawn_field_object(pLwc, 0, -7, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_BLUE], 6,
		1);

	change_to_title_state(pLwc);
	on_title_enter(pLwc);
}

void set_sprite_mvp_with_scale(const LWCONTEXT *pLwc, enum _LW_ATLAS_SPRITE las, float x, float y,
	float angle, float scale, const mat4x4 p, mat4x4 result) {
	mat4x4 kiwi_trans, kiwi_rotate, kiwi_scale_rotate, kiwi_mv;
	mat4x4 identity;
	mat4x4_identity(identity);
	mat4x4_rotate_Z(kiwi_rotate, identity, angle);
	mat4x4_scale_aniso(kiwi_scale_rotate, kiwi_rotate,
		(float)pLwc->sprite_data[las].w / pLwc->width * scale,
		(float)pLwc->sprite_data[las].h / pLwc->height * scale, (float)1);

	mat4x4_translate(kiwi_trans, x, y, 0);

	mat4x4_mul(kiwi_mv, kiwi_trans, kiwi_scale_rotate);
	mat4x4_mul(result, p, kiwi_mv);
}

void set_sprite_mvp(const LWCONTEXT *pLwc, enum _LW_ATLAS_SPRITE las, float x, float y, float angle,
	const mat4x4 p, mat4x4 result) {
	mat4x4 kiwi_trans, kiwi_rotate, kiwi_scale_rotate, kiwi_mv;
	mat4x4 identity;
	mat4x4_identity(identity);
	mat4x4_rotate_Z(kiwi_rotate, identity, angle);
	mat4x4_scale_aniso(kiwi_scale_rotate, kiwi_rotate,
		(float)pLwc->sprite_data[las].w / pLwc->width,
		(float)pLwc->sprite_data[las].h / pLwc->height, (float)1);

	mat4x4_translate(kiwi_trans, x, y, 0);

	mat4x4_mul(kiwi_mv, kiwi_trans, kiwi_scale_rotate);
	mat4x4_mul(result, p, kiwi_mv);
}

float get_greatest_bar_x(const LWCONTEXT *pLwc) {
	float g = -NORMALIZED_SCREEN_RESOLUTION_X / 2;
	for (int i = 0; i < ARRAY_SIZE(pLwc->bar); i++) {
		if (pLwc->bar[i].valid && g < pLwc->bar[i].x) {
			g = pLwc->bar[i].x;
		}
	}

	return g;
}

int has_collided(const LWCONTEXT *pLwc) {
	const int sign[][2] =
	{
			{+1, +1},
			{+1, -1},
			{-1, +1},
			{-1, -1},
	};

	// check with ground
	for (int i = 0; i < ARRAY_SIZE(sign); i++) {
		const float py = pLwc->kiwi.y + sign[i][1] * pLwc->kiwi.h / 2;

		if (py < pLwc->ground.ground_level_y + LWEPSILON) {
			return 1;
		}
	}

	// check with bars
	for (int b = 0; b < ARRAY_SIZE(pLwc->bar); b++) {
		if (!pLwc->bar[b].valid) {
			continue;
		}

		for (int i = 0; i < ARRAY_SIZE(sign); i++) {
			const float px =
				pLwc->kiwi.x + sign[i][0] * pLwc->kiwi.w * KIWI_COLLISION_BOX_W_MULTIPLIER / 2;
			const float py =
				pLwc->kiwi.y + sign[i][1] * pLwc->kiwi.h * KIWI_COLLISION_BOX_H_MULTIPLIER / 2;

			const float bx_l = pLwc->bar[b].x - pLwc->bar[b].w * BAR_COLLISION_BOX_W_MULTIPLIER / 2;
			const float bx_r = pLwc->bar[b].x + pLwc->bar[b].w * BAR_COLLISION_BOX_W_MULTIPLIER / 2;

			if (bx_l < px && px < bx_r) {
				// check with an upper bar

				const float by1_b =
					pLwc->bar[b].y1 - pLwc->bar[b].h * BAR_COLLISION_BOX_H_MULTIPLIER / 2;
				const float by1_t =
					pLwc->bar[b].y1 + pLwc->bar[b].h * BAR_COLLISION_BOX_H_MULTIPLIER / 2;

				if (by1_b < py && py < by1_t) {
					return 1;
				}

				// check with a lower bar

				const float by2_b =
					pLwc->bar[b].y2 - pLwc->bar[b].h * BAR_COLLISION_BOX_H_MULTIPLIER / 2;
				const float by2_t =
					pLwc->bar[b].y2 + pLwc->bar[b].h * BAR_COLLISION_BOX_H_MULTIPLIER / 2;

				if (by2_b < py && py < by2_t) {
					return 1;
				}
			}
		}
	}

	return 0;
}

void update_anim(LWCONTEXT *pLwc) {
	for (int i = 0; i < ARRAY_SIZE(pLwc->anim); i++) {
		LWANIM *anim = &pLwc->anim[i];

		if (!anim->valid) {
			continue;
		}

		anim->elapsed += (float)pLwc->delta_time;

		const float elapsed_frame = anim->elapsed * anim->fps;

		int current_animdata_index = -1;

		for (int j = anim->animdata_length - 1; j >= 0; j--) {
			if (elapsed_frame >= anim->animdata[j].frame) {
				current_animdata_index = j;
				break;
			}
		}

		//printf("cai=%d, cf=%f\n", current_animdata_index, elapsed_frame);

		// final frames already presented
		if (current_animdata_index >= anim->animdata_length - 1) {
			anim->x = anim->animdata[anim->animdata_length - 1].x;
			anim->y = anim->animdata[anim->animdata_length - 1].y;
			anim->alpha = anim->animdata[anim->animdata_length - 1].alpha;

			if (!anim->finalized) {
				anim->finalized = 1;

				if (anim->anim_finalized_proc_callback) {
					anim->anim_finalized_proc_callback(pLwc);
				}
			}
		} else {
			// not started
			if (current_animdata_index == -1) {
				anim->x = 0;
				anim->y = 0;
				anim->alpha = 0;
			} else {
				const LWKEYFRAME *f1 = &anim->animdata[current_animdata_index];
				const LWKEYFRAME *f2 = &anim->animdata[current_animdata_index + 1];

				const int frame_count = f2->frame - f1->frame;
				const float frame_distance = elapsed_frame - f1->frame;
				const float r = frame_distance / frame_count;

				anim->x = f1->x + r * (f2->x - f1->x);
				anim->y = f1->y + r * (f2->y - f1->y);
				anim->alpha = f1->alpha + r * (f2->alpha - f1->alpha);
			}
		}

		set_sprite_mvp(pLwc, anim->las, anim->x, anim->y, 0, pLwc->proj, anim->mvp);
	}
}

void render_anim(const LWCONTEXT *pLwc) {
	for (int i = 0; i < ARRAY_SIZE(pLwc->anim); i++) {
		const LWANIM *anim = &pLwc->anim[i];

		if (!anim->valid) {
			continue;
		}

		if (anim->custom_render_proc_callback) {
			anim->custom_render_proc_callback(pLwc, anim->x, anim->y, anim->alpha);
		} else {
			set_texture_parameter(pLwc, LAE_C2_PNG, anim->las);
			glUniformMatrix4fv(pLwc->shader[0].mvp_location, 1, GL_FALSE,
				(const GLfloat *)anim->mvp);
			glUniform1f(pLwc->shader[0].alpha_multiplier_location, anim->alpha);
			glDrawElements(GL_TRIANGLES, VERTEX_COUNT_PER_ARRAY, GL_UNSIGNED_SHORT,
				(GLvoid *)((char *)NULL));
		}
	}
}

// http://www.cse.yorku.ca/~oz/hash.html
unsigned long hash(const unsigned char *str) {
	unsigned long hash = 5381;
	int c;

	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}

int get_tex_index_by_hash_key(const LWCONTEXT *pLwc, const char *hash_key) {
	unsigned long h = hash((const unsigned char *)hash_key);
	for (int i = 0; i < MAX_TEX_ATLAS; i++) {
		if (pLwc->tex_atlas_hash[i] == h) {
			return i;
		}
	}
	return 0;
}

void lwc_render(const LWCONTEXT *pLwc) {
	if (pLwc->game_scene == LGS_BATTLE) {
		lwc_render_battle(pLwc);
	} else if (pLwc->game_scene == LGS_DIALOG) {
		lwc_render_dialog(pLwc);
	} else if (pLwc->game_scene == LGS_FIELD) {
		lwc_render_field(pLwc);
	} else if (pLwc->game_scene == LGS_FONT_TEST) {
		lwc_render_font_test(pLwc);
	} else if (pLwc->game_scene == LGS_ADMIN) {
		lwc_render_admin(pLwc);
	}
}

void get_dir_pad_center(float aspect_ratio, float *x, float *y) {
	if (aspect_ratio > 1) {
		*x = -1 * aspect_ratio + 0.5f;
		*y = -0.5f;
	} else {
		*x = -0.5f;
		*y = -1 / aspect_ratio + 0.5f;
	}
}

static int
lw_get_normalized_dir_pad_input(const LWCONTEXT *pLwc, float *dx, float *dy, float *dlen) {
	if (!pLwc->dir_pad_dragging) {
		return 0;
	}

	const float aspect_ratio = (float)pLwc->width / pLwc->height;
	float dir_pad_center_x = 0;
	float dir_pad_center_y = 0;
	get_dir_pad_center(aspect_ratio, &dir_pad_center_x, &dir_pad_center_y);

	*dx = pLwc->dir_pad_x - dir_pad_center_x;
	*dy = pLwc->dir_pad_y - dir_pad_center_y;

	*dlen = sqrtf(*dx * *dx + *dy * *dy);

	*dx /= *dlen;
	*dy /= *dlen;

	return 1;
}

static void move_player(LWCONTEXT *pLwc) {
	const float move_speed = 5.0f;
	const float move_speed_delta = (float)(pLwc->delta_time * move_speed);

	// Using keyboard
	pLwc->player_pos_x += (float)((pLwc->player_move_right - pLwc->player_move_left) *
		move_speed_delta);
	pLwc->player_pos_y += (float)((pLwc->player_move_up - pLwc->player_move_down) *
		move_speed_delta);

	// Using mouse
	float dx, dy, dlen;
	if (lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
		pLwc->player_pos_x += dx * move_speed_delta;
		pLwc->player_pos_y += dy * move_speed_delta;
	}
}

static float resolve_collision_one_fixed_axis(float fp, float fs, float mp, float ms) {
	const float f_lo = fp - fs / 2;
	const float f_hi = fp + fs / 2;
	const float m_lo = mp - ms / 2;
	const float m_hi = mp + ms / 2;
	float displacement = 0;
	if ((f_lo < m_lo && m_lo < f_hi) || (f_lo < m_hi && m_hi < f_hi) ||
		(m_lo < f_lo && f_hi < m_hi)) {
		if (fp < mp) {
			displacement = f_hi - m_lo;
		} else {
			displacement = -(m_hi - f_lo);
		}
	}

	return displacement;
}

static void resolve_collision_one_fixed(const LWBOX2DCOLLIDER *fixed, LWBOX2DCOLLIDER *movable) {
	const float dx = resolve_collision_one_fixed_axis(fixed->x, fixed->w, movable->x, movable->w);

	if (dx) {
		const float dy = resolve_collision_one_fixed_axis(fixed->y, fixed->h, movable->y,
			movable->h);

		if (dy) {
			if (fabs(dx) < fabs(dy)) {
				movable->x += dx;
			} else {
				movable->y += dy;
			}
		}
	}
}

static void resolve_player_collision(LWCONTEXT *pLwc) {
	LWBOX2DCOLLIDER player_collider;
	player_collider.x = pLwc->player_pos_x;
	player_collider.y = pLwc->player_pos_y;
	player_collider.w = 1.0f;
	player_collider.h = 1.0f;

	for (int i = 0; i < MAX_BOX_COLLIDER; i++) {
		if (pLwc->box_collider[i].valid) {
			resolve_collision_one_fixed(&pLwc->box_collider[i], &player_collider);
		}
	}

	pLwc->player_pos_x = player_collider.x;
	pLwc->player_pos_y = player_collider.y;
}

static void update_battle_wall(LWCONTEXT* pLwc) {
	pLwc->battle_wall_tex_v += (float)(pLwc->delta_time / 34);
	pLwc->battle_wall_tex_v = fmodf(pLwc->battle_wall_tex_v, 1.0f);
}

void lwc_update(LWCONTEXT *pLwc, double delta_time) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	const double cur_time = glfwGetTime();
	pLwc->delta_time = cur_time - pLwc->last_time;
	pLwc->last_time = cur_time;
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	//return now.tv_sec + (double)now.tv_nsec/1e9;

	long nsec_diff = now.tv_nsec - pLwc->last_time.tv_nsec;
	long sec_diff = now.tv_sec - pLwc->last_time.tv_sec;

	if (nsec_diff < 0) {
		nsec_diff += 1000000000LL;
		sec_diff--;
	}

	pLwc->delta_time = delta_time; //(double)nsec_diff / 1e9 + sec_diff;
	pLwc->last_time = now;
#endif

	// accumulate time since app startup
	pLwc->app_time += pLwc->delta_time;

	update_dialog(pLwc);

	//****//
	// fix delta time
	//pLwc->delta_time = 1.0f / 60;
	//****//

	float ratio = pLwc->width / (float)pLwc->height;

	LOGV("Update(): width: %d height: %d ratio: %f", pLwc->width, pLwc->height, ratio);

	if (ratio > 1) {
		mat4x4_ortho(pLwc->proj, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
	} else {
		mat4x4_ortho(pLwc->proj, -1.f, 1.f, -1 / ratio, 1 / ratio, 1.f, -1.f);
	}

	update_battle(pLwc);

#define ARRAY_ITERATE_VALID(v) \
    for (int i = 0; i < ARRAY_SIZE((v)); i++) \
    { \
        if (v[i].valid)

#define ARRAY_ITERATE_VALID_END() \
    }

	ARRAY_ITERATE_VALID(pLwc->enemy) {
		update_enemy(pLwc, i, &pLwc->enemy[i]);
	}
	ARRAY_ITERATE_VALID_END();

	move_player(pLwc);

	resolve_player_collision(pLwc);

	update_attack_trail(pLwc);

	update_damage_text(pLwc);

	update_battle_wall(pLwc);

	if (pLwc->font_fbo.dirty) {
		lwc_render_font_test_fbo(pLwc);
		pLwc->font_fbo.dirty = 0;
	}

	((LWCONTEXT *)pLwc)->update_count++;
}

void bind_all_vertex_attrib_shader(const LWCONTEXT *pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	glBindVertexArray(pLwc->vao[vbo_index]);
#else
	set_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

void bind_all_vertex_attrib(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_vertex_attrib_shader(pLwc, 0, vbo_index);
}

void bind_all_vertex_attrib_font(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_vertex_attrib_shader(pLwc, 1, vbo_index);
}

void bind_all_vertex_attrib_etc1_with_alpha(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_vertex_attrib_shader(pLwc, 2, vbo_index);
}

#if !(LW_PLATFORM_WIN32 || LW_PLATFORM_OSX)
static void load_pkm_texture(const char *tex_atlas_filename) {
	size_t file_size = 0;
	char *b = create_binary_from_file(tex_atlas_filename, &file_size);
	if (!b) {
		LOGE("load_pkm_texture: create_binary_from_file null, filename %s", tex_atlas_filename);
		return;
	}
	LWPKM *pPkm = (LWPKM *)b;

	GLenum error_enum;

	short extended_width = swap_bytes(pPkm->extended_width);
	short extended_height = swap_bytes(pPkm->extended_height);

	// TODO: iOS texture
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX || LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR || LW_PLATFORM_RPI
	LWBITMAPCONTEXT bitmap_context;
	create_image(tex_atlas_filename, &bitmap_context, 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap_context.width, bitmap_context.height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, bitmap_context.data);
	error_enum = glGetError();
	LOGI("glTexImage2D (ETC1 software decompression) result (%dx%d): %d", bitmap_context.width, bitmap_context.height,
		error_enum);

	release_image(&bitmap_context);
#else
	// calculate size of data with formula (extWidth / 4) * (extHeight / 4) * 8
	u32 dataLength = ((extended_width >> 2) * (extended_height >> 2)) << 3;

	glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, extended_width, extended_height, 0,
		dataLength, b + sizeof(LWPKM));
#endif

	error_enum = glGetError();
	LOGI("glCompressedTexImage2D result (%dx%d): %d", extended_width, extended_height,
		error_enum);

	release_binary(b);
}
#endif

static void load_tex_files(LWCONTEXT *pLwc) {
	glGenTextures(MAX_TEX_ATLAS, pLwc->tex_atlas);

	for (int i = 0; i < MAX_TEX_ATLAS; i++) {
		glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[i]);

		size_t tex_atlas_filename_len = (int)strlen(tex_atlas_filename[i]);

		GLenum error_enum;

		size_t filename_index = tex_atlas_filename_len;
		while (tex_atlas_filename[i][filename_index - 1] != PATH_SEPARATOR[0]) {
			filename_index--;
		}

#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
		// Do Nothing
#else
		// Hardware decoding of PKM
		if (strcmp(tex_atlas_filename[i] + tex_atlas_filename_len - 4, ".pkm") == 0) {
			load_pkm_texture(tex_atlas_filename[i]);
		} else
#endif
			// Software/Hardware decoding of KTX
			if (strcmp(tex_atlas_filename[i] + tex_atlas_filename_len - 4, ".ktx") == 0) {
				if (load_ktx_texture(tex_atlas_filename[i]) < 0) {
					LOGI("load_ktx_texture failure... %s", tex_atlas_filename[i]);
				}
			} else {
				// Software decoding of PNG or PKM

				LWBITMAPCONTEXT bitmap_context;

				create_image(tex_atlas_filename[i], &bitmap_context, i);

				if (bitmap_context.width > 0 && bitmap_context.height > 0) {
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap_context.width, bitmap_context.height,
						0,
						GL_RGBA, GL_UNSIGNED_BYTE, bitmap_context.data);
					error_enum = glGetError();
					LOGI("glTexImage2D result (%dx%d): %d", bitmap_context.width, bitmap_context.height,
						error_enum);

					release_image(&bitmap_context);

					// all atlas sizes should be equal
					pLwc->tex_atlas_width[i] = bitmap_context.width;
					pLwc->tex_atlas_height[i] = bitmap_context.height;

					glGenerateMipmap(GL_TEXTURE_2D);
					error_enum = glGetError();
					LOGI("glGenerateMipmap result: %d", error_enum);
				} else {
					LOGE("create_image: %s not loaded. Width=%d, height=%d", tex_atlas_filename[i],
						bitmap_context.width, bitmap_context.height);
				}
			}

		pLwc->tex_atlas_hash[i] = hash(
			(const unsigned char *)&tex_atlas_filename[i][filename_index]);
	}
}

void init_load_textures(LWCONTEXT *pLwc) {
	// Sprites

	load_tex_files(pLwc);

	// Fonts

	load_test_font(pLwc);

	// Textures generated by program

	load_tex_program(pLwc);

	glBindTexture(GL_TEXTURE_2D, 0);

}

void load_test_font(LWCONTEXT *pLwc) {
	glGenTextures(MAX_TEX_FONT_ATLAS, pLwc->tex_font_atlas);

	for (int i = 0; i < MAX_TEX_FONT_ATLAS; i++) {
		glBindTexture(GL_TEXTURE_2D, pLwc->tex_font_atlas[i]);

		size_t file_size = 0;
		char *b = create_binary_from_file(tex_font_atlas_filename[i], &file_size);
		if (!b) {
			LOGE("load_test_font: create_binary_from_file null, filename %s", tex_font_atlas_filename[i]);
			continue;
		}

		LOGI("load_test_font %s...", tex_font_atlas_filename[i]);

		const TGAHEADER *tga_header = (const TGAHEADER *)b;

		char *tex_data = (char *)malloc(tga_header->width * tga_header->height * 4);
		for (int j = 0; j < tga_header->width * tga_header->height; j++) {
			char v = *(b + sizeof(TGAHEADER) + j);

			tex_data[4 * j + 0] = v;
			tex_data[4 * j + 1] = v;
			tex_data[4 * j + 2] = v;
			tex_data[4 * j + 3] = v;
		}

		//glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, tga_header->width, tga_header->height, 0, GL_RED, GL_UNSIGNED_BYTE, b + sizeof(TGAHEADER));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tga_header->width, tga_header->height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, tex_data);

		free(tex_data);

		GLenum error_enum = glGetError();
		LOGI("font glTexImage2D result (%dx%d): %d", tga_header->width, tga_header->height,
			error_enum);
		glGenerateMipmap(GL_TEXTURE_2D);
		error_enum = glGetError();
		LOGI("font glGenerateMipmap result: %d", error_enum);

		release_binary(b);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

LWCONTEXT *lw_init(void) {
	init_ext_image_lib();

	init_ext_sound_lib();

	//test_image();

	LWCONTEXT *pLwc = (LWCONTEXT *)calloc(1, sizeof(LWCONTEXT));

	setlocale(LC_ALL, "");

	init_gl_context(pLwc);

	init_load_textures(pLwc);

	init_lwc_runtime_data(pLwc);

#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	pLwc->last_time = glfwGetTime();
#else
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	pLwc->last_time = now;
#endif

	return pLwc;
}

void lw_deinit(LWCONTEXT *pLwc) {
	free(pLwc);
}

void lw_set_size(LWCONTEXT *pLwc, int w, int h) {
	pLwc->width = w;
	pLwc->height = h;

	reset_dir_pad_position(pLwc);
}

void lw_set_window(LWCONTEXT *pLwc, struct GLFWwindow *window) {
	pLwc->window = window;
}

struct GLFWwindow *lw_get_window(const LWCONTEXT *pLwc) {
	return pLwc->window;
}

int lw_get_game_scene(LWCONTEXT *pLwc) {
	return pLwc->game_scene;
}

void lw_trigger_ready(LWCONTEXT *pLwc) {
	pLwc->black_curtain_alpha = 1.0f;

	pLwc->game_scene = LGS_INVALID;

	on_title_enter(pLwc);
	change_to_ready_state(pLwc);
}

static void convert_touch_coord_to_ui_coord(LWCONTEXT *pLwc, float *x, float *y) {
	if (pLwc->height < pLwc->width) {
		*x *= (float)pLwc->width / pLwc->height;
	} else {
		*y *= (float)pLwc->height / pLwc->width;
	}
}

void exec_attack_with_screen_point(LWCONTEXT* pLwc, float x, float y) {

	for (int i = 0; i < MAX_ENEMY_SLOT; i++) {
		if (pLwc->enemy[i].valid && pLwc->enemy[i].c.hp > 0) {
			if (pLwc->enemy[i].left_top_ui_point[0] <= x
				&& x <= pLwc->enemy[i].right_bottom_ui_point[0]
				&& y <= pLwc->enemy[i].left_top_ui_point[1]
				&& pLwc->enemy[i].right_bottom_ui_point[1] <= y) {

				pLwc->battle_state = LBS_SELECT_TARGET;
				pLwc->selected_enemy_slot = i;
				exec_attack(pLwc, i);
			}
		}
	}
}

void lw_trigger_mouse_press(LWCONTEXT *pLwc, float x, float y) {
	if (!pLwc) {
		return;
	}

	convert_touch_coord_to_ui_coord(pLwc, &x, &y);

	printf("mouse press ui coord x=%f, y=%f\n", x, y);

	pLwc->last_mouse_press_x = x;
	pLwc->last_mouse_press_y = y;

	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	float dir_pad_center_x = 0;
	float dir_pad_center_y = 0;
	get_dir_pad_center(aspect_ratio, &dir_pad_center_x, &dir_pad_center_y);

	if (fabs(dir_pad_center_x - x) < 0.5f && fabs(dir_pad_center_y - y) < 0.5f) {
		pLwc->dir_pad_x = x;
		pLwc->dir_pad_y = y;
		pLwc->dir_pad_dragging = 1;
	}

	if (pLwc->battle_state != LBS_COMMAND_IN_PROGRESS && pLwc->player_turn_creature_index >= 0) {
		const float command_palette_pos = -0.5f;

		if (y > command_palette_pos) {
			exec_attack_with_screen_point(pLwc, x, y);
		} else {
			// command palette area
			int command_slot = (int)((x + aspect_ratio) / (2.0f / 10 * aspect_ratio)) - 2;
			if (command_slot >= 0 && command_slot < 6) {
				const LWSKILL* skill = pLwc->player_creature[pLwc->player_turn_creature_index].skill[command_slot];
				if (skill && skill->valid) {
					pLwc->selected_command_slot = command_slot;
				}
			}

			//printf("mouse press command slot %d\n", command_slot);
		}
	}
}

void lw_trigger_mouse_move(LWCONTEXT *pLwc, float x, float y) {
	if (!pLwc) {
		return;
	}

	convert_touch_coord_to_ui_coord(pLwc, &x, &y);

	pLwc->last_mouse_move_x = x;
	pLwc->last_mouse_move_y = y;

	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	if (pLwc->dir_pad_dragging) {
		float dir_pad_center_x = 0;
		float dir_pad_center_y = 0;
		get_dir_pad_center(aspect_ratio, &dir_pad_center_x, &dir_pad_center_y);

		if (x < dir_pad_center_x - 0.5f) {
			x = dir_pad_center_x - 0.5f;
		}

		if (x > dir_pad_center_x + 0.5f) {
			x = dir_pad_center_x + 0.5f;
		}

		if (y < dir_pad_center_y - 0.5f) {
			y = dir_pad_center_y - 0.5f;
		}

		if (y > dir_pad_center_y + 0.5f) {
			y = dir_pad_center_y + 0.5f;
		}

		pLwc->dir_pad_x = x;
		pLwc->dir_pad_y = y;
	}
}

void lw_trigger_mouse_release(LWCONTEXT *pLwc, float x, float y) {
	if (!pLwc) {
		return;
	}

	convert_touch_coord_to_ui_coord(pLwc, &x, &y);

	printf("mouse release ui coord x=%f, y=%f (last move ui coord x=%f, y=%f)\n",
		x, y, pLwc->last_mouse_press_x, pLwc->last_mouse_press_y);

	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	if (pLwc->game_scene != LGS_ADMIN
		&& x < -aspect_ratio + 0.25f
		&& y > 1.0f - 0.25f) {

		change_to_admin(pLwc);
		return;
	}

	reset_dir_pad_position(pLwc);

	pLwc->dir_pad_dragging = 0;

	if (pLwc->game_scene == LGS_ADMIN) {
		touch_admin(pLwc, pLwc->last_mouse_press_x, pLwc->last_mouse_press_y, x, y);
	}
}

void lw_trigger_touch(LWCONTEXT *pLwc, float x, float y) {
	if (!pLwc) {
		return;
	}

	convert_touch_coord_to_ui_coord(pLwc, &x, &y);

	pLwc->dialog_move_next = 1;

	if (pLwc->game_scene == LGS_FIELD) {

	} else if (pLwc->game_scene == LGS_DIALOG) {
		//change_to_playing_state(pLwc);
	} else if (pLwc->game_scene == LGS_BATTLE) {
		apply_touch_impulse(pLwc);
	} else if (pLwc->game_scene == LGS_ADMIN) {
		
	}
}

void lw_trigger_spawn_bar(LWCONTEXT *pLwc) {
	if (pLwc->game_scene == LGS_BATTLE) {
		//spawn_bar(pLwc, 0);
	}
}

void lw_trigger_reset(LWCONTEXT *pLwc) {
	pLwc->black_curtain_alpha = 1.0f;

	if (pLwc->game_scene == LGS_FIELD) {
		pLwc->game_scene = LGS_INVALID;
	}

	change_to_title_state(pLwc);

	reset_runtime_context(pLwc);
}

void lw_trigger_boast(LWCONTEXT *pLwc) {
	pLwc->black_curtain_alpha = 1.0f;

	pLwc->game_scene = LGS_INVALID;
}

void lw_trigger_anim(LWCONTEXT *pLwc) {
	play_gameover_anim(pLwc, 1);
}

void lw_trigger_play_sound(LWCONTEXT *pLwc) {
	play_sound(LWS_TOUCH);
}

void lw_trigger_key_right(LWCONTEXT *pLwc) {

	// battle

	if (pLwc->battle_state == LBS_SELECT_COMMAND) {
		if (pLwc->selected_command_slot != -1 && pLwc->player_turn_creature_index >= 0) {
			int new_selected_command_slot = -1;
			for (int i = pLwc->selected_command_slot + 1; i < MAX_COMMAND_SLOT; i++) {
				const LWSKILL* s = pLwc->player_creature[pLwc->player_turn_creature_index].skill[i];
				if (s && s->valid) {

					new_selected_command_slot = i;
					break;
				}
			}

			if (new_selected_command_slot != -1) {
				pLwc->selected_command_slot = new_selected_command_slot;
			}
		}
	} else if (pLwc->battle_state == LBS_SELECT_TARGET) {

		if (pLwc->selected_enemy_slot != -1) {
			int new_selected_enemy_slot = -1;
			for (int i = pLwc->selected_enemy_slot + 1; i < MAX_ENEMY_SLOT; i++) {
				if (pLwc->enemy[i].valid
					&& pLwc->enemy[i].c.hp > 0) {

					new_selected_enemy_slot = i;
					break;
				}
			}

			if (new_selected_enemy_slot != -1) {
				pLwc->selected_enemy_slot = new_selected_enemy_slot;
			}
		}
	}
}

void lw_trigger_key_left(LWCONTEXT *pLwc) {

	// battle

	if (pLwc->battle_state == LBS_SELECT_COMMAND) {
		if (pLwc->selected_command_slot != -1 && pLwc->player_turn_creature_index >= 0) {
			int new_selected_command_slot = -1;
			for (int i = pLwc->selected_command_slot - 1; i >= 0; i--) {
				const LWSKILL* s = pLwc->player_creature[pLwc->player_turn_creature_index].skill[i];
				if (s && s->valid) {

					new_selected_command_slot = i;
					break;
				}
			}

			if (new_selected_command_slot != -1) {
				pLwc->selected_command_slot = new_selected_command_slot;
			}
		}
	} else if (pLwc->battle_state == LBS_SELECT_TARGET) {
		if (pLwc->selected_enemy_slot != -1) {
			int new_selected_enemy_slot = -1;
			for (int i = pLwc->selected_enemy_slot - 1; i >= 0; i--) {
				if (pLwc->enemy[i].valid
					&& pLwc->enemy[i].c.hp > 0) {

					new_selected_enemy_slot = i;
					break;
				}
			}

			if (new_selected_enemy_slot != -1) {
				pLwc->selected_enemy_slot = new_selected_enemy_slot;
			}
		}
	}
}

int exec_attack(LWCONTEXT *pLwc, int enemy_slot) {
	if (pLwc->battle_state == LBS_SELECT_TARGET && pLwc->player_turn_creature_index >= 0) {

		LWBATTLECREATURE *ca = &pLwc->player_creature[pLwc->player_turn_creature_index];
		const LWSKILL *s = ca->skill[pLwc->selected_command_slot];
		if (s && s->valid) {
			if (s->consume_hp > ca->hp) {
				return -1;
			}

			if (s->consume_mp > ca->mp) {
				return -2;
			}

			ca->hp -= s->consume_hp;
			ca->mp -= s->consume_mp;
		} else {
			return -3;
		}

		pLwc->battle_state = LBS_COMMAND_IN_PROGRESS;
		pLwc->command_in_progress_anim.t = pLwc->command_in_progress_anim.max_t = 1;
		pLwc->command_in_progress_anim.max_v = 1;


		LWENEMY* enemy = &pLwc->enemy[enemy_slot];
		LWBATTLECREATURE* cb = &enemy->c;

		LWBATTLECOMMAND cmd;
		cmd.skill = s;

		LWBATTLECOMMANDRESULT cmd_result_a;
		LWBATTLECOMMANDRESULT cmd_result_b;

		calculate_battle_command_result(ca, cb, &cmd, &cmd_result_a, &cmd_result_b);

		apply_battle_command_result(ca, &cmd_result_a);
		apply_battle_command_result(cb, &cmd_result_b);

		if (cb->hp <= 0) {
			enemy->death_anim.v0[4] = 1; // Phase 0 (last): alpha remove max
			enemy->death_anim.v1[0] = 1; enemy->death_anim.v1[3] = 1; // Phase 1 (middle): full red
			enemy->death_anim.v2[3] = 1; // Phase 2 (start): full black
			enemy->death_anim.anim_1d.t = enemy->death_anim.anim_1d.max_t = 0.45f;
		}

		const float enemy_x = get_battle_enemy_x_center(enemy_slot);

		char damage_str[128];

		if (cmd_result_a.type == LBCR_MISSED) {
			snprintf(damage_str, ARRAY_SIZE(damage_str), "MISSED");

			enemy->evasion_anim.t = enemy->evasion_anim.max_t = 0.25f;
			enemy->evasion_anim.max_v = 0.15f;
		} else {
			// 데미지는 음수이기 때문에 - 붙여서 양수로 바꿔줌
			snprintf(damage_str, ARRAY_SIZE(damage_str), "%d", -cmd_result_b.delta_hp);

			enemy->shake_duration = 0.15f;
			enemy->shake_magitude = 0.03f;
		}

		// TODO: MISSED 일 때 트레일을 그리지 않으면 전투가 도중에 멈추는 문제가 있어서 무조건 그려줌
		spawn_attack_trail(pLwc, enemy_x, -0.1f, 0.5f);

		spawn_damage_text(pLwc, 0, 0, 0, damage_str);

		pLwc->battle_fov_deg = pLwc->battle_fov_mag_deg_0;

		pLwc->battle_cam_center_x = enemy_x;

		return 0;
	} else {
		return -4;
	}
}

void lw_trigger_key_enter(LWCONTEXT *pLwc) {

	// battle

	if (pLwc->battle_state == LBS_SELECT_COMMAND && pLwc->player_turn_creature_index >= 0) {
		pLwc->battle_state = LBS_SELECT_TARGET;

		for (int i = 0; i < ARRAY_SIZE(pLwc->enemy); i++) {
			if (pLwc->enemy[i].valid && pLwc->enemy[i].c.hp > 0) {
				pLwc->selected_enemy_slot = i;
				break;
			}
		}
	} else if (pLwc->battle_state == LBS_SELECT_TARGET) {

		exec_attack(pLwc, pLwc->selected_enemy_slot);
	}
}

#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
long lw_get_last_time_sec(LWCONTEXT* pLwc) {
	return 0;
}

long lw_get_last_time_nsec(LWCONTEXT* pLwc) {
	return 0;
}
#else
long lw_get_last_time_sec(LWCONTEXT *pLwc) {
	return pLwc->last_time.tv_sec;
}

long lw_get_last_time_nsec(LWCONTEXT *pLwc) {
	return pLwc->last_time.tv_nsec;
}
#endif

double lw_get_delta_time(LWCONTEXT *pLwc) {
	return pLwc->delta_time;
}

int lw_get_update_count(LWCONTEXT *pLwc) {
	return pLwc->update_count;
}

int lw_get_render_count(LWCONTEXT *pLwc) {
	return pLwc->render_count;
}

void lw_on_destroy(LWCONTEXT *pLwc) {
	release_font(pLwc->pFnt);
	release_string(pLwc->dialog);
}

void lw_press_key_left(LWCONTEXT *pLwc) {
	pLwc->player_move_left = 1;
}

void lw_press_key_right(LWCONTEXT *pLwc) {
	pLwc->player_move_right = 1;
}

void lw_press_key_up(LWCONTEXT *pLwc) {
	pLwc->player_move_up = 1;
}

void lw_press_key_down(LWCONTEXT *pLwc) {
	pLwc->player_move_down = 1;
}

void lw_release_key_left(LWCONTEXT *pLwc) {
	pLwc->player_move_left = 0;
}

void lw_release_key_right(LWCONTEXT *pLwc) {
	pLwc->player_move_right = 0;
}

void lw_release_key_up(LWCONTEXT *pLwc) {
	pLwc->player_move_up = 0;
}

void lw_release_key_down(LWCONTEXT *pLwc) {
	pLwc->player_move_down = 0;
}

int spawn_field_object(struct _LWCONTEXT *pLwc, float x, float y, float w, float h, enum _LW_VBO_TYPE lvt,
	unsigned int tex_id, float sx, float sy) {
	for (int i = 0; i < MAX_FIELD_OBJECT; i++) {
		if (!pLwc->field_object[i].valid) {
			pLwc->field_object[i].x = x;
			pLwc->field_object[i].y = y;
			pLwc->field_object[i].sx = sx;
			pLwc->field_object[i].sy = sy;
			pLwc->field_object[i].lvt = lvt;
			pLwc->field_object[i].tex_id = tex_id;
			pLwc->field_object[i].valid = 1;

			pLwc->box_collider[i].x = x;
			pLwc->box_collider[i].y = y;
			pLwc->box_collider[i].w = w * sx;
			pLwc->box_collider[i].h = h * sy;
			pLwc->box_collider[i].valid = 1;

			return i;
		}
	}

	return -1;
}

int spawn_attack_trail(LWCONTEXT *pLwc, float x, float y, float z) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (!pLwc->trail[i].valid) {
			pLwc->trail[i].x = x;
			pLwc->trail[i].y = y;
			pLwc->trail[i].z = z;
			pLwc->trail[i].age = 0;
			pLwc->trail[i].max_age = 1;
			pLwc->trail[i].tex_coord_speed = 5;
			pLwc->trail[i].tex_coord = -1;
			pLwc->trail[i].valid = 1;
			return i;
		}
	}

	return -1;
}

void update_next_player_turn_creature(LWCONTEXT* pLwc) {

	if (pLwc->player_turn_creature_index >= 0) {
		pLwc->player_creature[pLwc->player_turn_creature_index].selected = 0;
		pLwc->player_creature[pLwc->player_turn_creature_index].turn_consumed = 1;
		const int next_turn_token = pLwc->player_creature[pLwc->player_turn_creature_index].turn_token + 1;

		for (int i = 0; i < MAX_BATTLE_CREATURE; i++) {
			if (pLwc->player_creature[i].turn_token == next_turn_token) {
				pLwc->player_creature[i].selected = 1;
				pLwc->player_turn_creature_index = i;
				return;
			}
		}

		pLwc->player_turn_creature_index = -1;
	}
}

void update_attack_trail(LWCONTEXT *pLwc) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (pLwc->trail[i].valid) {
			pLwc->trail[i].age += (float)pLwc->delta_time;
			pLwc->trail[i].tex_coord += (float)(pLwc->trail[i].tex_coord_speed * pLwc->delta_time);

			if (pLwc->trail[i].age >= pLwc->trail[i].max_age) {
				pLwc->trail[i].valid = 0;

				pLwc->battle_fov_deg = pLwc->battle_fov_deg_0;
				pLwc->battle_cam_center_x = 0;

				update_next_player_turn_creature(pLwc);

				pLwc->battle_state = LBS_SELECT_COMMAND;
				pLwc->selected_command_slot = 0;
			}
		}
	}
}

int spawn_damage_text(LWCONTEXT *pLwc, float x, float y, float z, const char *text) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (!pLwc->damage_text[i].valid) {
			LWDAMAGETEXT *dt = &pLwc->damage_text[i];

			dt->x = x;
			dt->y = y;
			dt->z = z;
			dt->age = 0;
			dt->max_age = 1;

			dt->valid = 1;
			strncpy(dt->text, text, ARRAY_SIZE(dt->text));
			dt->text[ARRAY_SIZE(dt->text) - 1] = '\0';

			LWTEXTBLOCK *tb = &dt->text_block;

			tb->text = dt->text;
			tb->text_bytelen = (int)strlen(tb->text);
			tb->begin_index = 0;
			tb->end_index = tb->text_bytelen;

			tb->text_block_x = x;
			tb->text_block_y = y;
			tb->text_block_width = 1;
			tb->text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT;
			tb->size = DEFAULT_TEXT_BLOCK_SIZE;

			SET_COLOR_RGBA_FLOAT(tb->color_normal_glyph, 1, 0.25f, 0.25f, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_normal_outline, 0.1f, 0.1f, 0.1f, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_emp_glyph, 1, 1, 0, 1);
			SET_COLOR_RGBA_FLOAT(tb->color_emp_outline, 0, 0, 0, 1);

			return i;
		}
	}

	return -1;
}

void update_damage_text(LWCONTEXT *pLwc) {
	for (int i = 0; i < MAX_TRAIL; i++) {
		if (pLwc->damage_text[i].valid) {
			pLwc->damage_text[i].age += (float)pLwc->delta_time;

			//pLwc->damage_text[i].text_block.text_block_x += (float)(0.2 * cos(LWDEG2RAD(60)) * pLwc->delta_time);
			pLwc->damage_text[i].text_block.text_block_y += (float)(0.2 * sin(LWDEG2RAD(60)) *
				pLwc->delta_time);

			const float t = pLwc->damage_text[i].age;
			const float expand_time = 0.15f;
			const float retain_time = 0.5f;
			const float contract_time = 0.15f;
			const float max_size = 1.0f;
			if (t < expand_time) {
				pLwc->damage_text[i].text_block.size = max_size / expand_time * t;
			} else if (expand_time <= t && t < expand_time + retain_time) {
				pLwc->damage_text[i].text_block.size = max_size;
			} else if (t < expand_time + retain_time + contract_time) {
				pLwc->damage_text[i].text_block.size = -(max_size / contract_time) * (t -
					(expand_time +
						retain_time +
						contract_time));
			} else {
				pLwc->damage_text[i].text_block.size = 0;
			}

			if (pLwc->damage_text[i].age >= pLwc->damage_text[i].max_age) {
				pLwc->damage_text[i].valid = 0;
			}
		}
	}
}

void change_to_field(LWCONTEXT *pLwc) {
	pLwc->game_scene = LGS_FIELD;
}

void change_to_dialog(LWCONTEXT *pLwc) {
	pLwc->game_scene = LGS_DIALOG;
}

void change_to_battle(LWCONTEXT *pLwc) {
	pLwc->game_scene = LGS_BATTLE;
}

void change_to_font_test(LWCONTEXT *pLwc) {
	pLwc->game_scene = LGS_FONT_TEST;
}

void change_to_admin(LWCONTEXT *pLwc) {
	pLwc->game_scene = LGS_ADMIN;
}

void toggle_font_texture_test_mode(LWCONTEXT *pLwc) {
	pLwc->font_texture_texture_mode = !pLwc->font_texture_texture_mode;
}
