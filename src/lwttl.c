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
#include "input.h"

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

typedef struct _LWTTLCHUNKVALUE {
    LWTTLCHUNKKEY key;
    unsigned int ts;
    int start;
    int count;
} LWTTLCHUNKVALUE;

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

#define TTL_OBJECT_CACHE_CHUNK_COUNT (256*1024)
#define TTL_OBJECT_CACHE_LAND_COUNT (TTL_OBJECT_CACHE_CHUNK_COUNT*32)
#define TTL_OBJECT_CACHE_SEAPORT_COUNT (TTL_OBJECT_CACHE_CHUNK_COUNT*32)

typedef struct _LWTTLOBJECTCACHE {
    int count;
    LWTTLCHUNKKEY key_array[TTL_OBJECT_CACHE_CHUNK_COUNT];
    LWTTLCHUNKVALUE value_array[TTL_OBJECT_CACHE_CHUNK_COUNT];
} LWTTLOBJECTCACHE;

typedef struct _LWTTLOBJECTCACHEGROUP {
    LWTTLOBJECTCACHE land_cache;
    LWPTTLSTATICOBJECT2 land_array[TTL_OBJECT_CACHE_LAND_COUNT];
    int land_count;

    LWTTLOBJECTCACHE seaport_cache;
    LWPTTLSEAPORTOBJECT seaport_array[TTL_OBJECT_CACHE_SEAPORT_COUNT];
    int seaport_count;
} LWTTLOBJECTCACHEGROUP;

typedef struct _LWTTLSAVEDATA {
    int magic;
    int version;
    float lng;
    float lat;
    int view_scale;
} LWTTLSAVEDATA;

typedef struct _LWTTLWORLDMAP {
    float render_org_x;
    float render_org_y;
    LWTTLLNGLAT center;
} LWTTLWORLDMAP;

typedef struct _LWTTL {
    int version;
    LWTTLDATA_SEAPORT* seaport;
    size_t seaport_len;
    LWTTLWORLDMAP worldmap;
    int track_object_id;
    int track_object_ship_id;
    char seaarea[128]; // should match with LWPTTLSEAAREA.name size
    int view_scale;
    int view_scale_max;
    int view_scale_ping_max;
    //int xc0;
    //int yc0;
    LWMUTEX rendering_mutex;
    LWUDP* sea_udp;
    LWPTTLWAYPOINTS waypoints;
    // packet cache
    LWPTTLFULLSTATE ttl_full_state;
    //LWPTTLSEAPORTSTATE ttl_seaport_state;
    LWTTLOBJECTCACHEGROUP object_cache;
    float earth_globe_scale;
    float earth_globe_scale_0;
} LWTTL;

LWTTL* lwttl_new(float aspect_ratio) {
    LWTTL* ttl = (LWTTL*)calloc(1, sizeof(LWTTL));
    ttl->version = 1;
    ttl->worldmap.render_org_x = 0;
    ttl->worldmap.render_org_y = -(2.0f - aspect_ratio) / 2;
    // Ulsan
    lwttl_worldmap_scroll_to(ttl, 129.496f, 35.494f, 0);
    size_t seaports_dat_size;
    ttl->seaport = (LWTTLDATA_SEAPORT*)create_binary_from_file(ASSETS_BASE_PATH "ttldata" PATH_SEPARATOR "seaports.dat", &seaports_dat_size);
    ttl->seaport_len = seaports_dat_size / sizeof(LWTTLDATA_SEAPORT);
    ttl->view_scale_max = 1 << 13; // should be no more than 2^15 (== 2 ^ MAX(LWTTLCHUNKKEY.view_scale_msb))
    ttl->view_scale_ping_max = 64;
    ttl->view_scale = ttl->view_scale_ping_max;
    LWMUTEX_INIT(ttl->rendering_mutex);
    ttl->earth_globe_scale_0 = earth_globe_render_scale;
    lwttl_set_earth_globe_scale(ttl, ttl->earth_globe_scale_0);
    return ttl;
}

void lwttl_destroy(LWTTL** __ttl) {
    LWTTL* ttl = *(LWTTL**)__ttl;
    LWMUTEX_DESTROY(ttl->rendering_mutex);
    free(*__ttl);
    *__ttl = 0;
}

float lnglat_to_xy(const LWCONTEXT* pLwc, float v) {
    return v * 1.0f / 180.0f * pLwc->aspect_ratio;
}

void lwttl_worldmap_scroll_to(LWTTL* ttl, float lng, float lat, LWUDP* sea_udp) {
    // cancel tracking if user want to scroll around
    lwttl_set_track_object_ship_id(ttl, 0);
    ttl->worldmap.center.lng = numcomp_wrap_min_max(lng, -180.0f, +180.0f);
    ttl->worldmap.center.lat = LWCLAMP(lat, -90.0f, +90.0f);
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

void lwttl_update_aspect_ratio(LWTTL* ttl, float aspect_ratio) {
    ttl->worldmap.render_org_x = 0;
    ttl->worldmap.render_org_y = -(2.0f - aspect_ratio) / 2;
}

const LWTTLLNGLAT* lwttl_center(const LWTTL* ttl) {
    return &ttl->worldmap.center;
}

void lwttl_set_center(LWTTL* ttl, float lng, float lat) {
    lwttl_worldmap_scroll_to(ttl, lng, lat, 0);
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
    if (lw_pinch() == 0) {
        if (lw_get_normalized_dir_pad_input(pLwc, &pLwc->left_dir_pad, &dx, &dy, &dlen) && (dx || dy)) {
            // cancel tracking if user want to scroll around
            lwttl_set_track_object_ship_id(ttl, 0);
            // direction inverted
            lwttl_worldmap_scroll_to(ttl,
                                     ttl->worldmap.center.lng + (-dx) / 50.0f * delta_time * ttl->view_scale,
                                     ttl->worldmap.center.lat + (-dy) / 50.0f * delta_time * ttl->view_scale,
                                     0);
        }
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

int lwttl_view_scale(const LWTTL* ttl) {
    return ttl->view_scale;
}

int lwttl_view_scale_max(const LWTTL* ttl) {
    return ttl->view_scale_max;
}

int lwttl_clamped_view_scale(const LWTTL* ttl) {
    // max value of view_scale bounded by LWTTLCHUNKKEY
    return LWCLAMP(ttl->view_scale, 1, ttl->view_scale_ping_max);
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

static int find_chunk_index(const LWTTLOBJECTCACHE* c, const LWTTLCHUNKKEY chunk_key) {
    int chunk_index = lower_bound_int(&c->key_array[0].v, c->count, chunk_key.v);
    if (chunk_index >= 0 && c->key_array[chunk_index].v == chunk_key.v) {
        return chunk_index;
    }
    return -1;
}

static unsigned int find_chunk_ts(const LWTTLOBJECTCACHE* c, const LWTTLCHUNKKEY chunk_key) {
    const int chunk_index = find_chunk_index(c, chunk_key);
    if (chunk_index >= 0 && chunk_index < c->count) {
        return c->value_array[chunk_index].ts;
    }
    return 0;
}

static int add_to_object_cache(LWTTLOBJECTCACHE* c,
                               LWTTLCHUNKVALUE* chunk_value_array,
                               int* cache_count,
                               void* cache_array,
                               const size_t entry_size,
                               const int entry_max_count,
                               const unsigned int ts,
                               const int xc0,
                               const int yc0,
                               const int view_scale,
                               const int count,
                               const void* obj) {
    if (c == 0) {
        LOGEP("c == 0");
        abort();
        return -1;
    }
    if (cache_count == 0) {
        LOGEP("cache_count == 0");
        abort();
        return -2;
    }
    if (cache_array == 0) {
        LOGEP("cache_array == 0");
        abort();
        return -3;
    }
    if (entry_size == 0) {
        LOGEP("entry_size == 0");
        abort();
        return -4;
    }
    if (view_scale <= 0) {
        LOGEP("view_scale <= 0");
        abort();
        return -5;
    }
    if (obj == 0) {
        LOGEP("obj == 0");
        abort();
        return -6;
    }
    // check for existing chunk entry
    LWTTLCHUNKKEY chunk_key;
    chunk_key.bf.xcc0 = xc0 >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_key.bf.ycc0 = yc0 >> msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
    chunk_key.bf.view_scale_msb = msb_index(view_scale);
    int chunk_index = lower_bound_int(&c->key_array[0].v, c->count, chunk_key.v);
    if (chunk_index >= 0 && c->key_array[chunk_index].v == chunk_key.v) {
        // chunk key found.
        // check for cache entry in chunk value array
        if (chunk_value_array[chunk_index].key.v == chunk_key.v) {
            // already exist. do nothing
            // TODO update?
            return 1;
        } else {
            // cache entry not exists.
            int a = 10;
        }
    } else {
        // new chunk entry received.
        // check current capacity
        if (c->count >= TTL_OBJECT_CACHE_CHUNK_COUNT) {
            LOGEP("TTL_OBJECT_CACHE_CHUNK_COUNT exceeded.");
            return -7;
        }
        if (*cache_count + count > entry_max_count) {
            LOGEP("entry_max_count exceeded.");
            return -8;
        }
        // safe to add a new chunk at 'chunk_index'
        // move chunk (move by 1-index by copying in backward direction)
        for (int from = c->count - 1; from >= chunk_index + 1; from--) {
            c->key_array[from + 1] = c->key_array[from];
            chunk_value_array[from + 1] = chunk_value_array[from];
        }
        c->count++;
        chunk_index++;
    }
    c->key_array[chunk_index] = chunk_key;
    chunk_value_array[chunk_index].key = chunk_key;
    chunk_value_array[chunk_index].ts = ts;
    chunk_value_array[chunk_index].start = *cache_count;
    chunk_value_array[chunk_index].count = count;
    // copy data
    memcpy((char*)cache_array + entry_size * (*cache_count),
           obj,
           entry_size * count);
    // increase count indices
    *cache_count += count;
    return 0;
}

static int add_to_object_cache_land(LWTTLOBJECTCACHE* c,
                                    LWPTTLSTATICOBJECT2* land_array,
                                    const size_t land_array_size,
                                    int* land_count,
                                    const LWPTTLSTATICSTATE2* s2) {
    return add_to_object_cache(c,
                               c->value_array,
                               land_count,
                               land_array,
                               sizeof(LWPTTLSTATICOBJECT2),
                               land_array_size,
                               s2->ts,
                               s2->xc0,
                               s2->yc0,
                               s2->view_scale,
                               s2->count,
                               s2->obj);
}

static int add_to_object_cache_seaport(LWTTLOBJECTCACHE* c,
                                       LWPTTLSEAPORTOBJECT* seaport_array,
                                       const size_t seaport_array_size,
                                       int* seaport_count,
                                       const LWPTTLSEAPORTSTATE* s2) {
    return add_to_object_cache(c,
                               c->value_array,
                               seaport_count,
                               seaport_array,
                               sizeof(LWPTTLSEAPORTOBJECT),
                               seaport_array_size,
                               s2->ts,
                               s2->xc0,
                               s2->yc0,
                               s2->view_scale,
                               s2->count,
                               s2->obj);
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
                         const int view_scale,
                         const float ex_lng,
                         const float ex_lat) {
    LWPTTLPING p;
    memset(&p, 0, sizeof(LWPTTLPING));
    p.type = LPGP_LWPTTLPING;
    p.lng = lng;
    p.lat = lat;
    p.ex_lng = ex_lng;
    p.ex_lat = ex_lat;
    p.track_object_id = lwttl_track_object_id(ttl);
    p.track_object_ship_id = lwttl_track_object_ship_id(ttl);
    p.view_scale = view_scale;
    udp_send(udp, (const char*)&p, sizeof(LWPTTLPING));
}

static void send_ttlpingchunk(const LWTTL* ttl,
                              LWUDP* udp,
                              const LWTTLCHUNKKEY chunk_key,
                              const unsigned char static_object,
                              const unsigned int ts) {
    LWPTTLPINGCHUNK p;
    memset(&p, 0, sizeof(LWPTTLPINGCHUNK));
    p.type = LPGP_LWPTTLPINGCHUNK;
    p.static_object = static_object;
    p.chunk_key = chunk_key;
    p.ts = ts;
    udp_send(udp, (const char*)&p, sizeof(LWPTTLPINGCHUNK));
}

static void send_ttlpingflush(LWTTL* ttl) {
    LWPTTLPINGFLUSH p;
    memset(&p, 0, sizeof(LWPTTLPINGFLUSH));
    p.type = LPGP_LWPTTLPINGFLUSH;
    udp_send(ttl->sea_udp, (const char*)&p, sizeof(LWPTTLPINGFLUSH));
}

float lwttl_half_lng_extent_in_degrees(const int view_scale) {
    return LNGLAT_SEA_PING_EXTENT_IN_DEGREES / 2 * view_scale * LNGLAT_RENDER_EXTENT_MULTIPLIER_LNG;
}

float lwttl_half_lat_extent_in_degrees(const int view_scale) {
    return LNGLAT_SEA_PING_EXTENT_IN_DEGREES / 2 * view_scale * LNGLAT_RENDER_EXTENT_MULTIPLIER_LAT;
}

static void send_ttlping_if_chunk_not_found(const LWTTL* ttl,
                                            LWUDP* udp,
                                            const LWTTLOBJECTCACHE* c,
                                            const LWTTLCHUNKKEY chunk_key,
                                            const unsigned char static_object) {
    const unsigned int ts = find_chunk_ts(c, chunk_key);
    send_ttlpingchunk(ttl,
                      udp,
                      chunk_key,
                      static_object,
                      ts);
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
            LWTTLCHUNKKEY chunk_key;
            chunk_key.bf.xcc0 = xcc0;
            chunk_key.bf.ycc0 = ycc0;
            chunk_key.bf.view_scale_msb = msb_index(clamped_view_scale);
            const struct {
                const LWTTLOBJECTCACHE* c;
                const unsigned char static_object;
            } cache_list[] = {
                { &ttl->object_cache.land_cache, 1 },
                { &ttl->object_cache.seaport_cache, 2 },
            };
            for (int k = 0; k < ARRAY_SIZE(cache_list); k++) {
                send_ttlping_if_chunk_not_found(ttl,
                                                udp,
                                                cache_list[k].c,
                                                chunk_key,
                                                cache_list[k].static_object);
            }
        }
    }
    // ping for dynamic object (ships)
    const int xc0 = lwttl_lng_to_round_int(center->lng) & ~(clamped_view_scale - 1);
    const int yc0 = lwttl_lat_to_round_int(center->lat) & ~(clamped_view_scale - 1);
    send_ttlping(ttl,
                 udp,
                 cell_x_to_lng(xc0),
                 cell_y_to_lat(yc0),
                 clamped_view_scale,
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
    float ping_send_interval = lwcontext_update_interval(pLwc) * 200;
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
                lwttl_set_seaarea(ttl, p->name);
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
                    lwttl_set_center(ttl, lng, lat);
                } else {
                    lwttl_set_track_object_id(ttl, 0);
                    lwttl_set_track_object_ship_id(ttl, 0);
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

                const int add_ret = add_to_object_cache_land(&ttl->object_cache.land_cache,
                                                             ttl->object_cache.land_array,
                                                             ARRAY_SIZE(ttl->object_cache.land_array),
                                                             &ttl->object_cache.land_count,
                                                             p);
                if (add_ret == 1) {
                    //send_ttlpingflush(ttl);
                }
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

                //memcpy(&ttl->ttl_seaport_state, p, sizeof(LWPTTLSEAPORTSTATE));

                const int add_ret = add_to_object_cache_seaport(&ttl->object_cache.seaport_cache,
                                                                ttl->object_cache.seaport_array,
                                                                ARRAY_SIZE(ttl->object_cache.seaport_array),
                                                                &ttl->object_cache.seaport_count,
                                                                p);
                if (add_ret == 1) {
                    //send_ttlpingflush(ttl);
                }


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
                lwttl_worldmap_scroll_to(ttl, save.lng, save.lat, 0);
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

static int lwttl_query_chunk_range(const LWTTL* ttl,
                                   const float lng_min,
                                   const float lng_max,
                                   const float lat_min,
                                   const float lat_max,
                                   const int view_scale,
                                   const LWTTLOBJECTCACHE* c,
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
            LWTTLCHUNKKEY chunk_key;
            chunk_key.bf.xcc0 = xcc0;
            chunk_key.bf.ycc0 = ycc0;
            chunk_key.bf.view_scale_msb = msb_index(view_scale);
            const int chunk_index = find_chunk_index(c, chunk_key);
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

int lwttl_query_chunk_range_land(const LWTTL* ttl,
                                 const float lng_min,
                                 const float lng_max,
                                 const float lat_min,
                                 const float lat_max,
                                 const int view_scale,
                                 int* chunk_index_array,
                                 const int chunk_index_array_len) {
    return lwttl_query_chunk_range(ttl,
                                   lng_min,
                                   lng_max,
                                   lat_min,
                                   lat_max,
                                   view_scale,
                                   &ttl->object_cache.land_cache,
                                   chunk_index_array,
                                   chunk_index_array_len);
}

int lwttl_query_chunk_range_seaport(const LWTTL* ttl,
                                    const float lng_min,
                                    const float lng_max,
                                    const float lat_min,
                                    const float lat_max,
                                    const int view_scale,
                                    int* chunk_index_array,
                                    const int chunk_index_array_len) {
    return lwttl_query_chunk_range(ttl,
                                   lng_min,
                                   lng_max,
                                   lat_min,
                                   lat_max,
                                   view_scale,
                                   &ttl->object_cache.seaport_cache,
                                   chunk_index_array,
                                   chunk_index_array_len);
}

const void* lwttl_query_chunk(const LWTTL* ttl,
                              const LWTTLOBJECTCACHE* c,
                              const int chunk_index,
                              const void* cache_array,
                              const size_t entry_size,
                              int* xc0,
                              int* yc0,
                              int* count) {
    if (chunk_index >= 0 && chunk_index < c->count) {
        const int start = c->value_array[chunk_index].start;
        const int view_scale = 1 << c->key_array[chunk_index].bf.view_scale_msb;
        *xc0 = c->key_array[chunk_index].bf.xcc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
        *yc0 = c->key_array[chunk_index].bf.ycc0 << msb_index(LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS * view_scale);
        *count = c->value_array[chunk_index].count;
        return (const char*)cache_array + start * entry_size;
    } else {
        return 0;
    }
}

const LWPTTLSTATICOBJECT2* lwttl_query_chunk_land(const LWTTL* ttl,
                                                  const int chunk_index,
                                                  int* xc0,
                                                  int* yc0,
                                                  int* count) {
    return lwttl_query_chunk(ttl,
                             &ttl->object_cache.land_cache,
                             chunk_index,
                             ttl->object_cache.land_array,
                             sizeof(LWPTTLSTATICOBJECT2),
                             xc0,
                             yc0,
                             count);
}

const LWPTTLSEAPORTOBJECT* lwttl_query_chunk_seaport(const LWTTL* ttl,
                                                     const int chunk_index,
                                                     int* xc0,
                                                     int* yc0,
                                                     int* count) {
    return lwttl_query_chunk(ttl,
                             &ttl->object_cache.seaport_cache,
                             chunk_index,
                             ttl->object_cache.seaport_array,
                             sizeof(LWPTTLSEAPORTOBJECT),
                             xc0,
                             yc0,
                             count);
}

LWUDP* lwttl_sea_udp(LWTTL* ttl) {
    return ttl->sea_udp;
}

void lwttl_set_earth_globe_scale(LWTTL* ttl, float earth_globe_scale) {
    ttl->earth_globe_scale = LWCLAMP(earth_globe_scale, 0.1f, ttl->earth_globe_scale_0);
    const float earth_globe_morph_weight = lwttl_earth_globe_morph_weight(ttl->earth_globe_scale);
    LOGI("Globe scale: %.2f, Morph weight: %.2f", ttl->earth_globe_scale, earth_globe_morph_weight);
}

void lwttl_scroll_view_scale(LWTTL* ttl, float offset) {
    int view_scale = lwttl_view_scale(ttl);
    if (offset > 0) {
        lwttl_set_view_scale(ttl, LWCLAMP(view_scale >> 1, 1, ttl->view_scale_max));
        lwttl_udp_send_ttlping(ttl, ttl->sea_udp, 0);
    } else {
        lwttl_set_view_scale(ttl, LWCLAMP(view_scale << 1, 1, ttl->view_scale_max));
        lwttl_udp_send_ttlping(ttl, ttl->sea_udp, 0);
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
