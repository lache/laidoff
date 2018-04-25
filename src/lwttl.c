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
    LWUDP* sea_udp;
    LWPTTLWAYPOINTS waypoints;
} LWTTL;

LWTTL* lwttl_new(float aspect_ratio) {
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
    ttl->worldmap.center.lat = lat;
    ttl->worldmap.center.lng = lng;
    if (sea_udp) {
        lwttl_udp_send_ttlping(ttl, sea_udp, 0);
    }
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

int lwttl_lng_to_int(float lng) {
    return (int)(LNGLAT_RES_WIDTH / 2 + (lng / 180.0f) * LNGLAT_RES_WIDTH / 2);
}

int lwttl_lat_to_int(float lat) {
    return (int)(LNGLAT_RES_HEIGHT / 2 - (lat / 90.0f) * LNGLAT_RES_HEIGHT / 2);
}

const char* lwttl_http_header(const LWTTL* _ttl) {
    static char http_header[1024];
    LWTTL* ttl = (LWTTL*)_ttl;
    const LWTTLLNGLAT* lnglat = lwttl_center(ttl);

    sprintf(http_header, "X-Lng: %d\r\nX-Lat: %d\r\n",
            lwttl_lng_to_int(lnglat->lng),
            lwttl_lat_to_int(lnglat->lat));
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

void lwttl_udp_send_ttlping(const LWTTL* ttl, LWUDP* udp, int ping_seq) {
    LWPTTLPING p;
    memset(&p, 0, sizeof(LWPTTLPING));
    const LWTTLLNGLAT* center = lwttl_center(ttl);
    p.type = LPGP_LWPTTLPING;
    p.lng = center->lng;
    p.lat = center->lat;
    p.ex = LNGLAT_SEA_PING_EXTENT_IN_CELL_PIXELS;
    p.ping_seq = ping_seq;
    p.track_object_id = lwttl_track_object_id(ttl);
    p.track_object_ship_id = lwttl_track_object_ship_id(ttl);
    p.view_scale = lwttl_view_scale(ttl);
    udp_send(udp, (const char*)&p, sizeof(LWPTTLPING));
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


static void convert_ttl_static_state2_to_1(const LWPTTLSTATICSTATE2* s2, LWPTTLSTATICSTATE* s) {
    memset(s, 0, sizeof(LWPTTLSTATICSTATE));
    s->count = s2->count;
    s->type = LPGP_LWPTTLSTATICSTATE;
    for (int i = 0; i < s2->count; i++) {
        s->obj[i].x0 = s2->xc0 + s2->view_scale * s2->obj[i].x_scaled_offset_0;
        s->obj[i].y0 = s2->yc0 + s2->view_scale * s2->obj[i].y_scaled_offset_0;
        s->obj[i].x1 = s2->xc0 + s2->view_scale * s2->obj[i].x_scaled_offset_1;
        s->obj[i].y1 = s2->yc0 + s2->view_scale * s2->obj[i].y_scaled_offset_1;
    }
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
                memcpy(&pLwc->ttl_full_state, p, sizeof(LWPTTLFULLSTATE));
                break;
            }
            case LPGP_LWPTTLSTATICSTATE:
            {
                if (decompressed_bytes != sizeof(LWPTTLSTATICSTATE)) {
                    LOGE("LWPTTLSTATICSTATE: Size error %d (%zu expected)",
                         decompressed_bytes,
                         sizeof(LWPTTLSTATICSTATE));
                }

                LWPTTLSTATICSTATE* p = (LWPTTLSTATICSTATE*)decompressed;
                LOGIx("LWPTTLSTATICSTATE: %d objects.", p->count);
                memcpy(&pLwc->ttl_static_state, p, sizeof(LWPTTLSTATICSTATE));
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
                LWPTTLSTATICSTATE pp;
                convert_ttl_static_state2_to_1(p, &pp);
                lwttl_set_xc0(pLwc->ttl, p->xc0);
                lwttl_set_yc0(pLwc->ttl, p->yc0);
                //lwttl_lock_rendering_mutex(pLwc->ttl);
                memcpy(&pLwc->ttl_static_state, &pp, sizeof(LWPTTLSTATICSTATE));
                //lwttl_unlock_rendering_mutex(pLwc->ttl);
                lwttl_set_view_scale(pLwc->ttl, p->view_scale);
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
                memcpy(&pLwc->ttl_seaport_state, p, sizeof(LWPTTLSEAPORTSTATE));
                // fill nearest seaports
                /*htmlui_clear_loop(pLwc->htmlui, "seaport");
                for (int i = 0; i < p->count; i++) {
                htmlui_set_loop_key_value(pLwc->htmlui, "seaport", "name", pLwc->ttl_seaport_state.obj[i].name);
                char script[128];
                sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.sea_udp)",
                cell_x_to_lng(pLwc->ttl_seaport_state.obj[i].x0),
                cell_y_to_lat(pLwc->ttl_seaport_state.obj[i].y0));
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
    write_file_binary(pLwc->user_data_path, "lwttl.dat", (const char*)ttl, sizeof(LWTTL));
}

void lwttl_read_last_state(LWTTL* ttl, const LWCONTEXT* pLwc) {
    if (is_file_exist(pLwc->user_data_path, "lwttl.dat")) {
        LWTTL last_ttl;
        read_file_binary(pLwc->user_data_path, "lwttl.dat", sizeof(LWTTL), (char*)&last_ttl);
        ttl->worldmap.center = last_ttl.worldmap.center;
        ttl->view_scale = last_ttl.view_scale;
    }
}
