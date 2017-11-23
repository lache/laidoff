#include "lwcontext.h"
#include "render_physics.h"
#include "render_solid.h"
#include "laidoff.h"
#include "lwlog.h"
#include "puckgame.h"
#include "render_field.h"
#include "lwtextblock.h"
#include "render_text_block.h"
#include "lwudp.h"
#include "lwdamagetext.h"
#include "lwdirpad.h"

typedef struct _LWSPHERERENDERUNIFORM {
    float sphere_col_ratio[3];
    float sphere_pos[3][3];
    float sphere_col[3][3];
    float sphere_speed[3];
    float sphere_move_rad[3];
    float reflect_size[3];
    float arrowRotMat2[2][2];
    float arrow_center[2];
    float arrow_scale;
} LWSPHERERENDERUNIFORM;

typedef struct _LWTOWERRENDERDATA {
    LW_VBO_TYPE lvt;
    float r;
    float g;
    float b;
} LWTOWERRENDERDATA;

static void render_tower(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const float* pos, const LWPUCKGAMETOWER* tower) {
    int shader_index = LWST_DEFAULT;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    glUseProgram(shader->program);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, 1.0f);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, 1, 1, 1);
    glUniform1f(shader->overlay_color_ratio_location, 0);
    
    mat4x4 rot;
    mat4x4_identity(rot);
    float sx = 1.0f;
    float sy = 1.0f;
    float sz = 1.0f;
    
    float hp_ratio = 0;
    const int remote = pLwc->puck_game->remote;
    if (remote) {
        float hp_ratio1 = 0;
        float hp_ratio2 = 0;
        if (pLwc->puck_game_state.player_total_hp) {
            hp_ratio1 = (float)pLwc->puck_game_state.player_current_hp / pLwc->puck_game_state.player_total_hp;
        }
        if (pLwc->puck_game_state.target_total_hp) {
            hp_ratio2 = (float)pLwc->puck_game_state.target_current_hp / pLwc->puck_game_state.target_total_hp;
        }
        
        if (tower->owner_player_no == 1) {
            if (pLwc->puck_game->player_no == 2) {
                hp_ratio = hp_ratio2;
            } else {
                hp_ratio = hp_ratio1;
            }
        } else {
            if (pLwc->puck_game->player_no == 2) {
                hp_ratio = hp_ratio1;
            } else {
                hp_ratio = hp_ratio2;
            }
        }
    } else {
        hp_ratio = (float)tower->hp / pLwc->puck_game->tower_total_hp;
    }
    int hp = (int)ceilf(pLwc->puck_game->tower_total_hp * hp_ratio);

    glUniform1f(shader->overlay_color_ratio_location, 1);

    LWTOWERRENDERDATA tower_render_data[] = {
        { LVT_TOWER_BASE, 0.1f, 0.1f, 0.1f },
        { LVT_TOWER_1, 0.2f, 0.9f, 0.2f },
        { LVT_TOWER_2, 0.2f, 0.9f, 0.2f },
        { LVT_TOWER_3, 0.2f, 0.9f, 0.2f },
        { LVT_TOWER_4, 0.2f, 0.9f, 0.2f },
        { LVT_TOWER_5, 0.2f, 0.9f, 0.2f },
    };
    for (int i = 0; i < ARRAY_SIZE(tower_render_data); i++) {
        float x = pos[0];
        float y = pos[1];
        float z = pos[2];
        // Positioinal offset by shake
        if (tower->shake_remain_time > 0) {
            const float ratio = tower->shake_remain_time / pLwc->puck_game->tower_shake_time;
            const float shake_magnitude = 0.03f;
            x += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * pLwc->aspect_ratio;
            y += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude;
        }

        mat4x4 model;
        mat4x4_identity(model);
        mat4x4_mul(model, model, rot);
        mat4x4_scale_aniso(model, model, sx, sy, sz);
        //mat4x4_scale_aniso(model, model, 5, 5, 5);

        mat4x4 model_translate;
        mat4x4_translate(model_translate, x, y, z);

        mat4x4_mul(model, model_translate, model);

        mat4x4 view_model;
        mat4x4_mul(view_model, view, model);

        mat4x4 proj_view_model;
        mat4x4_identity(proj_view_model);
        mat4x4_mul(proj_view_model, proj, view_model);

        const LWTOWERRENDERDATA* d = tower_render_data + i;
        float r = d->r;
        float g = d->g;
        float b = d->b;
        if (i > 0 && 5 - (i - 1) > hp) {
            r = 0.7f;
            g = 0.1f;
        }
        glUniform3f(shader->overlay_color_location, r, g, b);
        const LW_VBO_TYPE lvt = d->lvt;
        
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
        bind_all_vertex_attrib(pLwc, lvt);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        set_tex_filter(GL_LINEAR, GL_LINEAR);
        //set_tex_filter(GL_NEAREST, GL_NEAREST);
        glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
        glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
    }
}

static void render_go(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj, const LWPUCKGAMEOBJECT* go, int tex_index, float render_scale, const float* remote_pos, const mat4x4 remote_rot, int remote, float speed) {
    int shader_index = LWST_DEFAULT;
    const LWSHADER* shader = &pLwc->shader[shader_index];
    glUseProgram(shader->program);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, 1.0f);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, 1, 1, 1);
    glUniform1f(shader->overlay_color_ratio_location, 0);
    
    mat4x4 rot;
    mat4x4_identity(rot);
    float sx = render_scale * go->radius, sy = render_scale * go->radius, sz = render_scale * go->radius;
    float x = render_scale * (remote_pos ? remote_pos[0] : go->pos[0]);
    float y = render_scale * (remote_pos ? remote_pos[1] : go->pos[1]);
    float z = render_scale * (remote_pos ? remote_pos[2] : go->pos[2]);
    
    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_mul(model, model, rot);
    mat4x4_scale_aniso(model, model, sx, sy, sz);
    //mat4x4_scale_aniso(model, model, 5, 5, 5);
    
    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, z);
    
    mat4x4_mul(model, remote ? remote_rot : go->rot, model);
    mat4x4_mul(model, model_translate, model);
    
    mat4x4 view_model;
    mat4x4_mul(view_model, view, model);
    
    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, proj, view_model);
    
    int puck_owner_player_no = pLwc->puck_game->remote ? pLwc->puck_game_state.puck_owner_player_no : pLwc->puck_game->puck_owner_player_no;
    
    const float e = 2.718f;
    const float red_overlay_ratio = go->red_overlay ? LWMIN(1.0f, speed / go->puck_game->puck_damage_contact_speed_threshold) : 0;
    const float red_overlay_logistic_ratio = LWMIN(0.3f, 1 / (1 + powf(e, -(20.0f * (red_overlay_ratio - 0.8f)))));
    if (go->red_overlay) {
        if (puck_owner_player_no == 1) {
            if (pLwc->puck_game->player_no == 2) {
                glUniform3f(shader->overlay_color_location, 1, 0, 0);
                glUniform3f(shader->multiply_color_location, 1, 0, 0);
            } else {
                glUniform3f(shader->overlay_color_location, 0, 0, 1);
                glUniform3f(shader->multiply_color_location, 0, 0, 1);
            }
        } else if (puck_owner_player_no == 2) {
            if (pLwc->puck_game->player_no == 2) {
                glUniform3f(shader->overlay_color_location, 0, 0, 1);
                glUniform3f(shader->multiply_color_location, 0, 0, 1);
            } else {
                glUniform3f(shader->overlay_color_location, 1, 0, 0);
                glUniform3f(shader->multiply_color_location, 1, 0, 0);
            }
        } else {
            glUniform3f(shader->multiply_color_location, 1, 1, 1);
        }
    } else {
        glUniform3f(shader->multiply_color_location, 1, 1, 1);
    }
    glUniform1f(shader->overlay_color_ratio_location, red_overlay_logistic_ratio);
    
    const LW_VBO_TYPE lvt = LVT_PUCK;
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
    bind_all_vertex_attrib(pLwc, lvt);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
    glUniform3f(shader->multiply_color_location, 1, 1, 1);
}

static void render_timer(const LWCONTEXT* pLwc, float remain_sec) {
    // Render text
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_TOP;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_A;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    char str[32];
    if (remain_sec < 0) {
        sprintf(str, u8"--");
    } else {
        sprintf(str, u8"%.0f", remain_sec);
    }
    text_block.text = str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = 0.0f;
    text_block.text_block_y = 1.0f - 0.05f;
    render_text_block(pLwc, &text_block);
}

static void render_match_state(const LWCONTEXT* pLwc) {
    // Render text
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_BOTTOM;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_C;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    char str[128];
    if (pLwc->puck_game_state.finished) {
        int hp_diff = pLwc->puck_game_state.player_current_hp - pLwc->puck_game_state.target_current_hp;
        if (hp_diff == 0) {
            sprintf(str, u8"DRAW (BID:%d) (TOUCH 'JUMP' TO REMATCH)", pLwc->puck_game->battle_id);
        } else if (hp_diff > 0) {
            if (pLwc->puck_game->battle_id == 0) {
                sprintf(str, "YOU WON (BID:%d) (Searching...) [PRACTICE MODE]", pLwc->puck_game->battle_id);
            } else {
                sprintf(str, "YOU WON (BID:%d) (TOUCH 'JUMP' TO REMATCH)", pLwc->puck_game->battle_id);
            }
        } else {
            if (pLwc->puck_game->battle_id == 0) {
                sprintf(str, "YOU LOSE (BID:%d) (Searching...) [PRACTICE MODE]", pLwc->puck_game->battle_id);
            } else {
                sprintf(str, "YOU LOSE (BID:%d) (TOUCH 'JUMP' TO REMATCH)", pLwc->puck_game->battle_id);
            }
        }
    } else {
        if (pLwc->puck_game->token) {
            sprintf(str, "FIGHT!!! (BID:%d)", pLwc->puck_game->battle_id);
        } else {
            sprintf(str, "Searching... [PRACTICE MODE]");
        }
    }
    
    text_block.text = str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = 0;
    text_block.text_block_y = -0.9f;
    render_text_block(pLwc, &text_block);
}

static void render_hp_gauge(const LWCONTEXT* pLwc,
                            float x, float y, int current_hp, int total_hp, float hp_shake_remain_time, int right, const char* str) {
    const float gauge_width = 1.2f;
    const float gauge_height = 0.1f;
    //const float gauge_flush_height = 0.07f;
    const float base_color = 0.1f;
    // Positioinal offset by shake
    if (hp_shake_remain_time > 0) {
        const float ratio = hp_shake_remain_time / pLwc->puck_game->hp_shake_time;
        const float shake_magnitude = 0.02f;
        x += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * pLwc->aspect_ratio;
        y += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude;
    }
    // Render background (gray)
    render_solid_vb_ui(pLwc,
                       x, y, gauge_width, gauge_height,
                       0,
                       LVT_CENTER_TOP_ANCHORED_SQUARE,
                       1, base_color, base_color, base_color, 1);
    const float cell_border = 0.015f;
    if (total_hp > 0) {
        const float cell_width = (gauge_width - cell_border * (total_hp + 1)) / total_hp;
        const float cell_x_0 = x - gauge_width / 2 + cell_border;
        const float cell_x_stride = cell_width + cell_border;
        for (int i = 0; i < total_hp; i++) {
            float r = base_color, g = base_color, b = base_color;
            if (right) {
                if (total_hp - current_hp > i) {
                    r = 1;
                } else {
                    g = 1;
                }
            } else {
                if (i < current_hp) {
                    g = 1;
                } else {
                    r = 1;
                }
            }
            
            // Render background (green)
            render_solid_vb_ui(pLwc,
                               cell_x_0 + cell_x_stride * i, y - gauge_height / 2, cell_width, gauge_height - cell_border * 2,
                               0,
                               LVT_LEFT_CENTER_ANCHORED_SQUARE,
                               1, r, g, b, 1);
        }
    }
    // Render text
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_CENTER_BOTTOM;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_E;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    text_block.text = str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x;
    text_block.text_block_y = y;
    render_text_block(pLwc, &text_block);
}

static void render_dash_gauge(const LWCONTEXT* pLwc) {
    const float margin_x = 1.5f;
    const float margin_y = 0.3f;
    const float gauge_width = 0.75f;
    const float gauge_height = 0.07f;
    //const float gauge_flush_height = 0.07f;
    const float base_color = 0.3f;
    float x = -pLwc->aspect_ratio + margin_x;
    float y = 1 - margin_y;
    const float boost_gauge_ratio = puck_game_dash_gauge_ratio(pLwc->puck_game);
    // Positioinal offset by shake
    if (pLwc->puck_game->dash.shake_remain_time > 0) {
        const float ratio = pLwc->puck_game->dash.shake_remain_time / pLwc->puck_game->dash_shake_time;
        const float shake_magnitude = 0.02f;
        x += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude * pLwc->aspect_ratio;
        y += ratio * (2 * rand() / (float)RAND_MAX - 1.0f) * shake_magnitude;
    }
    // Render background (red)
    if (boost_gauge_ratio < 1.0f) {
        render_solid_vb_ui(pLwc,
                           x - gauge_width * boost_gauge_ratio, y, gauge_width * (1.0f - boost_gauge_ratio), gauge_height,
                           0,
                           LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
                           1, 1, base_color, base_color, 1);
    }
    // Render foreground (green)
    render_solid_vb_ui(pLwc,
                       x, y, gauge_width * boost_gauge_ratio, gauge_height,
                       0,
                       LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
                       1, base_color, 1, base_color, 1);
    if (boost_gauge_ratio < 1.0f) {
        // Render flush gauge
        render_solid_vb_ui(pLwc,
                           x, y + boost_gauge_ratio / 4.0f, gauge_width, gauge_height,
                           0,
                           LVT_RIGHT_BOTTOM_ANCHORED_SQUARE,
                           powf(1.0f - boost_gauge_ratio, 3.0f), base_color, 1, base_color, 1);
    }
    // Render text
    LWTEXTBLOCK text_block;
    text_block.align = LTBA_RIGHT_BOTTOM;
    text_block.text_block_width = DEFAULT_TEXT_BLOCK_WIDTH;
    text_block.text_block_line_height = DEFAULT_TEXT_BLOCK_LINE_HEIGHT_E;
    text_block.size = DEFAULT_TEXT_BLOCK_SIZE_F;
    text_block.multiline = 1;
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_glyph, 1, 1, 1, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_normal_outline, 0, 0, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_glyph, 1, 1, 0, 1);
    SET_COLOR_RGBA_FLOAT(text_block.color_emp_outline, 0, 0, 0, 1);
    char str[32];
    sprintf(str, "%.1f%% DASH GAUGE", boost_gauge_ratio * 100);
    text_block.text = str;
    text_block.text_bytelen = (int)strlen(text_block.text);
    text_block.begin_index = 0;
    text_block.end_index = text_block.text_bytelen;
    text_block.text_block_x = x;
    text_block.text_block_y = y;
    render_text_block(pLwc, &text_block);
}

static void render_wall(const LWCONTEXT *pLwc, const mat4x4 proj, const LWPUCKGAME *puck_game,
                        int shader_index, const mat4x4 view, float x, float y, float z,
                        float x_rot, float y_rot, LW_VBO_TYPE lvt, float sx, float sy, float sz, const LWSPHERERENDERUNIFORM* sphere_render_uniform) {
    const LWSHADER* shader = &pLwc->shader[shader_index];
    glUseProgram(shader->program);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    glUniform2fv(shader->vuvscale_location, 1, default_uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, 1.0f);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, 1, 1, 1);
    glUniform1f(shader->overlay_color_ratio_location, 0);
    glUniform1fv(shader->sphere_col_ratio, 3, sphere_render_uniform->sphere_col_ratio);
    glUniform3fv(shader->sphere_pos, 3, (const float*)sphere_render_uniform->sphere_pos);
    glUniform3fv(shader->sphere_col, 3, (const float*)sphere_render_uniform->sphere_col);
    glUniform1fv(shader->sphere_speed, 3, (const float*)sphere_render_uniform->sphere_speed);
    glUniform1fv(shader->sphere_move_rad, 3, (const float*)sphere_render_uniform->sphere_move_rad);
    glUniform3fv(shader->reflect_size, 1, (const float*)sphere_render_uniform->reflect_size);
    
    const int tex_index = 0;
    mat4x4 rot_x;
    mat4x4_identity(rot_x);
    mat4x4_rotate_X(rot_x, rot_x, x_rot);
    mat4x4 rot_y;
    mat4x4_identity(rot_y);
    mat4x4_rotate_Y(rot_y, rot_y, y_rot);
    mat4x4 rot;
    mat4x4_mul(rot, rot_x, rot_y);
    
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
    
    glUniform3f(shader->overlay_color_location, 0.2f, 0.2f, 0.2f);
    glUniform1f(shader->overlay_color_ratio_location, 1.0f);
    
    //glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
    bind_all_vertex_attrib(pLwc, lvt);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_floor(const LWCONTEXT *pLwc, const mat4x4 proj, const LWPUCKGAME *puck_game, int shader_index, const mat4x4 view,
                         const LWSPHERERENDERUNIFORM* sphere_render_uniform) {
    
    const LWSHADER* shader = &pLwc->shader[shader_index];
    glUseProgram(shader->program);
    glUniform2fv(shader->vuvoffset_location, 1, default_uv_offset);
    const float uv_scale[2] = { 5, 5 };
    glUniform2fv(shader->vuvscale_location, 1, uv_scale);
    glUniform2fv(shader->vs9offset_location, 1, default_uv_offset);
    glUniform1f(shader->alpha_multiplier_location, 1.0f);
    glUniform1i(shader->diffuse_location, 0); // 0 means GL_TEXTURE0
    glUniform1i(shader->alpha_only_location, 1); // 1 means GL_TEXTURE1
    glUniform3f(shader->overlay_color_location, 1, 1, 1);
    glUniform1f(shader->overlay_color_ratio_location, 0);
    glUniform1fv(shader->sphere_col_ratio, 3, sphere_render_uniform->sphere_col_ratio);
    glUniform3fv(shader->sphere_pos, 3, (const float*)sphere_render_uniform->sphere_pos);
    glUniform3fv(shader->sphere_col, 3, (const float*)sphere_render_uniform->sphere_col);
    glUniform1fv(shader->sphere_speed, 3, (const float*)sphere_render_uniform->sphere_speed);
    glUniform1fv(shader->sphere_move_rad, 3, (const float*)sphere_render_uniform->sphere_move_rad);
    glUniformMatrix2fv(shader->arrowRotMat2, 1, 0, (const float*)sphere_render_uniform->arrowRotMat2);
    glUniform2fv(shader->arrow_center, 1, sphere_render_uniform->arrow_center);
    glUniform1f(shader->arrow_scale, sphere_render_uniform->arrow_scale);
    glUniform3fv(shader->reflect_size, 1, (const float*)sphere_render_uniform->reflect_size);
    
    const int tex_index = pLwc->tex_atlas[LAE_PUCK_FLOOR_KTX];
    const int arrow_tex_index = pLwc->tex_atlas[LAE_ARROW];
    mat4x4 rot;
    mat4x4_identity(rot);
    
    float sx = 2.0f;
    float sy = 2.0f;
    float sz = 2.0f;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    
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
    
    glUniform3f(shader->overlay_color_location, 0.5f, 0.5f, 0.5f);
    glUniform1f(shader->overlay_color_ratio_location, 0.0f);
    
    //glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    
    const LW_VBO_TYPE lvt = LVT_CENTER_CENTER_ANCHORED_SQUARE;
    
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
    bind_all_vertex_attrib(pLwc, lvt);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_index);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0);
    set_tex_filter(GL_LINEAR, GL_LINEAR);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, arrow_tex_index);
    glUniform1i(pLwc->shader[shader_index].diffuse_arrow_location, 1);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //set_tex_filter(GL_NEAREST, GL_NEAREST);
    glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glUniformMatrix4fv(shader->m_location, 1, GL_FALSE, (const GLfloat*)model);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[lvt].vertex_count);
}

static void render_lwbutton(const LWCONTEXT* pLwc, const LWBUTTONLIST* button_list) {
    for (int i = 0; i < button_list->button_count; i++) {
        float or = 1.0f;
        float og = 1.0f;
        float ob = 1.0f;
        if (i == 0 && pLwc->puck_game->pull_puck) {
            or = 0.2f;
            ob = 0.2f;
        }
        const LWBUTTON* b = &button_list->button[i];
        render_solid_vb_ui_alpha(pLwc,
                           b->x,
                           b->y,
                           b->w,
                           b->h,
                           pLwc->tex_atlas[b->lae],
                           pLwc->tex_atlas[b->lae_alpha],
                           LVT_LEFT_TOP_ANCHORED_SQUARE,
                           1.0f,
                           or,
                           og,
                           ob,
                           1.0f);
    }
}

void lwc_render_physics(const LWCONTEXT* pLwc, const mat4x4 view, const mat4x4 proj) {
    const LWPUCKGAME* puck_game = pLwc->puck_game;
    const LWPSTATE* state = &pLwc->puck_game_state;
    glViewport(0, 0, pLwc->width, pLwc->height);
    lw_clear_color();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    //int shader_index = LWST_DEFAULT;
    int wall_shader_index = LWST_SPHERE_REFLECT;
    int floor_shader_index = LWST_SPHERE_REFLECT_FLOOR;
    
    int single_play = puck_game->battle_id == 0;
    int remote = !single_play && !pLwc->udp->master;
    const float* player_pos = puck_game->go[LPGO_PLAYER].pos;
    const float* target_pos = puck_game->go[LPGO_TARGET].pos;
    const float* puck_pos = puck_game->go[LPGO_PUCK].pos;
    const float* remote_player_pos = 0;
    const float* remote_puck_pos = 0;
    const float* remote_target_pos = 0;
    if (remote) {
        remote_player_pos = state->player;
        remote_puck_pos = state->puck;
        remote_target_pos = state->target;
    } else {
        remote_player_pos = player_pos;
        remote_puck_pos = puck_pos;
        remote_target_pos = target_pos;
    }
    
    const float* player_controlled_pos = remote_player_pos;
    if (puck_game->player_no == 2) {
        player_controlled_pos = remote_target_pos;
    }
    
    float arrow_scale = 0.4f;
    float arrowAngle = (float)LWDEG2RAD(70.0f);
    float dx = 0, dy = 0, dlen = 0;
    int fire = 0;
    if (lw_get_normalized_dir_pad_input(pLwc, &pLwc->right_dir_pad, &dx, &dy, &dlen) && (dx || dy)) {
        fire = 1;
        arrowAngle = atan2f(dy, dx);
        arrow_scale = LWMIN(0.01f + dlen * 0.7f, 1.0f);
    }
    if (pLwc->puck_game->player_no != 2) {
        arrow_scale *= -1;
    }

    float puck_sphere_col[3];
    int puck_owner_player_no = puck_game->remote ? state->puck_owner_player_no : puck_game->puck_owner_player_no;
    if (puck_owner_player_no == 0) {
        puck_sphere_col[0] = 0.3f;
        puck_sphere_col[1] = 0.3f;
        puck_sphere_col[2] = 0.3f;
    } else if (puck_owner_player_no == 1) {
        if (puck_game->player_no == 2) {
            puck_sphere_col[0] = 1.0f;
            puck_sphere_col[1] = 0.1f;
            puck_sphere_col[2] = 0.2f;
        } else {
            puck_sphere_col[0] = 0.2f;
            puck_sphere_col[1] = 0.1f;
            puck_sphere_col[2] = 1.0f;
        }
    } else {
        if (puck_game->player_no == 2) {
            puck_sphere_col[0] = 0.2f;
            puck_sphere_col[1] = 0.1f;
            puck_sphere_col[2] = 1.0f;
        } else {
            puck_sphere_col[0] = 1.0f;
            puck_sphere_col[1] = 0.1f;
            puck_sphere_col[2] = 0.2f;
        }
    }
    LWSPHERERENDERUNIFORM sphere_render_uniform = {
        // float sphere_col_ratio[3];
        { 1.0f, 1.0f, 1.0f },
        // float sphere_pos[3][3];
        {
            { remote_player_pos[0], remote_player_pos[1], remote_player_pos[2] },
            { remote_target_pos[0], remote_target_pos[1], remote_target_pos[2] },
            { remote_puck_pos[0], remote_puck_pos[1], remote_puck_pos[2] }
        },
        // float sphere_col[3][3];
        {
            { 0.0f, 1.0f, 0.8f },
            { 1.0f, 0.0f, 0.0f },
            { puck_sphere_col[0], puck_sphere_col[1], puck_sphere_col[2] }
        },
        // float sphere_speed[3];
        {
            !remote ? puck_game->go[LPGO_PLAYER].speed : state->player_speed,
            !remote ? puck_game->go[LPGO_TARGET].speed : state->target_speed,
            !remote ? puck_game->go[LPGO_PUCK].speed : state->puck_speed
        },
        // float sphere_move_rad[3];
        {
            !remote ? puck_game->go[LPGO_PLAYER].move_rad : state->player_move_rad,
            !remote ? puck_game->go[LPGO_TARGET].move_rad : state->target_move_rad,
            !remote ? puck_game->go[LPGO_PUCK].move_rad : state->puck_move_rad
        },
        // float reflect_size[3];
        {
            1.0f,
            1.0f,
            remote ? state->puck_reflect_size : puck_game->puck_reflect_size,
        },
        // float arrowRotMat[2][2];
        {
            { cosf(-arrowAngle), -sinf(-arrowAngle) },
            { sinf(-arrowAngle), +cosf(-arrowAngle) },
        },
        // float arrow_center[2];
        {
            fire ? +player_controlled_pos[0] / puck_game->world_size : 999.0f,
            fire ? -player_controlled_pos[1] / puck_game->world_size : 999.0f,
        },
        // float arrow_scale;
        arrow_scale,
    };
    // Switch sphere reflection color if player number is 2.
    if (puck_game->player_no == 2) {
        sphere_render_uniform.sphere_col[0][0] = 1.0f;
        sphere_render_uniform.sphere_col[0][1] = 0.0f;
        sphere_render_uniform.sphere_col[0][2] = 0.0f;
        
        sphere_render_uniform.sphere_col[1][0] = 0.0f;
        sphere_render_uniform.sphere_col[1][1] = 1.0f;
        sphere_render_uniform.sphere_col[1][2] = 0.8f;
    }
    const float wall_height = 0.8f;
    const LWPUCKGAMEPLAYER* player = &puck_game->player;
    const LWPUCKGAMEPLAYER* target = &puck_game->target;
    // Floor
    render_floor(pLwc, proj, puck_game, floor_shader_index, view, &sphere_render_uniform);
    // North wall
    render_wall(pLwc, proj, puck_game, wall_shader_index, view, 0, 2, 0, (float)LWDEG2RAD(90), 0,
                LVT_CENTER_BOTTOM_ANCHORED_SQUARE, 2.0f, wall_height, 2.0f, &sphere_render_uniform);
    // South wall
    render_wall(pLwc, proj, puck_game, wall_shader_index, view, 0, -2, 0, (float)LWDEG2RAD(-90), 0,
                LVT_CENTER_TOP_ANCHORED_SQUARE, 2.0f, wall_height, 2.0f, &sphere_render_uniform);
    // East wall
    render_wall(pLwc, proj, puck_game, wall_shader_index, view, 2, 0, 0, 0, (float)LWDEG2RAD(90),
                LVT_LEFT_CENTER_ANCHORED_SQUARE, wall_height, 2.0f, 2.0f, &sphere_render_uniform);
    // West wall
    render_wall(pLwc, proj, puck_game, wall_shader_index, view, -2, 0, 0, 0, (float)LWDEG2RAD(-90),
                LVT_RIGHT_CENTER_ANCHORED_SQUARE, wall_height, 2.0f, 2.0f, &sphere_render_uniform);
    const int player_no = pLwc->puck_game->player_no;
    // Game object: Puck
    render_go(pLwc, view, proj, &puck_game->go[LPGO_PUCK], pLwc->tex_atlas[LAE_PUCK_GRAY_KTX],
              1.0f, remote_puck_pos, state->puck_rot, remote, !remote ? puck_game->go[LPGO_PUCK].speed : state->puck_speed);
    // Game object: Player
    render_go(pLwc, view, proj, &puck_game->go[LPGO_PLAYER], pLwc->tex_atlas[player_no == 2 ? LAE_PUCK_ENEMY_KTX : LAE_PUCK_PLAYER_KTX],
              1.0f, remote_player_pos, state->player_rot, remote, 0);
    // Game object: Enemy
    render_go(pLwc, view, proj, &puck_game->go[LPGO_TARGET], pLwc->tex_atlas[player_no == 2 ? LAE_PUCK_PLAYER_KTX : LAE_PUCK_ENEMY_KTX],
              1.0f, remote_target_pos, state->target_rot, remote, 0);
    // Towers
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        float tower_pos[] = {
            puck_game->tower_pos * puck_game->tower_pos_multiplier[i][0],
            puck_game->tower_pos * puck_game->tower_pos_multiplier[i][1],
            0
        };
        render_tower(pLwc, view, proj, tower_pos, &puck_game->tower[i]);
    }
    // Render damage texts
    render_damage_text(pLwc, view, proj, pLwc->proj);
    render_dir_pad_with_start(pLwc, &pLwc->left_dir_pad);
    if (pLwc->control_flags & LCF_PUCK_GAME_RIGHT_DIR_PAD) {
        render_dir_pad_with_start(pLwc, &pLwc->right_dir_pad);
    }
    // Dash button
    //render_fist_button(pLwc);
    // Pull button
    //render_top_button(pLwc);
    // Dash cooltime gauge
    render_dash_gauge(pLwc);
    // HP gauges (player & target)
    const char* target_nickname = puck_game->battle_id ? puck_game->target_nickname : "== Searching opponent ==";
    render_hp_gauge(pLwc, -0.8f, 1.0f - 0.1f, state->player_current_hp, state->player_total_hp, player->hp_shake_remain_time, 1, puck_game->nickname);
    render_hp_gauge(pLwc, +0.8f, 1.0f - 0.1f, state->target_current_hp, state->target_total_hp, target->hp_shake_remain_time, 0, target_nickname);
    // Battle timer (center top of the screen)
    render_timer(pLwc, puck_game_remain_time(pLwc->puck_game->total_time, state->update_tick));
    // Match state text (bottom of the screen)
    render_match_state(pLwc);
    
    // Register as a button
    const float button_size = 0.35f;
    const float button_margin_x = 0.025f;
    const float button_x_interval = button_size + button_margin_x;
    const float button_x_0 = 0.15f;
    const float button_y_interval = 0.25f;
    const float button_y_0 = -0.50f;
    if (pLwc->control_flags & LCF_PUCK_GAME_PULL) {
        lwbutton_lae_append(&(((LWCONTEXT*)pLwc)->button_list), "pull_button", button_x_interval * 1 + button_x_0, button_y_0 + button_y_interval * 0, button_size, button_size, LAE_BUTTON_PULL, LAE_BUTTON_PULL_ALPHA);
    }
    if (pLwc->control_flags & LCF_PUCK_GAME_DASH) {
        lwbutton_lae_append(&(((LWCONTEXT*)pLwc)->button_list), "dash_button", button_x_interval * 2 + button_x_0, button_y_0 + button_y_interval * 1, button_size, button_size, LAE_BUTTON_DASH, LAE_BUTTON_DASH_ALPHA);
    }
    if (pLwc->control_flags & LCF_PUCK_GAME_JUMP) {
        lwbutton_lae_append(&(((LWCONTEXT*)pLwc)->button_list), "jump_button", button_x_interval * 3 + button_x_0, button_y_0 + button_y_interval * 2, button_size, button_size, LAE_BUTTON_JUMP, LAE_BUTTON_JUMP_ALPHA);
    }
    render_lwbutton(pLwc, &pLwc->button_list);
}
