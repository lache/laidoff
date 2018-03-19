#include "lwttl.h"
#include <stdio.h>
#include <stdlib.h>
#include "file.h"
#include "lwmacro.h"
#include "lwgl.h"
#include "lwcontext.h"
#include "laidoff.h"
#include "render_solid.h"

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
} LWTTL;

void* lwttl_new(float aspect_ratio) {
    LWTTL* ttl = (LWTTL*)malloc(sizeof(LWTTL));
    ttl->worldmap.render_org_x = 0;
    ttl->worldmap.render_org_y = -(2.0f - aspect_ratio) / 2;
    ttl->worldmap.center_lat = 35.0f;
    ttl->worldmap.center_lng = 110.0f;
    ttl->worldmap.zoom_scale = 5.0f;
    size_t seaports_dat_size;
    ttl->seaport = (LWTTLDATA_SEAPORT*)create_binary_from_file(ASSETS_BASE_PATH "ttldata" PATH_SEPARATOR "seaports.dat", &seaports_dat_size);
    ttl->seaport_len = seaports_dat_size / sizeof(LWTTLDATA_SEAPORT);
    return ttl;
}

void lwttl_destroy(void** __ttl) {
    LWTTL* ttl = *(LWTTL**)__ttl;
    free(*__ttl);
    *__ttl = 0;
}

void lwttl_render_all_seaports(const LWCONTEXT* pLwc, const void* _ttl, const LWTTLWORLDMAP* worldmap) {
    const LWTTL* ttl = (const LWTTL*)_ttl;
    for (size_t i = 0; i < ttl->seaport_len; i++) {
        const LWTTLDATA_SEAPORT* sp = ttl->seaport + i;
        float x = lnglat_to_xy(pLwc, sp->lng - worldmap->center_lng) * worldmap->zoom_scale;
        float y = lnglat_to_xy(pLwc, sp->lat - worldmap->center_lat) * worldmap->zoom_scale;
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
        ttl->worldmap.center_lat = 0;
    } else {
        ttl->worldmap.center_lat = fmodf(ttl->worldmap.center_lat + dlat / ttl->worldmap.zoom_scale, 180.0f);
    }
    ttl->worldmap.center_lng = fmodf(ttl->worldmap.center_lng + dlng / ttl->worldmap.zoom_scale, 360.0f);
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
