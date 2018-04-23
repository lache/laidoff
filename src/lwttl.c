#include "lwttl.h"
#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include "lwmacro.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "laidoff.h"
#include "render_solid.h"
#include "lwudp.h"
#include "lwlnglat.h"
#include "lwlog.h"
#include "lwmutex.h"

typedef struct _LWTTLDATA_SEAPORT {
    char locode[8];
    char name[64];
    float lat;
    float lng;
} LWTTLDATA_SEAPORT;

typedef struct _LWTTL {
    LWTTLDATA_SEAPORT* seaport;
    size_t seaport_len;
    LWTTLWORLDMAP worldmap;
    int track_object_id;
    int track_object_ship_id;
    char seaarea[128]; // should match with LWPTTLSEAAREA.name size
    int view_scale;
    int xc0;
    int yc0;
    LWMUTEX rendering_mutex;
} LWTTL;

void* lwttl_new(float aspect_ratio) {
    LWTTL* ttl = (LWTTL*)calloc(1, sizeof(LWTTL));
    ttl->worldmap.render_org_x = 0;
    ttl->worldmap.render_org_y = -(2.0f - aspect_ratio) / 2;
    // Ulsan
    ttl->worldmap.center.lng = 129.436f;
    ttl->worldmap.center.lat = 35.494f;
    ttl->worldmap.zoom_scale = 5.0f;
    size_t seaports_dat_size;
    ttl->seaport = (LWTTLDATA_SEAPORT*)create_binary_from_file(ASSETS_BASE_PATH "ttldata" PATH_SEPARATOR "seaports.dat", &seaports_dat_size);
    ttl->seaport_len = seaports_dat_size / sizeof(LWTTLDATA_SEAPORT);
    ttl->view_scale = 64;
    LWMUTEX_INIT(ttl->rendering_mutex);
    return ttl;
}

void lwttl_destroy(void** __ttl) {
    LWTTL* ttl = *(LWTTL**)__ttl;
    LWMUTEX_DESTROY(ttl->rendering_mutex);
    free(*__ttl);
    *__ttl = 0;
}

void lwttl_render_all_seaports(const LWCONTEXT* pLwc, const void* _ttl, const LWTTLWORLDMAP* worldmap) {
    const LWTTL* ttl = (const LWTTL*)_ttl;
    for (size_t i = 0; i < ttl->seaport_len; i++) {
        const LWTTLDATA_SEAPORT* sp = ttl->seaport + i;
        float x = lnglat_to_xy(pLwc, sp->lng - worldmap->center.lng) * worldmap->zoom_scale;
        float y = lnglat_to_xy(pLwc, sp->lat - worldmap->center.lat) * worldmap->zoom_scale;
        lazy_tex_atlas_glBindTexture(pLwc, LAE_STOP_MARK);
        render_solid_vb_ui_uv_shader_rot(pLwc,
                                         worldmap->render_org_x + x,
                                         worldmap->render_org_y + y,
                                         0.01f,
                                         0.01f,
                                         pLwc->tex_atlas[LAE_STOP_MARK],
                                         LVT_CENTER_CENTER_ANCHORED_SQUARE,
                                         1.0f,
                                         1.0f,
                                         0.0f,
                                         0.0f,
                                         1.0f,
                                         default_uv_offset,
                                         default_uv_scale,
                                         LWST_DEFAULT,
                                         0);
    }
}

float lnglat_to_xy(const LWCONTEXT* pLwc, float v) {
    return v * 1.0f / 180.0f * pLwc->aspect_ratio;
}

void lwttl_worldmap_scroll(void* _ttl, float dlng, float dlat, float dzoom) {
    LWTTL* ttl = (LWTTL*)_ttl;
    ttl->worldmap.zoom_scale = LWCLAMP(ttl->worldmap.zoom_scale + dzoom, 1.0f, 25.0f);
    if (ttl->worldmap.zoom_scale <= 1.0f) {
        ttl->worldmap.center.lat = 0;
    } else {
        ttl->worldmap.center.lat = fmodf(ttl->worldmap.center.lat + dlat / ttl->worldmap.zoom_scale, 180.0f);
    }
    ttl->worldmap.center.lng = fmodf(ttl->worldmap.center.lng + dlng / ttl->worldmap.zoom_scale, 360.0f);
}

void lwttl_worldmap_scroll_to(void* _ttl, float lng, float lat, LWUDP* udp_sea) {
    LWTTL* ttl = (LWTTL*)_ttl;
    ttl->worldmap.center.lat = lat;
    ttl->worldmap.center.lng = lng;
    if (udp_sea) {
        lwttl_udp_send_ttlping(ttl, udp_sea, 0);
    }
}

const LWTTLWORLDMAP* lwttl_worldmap(void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return &ttl->worldmap;
}

void lwttl_update_aspect_ratio(void* _ttl, float aspect_ratio) {
    LWTTL* ttl = (LWTTL*)_ttl;
    ttl->worldmap.render_org_x = 0;
    ttl->worldmap.render_org_y = -(2.0f - aspect_ratio) / 2;
}

const LWTTLLNGLAT* lwttl_center(const void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return &ttl->worldmap.center;
}

void lwttl_set_center(void* _ttl, float lng, float lat) {
    LWTTL* ttl = (LWTTL*)_ttl;
    ttl->worldmap.center.lng = lng;
    ttl->worldmap.center.lat = lat;
}

void lwttl_set_seaarea(void* _ttl, const char* name) {
    LWTTL* ttl = (LWTTL*)_ttl;
    strncpy(ttl->seaarea, name, ARRAY_SIZE(ttl->seaarea) - 1);
    ttl->seaarea[ARRAY_SIZE(ttl->seaarea) - 1] = 0;
}

const char* lwttl_seaarea(void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return ttl->seaarea;
}

void lwttl_update(LWCONTEXT* pLwc, void* _ttl, float delta_time) {
    LWTTL* ttl = (LWTTL*)_ttl;
    float dx = 0, dy = 0, dlen = 0;
    if (lw_get_normalized_dir_pad_input(pLwc, &pLwc->left_dir_pad, &dx, &dy, &dlen) && (dx || dy)) {
        ttl->worldmap.center.lng += dx / 50.0f * delta_time * ttl->view_scale;
        ttl->worldmap.center.lat += dy / 50.0f * delta_time * ttl->view_scale;
    }
}

int lwttl_lng_to_int(float lng) {
    return (int)(LNGLAT_RES_WIDTH / 2 + (lng / 180.0f) * LNGLAT_RES_WIDTH / 2);
}

int lwttl_lat_to_int(float lat) {
    return (int)(LNGLAT_RES_HEIGHT / 2 - (lat / 90.0f) * LNGLAT_RES_HEIGHT / 2);
}

const char* lwttl_http_header(const void* _ttl) {
    static char http_header[1024];
    LWTTL* ttl = (LWTTL*)_ttl;
    const LWTTLLNGLAT* lnglat = lwttl_center(ttl);

    sprintf(http_header, "X-Lng: %d\r\nX-Lat: %d\r\n",
            lwttl_lng_to_int(lnglat->lng),
            lwttl_lat_to_int(lnglat->lat));
    return http_header;
}

int lwttl_track_object_id(const void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return ttl->track_object_id;
}

void lwttl_set_track_object_id(const void* _ttl, int v) {
    LWTTL* ttl = (LWTTL*)_ttl;
    ttl->track_object_id = v;
}

int lwttl_track_object_ship_id(const void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return ttl->track_object_ship_id;
}

void lwttl_set_track_object_ship_id(const void* _ttl, int v) {
    LWTTL* ttl = (LWTTL*)_ttl;
    ttl->track_object_ship_id = v;
}

void lwttl_set_view_scale(const void* _ttl, int v) {
    LWTTL* ttl = (LWTTL*)_ttl;
    LOGIx("ttl->view_scale %d -> %d", ttl->view_scale, v);
    ttl->view_scale = v;
}

int lwttl_view_scale(const void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return ttl->view_scale;
}

void lwttl_udp_send_ttlping(const void* ttl, LWUDP* udp, int ping_seq) {
    LWPTTLPING ttl_ping;
    memset(&ttl_ping, 0, sizeof(LWPTTLPING));
    const LWTTLLNGLAT* center = lwttl_center(ttl);
    ttl_ping.type = LPGP_LWPTTLPING;
    ttl_ping.lng = center->lng;
    ttl_ping.lat = center->lat;
    ttl_ping.ex = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS;
    ttl_ping.ping_seq = ping_seq;
    ttl_ping.track_object_id = lwttl_track_object_id(ttl);
    ttl_ping.track_object_ship_id = lwttl_track_object_ship_id(ttl);
    ttl_ping.view_scale = lwttl_view_scale(ttl);
    udp_send(udp, (const char*)&ttl_ping, sizeof(LWPTTLPING));
}

void lwttl_set_xc0(void* _ttl, int v) {
    LWTTL* ttl = (LWTTL*)_ttl;
    ttl->xc0 = v;
}

int lwttl_xc0(const void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return ttl->xc0;
}

void lwttl_set_yc0(void* _ttl, int v) {
    LWTTL* ttl = (LWTTL*)_ttl;
    ttl->yc0 = v;
}

int lwttl_yc0(const void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return ttl->yc0;
}

void lwttl_lock_rendering_mutex(void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    LWMUTEX_LOCK(ttl->rendering_mutex);
}

void lwttl_unlock_rendering_mutex(void* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    LWMUTEX_UNLOCK(ttl->rendering_mutex);
}
