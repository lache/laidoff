#include "platform_detection.h"
#include "lwtcp.h"

#if LW_PLATFORM_WIN32
#include <winsock2.h>
#else

#include <fcntl.h>
#include <sys/socket.h>
#include <stdlib.h>

#if !LW_PLATFORM_OSX
#include <endian.h>
#endif

#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include<arpa/inet.h> //inet_addr
#include <unistd.h> // chdir
#include <inttypes.h>

#endif

#include "lwlog.h"
#include "puckgame.h"
#include <inttypes.h>
#include <fcntl.h>
#include "puckgamepacket.h"
#include "lwtimepoint.h"
#include <tinycthread.h>
#include "pcg_basic.h"
#include "puckgamepacket.h"

#if LW_PLATFORM_WIN32
#define LwChangeDirectory(x) SetCurrentDirectory(x)
#else
#define LwChangeDirectory(x) chdir(x)
#endif

_Thread_local int thread_local_var;

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef BOOL
#define BOOL int
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (~0)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

#define BUFLEN 512  //Max length of buffer
#define PORT 10288   //The port on which to listen for incoming data

#define TCP_INTERNAL_PORT 29856
#define LW_PUCK_GAME_POOL_CAPACITY (512)

typedef struct _LWSERVER {
    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen, recv_len;
    char buf[BUFLEN];
#if LW_PLATFORM_WIN32
    WSADATA wsa;
#endif
    double last_broadcast_sent;
    double server_start_tp;
    int broadcast_count;
    int token_counter;
    int battle_counter;
    LWPUCKGAME *puck_game_pool[LW_PUCK_GAME_POOL_CAPACITY];
} LWSERVER;

typedef struct _LWTCPSERVER {
    SOCKET s;
    struct sockaddr_in server, si_other;
    int slen, recv_len;
    char buf[BUFLEN];
#if LW_PLATFORM_WIN32
    WSADATA wsa;
#endif
} LWTCPSERVER;

static BOOL directory_exists(const char *szPath) {
#if LW_PLATFORM_WIN32
    DWORD dwAttrib = GetFileAttributes(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    DIR *dir = opendir(szPath);
    if (dir) {
        return 1;
    } else {
        return 0;
    }
#endif
}

static int lw_get_normalized_dir_pad_input(const LWREMOTEPLAYERCONTROL *control, float *dx, float *dy, float *dlen) {
    if (control->dir_pad_dragging == 0) {
        return 0;
    }
    *dx = control->dx;
    *dy = control->dy;
    *dlen = control->dlen;
    return 1;
}

static int update_puck_game(LWSERVER *server, LWPUCKGAME *puck_game, double delta_time) {
    puck_game->time += (float) delta_time;
    if (puck_game->player.current_hp <= 0
        || puck_game->target.current_hp <= 0
        || puck_game->time >= puck_game->total_time) {
        return -1;
    }
    puck_game->player.puck_contacted = 0;
    puck_game->target.puck_contacted = 0;
    dSpaceCollide(puck_game->space, puck_game, puck_game_near_callback);
    //dWorldStep(puck_game->world, 0.005f);
    dWorldQuickStep(puck_game->world, 1.0f / 60);
    dJointGroupEmpty(puck_game->contact_joint_group);
    if (puck_game->player.puck_contacted == 0) {
        puck_game->player.last_contact_puck_body = 0;
    }
    if (puck_game->target.puck_contacted == 0) {
        puck_game->target.last_contact_puck_body = 0;
    }

    const dReal *p = dBodyGetPosition(puck_game->go[LPGO_PUCK].body);

    //LOGI("pos %.2f %.2f %.2f", p[0], p[1], p[2]);

    dJointID pcj[2] = {
            puck_game->player_control_joint,
            puck_game->target_control_joint,
    };
    LW_PUCK_GAME_OBJECT control_enum[2] = {
            LPGO_PLAYER,
            LPGO_TARGET,
    };
    for (int i = 0; i < 2; i++) {
        float player_speed = puck_game_player_speed(); // Should be in this scope
        float dx, dy, dlen;
        if (lw_get_normalized_dir_pad_input(&puck_game->remote_control[i], &dx, &dy, &dlen)) {
            dJointEnable(pcj[i]);
            dJointSetLMotorParam(pcj[i], dParamVel1, player_speed * dx * dlen);
            dJointSetLMotorParam(pcj[i], dParamVel2, player_speed * dy * dlen);
        } else {
            dJointSetLMotorParam(pcj[i], dParamVel1, 0);
            dJointSetLMotorParam(pcj[i], dParamVel2, 0);
        }

        // Move direction fixed while dashing
        if (puck_game->remote_dash[i].remain_time > 0) {
            player_speed *= puck_game->dash_speed_ratio;
            dx = puck_game->remote_dash[i].dir_x;
            dy = puck_game->remote_dash[i].dir_y;
            dJointSetLMotorParam(pcj[i], dParamVel1, player_speed * dx);
            dJointSetLMotorParam(pcj[i], dParamVel2, player_speed * dy);
            puck_game->remote_dash[i].remain_time = LWMAX(0,
                                                          puck_game->remote_dash[i].remain_time - (float) delta_time);
        }

        // Jump
        if (puck_game->remote_jump[i].remain_time > 0) {
            puck_game->remote_jump[i].remain_time = 0;
            dBodyAddForce(puck_game->go[control_enum[i]].body, 0, 0, puck_game->jump_force);
        }
        // Pull
        if (puck_game->remote_control[i].pull_puck) {
            const dReal *puck_pos = dBodyGetPosition(puck_game->go[LPGO_PUCK].body);
            const dReal *player_pos = dBodyGetPosition(puck_game->go[control_enum[i]].body);
            const dVector3 f = {
                    player_pos[0] - puck_pos[0],
                    player_pos[1] - puck_pos[1],
                    player_pos[2] - puck_pos[2]
            };
            const dReal flen = dLENGTH(f);
            const dReal power = 0.1f;
            const dReal scale = power / flen;
            dBodyAddForce(puck_game->go[LPGO_PUCK].body, f[0] * scale, f[1] * scale, f[2] * scale);
        }
        // Fire
        if (puck_game->remote_fire[i].remain_time > 0) {
            // [1] Player Control Joint Version
            /*dJointSetLMotorParam(pcj, dParamVel1, puck_game->fire.dir_x * puck_game->fire_max_force * puck_game->fire.dir_len);
            dJointSetLMotorParam(pcj, dParamVel2, puck_game->fire.dir_y * puck_game->fire_max_force * puck_game->fire.dir_len);
            puck_game->fire.remain_time = LWMAX(0, puck_game->fire.remain_time - (float)delta_time);*/

            // [2] Impulse Force Version
            dJointDisable(pcj[i]);
            dBodySetLinearVel(puck_game->go[control_enum[i]].body, 0, 0, 0);
            dBodyAddForce(puck_game->go[control_enum[i]].body,
                          puck_game->remote_fire[i].dir_x * puck_game->fire_max_force *
                          puck_game->remote_fire[i].dir_len,
                          puck_game->remote_fire[i].dir_y * puck_game->fire_max_force *
                          puck_game->remote_fire[i].dir_len,
                          0);
            puck_game->remote_fire[i].remain_time = 0;
        }
    }
    update_puck_ownership(puck_game);
    update_puck_reflect_size(puck_game, (float)delta_time);
    puck_game->update_tick++;
    return 0;
}

LWSERVER *new_server() {
    LWSERVER *server = malloc(sizeof(LWSERVER));
    memset(server, 0, sizeof(LWSERVER));
    server->slen = sizeof(server->si_other);
#if LW_PLATFORM_WIN32
    //Initialise winsock
    LOGI("Initialising Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &server->wsa) != 0)
    {
        printf("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    LOGI("Initialised.\n");
#endif
    //Create a socket
    if ((server->s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        LOGE("Could not create socket : %d", WSAGetLastError());
    }
    LOGI("Socket created.\n");

    //Prepare the sockaddr_in structure
    server->server.sin_family = AF_INET;
    server->server.sin_addr.s_addr = INADDR_ANY;
    server->server.sin_port = htons(PORT);

    //Bind
    if (bind(server->s, (struct sockaddr *) &server->server, sizeof(server->server)) == SOCKET_ERROR) {
        LOGE("Bind failed with error code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    LOGI("Bind done");
    return server;
}

LWTCPSERVER *new_tcp_server() {
    LWTCPSERVER *server = malloc(sizeof(LWTCPSERVER));
    memset(server, 0, sizeof(LWTCPSERVER));
    server->slen = sizeof(server->si_other);
    //Create a socket
    if ((server->s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        LOGE("Could not create tcp socket : %d", WSAGetLastError());
    }
    LOGI("TCP Socket created.\n");

    if (setsockopt(server->s, SOL_SOCKET, SO_REUSEADDR, (const char *) &(int) {1}, sizeof(int)) < 0) {
        LOGE("setsockopt(SO_REUSEADDR) failed");
    }

    //Prepare the sockaddr_in structure
    server->server.sin_family = AF_INET;
    server->server.sin_addr.s_addr = INADDR_ANY;
    server->server.sin_port = htons(TCP_INTERNAL_PORT);

    //Bind
    if (bind(server->s, (struct sockaddr *) &server->server, sizeof(server->server)) == SOCKET_ERROR) {
        LOGE("TCP Bind failed with error code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    LOGI("TCP Bind done");
    listen(server->s, 3);
    LOGI("TCP Listening...");
    return server;
}

int make_socket_nonblocking(int sock) {
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

void normalize_quaternion(float *q) {
    const float l = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    q[0] /= l;
    q[1] /= l;
    q[2] /= l;
    q[3] /= l;
}

void server_send(LWSERVER *server, const char *p, int s) {
    sendto(server->s, p, s, 0, (struct sockaddr *) &server->si_other, server->slen);
}

#define SERVER_SEND(server, packet) server_send(server, (const char*)&packet, sizeof(packet))

typedef struct _LWCONN {
    unsigned long long ipport;
    struct sockaddr_in si;
    double last_ingress_timepoint;
    int battle_id;
    unsigned int token;
    int player_no;
} LWCONN;

#define LW_CONN_CAPACITY (32)

unsigned long long to_ipport(unsigned int ip, unsigned short port) {
    return ((unsigned long long) port << 32) | ip;
}

void add_conn(LWCONN *conn, int conn_capacity, struct sockaddr_in *si) {
    unsigned long long ipport = to_ipport(si->sin_addr.s_addr, si->sin_port);
    // Update last ingress for existing element
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport == ipport) {
            conn[i].last_ingress_timepoint = lwtimepoint_now_seconds();
            return;
        }
    }
    // New element
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport == 0) {
            conn[i].ipport = ipport;
            conn[i].last_ingress_timepoint = lwtimepoint_now_seconds();
            memcpy(&conn[i].si, si, sizeof(struct sockaddr_in));
            return;
        }
    }
    LOGE("add_conn: maximum capacity exceeded.");
}

void add_conn_with_token(LWCONN *conn, int conn_capacity, struct sockaddr_in *si, int battle_id, unsigned int token,
                         int player_no) {
    unsigned long long ipport = to_ipport(si->sin_addr.s_addr, si->sin_port);
    // Update last ingress for existing element
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport == ipport) {
            conn[i].last_ingress_timepoint = lwtimepoint_now_seconds();
            conn[i].battle_id = battle_id;
            conn[i].token = token;
            conn[i].player_no = player_no;
            return;
        }
    }
    LOGE("add_conn_with_token: not exist...");
}

void invalidate_dead_conn(LWCONN *conn, int conn_capacity, double current_timepoint, double life) {
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport) {
            if (current_timepoint - conn[i].last_ingress_timepoint > life) {
                conn[i].ipport = 0;
                conn[i].battle_id = 0;
            }
        }
    }
}

void broadcast_packet(LWSERVER *server, const LWCONN *conn, int conn_capacity, const char *p, int s) {
    int sent = 0;
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport) {
            double tp = lwtimepoint_now_seconds();
            sendto(server->s, p, s, 0, (struct sockaddr *) &conn[i].si, server->slen);
            double elapsed = lwtimepoint_now_seconds() - tp;
            //LOGI("Broadcast sendto elapsed: %.3f ms", elapsed * 1000);
            sent = 1;
        }
    }
    if (sent) {
        server->broadcast_count++;
        double tp = lwtimepoint_now_seconds();
        double server_start_elapsed = tp - server->server_start_tp;
        /*LOGI("Broadcast interval: %.3f ms (%.2f pps)",
            (tp - server->last_broadcast_sent) * 1000,
            server->broadcast_count / server_start_elapsed);*/
        server->last_broadcast_sent = tp;
    }
}

void on_player_damaged(LWPUCKGAME *puck_game) {
    puck_game_go_decrease_hp_test(puck_game, &puck_game->player, &puck_game->remote_dash[0], &puck_game->tower[0]);
}

void on_target_damaged(LWPUCKGAME *puck_game) {
    puck_game_go_decrease_hp_test(puck_game, &puck_game->target, &puck_game->remote_dash[1], &puck_game->tower[1]);
}

void select_tcp_server(LWTCPSERVER *server) {
    memset(server->buf, '\0', BUFLEN);
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server->s, &readfds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 2500;
    int rv = select(server->s + 1, &readfds, NULL, NULL, &tv);
    //try to receive some data, this is a blocking call
    if (rv == 1) {
        if ((server->recv_len = recvfrom(server->s, server->buf, BUFLEN, 0, (struct sockaddr *) &server->si_other,
                                         &server->slen)) == SOCKET_ERROR) {
            //LOGE("recvfrom() failed with error code : %d", WSAGetLastError());
            //exit(EXIT_FAILURE);
        } else {
            const int packet_type = *(int *) server->buf;
            switch (packet_type) {
                case LPGP_LWPCREATEBATTLE: {
                    LOGI("LSBPT_LWPCREATEBATTLE received");
                    break;
                }
                default: {
                    LOGE("UNKNOWN INTERNAL PACKET RECEIVED");
                    break;
                }
            }
        }


    } else {
        //LOGI("EMPTY");
    }
}

int check_token(LWSERVER *server, LWPUDPHEADER *p, LWPUCKGAME **puck_game) {
    if (p->battle_id<1 || p->battle_id>LW_PUCK_GAME_POOL_CAPACITY) {
        if (p->battle_id != 0) {
            LOGE("Invalid battle id range: %d", p->battle_id);
        } else {
            // p->battle_id == 0 everytime client runs
        }
        return -1;
    }
    LWPUCKGAME *pg = server->puck_game_pool[p->battle_id - 1];
    if (!pg) {
        //LOGE("Battle id %d is null.", p->battle_id);
        return -2;
    }
    if (pg->c1_token == p->token) {
        *puck_game = pg;
        return 1;
    } else if (pg->c2_token == p->token) {
        *puck_game = pg;
        return 2;
    } else {
        LOGE("Battle id %d token not match.", p->battle_id);
        return -3;
    }
    return -4;
}

int check_player_no(int player_no) {
    return player_no == 1 || player_no == 2;
}

int control_index_from_player_no(int player_no) {
    return player_no - 1;
}

void select_server(LWSERVER *server, LWPUCKGAME *puck_game, LWCONN *conn, int conn_capacity) {
    //clear the buffer by filling null, it might have previously received data
    memset(server->buf, '\0', BUFLEN);
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(server->s, &readfds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 2500;
    int rv = select(server->s + 1, &readfds, NULL, NULL, &tv);
    //printf("rv");
    //try to receive some data, this is a blocking call
    if (rv == 1) {
        if ((server->recv_len = recvfrom(server->s, server->buf, BUFLEN, 0, (struct sockaddr *) &server->si_other,
                                         &server->slen)) == SOCKET_ERROR) {
            //printf("recvfrom() failed with error code : %d", WSAGetLastError());
            //exit(EXIT_FAILURE);
        } else {
            add_conn(conn, LW_CONN_CAPACITY, &server->si_other);

            //print details of the client/peer and the data received
            /*printf("Received packet from %s:%d (size:%d)\n",
            inet_ntoa(server->si_other.sin_addr),
            ntohs(server->si_other.sin_port),
            server->recv_len);*/

            const int packet_type = *(int *) server->buf;
            switch (packet_type) {
                case LPGP_LWPGETTOKEN: {
                    LWPTOKEN p;
                    p.type = LPGP_LWPTOKEN;
                    ++server->token_counter;
                    p.token = server->token_counter;
                    SERVER_SEND(server, p);
                    break;
                }
                case LPGP_LWPQUEUE: {
                    LWPMATCHED p;
                    p.type = LPGP_LWPMATCHED;
                    p.master = 0;
                    SERVER_SEND(server, p);
                    break;
                }
                case LPGP_LWPMOVE: {
                    LWPMOVE *p = (LWPMOVE *) server->buf;
                    LWPUCKGAME *pg = 0;
                    int player_no = check_token(server, (LWPUDPHEADER *) p, &pg);
                    if (check_player_no(player_no)) {
                        //LOGI("MOVE dx=%.2f dy=%.2f", p->dx, p->dy);
                        LWREMOTEPLAYERCONTROL *control = &pg->remote_control[control_index_from_player_no(player_no)];
                        control->dir_pad_dragging = 1;
                        control->dx = p->dx;
                        control->dy = p->dy;
                        control->dlen = p->dlen;
                        add_conn_with_token(conn, LW_CONN_CAPACITY, &server->si_other, p->battle_id, p->token,
                                            player_no);
                    }
                    break;
                }
                case LPGP_LWPSTOP: {
                    LWPSTOP *p = (LWPSTOP *) server->buf;
                    LWPUCKGAME *pg = 0;
                    int player_no = check_token(server, (LWPUDPHEADER *) p, &pg);
                    if (check_player_no(player_no)) {
                        //LOGI("STOP");
                        LWREMOTEPLAYERCONTROL *control = &pg->remote_control[control_index_from_player_no(player_no)];
                        control->dir_pad_dragging = 0;
                        add_conn_with_token(conn, LW_CONN_CAPACITY, &server->si_other, p->battle_id, p->token,
                                            player_no);
                    }
                    break;
                }
                case LPGP_LWPDASH: {
                    LWPDASH *p = (LWPDASH *) server->buf;
                    LWPUCKGAME *pg = 0;
                    int player_no = check_token(server, (LWPUDPHEADER *) p, &pg);
                    if (check_player_no(player_no)) {
                        //LOGI("DASH");
                        LWPUCKGAMEDASH *dash = &pg->remote_dash[control_index_from_player_no(player_no)];
                        puck_game_commit_dash_to_puck(pg, dash, player_no);
                        add_conn_with_token(conn, LW_CONN_CAPACITY, &server->si_other, p->battle_id, p->token,
                                            player_no);
                    }
                    break;
                }
                case LPGP_LWPJUMP: {
                    LWPJUMP *p = (LWPJUMP *) server->buf;
                    LWPUCKGAME *pg = 0;
                    int player_no = check_token(server, (LWPUDPHEADER *) p, &pg);
                    if (check_player_no(player_no)) {
                        //LOGI("JUMP");
                        LWPUCKGAMEJUMP *jump = &pg->remote_jump[control_index_from_player_no(player_no)];
                        puck_game_commit_jump(pg, jump, player_no);
                        add_conn_with_token(conn, LW_CONN_CAPACITY, &server->si_other, p->battle_id, p->token,
                                            player_no);
                    }
                    break;
                }
                case LPGP_LWPFIRE: {
                    LWPFIRE *p = (LWPFIRE *) server->buf;
                    LWPUCKGAME *pg = 0;
                    int player_no = check_token(server, (LWPUDPHEADER *) p, &pg);
                    if (check_player_no(player_no)) {
                        //LOGI("FIRE");
                        LWPUCKGAMEFIRE *fire = &pg->remote_fire[control_index_from_player_no(player_no)];
                        puck_game_commit_fire(pg, fire, player_no, p->dx, p->dy, p->dlen);
                        add_conn_with_token(conn, LW_CONN_CAPACITY, &server->si_other, p->battle_id, p->token,
                                            player_no);
                    }
                    break;
                }
                case LPGP_LWPPULLSTART: {
                    LWPPULLSTART *p = (LWPPULLSTART *) server->buf;
                    LWPUCKGAME *pg = 0;
                    int player_no = check_token(server, (LWPUDPHEADER *) p, &pg);
                    if (check_player_no(player_no)) {
                        //LOGI("PULL START");
                        LWREMOTEPLAYERCONTROL *control = &pg->remote_control[control_index_from_player_no(player_no)];
                        control->pull_puck = 1;
                        add_conn_with_token(conn, LW_CONN_CAPACITY, &server->si_other, p->battle_id, p->token,
                                            player_no);
                    }
                    break;
                }
                case LPGP_LWPPULLSTOP: {
                    LWPPULLSTOP *p = (LWPPULLSTOP *) server->buf;
                    LWPUCKGAME *pg = 0;
                    int player_no = check_token(server, (LWPUDPHEADER *) p, &pg);
                    if (check_player_no(player_no)) {
                        //LOGI("PULL STOP");
                        LWREMOTEPLAYERCONTROL *control = &pg->remote_control[control_index_from_player_no(player_no)];
                        control->pull_puck = 0;
                        add_conn_with_token(conn, LW_CONN_CAPACITY, &server->si_other, p->battle_id, p->token,
                                            player_no);
                    }
                    break;
                }
                default: {
                    break;
                }
            }
        }


    } else {
        //LOGI("EMPTY");
    }
}

int tcp_server_entry(void *context) {
    LWTCPSERVER *tcp_server = new_tcp_server();
    LWSERVER *server = context;
    while (1) {
        int c = sizeof(struct sockaddr_in);
        SOCKET client_sock = accept(tcp_server->s, (struct sockaddr *) &tcp_server->server, &c);
        char recv_buf[512];
        int recv_len = recv(client_sock, recv_buf, 512, 0);
        LOGI("Admin TCP recv len: %d", recv_len);
        LWPBASE *base = (LWPBASE *) recv_buf;
        if (base->type == LPGP_LWPCREATEBATTLE && base->size == sizeof(LWPCREATEBATTLE)) {
            LWPCREATEBATTLEOK reply_p;
            LOGI("LWPCREATEBATTLE: Create a new puck game instance");
            LWPCREATEBATTLE *p = (LWPCREATEBATTLE *) base;
            LWPUCKGAME *puck_game = new_puck_game();
            memcpy(puck_game->id1, p->Id1, sizeof(puck_game->id1));
            memcpy(puck_game->id2, p->Id2, sizeof(puck_game->id2));
            memcpy(puck_game->nickname, p->Nickname1, sizeof(puck_game->nickname));
            memcpy(puck_game->target_nickname, p->Nickname2, sizeof(puck_game->target_nickname));
            const int battle_id = server->battle_counter + 1; // battle id is 1-based index
            puck_game->server = server;
            puck_game->battle_id = battle_id;
            //puck_game->on_player_damaged = on_player_damaged;
            //puck_game->on_target_damaged = on_target_damaged;
            puck_game->c1_token = pcg32_random();
            puck_game->c2_token = pcg32_random();
            // Build reply packet
            reply_p.Type = LPGP_LWPCREATEBATTLEOK;
            reply_p.Size = sizeof(LWPCREATEBATTLEOK);
            reply_p.Battle_id = battle_id;
            reply_p.C1_token = puck_game->c1_token;
            reply_p.C2_token = puck_game->c2_token;
            // IpAddr, Port field is ignored for now...
            reply_p.IpAddr[0] = 192;
            reply_p.IpAddr[1] = 168;
            reply_p.IpAddr[2] = 0;
            reply_p.IpAddr[3] = 28;
            reply_p.Port = 10288;
            server->puck_game_pool[server->battle_counter] = puck_game;
            // send reply to client
            send(client_sock, (const char *) &reply_p, sizeof(LWPCREATEBATTLEOK), 0);
            // Increment and wrap battle counter (XXX)
            server->battle_counter++;
            if (server->battle_counter >= LW_PUCK_GAME_POOL_CAPACITY) {
                server->battle_counter = 0;
                if (server->puck_game_pool[server->battle_counter]) {
                    delete_puck_game(&server->puck_game_pool[server->battle_counter]);
                }
            }
        } else if (base->type == LPGP_LWPSUDDENDEATH && base->size == sizeof(LWPSUDDENDEATH)) {
            LOGI("LWPSUDDENDEATH!");
            LWPSUDDENDEATH *p = (LWPSUDDENDEATH *) base;
            if (p->Battle_id<1 || p->Battle_id>LW_PUCK_GAME_POOL_CAPACITY) {

            } else {
                LWPUCKGAME *pg = server->puck_game_pool[p->Battle_id - 1];
                if (pg->c1_token == p->Token || pg->c2_token == p->Token) {
                    pg->player.current_hp = 1;
                    pg->target.current_hp = 1;
                }
            }

        } else {
            LOGE("Admin TCP unexpected packet");
        }
    }
    return 0;
}

void broadcast_state_packet(LWSERVER *server, const LWCONN *conn, int conn_capacity) {
    int sent = 0;
    for (int i = 0; i < conn_capacity; i++) {
        if (conn[i].ipport
            && check_player_no(conn[i].player_no)
            && conn[i].battle_id >= 1
            && conn[i].battle_id <= LW_PUCK_GAME_POOL_CAPACITY) {
            const LWPUCKGAME *puck_game = server->puck_game_pool[conn[i].battle_id - 1];
            if (puck_game) {
                float xscale = 1.0f;
                float yscale = 1.0f;
                LW_PUCK_GAME_OBJECT player_go_enum = LPGO_PLAYER;
                LW_PUCK_GAME_OBJECT target_go_enum = LPGO_TARGET;
                const LWPUCKGAMEPLAYER *player = &puck_game->player;
                const LWPUCKGAMEPLAYER *target = &puck_game->target;
                if (conn[i].player_no == 2) {
                    /*player_go_enum = LPGO_TARGET;
                    target_go_enum = LPGO_PLAYER;
                    xscale = -1.0f;
                    yscale = -1.0f;*/
                    player = &puck_game->target;
                    target = &puck_game->player;
                }

                LWPSTATE packet_state;
                packet_state.type = LPGP_LWPSTATE;
                packet_state.update_tick = puck_game->update_tick;
                packet_state.puck[0] = xscale * puck_game->go[LPGO_PUCK].pos[0];
                packet_state.puck[1] = yscale * puck_game->go[LPGO_PUCK].pos[1];
                packet_state.puck[2] = puck_game->go[LPGO_PUCK].pos[2];
                packet_state.player[0] = xscale * puck_game->go[player_go_enum].pos[0];
                packet_state.player[1] = yscale * puck_game->go[player_go_enum].pos[1];
                packet_state.player[2] = puck_game->go[player_go_enum].pos[2];
                packet_state.target[0] = xscale * puck_game->go[target_go_enum].pos[0];
                packet_state.target[1] = yscale * puck_game->go[target_go_enum].pos[1];
                packet_state.target[2] = puck_game->go[target_go_enum].pos[2];
                memcpy(packet_state.puck_rot, puck_game->go[LPGO_PUCK].rot, sizeof(mat4x4));
                memcpy(packet_state.player_rot, puck_game->go[player_go_enum].rot, sizeof(mat4x4));
                memcpy(packet_state.target_rot, puck_game->go[target_go_enum].rot, sizeof(mat4x4));
                packet_state.puck_speed = puck_game->go[LPGO_PUCK].speed;
                packet_state.player_speed = puck_game->go[player_go_enum].speed;
                packet_state.target_speed = puck_game->go[target_go_enum].speed;
                packet_state.puck_move_rad = puck_game->go[LPGO_PUCK].move_rad;
                packet_state.player_move_rad = puck_game->go[player_go_enum].move_rad;
                packet_state.target_move_rad = puck_game->go[target_go_enum].move_rad;
                packet_state.player_current_hp = player->current_hp;
                packet_state.player_total_hp = player->total_hp;
                packet_state.target_current_hp = target->current_hp;
                packet_state.target_total_hp = target->total_hp;
                packet_state.puck_owner_player_no = puck_game->puck_owner_player_no;
                packet_state.puck_reflect_size = puck_game->puck_reflect_size;
                packet_state.finished = puck_game->finished;
                double tp = lwtimepoint_now_seconds();
                sendto(server->s, (const char *) &packet_state, sizeof(packet_state), 0,
                       (struct sockaddr *) &conn[i].si, server->slen);
                double elapsed = lwtimepoint_now_seconds() - tp;
                //LOGI("Broadcast sendto elapsed: %.3f ms", elapsed * 1000);
                sent = 1;
            }
        }
    }
    if (sent) {
        server->broadcast_count++;
        double tp = lwtimepoint_now_seconds();
        double server_start_elapsed = tp - server->server_start_tp;
        /*LOGI("Broadcast interval: %.3f ms (%.2f pps)",
        (tp - server->last_broadcast_sent) * 1000,
        server->broadcast_count / server_start_elapsed);*/
        server->last_broadcast_sent = tp;
    }
}

static int puck_game_winner(const LWPUCKGAME *puck_game) {
    if (puck_game->player.current_hp > 0
        && puck_game->target.current_hp > 0
        && puck_game->time < puck_game->total_time) {
        // not yet finished
        return -1;
    }
    const int hp_diff = puck_game->player.current_hp - puck_game->target.current_hp;
    if (hp_diff == 0) {
        return 0;
    } else if (hp_diff > 0) {
        return 1;
    } else {
        return 2;
    }
}

int tcp_send_battle_result(LWTCP *tcp,
                           int backoffMs,
                           const unsigned int *id1,
                           const unsigned int *id2,
                           const char *nickname1,
                           const char *nickname2,
                           int winner) {
    if (tcp == 0) {
        LOGE("tcp null");
        return -1;
    }
    NEW_TCP_PACKET_CAPITAL(LWPBATTLERESULT, p);
    memcpy(p.Id1, id1, sizeof(p.Id1));
    memcpy(p.Id2, id2, sizeof(p.Id2));
    p.Winner = winner;
    memcpy(p.Nickname1, nickname1, sizeof(p.Nickname1));
    memcpy(p.Nickname2, nickname2, sizeof(p.Nickname2));
    memcpy(tcp->sendbuf, &p, sizeof(p));
    int send_result = (int) send(tcp->ConnectSocket, tcp->sendbuf, sizeof(p), 0);
    if (send_result < 0) {
        LOGI("Send result error: %d", send_result);
        if (backoffMs > 10 * 1000 /* 10 seconds */) {
            LOGE("tcp_send_push_token: failed");
            return -1;
        } else if (backoffMs > 0) {
            struct timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = backoffMs * 1000 * 1000;
            thrd_sleep(&ts, 0);
        }
        tcp_connect(tcp);
        return tcp_send_battle_result(tcp,
                                      backoffMs * 2,
                                      id1,
                                      id2,
                                      nickname1,
                                      nickname2,
                                      winner);
    }
    return send_result;
}

void process_battle_reward(LWPUCKGAME *puck_game, LWTCP *reward_service) {
    tcp_send_battle_result(reward_service,
                           300,
                           puck_game->id1,
                           puck_game->id2,
                           puck_game->nickname,
                           puck_game->target_nickname,
                           puck_game_winner(puck_game));
}

void reward_service_on_connect(LWTCP *tcp, const char *path_prefix) {
    LOGI("Reward service connected.");
}

int reward_service_on_recv_packets(LWTCP *tcp) {
    LOGI("Packet received (%d bytes) from reward service.", tcp->recvbufnotparsed);
    return tcp->recvbufnotparsed;
}

int main(int argc, char *argv[]) {
    LOGI("LAIDOFF-SERVER: Greetings.");
    while (!directory_exists("assets") && LwChangeDirectory("..")) {
    }
    // Create main server instance
    LWSERVER *server = new_server();
    // TCP listen & reply thread (listen requests from match server)
    thrd_t thr;
    thrd_create(&thr, tcp_server_entry, server);
    // Create reward server connection
    LWHOSTADDR reward_host_addr;
    strcpy(reward_host_addr.host, "127.0.0.1");
    strcpy(reward_host_addr.port_str, "10290");
    LWTCP *reward_service = new_tcp(0,
                                    0,
                                    &reward_host_addr,
                                    reward_service_on_connect,
                                    reward_service_on_recv_packets);
    // Create a test puck game instance (not used for battle)
    LWPUCKGAME *puck_game = new_puck_game();
    puck_game->server = server;
    puck_game->on_player_damaged = on_player_damaged;
    puck_game->on_target_damaged = on_target_damaged;
    const int logic_hz = 125;
    const double logic_timestep = 1.0 / logic_hz;
    const int rendering_hz = 60;
    const double sim_timestep = 0.02; // 1.0f / rendering_hz; // sec
    const double sync_timestep = 1.0 / 70;
    //struct timeval tv;
    //tv.tv_sec = 10;
    //tv.tv_usec = 8000;// sim_timestep * 1000 * 1000;
    //make_socket_nonblocking(server->s);
    LWCONN conn[LW_CONN_CAPACITY];
    double logic_elapsed_ms = 0;
    double sync_elapsed_ms = 0;
    server->server_start_tp = lwtimepoint_now_seconds();
    int forever = 1;
    while (forever) {
        const double loop_start = lwtimepoint_now_seconds();
        if (logic_elapsed_ms > 0) {
            int iter = (int) (logic_elapsed_ms / (logic_timestep * 1000));
            for (int i = 0; i < iter; i++) {
                //update_puck_game(server, puck_game, logic_timestep);
                for (int j = 0; j < LW_PUCK_GAME_POOL_CAPACITY; j++) {
                    if (server->puck_game_pool[j]
                        && server->puck_game_pool[j]->init_ready) {
                        if (server->puck_game_pool[j]->finished == 0
                            && update_puck_game(server, server->puck_game_pool[j], logic_timestep) < 0) {
                            server->puck_game_pool[j]->finished = 1;
                            process_battle_reward(server->puck_game_pool[j], reward_service);
                        }
                    }
                }
            }
            logic_elapsed_ms = fmod(logic_elapsed_ms, (logic_timestep * 1000));
        }
        if (sync_elapsed_ms > sync_timestep * 1000) {
            sync_elapsed_ms = fmod(sync_elapsed_ms, (sync_timestep * 1000));
            // Broadcast state to clients
            broadcast_state_packet(server, conn, LW_CONN_CAPACITY);
        }


        //LOGI("Update tick %"PRId64, update_tick);

        invalidate_dead_conn(conn, LW_CONN_CAPACITY, lwtimepoint_now_seconds(), 2.0);

        //LOGI("Waiting for data...");
        fflush(stdout);

        select_server(server, puck_game, conn, LW_CONN_CAPACITY);

        double loop_time = lwtimepoint_now_seconds() - loop_start;
        logic_elapsed_ms += loop_time * 1000;
        sync_elapsed_ms += loop_time * 1000;
        //LOGI("Loop time: %.3f ms", loop_time * 1000);
    }
    delete_puck_game(&puck_game);
    destroy_tcp(&reward_service);
    return 0;
}
