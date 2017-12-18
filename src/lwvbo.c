#include "lwvbo.h"
#include "lwvbotype.h"
#include "file.h"
#include "lwcontext.h"
#include <assert.h>
#include "laidoff.h"

void lw_load_vbo(LWCONTEXT* pLwc, const char* filename, LWVBO* pVbo) {
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

void lw_load_all_vbo(LWCONTEXT* pLwc) {
    for (int i = 0; i < LVT_COUNT; i++) {
        if (vbo_filename[i].filename[0] != '\0') {
            lw_load_vbo(pLwc, vbo_filename[i].filename, &pLwc->vertex_buffer[i]);
        }
    }
}

static void lw_lazy_load_vbo(LWCONTEXT* pLwc, LW_VBO_TYPE lvt) {
    LWVBO* vbo = &pLwc->vertex_buffer[lvt];
    if (vbo->vertex_buffer) {
        return;
    }
    lw_load_vbo(pLwc, vbo_filename[lvt].filename, &pLwc->vertex_buffer[lvt]);
}

void lw_setup_vao(LWCONTEXT* pLwc, int lvt) {
// Vertex Array Objects
#if LW_SUPPORT_VAO
    if (pLwc->vao_ready[lvt] == 0) {
        assert(pLwc->vao[lvt]);
        glBindVertexArray(pLwc->vao[lvt]);
        assert(pLwc->vertex_buffer[lvt].vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
        //assert(pLwc->shader[vbo_filename[lvt].shader_index].program);
        set_vertex_attrib_pointer(pLwc, vbo_filename[lvt].shader_index);
        pLwc->vao_ready[lvt] = 1;
    }
#endif
}

void lazy_glBindBuffer(const LWCONTEXT* _pLwc, int lvt) {
    LWCONTEXT* pLwc = (LWCONTEXT*)_pLwc;
    lw_lazy_load_vbo(pLwc, lvt);
    lw_setup_vao(pLwc, lvt);
    glBindBuffer(GL_ARRAY_BUFFER, pLwc->vertex_buffer[lvt].vertex_buffer);
}
