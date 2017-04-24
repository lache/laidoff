#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>

#include "lwgl.h"
#include "laidoff.h"
#include "lwbitmapcontext.h"
#include "constants.h"
#include "file.h"
#include "font.h"
#include "lwcontext.h"
#include "lwlog.h"
#include "ktx.h"
#include "script.h"
#include "unicode.h"
#include "dialog.h"
#include "tex.h"
#include "lwmacro.h"
#include "lwenemy.h"
#include "battle.h"
#include "render_font_test.h"
#include "render_admin.h"
#include "lwkeyframe.h"
#include "input.h"
#include "field.h"
#include "platform_detection.h"
#include "lwpkm.h"
#include "render_battle_result.h"
#include "battle_result.h"
#include "net.h"

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

#define LW_SUPPORT_ETC1_HARDWARE_DECODING LW_PLATFORM_ANDROID
#define LW_SUPPORT_VAO (LW_PLATFORM_WIN32 || LW_PLATFORM_OSX)

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
void update_damage_text(LWCONTEXT *pLwc);

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

	pShader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);

	const GLchar* vstarray[] = { vst };
	glShaderSource(pShader->vertex_shader, 1, vstarray, NULL);
	glCompileShader(pShader->vertex_shader);
	GLint isCompiled = 0;
	glGetShaderiv(pShader->vertex_shader, GL_COMPILE_STATUS, &isCompiled);
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
	const GLchar* fstarray[] = { fst };
	glShaderSource(pShader->fragment_shader, 1, fstarray, NULL);
	glCompileShader(pShader->fragment_shader);
	glGetShaderiv(pShader->fragment_shader, GL_COMPILE_STATUS, &isCompiled);
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
	glAttachShader(pShader->program, pShader->vertex_shader);
	glAttachShader(pShader->program, pShader->fragment_shader);
	glLinkProgram(pShader->program);
	//Note the different functions here: glGetProgram* instead of glGetShader*.
	GLint isLinked = 0;
	glGetProgramiv(pShader->program, GL_LINK_STATUS, (int *)&isLinked);
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
	pShader->vuvoffset_location = glGetUniformLocation(pShader->program, "vUvOffset");
	pShader->vuvscale_location = glGetUniformLocation(pShader->program, "vUvScale");
	pShader->vs9offset_location = glGetUniformLocation(pShader->program, "vS9Offset");
	pShader->alpha_multiplier_location = glGetUniformLocation(pShader->program, "alpha_multiplier");
	pShader->overlay_color_location = glGetUniformLocation(pShader->program, "overlay_color");
	pShader->overlay_color_ratio_location = glGetUniformLocation(pShader->program,
		"overlay_color_ratio");
	pShader->diffuse_location = glGetUniformLocation(pShader->program, "diffuse");
	pShader->alpha_only_location = glGetUniformLocation(pShader->program, "alpha_only");
	pShader->glyph_color_location = glGetUniformLocation(pShader->program, "glyph_color");
	pShader->outline_color_location = glGetUniformLocation(pShader->program, "outline_color");

	// Attribs
	pShader->vpos_location = glGetAttribLocation(pShader->program, "vPos");
	pShader->vcol_location = glGetAttribLocation(pShader->program, "vCol");
	pShader->vuv_location = glGetAttribLocation(pShader->program, "vUv");
	pShader->vs9_location = glGetAttribLocation(pShader->program, "vS9");

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
	glEnableVertexAttribArray(pLwc->shader[shader_index].vpos_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
		stride_in_bytes, (void *)0);
	glEnableVertexAttribArray(pLwc->shader[shader_index].vcol_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vcol_location, 3, GL_FLOAT, GL_FALSE,
		stride_in_bytes, (void *)(sizeof(float) * 3));
	glEnableVertexAttribArray(pLwc->shader[shader_index].vuv_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vuv_location, 2, GL_FLOAT, GL_FALSE,
		stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
	glEnableVertexAttribArray(pLwc->shader[shader_index].vs9_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vs9_location, 2, GL_FLOAT, GL_FALSE,
		stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2)));
}

static void init_vao(LWCONTEXT *pLwc, int shader_index) {
#if LW_SUPPORT_VAO
	glGenVertexArrays(VERTEX_BUFFER_COUNT, pLwc->vao);
	for (int i = 0; i < VERTEX_BUFFER_COUNT; i++) {
		glBindVertexArray(pLwc->vao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[i].vertex_buffer);
		set_vertex_attrib_pointer(pLwc, shader_index);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
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

void set_creature_data(LWBATTLECREATURE* c, const char* name, int lv, int hp, int max_hp, int mp, int max_mp,
	int turn_token, int exp, int max_exp) {
	c->valid = 1;
	//strcpy(c->name, name);
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
	c->exp = exp;
	c->max_exp = max_exp;
}

void reset_battle_context(LWCONTEXT* pLwc) {
	pLwc->battle_fov_deg_0 = 49.134f;
	pLwc->battle_fov_mag_deg_0 = 35.134f;
	pLwc->battle_fov_deg = pLwc->battle_fov_deg_0;
	pLwc->selected_enemy_slot = -1;

	const int enemy_count = LWMIN(pLwc->field_event_id, MAX_ENEMY_SLOT);
	// Fill enemy slots with test enemies
	for (int i = 0; i < enemy_count; i++) {
		pLwc->enemy[i] = ENEMY_DATA_LIST[i];

		for (int j = 0; j < MAX_SKILL_PER_CREATURE; j++) {
			pLwc->enemy[i].c.skill[j] = &SKILL_DATA_LIST[j];
		}
	}
	// Invalidate empty enemy slot
	for (int i = enemy_count; i < MAX_ENEMY_SLOT; i++) {
		pLwc->enemy[i].valid = 0;
	}
	
	memset(pLwc->player, 0, sizeof(pLwc->player));

	for (int i = 0; i < MAX_PLAYER_SLOT; i++) {
		pLwc->player[i] = ENEMY_DATA_LIST[LET_TEST_PLAYER_1 + i].c;
		pLwc->player[i].valid = 0;
	}
	
	set_creature_data(
		&pLwc->player[0],
		u8"주인공",
		1,
		50,
		50,
		30,
		30,
		1,
		5,
		38
	);
	pLwc->player[0].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player[0].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player[0].skill[1] = &SKILL_DATA_LIST[1];
	pLwc->player[0].skill[2] = &SKILL_DATA_LIST[2];
	pLwc->player[0].skill[3] = &SKILL_DATA_LIST[3];
	pLwc->player[0].skill[4] = &SKILL_DATA_LIST[4];
	pLwc->player[0].skill[5] = &SKILL_DATA_LIST[5];

	set_creature_data(
		&pLwc->player[1],
		u8"극작가",
		2,
		25,
		50,
		30,
		30,
		2,
		1,
		30
	);
	pLwc->player[1].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player[1].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player[1].skill[1] = &SKILL_DATA_LIST[2];

	set_creature_data(
		&pLwc->player[2],
		u8"손톱깎이",
		3,
		46,
		46,
		15,
		55,
		3,
		0,
		50
	);
	pLwc->player[2].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player[2].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player[2].skill[1] = &SKILL_DATA_LIST[3];

	set_creature_data(
		&pLwc->player[3],
		u8"대문호",
		8,
		105,
		105,
		0,
		20,
		4,
		45,
		56
	);
	pLwc->player[3].stat = ENEMY_DATA_LIST[0].c.stat;
	pLwc->player[3].skill[0] = &SKILL_DATA_LIST[0];
	pLwc->player[3].skill[1] = &SKILL_DATA_LIST[4];

	for (int i = 0; i < MAX_PLAYER_SLOT; i++) {
		pLwc->player[i].selected = 0;
	}

	pLwc->player[0].selected = 1;

	pLwc->player_turn_creature_index = 0;

	pLwc->battle_state = LBS_SELECT_COMMAND;

	pLwc->selected_command_slot = 0;
}

void reset_field_context(LWCONTEXT* pLwc) {

	despawn_all_field_object(pLwc);

	spawn_all_field_object(pLwc);
	
	spawn_field_object(pLwc, 0, 5, 2, 2, LVT_HOME, pLwc->tex_programmed[LPT_SOLID_GREEN], 1, 1, 1, 0);

	spawn_field_object(pLwc, 0, -7, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_BLUE], 6,
		1, 1, 0);

	pLwc->player_pos_x = 0;
	pLwc->player_pos_y = 0;
}

void reset_runtime_context(LWCONTEXT* pLwc) {

	pLwc->sprite_data = SPRITE_DATA[0];

	pLwc->next_game_scene = LGS_FIELD;

	pLwc->font_texture_texture_mode = 0;

	reset_battle_context(pLwc);

	reset_field_context(pLwc);

	pLwc->font_fbo.dirty = 1;

	pLwc->admin_button_command[0].name = LWU("필드");
	pLwc->admin_button_command[0].command_handler = change_to_field;
	pLwc->admin_button_command[1].name = LWU("대화");
	pLwc->admin_button_command[1].command_handler = change_to_dialog;
	pLwc->admin_button_command[2].name = LWU("전투");
	pLwc->admin_button_command[2].command_handler = change_to_battle;
	pLwc->admin_button_command[3].name = LWU("글꼴");
	pLwc->admin_button_command[3].command_handler = change_to_font_test;
	pLwc->admin_button_command[4].name = LWU("런타임리셋");
	pLwc->admin_button_command[4].command_handler = reset_runtime_context;
	pLwc->admin_button_command[5].name = LWU("글꼴디버그");
	pLwc->admin_button_command[5].command_handler = toggle_font_texture_test_mode;
	pLwc->admin_button_command[6].name = LWU("UDP");
	pLwc->admin_button_command[6].command_handler = net_rtt_test;
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

	init_lua(pLwc);

	reset_runtime_context(pLwc);

	//pLwc->pFnt = load_fnt(ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "arita-semi-bold.fnt");
	pLwc->pFnt = load_fnt(ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test6.fnt");

	pLwc->dialog = create_string_from_file(ASSETS_BASE_PATH "d" PATH_SEPARATOR "d1.txt");
	if (pLwc->dialog) {
		pLwc->dialog_bytelen = (int)strlen(pLwc->dialog);
	} else {
		LOGE("dialog loading failed.");
	}
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
	} else if (pLwc->game_scene == LGS_BATTLE_RESULT) {
		lwc_render_battle_result(pLwc);
	}
}

static void update_battle_wall(LWCONTEXT* pLwc) {
	pLwc->battle_wall_tex_v += (float)(pLwc->delta_time / 34);
	pLwc->battle_wall_tex_v = fmodf(pLwc->battle_wall_tex_v, 1.0f);
}

void lwc_update(LWCONTEXT *pLwc, double delta_time) {

	LWTIMEPOINT cur_time;
	lwtimepoint_now(&cur_time);

	pLwc->delta_time = lwtimepoint_diff(&cur_time, &pLwc->last_time);
	pLwc->last_time = cur_time;

	if (pLwc->next_game_scene == LGS_INVALID && pLwc->game_scene == LGS_INVALID) {
		// Default game scene
		pLwc->next_game_scene = LGS_FIELD;
	}

	if (pLwc->next_game_scene != LGS_INVALID && pLwc->next_game_scene != pLwc->game_scene) {

		pLwc->game_scene = pLwc->next_game_scene;
		pLwc->next_game_scene = LGS_INVALID;
		pLwc->scene_time = 0;

		if (pLwc->game_scene == LGS_BATTLE) {
			reset_battle_context(pLwc);
		}
	}

	// accumulate time since app startup
	pLwc->app_time += pLwc->delta_time;
	pLwc->scene_time += pLwc->delta_time;

	update_dialog(pLwc);

	//****//
	// fix delta time
	//pLwc->delta_time = 1.0f / 60;
	//****//

	float ratio = pLwc->width / (float)pLwc->height;

	//LOGV("Update(): width: %d height: %d ratio: %f", pLwc->width, pLwc->height, ratio);

	if (ratio > 1) {
		mat4x4_ortho(pLwc->proj, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
	} else {
		mat4x4_ortho(pLwc->proj, -1.f, 1.f, -1 / ratio, 1 / ratio, 1.f, -1.f);
	}

	update_battle(pLwc);

	move_player(pLwc);

	resolve_player_collision(pLwc);

	update_attack_trail(pLwc);

	update_damage_text(pLwc);

	update_battle_wall(pLwc);

	if (pLwc->font_fbo.dirty) {
		lwc_render_font_test_fbo(pLwc);
		pLwc->font_fbo.dirty = 0;
	}

	if (pLwc->battle_state == LBS_START_PLAYER_WIN || pLwc->battle_state == LBS_PLAYER_WIN_IN_PROGRESS) {
		update_battle_result(pLwc);
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

static void load_pkm_hw_decoding(const char *tex_atlas_filename) {
	size_t file_size = 0;
	char *b = create_binary_from_file(tex_atlas_filename, &file_size);
	if (!b) {
		LOGE("load_pkm_hw_decoding: create_binary_from_file null, filename %s", tex_atlas_filename);
		return;
	}
	LWPKM *pPkm = (LWPKM *)b;

	GLenum error_enum;

	short extended_width = swap_bytes(pPkm->extended_width);
	short extended_height = swap_bytes(pPkm->extended_height);

	// TODO: iOS texture
#if LW_SUPPORT_ETC1_HARDWARE_DECODING
	// calculate size of data with formula (extWidth / 4) * (extHeight / 4) * 8
	u32 dataLength = ((extended_width >> 2) * (extended_height >> 2)) << 3;

	glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_ETC1_RGB8_OES, extended_width, extended_height, 0,
		dataLength, b + sizeof(LWPKM));
#else
	LWBITMAPCONTEXT bitmap_context;
	create_image(tex_atlas_filename, &bitmap_context, 0);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap_context.width, bitmap_context.height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, bitmap_context.data);
	error_enum = glGetError();
	LOGI("glTexImage2D (ETC1 software decompression) result (%dx%d): %d", bitmap_context.width, bitmap_context.height,
		error_enum);

	release_image(&bitmap_context);
#endif

	error_enum = glGetError();
	LOGI("glCompressedTexImage2D result (%dx%d): %d", extended_width, extended_height,
		error_enum);

	release_binary(b);
}

static void load_png_pkm_sw_decoding(LWCONTEXT* pLwc, int i) {
	LWBITMAPCONTEXT bitmap_context;

	create_image(tex_atlas_filename[i], &bitmap_context, i);

	if (bitmap_context.width > 0 && bitmap_context.height > 0) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap_context.width, bitmap_context.height,
			0,
			GL_RGBA, GL_UNSIGNED_BYTE, bitmap_context.data);
		GLenum error_enum = glGetError();
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

static void load_tex_files(LWCONTEXT *pLwc) {
	glGenTextures(MAX_TEX_ATLAS, pLwc->tex_atlas);

	for (int i = 0; i < MAX_TEX_ATLAS; i++) {
		glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[i]);

		size_t tex_atlas_filename_len = (int)strlen(tex_atlas_filename[i]);

		size_t filename_index = tex_atlas_filename_len;
		while (tex_atlas_filename[i][filename_index - 1] != PATH_SEPARATOR[0]) {
			filename_index--;
		}

		if (strcmp(tex_atlas_filename[i] + tex_atlas_filename_len - 4, ".ktx") == 0) {
			if (load_ktx_hw_or_sw(tex_atlas_filename[i]) < 0) {
				LOGE("load_tex_files: load_ktx_hw_or_sw failure - %s", tex_atlas_filename[i]);
			}
		} else if (strcmp(tex_atlas_filename[i] + tex_atlas_filename_len - 4, ".png") == 0) {
			// Software decoding of PNG
			load_png_pkm_sw_decoding(pLwc, i);
		} else if (strcmp(tex_atlas_filename[i] + tex_atlas_filename_len - 4, ".pkm") == 0) {
#if LW_SUPPORT_ETC1_HARDWARE_DECODING
			load_pkm_hw_decoding(tex_atlas_filename[i]);
#else
			load_png_pkm_sw_decoding(pLwc, i);
#endif
		} else {
			LOGE("load_tex_files: unknown tex file extension - %s", tex_atlas_filename[i]);
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

	lwtimepoint_now(&pLwc->last_time);

	init_net(pLwc);

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

int spawn_field_object(struct _LWCONTEXT *pLwc, float x, float y, float w, float h, enum _LW_VBO_TYPE lvt,
	unsigned int tex_id, float sx, float sy, float alpha_multiplier, int field_event_id) {
	for (int i = 0; i < MAX_FIELD_OBJECT; i++) {
		if (!pLwc->field_object[i].valid) {
			pLwc->field_object[i].x = x;
			pLwc->field_object[i].y = y;
			pLwc->field_object[i].sx = sx;
			pLwc->field_object[i].sy = sy;
			pLwc->field_object[i].lvt = lvt;
			pLwc->field_object[i].tex_id = tex_id;
			pLwc->field_object[i].alpha_multiplier = alpha_multiplier;
			pLwc->field_object[i].valid = 1;

			pLwc->box_collider[i].x = x;
			pLwc->box_collider[i].y = y;
			pLwc->box_collider[i].w = w * sx;
			pLwc->box_collider[i].h = h * sy;
			pLwc->box_collider[i].field_event_id = field_event_id;
			pLwc->box_collider[i].valid = 1;

			return i;
		}
	}

	return -1;
}

void change_to_field(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_FIELD;
}

void change_to_dialog(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_DIALOG;
}

void change_to_battle(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_BATTLE;
}

void change_to_font_test(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_FONT_TEST;
}

void change_to_admin(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_ADMIN;
}

void change_to_battle_result(LWCONTEXT *pLwc) {
	pLwc->next_game_scene = LGS_BATTLE_RESULT;
}

long lw_get_last_time_sec(LWCONTEXT *pLwc) {
	return lwtimepoint_get_second_portion(&pLwc->last_time);
}

long lw_get_last_time_nsec(LWCONTEXT *pLwc) {
	return lwtimepoint_get_nanosecond_portion(&pLwc->last_time);
}

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
