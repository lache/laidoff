#include "lwdirpad.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "linmath.h"
#include "laidoff.h"

void render_dir_pad(const LWCONTEXT* pLwc, float x, float y) {
    int shader_index = LWST_DEFAULT;
    const int vbo_index = LVT_CENTER_CENTER_ANCHORED_SQUARE;

    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_rotate_X(model, model, 0);
    mat4x4_scale_aniso(model, model, 0.05f, 0.05f, 0.05f);
    mat4x4 model_translate;
    mat4x4_translate(model_translate, x, y, 0);
    mat4x4_mul(model, model_translate, model);

    mat4x4 view_model;
    mat4x4 view; mat4x4_identity(view);
    mat4x4_mul(view_model, view, model);

    mat4x4 proj_view_model;
    mat4x4_identity(proj_view_model);
    mat4x4_mul(proj_view_model, pLwc->proj, view_model);
    glUseProgram(pLwc->shader[shader_index].program);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[vbo_index].vertex_buffer);
    bind_all_vertex_attrib(pLwc, vbo_index);
    glUniformMatrix4fv(pLwc->shader[shader_index].mvp_location, 1, GL_FALSE, (const GLfloat*)proj_view_model);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(pLwc->shader[shader_index].diffuse_location, 0); // 0 means GL_TEXTURE0
    glBindTexture(GL_TEXTURE_2D, pLwc->tex_programmed[LPT_DIR_PAD]);
    set_tex_filter(GL_LINEAR, GL_LINEAR);
    glDrawArrays(GL_TRIANGLES, 0, pLwc->vertex_buffer[vbo_index].vertex_count);
}

void render_dir_pad_with_start(const LWCONTEXT* pLwc, float x, float y, float start_x, float start_y, int dragging) {
    // Current touch position
    render_dir_pad(pLwc, x, y);
    // Touch origin position
    if (dragging) {
        render_dir_pad(pLwc, start_x, start_y);
    }
}

float get_dir_pad_size_radius() {
    return 0.75f;
}

void get_right_dir_pad_original_center(const float aspect_ratio, float *x, float *y) {
    const float sr = get_dir_pad_size_radius();
    if (aspect_ratio > 1) {
        *x = 1 * aspect_ratio - sr;
        *y = -1 + sr;
    } else {
        *x = 1 - sr;
        *y = -1 / aspect_ratio + sr;
    }
}

void get_left_dir_pad_original_center(const float aspect_ratio, float *x, float *y) {
    const float sr = get_dir_pad_size_radius();
    if (aspect_ratio > 1) {
        *x = -1 * aspect_ratio + sr;
        *y = -1 + sr;
    } else {
        *x = -1 + sr;
        *y = -1 / aspect_ratio + sr;
    }
}

int lw_get_normalized_dir_pad_input(const LWCONTEXT* pLwc, const LWDIRPAD* dir_pad, float *dx, float *dy, float *dlen) {
    if (!dir_pad->dragging) {
        return 0;
    }

    float dir_pad_center_x = 0;
    float dir_pad_center_y = 0;
    get_left_dir_pad_original_center(pLwc->aspect_ratio, &dir_pad_center_x, &dir_pad_center_y);

    *dx = dir_pad->x - dir_pad->start_x;
    *dy = dir_pad->y - dir_pad->start_y;

    *dlen = sqrtf(*dx * *dx + *dy * *dy);

    if (*dlen < LWEPSILON) {
        *dlen = 0;
        *dx = 0;
        *dy = 0;
    } else {
        *dx /= *dlen;
        *dy /= *dlen;
    }

    return 1;
}

void reset_dir_pad_position(LWDIRPAD* dir_pad, float aspect_ratio) {
    float dir_pad_center_x = 0;
    float dir_pad_center_y = 0;
    get_left_dir_pad_original_center(aspect_ratio, &dir_pad_center_x, &dir_pad_center_y);

    dir_pad->x = dir_pad_center_x;
    dir_pad->y = dir_pad_center_y;
}

int dir_pad_press(LWDIRPAD* dir_pad, float x, float y, int pointer_id,
                  float dir_pad_center_x, float dir_pad_center_y, float sr) {
    if (fabs(dir_pad_center_x - x) < sr && fabs(dir_pad_center_y - y) < sr
        && !dir_pad->dragging) {
        dir_pad->start_x = x;
        dir_pad->start_y = y;
        dir_pad->x = x;
        dir_pad->y = y;
        dir_pad->dragging = 1;
        dir_pad->pointer_id = pointer_id;
        return 0;
    }
    return -1;
}

void dir_pad_move(LWDIRPAD* dir_pad, float x, float y, int pointer_id,
                  float dir_pad_center_x, float dir_pad_center_y, float sr) {
    if (dir_pad->dragging && dir_pad->pointer_id == pointer_id) {
        if (x < dir_pad_center_x - sr) {
            x = dir_pad_center_x - sr;
        }

        if (x > dir_pad_center_x + sr) {
            x = dir_pad_center_x + sr;
        }

        if (y < dir_pad_center_y - sr) {
            y = dir_pad_center_y - sr;
        }

        if (y > dir_pad_center_y + sr) {
            y = dir_pad_center_y + sr;
        }

        dir_pad->x = x;
        dir_pad->y = y;
    }
}

void dir_pad_release(LWDIRPAD* dir_pad, int pointer_id, float aspect_ratio) {
    if (dir_pad->pointer_id == pointer_id) {
        reset_dir_pad_position(dir_pad, aspect_ratio);
        dir_pad->dragging = 0;
    }
}
