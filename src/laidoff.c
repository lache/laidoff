#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
#endif
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
#include "lwparabola.h"
#include "render_ui.h"
#include "render_splash.h"
#include "lwbutton.h"
#include "puckgame.h"
#include "lwudp.h"
#include "lwtcp.h"
#include "lwtcpclient.h"
#include "lwdamagetext.h"
#include "lwmath.h"
#include "puckgameupdate.h"
#include "render_leaderboard.h"
#include "numcomp_puck_game.h"
// SWIG output file
#include "lo_wrap.inl"

#define GLSL_DIR_NAME "glsl"
#define LW_SUPPORT_ETC1_HARDWARE_DECODING LW_PLATFORM_ANDROID
#define LW_SUPPORT_VAO (LW_PLATFORM_WIN32 || LW_PLATFORM_OSX || LW_PLATFORM_LINUX)

const float default_uv_offset[2] = { 0, 0 };
const float default_uv_scale[2] = { 1, 1 };
const float default_flip_y_uv_scale[2] = { 1, -1 };

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

void play_sound(LW_SOUND lws);
void stop_sound(LW_SOUND lws);
HRESULT init_ext_image_lib();
HRESULT init_ext_sound_lib();
void destroy_ext_sound_lib();
void create_image(const char *filename, LWBITMAPCONTEXT *pBitmapContext, int tex_atlas_index);
void release_image(LWBITMAPCONTEXT *pBitmapContext);
void lwc_render_battle(const LWCONTEXT* pLwc);
void lwc_render_dialog(const LWCONTEXT* pLwc);
void lwc_render_field(const LWCONTEXT* pLwc);
void init_load_textures(LWCONTEXT* pLwc);
void load_test_font(LWCONTEXT* pLwc);
int LoadObjAndConvert(float bmin[3], float bmax[3], const char *filename);
int spawn_attack_trail(LWCONTEXT* pLwc, float x, float y, float z);
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

void set_texture_parameter_values(const LWCONTEXT* pLwc, float x, float y, float w, float h,
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
set_texture_parameter(const LWCONTEXT* pLwc, LWENUM _LW_ATLAS_ENUM lae, LWENUM _LW_ATLAS_SPRITE las) {
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
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
#define LW_GLSL_VERSION_STATEMENT "#version 150\n"
#else
#define LW_GLSL_VERSION_STATEMENT "#version 100\n"
#endif
    pShader->valid = 0;

    pShader->vertex_shader = glCreateShader(GL_VERTEX_SHADER);

    const GLchar* vstarray[] = { LW_GLSL_VERSION_STATEMENT, vst };
    glShaderSource(pShader->vertex_shader, 2, vstarray, NULL);
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
    const GLchar* fstarray[] = { LW_GLSL_VERSION_STATEMENT, fst };
    glShaderSource(pShader->fragment_shader, 2, fstarray, NULL);
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
    pShader->m_location = glGetUniformLocation(pShader->program, "M");
    pShader->alpha_multiplier_location = glGetUniformLocation(pShader->program, "alpha_multiplier");
    pShader->overlay_color_location = glGetUniformLocation(pShader->program, "overlay_color");
    pShader->overlay_color_ratio_location = glGetUniformLocation(pShader->program, "overlay_color_ratio");
    pShader->multiply_color_location = glGetUniformLocation(pShader->program, "multiply_color");
    pShader->diffuse_location = glGetUniformLocation(pShader->program, "diffuse");
    pShader->diffuse_arrow_location = glGetUniformLocation(pShader->program, "diffuse_arrow");
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
    pShader->u_ProjectionViewMatrix = glGetUniformLocation(pShader->program, "u_ProjectionViewMatrix");
    pShader->u_ModelMatrix = glGetUniformLocation(pShader->program, "u_ModelMatrix");
    pShader->u_Gravity = glGetUniformLocation(pShader->program, "u_Gravity");
    pShader->u_Time = glGetUniformLocation(pShader->program, "u_Time");
    pShader->u_eRadius = glGetUniformLocation(pShader->program, "u_eRadius");
    pShader->u_eVelocity = glGetUniformLocation(pShader->program, "u_eVelocity");
    pShader->u_eDecay = glGetUniformLocation(pShader->program, "u_eDecay");
    pShader->u_eSizeStart = glGetUniformLocation(pShader->program, "u_eSizeStart");
    pShader->u_eSizeEnd = glGetUniformLocation(pShader->program, "u_eSizeEnd");
    pShader->u_eScreenWidth = glGetUniformLocation(pShader->program, "u_eScreenWidth");
    pShader->u_eColorStart = glGetUniformLocation(pShader->program, "u_eColorStart");
    pShader->u_eColorEnd = glGetUniformLocation(pShader->program, "u_eColorEnd");
    pShader->u_Texture = glGetUniformLocation(pShader->program, "u_Texture");
    pShader->u_TextureAlpha = glGetUniformLocation(pShader->program, "u_TextureAlpha");
    pShader->time = glGetUniformLocation(pShader->program, "time");
    pShader->resolution = glGetUniformLocation(pShader->program, "resolution");
    pShader->sphere_pos = glGetUniformLocation(pShader->program, "sphere_pos");
    pShader->sphere_col = glGetUniformLocation(pShader->program, "sphere_col");
    pShader->sphere_col_ratio = glGetUniformLocation(pShader->program, "sphere_col_ratio");
    pShader->sphere_speed = glGetUniformLocation(pShader->program, "sphere_speed");
    pShader->sphere_move_rad = glGetUniformLocation(pShader->program, "sphere_move_rad");
    pShader->reflect_size = glGetUniformLocation(pShader->program, "reflect_size");
    pShader->arrow_center = glGetUniformLocation(pShader->program, "arrow_center");
    pShader->arrow_angle = glGetUniformLocation(pShader->program, "arrow_angle");
    pShader->arrow_scale = glGetUniformLocation(pShader->program, "arrow_scale");
    pShader->arrowRotMat2 = glGetUniformLocation(pShader->program, "arrowRotMat2");
    pShader->gauge_ratio = glGetUniformLocation(pShader->program, "gauge_ratio");
    pShader->full_color = glGetUniformLocation(pShader->program, "full_color");
    pShader->empty_color = glGetUniformLocation(pShader->program, "empty_color");
    pShader->wrap_offset = glGetUniformLocation(pShader->program, "wrap_offset");

    // Set initial value...
    glUseProgram(pShader->program);
    glUniform3f(pShader->multiply_color_location, 1.0f, 1.0f, 1.0f);

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

void init_gl_shaders(LWCONTEXT* pLwc) {
    // Vertex Shader
    char* default_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                      GLSL_DIR_NAME PATH_SEPARATOR
                                                      "default-vert.glsl");
    char* skin_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                   GLSL_DIR_NAME PATH_SEPARATOR
                                                   "skin-vert.glsl");
    char* fan_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                  GLSL_DIR_NAME PATH_SEPARATOR
                                                  "fan-vert.glsl");
    char* emitter_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                      GLSL_DIR_NAME PATH_SEPARATOR
                                                      "emitter-vert.glsl");
    char* emitter2_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                       GLSL_DIR_NAME PATH_SEPARATOR
                                                       "emitter2-vert.glsl");
    char* sphere_reflect_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                             GLSL_DIR_NAME PATH_SEPARATOR
                                                             "sphere-reflect-vert.glsl");
    char* default_normal_vert_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                             GLSL_DIR_NAME PATH_SEPARATOR
                                                             "default-normal-vert.glsl");
    // Fragment Shader
    char* default_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                      GLSL_DIR_NAME PATH_SEPARATOR
                                                      "default-frag.glsl");
    char* color_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                    GLSL_DIR_NAME PATH_SEPARATOR
                                                    "color-frag.glsl");
    char* panel_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                    GLSL_DIR_NAME PATH_SEPARATOR
                                                    "panel-frag.glsl");
    char* font_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                   GLSL_DIR_NAME PATH_SEPARATOR
                                                   "font-frag.glsl");
    char* etc1_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                   GLSL_DIR_NAME PATH_SEPARATOR
                                                   "etc1-frag.glsl");
    char* fan_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                  GLSL_DIR_NAME PATH_SEPARATOR
                                                  "fan-frag.glsl");
    char* emitter_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                      GLSL_DIR_NAME PATH_SEPARATOR
                                                      "emitter-frag.glsl");
    char* emitter2_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                       GLSL_DIR_NAME PATH_SEPARATOR
                                                       "emitter2-frag.glsl");
    char* sphere_reflect_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                             GLSL_DIR_NAME PATH_SEPARATOR
                                                             "sphere-reflect-frag.glsl");
    char* sphere_reflect_floor_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                                   GLSL_DIR_NAME PATH_SEPARATOR
                                                                   "sphere-reflect-floor-frag.glsl");
    char* ringgauge_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                        GLSL_DIR_NAME PATH_SEPARATOR
                                                        "ringgauge-frag.glsl");
    char* radialwave_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                         GLSL_DIR_NAME PATH_SEPARATOR
                                                         "radialwave-frag.glsl");
    char* default_normal_frag_glsl = create_string_from_file(ASSETS_BASE_PATH
                                                             GLSL_DIR_NAME PATH_SEPARATOR
                                                             "default-normal-frag.glsl");

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

    if (!sphere_reflect_vert_glsl) {
        LOGE("init_gl_shaders: sphere-reflect-vert.glsl not loaded. Abort...");
        return;
    }

    if (!default_normal_vert_glsl) {
        LOGE("init_gl_shaders: default-normal-vert.glsl not loaded. Abort...");
        return;
    }

    if (!default_frag_glsl) {
        LOGE("init_gl_shaders: default-frag.glsl not loaded. Abort...");
        return;
    }

    if (!color_frag_glsl) {
        LOGE("init_gl_shaders: color-frag.glsl not loaded. Abort...");
        return;
    }

    if (!panel_frag_glsl) {
        LOGE("init_gl_shaders: panel-frag.glsl not loaded. Abort...");
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

    if (!sphere_reflect_frag_glsl) {
        LOGE("init_gl_shaders: sphere-reflect-frag.glsl not loaded. Abort...");
        return;
    }

    if (!sphere_reflect_floor_frag_glsl) {
        LOGE("init_gl_shaders: sphere-reflect-floor-frag.glsl not loaded. Abort...");
        return;
    }

    if (!ringgauge_frag_glsl) {
        LOGE("init_gl_shaders: ringgauge-frag.glsl not loaded. Abort...");
        return;
    }

    if (!radialwave_frag_glsl) {
        LOGE("init_gl_shaders: radialwave-frag.glsl not loaded. Abort...");
        return;
    }

    if (!default_normal_frag_glsl) {
        LOGE("init_gl_shaders: default-normal-frag.glsl not loaded. Abort...");
        return;
    }

    create_shader("Default Shader", &pLwc->shader[LWST_DEFAULT], default_vert_glsl, default_frag_glsl);
    create_shader("Font Shader", &pLwc->shader[LWST_FONT], default_vert_glsl, font_frag_glsl);
    create_shader("ETC1 with Alpha Shader", &pLwc->shader[LWST_ETC1], default_vert_glsl, etc1_frag_glsl);
    create_shader("Skin Shader", &pLwc->shader[LWST_SKIN], skin_vert_glsl, default_frag_glsl);
    create_shader("Fan Shader", &pLwc->shader[LWST_FAN], fan_vert_glsl, fan_frag_glsl);
    create_shader("Emitter Shader", &pLwc->shader[LWST_EMITTER], emitter_vert_glsl, emitter_frag_glsl);
    create_shader("Emitter2 Shader", &pLwc->shader[LWST_EMITTER2], emitter2_vert_glsl, emitter2_frag_glsl);
    create_shader("Color Shader", &pLwc->shader[LWST_COLOR], default_vert_glsl, color_frag_glsl);
    create_shader("Panel Shader", &pLwc->shader[LWST_PANEL], default_vert_glsl, panel_frag_glsl);
    create_shader("Sphere Reflect Shader", &pLwc->shader[LWST_SPHERE_REFLECT], sphere_reflect_vert_glsl, sphere_reflect_frag_glsl);
    create_shader("Sphere Reflect Floor Shader", &pLwc->shader[LWST_SPHERE_REFLECT_FLOOR], sphere_reflect_vert_glsl, sphere_reflect_floor_frag_glsl);
    create_shader("Ringgauge Shader", &pLwc->shader[LWST_RINGGAUGE], default_vert_glsl, ringgauge_frag_glsl);
    create_shader("Radial wave Shader", &pLwc->shader[LWST_RADIALWAVE], default_vert_glsl, radialwave_frag_glsl);
    create_shader("Default Normal Shader", &pLwc->shader[LWST_DEFAULT_NORMAL], default_normal_vert_glsl, default_normal_frag_glsl);

    release_string(default_vert_glsl);
    release_string(skin_vert_glsl);
    release_string(fan_vert_glsl);
    release_string(emitter_vert_glsl);
    release_string(emitter2_vert_glsl);
    release_string(sphere_reflect_vert_glsl);
    release_string(default_normal_vert_glsl);
    release_string(default_frag_glsl);
    release_string(color_frag_glsl);
    release_string(panel_frag_glsl);
    release_string(font_frag_glsl);
    release_string(etc1_frag_glsl);
    release_string(fan_frag_glsl);
    release_string(emitter_frag_glsl);
    release_string(emitter2_frag_glsl);
    release_string(sphere_reflect_frag_glsl);
    release_string(sphere_reflect_floor_frag_glsl);
    release_string(ringgauge_frag_glsl);
    release_string(radialwave_frag_glsl);
    release_string(default_normal_frag_glsl);
}

static void load_vbo(LWCONTEXT* pLwc, const char* filename, LWVBO* pVbo) {
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

static void load_skin_vbo(LWCONTEXT* pLwc, const char *filename, LWVBO *pSvbo) {
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

static void init_fvbo(LWCONTEXT* pLwc) {
    load_fvbo(pLwc, ASSETS_BASE_PATH "fvbo" PATH_SEPARATOR "whole-tower_cell.fvbo",
              &pLwc->fvertex_buffer[LFT_TOWER]);
}

static void init_fanim(LWCONTEXT* pLwc) {
    load_fanim(pLwc, ASSETS_BASE_PATH "fanim" PATH_SEPARATOR "whole-tower_cell.fanim",
               &pLwc->fanim[LFAT_TOWER_COLLAPSE]);
}

static void init_vbo(LWCONTEXT* pLwc) {

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

    // LVT_CATAPULT_BALL
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "catapult-ball.vbo",
             &pLwc->vertex_buffer[LVT_CATAPULT_BALL]);

    // LVT_DEVIL
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "devil.vbo",
             &pLwc->vertex_buffer[LVT_DEVIL]);

    // LVT_CRYSTAL
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "crystal.vbo",
             &pLwc->vertex_buffer[LVT_CRYSTAL]);

    // LVT_SPIRAL
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "spiral.vbo",
             &pLwc->vertex_buffer[LVT_SPIRAL]);
    // LVT_PUCK
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "puck.vbo",
             &pLwc->vertex_buffer[LVT_PUCK]);
    // LVT_TOWER_BASE
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-base.vbo",
             &pLwc->vertex_buffer[LVT_TOWER_BASE]);
    // LVT_TOWER_1
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-1.vbo",
             &pLwc->vertex_buffer[LVT_TOWER_1]);
    // LVT_TOWER_2
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-2.vbo",
             &pLwc->vertex_buffer[LVT_TOWER_2]);
    // LVT_TOWER_3
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-3.vbo",
             &pLwc->vertex_buffer[LVT_TOWER_3]);
    // LVT_TOWER_4
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-4.vbo",
             &pLwc->vertex_buffer[LVT_TOWER_4]);
    // LVT_TOWER_5
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "tower-5.vbo",
             &pLwc->vertex_buffer[LVT_TOWER_5]);
    // LVT_RINGGAUGE
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "ringgauge.vbo",
             &pLwc->vertex_buffer[LVT_RINGGAUGE]);
    // LVT_RINGGAUGETHICK
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "ringgaugethick.vbo",
             &pLwc->vertex_buffer[LVT_RINGGAUGETHICK]);
    // LVT_RADIALWAVE
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "radialwave.vbo",
             &pLwc->vertex_buffer[LVT_RADIALWAVE]);
    // LVT_PHYSICS_MENU
    load_vbo(pLwc, ASSETS_BASE_PATH "vbo" PATH_SEPARATOR "physics-menu.vbo",
             &pLwc->vertex_buffer[LVT_PHYSICS_MENU]);

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

    // LSVT_CATAPULT
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "catapult.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_CATAPULT]);

    // LSVT_PYRO
    load_skin_vbo(pLwc, ASSETS_BASE_PATH "svbo" PATH_SEPARATOR "pyro.svbo",
                  &pLwc->skin_vertex_buffer[LSVT_PYRO]);

    // === STATIC MESHES (FAN TYPE) ===
    load_fan_vbo(pLwc);

    lwc_create_ui_vbo(pLwc);
}

void set_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    // vertex coordinates
    glEnableVertexAttribArray(pLwc->shader[shader_index].vpos_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
                          stride_in_bytes, (void *)0);
    // vertex color / normal
    glEnableVertexAttribArray(pLwc->shader[shader_index].vcol_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vcol_location, 3, GL_FLOAT, GL_FALSE,
                          stride_in_bytes, (void *)(sizeof(float) * 3));
    // uv coordinates
    glEnableVertexAttribArray(pLwc->shader[shader_index].vuv_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vuv_location, 2, GL_FLOAT, GL_FALSE,
                          stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
    // scale-9 coordinates
    glEnableVertexAttribArray(pLwc->shader[shader_index].vs9_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vs9_location, 2, GL_FLOAT, GL_FALSE,
                          stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2)));
}

void set_skin_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    // vertex coordinates
    glEnableVertexAttribArray(pLwc->shader[shader_index].vpos_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vpos_location, 3, GL_FLOAT, GL_FALSE,
                          skin_stride_in_bytes, (void *)0);
    // vertex color / normal
    glEnableVertexAttribArray(pLwc->shader[shader_index].vcol_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vcol_location, 3, GL_FLOAT, GL_FALSE,
                          skin_stride_in_bytes, (void *)(sizeof(float) * 3));
    // uv coordinates
    glEnableVertexAttribArray(pLwc->shader[shader_index].vuv_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vuv_location, 2, GL_FLOAT, GL_FALSE,
                          skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3)));
    // bone weights
    glEnableVertexAttribArray(pLwc->shader[shader_index].vbweight_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vbweight_location, 4, GL_FLOAT, GL_FALSE,
                          skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2)));
    // bone transformations
    glEnableVertexAttribArray(pLwc->shader[shader_index].vbmat_location);
    glVertexAttribPointer(pLwc->shader[shader_index].vbmat_location, 4, GL_FLOAT, GL_FALSE,
                          skin_stride_in_bytes, (void *)(sizeof(float) * (3 + 3 + 2 + 4)));
}

void set_fan_vertex_attrib_pointer(const LWCONTEXT* pLwc, int shader_index) {
    // vertex coordinates
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

static void init_vao(LWCONTEXT* pLwc, int shader_index) {
    // Vertex Array Objects
#if LW_SUPPORT_VAO
    glGenVertexArrays(VERTEX_BUFFER_COUNT, pLwc->vao);
    for (int i = 0; i < VERTEX_BUFFER_COUNT; i++) {
        glBindVertexArray(pLwc->vao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[i].vertex_buffer);
        if (i == LVT_UI_SCRAP_BG
            || i == LVT_UI_TOWER_BUTTON_BG
            || i == LVT_UI_LEFT_BUTTON_BG
            || i == LVT_UI_BUTTON_BG) {
            set_vertex_attrib_pointer(pLwc, LWST_COLOR);
        } else if (i == LVT_UI_FULL_PANEL_BG) {
            //set_vertex_attrib_pointer(pLwc, LWST_PANEL);
            set_vertex_attrib_pointer(pLwc, LWST_COLOR);
        } else if (i == LVT_TOWER_BASE
                   || i == LVT_TOWER_1
                   || i == LVT_TOWER_2
                   || i == LVT_TOWER_3
                   || i == LVT_TOWER_4
                   || i == LVT_TOWER_5) {
            set_vertex_attrib_pointer(pLwc, LWST_DEFAULT_NORMAL);
        } else {
            set_vertex_attrib_pointer(pLwc, shader_index);
        }
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_fvao(LWCONTEXT* pLwc, int shader_index) {
    // Vertex Array Objects for FVBO
#if LW_SUPPORT_VAO
    glGenVertexArrays(LFT_COUNT, pLwc->fvao);
    for (int i = 0; i < LFT_COUNT; i++) {
        glBindVertexArray(pLwc->fvao[i]);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->fvertex_buffer[i].vertex_buffer);
        set_vertex_attrib_pointer(pLwc, shader_index);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
#endif
}

static void init_skin_vao(LWCONTEXT* pLwc, int shader_index) {
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

static void init_fan_vao(LWCONTEXT* pLwc, int shader_index) {
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

static void init_ps_vao(LWCONTEXT* pLwc, int shader_index) {
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

static void init_gl_context(LWCONTEXT* pLwc) {
    init_gl_shaders(pLwc);

    init_vbo(pLwc);
    init_fvbo(pLwc);
    init_fanim(pLwc);
    // Particle system's VAOs are configured here. Should be called before setting VAOs.
    init_ps(pLwc);

    init_vao(pLwc, 0/* ??? */);
    init_fvao(pLwc, LWST_DEFAULT_NORMAL/* ??? */);

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

int get_tex_index_by_hash_key(const LWCONTEXT* pLwc, const char *hash_key) {
    unsigned long h = hash((const unsigned char *)hash_key);
    for (int i = 0; i < MAX_TEX_ATLAS; i++) {
        if (pLwc->tex_atlas_hash[i] == h) {
            return i;
        }
    }
    return 0;
}

void render_stat(const LWCONTEXT* pLwc) {
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_RIGHT_BOTTOM;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH / 2;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    char msg[128];
    sprintf(msg, "L:%.1f R:%.1f RMSG:%d(%d-%d)",
        (float)(1.0 / deltatime_history_avg(pLwc->update_dt)),
            (float)(1.0 / deltatime_history_avg(pLwc->render_dt)),
            pLwc->rmsg_send_count - pLwc->rmsg_recv_count,
            pLwc->rmsg_send_count,
            pLwc->rmsg_recv_count
    );
    text_block.text = msg;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = pLwc->aspect_ratio;
    text_block.text_block_y = -1.0f;
    text_block.multiline = 1;
    render_text_block(pLwc, &text_block);
}

void render_addr(const LWCONTEXT* pLwc) {
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_LEFT_BOTTOM;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    char msg[128];
    sprintf(msg, "T:%s:%d / U:%s:%d",
            lw_tcp_addr(pLwc), lw_tcp_port(pLwc),
            lw_udp_addr(pLwc), lw_udp_port(pLwc)
    );
    text_block.text = msg;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = -pLwc->aspect_ratio;
    text_block.text_block_y = -1.0f;
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
            pLwc->render_command[i].anim_marker_search_begin = 0;
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
            pLwc->render_command[i].bullet_spawn_height = cmd->bullet_spawn_height;
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
    if (pLwc->mq == 0) {
        return;
    }
    zmq_msg_t rmsg;
    while (1) {
        zmq_msg_init(&rmsg);
        int rc = zmq_msg_recv(&rmsg, mq_rmsg_reader(pLwc->mq), ZMQ_DONTWAIT);
        if (rc == -1) {
            zmq_msg_close(&rmsg);
            break;
        }
        lwcontext_inc_rmsg_recv(pLwc);
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

void vec3_lerp(vec3 vm, vec3 va, vec3 vb, float t) {
    for (int i = 0; i < 3; i++) {
        vm[i] = va[i] * (1.0f - t) + vb[i] * t;
    }
}

void slerp(quat qm, const quat qa, const quat qb, float t) {
    // Calculate angle between them.
    float cosHalfTheta = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
    // if qa=qb or qa=-qb then theta = 0 and we can return qa
    if (fabsf(cosHalfTheta) >= 1.0) {
        qm[0] = qa[0]; qm[1] = qa[1]; qm[2] = qa[2]; qm[3] = qa[3];
        return;
    }
    // Calculate temporary values.
    float halfTheta = acosf(cosHalfTheta);
    float sinHalfTheta = sqrtf(1.0f - cosHalfTheta*cosHalfTheta);
    // if theta = 180 degrees then result is not fully defined
    // we could rotate around any axis normal to qa or qb
    if (fabsf(sinHalfTheta) < 0.001f) { // fabs is floating point absolute
        qm[0] = (qa[0] * 0.5f + qb[0] * 0.5f);
        qm[1] = (qa[1] * 0.5f + qb[1] * 0.5f);
        qm[2] = (qa[2] * 0.5f + qb[2] * 0.5f);
        qm[3] = (qa[3] * 0.5f + qb[3] * 0.5f);
        return;
    }
    float ratioA = sinf((1 - t) * halfTheta) / sinHalfTheta;
    float ratioB = sinf(t * halfTheta) / sinHalfTheta;
    //calculate Quaternion.
    qm[0] = (qa[0] * ratioA + qb[0] * ratioB);
    qm[1] = (qa[1] * ratioA + qb[1] * ratioB);
    qm[2] = (qa[2] * ratioA + qb[2] * ratioB);
    qm[3] = (qa[3] * ratioA + qb[3] * ratioB);
}

void linear_interpolate_state(LWPSTATE* p, LWPSTATE* state_buffer, int state_buffer_len, double sample_update_tick) {
    int sample1_idx = 0;
    double sample1_diff = DBL_MAX;
    int sample2_idx = 0;
    double sample2_diff = DBL_MAX;

    for (int i = 0; i < state_buffer_len; i++) {
        double d = state_buffer[i].update_tick - sample_update_tick;
        if (d >= 0) {
            if (sample2_diff > d) {
                sample2_diff = d;
                sample2_idx = i;
            }
        } else {
            if (sample1_diff > fabs(d)) {
                sample1_diff = fabs(d);
                sample1_idx = i;
            }
        }
    }
    if (sample1_idx != sample2_idx
        && state_buffer[sample1_idx].update_tick <= sample_update_tick && sample_update_tick <= state_buffer[sample2_idx].update_tick) {

        int update_tick_diff = state_buffer[sample2_idx].update_tick - state_buffer[sample1_idx].update_tick;
        double dist = sample_update_tick - state_buffer[sample1_idx].update_tick;

        double ratio = dist / update_tick_diff;

        quat player_quat1, puck_quat1, target_quat1;
        quat_from_mat4x4_skin(player_quat1, state_buffer[sample1_idx].player_rot);
        quat_from_mat4x4_skin(puck_quat1, state_buffer[sample1_idx].puck_rot);
        quat_from_mat4x4_skin(target_quat1, state_buffer[sample1_idx].target_rot);
        quat player_quat2, puck_quat2, target_quat2;
        quat_from_mat4x4_skin(player_quat2, state_buffer[sample2_idx].player_rot);
        quat_from_mat4x4_skin(puck_quat2, state_buffer[sample2_idx].puck_rot);
        quat_from_mat4x4_skin(target_quat2, state_buffer[sample2_idx].target_rot);
        quat player_quatm, puck_quatm, target_quatm;
        slerp(player_quatm, player_quat1, player_quat2, (float)ratio);
        slerp(puck_quatm, puck_quat1, puck_quat2, (float)ratio);
        slerp(target_quatm, target_quat1, target_quat2, (float)ratio);
        mat4x4_from_quat_skin(p->player_rot, player_quatm);
        mat4x4_from_quat_skin(p->puck_rot, puck_quatm);
        mat4x4_from_quat_skin(p->target_rot, target_quatm);
        vec3_lerp(p->player, state_buffer[sample1_idx].player, state_buffer[sample2_idx].player, (float)ratio);
        vec3_lerp(p->puck, state_buffer[sample1_idx].puck, state_buffer[sample2_idx].puck, (float)ratio);
        vec3_lerp(p->target, state_buffer[sample1_idx].target, state_buffer[sample2_idx].target, (float)ratio);
        //LOGI("Interpolate state ratio: %.3f", ratio);
    } else {
        //LOGE("Error in logic");
    }
}

static void dequeue_puck_game_state_and_apply(LWCONTEXT* pLwc) {
    LWPSTATE new_state;
    if (ringbuffer_dequeue(&pLwc->udp->state_ring_buffer, &new_state) == 0) {
        //double client_elapsed = lwtimepoint_now_seconds() - pLwc->udp->puck_state_sync_client_timepoint;
        //double sample_update_tick = (pLwc->udp->puck_state_sync_server_timepoint + client_elapsed) * state_sync_hz;
        //LWPSTATE sampled_state;
        //linear_interpolate_state(&sampled_state, pLwc->udp->state_buffer, LW_STATE_RING_BUFFER_CAPACITY, sample_update_tick);
        //memcpy(&pLwc->puck_game_state, &sampled_state, sizeof(LWPSTATE));

        // 'player' is currently playing player
        // 'target' is currently opponent player
        const int player_damage = pLwc->puck_game_state.bf.player_current_hp - new_state.bf.player_current_hp;
        if (player_damage > 0) {
            puck_game_shake_player(pLwc->puck_game, &pLwc->puck_game->player);
            LWPUCKGAMETOWER* tower = &pLwc->puck_game->tower[pLwc->puck_game->player_no == 2 ? 1 : 0/*player*/];
            puck_game_spawn_tower_damage_text(pLwc,
                                              pLwc->puck_game,
                                              tower,
                                              player_damage);
            // on_death...
            if (new_state.bf.player_current_hp == 0) {
                tower->collapsing = 1;
                tower->collapsing_time = 0;
            }
        }
        const int target_damage = pLwc->puck_game_state.bf.target_current_hp - new_state.bf.target_current_hp;
        if (target_damage > 0) {
            puck_game_shake_player(pLwc->puck_game, &pLwc->puck_game->target);
            LWPUCKGAMETOWER* tower = &pLwc->puck_game->tower[pLwc->puck_game->player_no == 2 ? 0 : 1/*target*/];
            puck_game_spawn_tower_damage_text(pLwc,
                                              pLwc->puck_game,
                                              tower,
                                              target_damage);
            // on_death...
            if (new_state.bf.target_current_hp == 0) {
                tower->collapsing = 1;
                tower->collapsing_time = 0;
            }
        }
        // If the battle is finished (called once)
        if (puck_game_state_phase_finished(pLwc->puck_game_state.bf.phase) == 0
            && puck_game_state_phase_finished(new_state.bf.phase) == 1) {
            LOGI(LWLOGPOS "Battle finished. Destroying UDP context...");
            destroy_udp(&pLwc->udp);
            //puck_game_roll_to_main_menu(pLwc->puck_game);
            pLwc->puck_game->battle_control_ui_alpha = 0.0f;
        }
        // Overwrite old game state with a new one
        memcpy(&pLwc->puck_game_state, &new_state, sizeof(LWPSTATE));
        pLwc->puck_game->battle_phase = new_state.bf.phase;
        pLwc->puck_game->update_tick = new_state.update_tick;
    } else {
        LOGE("State buffer dequeue failed.");
    }
}

static void convert_state2_to_state(LWPSTATE* state, const LWPSTATE2* state2, const LWNUMCOMPPUCKGAME* numcomp) {
    state->type = LPGP_LWPSTATE;
    state->update_tick = state2->update_tick;
    numcomp_decompress_vec3(state->puck, state2->go[0].pos, &numcomp->v[LNVT_POS]);
    numcomp_decompress_vec3(state->player, state2->go[1].pos, &numcomp->v[LNVT_POS]);
    LOGIx("player z = %f", state->player[2]);
    numcomp_decompress_vec3(state->target, state2->go[2].pos, &numcomp->v[LNVT_POS]);
    numcomp_decompress_mat4x4(state->puck_rot, state2->go[0].rot, &numcomp->q[LNQT_ROT]);
    numcomp_decompress_mat4x4(state->player_rot, state2->go[1].rot, &numcomp->q[LNQT_ROT]);
    numcomp_decompress_mat4x4(state->target_rot, state2->go[2].rot, &numcomp->q[LNQT_ROT]);
    state->puck_speed = numcomp_decompress_float(state2->go[0].speed, &numcomp->f[LNFT_PUCK_SPEED]);
    state->player_speed = numcomp_decompress_float(state2->go[1].speed, &numcomp->f[LNFT_PUCK_SPEED]);
    state->target_speed = numcomp_decompress_float(state2->go[2].speed, &numcomp->f[LNFT_PUCK_SPEED]);
    state->puck_move_rad = numcomp_decompress_float(state2->go[0].move_rad, &numcomp->f[LNFT_PUCK_MOVE_RAD]);
    state->player_move_rad = numcomp_decompress_float(state2->go[1].move_rad, &numcomp->f[LNFT_PUCK_MOVE_RAD]);
    state->target_move_rad = numcomp_decompress_float(state2->go[2].move_rad, &numcomp->f[LNFT_PUCK_MOVE_RAD]);
    state->puck_reflect_size = numcomp_decompress_float(state2->puck_reflect_size, &numcomp->f[LNFT_PUCK_REFLECT_SIZE]);
    state->bf = state2->bf;
}

static void dequeue_puck_game_state2_and_push_to_state_queue(LWCONTEXT* pLwc) {
    LWPSTATE2 new_state2;
    if (ringbuffer_dequeue(&pLwc->udp->state2_ring_buffer, &new_state2) == 0) {
        LWPSTATE new_state;
        convert_state2_to_state(&new_state, &new_state2, &pLwc->udp->numcomp);
        ringbuffer_queue(&pLwc->udp->state_ring_buffer, &new_state);
    } else {
        LOGE("State2 buffer dequeue failed.");
    }
}

static void dequeue_from_state_queue(LWCONTEXT* pLwc) {
    int size = ringbuffer_size(&pLwc->udp->state_ring_buffer);
    if (size >= 1) {
        while (ringbuffer_size(&pLwc->udp->state_ring_buffer) >= 6) {
            LWPSTATE pout_unused;
            ringbuffer_dequeue(&pLwc->udp->state_ring_buffer, &pout_unused);
        }
        dequeue_puck_game_state_and_apply(pLwc);
    } else {
        LOGIx("Puck game state buffer underrun");
    }
}

static void dequeue_from_state2_queue(LWCONTEXT* pLwc) {
    int size = ringbuffer_size(&pLwc->udp->state2_ring_buffer);
    if (size >= 1) {
        while (ringbuffer_size(&pLwc->udp->state2_ring_buffer) >= 6) {
            LWPSTATE2 pout_unused;
            ringbuffer_dequeue(&pLwc->udp->state2_ring_buffer, &pout_unused);
        }
        dequeue_puck_game_state2_and_push_to_state_queue(pLwc);
        dequeue_puck_game_state_and_apply(pLwc);
    } else {
        LOGIx("Puck game state2 buffer underrun");
    }
}

void lwc_prerender_mutable_context(LWCONTEXT* pLwc) {
    if (pLwc->udp == 0 || pLwc->udp->ready == 0) {
        return;
    }
    // using LWPSTATE
    //dequeue_from_state_queue(pLwc);
    // using LWPSTATE2
    dequeue_from_state2_queue(pLwc);
}

void lwc_render(const LWCONTEXT* pLwc) {
    // Busy wait for rendering okay sign
    while (!lwcontext_safe_to_start_render(pLwc)) {}
    // Set rendering flag to 1 (ignoring const-ness.......)
    lwcontext_set_rendering((LWCONTEXT*)pLwc, 1);
    // Button count to zero (ignoring const-ness......)
    ((LWCONTEXT*)pLwc)->button_list.button_count = 0;
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
        lwc_render_physics(pLwc, pLwc->puck_game_view, pLwc->puck_game_proj);
    } else if (pLwc->game_scene == LGS_PARTICLE_SYSTEM) {
        lwc_render_ps(pLwc);
    } else if (pLwc->game_scene == LGS_UI) {
        lwc_render_ui(pLwc);
    } else if (pLwc->game_scene == LGS_SPLASH) {
        lwc_render_splash(pLwc);
    } else if (pLwc->game_scene == LGS_LEADERBOARD) {
        lwc_render_leaderboard(pLwc);
    }
    // Rendering a system message
    render_sys_msg(pLwc, pLwc->def_sys_msg);
    // Rendering stats
    render_stat(pLwc);
    render_addr(pLwc);
    // Set rendering flag to 0 (ignoring const-ness......)
    lwcontext_set_rendering((LWCONTEXT*)pLwc, 0);
}

static void bind_all_fvertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int fvbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->fvao[fvbo_index]);
#else
    set_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->vao[vbo_index]);
#else
    set_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_skin_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->skin_vao[vbo_index]);
#else
    set_skin_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_fan_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->fan_vao[vbo_index]);
#else
    set_fan_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

static void bind_all_ps_vertex_attrib_shader(const LWCONTEXT* pLwc, int shader_index, int vbo_index) {
#if LW_PLATFORM_WIN32 || LW_PLATFORM_OSX
    glBindVertexArray(pLwc->ps_vao[vbo_index]);
#else
    set_ps_vertex_attrib_pointer(pLwc, shader_index);
#endif
}

void bind_all_fvertex_attrib(const LWCONTEXT* pLwc, int fvbo_index) {
    bind_all_fvertex_attrib_shader(pLwc, LWST_DEFAULT_NORMAL, fvbo_index);
}

void bind_all_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_vertex_attrib_shader(pLwc, LWST_DEFAULT, vbo_index);
}

void bind_all_vertex_attrib_font(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_vertex_attrib_shader(pLwc, LWST_FONT, vbo_index);
}

void bind_all_vertex_attrib_etc1_with_alpha(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_vertex_attrib_shader(pLwc, LWST_ETC1, vbo_index);
}

void bind_all_skin_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_skin_vertex_attrib_shader(pLwc, LWST_SKIN, vbo_index);
}

void bind_all_fan_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
    bind_all_fan_vertex_attrib_shader(pLwc, LWST_FAN, vbo_index);
}

void bind_all_ps_vertex_attrib(const LWCONTEXT* pLwc, int vbo_index) {
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

static void load_tex_files(LWCONTEXT* pLwc) {
    glGenTextures(MAX_TEX_ATLAS, pLwc->tex_atlas);

    for (int i = 0; i < MAX_TEX_ATLAS; i++) {
        glBindTexture(GL_TEXTURE_2D, pLwc->tex_atlas[i]);

        size_t tex_atlas_filename_len = (int)strlen(tex_atlas_filename[i]);

        size_t filename_index = tex_atlas_filename_len;
        while (tex_atlas_filename[i][filename_index - 1] != PATH_SEPARATOR[0]) {
            filename_index--;
        }

        if (strcmp(tex_atlas_filename[i] + tex_atlas_filename_len - 4, ".ktx") == 0) {
            int width = 0, height = 0;
            if (load_ktx_hw_or_sw(tex_atlas_filename[i], &width, &height) < 0) {
                LOGE("load_tex_files: load_ktx_hw_or_sw failure - %s", tex_atlas_filename[i]);
            } else {
                pLwc->tex_atlas_width[i] = width;
                pLwc->tex_atlas_height[i] = height;
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

        pLwc->tex_atlas_hash[i] = hash((const unsigned char*)&tex_atlas_filename[i][filename_index]);
    }
}

void init_load_textures(LWCONTEXT* pLwc) {
    // Sprites

    load_tex_files(pLwc);

    // Fonts

    load_test_font(pLwc);

    // Textures generated by program

    load_tex_program(pLwc);

    glBindTexture(GL_TEXTURE_2D, 0);

}

void load_test_font(LWCONTEXT* pLwc) {
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

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

#define ALIGNED_STR(S) ((const union { char s[sizeof S]; int align; }){ S }.s)

static void parse_conf(LWCONTEXT* pLwc) {
#define LW_CONF_FILE_NAME "conf.json"
    const char *conf_path = ASSETS_BASE_PATH "conf" PATH_SEPARATOR LW_CONF_FILE_NAME;
    jsmn_parser conf_parser;
    jsmn_init(&conf_parser);
    jsmntok_t conf_token[LW_MAX_CONF_TOKEN];
    LOGI("sizeof(char) == %zu", sizeof(char));
    char *conf_str = create_string_from_file(conf_path);
    if (conf_str) {
        int token_count = jsmn_parse(&conf_parser, conf_str, strlen(conf_str), conf_token, ARRAY_SIZE(conf_token));
        jsmntok_t* t = conf_token;
        if (token_count < 1 || t[0].type != JSMN_OBJECT) {
            LOGE("Conf file broken...");
            fflush(stdout);
            exit(-1);
        }
        LOGI("Conf file: %s", conf_path);
        for (int i = 1; i < token_count; i++) {
            if (jsoneq(conf_str, &t[i], "ClientTcpHost") == 0) {
                LOGI("ClientTcpHost: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->tcp_host_addr.host, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "ConnPort") == 0) {
                LOGI("ConnPort: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                strncpy(pLwc->tcp_host_addr.port_str, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
                pLwc->tcp_host_addr.port = atoi(pLwc->tcp_host_addr.port_str);
            }
        }
        release_string(conf_str);
        conf_str = 0;
    } else {
        LOGE("Conf file not found!");
        fflush(stdout);
        exit(-2);
    }
}

static int str2int(const char* str, int len) {
    int i;
    int ret = 0;
    for (i = 0; i < len; ++i) {
        ret = ret * 10 + (str[i] - '0');
    }
    return ret;
}

static void parse_atlas_conf(LWCONTEXT* pLwc, LWATLASSPRITEARRAY* atlas_array, const char* conf_path) {
    jsmn_parser conf_parser;
    jsmn_init(&conf_parser);
    jsmntok_t conf_token[LW_MAX_CONF_TOKEN];
    LOGI("sizeof(char) == %zu", sizeof(char));
    char *conf_str = create_string_from_file(conf_path);
    if (conf_str) {
        int token_count = jsmn_parse(&conf_parser, conf_str, strlen(conf_str), conf_token, ARRAY_SIZE(conf_token));
        jsmntok_t* t = conf_token;
        if (token_count < 1 || t[0].type != JSMN_OBJECT) {
            LOGE("Conf file broken...");
            fflush(stdout);
            exit(-1);
        }
        LOGI("atlas file: %s", conf_path);
        int entry_count = 0;
        for (int i = 1; i < token_count; i++) {
            if (jsoneq(conf_str, &t[i], "name") == 0) {
                entry_count++;
            }
        }
        LWATLASSPRITE* atlas_sprite = (LWATLASSPRITE*)calloc(entry_count, sizeof(LWATLASSPRITE));
        int entry_index = -1;
        for (int i = 1; i < token_count; i++) {
            if (jsoneq(conf_str, &t[i], "name") == 0) {
                LOGI("name: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                entry_index++;
                strncpy(atlas_sprite[entry_index].name, conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "x") == 0) {
                LOGI("x: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                atlas_sprite[entry_index].x = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "y") == 0) {
                LOGI("y: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                atlas_sprite[entry_index].y = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "width") == 0) {
                LOGI("width: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                atlas_sprite[entry_index].width = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            } else if (jsoneq(conf_str, &t[i], "height") == 0) {
                LOGI("height: %.*s", t[i + 1].end - t[i + 1].start, conf_str + t[i + 1].start);
                atlas_sprite[entry_index].height = str2int(conf_str + t[i + 1].start, t[i + 1].end - t[i + 1].start);
            }
        }
        atlas_array->count = entry_count;
        atlas_array->first = atlas_sprite;
        //free(atlas_sprite);
        release_string(conf_str);
        conf_str = 0;
    } else {
        LOGE("Atlas conf file %s not found!", conf_path);
        fflush(stdout);
        atlas_array->count = 0;
        atlas_array->first = 0;
        exit(-2);
    }
}

static void parse_atlas(LWCONTEXT* pLwc) {
    for (int i = 0; i < LAC_COUNT; i++) {
        parse_atlas_conf(pLwc, &pLwc->atlas_conf[i], atlas_conf_filename[i]);
    }
}

LWCONTEXT* lw_init_initial_size(int width, int height) {

    init_ext_image_lib();

    init_ext_sound_lib();

    //test_image();

    LWCONTEXT* pLwc = (LWCONTEXT *)calloc(1, sizeof(LWCONTEXT));

    pLwc->control_flags = LCF_PUCK_GAME_DASH | /*LCF_PUCK_GAME_JUMP | */LCF_PUCK_GAME_PULL;

    parse_conf(pLwc);
    parse_atlas(pLwc);

    pLwc->width = width;
    pLwc->height = height;

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

    lwparabola_test();

    pLwc->update_frequency = 125;
    pLwc->update_interval = 1.0 / pLwc->update_frequency;// 1 / 120.0;// 0.02; // seconds

    pLwc->puck_game = new_puck_game(pLwc->update_frequency);
    pLwc->puck_game->pLwc = pLwc;

    float dir_pad_origin_x, dir_pad_origin_y;
    get_left_dir_pad_original_center(pLwc->aspect_ratio, &dir_pad_origin_x, &dir_pad_origin_y);
    dir_pad_init(&pLwc->left_dir_pad,
                 dir_pad_origin_x,
                 dir_pad_origin_y,
                 0.1f,
                 0.2f);
    get_right_dir_pad_original_center(pLwc->aspect_ratio, &dir_pad_origin_x, &dir_pad_origin_y);
    dir_pad_init(&pLwc->right_dir_pad,
                 dir_pad_origin_x,
                 dir_pad_origin_y,
                 0.1f,
                 0.2f);
    return pLwc;
}

LWCONTEXT *lw_init(void) {
    return lw_init_initial_size(0, 0);
}

void lw_set_size(LWCONTEXT* pLwc, int w, int h) {
    pLwc->width = w;
    pLwc->height = h;
    if (pLwc->width > 0 && pLwc->height > 0) {
        pLwc->aspect_ratio = (float)pLwc->width / pLwc->height;
    } else {
        if (pLwc->width <= 0) {
            LOGE("Screen width is 0 or below! (%d) Aspect ratio set to 1.0f", pLwc->width);
        }
        if (pLwc->height <= 0) {
            LOGE("Screen heightis 0 or below! (%d) Aspect ratio set to 1.0f", pLwc->height);
        }
        pLwc->aspect_ratio = 1.0f;
    }

    get_left_dir_pad_original_center(pLwc->aspect_ratio,
                                     &pLwc->left_dir_pad.origin_x,
                                     &pLwc->left_dir_pad.origin_y);
    get_right_dir_pad_original_center(pLwc->aspect_ratio,
                                      &pLwc->right_dir_pad.origin_x,
                                      &pLwc->right_dir_pad.origin_y);

    // Update default projection matrix (pLwc->proj)
    logic_udate_default_projection(pLwc);
    // Initialize test font FBO
    init_font_fbo(pLwc);
    // Render font FBO using render-to-texture
    lwc_render_font_test_fbo(pLwc);

    // Reset dir pad input state
    reset_dir_pad_position(&pLwc->left_dir_pad);
    reset_dir_pad_position(&pLwc->right_dir_pad);

    puck_game_reset_view_proj(pLwc, pLwc->puck_game);
}

void lw_set_window(LWCONTEXT* pLwc, struct GLFWwindow *window) {
    pLwc->window = window;
}

struct GLFWwindow *lw_get_window(const LWCONTEXT* pLwc) {
    return pLwc->window;
}

int lw_get_game_scene(LWCONTEXT* pLwc) {
    return pLwc->game_scene;
}

int lw_get_update_count(LWCONTEXT* pLwc) {
    return pLwc->update_count;
}

int lw_get_render_count(LWCONTEXT* pLwc) {
    return pLwc->render_count;
}

void lw_deinit(LWCONTEXT* pLwc) {
    for (int i = 0; i < LAC_COUNT; i++) {
        if (pLwc->atlas_conf[i].first) {
            free(pLwc->atlas_conf[i].first);
            pLwc->atlas_conf[i].first = 0;
            pLwc->atlas_conf[i].count = 0;
        }
    }

    for (int i = 0; i < LVT_COUNT; i++) {
        glDeleteBuffers(1, &pLwc->vertex_buffer[i].vertex_buffer);
    }

    for (int i = 0; i < LFT_COUNT; i++) {
        glDeleteBuffers(1, &pLwc->fvertex_buffer[i].vertex_buffer);
    }

    for (int i = 0; i < LFAT_COUNT; i++) {
        release_binary(pLwc->fanim[i].data);
        memset(&pLwc->fanim[i], 0, sizeof(pLwc->fanim[i]));
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
    glDeleteVertexArrays(LFT_COUNT, pLwc->fvao);
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

    delete_puck_game(&pLwc->puck_game);

    free(pLwc);
}

void lw_on_destroy(LWCONTEXT* pLwc) {
    release_font(pLwc->pFnt);
    release_string(pLwc->dialog);
    deinit_net(pLwc);
    zactor_destroy((zactor_t**)&pLwc->logic_actor);
    lw_deinit(pLwc);
    mq_shutdown();
}

void lw_set_kp(LWCONTEXT* pLwc, int kp) {
    pLwc->kp = kp;
}

double lwcontext_delta_time(const LWCONTEXT* pLwc) {
    return deltatime_delta_time(pLwc->update_dt);
}

void lw_set_push_token(LWCONTEXT* pLwc, int domain, const char* token) {
    tcp_send_push_token(pLwc->tcp, 300, domain, token);
}
