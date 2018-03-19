#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

typedef struct _LWTTLWORLDMAP {
    float render_org_x;
    float render_org_y;
    float center_lat;
    float center_lng;
    float zoom_scale;
} LWTTLWORLDMAP;

void* lwttl_new(float aspect_ratio);
void lwttl_destroy(void** __ttl);
void lwttl_render_all_seaports(const LWCONTEXT* pLwc, const void* _ttl, const LWTTLWORLDMAP* worldmap);
float lnglat_to_xy(const LWCONTEXT* pLwc, float v);
void lwttl_worldmap_scroll(void* _ttl, float dlng, float dlat, float dzoom);
const LWTTLWORLDMAP* lwttl_worldmap(void* _ttl);
void lwttl_update_aspect_ratio(void* _ttl, float aspect_ratio);