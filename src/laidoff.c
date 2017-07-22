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
#include "tex.h"
#include "lwmacro.h"
//#include "lwenemy.h"
#include "render_font_test.h"
#include "render_admin.h"
#include "input.h"
#include "field.h"
#include "platform_detection.h"
#include "lwpkm.h"
#include "render_battle_result.h"
#include "net.h"
#include "render_skin.h"
#include "armature.h"
#include "lwanim.h"
#include "lwskinmesh.h"
#include "render_physics.h"
#include "mq.h"
#include "sysmsg.h"
#include "render_text_block.h"
#include <czmq.h>
#include "logic.h"
#include "ps.h"
#include "render_ps.h"
//#include "script.h"
#include "lwtimepoint.h"

#define LW_SUPPORT_ETC1_HARDWARE_DECODING LW_PLATFORM_ANDROID
#define LW_SUPPORT_VAO (LW_PLATFORM_WIN32 || LW_PLATFORM_OSX)

const float default_uv_offset[2] = { 0, 0 };
const float default_uv_scale[2] = { 1, 1 };
const float default_flip_y_uv_scale[2] = { 1, -1 };

// Vertex attributes: Coordinates (3xf) + Normal (3xf) + UV (2xf) + S9 (2xf)
// See Also: LWVERTEX
const static GLsizei stride_in_bytes = (GLsizei)(sizeof(float) * (3 + 3 + 2 + 2));
LwStaticAssert(sizeof(LWVERTEX) == (GLsizei)(sizeof(float) * (3 + 3 + 2 + 2)), "LWVERTEX size error");

// Skin Vertex attributes: Coordinates (3xf) + Normal (3xf) + UV (2xf) + Bone Weight (4xf) + Bone Matrix (4xi)
// See Also: LWSKINVERTEX
const static GLsizei skin_stride_in_bytes = (GLsizei)(sizeof(float) * (3 + 3 + 2 + 4) + sizeof(int) * 4);
LwStaticAssert(sizeof(LWSKINVERTEX) == (GLsizei)(sizeof(float) * (3 + 3 + 2 + 4) + sizeof(int) * 4), "LWSKINVERTEX size error");

// Fan Vertex attributes: Coordinates (3xf)
// See Also: LWFANVERTEX
const static GLsizei fan_stride_in_bytes = (GLsizei)(sizeof(float) * 3);
LwStaticAssert(sizeof(LWFANVERTEX) == (GLsizei)(sizeof(float) * 3), "LWFANVERTEX size error");


#if LW_PLATFORM_ANDROID || LW_PLATFORM_IOS || LW_PLATFORM_IOS_SIMULATOR
#include "lwtimepoint.h"
double glfwGetTime() {
	LWTIMEPOINT tp;
	lwtimepoint_now(&tp);
	return tp.last_time.tv_sec + (double)tp.last_time.tv_nsec / 1e9;
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
float get_battle_enemy_x_center(int enemy_slot_index);
void init_font_fbo(LWCONTEXT* pLwc);

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

static void delete_shader(LWSHADER *pShader) {
	glDeleteProgram(pShader->program);
	pShader->program = 0;
	glDeleteShader(pShader->vertex_shader);
	pShader->vertex_shader = 0;
	glDeleteShader(pShader->fragment_shader);
	pShader->fragment_shader = 0;
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
	pShader->bone_location = glGetUniformLocation(pShader->program, "bone");
	pShader->rscale_location = glGetUniformLocation(pShader->program, "rscale");
	pShader->thetascale_location = glGetUniformLocation(pShader->program, "thetascale");
	pShader->projection_matrix_location = glGetUniformLocation(pShader->program, "uProjectionMatrix");
	pShader->k_location = glGetUniformLocation(pShader->program, "uK");
	pShader->color_location = glGetUniformLocation(pShader->program, "uColor");
	pShader->time_location = glGetUniformLocation(pShader->program, "uTime");
	pShader->texture_location = glGetUniformLocation(pShader->program, "uTexture");
	pShader->u_ProjectionMatrix = glGetUniformLocation(pShader->program, "u_ProjectionMatrix");
	pShader->u_Gravity = glGetUniformLocation(pShader->program, "u_Gravity");
	pShader->u_Time = glGetUniformLocation(pShader->program, "u_Time");
	pShader->u_eRadius = glGetUniformLocation(pShader->program, "u_eRadius");
	pShader->u_eVelocity = glGetUniformLocation(pShader->program, "u_eVelocity");
	pShader->u_eDecay = glGetUniformLocation(pShader->program, "u_eDecay");
	pShader->u_eSizeStart = glGetUniformLocation(pShader->program, "u_eSizeStart");
	pShader->u_eSizeEnd = glGetUniformLocation(pShader->program, "u_eSizeEnd");
	pShader->u_eColorStart = glGetUniformLocation(pShader->program, "u_eColorStart");
	pShader->u_eColorEnd = glGetUniformLocation(pShader->program, "u_eColorEnd");
	pShader->u_Texture = glGetUniformLocation(pShader->program, "u_Texture");
	pShader->u_TextureAlpha = glGetUniformLocation(pShader->program, "u_TextureAlpha");

	// Attribs
	pShader->vpos_location = glGetAttribLocation(pShader->program, "vPos");
	pShader->vcol_location = glGetAttribLocation(pShader->program, "vCol");
	pShader->vuv_location = glGetAttribLocation(pShader->program, "vUv");
	pShader->vs9_location = glGetAttribLocation(pShader->program, "vS9");
	pShader->vbweight_location = glGetAttribLocation(pShader->program, "vBw");
	pShader->vbmat_location = glGetAttribLocation(pShader->program, "vBm");
	pShader->theta_location = glGetAttribLocation(pShader->program, "aTheta");
	pShader->shade_location = glGetAttribLocation(pShader->program, "aShade");
	pShader->a_pID = glGetAttribLocation(pShader->program, "a_pID");
	pShader->a_pRadiusOffset = glGetAttribLocation(pShader->program, "a_pRadiusOffset");
	pShader->a_pVelocityOffset = glGetAttribLocation(pShader->program, "a_pVelocityOffset");
	pShader->a_pDecayOffset = glGetAttribLocation(pShader->program, "a_pDecayOffset");
	pShader->a_pSizeOffset = glGetAttribLocation(pShader->program, "a_pSizeOffset");
	pShader->a_pColorOffset = glGetAttribLocation(pShader->program, "a_pColorOffset");

	pShader->valid = 1;
}

void init_gl_shaders(LWCONTEXT *pLwc) {

#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
#define GLSL_DIR_NAME "glsl"
#else
#define GLSL_DIR_NAME "glsles"
#endif

	// Vertex Shader
	char *default_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "default-vert.glsl");
	char *skin_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "skin-vert.glsl");
	char *fan_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "fan-vert.glsl");
	char *emitter_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "emitter-vert.glsl");
	char *emitter2_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "emitter2-vert.glsl");

	// Fragment Shader
	char *default_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "default-frag.glsl");
	char *font_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "font-frag.glsl");
	char *etc1_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "etc1-frag.glsl");
	char *fan_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "fan-frag.glsl");
	char *emitter_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "emitter-frag.glsl");
	char *emitter2_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
		GLSL_DIR_NAME PATH_SEPARATOR "emitter2-frag.glsl");

	if (!default_vert_glsl) {
		LOGE("init_gl_shaders: default-vert.glsl not loaded. Abort...");
		return;
	}

	if (!skin_vert_glsl) {
		LOGE("init_gl_shaders: skin-vert.glsl not loaded. Abort...");
		return;
	}

	if (!fan_vert_glsl) {
		LOGE("init_gl_shaders: fan-vert.glsl not loaded. Abort...");
		return;
	}

	if (!emitter_vert_glsl) {
		LOGE("init_gl_shaders: emitter-vert.glsl not loaded. Abort...");
		return;
	}

	if (!emitter2_vert_glsl) {
		LOGE("init_gl_shaders: emitter2-vert.glsl not loaded. Abort...");
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

	if (!fan_frag_glsl) {
		LOGE("init_gl_shaders: fan-frag.glsl not loaded. Abort...");
		return;
	}

	if (!emitter_frag_glsl) {
		LOGE("init_gl_shaders: emitter-frag.glsl not loaded. Abort...");
		return;
	}

	if (!emitter2_frag_glsl) {
		LOGE("init_gl_shaders: emitter2-frag.glsl not loaded. Abort...");
		return;
	}

	create_shader("Default Shader", &pLwc->shader[LWST_DEFAULT], default_vert_glsl, default_frag_glsl);
	create_shader("Font Shader", &pLwc->shader[LWST_FONT], default_vert_glsl, font_frag_glsl);
	create_shader("ETC1 with Alpha Shader", &pLwc->shader[LWST_ETC1], default_vert_glsl, etc1_frag_glsl);
	create_shader("Skin Shader", &pLwc->shader[LWST_SKIN], skin_vert_glsl, default_frag_glsl);
	create_shader("Fan Shader", &pLwc->shader[LWST_FAN], fan_vert_glsl, fan_frag_glsl);
	create_shader("Emitter Shader", &pLwc->shader[LWST_EMITTER], emitter_vert_glsl, emitter_frag_glsl);
	create_shader("Emitter2 Shader", &pLwc->shader[LWST_EMITTER2], emitter2_vert_glsl, emitter2_frag_glsl);

	release_string(default_vert_glsl);
	release_string(skin_vert_glsl);
	release_string(fan_vert_glsl);
	release_string(emitter_vert_glsl);
	release_string(emitter2_vert_glsl);
	release_string(default_frag_glsl);
	release_string(font_frag_glsl);
	release_string(etc1_frag_glsl);
	release_string(fan_frag_glsl);
	release_string(emitter_frag_glsl);
	release_string(emitter2_frag_glsl);
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

static void load_skin_vbo(LWCONTEXT *pLwc, const char *filename, LWVBO *pSvbo) {
	GLuint vbo = 0;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	size_t mesh_file_size = 0;
	char *mesh_vbo_data = create_binary_from_file(filename, &mesh_file_size);
	LWSKINVERTEX* mesh_vbo_data_debug = (LWSKINVERTEX*)mesh_vbo_data;
	glBufferData(GL_ARRAY_BUFFER, mesh_file_size, mesh_vbo_data, GL_STATIC_DRAW);
	release_binary(mesh_vbo_data);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	pSvbo->vertex_buffer = vbo;
	pSvbo->vertex_count = (int)(mesh_file_size / skin_stride_in_bytes);
}

static void load_fan_vbo(LWCONTEXT* pLwc) {
	LWFANVERTEX fan_vertices[FAN_VERTEX_COUNT_PER_ARRAY];
	fan_vertices[0].r = 0;
	fan_vertices[0].theta = 0;
	fan_vertices[0].index = 0;
	for (int i = 1; i < FAN_VERTEX_COUNT_PER_ARRAY; i++) {
		fan_vertices[i].r = 1.0f;
		fan_vertices[i].theta = (float)((i - 1) * (2 * M_PI / FAN_SECTOR_COUNT_PER_ARRAY));
		fan_vertices[i].index = (float)i;
	}
	glGenBuffers(1, &pLwc->fan_vertex_buffer[LFVT_DEFAULT].vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, pLwc->fan_vertex_buffer[LFVT_DEFAULT].vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(LWFANVERTEX) * FAN_VERTEX_COUNT_PER_ARRAY,
		fan_vertices, GL_STATIC_DRAW);
	pLwc->fan_vertex_buffer[LFVT_DEFAULT].vertex_count = FAN_VERTEX_COUNT_PER_ARRAY;
}

static void init_vbo(LWCONTEXT *pLwc) {

	// === STATIC MESHES ===

	// LVT_BATTLE_BOWL_OUTER
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Bowl_Outer.vbo",
		&pLwc->vertex_buffer[LVT_BATTLE_BOWL_OUTER]);

	// LVT_BATTLE_BOWL_INNER
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Bowl_Inner.vbo",
		&pLwc->vertex_buffer[LVT_BATTLE_BOWL_INNER]);

	// LVT_ENEMY_SCOPE
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "EnemyScope.vbo",
		&pLwc->vertex_buffer[LVT_ENEMY_SCOPE]);

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

	// LVT_FLOOR
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Floor.vbo",
		&pLwc->vertex_buffer[LVT_FLOOR]);

	// LVT_FLOOR2
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Floor2.vbo",
		&pLwc->vertex_buffer[LVT_FLOOR2]);

	// LVT_ROOM
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "room.vbo",
		&pLwc->vertex_buffer[LVT_ROOM]);

	// LVT_BATTLEGROUND_FLOOR
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "battleground-floor.vbo",
		&pLwc->vertex_buffer[LVT_BATTLEGROUND_FLOOR]);

	// LVT_BATTLEGROUND_WALL
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "battleground-wall.vbo",
		&pLwc->vertex_buffer[LVT_BATTLEGROUND_WALL]);

	// LVT_SPHERE
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Sphere.vbo",
		&pLwc->vertex_buffer[LVT_SPHERE]);

	// LVT_APT
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Apt.vbo",
		&pLwc->vertex_buffer[LVT_APT]);

	// LVT_BEAM
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Beam.vbo",
		&pLwc->vertex_buffer[LVT_BEAM]);

	// LVT_PUMP
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "Pump.vbo",
		&pLwc->vertex_buffer[LVT_PUMP]);

	// LVT_OIL_TRUCK
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "OilTruck.vbo",
		&pLwc->vertex_buffer[LVT_OIL_TRUCK]);

	// LVT_CROSSBOW_ARROW
	load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "crossbow-arrow.vbo",
		&pLwc->vertex_buffer[LVT_CROSSBOW_ARROW]);

	// LVT_LEFT_TOP_ANCHORED_SQUARE ~ LVT_RIGHT_BOTTOM_ANCHORED_SQUARE
	// 9 anchored squares...
	const float anchored_square_offset[][2] = {
		{ +1, -1 },{ +0, -1 },{ -1, -1 },
		{ +1, +0 },{ +0, +0 },{ -1, +0 },
		{ +1, +1 },{ +0, +1 },{ -1, +1 },
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

	// === SKIN VERTEX BUFFERS ===

	// LSVT_TRIANGLE
	load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "Triangle.svbo",
		&pLwc->skin_vertex_buffer[LSVT_TRIANGLE]);

	// LSVT_TREEPLANE
	load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "TreePlane.svbo",
		&pLwc->skin_vertex_buffer[LSVT_TREEPLANE]);

	// LSVT_HUMAN
	load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "Human.svbo",
		&pLwc->skin_vertex_buffer[LSVT_HUMAN]);

	// LSVT_DETACHPLANE
	load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "DetachPlane.svbo",
		&pLwc->skin_vertex_buffer[LSVT_DETACHPLANE]);

	// LSVT_GUNTOWER
	load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "guntower.svbo",
		&pLwc->skin_vertex_buffer[LSVT_GUNTOWER]);

	// LSVT_TURRET
	load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "turret.svbo",
		&pLwc->skin_vertex_buffer[LSVT_TURRET]);

	// LSVT_CROSSBOW
	load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "crossbow.svbo",
		&pLwc->skin_vertex_buffer[LSVT_CROSSBOW]);

	// === STATIC MESHES (FAN TYPE) ===
	load_fan_vbo(pLwc);
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

void set_skin_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
	glEnableVertexAttribArray(pLwc->shader[shader_index].vpos_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
		skin_stride_in_bytes, (void *)0);
	glEnableVertexAttribArray(pLwc->shader[shader_index].vcol_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vcol_location, 3, GL_FLOAT, GL_FALSE,
		skin_stride_in_bytes, (void *)(sizeof(float) * 3));
	glEnableVertexAttribArray(pLwc->shader[shader_index].vuv_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vuv_location, 2, GL_FLOAT, GL_FALSE,
		skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
	glEnableVertexAttribArray(pLwc->shader[shader_index].vbweight_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vbweight_location, 4, GL_FLOAT, GL_FALSE,
		skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2)));
	glEnableVertexAttribArray(pLwc->shader[shader_index].vbmat_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vbmat_location, 4, GL_FLOAT, GL_FALSE,
		skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2 + 4)));
}

void set_fan_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
	glEnableVertexAttribArray(pLwc->shader[shader_index].vpos_location);
	glVertexAttribPointer(pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
		fan_stride_in_bytes, (void *)0);
}

void set_ps_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pID);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pRadiusOffset);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pVelocityOffset);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pDecayOffset);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pSizeOffset);
	glEnableVertexAttribArray(pLwc->shader[shader_index].a_pColorOffset);

	glVertexAttribPointer(pLwc->shader[shader_index].a_pID, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pId)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pRadiusOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pRadiusOffset)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pVelocityOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pVelocityOffset)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pDecayOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pDecayOffset)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pSizeOffset, 1, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pSizeOffset)));
	glVertexAttribPointer(pLwc->shader[shader_index].a_pColorOffset, 3, GL_FLOAT, GL_FALSE, sizeof(LWPARTICLE2), (void*)(LWOFFSETOF(LWPARTICLE2, pColorOffset)));
}

static void init_vao(LWCONTEXT *pLwc, int shader_index) {
	// Vertex Array Objects
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

static void init_skin_vao(LWCONTEXT *pLwc, int shader_index) {
	// Skin Vertex Array Objects
#if LW_SUPPORT_VAO
	glGenVertexArrays(SKIN_VERTEX_BUFFER_COUNT, pLwc->skin_vao);
	for (int i = 0; i < SKIN_VERTEX_BUFFER_COUNT; i++) {
		glBindVertexArray(pLwc->skin_vao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, pLwc->skin_vertex_buffer[i].vertex_buffer);
		set_skin_vertex_attrib_pointer(pLwc, shader_index);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
#endif
}

static void init_fan_vao(LWCONTEXT *pLwc, int shader_index) {
	// Skin Vertex Array Objects
#if LW_SUPPORT_VAO
	glGenVertexArrays(FAN_VERTEX_BUFFER_COUNT, pLwc->fan_vao);
	for (int i = 0; i < FAN_VERTEX_BUFFER_COUNT; i++) {
		glBindVertexArray(pLwc->fan_vao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, pLwc->fan_vertex_buffer[i].vertex_buffer);
		set_fan_vertex_attrib_pointer(pLwc, shader_index);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
#endif
}

static void init_ps_vao(LWCONTEXT *pLwc, int shader_index) {
	// Particle System Vertex Array Objects
#if LW_SUPPORT_VAO
	glGenVertexArrays(PS_VERTEX_BUFFER_COUNT, pLwc->ps_vao);
	for (int i = 0; i < PS_VERTEX_BUFFER_COUNT; i++) {
		glBindVertexArray(pLwc->ps_vao[i]);
		glBindBuffer(GL_ARRAY_BUFFER, pLwc->particle_buffer2);
		set_ps_vertex_attrib_pointer(pLwc, shader_index);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
#endif
}

void lw_clear_color() {
	// Alpha component should be 1 in RPI platform.
	glClearColor(90 / 255.f, 173 / 255.f, 255 / 255.f, 1);
}

void init_ps(LWCONTEXT* pLwc) {
	ps_load_emitter(pLwc);
	ps_load_particles(pLwc);
}

static void init_gl_context(LWCONTEXT *pLwc) {
	init_gl_shaders(pLwc);

	init_vbo(pLwc);
	// Particle system's VAOs are configured here. Should be called before setting VAOs.
	init_ps(pLwc);

	init_vao(pLwc, 0/* ??? */);

	init_skin_vao(pLwc, LWST_SKIN);

	init_fan_vao(pLwc, LWST_FAN);

	init_ps_vao(pLwc, LWST_EMITTER2);

	// Enable culling (CCW is default)
	glEnable(GL_CULL_FACE);

	//glEnable(GL_ALPHA_TEST);
	//glCullFace(GL_CW);
	//glDisable(GL_CULL_FACE);
	lw_clear_color();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_ONE, GL_ONE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//glDepthMask(GL_TRUE);
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

void render_stat(const LWCONTEXT* pLwc) {

	const float aspect_ratio = (float)pLwc->width / pLwc->height;

	LWTEXTBLOCK text_block;
	text_block.align = LTBA_LEFT_TOP;
	text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
	text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
	text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
	SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
	char msg[32];
	sprintf(msg, "L:%.1f\nR:%.1f",
		(float)(1.0 / deltatime_history_avg(pLwc->update_dt)),
		(float)(1.0 / deltatime_history_avg(pLwc->render_dt)));
	text_block.text = msg;
	text_block.text_bytelen = (int)strlen(text_block.text);
	text_block.begin_index = 0;
	text_block.end_index = text_block.text_bytelen;
	text_block.text_block_x = -aspect_ratio;
	text_block.text_block_y = 1.0f;
	text_block.multiline = 1;
	render_text_block(pLwc, &text_block);
}

void handle_rmsg_spawn(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
	for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
		if (!pLwc->render_command[i].key) {
			pLwc->render_command[i] = *cmd;
			return;
		}
	}
	LOGE(LWLOGPOS "maximum capacity exceeded");
	abort();
}

void handle_rmsg_anim(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
	for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
		if (pLwc->render_command[i].key == cmd->key) {
			pLwc->render_command[i].animstarttime = lwtimepoint_now_seconds();
			pLwc->render_command[i].actionid = cmd->actionid;
			return;
		}
	}
	LOGE(LWLOGPOS "object key %d not exist", cmd->key);
	abort();
}

void handle_rmsg_despawn(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
	for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
		if (pLwc->render_command[i].key == cmd->key) {
			pLwc->render_command[i].key = 0;
			return;
		}
	}
	LOGE(LWLOGPOS "object key %d not exist", cmd->key);
	abort();
}

void handle_rmsg_pos(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
	for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
		if (pLwc->render_command[i].key == cmd->key) {
			memcpy(pLwc->render_command[i].pos, cmd->pos, sizeof(vec3));
			return;
		}
	}
	LOGE(LWLOGPOS "object key %d not exist", cmd->key);
	abort();
}

void handle_rmsg_turn(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
	for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
		if (pLwc->render_command[i].key == cmd->key) {
			pLwc->render_command[i].angle = cmd->angle;
			return;
		}
	}
	LOGE(LWLOGPOS "object key %d not exist", cmd->key);
	abort();
}

void handle_rmsg_rparams(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
	for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
		if (pLwc->render_command[i].key == cmd->key) {
			if (pLwc->render_command[i].objtype == 1) {
				// Skinned mesh
				pLwc->render_command[i].atlas = cmd->atlas;
				pLwc->render_command[i].skin_vbo = cmd->skin_vbo;
				pLwc->render_command[i].armature = cmd->armature;
			} else if (pLwc->render_command[i].objtype == 2) {
				// Static mesh
				pLwc->render_command[i].atlas = cmd->atlas;
				pLwc->render_command[i].vbo = cmd->skin_vbo;
				memcpy(pLwc->render_command[i].scale, cmd->scale, sizeof(vec3));
			}
			return;
		}
	}
	LOGE(LWLOGPOS "object key %d not exist", cmd->key);
	abort();
}

void handle_rmsg_bulletspawnheight(LWCONTEXT* pLwc, const LWFIELDRENDERCOMMAND* cmd) {
	for (int i = 0; i < MAX_RENDER_QUEUE_CAPACITY; i++) {
		if (pLwc->render_command[i].key == cmd->key) {
			pLwc->render_command[i].bullet_spawn_height= cmd->bullet_spawn_height;
			return;
		}
	}
	LOGE(LWLOGPOS "object key %d not exist", cmd->key);
	abort();
}

void delete_all_rmsgs(LWCONTEXT* pLwc) {
	zmq_msg_t rmsg;
	while (1) {
		zmq_msg_init(&rmsg);
		int rc = zmq_msg_recv(&rmsg, mq_rmsg_reader(pLwc->mq), ZMQ_DONTWAIT);
		zmq_msg_close(&rmsg);
		if (rc == -1) {
			break;
		}
	}
}
static void read_all_rmsgs(LWCONTEXT* pLwc) {
	zmq_msg_t rmsg;
	while (1) {
		zmq_msg_init(&rmsg);
		int rc = zmq_msg_recv(&rmsg, mq_rmsg_reader(pLwc->mq), ZMQ_DONTWAIT);
		if (rc == -1) {
			zmq_msg_close(&rmsg);
			break;
		}
		// Process command here
		LWFIELDRENDERCOMMAND* cmd = zmq_msg_data(&rmsg);
		switch (cmd->cmdtype) {
		case LRCT_SPAWN:
			handle_rmsg_spawn(pLwc, cmd);
			break;
		case LRCT_ANIM:
			handle_rmsg_anim(pLwc, cmd);
			break;
		case LRCT_DESPAWN:
			handle_rmsg_despawn(pLwc, cmd);
			break;
		case LRCT_POS:
			handle_rmsg_pos(pLwc, cmd);
			break;
		case LRCT_TURN:
			handle_rmsg_turn(pLwc, cmd);
			break;
		case LRCT_RPARAMS:
			handle_rmsg_rparams(pLwc, cmd);
			break;
		case LRCT_BULLETSPAWNHEIGHT:
			handle_rmsg_bulletspawnheight(pLwc, cmd);
			break;
		}
		zmq_msg_close(&rmsg);
	}
}

void lwc_render(const LWCONTEXT* pLwc) {
	// Busy wait for rendering okay sign
	while (!lwcontext_safe_to_start_render(pLwc)) {}
	// Set rendering flag to 1 (ignoring const-ness.......)
	lwcontext_set_rendering((LWCONTEXT*)pLwc, 1);
	// Tick rendering thread
	deltatime_tick(pLwc->render_dt);
	// Process all render messages (ignoring const-ness.......)
	read_all_rmsgs((LWCONTEXT*)pLwc);
	// Rendering function w.r.t. game scene dispatched here
	if (pLwc->game_scene == LGS_BATTLE) {
		lwc_render_battle(pLwc);
	} else if (pLwc->game_scene == LGS_DIALOG) {
		lwc_render_dialog(pLwc);
	} else if (pLwc->game_scene == LGS_FIELD) {
		if (pLwc->field) {
			lwc_render_field(pLwc);
		}
	} else if (pLwc->game_scene == LGS_FONT_TEST) {
		lwc_render_font_test(pLwc);
	} else if (pLwc->game_scene == LGS_ADMIN) {
		lwc_render_admin(pLwc);
	} else if (pLwc->game_scene == LGS_BATTLE_RESULT) {
		lwc_render_battle_result(pLwc);
	} else if (pLwc->game_scene == LGS_SKIN) {
		lwc_render_skin(pLwc);
	} else if (pLwc->game_scene == LGS_PHYSICS) {
		lwc_render_physics(pLwc);
	} else if (pLwc->game_scene == LGS_PARTICLE_SYSTEM) {
		lwc_render_ps(pLwc);
	}
	// Rendering a system message
	render_sys_msg(pLwc, pLwc->def_sys_msg);
	// Rendering stats
	render_stat(pLwc);
	// Set rendering flag to 0 (ignoring const-ness......)
	lwcontext_set_rendering((LWCONTEXT*)pLwc, 0);
}

static void bind_all_vertex_attrib_shader(const LWCONTEXT *pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	glBindVertexArray(pLwc->vao[vbo_index]);
#else
	set_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_skin_vertex_attrib_shader(const LWCONTEXT *pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	glBindVertexArray(pLwc->skin_vao[vbo_index]);
#else
	set_skin_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_fan_vertex_attrib_shader(const LWCONTEXT *pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	glBindVertexArray(pLwc->fan_vao[vbo_index]);
#else
	set_fan_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_ps_vertex_attrib_shader(const LWCONTEXT *pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
	glBindVertexArray(pLwc->ps_vao[vbo_index]);
#else
	set_ps_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

void bind_all_vertex_attrib(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_vertex_attrib_shader(pLwc, LWST_DEFAULT, vbo_index);
}

void bind_all_vertex_attrib_font(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_vertex_attrib_shader(pLwc, LWST_FONT, vbo_index);
}

void bind_all_vertex_attrib_etc1_with_alpha(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_vertex_attrib_shader(pLwc, LWST_ETC1, vbo_index);
}

void bind_all_skin_vertex_attrib(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_skin_vertex_attrib_shader(pLwc, LWST_SKIN, vbo_index);
}

void bind_all_fan_vertex_attrib(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_fan_vertex_attrib_shader(pLwc, LWST_FAN, vbo_index);
}

void bind_all_ps_vertex_attrib(const LWCONTEXT *pLwc, int vbo_index) {
	bind_all_ps_vertex_attrib_shader(pLwc, LWST_EMITTER2, vbo_index);
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

void init_armature(LWCONTEXT* pLwc) {
	for (int i = 0; i < LWAR_COUNT; i++) {
		load_armature(armature_filename[i], &pLwc->armature[i]);
	}
}

void init_action(LWCONTEXT* pLwc) {
	for (int i = 0; i < LWAC_COUNT; i++) {
		load_action(action_filename[i], &pLwc->action[i]);
	}
}

LWCONTEXT *lw_init(void) {

	init_ext_image_lib();

	init_ext_sound_lib();

	//test_image();

	LWCONTEXT *pLwc = (LWCONTEXT *)calloc(1, sizeof(LWCONTEXT));

	setlocale(LC_ALL, "");

	init_gl_context(pLwc);

	init_load_textures(pLwc);

	pLwc->pFnt = load_fnt(ASSETS_BASE_PATH "fnt" PATH_SEPARATOR "test6.fnt");

	pLwc->def_sys_msg = init_sys_msg();

	pLwc->update_dt = deltatime_new();

	deltatime_set_to_now(pLwc->update_dt);

	pLwc->render_dt = deltatime_new();

	deltatime_set_to_now(pLwc->render_dt);

	init_net(pLwc);

	pLwc->mq = init_mq(logic_server_addr(pLwc->server_index), pLwc->def_sys_msg);

	init_armature(pLwc);

	init_action(pLwc);

	return pLwc;
}

void lw_set_size(LWCONTEXT *pLwc, int w, int h) {
	pLwc->width = w;
	pLwc->height = h;
	// Update default projection matrix (pLwc->proj)
	logic_udate_default_projection(pLwc);
	// Initialize test font FBO
	init_font_fbo(pLwc);
	// Render font FBO using render-to-texture
	lwc_render_font_test_fbo(pLwc);
	// Reset dir pad input state
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

int lw_get_update_count(LWCONTEXT *pLwc) {
	return pLwc->update_count;
}

int lw_get_render_count(LWCONTEXT *pLwc) {
	return pLwc->render_count;
}

void lw_deinit(LWCONTEXT *pLwc) {

	for (int i = 0; i < LVT_COUNT; i++) {
		glDeleteBuffers(1, &pLwc->vertex_buffer[i].vertex_buffer);
	}

	for (int i = 0; i < LSVT_COUNT; i++) {
		glDeleteBuffers(1, &pLwc->skin_vertex_buffer[i].vertex_buffer);
	}

	for (int i = 0; i < LFVT_COUNT; i++) {
		glDeleteBuffers(1, &pLwc->fan_vertex_buffer[i].vertex_buffer);
	}

	glDeleteTextures(1, &pLwc->font_fbo.color_tex);
	glDeleteTextures(MAX_TEX_ATLAS, pLwc->tex_atlas);
	glDeleteTextures(MAX_TEX_FONT_ATLAS, pLwc->tex_font_atlas);
	glDeleteTextures(MAX_TEX_PROGRAMMED, pLwc->tex_programmed);

#if LW_SUPPORT_VAO
	glDeleteVertexArrays(VERTEX_BUFFER_COUNT, pLwc->vao);
	glDeleteVertexArrays(SKIN_VERTEX_BUFFER_COUNT, pLwc->skin_vao);
	glDeleteVertexArrays(FAN_VERTEX_BUFFER_COUNT, pLwc->fan_vao);
	glDeleteVertexArrays(PS_VERTEX_BUFFER_COUNT, pLwc->ps_vao);
#endif

	for (int i = 0; i < LWST_COUNT; i++) {
		delete_shader(&pLwc->shader[i]);
	}

	for (int i = 0; i < LWAC_COUNT; i++) {
		unload_action(&pLwc->action[i]);
	}

	for (int i = 0; i < LWAR_COUNT; i++) {
		unload_armature(&pLwc->armature[i]);
	}

	deinit_mq(pLwc->mq);

	unload_field(pLwc->field);

	deinit_sys_msg(pLwc->def_sys_msg);

	deltatime_destroy(&pLwc->update_dt);

	free(pLwc);
}

void lw_on_destroy(LWCONTEXT *pLwc) {
	release_font(pLwc->pFnt);
	release_string(pLwc->dialog);
	deinit_net(pLwc);
	zactor_destroy((zactor_t**)&pLwc->logic_actor);
	lw_deinit(pLwc);
	mq_shutdown();
}

void lw_set_kp(LWCONTEXT *pLwc, int kp) {
	pLwc->kp = kp;
}

double lwcontext_delta_time(const LWCONTEXT* pLwc) {
	return deltatime_delta_time(pLwc->update_dt);
}
