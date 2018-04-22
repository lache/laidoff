#include "lwudp.h"
#include "lwlog.h"
#include "puckgamepacket.h"
#include "lwcontext.h"
#include "puckgameupdate.h"
#include "lwtimepoint.h"
#include "puckgame.h"
#include "platform_detection.h" 
#include "lwttl.h"
#include "htmlui.h"
#include "lwlnglat.h"
#if LW_PLATFORM_WIN32
#include <Ws2tcpip.h>
#endif
#include "lz4.h"
#include "remtex.h"
#include "lwcontext.h"

static int make_socket_nonblocking(int sock) {
#if defined(WIN32) || defined(_WIN32) || defined(IMN_PIM)
    unsigned long arg = 1;
    return ioctlsocket(sock, FIONBIO, &arg) == 0;
#elif defined(VXWORKS)
    int arg = 1;
    return ioctl(sock, FIONBIO, (int)&arg) == 0;
#else
    int curFlags = fcntl(sock, F_GETFL, 0);
    return fcntl(sock, F_SETFL, curFlags | O_NONBLOCK) >= 0;
#endif
}

LWUDP* new_udp() {
    LWUDP* udp = (LWUDP*)malloc(sizeof(LWUDP));
    memset(udp, 0, sizeof(LWUDP));
    udp->slen = sizeof(udp->si_other);
#if LW_PLATFORM_WIN32
    LOGI("Initialising Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &udp->wsa) != 0) {
        LOGE("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    LOGI("Initialised.\n");
#endif

    //create socket
    if ((udp->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
#if LW_PLATFORM_WIN32
        LOGE("socket() failed with error code : %d", WSAGetLastError());
#else
        LOGE("socket() failed...");
#endif
        exit(EXIT_FAILURE);
    }
#if LW_PLATFORM_IOS
    int set = 1;
    setsockopt(udp->s, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(int));
#endif
    //setup address structure
    memset((char *)&udp->si_other, 0, sizeof(udp->si_other));
    udp->si_other.sin_family = AF_INET;
    //struct hostent* he = gethostbyname(LW_UDP_SERVER);
    //struct in_addr** addr_list = (struct in_addr **) he->h_addr_list;
    //udp_update_addr(udp, addr_list[0]->s_addr, LW_UDP_PORT);
    udp->tv.tv_sec = 0;
    udp->tv.tv_usec = 0;
    make_socket_nonblocking(udp->s);
    udp->ready = 1;
    ringbuffer_init(&udp->state_ring_buffer, udp->state_buffer, sizeof(LWPSTATE), LW_STATE_RING_BUFFER_CAPACITY);
    ringbuffer_init(&udp->state2_ring_buffer, udp->state2_buffer, sizeof(LWPSTATE2), LW_STATE_RING_BUFFER_CAPACITY);
    numcomp_puck_game_init(&udp->numcomp);
    return udp;
}

static unsigned long udp_hostname_to_ip(const char* hostname, const char* port_str) {
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(hostname, port_str, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 0;
    }

    int ip = 0;
    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        h = (struct sockaddr_in *) p->ai_addr;
        ip = h->sin_addr.s_addr;
        break;
    }

    freeaddrinfo(servinfo); // all done with this structure
    return ip;
}

void udp_update_addr_host(LWUDP* udp, const char* host, unsigned short port, const char* port_str) {
    udp_update_addr(udp, udp_hostname_to_ip(host, port_str), port);
}

void udp_update_addr(LWUDP* udp, unsigned long ip, unsigned short port) {
    udp->si_other.sin_addr.s_addr = ip;
    udp->si_other.sin_port = htons(port);
}

void destroy_udp(LWUDP** udp) {
    closesocket((*udp)->s);
    free(*udp);
    *udp = 0;
}

void udp_send(LWUDP* udp, const char* data, int size) {
    if (!udp) {
        return;
    }
    //send the message
    if (sendto(udp->s, data, size, 0, (struct sockaddr *) &udp->si_other, udp->slen) == SOCKET_ERROR) {
#if LW_PLATFORM_WIN32
        //LOGE("sendto() failed with error code : %d", WSAGetLastError());
#else
        //LOGE("sendto() failed with error...");
#endif
        //exit(EXIT_FAILURE);
    }
}

void queue_state(LWUDP* udp, const LWPSTATE* p) {
    ringbuffer_queue(&udp->state_ring_buffer, p);
}

void queue_state2(LWUDP* udp, const LWPSTATE2* p) {
    ringbuffer_queue(&udp->state2_ring_buffer, p);
}

void udp_update(LWCONTEXT* pLwc, LWUDP* udp) {
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
            return;
#endif
        }
        const int packet_type = *(int*)udp->buf;
        switch (packet_type) {
        case LPGP_LWPSTATE:
        {
            if (udp->recv_len != sizeof(LWPSTATE)) {
                LOGE("LWPSTATE: Size error %d (%zu expected)", udp->recv_len, sizeof(LWPSTATE));
            }
            //int tick_diff = p->update_tick - pLwc->puck_game_state.update_tick;
            /*if (tick_diff > 0)*/
            {
                /*if (tick_diff != 1) {
                 LOGI("Packet jitter");
                 }*/
                 //memcpy(&pLwc->puck_game_state, p, sizeof(LWPSTATE));
                double last_received = lwtimepoint_now_seconds();
                double state_packet_interval = last_received - pLwc->puck_game_state_last_received;
                pLwc->puck_game_state_last_received = last_received;
                pLwc->puck_game_state_last_received_interval = state_packet_interval * 1000;
                // IGNORE LWPSTATE packet
                //queue_state(pLwc->udp, p);
                int rb_size = ringbuffer_size(&pLwc->udp->state_ring_buffer);
                if (pLwc->udp->state_count == 0) {
                    pLwc->udp->state_start_timepoint = lwtimepoint_now_seconds();
                }
                pLwc->udp->state_count++;
                double elapsed_from_start = lwtimepoint_now_seconds() - pLwc->udp->state_start_timepoint;
                LOGIx("State packet interval: %.3f ms (rb size=%d) (%.2f pps)",
                      pLwc->puck_game_state_last_received_interval,
                      rb_size,
                      (float)pLwc->udp->state_count / elapsed_from_start);
            }
            break;
        }
        default:
        {
            const unsigned char packet_type_short = *(unsigned char*)udp->buf;
            switch (packet_type_short) {
            case LPGP_LWPSTATE2:
            {
                LWPSTATE2* p = (LWPSTATE2*)udp->buf;
                if (udp->recv_len != sizeof(LWPSTATE2)) {
                    LOGE("LWPSTATE2: Size error %d (%zu expected)", udp->recv_len, sizeof(LWPSTATE));
                }
                double last_received = lwtimepoint_now_seconds();
                double state_packet_interval = last_received - pLwc->puck_game_state2_last_received;
                pLwc->puck_game_state2_last_received = last_received;
                pLwc->puck_game_state2_last_received_interval = state_packet_interval * 1000;
                queue_state2(pLwc->udp, p);
                if (pLwc->udp->state2_count == 0) {
                    pLwc->udp->state2_start_timepoint = lwtimepoint_now_seconds();
                }
                pLwc->udp->state2_count++;
                break;
            }
            default:
            {
                LOGE("Unknown datagram (UDP packet) received.");
                break;
            }
            }
            break;
        }
        }
    }
}

const char* lw_udp_addr(const LWCONTEXT* pLwc) {
    return pLwc->udp_host_addr.host;
}

unsigned long lw_udp_addr_resolved(const LWCONTEXT* pLwc) {
    return pLwc->udp_host_addr.host_resolved;
}

int lw_udp_port(const LWCONTEXT* pLwc) {
    return pLwc->udp_host_addr.port;
}

static void convert_ttl_static_state2_to_1(const LWPTTLSTATICSTATE2* s2, LWPTTLSTATICSTATE* s) {
    memset(s, 0, sizeof(LWPTTLSTATICSTATE));
    s->count = s2->count;
    s->type = LPGP_LWPTTLSTATICSTATE;
    for (int i = 0; i < s2->count; i++) {
        s->obj[i].x0 = s2->xc0 + s2->obj[i].x0;
        s->obj[i].y0 = s2->yc0 + s2->obj[i].y0;
        s->obj[i].x1 = s2->xc0 + s2->obj[i].x1;
        s->obj[i].y1 = s2->yc0 + s2->obj[i].y1;
    }
}

void udp_sea_update(LWCONTEXT* pLwc, LWUDP* udp) {
    if (pLwc->game_scene != LGS_FONT_TEST) {
        return;
    }
    if (udp->reinit_next_update) {
        destroy_udp(&pLwc->udp_sea);
        pLwc->udp_sea = new_udp();
        udp_update_addr_host(pLwc->udp_sea,
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
        lwttl_udp_send_ttlping(pLwc->ttl, udp, udp->ping_seq);
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
        
        char decompressed[1500*255]; // maximum lz4 compression ratio is 255...
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
                    memcpy(&pLwc->ttl_static_state, &pp, sizeof(LWPTTLSTATICSTATE));
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
                        sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                                cell_x_to_lng(pLwc->ttl_seaport_state.obj[i].x0),
                                cell_y_to_lat(pLwc->ttl_seaport_state.obj[i].y0));
                        htmlui_set_loop_key_value(pLwc->htmlui, "seaport", "script", script);
                    }*/
                    // fill world seaports
                    char script[128];
                    htmlui_clear_loop(pLwc->htmlui, "world-seaport");

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Yokohama");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            139.0f + 39.0f / 60.0f,
                            35.0f + 27.0f / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Shanghai");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            121.0f + 29.0f / 60.0f,
                            31.0f + 14.0f / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Tianjin");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            117.0f + 12.0f / 60.0f,
                            39.0f + 2.0f / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Busan");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            129.0f + 3.0f / 60.0f,
                            35.0f + 8.0f / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Panama Canal");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            -(79.0f + 40.0f / 60.0f),
                            9.0f + 4.0f / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "As Suways(Suez)");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            32.0f + 31.0f / 60.0f,
                            29.0f + 58.0f / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Hong Kong Central");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            114.0f + 9.0f / 60.0f,
                            22.0f + 16.0f / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Jurong/Singapore");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            103.0f + 42.0f / 60.0f,
                            1.0f + 20.0f / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Port Of South Louisiana");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            -(90 + 30 / 60.0f),
                            30 + 3 / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Pilottown");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            -(89 + 15 / 60.0f),
                            29 + 11 / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);
                    
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Sydney");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            151 + 12 / 60.0f,
                            -(33 + 51 / 60.0f));
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Honolulu");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            -(157 + 51 / 60.0f),
                            21 + 18 / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Seoul");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            126 + 56 / 60.0f,
                            37 + 31 / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Civitavecchia");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            11 + 48 / 60.0f,
                            42 + 6 / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Rome (Roma)");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            12 + 29 / 60.0f,
                            41 + 54 / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Sunderland");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            -(1 + 23 / 60.0f),
                            54 + 54 / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);

                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "name", "Dokdo");
                    sprintf(script, "script:local c = lo.script_context();lo.lwttl_worldmap_scroll_to(c.ttl, %f, %f, c.udp_sea)",
                            131 + 51 / 60.0f,
                            37 + 14 / 60.0f);
                    htmlui_set_loop_key_value(pLwc->htmlui, "world-seaport", "script", script);
                    
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
