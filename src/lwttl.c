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
#include "lz4.h"
#include "lwttl.h"
#include "htmlui.h"
#include "lwlnglat.h"
#include "script.h"

#ifdef __GNUC__
int __builtin_ctz(unsigned int x);
int msb_index(unsigned int v) {
    return __builtin_ctz(v);
}
#else
// MSVC perhaps...
#include <intrin.h> 
#pragma intrinsic(_BitScanReverse)
int msb_index(unsigned int v) {
    unsigned long view_scale_msb_index = 0;
    _BitScanReverse(&view_scale_msb_index, (unsigned long)v);
    return (int)view_scale_msb_index;
}
#endif

typedef struct _LWTTLDATA_SEAPORT {
    char locode[8];
    char name[64];
    float lat;
    float lng;
} LWTTLDATA_SEAPORT;

typedef union _LWTTLSTATICOBJECT2_CHUNK_KEY {
    int v;
    struct {
        unsigned int xcc0 : 14; // right shifted xc0  200,000 pixels / chunk_size
        unsigned int ycc0 : 14; // right shifted yc0
        unsigned int view_scale_msb : 4; // 2^(view_scale_msb) == view_scale
    } bf;
} LWTTLSTATICOBJECT2_CHUNK_KEY;

typedef struct _LWPTTLSTATICOBJECT2_CHUNK_VALUE {
    int start;
    int count;
} LWPTTLSTATICOBJECT2_CHUNK_VALUE;

typedef struct _LWTTLCELLBOUND {
    int xc0;
    int yc0;
    int xc1;
    int yc1;
} LWTTLCELLBOUND;

typedef struct _LWTTLCHUNKBOUND {
    int xcc0;
    int ycc0;
    int xcc1;
    int ycc1;
} LWTTLCHUNKBOUND;

#define TTL_STATIC_OBJECT_CHUNK_COUNT (256*1024)
#define TTL_STATIC_OBJECT_COUNT (32*TTL_STATIC_OBJECT_CHUNK_COUNT)

typedef struct _LWTTLSTATICOBJECTCACHE {
    LWTTLSTATICOBJECT2_CHUNK_KEY static_object_chunk_key[TTL_STATIC_OBJECT_CHUNK_COUNT];
    LWPTTLSTATICOBJECT2_CHUNK_VALUE static_object_chunk_value[TTL_STATIC_OBJECT_CHUNK_COUNT];
    int static_object_chunk_count;

    LWPTTLSTATICOBJECT2 static_object[TTL_STATIC_OBJECT_COUNT];
    int static_object_count;
} LWTTLSTATICOBJECTCACHE;

typedef struct _LWTTLSAVEDATA {
    int magic;
    int version;
    float lng;
    float lat;
    int view_scale;
} LWTTLSAVEDATA;

typedef struct _LWTTL {
    int version;
    LWTTLDATA_SEAPORT* seaport;
    size_t seaport_len;
    LWTTLWORLDMAP worldmap;
    int track_object_id;
    int track_object_ship_id;
    char seaarea[128]; // should match with LWPTTLSEAAREA.name size
    int view_scale;
    int view_scale_render_max;
    int xc0;
    int yc0;
    LWMUTEX rendering_mutex;
    LWUDP* sea_udp;
    LWPTTLWAYPOINTS waypoints;
    // packet cache
    LWPTTLFULLSTATE ttl_full_state;
    LWPTTLSEAPORTSTATE ttl_seaport_state;
    LWTTLSTATICOBJECTCACHE static_object_cache;
    float earth_globe_scale;
    float earth_globe_scale_0;
} LWTTL;

LWTTL* lwttl_new(float aspect_ratio) {
    LWTTL* ttl = (LWTTL*)calloc(1, sizeof(LWTTL));
    ttl->version = 1;
    ttl->worldmap.render_org_x = 0;
    ttl->worldmap.render_org_y = -(2.0f - aspect_ratio) / 2;
    // Ulsan
    ttl->worldmap.center.lng = 129.436f;
    ttl->worldmap.center.lat = 35.494f;
    ttl->worldmap.zoom_scale = 5.0f;
    size_t seaports_dat_size;
    ttl->seaport = (LWTTLDATA_SEAPORT*)create_binary_from_file(ASSETS_BASE_PATH "ttldata" PATH_SEPARATOR "seaports.dat", &seaports_dat_size);
    ttl->seaport_len = seaports_dat_size / sizeof(LWTTLDATA_SEAPORT);
    ttl->view_scale_render_max = 64;
    ttl->view_scale = ttl->view_scale_render_max;
    LWMUTEX_INIT(ttl->rendering_mutex);
    ttl->earth_globe_scale_0 = 45.0f * 4;
    lwttl_set_earth_globe_scale(ttl, ttl->earth_globe_scale_0);
    return ttl;
}

void lwttl_destroy(LWTTL** __ttl) {
    LWTTL* ttl = *(LWTTL**)__ttl;
    LWMUTEX_DESTROY(ttl->rendering_mutex);
    free(*__ttl);
    *__ttl = 0;
}

void lwttl_render_all_seaports(const LWCONTEXT* pLwc, const LWTTL* _ttl, const LWTTLWORLDMAP* worldmap) {
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

void lwttl_worldmap_scroll(LWTTL* ttl, float dlng, float dlat, float dzoom) {
    ttl->worldmap.zoom_scale = LWCLAMP(ttl->worldmap.zoom_scale + dzoom, 1.0f, 25.0f);
    if (ttl->worldmap.zoom_scale <= 1.0f) {
        ttl->worldmap.center.lat = 0;
    } else {
        ttl->worldmap.center.lat = fmodf(ttl->worldmap.center.lat + dlat / ttl->worldmap.zoom_scale, 180.0f);
    }
    ttl->worldmap.center.lng = fmodf(ttl->worldmap.center.lng + dlng / ttl->worldmap.zoom_scale, 360.0f);
}

void lwttl_worldmap_scroll_to(LWTTL* ttl, float lng, float lat, LWUDP* sea_udp) {
    // cancel tracking if user want to scroll around
    lwttl_set_track_object_ship_id(ttl, 0);
    ttl->worldmap.center.lat = lat;
    ttl->worldmap.center.lng = lng;
    if (sea_udp) {
        lwttl_udp_send_ttlping(ttl, sea_udp, 0);
    }
}

void lwttl_worldmap_scroll_to_int(LWTTL* ttl, int xc, int yc, LWUDP* sea_udp) {
    lwttl_worldmap_scroll_to(ttl,
                             cell_x_to_lng(xc),
                             cell_y_to_lat(yc),
                             sea_udp);
}

const LWTTLWORLDMAP* lwttl_worldmap(LWTTL* ttl) {
    return &ttl->worldmap;
}

void lwttl_update_aspect_ratio(LWTTL* ttl, float aspect_ratio) {
    ttl->worldmap.render_org_x = 0;
    ttl->worldmap.render_org_y = -(2.0f - aspect_ratio) / 2;
}

const LWTTLLNGLAT* lwttl_center(const LWTTL* ttl) {
    return &ttl->worldmap.center;
}

void lwttl_set_center(LWTTL* ttl, float lng, float lat) {
    ttl->worldmap.center.lng = lng;
    ttl->worldmap.center.lat = lat;
}

void lwttl_set_seaarea(LWTTL* ttl, const char* name) {
    strncpy(ttl->seaarea, name, ARRAY_SIZE(ttl->seaarea) - 1);
    ttl->seaarea[ARRAY_SIZE(ttl->seaarea) - 1] = 0;
}

const char* lwttl_seaarea(LWTTL* ttl) {
    return ttl->seaarea;
}

void lwttl_update(LWTTL* ttl, LWCONTEXT* pLwc, float delta_time) {
    for (int i = 0; i < ttl->ttl_full_state.count; i++) {
        ttl->ttl_full_state.obj[i].fx0 += (float)delta_time * ttl->ttl_full_state.obj[i].fvx;
        ttl->ttl_full_state.obj[i].fy0 += (float)delta_time * ttl->ttl_full_state.obj[i].fvy;
        ttl->ttl_full_state.obj[i].fx1 += (float)delta_time * ttl->ttl_full_state.obj[i].fvx;
        ttl->ttl_full_state.obj[i].fy1 += (float)delta_time * ttl->ttl_full_state.obj[i].fvy;
    }

    if (ttl->sea_udp) {
        lwttl_udp_update(ttl, ttl->sea_udp, pLwc);
    }

    float dx = 0, dy = 0, dlen = 0;
    if (lw_get_normalized_dir_pad_input(pLwc, &pLwc->left_dir_pad, &dx, &dy, &dlen) && (dx || dy)) {
        // cancel tracking if user want to scroll around
        lwttl_set_track_object_ship_id(ttl, 0);
        ttl->worldmap.center.lng += dx / 50.0f * delta_time * ttl->view_scale;
        ttl->worldmap.center.lat += dy / 50.0f * delta_time * ttl->view_scale;
    }
}

static float lwttl_lng_to_int_float(float lng) {
    return LNGLAT_RES_WIDTH / 2 + (lng / 180.0f) * LNGLAT_RES_WIDTH / 2;
}

static int lwttl_lng_to_round_int(float lng) {
    return (int)(roundf(lwttl_lng_to_int_float(lng)));
}

int lwttl_lng_to_floor_int(float lng) {
    return (int)(floorf(lwttl_lng_to_int_float(lng)));
}

int lwttl_lng_to_ceil_int(float lng) {
    return (int)(ceilf(lwttl_lng_to_int_float(lng)));
}

static float lwttl_lat_to_int_float(float lat) {
    return LNGLAT_RES_HEIGHT / 2 - (lat / 90.0f) * LNGLAT_RES_HEIGHT / 2;
}

static int lwttl_lat_to_round_int(float lat) {
    return (int)(roundf(lwttl_lat_to_int_float(lat)));
}

int lwttl_lat_to_floor_int(float lat) {
    return (int)(floorf(lwttl_lat_to_int_float(lat)));
}

int lwttl_lat_to_ceil_int(float lat) {
    return (int)(ceilf(lwttl_lat_to_int_float(lat)));
}

const char* lwttl_http_header(const LWTTL* _ttl) {
    static char http_header[2048];
    LWTTL* ttl = (LWTTL*)_ttl;
    const LWTTLLNGLAT* lnglat = lwttl_center(ttl);
    snprintf(http_header,
             ARRAY_SIZE(http_header),
             "X-Lng: %d\r\nX-Lat: %d\r\n",
             lwttl_lng_to_floor_int(lnglat->lng),
             lwttl_lat_to_floor_int(lnglat->lat));
    script_http_header(script_context()->L,
                       http_header + strlen(http_header),
                       ARRAY_SIZE(http_header) - strlen(http_header));
    return http_header;
}

int lwttl_track_object_id(const LWTTL* ttl) {
    return ttl->track_object_id;
}

void lwttl_set_track_object_id(LWTTL* ttl, int v) {
    ttl->track_object_id = v;
}

int lwttl_track_object_ship_id(const LWTTL* ttl) {
    return ttl->track_object_ship_id;
}

void lwttl_set_track_object_ship_id(LWTTL* ttl, int v) {
    ttl->track_object_ship_id = v;
}

void lwttl_request_waypoints(const LWTTL* ttl, int v) {
    lwttl_udp_send_request_waypoints(ttl, ttl->sea_udp, v);
}

void lwttl_set_view_scale(LWTTL* ttl, int v) {
    if (ttl->view_scale != v) {
        LOGI("ttl->view_scale %d -> %d", ttl->view_scale, v);
        ttl->view_scale = v;
    }
}

int lwttl_view_scale(const LWTTL* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    return ttl->view_scale;
}

int lwttl_clamped_view_scale(const LWTTL* _ttl) {
    LWTTL* ttl = (LWTTL*)_ttl;
    // max value of view_scale bounded by LWTTLSTATICOBJECT2_CHUNK_KEY
    return LWCLAMP(ttl->view_scale, 1, ttl->view_scale_render_max);
}

static int lower_bound_int(const int* a, int len, int v) {
    if (len <= 0) {
        return -1;
    }
    // Lower then the first element
    int beg_value = a[0];
    if (v < beg_value) {
        return -1;
    }
    // only one element
    if (len == 1) {
        return 0;
    }
    // Higher than the last element
    int end_value = a[len - 1];
    if (v >= end_value) {
        return len - 1;
    }
    int beg = 0;
    int end = len - 1;
    while (end - beg > 1) {
        int mid = (beg + end) / 2;
        int mid_value = a[mid];
        if (mid_value < v) {
            beg = mid;
        } else if (v < mid_value) {
            end = mid;
        } else {
            return mid;
        }
    }
    return beg;
}

static int find_static_object_chunk_index(const LWTTLSTATICOBJECTCACHE* c, const LWTTLSTATICOBJECT2_CHUNK_KEY chunk_key) {
    int chunk_index = lower_bound_int(&c->static_object_chunk_key[0].v, c->static_object_chunk_count, chunk_key.v);
    if (chunk_index >= 0 && c->static_object_chunk_key[chunk_index].v == chunk_key.v) {
        return chunk_index;
    }
    return -1;
}

static void add_to_static_object_cache(LWTTLSTATICOBJECTCACHE* c, const LWPTTLSTATICSTATE2* s2) {
    if (c == 0) {
        LOGEP("c == 0");
        abort();
        return;
    }
    if (s2 == 0) {
        LOGEP("s2 == 0");
        abort();
        return;
    }
    // check for existing chunk entry
    LWTTLSTATICOBJECT2_CHUNK_KEY chunk_key;
    chunk_key.bf.xcc0 = s2->xc0 >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * s2->view_scale);
    chunk_key.bf.ycc0 = s2->yc0 >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * s2->view_scale);
    chunk_key.bf.view_scale_msb = msb_index(s2->view_scale);
    int chunk_index = lower_bound_int(&c->static_object_chunk_key[0].v, c->static_object_chunk_count, chunk_key.v);
    if (chunk_index >= 0 && c->static_object_chunk_key[chunk_index].v == chunk_key.v) {
        // already exist. do nothing
        // TODO update?
    } else {
        // new chunk entry received.
        // check current capacity
        if (c->static_object_chunk_count >= TTL_STATIC_OBJECT_CHUNK_COUNT) {
            LOGEP("static_object_chunk_count exceeded.");
            return;
        }
        if (c->static_object_count + s2->count > TTL_STATIC_OBJECT_COUNT) {
            LOGEP("static_object_count exceeded.");
            return;
        }
        // safe to add a new chunk at 'chunk_index'
        // move chunk (move by 1-index by copying in backward direction)
        for (int from = c->static_object_chunk_count - 1; from >= chunk_index + 1; from--) {
            c->static_object_chunk_key[from + 1] = c->static_object_chunk_key[from];
            c->static_object_chunk_value[from + 1] = c->static_object_chunk_value[from];
        }
        c->static_object_chunk_key[chunk_index + 1] = chunk_key;
        c->static_object_chunk_value[chunk_index + 1].start = c->static_object_count;
        c->static_object_chunk_value[chunk_index + 1].count = s2->count;
        // copy static_object data
        memcpy(&c->static_object[c->static_object_count], s2->obj, sizeof(LWPTTLSTATICOBJECT2) * s2->count);
        // increase count indices
        c->static_object_chunk_count++;
        c->static_object_count += s2->count;
    }
}

static int aligned_chunk_index(const int cell_index, const int view_scale) {
    const int half_cell_pixel_extent = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2 * view_scale;
    return (cell_index + half_cell_pixel_extent) & ~(2 * half_cell_pixel_extent - 1) & ~(view_scale - 1);
}

static void cell_bound_to_chunk_bound(const LWTTLCELLBOUND* cell_bound, const int view_scale, LWTTLCHUNKBOUND* chunk_bound) {
    const int half_cell_pixel_extent = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS / 2 * view_scale;
    chunk_bound->xcc0 = aligned_chunk_index(cell_bound->xc0, view_scale) >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_bound->ycc0 = aligned_chunk_index(cell_bound->yc0, view_scale) >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_bound->xcc1 = aligned_chunk_index(cell_bound->xc1, view_scale) >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_bound->ycc1 = aligned_chunk_index(cell_bound->yc1, view_scale) >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
}

static void send_ttlping(const LWTTL* ttl,
                         LWUDP* udp,
                         const float lng,
                         const float lat,
                         const int ping_seq,
                         const int view_scale,
                         const int static_object,
                         const float ex_lng,
                         const float ex_lat) {
    LWPTTLPING p;
    memset(&p, 0, sizeof(LWPTTLPING));
    p.type = LPGP_LWPTTLPING;
    p.lng = lng;
    p.lat = lat;
    p.ex_lng = ex_lng;
    p.ex_lat = ex_lat;
    p.ping_seq = ping_seq;
    p.track_object_id = lwttl_track_object_id(ttl);
    p.track_object_ship_id = lwttl_track_object_ship_id(ttl);
    p.view_scale = view_scale;
    p.static_object = static_object;
    udp_send(udp, (const char*)&p, sizeof(LWPTTLPING));
}

float lwttl_half_lng_extent_in_degrees(const int view_scale) {
    return LNGLAT_SEA_PING_EXTENT_IN_DEGREES / 2 * view_scale * LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG;
}

float lwttl_half_lat_extent_in_degrees(const int view_scale) {
    return LNGLAT_SEA_PING_EXTENT_IN_DEGREES / 2 * view_scale * LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT;
}

void lwttl_udp_send_ttlping(const LWTTL* ttl, LWUDP* udp, int ping_seq) {
    const LWTTLLNGLAT* center = lwttl_center(ttl);
    const int clamped_view_scale = lwttl_clamped_view_scale(ttl);

    const float half_lng_extent_in_deg = lwttl_half_lng_extent_in_degrees(clamped_view_scale);
    const float half_lat_extent_in_deg = lwttl_half_lat_extent_in_degrees(clamped_view_scale);
    const float lng_min = center->lng - half_lng_extent_in_deg;
    const float lng_max = center->lng + half_lng_extent_in_deg;
    const float lat_min = center->lat - half_lat_extent_in_deg;
    const float lat_max = center->lat + half_lat_extent_in_deg;

    LWTTLCELLBOUND cell_bound = {
        lwttl_lng_to_floor_int(lng_min),
        lwttl_lat_to_floor_int(lat_max),
        lwttl_lng_to_ceil_int(lng_max),
        lwttl_lat_to_ceil_int(lat_min),
    };
    LWTTLCHUNKBOUND chunk_bound;
    cell_bound_to_chunk_bound(&cell_bound, clamped_view_scale, &chunk_bound);
    chunk_bound.xcc0--;
    chunk_bound.xcc1++;
    chunk_bound.ycc0--;
    chunk_bound.ycc1++;
    const int xc_count = chunk_bound.xcc1 - chunk_bound.xcc0 + 1;
    const int yc_count = chunk_bound.ycc1 - chunk_bound.ycc0 + 1;
    int chunk_index_count = 0;
    for (int i = 0; i < xc_count; i++) {
        for (int j = 0; j < yc_count; j++) {
            const int xcc0 = chunk_bound.xcc0 + i;
            const int ycc0 = chunk_bound.ycc0 + j;
            LWTTLSTATICOBJECT2_CHUNK_KEY chunk_key;
            chunk_key.bf.xcc0 = xcc0;
            chunk_key.bf.ycc0 = ycc0;
            chunk_key.bf.view_scale_msb = msb_index(clamped_view_scale);
            if (find_static_object_chunk_index(&ttl->static_object_cache, chunk_key) == -1) {
                const int xc0 = xcc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
                const int yc0 = ycc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * clamped_view_scale);
                // ping for static object (land mass)
                send_ttlping(ttl,
                             udp,
                             cell_x_to_lng(xc0),
                             cell_y_to_lat(yc0),
                             ping_seq,
                             clamped_view_scale,
                             1,
                             LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS,
                             LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS);
            }
        }
    }
    // ping for dynamic object (ships, seaports)
    const int xc0 = lwttl_lng_to_round_int(center->lng) & ~(clamped_view_scale - 1);
    const int yc0 = lwttl_lat_to_round_int(center->lat) & ~(clamped_view_scale - 1);
    send_ttlping(ttl,
                 udp,
                 cell_x_to_lng(xc0),
                 cell_y_to_lat(yc0),
                 ping_seq,
                 clamped_view_scale,
                 0,
                 LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG,
                 LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT);
}

void lwttl_udp_send_request_waypoints(const LWTTL* ttl, LWUDP* sea_udp, int ship_id) {
    LWPTTLREQUESTWAYPOINTS p;
    memset(&p, 0, sizeof(LWPTTLREQUESTWAYPOINTS));
    p.type = LPGP_LWPTTLREQUESTWAYPOINTS;
    p.ship_id = ship_id;
    udp_send(sea_udp, (const char*)&p, sizeof(LWPTTLREQUESTWAYPOINTS));
}

void lwttl_set_xc0(LWTTL* ttl, int v) {
    ttl->xc0 = v;
}

int lwttl_xc0(const LWTTL* ttl) {
    return ttl->xc0;
}

void lwttl_set_yc0(LWTTL* ttl, int v) {
    ttl->yc0 = v;
}

int lwttl_yc0(const LWTTL* ttl) {
    return ttl->yc0;
}

void lwttl_lock_rendering_mutex(LWTTL* ttl) {
    LWMUTEX_LOCK(ttl->rendering_mutex);
}

void lwttl_unlock_rendering_mutex(LWTTL* ttl) {
    LWMUTEX_UNLOCK(ttl->rendering_mutex);
}

void lwttl_set_sea_udp(LWTTL* ttl, LWUDP* sea_udp) {
    ttl->sea_udp = sea_udp;
}

void lwttl_udp_update(LWTTL* ttl, LWUDP* udp, LWCONTEXT* pLwc) {
    if (pLwc->game_scene != LGS_TTL) {
        return;
    }
    if (udp->reinit_next_update) {
        destroy_udp(&ttl->sea_udp);
        ttl->sea_udp = new_udp();
        udp_update_addr_host(ttl->sea_udp,
                             pLwc->sea_udp_host_addr.host,
                             pLwc->sea_udp_host_addr.port,
                             pLwc->sea_udp_host_addr.port_str);
        udp->reinit_next_update = 0;
    }
    if (udp->ready == 0) {
        return;
    }
    float app_time = (float)pLwc->app_time;
    float ping_send_interval = lwcontext_update_interval(pLwc) * 2;
    if (udp->last_updated == 0 || app_time > udp->last_updated + ping_send_interval) {
        lwttl_udp_send_ttlping(ttl, udp, udp->ping_seq);
        udp->ping_seq++;
        udp->last_updated = app_time;
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
                LOGEP("recvfrom() failed with error code : %d", wsa_error_code);
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

        char decompressed[1500 * 255]; // maximum lz4 compression ratio is 255...
        int decompressed_bytes = LZ4_decompress_safe(udp->buf, decompressed, udp->recv_len, ARRAY_SIZE(decompressed));
        if (decompressed_bytes > 0) {
            const int packet_type = *(int*)decompressed;
            switch (packet_type) {
            case LPGP_LWPTTLSEAAREA:
            {
                if (decompressed_bytes != sizeof(LWPTTLSEAAREA)) {
                    LOGE("LWPTTLSEAAREA: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSEAAREA));
                }

                LWPTTLSEAAREA* p = (LWPTTLSEAAREA*)decompressed;
                LOGIx("LWPTTLSEAAREA: name=%s", p->name);
                lwttl_set_seaarea(pLwc->ttl, p->name);
                break;
            }
            case LPGP_LWPTTLTRACKCOORDS:
            {
                if (decompressed_bytes != sizeof(LWPTTLTRACKCOORDS)) {
                    LOGE("LPGP_LWPTTLTRACKCOORDS: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLTRACKCOORDS));
                }

                LWPTTLTRACKCOORDS* p = (LWPTTLTRACKCOORDS*)decompressed;
                if (p->id) {
                    LOGIx("LWPTTLTRACKCOORDS: id=%d x=%f y=%f", p->id, p->x, p->y);
                    const float lng = cell_fx_to_lng(p->x);
                    const float lat = cell_fy_to_lat(p->y);
                    lwttl_set_center(pLwc->ttl, lng, lat);
                } else {
                    lwttl_set_track_object_id(pLwc->ttl, 0);
                    lwttl_set_track_object_ship_id(pLwc->ttl, 0);
                }
                break;
            }
            case LPGP_LWPTTLFULLSTATE:
            {
                if (decompressed_bytes != sizeof(LWPTTLFULLSTATE)) {
                    LOGE("LWPTTLFULLSTATE: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLFULLSTATE));
                }

                LWPTTLFULLSTATE* p = (LWPTTLFULLSTATE*)decompressed;
                LOGIx("LWPTTLFULLSTATE: %d objects.", p->count);
                memcpy(&ttl->ttl_full_state, p, sizeof(LWPTTLFULLSTATE));
                break;
            }
            case LPGP_LWPTTLSTATICSTATE2:
            {
                if (decompressed_bytes != sizeof(LWPTTLSTATICSTATE2)) {
                    LOGE("LWPTTLSTATICSTATE2: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSTATICSTATE2));
                }

                LWPTTLSTATICSTATE2* p = (LWPTTLSTATICSTATE2*)decompressed;
                LOGIx("LWPTTLSTATICSTATE2: %d objects.", p->count);
                lwttl_set_xc0(pLwc->ttl, p->xc0);
                lwttl_set_yc0(pLwc->ttl, p->yc0);
                //lwttl_lock_rendering_mutex(pLwc->ttl);
                //lwttl_unlock_rendering_mutex(pLwc->ttl);
                //lwttl_set_view_scale(pLwc->ttl, p->view_scale);
                add_to_static_object_cache(&ttl->static_object_cache, p);
                break;
            }
            case LPGP_LWPTTLSEAPORTSTATE:
            {
                if (decompressed_bytes != sizeof(LWPTTLSEAPORTSTATE)) {
                    LOGE("LWPTTLSEAPORTSTATE: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSEAPORTSTATE));
                }

                LWPTTLSEAPORTSTATE* p = (LWPTTLSEAPORTSTATE*)decompressed;
                LOGIx("LWPTTLSEAPORTSTATE: %d objects.", p->count);
                memcpy(&ttl->ttl_seaport_state, p, sizeof(LWPTTLSEAPORTSTATE));
                // fill nearest seaports
                /*htmlui_clear_loop(pLwc->htmlui, "seaport");
                for (int i = 0; i < p->count; i++) {
                htmlui_set_loop_key_value(pLwc->htmlui, "seaport", "name", ttl->ttl_seaport_state.obj[i].name);
                char script[128];
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                cell_x_to_lng(ttl->ttl_seaport_state.obj[i].x0),
                cell_y_to_lat(ttl->ttl_seaport_state.obj[i].y0));
                htmlui_set_loop_key_value(pLwc->htmlui, "seaport", "script", script);
                }*/
                // fill world seaports
                char script[128];
                htmlui_clear_loop(pLwc->htmlui, "world-seaport");

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Yokohama");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        139.0f + 39.0f / 60.0f,
                        35.0f + 27.0f / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Shanghai");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        121.0f + 29.0f / 60.0f,
                        31.0f + 14.0f / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Tianjin");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        117.0f + 12.0f / 60.0f,
                        39.0f + 2.0f / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Busan");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        129.0f + 3.0f / 60.0f,
                        35.0f + 8.0f / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Panama Canal");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        -(79.0f + 40.0f / 60.0f),
                        9.0f + 4.0f / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "As Suways(Suez)");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        32.0f + 31.0f / 60.0f,
                        29.0f + 58.0f / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Hong Kong Central");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        114.0f + 9.0f / 60.0f,
                        22.0f + 16.0f / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Jurong/Singapore");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        103.0f + 42.0f / 60.0f,
                        1.0f + 20.0f / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Port Of South Louisiana");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        -(90 + 30 / 60.0f),
                        30 + 3 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Pilottown");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        -(89 + 15 / 60.0f),
                        29 + 11 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Sydney");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        151 + 12 / 60.0f,
                        -(33 + 51 / 60.0f));
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Honolulu");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        -(157 + 51 / 60.0f),
                        21 + 18 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Seoul");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        126 + 56 / 60.0f,
                        37 + 31 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Civitavecchia");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        11 + 48 / 60.0f,
                        42 + 6 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Rome (Roma)");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        12 + 29 / 60.0f,
                        41 + 54 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Sunderland");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        -(1 + 23 / 60.0f),
                        54 + 54 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Dokdo");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        131 + 51 / 60.0f,
                        37 + 14 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Dokdo Precise");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        131 + 52 / 60.0f + 8.045f / 3600.0f,
                        37 + 14 / 60.0f + 21.786f / 3600.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "[0,0] (LNG -180 LAT 90)");
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                        -180 + 0 / 60.0f,
                        90 + 0 / 60.0f);
                htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                break;
            }
            case LPGP_LWPTTLWAYPOINTS:
            {
                if (decompressed_bytes != sizeof(LWPTTLWAYPOINTS)) {
                    LOGE("LWPTTLWAYPOINTS: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLWAYPOINTS));
                }
                LWPTTLWAYPOINTS* p = (LWPTTLWAYPOINTS*)decompressed;
                LOGI("LWPTTLWAYPOINTS: %d objects.", p->count);
                memcpy(&ttl->waypoints, p, sizeof(LWPTTLWAYPOINTS));
                break;
            }
            default:
            {
                LOGEP("Unknown UDP packet");
                break;
            }
            }
        } else {
            LOGEP("lz4 decompression failed!");
        }
        }
    }

const LWPTTLWAYPOINTS* lwttl_get_waypoints(const LWTTL* ttl) {
    return &ttl->waypoints;
}

void lwttl_write_last_state(const LWTTL* ttl, const LWCONTEXT* pLwc) {
    LWTTLSAVEDATA save;
    save.magic = 0x19850506;
    save.version = 1;
    save.lng = ttl->worldmap.center.lng;
    save.lat = ttl->worldmap.center.lat;
    save.view_scale = ttl->view_scale;
    write_file_binary(pLwc->user_data_path, "lwttl.dat", (const char*)&save, sizeof(LWTTLSAVEDATA));
}

void lwttl_read_last_state(LWTTL* ttl, const LWCONTEXT* pLwc) {
    if (is_file_exist(pLwc->user_data_path, "lwttl.dat")) {
        LWTTLSAVEDATA save;
        memset(&save, 0, sizeof(LWTTLSAVEDATA));
        int ret = read_file_binary(pLwc->user_data_path, "lwttl.dat", sizeof(LWTTLSAVEDATA), (char*)&save);
        if (ret == 0) {
            if (save.magic != 0x19850506) {
                LOGIP("TTL save data magic not match. Will be ignored and rewritten upon exit.");
            } else if (save.version != 1) {
                LOGIP("TTL save data version not match. Will be ignored and rewritten upon exit.");
            } else {
                ttl->worldmap.center.lng = LWCLAMP(save.lng, -180.0f, +180.0f);
                ttl->worldmap.center.lat = LWCLAMP(save.lat, -90.0f, +90.0f);
                ttl->view_scale = LWCLAMP(save.view_scale, 1, 64);
            }
        } else {
            LOGEP("TTL save data read failed! - return code: %d", ret);
        }
    }
}

const LWPTTLFULLSTATE* lwttl_full_state(const LWTTL* ttl) {
    return &ttl->ttl_full_state;
}

const LWPTTLSEAPORTSTATE* lwttl_seaport_state(const LWTTL* ttl) {
    return &ttl->ttl_seaport_state;
}

int lwttl_query_static_object_chunk_range(const LWTTL* ttl,
                                          const float lng_min,
                                          const float lng_max,
                                          const float lat_min,
                                          const float lat_max,
                                          const int view_scale,
                                          int* chunk_index_array,
                                          const int chunk_index_array_len) {
    LWTTLCELLBOUND cell_bound = {
        lwttl_lng_to_floor_int(lng_min),
        lwttl_lat_to_floor_int(lat_max),
        lwttl_lng_to_ceil_int(lng_max),
        lwttl_lat_to_ceil_int(lat_min),
    };
    LWTTLCHUNKBOUND chunk_bound;
    cell_bound_to_chunk_bound(&cell_bound, view_scale, &chunk_bound);
    const int xc_count = chunk_bound.xcc1 - chunk_bound.xcc0 + 1;
    const int yc_count = chunk_bound.ycc1 - chunk_bound.ycc0 + 1;
    int chunk_index_count = 0;
    for (int i = 0; i < xc_count; i++) {
        for (int j = 0; j < yc_count; j++) {
            const int xcc0 = chunk_bound.xcc0 + i;
            const int ycc0 = chunk_bound.ycc0 + j;
            LWTTLSTATICOBJECT2_CHUNK_KEY chunk_key;
            chunk_key.bf.xcc0 = xcc0;
            chunk_key.bf.ycc0 = ycc0;
            chunk_key.bf.view_scale_msb = msb_index(view_scale);
            const int chunk_index = find_static_object_chunk_index(&ttl->static_object_cache, chunk_key);
            if (chunk_index >= 0) {
                if (chunk_index_count < chunk_index_array_len) {
                    chunk_index_array[chunk_index_count] = chunk_index;
                }
                chunk_index_count++;
            }
        }
    }
    return chunk_index_count;
}

const LWPTTLSTATICOBJECT2* lwttl_query_static_object_chunk(const LWTTL* ttl,
                                                           const int chunk_index,
                                                           int* xc0,
                                                           int* yc0,
                                                           int* count) {
    if (chunk_index >= 0 && chunk_index < ttl->static_object_cache.static_object_chunk_count) {
        const int start = ttl->static_object_cache.static_object_chunk_value[chunk_index].start;
        const int view_scale = 1 << ttl->static_object_cache.static_object_chunk_key[chunk_index].bf.view_scale_msb;
        *xc0 = ttl->static_object_cache.static_object_chunk_key[chunk_index].bf.xcc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
        *yc0 = ttl->static_object_cache.static_object_chunk_key[chunk_index].bf.ycc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
        *count = ttl->static_object_cache.static_object_chunk_value[chunk_index].count;
        return &ttl->static_object_cache.static_object[start];
    } else {
        return 0;
    }
}

LWUDP* lwttl_sea_udp(LWTTL* ttl) {
    return ttl->sea_udp;
}

void lwttl_set_earth_globe_scale(LWTTL* ttl, float earth_globe_scale) {
    ttl->earth_globe_scale = LWCLAMP(earth_globe_scale, 0.1f, ttl->earth_globe_scale_0);
    const float earth_globe_morph_weight = lwttl_earth_globe_morph_weight(ttl->earth_globe_scale);
    LOGI("Globe scale: %.2f, Morph weight: %.2f", ttl->earth_globe_scale, earth_globe_morph_weight);
}

void lwttl_scroll_earth_globe_scale(LWTTL* ttl, float offset) {
    if (offset > 0) {
        lwttl_set_earth_globe_scale(ttl, ttl->earth_globe_scale * 1.2f);
    } else {
        lwttl_set_earth_globe_scale(ttl, ttl->earth_globe_scale / 1.2f);
    }
}

float lwttl_earth_globe_scale(LWTTL* ttl) {
    return ttl->earth_globe_scale;
}

float lwttl_earth_globe_morph_weight(float earth_globe_scale) {
    return 1.0f / (1.0f + expf(0.5f * (earth_globe_scale - 35.0f))); // 0: plane, 1: globe
}

float lwttl_earth_globe_y(const LWTTLLNGLAT* center, float earth_globe_scale, float earth_globe_morph_weight) {
    const float globe_y = -sinf((float)LWDEG2RAD(center->lat)) * earth_globe_scale;
    const float plane_y = -(float)M_PI / 2 * center->lat / 90.0f * earth_globe_scale; // plane height is M_PI
    return (1.0f - earth_globe_morph_weight) * plane_y + earth_globe_morph_weight * globe_y;
}
