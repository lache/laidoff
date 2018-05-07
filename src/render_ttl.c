#include <string.h>
#include "render_ttl.h"
#include "lwcontext.h"
#include "render_text_block.h"
#include "render_solid.h"
#include "laidoff.h"
#include "htmlui.h"
#include "linmath.h"
#include "lwmath.h"
#include <stdio.h>
#include "lwttl.h"
#include "lwlog.h"
#include "lwlnglat.h"
#include <assert.h>
#include "render_morph.h"

#define MAX_VISIBILITY_ENTRY_COUNT (32)
#define MAX_VISIBILITY_ENTRY_NAME_LENGTH (32)
static char visibility[MAX_VISIBILITY_ENTRY_COUNT][MAX_VISIBILITY_ENTRY_NAME_LENGTH];

#define WATER_COLOR_R (0 / 255.f)
#define WATER_COLOR_G (94 / 255.f)
#define WATER_COLOR_B (190 / 255.f)

void lwc_render_ttl_fbo_body(const LWCONTEXT* pLwc, const char* html_body) {
    glBindFramebuffer(GL_FRAMEBUFFER, pLwc->shared_fbo.fbo);
    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, pLwc->shared_fbo.width, pLwc->shared_fbo.height);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    htmlui_load_render_draw_body(pLwc->htmlui, html_body);

    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lwc_prerender_ttl_fbo(const LWCONTEXT* pLwc) {
    assert(pLwc->shared_fbo.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, pLwc->shared_fbo.fbo);
    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, pLwc->shared_fbo.width, pLwc->shared_fbo.height);
    glClearColor(0, 0, 0, 0); // alpha should be cleared to zero
                              //lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void lwc_postrender_ttl_fbo(const LWCONTEXT* pLwc) {
    glEnable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void lwc_render_ttl_fbo(const LWCONTEXT* pLwc, const char* html_path) {
    lwc_prerender_ttl_fbo(pLwc);
    htmlui_load_render_draw(pLwc->htmlui, html_path);
    lwc_postrender_ttl_fbo(pLwc);
}

typedef struct _LWWAVE {
    int valid;
    float y;
    float age;
} LWWAVE;
static LWWAVE wave[5];
float last_wave_spawn = 0;

static void render_port(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float port_y) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);

    float sx = 1, sy = 1, sz = 1;
    float x = 0, y = port_y, z = 0;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    const LW_VBO_TYPE lvt = LVT_PORT;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glUniform1f(shader->vertex_color_ratio, 0);
    glUniform3f(shader->vertex_color, 0, 0, 0);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_sea_city(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);

    float sx = 1, sy = 1, sz = 1;
    float x = 0, y = 25, z = 0;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    const LW_VBO_TYPE lvt = LVT_SEA_CITY;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glUniform1f(shader->vertex_color_ratio, 0);
    glUniform3f(shader->vertex_color, 0, 0, 0);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_ship(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);
    const int view_scale = lwttl_view_scale(pLwc->ttl);
    const float sx = 0.075f / view_scale;
    const float sy = 0.075f / view_scale;
    const float sz = 0.075f / view_scale;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    mat4x4 model_normal_transform;
    mat4x4_identity(model_normal_transform);

    const LW_VBO_TYPE lvt = LVT_SHIP;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model_normal_transform);
    glUniform1f(shader->vertex_color_ratio, 0);
    glUniform3f(shader->vertex_color, 0, 0, 0);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_seaport_icon(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float w, float h) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);

    mat4x4 model_normal_transform;
    mat4x4_identity(model_normal_transform);

    float sx = w, sy = h, sz = 1;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    const LW_VBO_TYPE lvt = LVT_SEAPORT_ICON;
    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model_normal_transform);
    glUniform1f(shader->vertex_color_ratio, 0);
    glUniform3f(shader->vertex_color, 0, 0, 0);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_cell_color(const LWCONTEXT* pLwc,
                              const mat4x4 view,
                              const mat4x4 proj,
                              float x,
                              float y,
                              float z,
                              float w,
                              float h,
                              LW_VBO_TYPE lvt,
                              float vertex_color_ratio,
                              float vertex_color_r,
                              float vertex_color_g,
                              float vertex_color_b) {
    int shader_index = LWST_DEFAULT_NORMAL_COLOR;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    lazy_glUseProgram(pLwc, shader_index);
    mat4x4 rot;
    mat4x4_identity(rot);

    mat4x4 model_normal_transform;
    mat4x4_identity(model_normal_transform);

    float sx = w, sy = h, sz = 1;
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);

    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);

    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);

    lazy_glBindBuffer(pLwc, lvt);
    bind_all_color_vertex_attrib(pLwc, lvt);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model_normal_transform);
    glUniform1f(shader->vertex_color_ratio, vertex_color_ratio);
    glUniform3f(shader->vertex_color, vertex_color_r, vertex_color_g, vertex_color_b);
    //glShadeModel(GL_FLAT);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}


static void render_cell(const LWCONTEXT* pLwc,
                        const mat4x4 view,
                        const mat4x4 proj,
                        float x,
                        float y,
                        float z,
                        float w,
                        float h,
                        LW_VBO_TYPE lvt) {
    render_cell_color(pLwc,
                      view,
                      proj,
                      x,
                      y,
                      z,
                      w,
                      h,
                      lvt,
                      0,
                      0,
                      0,
                      0);
}

static void render_land_cell(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float w, float h) {
    const LW_VBO_TYPE lvt = LVT_LAND_CELL;
    render_cell(pLwc, view, proj, x, y, z, w, h, lvt);
}

static void render_sea_cell_debug(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float x, float y, float z, float w, float h) {
    const LW_VBO_TYPE lvt = LVT_SEA_CELL_DEBUG;
    render_cell(pLwc, view, proj, x, y, z, w, h, lvt);
}


static void render_timer(const LWCONTEXT* pLwc,
                         const mat4x4 view,
                         const mat4x4 proj,
                         const float x,
                         const float y,
                         const float w,
                         const float h,
                         const float remain_sec,
                         const float total_sec,
                         const float ui_alpha) {
    float remain_ratio = 1.0f;
    if (total_sec != 0) {
        remain_ratio = remain_sec / total_sec;
    }
    const LWSHADER* shader = &pLwc->shader[LWST_RINGGAUGE];
    lazy_glUseProgram(pLwc, LWST_RINGGAUGE);
    glUniform3f(shader->full_color, 0, 1, 0);
    glUniform3f(shader->empty_color, 1, 0, 0);
    glUniform1f(shader->gauge_ratio, remain_ratio);
    // text origin point debug indicator
    render_solid_vb_ui_uv_shader_rot_view_proj(pLwc,
                                               x,
                                               y,
                                               w,
                                               h,
                                               0,//pLwc->tex_atlas[LVT_RINGGAUGE],
                                               LVT_RINGGAUGETHICK,
                                               ui_alpha,
                                               0,
                                               1,
                                               0,
                                               1,
                                               default_uv_offset,
                                               default_uv_scale,
                                               LWST_RINGGAUGE,
                                               (float)M_PI,
                                               view,
                                               proj);
}

static void render_cell_pixel_selector(const LWTTL* ttl,
                                       const LWCONTEXT* pLwc,
                                       const mat4x4 view,
                                       const mat4x4 proj,
                                       const float x,
                                       const float y,
                                       const float z,
                                       const float w,
                                       const float h) {
    const LW_VBO_TYPE lvt = LVT_CELL_PIXEL_SELECTOR;
    render_cell_color(pLwc, view, proj, x, y, z, w, h, lvt, 0.0f, 0.0f, 0.0f, 0.0f);
    float press_menu_gauge_total;
    float press_menu_gauge_appear_delay;
    float press_at;
    float app_time = (float)pLwc->app_time;
    if (lwttl_press_menu_info(ttl,
                              &press_menu_gauge_total,
                              &press_menu_gauge_appear_delay,
                              &press_at)) {
        if (app_time > press_at + press_menu_gauge_appear_delay) {
            const float filled = app_time - (press_at + press_menu_gauge_appear_delay);
            if (filled + LWEPSILON > press_menu_gauge_total) {

            } else {
                render_timer(pLwc,
                             view,
                             proj,
                             x,
                             y + 2.5f,
                             w / 4,
                             h / 4,
                             filled,
                             press_menu_gauge_total,
                             1.0f);
            }
        }
    }
}

static void render_waves(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float ship_y) {
    if (last_wave_spawn + 1.1f < (float)pLwc->app_time) {
        for (int i = 0; i < ARRAY_SIZE(wave); i++) {
            if (wave[i].valid == 0) {
                last_wave_spawn = (float)pLwc->app_time;
                wave[i].age = 0;
                wave[i].y = -5.5f + ship_y;
                wave[i].valid = 1;
                break;
            }
        }
    }
    float delta_time = (float)deltatime_delta_time(pLwc->render_dt);
    for (int i = 0; i < ARRAY_SIZE(wave); i++) {
        if (wave[i].valid) {
            float alpha = LWMAX(0, sinf(wave[i].age * ((float)M_PI / 2.0f)));
            lw_load_tex(pLwc, LAE_WAVE);
            lw_load_tex(pLwc, LAE_WAVE_ALPHA);
            render_solid_vb_ui_alpha_uv_shader_view_proj(pLwc,
                                                         0,
                                                         wave[i].y,
                                                         3.0f + 0.5f * alpha,
                                                         2.5f,
                                                         LAE_WAVE,
                                                         LAE_WAVE_ALPHA,
                                                         LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                                         alpha,
                                                         1.0f,
                                                         1.0f,
                                                         1.0f,
                                                         1.0f,
                                                         default_uv_offset,
                                                         default_uv_scale,
                                                         LWST_ETC1,
                                                         view,
                                                         proj);
            wave[i].y -= delta_time * 1.5f;
            wave[i].age += delta_time;
            if (wave[i].age > 4.0f) {
                wave[i].valid = 0;
            }
        }
    }
}

static void render_morphed_earth(const LWCONTEXT* pLwc,
                                 const mat4x4 view,
                                 const mat4x4 proj,
                                 const LWTTLLNGLAT* center,
                                 int view_scale) {
    const LW_MORPH_VBO_TYPE lmvt = LMVT_EARTH;
    const LW_ATLAS_ENUM lae = LAE_WATER_2048_1024_AA;
    const int tex_index = pLwc->tex_atlas[lae];
    const float earth_globe_scale = lwttl_earth_globe_scale(pLwc->ttl) * (64.0f / view_scale);
    const float earth_globe_morph_weight = lwttl_earth_globe_morph_weight(earth_globe_scale);
    const float x = 0.0f;
    const float y = lwttl_earth_globe_y(center, earth_globe_scale, earth_globe_morph_weight);
    lazy_glBindBuffer(pLwc, lmvt);
    lazy_tex_atlas_glBindTexture(pLwc, lae);
    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 model_scale;
    mat4x4_identity(model_scale);
    mat4x4_scale_aniso(model_scale, model_scale, earth_globe_scale, earth_globe_scale, earth_globe_scale);
    mat4x4_translate(model_translate, x, y, 0);
    mat4x4_identity(model);
    mat4x4_mul(model, model_translate, model_scale);
    const float uv_offset[] = {
        +(center->lng) / 360.0f,
        0.0f,
    };
    render_morph(pLwc,
                 tex_index,
                 lmvt,
                 1.0f,
                 1.0f,
                 1.0f,
                 1.0f,
                 0.0f,
                 proj,
                 view,
                 model,
                 uv_offset,
                 default_flip_y_uv_scale,
                 earth_globe_morph_weight);
}

static void render_earth(const LWCONTEXT* pLwc, const LWTTLLNGLAT* center, int view_scale) {
    const LW_VBO_TYPE lvt = LVT_EARTH;
    const LW_ATLAS_ENUM lae = LAE_WATER_2048_1024_AA;
    const float scale = 2.5f;
    const float x = -(pLwc->aspect_ratio - 0.5f);
    const float y = 0.5f;
    const float alpha_multiplier = 0.5f;
    lazy_glBindBuffer(pLwc, lvt);
    lazy_tex_atlas_glBindTexture(pLwc, lae);

    const int shader_index = LWST_DEFAULT;
    lazy_glUseProgram(pLwc, shader_index);
    glUniform2fv(pLwc->shader[shader_index].vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(pLwc->shader[shader_index].vuvscale_location, 1, default_uv_scale);
    glUniform1f(pLwc->shader[shader_index].alpha_multiplier_location, alpha_multiplier);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(pLwc->shader[shader_index].alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(pLwc->shader[shader_index].overlay_color_location, 0, 0, 0);
    glUniform1f(pLwc->shader[shader_index].overlay_color_ratio_location, 0);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)pLwc->proj);

    const int tex_index = pLwc->tex_atlas[lae];
    mat4x4 model_translate;
    mat4x4 model;
    mat4x4 view_model;
    mat4x4 proj_view_model;
    mat4x4 model_scale;
    mat4x4 model_scale_rotate_1;
    mat4x4 model_scale_rotate;
    mat4x4 view_identity;
    mat4x4_identity(view_identity);
    mat4x4_identity(model_scale);
    mat4x4_scale_aniso(model_scale, model_scale, scale, scale, 0.0f);
    mat4x4_rotate_X(model_scale_rotate_1, model_scale, (float)LWDEG2RAD(center->lat));
    mat4x4_rotate_Y(model_scale_rotate, model_scale_rotate_1, (float)LWDEG2RAD(center->lng));
    mat4x4_translate(model_translate, x, y, 0);
    mat4x4_identity(model);
    mat4x4_mul(model, model_translate, model_scale_rotate);
    mat4x4_mul(view_model, view_identity, model);
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, pLwc->proj, view_model);

    lazy_glBindBuffer(pLwc, lvt);
    bind_all_vertex_attrib(pLwc, lvt);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    //assert(tex_index);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);

    //render_solid_box_ui_lvt_flip_y_uv(pLwc,
    //                                  pLwc->aspect_ratio - 0.5f,
    //                                  0.5f,
    //                                  1.0f,
    //                                  1.0f,
    //                                  pLwc->tex_atlas[LAE_WATER_2048_1024_AA],
    //                                  LVT_EARTH,
    //                                  0);

    const float half_lng_extent_in_deg = lwttl_half_lng_extent_in_degrees(view_scale);
    const float half_lat_extent_in_deg = lwttl_half_lat_extent_in_degrees(view_scale);
    const float lng_min = center->lng - half_lng_extent_in_deg;
    const float lng_max = center->lng + half_lng_extent_in_deg;
    const float lat_min = center->lat - half_lat_extent_in_deg;
    const float lat_max = center->lat + half_lat_extent_in_deg;
    const float lng_extent = lng_max - lng_min;
    const float lat_extent = lat_max - lat_min;
    // current view window indicator
    render_solid_vb_ui_flip_y_uv(pLwc,
                                 x,
                                 y,
                                 scale * 2 * sinf((float)LWDEG2RAD(lng_extent) / 2),
                                 scale * 2 * sinf((float)LWDEG2RAD(lat_extent) / 2),
                                 pLwc->tex_atlas[LAE_ZERO_FOR_BLACK],
                                 LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                 0.5f,
                                 0.0f,
                                 1.0f,
                                 0.2f,
                                 1.0f,
                                 0);
}

static void render_sea_objects_nameplate(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWTTLLNGLAT* center) {
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, proj, view);
    const int view_scale = lwttl_view_scale(pLwc->ttl);
    const LWPTTLFULLSTATE* ttl_full_state = lwttl_full_state(pLwc->ttl);

    for (int i = 0; i < ttl_full_state->count; i++) {
        const float x = (ttl_full_state->obj[i].fx1 + ttl_full_state->obj[i].fx0) / 2;
        const float y = (ttl_full_state->obj[i].fy1 + ttl_full_state->obj[i].fy0) / 2;
        const float rx = cell_fx_to_render_coords(x, center, view_scale);
        const float ry = cell_fy_to_render_coords(y, center, view_scale);
        vec4 obj_pos_vec4 = {
            rx,
            ry,
            0,
            1,
        };
        vec2 ui_point;
        calculate_ui_point_from_world_point(pLwc->aspect_ratio, proj_view, obj_pos_vec4, ui_point);
        LWTEXTBLOCK test_text_block;
        test_text_block.text_block_width = 999.0f;// 2.00f * aspect_ratio;
        test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
        test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
        SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
        SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
        SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
        SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
        char obj_nameplate[256];
        sprintf(obj_nameplate,
                "%d %s %.0f",
                //ttl_full_state->obj[i].id, // sea-server instance id
                ttl_full_state->obj[i].type, // laidoff-server db id
                ttl_full_state->obj[i].guid,
                ttl_full_state->obj[i].route_left);
        test_text_block.text = obj_nameplate;
        test_text_block.text_bytelen = (int)strlen(test_text_block.text);
        test_text_block.begin_index = 0;
        test_text_block.end_index = test_text_block.text_bytelen;
        test_text_block.multiline = 1;
        test_text_block.text_block_x = ui_point[0];
        test_text_block.text_block_y = ui_point[1];
        test_text_block.align = LTBA_LEFT_TOP;
        render_text_block(pLwc, &test_text_block);
    }
}

static void render_sea_objects(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWTTLLNGLAT* center) {
    const int view_scale = lwttl_view_scale(pLwc->ttl);
    const LWPTTLFULLSTATE* ttl_full_state = lwttl_full_state(pLwc->ttl);
    for (int i = 0; i < ttl_full_state->count; i++) {
        const float x = (ttl_full_state->obj[i].fx1 + ttl_full_state->obj[i].fx0) / 2;
        const float y = (ttl_full_state->obj[i].fy1 + ttl_full_state->obj[i].fy0) / 2;
        const float rx = cell_fx_to_render_coords(x, center, view_scale);
        const float ry = cell_fy_to_render_coords(y, center, view_scale);
        render_ship(pLwc,
                    view,
                    proj,
                    rx,
                    ry,
                    0);
    }

    // test ship
    float x = cell_fx_to_render_coords(0, center, view_scale);
    float y = cell_fy_to_render_coords(0, center, view_scale);
    render_ship(pLwc,
                view,
                proj,
                x,
                y,
                0);
}

static void render_waypoint_line_segment(const LWTTL* ttl,
                                         const LWCONTEXT* pLwc,
                                         const mat4x4 view,
                                         const mat4x4 proj,
                                         const LWTTLLNGLAT* center,
                                         int x0,
                                         int y0,
                                         int x1,
                                         int y1) {
    const int view_scale = lwttl_view_scale(pLwc->ttl);
    const int view_scale_msb = msb_index(view_scale);
    const float lng0_not_clamped = cell_fx_to_lng(x0 + 0.5f);
    const float lat0_not_clamped = cell_fy_to_lat(y0 + 0.5f);
    const float lng1_not_clamped = cell_fx_to_lng(x1 + 0.5f);
    const float lat1_not_clamped = cell_fy_to_lat(y1 + 0.5f);

    const float cell_x0 = lng_to_render_coords(lng0_not_clamped, center, view_scale);
    const float cell_y0 = lat_to_render_coords(lat0_not_clamped, center, view_scale);
    const float cell_x1 = lng_to_render_coords(lng1_not_clamped, center, view_scale);
    const float cell_y1 = lat_to_render_coords(lat1_not_clamped, center, view_scale);

    const float dx = cell_x1 - cell_x0;
    const float dy = cell_y1 - cell_y0;
    const float dlen = sqrtf(dx * dx + dy * dy);

    const float thickness = 0.2f / sqrtf((float)(view_scale_msb + 1));
    const float rot_z = atan2f(dy, dx);

    render_solid_vb_ui_uv_shader_rot_view_proj(pLwc,
                                               cell_x0,
                                               cell_y0,
                                               dlen,
                                               thickness,
                                               pLwc->tex_atlas[LAE_ZERO_FOR_BLACK],
                                               LVT_LEFT_CENTER_ANCHORED_SQUARE,
                                               1.0f,
                                               0.2f,
                                               0.7f,
                                               0.2f,
                                               1.0f,
                                               default_uv_offset,
                                               default_uv_scale,
                                               LWST_DEFAULT,
                                               rot_z,
                                               view,
                                               proj);
}

static void render_waypoints(const LWTTL* ttl,
                             const LWCONTEXT* pLwc,
                             const mat4x4 view,
                             const mat4x4 proj,
                             const LWTTLLNGLAT* center) {
    const LWPTTLWAYPOINTS* waypoints = lwttl_get_waypoints(ttl);
    for (int i = 0; i < waypoints->count - 1; i++) {
        render_waypoint_line_segment(ttl,
                                     pLwc,
                                     view,
                                     proj,
                                     center,
                                     waypoints->waypoints[i + 0].x,
                                     waypoints->waypoints[i + 0].y,
                                     waypoints->waypoints[i + 1].x,
                                     waypoints->waypoints[i + 1].y);
    }
}

static void render_seaports(const LWCONTEXT* pLwc,
                            const mat4x4 view,
                            const mat4x4 proj,
                            const LWTTLLNGLAT* center) {
    const int clamped_view_scale = lwttl_clamped_view_scale(pLwc->ttl);
    const int clamped_to_original_view_scale_ratio = lwttl_view_scale(pLwc->ttl) / clamped_view_scale;
    const float half_lng_extent_in_deg = lwttl_half_lng_extent_in_degrees(clamped_view_scale);
    const float half_lat_extent_in_deg = lwttl_half_lat_extent_in_degrees(clamped_view_scale);
    const float lng_min = center->lng - half_lng_extent_in_deg;
    const float lng_max = center->lng + half_lng_extent_in_deg;
    const float lat_min = center->lat - half_lat_extent_in_deg;
    const float lat_max = center->lat + half_lat_extent_in_deg;

    const float cell_render_width = cell_x_to_render_coords(1, center, clamped_view_scale) - cell_x_to_render_coords(0, center, clamped_view_scale);
    const float cell_render_height = cell_y_to_render_coords(0, center, clamped_view_scale) - cell_y_to_render_coords(1, center, clamped_view_scale);
    const int view_scale_msb = msb_index(clamped_view_scale);
    const float size_ratio = 1.0f / sqrtf((float)(view_scale_msb + 1));

    int chunk_index_array[(1 + LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG + 1)*(1 + LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT + 1)];
    const int chunk_index_array_count = lwttl_query_chunk_range_seaport(pLwc->ttl,
                                                                        lng_min,
                                                                        lng_max,
                                                                        lat_min,
                                                                        lat_max,
                                                                        clamped_view_scale,
                                                                        chunk_index_array,
                                                                        ARRAY_SIZE(chunk_index_array));
    if (chunk_index_array_count > ARRAY_SIZE(chunk_index_array)) {
        LOGEP("incorrect query result");
        assert(0);
    }
    const int chunk_index_max = LWMIN(chunk_index_array_count, ARRAY_SIZE(chunk_index_array));
    for (int ci = 0; ci < chunk_index_max; ci++) {
        int obj_count = 0;
        int xc0 = 0;
        int yc0 = 0;
        const LWPTTLSEAPORTOBJECT* obj_begin = lwttl_query_chunk_seaport(pLwc->ttl,
                                                                         chunk_index_array[ci],
                                                                         &xc0,
                                                                         &yc0,
                                                                         &obj_count);
        if (obj_begin && obj_count > 0) {
            for (int i = 0; i < obj_count; i++) {
                const float x0 = (float)(xc0 + clamped_view_scale * obj_begin[i].x_scaled_offset_0);
                const float y0 = (float)(yc0 + clamped_view_scale * obj_begin[i].y_scaled_offset_0);

                const float lng0_not_clamped = cell_fx_to_lng(x0 + 0.5f);
                const float lat0_not_clamped = cell_fy_to_lat(y0 + 0.5f);

                const float cell_x0 = lng_to_render_coords(lng0_not_clamped, center, clamped_view_scale * clamped_to_original_view_scale_ratio);
                const float cell_y0 = lat_to_render_coords(lat0_not_clamped, center, clamped_view_scale * clamped_to_original_view_scale_ratio);

                render_seaport_icon(pLwc,
                                    view,
                                    proj,
                                    cell_x0,
                                    cell_y0,
                                    0,
                                    cell_render_width * clamped_view_scale * size_ratio,
                                    cell_render_height * clamped_view_scale * size_ratio);
            }
        }
    }
}

static void render_background_sea_water(const LWCONTEXT* pLwc,
                                        const mat4x4 view,
                                        const mat4x4 proj,
                                        const LWTTLLNGLAT* center,
                                        const float lng_min,
                                        const float lng_max,
                                        const float lat_min,
                                        const float lat_max,
                                        const int view_scale) {
    const float cell_x0 = lng_to_render_coords(lng_min, center, view_scale);
    const float cell_y0 = lat_to_render_coords(lat_max, center, view_scale);
    const float cell_x1 = lng_to_render_coords(lng_max, center, view_scale);
    const float cell_y1 = lat_to_render_coords(lat_min, center, view_scale);
    const float cell_w = cell_x1 - cell_x0;
    const float cell_h = cell_y0 - cell_y1;
    render_solid_vb_ui_uv_shader_rot_view_proj(pLwc,
                                               0,
                                               0,
                                               cell_w,
                                               cell_h,
                                               0,
                                               LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                               0.75f,
                                               WATER_COLOR_R,
                                               WATER_COLOR_G,
                                               WATER_COLOR_B,
                                               1.0f,
                                               default_uv_offset,
                                               default_uv_scale,
                                               LWST_DEFAULT,
                                               0,
                                               view,
                                               proj);
}

static void render_sea_static_objects(const LWCONTEXT* pLwc,
                                      const mat4x4 view,
                                      const mat4x4 proj,
                                      const LWTTLLNGLAT* center) {
    const int clamped_view_scale = lwttl_clamped_view_scale(pLwc->ttl);
    const int clamped_to_original_view_scale_ratio = lwttl_view_scale(pLwc->ttl) / clamped_view_scale;
    const float half_lng_extent_in_deg = lwttl_half_lng_extent_in_degrees(clamped_view_scale);
    const float half_lat_extent_in_deg = lwttl_half_lat_extent_in_degrees(clamped_view_scale);
    const float lng_min = center->lng - half_lng_extent_in_deg;
    const float lng_max = center->lng + half_lng_extent_in_deg;
    const float lat_min = center->lat - half_lat_extent_in_deg;
    const float lat_max = center->lat + half_lat_extent_in_deg;

    render_background_sea_water(pLwc,
                                view,
                                proj,
                                center,
                                lng_min,
                                lng_max,
                                lat_min,
                                lat_max,
                                clamped_view_scale * clamped_to_original_view_scale_ratio);

    // land
    //lwttl_lock_rendering_mutex(pLwc->ttl);
    int chunk_index_array[(1 + LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG + 1)*(1 + LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT + 1)];
    const int chunk_index_array_count = lwttl_query_chunk_range_land(pLwc->ttl,
                                                                     lng_min,
                                                                     lng_max,
                                                                     lat_min,
                                                                     lat_max,
                                                                     clamped_view_scale,
                                                                     chunk_index_array,
                                                                     ARRAY_SIZE(chunk_index_array));
    if (chunk_index_array_count > ARRAY_SIZE(chunk_index_array)) {
        LOGEP("incorrect query result");
        assert(0);
    }
    const int chunk_index_max = LWMIN(chunk_index_array_count, ARRAY_SIZE(chunk_index_array));
    for (int ci = 0; ci < chunk_index_max; ci++) {
        int obj_count = 0;
        int xc0 = 0;
        int yc0 = 0;
        const LWPTTLSTATICOBJECT2* obj_begin = lwttl_query_chunk_land(pLwc->ttl,
                                                                      chunk_index_array[ci],
                                                                      &xc0,
                                                                      &yc0,
                                                                      &obj_count);
        if (obj_begin && obj_count > 0) {
            for (int i = 0; i < obj_count; i++) {
                const float x0 = (float)(xc0 + clamped_view_scale * obj_begin[i].x_scaled_offset_0);
                const float y0 = (float)(yc0 + clamped_view_scale * obj_begin[i].y_scaled_offset_0);
                const float x1 = (float)(xc0 + clamped_view_scale * obj_begin[i].x_scaled_offset_1);
                const float y1 = (float)(yc0 + clamped_view_scale * obj_begin[i].y_scaled_offset_1);

                const float lng0_not_clamped = cell_fx_to_lng(x0);
                const float lat0_not_clamped = cell_fy_to_lat(y0);
                const float lng1_not_clamped = cell_fx_to_lng(x1);
                const float lat1_not_clamped = cell_fy_to_lat(y1);

                const float lng0 = LWCLAMP(lng0_not_clamped, lng_min, lng_max);
                const float lat0 = LWCLAMP(lat0_not_clamped, lat_min, lat_max);
                const float lng1 = LWCLAMP(lng1_not_clamped, lng_min, lng_max);
                const float lat1 = LWCLAMP(lat1_not_clamped, lat_min, lat_max);

                const float cell_x0 = lng_to_render_coords(lng0, center, clamped_view_scale * clamped_to_original_view_scale_ratio);
                const float cell_y0 = lat_to_render_coords(lat0, center, clamped_view_scale * clamped_to_original_view_scale_ratio);
                const float cell_x1 = lng_to_render_coords(lng1, center, clamped_view_scale * clamped_to_original_view_scale_ratio);
                const float cell_y1 = lat_to_render_coords(lat1, center, clamped_view_scale * clamped_to_original_view_scale_ratio);
                const float cell_w = cell_x1 - cell_x0;
                const float cell_h = cell_y0 - cell_y1; // cell_y0 and cell_y1 are in OpenGL rendering coordinates (always cell_y0 > cell_y1)
                                                        // skip degenerated cell
                if (cell_w <= 0 || cell_h <= 0) {
                    continue;
                }
                render_land_cell(pLwc,
                                 view,
                                 proj,
                                 cell_x0,
                                 cell_y0,
                                 0,
                                 cell_w,
                                 cell_h);
            }
        }
    }
    //lwttl_unlock_rendering_mutex(pLwc->ttl);



    //// water cells for debugging
    //struct {
    //    int x0, y0;
    //    int x1, y1;
    //} debug_cell[] = {
    //{ 148621, 24976, 148819, 26934 },
    //{ 148819, 25452, 148823, 26761 },
    //{ 148823, 26752, 148866, 26761 },
    //{ 148866, 26758, 148874, 26761 },
    //{ 148874, 26753, 148876, 26760 },
    //{ 148876, 26752, 149143, 26761 },
    //{ 149143, 26752, 149234, 26817 },
    //{ 149200, 26678, 149222, 26752 },
    //{ 149200, 25214, 149271, 26678 },
    //};
    //for (int i = 0; i < ARRAY_SIZE(debug_cell); i++) {
    //    const float rx0 = cell_x_to_render_coords(debug_cell[i].x0, center, clamped_view_scale);
    //    const float ry0 = cell_y_to_render_coords(debug_cell[i].y0, center, clamped_view_scale);
    //    const float rx1 = cell_x_to_render_coords(debug_cell[i].x1, center, clamped_view_scale);
    //    const float ry1 = cell_y_to_render_coords(debug_cell[i].y1, center, clamped_view_scale);
    //    const float rw = rx1 - rx0;
    //    const float rh = ry0 - ry1; // cell_y0 and cell_y1 are in OpenGL rendering coordinates (always cell_y0 > cell_y1)
    //    render_sea_cell_debug(pLwc, view, proj, rx0, ry0, 0, rw, rh);
    //}
}

static void render_single_cell_info(const LWCONTEXT* pLwc,
                                    const float x,
                                    const float y,
                                    const mat4x4 view,
                                    const mat4x4 proj,
                                    const int view_scale) {
    if (lwttl_selected(pLwc->ttl, 0) == 0) {
        return;
    }
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, proj, view);
    vec4 obj_pos_vec4 = {
        x,
        y,
        0,
        1,
    };
    vec2 ui_point;
    calculate_ui_point_from_world_point(pLwc->aspect_ratio, proj_view, obj_pos_vec4, ui_point);

    LWTEXTBLOCK tb;
    tb.text_block_width = 999.0f;// 2.00f * aspect_ratio;
    tb.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    tb.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    SET_COLOR_RGBA_FLOAT(tb.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(tb.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(tb.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(tb.color_emp_outline, 0, 0, 0, 1);
    char info[512];
    const LWPTTLSINGLECELL* p = lwttl_single_cell(pLwc->ttl);
    const char* cell_type = 0;
    if ((p->attr >> 0) & 1) {
        cell_type = "Land";
    } else if ((p->attr >> 2) & 1) {
        cell_type = "Seawater";
    } else {
        cell_type = "Water";
    }
    if (p->port_id >= 0 && p->port_name) {
        sprintf(info,
                "%s[%d] %s",
                p->port_name,
                p->port_id,
                cell_type);
    } else {
        sprintf(info,
                "%s",
                cell_type);
    }
    tb.text = info;
    tb.text_bytelen = (int)strlen(tb.text);
    tb.begin_index = 0;
    tb.end_index = tb.text_bytelen;
    tb.multiline = 1;
    tb.text_block_x = ui_point[0];
    tb.text_block_y = ui_point[1] + 0.05f / view_scale;
    tb.align = LTBA_LEFT_BOTTOM;
    render_text_block(pLwc, &tb);
}

static void render_cell_pixel_selector_lng_lat(const LWTTL* ttl,
                                               const LWCONTEXT* pLwc,
                                               const mat4x4 view,
                                               const mat4x4 proj,
                                               const int xc0,
                                               const int yc0,
                                               const LWTTLLNGLAT* center,
                                               const int view_scale) {
    const float cell_render_width = cell_x_to_render_coords(1, center, view_scale) - cell_x_to_render_coords(0, center, view_scale);
    const float cell_render_height = cell_y_to_render_coords(0, center, view_scale) - cell_y_to_render_coords(1, center, view_scale);
    const float selector_rx = cell_x_to_render_coords(xc0, center, view_scale);
    const float selector_ry = cell_y_to_render_coords(yc0, center, view_scale);
    render_cell_pixel_selector(ttl,
                               pLwc,
                               view,
                               proj,
                               selector_rx,
                               selector_ry,
                               0,
                               cell_render_width,
                               cell_render_height);
    render_single_cell_info(pLwc,
                            selector_rx,
                            selector_ry,
                            view,
                            proj,
                            view_scale);
}

static void render_sea_static_objects_nameplate(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWTTLLNGLAT* center) {
    mat4x4 proj_view;
    mat4x4_identity(proj_view);
    mat4x4_mul(proj_view, proj, view);
    const int view_scale = lwttl_view_scale(pLwc->ttl);
    //const LWPTTLSEAPORTSTATE* ttl_seaport_state = lwttl_seaport_state(pLwc->ttl);
    //for (int i = 0; i < ttl_seaport_state->count; i++) {
        //const float x = cell_fx_to_render_coords(ttl_seaport_state->obj[i].x0 + 1.0f, center, view_scale);
        //const float y = cell_fy_to_render_coords(ttl_seaport_state->obj[i].y0 + 0.5f, center, view_scale);
        //vec4 obj_pos_vec4 = {
        //    x,
        //    y,
        //    0,
        //    1,
        //};
        //vec2 ui_point;
        //calculate_ui_point_from_world_point(pLwc->aspect_ratio, proj_view, obj_pos_vec4, ui_point);
        //LWTEXTBLOCK test_text_block;
        //test_text_block.text_block_width = 999.0f;// 2.00f * aspect_ratio;
        //test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
        //test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
        //SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
        //SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
        //SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
        //SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
        //test_text_block.text = ttl_seaport_state->obj[i].name;
        //test_text_block.text_bytelen = (int)strlen(test_text_block.text);
        //test_text_block.begin_index = 0;
        //test_text_block.end_index = test_text_block.text_bytelen;
        //test_text_block.multiline = 1;
        //test_text_block.text_block_x = ui_point[0];
        //test_text_block.text_block_y = ui_point[1];
        //test_text_block.align = LTBA_LEFT_CENTER;
        //render_text_block(pLwc, &test_text_block);
    //}
}

static void render_world(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, float ship_y, const LWTTLLNGLAT* center) {
    //render_ship(pLwc, view, proj, 0, ship_y, 0);
    render_sea_objects(pLwc, view, proj, center);
    //render_port(pLwc, view, proj, 0);
    //render_port(pLwc, view, proj, 160);
    //render_sea_city(pLwc, view, proj);
    //render_waves(pLwc, view, proj, ship_y);
}

static void degrees_to_dms(int* d, int* m, float* s, const float degrees) {
    *d = (int)degrees;
    const float minutes = (degrees - *d) * 60;
    *m = (int)minutes;
    *s = (minutes - *m) * 60;
}

static void render_coords(const LWCONTEXT* pLwc, const LWTTLLNGLAT* lng_lat_center) {
    LWTEXTBLOCK test_text_block;
    test_text_block.text_block_width = 999.0f;// 2.00f * aspect_ratio;
    test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
    char coords[256];
    snprintf(coords,
             ARRAY_SIZE(coords),
             "LNG %.3f LAT %.3f",
             lng_lat_center->lng,
             lng_lat_center->lat);
    coords[ARRAY_SIZE(coords) - 1] = 0;
    test_text_block.text = coords;
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.multiline = 1;
    test_text_block.text_block_x = -pLwc->aspect_ratio;
    test_text_block.text_block_y = 1.0f;
    test_text_block.align = LTBA_LEFT_TOP;
    render_text_block(pLwc, &test_text_block);
}

static void render_coords_dms(const LWCONTEXT* pLwc, const LWTTLLNGLAT* lng_lat_center) {
    LWTEXTBLOCK test_text_block;
    test_text_block.text_block_width = 999.0f;// 2.00f * aspect_ratio;
    test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
    char coords[256];
    int lng_d, lng_m;
    float lng_s;
    int lat_d, lat_m;
    float lat_s;
    degrees_to_dms(&lng_d, &lng_m, &lng_s, lng_lat_center->lng);
    degrees_to_dms(&lat_d, &lat_m, &lat_s, lng_lat_center->lat);
    snprintf(coords,
             ARRAY_SIZE(coords),
             "LNG\n%dD\n%dM\n%.3fS\nLAT\n%dD\n%dM\n%.3fS",
             lng_d,
             lng_m,
             lng_s,
             lat_d,
             lat_m,
             lat_s);
    coords[ARRAY_SIZE(coords) - 1] = 0;
    test_text_block.text = coords;
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.multiline = 1;
    test_text_block.text_block_x = -pLwc->aspect_ratio + 0.3f;
    test_text_block.text_block_y = 0;
    test_text_block.align = LTBA_LEFT_TOP;
    render_text_block(pLwc, &test_text_block);
}

static void render_region_name(const LWCONTEXT* pLwc) {
    LWTEXTBLOCK test_text_block;
    test_text_block.text_block_width = 999.0f;// 2.00f * aspect_ratio;
    test_text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_F;
    test_text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(test_text_block.color_emp_outline, 0, 0, 0, 1);
    test_text_block.text = lwttl_seaarea(pLwc->ttl);
    test_text_block.text_bytelen = (int)strlen(test_text_block.text);
    test_text_block.begin_index = 0;
    test_text_block.end_index = test_text_block.text_bytelen;
    test_text_block.multiline = 1;
    test_text_block.text_block_x = -pLwc->aspect_ratio;
    test_text_block.text_block_y = -1.0f;
    test_text_block.align = LTBA_LEFT_BOTTOM;
    render_text_block(pLwc, &test_text_block);
}

void lwc_render_ttl(const LWCONTEXT* pLwc) {
    LW_GL_VIEWPORT();
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mat4x4 view, proj;
    lwttl_view_proj(pLwc->ttl, view, proj);

    const int view_scale = lwttl_view_scale(pLwc->ttl);
    const LWTTLLNGLAT view_center = *lwttl_center(pLwc->ttl);
    // render earth minimap
    //render_earth(pLwc, &view_center, view_scale);
    // render morphed earth
    render_morphed_earth(pLwc, view, proj, &view_center, view_scale);
    glDisable(GL_DEPTH_TEST);
    // render land
    if (lwc_render_ttl_render("landcell")) {
        render_sea_static_objects(pLwc, view, proj, &view_center);
    }
    render_waypoints(pLwc->ttl, pLwc, view, proj, &view_center);
    render_seaports(pLwc, view, proj, &view_center);
    glEnable(GL_DEPTH_TEST);
    // render sea objects(ships)
    if (lwc_render_ttl_render("world")) {
        render_world(pLwc, view, proj, 0, &view_center);
    }
    glDisable(GL_DEPTH_TEST);
    int selected_xc0;
    int selected_yc0;
    if (lwttl_selected_int(pLwc->ttl, &selected_xc0, &selected_yc0)) {
        render_cell_pixel_selector_lng_lat(pLwc->ttl,
                                           pLwc,
                                           view,
                                           proj,
                                           selected_xc0,
                                           selected_yc0,
                                           &view_center,
                                           view_scale);
    }
    // UI
    if (lwc_render_ttl_render("world")) {
        render_sea_objects_nameplate(pLwc, view, proj, &view_center);
    }
    if (lwc_render_ttl_render("landcell_nameplate")) {
        render_sea_static_objects_nameplate(pLwc, view, proj, &view_center);
    }
    //render_coords(pLwc, &view_center);
    render_region_name(pLwc);
    //render_coords_dms(pLwc, &view_center);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    // render FBO (HTML UI)
    render_solid_box_ui_lvt_flip_y_uv(pLwc, 0, 0, 2 * pLwc->aspect_ratio, 2, pLwc->shared_fbo.color_tex, LVT_CENTER_CENTER_ANCHORED_SQUARE, 1);
    // render joystick
    if (0) {
        render_dir_pad_with_start_joystick(pLwc, &pLwc->left_dir_pad, 1.0f);
    }
    glEnable(GL_DEPTH_TEST);
}

int lwc_render_ttl_render(const char* name) {
    size_t i;
    for (i = 0; i < MAX_VISIBILITY_ENTRY_COUNT; i++) {
        if (strcmp(visibility[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

void lwc_render_ttl_enable_render(const char* name, int v) {
    size_t i;
    for (i = 0; i < MAX_VISIBILITY_ENTRY_COUNT; i++) {
        if (strcmp(visibility[i], name) == 0) {
            if (v) {
                // already visible...
                return;
            } else {
                // clear
                visibility[i][0] = 0;
                return;
            }
        }
    }
    if (v) {
        for (i = 0; i < MAX_VISIBILITY_ENTRY_COUNT; i++) {
            if (visibility[i][0] == 0) {
                strcpy(visibility[i], name);
                return;
            }
        }
        LOGE("visibility_hash capacity exceeded.");
    }
}
