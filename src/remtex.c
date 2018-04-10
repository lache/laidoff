#include "lwgl.h"
#include "remtex.h"
#include "lwudp.h"
#include "lwlog.h"
#include "lwhostaddr.h"
#include "lwmacro.h"
#include "ktx.h"
#include "lwtimepoint.h"

unsigned long hash(const unsigned char *str);

typedef enum _LW_REM_TEX_STATE {
    LRTS_EMPTY,
    LRTS_DOWNLOADING,
    LRTS_DOWNLOADED,
    LRTS_GPU_LOADED,
} LW_REM_TEX_STATE;

typedef struct _LWREMTEXTEX {
    char name[64];
    unsigned int name_hash;
    LW_REM_TEX_STATE state;
    //GLint level; // mip level
    //GLint internalformat; // GL_RGB
    GLsizei width;
    GLsizei height;
    //GLenum format; // GL_RGB
    //GLenum type; // GL_UNSIGNED_BYTE
    unsigned char* data;
    size_t data_size;
    size_t total_size;
    GLuint tex;
} LWREMTEXTEX;

#define MAX_TEX_COUNT (512)
typedef struct _LWREMTEX {
    int tex_count;
    LWREMTEXTEX tex[MAX_TEX_COUNT];
    LWHOSTADDR host_addr;
    LWUDP* udp;
    double last_request;
} LWREMTEX;

typedef struct _LWREMTEXREQUEST {
    unsigned int name_hash;
    unsigned int offset;
} LWREMTEXREQUEST;

typedef struct _LWREMTEXTEXPART {
    unsigned int name_hash;
    unsigned int total_size;
    unsigned int offset;
    unsigned int payload_size;
    unsigned char data[1024];
} LWREMTEXTEXPART;

void* remtex_new(const char* host) {
    LWREMTEX* remtex = calloc(1, sizeof(LWREMTEX));
    remtex->udp = new_udp();
    strcpy(remtex->host_addr.host, host);
    remtex->host_addr.port = 19876;
    strcpy(remtex->host_addr.port_str, "19876");
    udp_update_addr_host(remtex->udp,
                         remtex->host_addr.host,
                         remtex->host_addr.port,
                         remtex->host_addr.port_str);
    return remtex;
}

void remtex_destroy_render(void* r) {
    LWREMTEX* remtex = (LWREMTEX*)r;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_GPU_LOADED) {
            glDeleteTextures(1, &remtex->tex[i].tex);
        }
    }
}

void remtex_destroy(void** r) {
    LWREMTEX* remtex = *(LWREMTEX**)r;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state != LRTS_EMPTY) {
            if (remtex->tex[i].data) {
                free(remtex->tex[i].data);
            }
        }
    }
    free(*r);
    *r = 0;
}

void remtex_preload(void* r, const char* name) {
    LWREMTEX* remtex = (LWREMTEX*)r;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_EMPTY) {
            // clear all
            memset(&remtex->tex[i], 0, sizeof(LWREMTEXTEX));
            remtex->tex[i].state = LRTS_DOWNLOADING;
            strcpy(remtex->tex[i].name, name);
            remtex->tex[i].name_hash = hash(name);
            return;
        }
    }
    LOGEP("remtex load capacity exceeded: %s", name);
}

GLuint remtex_load_tex(void* r, const char* name) {
    LWREMTEX* remtex = (LWREMTEX*)r;
    unsigned int name_hash = hash(name);
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].name_hash == name_hash) {
            glBindTexture(GL_TEXTURE_2D, remtex->tex[i].tex);
            return remtex->tex[i].tex;
        }
    }
    // preload it
    remtex_preload(r, name);
    return 0;
}

void remtex_update(void* r) {
    if (!r) {
        return;
    }
    LWREMTEX* remtex = (LWREMTEX*)r;
    double now = lwtimepoint_now_seconds();
    if (now - remtex->last_request < 1.0 / 120) {
        return;
    }
    remtex->last_request = lwtimepoint_now_seconds();
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_DOWNLOADING) {
            LWREMTEXREQUEST req;
            req.name_hash = remtex->tex[i].name_hash;
            req.offset = remtex->tex[i].data_size;
            udp_send(remtex->udp, (const char*)&req, sizeof(LWREMTEXREQUEST));
        }
    }
}

void remtex_render(void* r) {
    if (!r) {
        return;
    }
    LWREMTEX* remtex = (LWREMTEX*)r;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_DOWNLOADED) {
            if (remtex->tex[i].tex == 0) {
                glGenTextures(1, &remtex->tex[i].tex);
            }
            glBindTexture(GL_TEXTURE_2D, remtex->tex[i].tex);
            load_ktx_hw_or_sw_memory(remtex->tex[i].data, &remtex->tex[i].width, &remtex->tex[i].height, remtex->tex[i].name);
            glBindTexture(GL_TEXTURE_2D, 0);
            remtex->tex[i].state = LRTS_GPU_LOADED;
            // release download buffer
            free(remtex->tex[i].data);
            remtex->tex[i].data = 0;
        }
    }
}

void remtex_on_receive(void* r, void* t) {
    LWREMTEX* remtex = (LWREMTEX*)r;
    LWREMTEXTEXPART* texpart = (LWREMTEXTEXPART*)t;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_DOWNLOADING
            && remtex->tex[i].name_hash == texpart->name_hash
            && texpart->total_size != 0) {
            // allocate memory if first chunk of part received
            if (remtex->tex[i].data == 0) {
                remtex->tex[i].data = calloc(texpart->total_size, 1);
                remtex->tex[i].total_size = texpart->total_size;
            }
            if (remtex->tex[i].total_size < texpart->offset + texpart->payload_size) {
                // just ignore it
                return;
            }
            memcpy(remtex->tex[i].data + texpart->offset, texpart->data, texpart->payload_size);
            if (texpart->offset == remtex->tex[i].data_size) {
                remtex->tex[i].data_size += texpart->payload_size;
            }
            if (remtex->tex[i].data_size == texpart->total_size) {
                remtex->tex[i].state = LRTS_DOWNLOADED;
            }
            return;
        } else if (remtex->tex[i].state == LRTS_GPU_LOADED
                   && remtex->tex[i].name_hash == texpart->name_hash
                   && texpart->offset == 0
                   && texpart->payload_size == 0
                   && texpart->total_size == 0) {
            // download again
            remtex->tex[i].state = LRTS_DOWNLOADING;
            remtex->tex[i].data_size = 0;
            return;
        }
    }
}

void remtex_udp_update(void* r) {
    if (!r) {
        return;
    }
    LWREMTEX* remtex = (LWREMTEX*)r;
    LWUDP* udp = remtex->udp;
    if (udp->reinit_next_update) {
        destroy_udp(&remtex->udp);
        remtex->udp = new_udp();
        udp = remtex->udp;
        udp_update_addr_host(udp,
                             remtex->host_addr.host,
                             remtex->host_addr.port,
                             remtex->host_addr.port_str);
        udp->reinit_next_update = 0;
    }
    if (udp->ready == 0) {
        return;
    }
    FD_ZERO(&udp->readfds);
    FD_SET(udp->s, &udp->readfds);
    int rv = 0;
    while ((rv = select(udp->s + 1, &udp->readfds, NULL, NULL, &udp->tv)) == 1) {
        if ((udp->recv_len = recvfrom(udp->s, udp->buf, LW_UDP_BUFLEN, 0, (struct sockaddr*)&udp->si_other, (socklen_t*)&udp->slen)) == SOCKET_ERROR) {
#if LW_PLATFORM_WIN32
            int wsa_error_code = WSAGetLastError();
            if (wsa_error_code == WSAECONNRESET) {
                // UDP server not ready?
                // Go back to single play mode
                //udp->master = 1;
                return;
            } else {
                LOGI("recvfrom() failed with error code : %d", wsa_error_code);
                exit(EXIT_FAILURE);
            }
#else
            // Socket recovery needed
            LOGEP("UDP socket error! Socket recovery needed...");
            udp->ready = 0;
            udp->reinit_next_update = 1;
            return;
#endif
            }

        if (udp->recv_len == sizeof(LWREMTEXTEXPART)) {
            remtex_on_receive(r, udp->buf);
        } else {
            LOGEP("Unknown size of UDP packet received. (%d bytes)", udp->recv_len);
        }
        }
    }

void remtex_loading_str(void* r, char* str, size_t max_len) {
    if (!r) {
        return;
    }
    LWREMTEX* remtex = (LWREMTEX*)r;
    int downloading_count = 0;
    int downloaded_bytes = 0;
    int total_bytes = 0;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_DOWNLOADING) {
            downloading_count++;
            downloaded_bytes += remtex->tex[i].data_size;
            total_bytes += remtex->tex[i].total_size;
        }
    }
    if (downloading_count) {
        if (total_bytes) {
            snprintf(str,
                     max_len,
                     "Queue Count: %d, Current: %d bytes, Total:%d bytes (%.1f %%)",
                     downloading_count,
                     downloaded_bytes,
                     total_bytes,
                     100.0f * downloaded_bytes / total_bytes);
        } else {
            snprintf(str,
                     max_len,
                     "Queue Count: %d, Current: %d bytes, Total: --unknown--",
                     downloading_count,
                     downloaded_bytes);
        }

    } else {
        strcpy(str, "Texture downloading completed.");
    }
}
