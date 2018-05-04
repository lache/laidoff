#pragma once

#include <linmath.h>

#ifdef __cplusplus
extern "C" {;
#endif
typedef struct _LWCONTEXT LWCONTEXT;
typedef struct _LWUDP LWUDP;
typedef struct _LWTTL LWTTL;
typedef struct _LWPTTLWAYPOINTS LWPTTLWAYPOINTS;
typedef struct _LWPTTLFULLSTATE LWPTTLFULLSTATE;
typedef struct _LWPTTLSTATICSTATE2 LWPTTLSTATICSTATE2;
typedef struct _LWPTTLSEAPORTSTATE LWPTTLSEAPORTSTATE;
typedef struct _LWPTTLSTATICOBJECT2 LWPTTLSTATICOBJECT2;
typedef struct _LWPTTLSEAPORTOBJECT LWPTTLSEAPORTOBJECT;
typedef struct _LWTTLLNGLAT LWTTLLNGLAT;
typedef struct _LWHTMLUI LWHTMLUI;

LWTTL* lwttl_new(float aspect_ratio);
void lwttl_destroy(LWTTL** _ttl);
float lnglat_to_xy(const LWCONTEXT* pLwc, float v);
void lwttl_worldmap_scroll_to(LWTTL* ttl, float lng, float lat, LWUDP* sea_udp);
void lwttl_worldmap_scroll_to_cell_center(LWTTL* ttl, int xc, int yc, LWUDP* sea_udp);
void lwttl_update_aspect_ratio(LWTTL* ttl, float aspect_ratio);
const LWTTLLNGLAT* lwttl_center(const LWTTL* ttl);
void lwttl_update(LWTTL* ttl, LWCONTEXT* pLwc, float delta_time);
const char* lwttl_http_header(const LWTTL* ttl);
int lwttl_track_object_id(const LWTTL* ttl);
void lwttl_set_track_object_id(LWTTL* ttl, int v);
int lwttl_track_object_ship_id(const LWTTL* ttl);
void lwttl_set_track_object_ship_id(LWTTL* ttl, int v);
void lwttl_request_waypoints(const LWTTL* ttl, int v);
void lwttl_set_center(LWTTL* ttl, float lng, float lat);
void lwttl_set_seaarea(LWTTL* ttl, const char* name);
const char* lwttl_seaarea(LWTTL* ttl);
void lwttl_set_view_scale(LWTTL* ttl, int v);
int lwttl_view_scale(const LWTTL* ttl);
int lwttl_view_scale_max(const LWTTL* ttl);
int lwttl_clamped_view_scale(const LWTTL* ttl);
void lwttl_set_sea_udp(LWTTL* ttl, LWUDP* sea_udp);
void lwttl_udp_send_ttlping(const LWTTL* ttl, LWUDP* udp, int ping_seq);
void lwttl_udp_send_request_waypoints(const LWTTL* ttl, LWUDP* sea_udp, int ship_id);
void lwttl_udp_update(LWTTL* ttl, LWUDP* udp, LWCONTEXT* pLwc);
void lwttl_lock_rendering_mutex(LWTTL* ttl);
void lwttl_unlock_rendering_mutex(LWTTL* ttl);
const LWPTTLWAYPOINTS* lwttl_get_waypoints(const LWTTL* ttl);
void lwttl_write_last_state(const LWTTL* ttl, const LWCONTEXT* pLwc);
void lwttl_read_last_state(LWTTL* ttl, const LWCONTEXT* pLwc);
const LWPTTLFULLSTATE* lwttl_full_state(const LWTTL* ttl);
int lwttl_query_chunk_range_land(const LWTTL* ttl,
                                 const float lng_min,
                                 const float lng_max,
                                 const float lat_min,
                                 const float lat_max,
                                 const int view_scale,
                                 int* chunk_index_array,
                                 const int chunk_index_array_len);
int lwttl_query_chunk_range_seaport(const LWTTL* ttl,
                                    const float lng_min,
                                    const float lng_max,
                                    const float lat_min,
                                    const float lat_max,
                                    const int view_scale,
                                    int* chunk_index_array,
                                    const int chunk_index_array_len);
const LWPTTLSTATICOBJECT2* lwttl_query_chunk_land(const LWTTL* ttl,
                                                  const int chunk_index,
                                                  int* xc0,
                                                  int* yc0,
                                                  int* count);
const LWPTTLSEAPORTOBJECT* lwttl_query_chunk_seaport(const LWTTL* ttl,
                                                     const int chunk_index,
                                                     int* xc0,
                                                     int* yc0,
                                                     int* count);
float lwttl_half_lng_extent_in_degrees(const int view_scale);
float lwttl_half_lat_extent_in_degrees(const int view_scale);
int lwttl_lng_to_floor_int(float lng);
int lwttl_lat_to_floor_int(float lat);
int lwttl_lng_to_ceil_int(float lng);
int lwttl_lat_to_ceil_int(float lat);
LWUDP* lwttl_sea_udp(LWTTL* ttl);
void lwttl_set_earth_globe_scale(LWTTL* ttl, float earth_globe_scale);
void lwttl_scroll_view_scale(LWTTL* ttl, float offset);
float lwttl_earth_globe_scale(LWTTL* ttl);
float lwttl_earth_globe_morph_weight(float earth_globe_scale);
float lwttl_earth_globe_y(const LWTTLLNGLAT* center, float earth_globe_scale, float earth_globe_morph_weight);
void lwttl_fill_world_seaports_bookmarks(LWHTMLUI* htmlui);
void lwttl_send_ping_now(LWTTL* ttl);
void lwttl_prerender_mutable_context(LWTTL* ttl, LWCONTEXT* pLwc, LWHTMLUI* htmlui);
int lwttl_selected(const LWTTL* ttl, LWTTLLNGLAT* pos);
void lwttl_on_press(LWTTL* ttl, const LWCONTEXT* pLwc, float x, float y);
void lwttl_view_proj(LWTTL* ttl, mat4x4 view, mat4x4 proj);
void lwttl_calc_view_proj(LWTTL* ttl, float aspect_ratio);
void GetWorldCoords(const float touchX,
                    const float touchY,
                    const float screenW,
                    const float screenH,
                    const mat4x4 proj,
                    const mat4x4 view_model,
                    vec2 worldPos);
#ifdef __cplusplus
}
#endif
