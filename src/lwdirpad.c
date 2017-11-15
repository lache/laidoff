#include "lwdirpad.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "linmath.h"
#include "laidoff.h"

void render_dir_pad(const LWCONTEXT* pLwc, float x, float y) {
    int shader_index = LWST_DEFAULT;
    const int vbo_index = LVT_CENTER_CENTER_ANCHORED_SQUARE;

    //float aspect_ratio = (float)pLwc->width / pLwc->height;
    //int dir_pad_size_pixel = pLwc->width < pLwc->height ? pLwc->width : pLwc->height;

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
