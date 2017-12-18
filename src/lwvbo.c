#include "lwvbo.h"
#include "lwvbotype.h"
#include "file.h"
#include "lwcontext.h"

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
