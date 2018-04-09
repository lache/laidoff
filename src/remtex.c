#include "lwgl.h"
#include "remtex.h"
#include "lwudp.h"
#include "lwlog.h"
#include "lwhostaddr.h"
#include "lwmacro.h"

unsigned long hash(const unsigned char *str);

typedef enum _LW_REM_TEX_STATE {
    LRTS_EMPTY,
    LRTS_DOWNLOADING,
    LRTS_DOWNLOADED,
    LRTS_GPU_LOADED,
} LW_REM_TEX_STATE;

typedef struct _LWREMTEXTEX {
    char name[64];
    unsigned int namehash;
    int state;
    //GLint level; // mip level
    //GLint internalformat; // GL_RGB
    GLsizei width;
    GLsizei height;
    //GLenum format; // GL_RGB
    //GLenum type; // GL_UNSIGNED_BYTE
    unsigned char* pixels;
    size_t pixels_len;
} LWREMTEXTEX;

#define MAX_TEX_COUNT (512)
typedef struct _LWREMTEX {
    int tex_count;
    LWREMTEXTEX tex[MAX_TEX_COUNT];
    LWHOSTADDR host_addr;
    LWUDP* udp;
} LWREMTEX;

typedef struct _LWREMTEXREQUEST {
    unsigned int namehash;
    unsigned int offset;
} LWREMTEXREQUEST;

typedef struct _LWREMTEXTEXPART {
    unsigned int namehash;
    unsigned short w;
    unsigned short h;
    unsigned short len;
    unsigned short padding;
    unsigned int offset;
    unsigned char data[1024];
} LWREMTEXTEXPART;

void* remtex_new() {
    LWREMTEX* remtex = calloc(1, sizeof(LWREMTEX));
    remtex->udp = new_udp();
    strcpy(remtex->host_addr.host, "localhost");
    remtex->host_addr.port = 19876;
    strcpy(remtex->host_addr.port_str, "19876");
    udp_update_addr_host(remtex->udp,
                         remtex->host_addr.host,
                         remtex->host_addr.port,
                         remtex->host_addr.port_str);
    return remtex;
}

void remtex_destroy(void** r) {
    free(*r);
    *r = 0;
}

void remtex_load(void* r, const char* name) {
    LWREMTEX* remtex = (LWREMTEX*)r;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_EMPTY) {
            // clear all
            memset(&remtex->tex[i], 0, sizeof(LWREMTEXTEX));
            remtex->tex[i].state = LRTS_DOWNLOADING;
            strcpy(remtex->tex[i].name, name);
            remtex->tex[i].namehash = hash(name);
            return;
        }
    }
    LOGEP("remtex load capacity exceeded: %s", name);
}

void remtex_update(void* r) {
    LWREMTEX* remtex = (LWREMTEX*)r;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_DOWNLOADING) {
            LWREMTEXREQUEST req;
            req.namehash = remtex->tex[i].namehash;
            req.offset = remtex->tex[i].pixels_len;
            udp_send(remtex->udp, (const char*)&req, sizeof(LWREMTEXREQUEST));
        }
    }
}

void remtex_on_receive(void* r, void* t) {
    LWREMTEX* remtex = (LWREMTEX*)r;
    LWREMTEXTEXPART* texpart = (LWREMTEXTEXPART*)t;
    for (int i = 0; i < MAX_TEX_COUNT; i++) {
        if (remtex->tex[i].state == LRTS_DOWNLOADING) {
            if (texpart->len == 0) {
                remtex->tex[i].state = LRTS_DOWNLOADED;
            }
            if (remtex->tex[i].pixels_len >= texpart->offset + texpart->len) {
                return;
            }
            remtex->tex[i].width = texpart->w;
            remtex->tex[i].height = texpart->h;
            size_t total_size = remtex->tex[i].width * remtex->tex[i].height * 3; // 3 for RGB
            if (remtex->tex[i].pixels == 0) {
                remtex->tex[i].pixels = malloc(total_size);
            }
            if (texpart->offset >= total_size) {
                abort();
            }
            memcpy(remtex->tex[i].pixels + texpart->offset, texpart->data, texpart->len);
            remtex->tex[i].pixels_len += texpart->len;
            return;
        }
    }
}

void remtex_udp_update(void* r) {
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
