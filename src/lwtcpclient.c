#include "lwtcpclient.h"
#include <string.h>
#include "lwtcp.h"
#include "puckgamepacket.h"
#include "lwlog.h"
#include "lwcontext.h"
#include "puckgame.h"
#include "file.h"
#include "sysmsg.h"
#include "puckgame.h"
#include "lwudp.h"
#include "puckgameupdate.h"
#include "logic.h"

void tcp_on_connect(LWTCP* tcp, const char* path_prefix) {
    if (get_cached_user_id(path_prefix, &tcp->user_id) == 0) {
        LOGI("Cached user id: %08x-%08x-%08x-%08x",
             tcp->user_id.v[0], tcp->user_id.v[1], tcp->user_id.v[2], tcp->user_id.v[3]);
        tcp_send_querynick(tcp, &tcp->user_id);
    } else {
        // Request a new user to be created
        tcp_send_newuser(tcp);
    }
    request_player_reveal_leaderboard(tcp);
}

int tcp_send_sendbuf(LWTCP* tcp, int s) {
    int send_result = (int)send(tcp->ConnectSocket, tcp->sendbuf, s, 0);
    if (send_result < 0) {
        tcp->send_fail = 1;
    }
    return send_result;
}

int tcp_send_newuser(LWTCP* tcp) {
    LOGI("Sending LWPNEWUSER");
    NEW_TCP_PACKET(LWPNEWUSER, p);
    memcpy(tcp->sendbuf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_querynick(LWTCP* tcp, const LWUNIQUEID* id) {
    LOGI("Sending LWPQUERYNICK");
    NEW_TCP_PACKET_CAPITAL(LWPQUERYNICK, p);
    memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
    memcpy(tcp->sendbuf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_queue2(LWTCP* tcp, const LWUNIQUEID* id) {
    LOGI("Sending LWPQUEUE2");
    NEW_TCP_PACKET_CAPITAL(LWPQUEUE2, p);
    memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
    memcpy(tcp->sendbuf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_queue3(LWTCP* tcp, const LWUNIQUEID* id, int queue_type) {
    LOGI("Sending LWPQUEUE3");
    NEW_TCP_PACKET_CAPITAL(LWPQUEUE3, p);
    memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
    p.QueueType = queue_type;
    memcpy(tcp->sendbuf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_cancelqueue(LWTCP* tcp, const LWUNIQUEID* id) {
    LOGI("Sending LWPCANCELQUEUE");
    NEW_TCP_PACKET_CAPITAL(LWPCANCELQUEUE, p);
    memcpy(p.Id, id->v, sizeof(LWUNIQUEID));
    memcpy(tcp->sendbuf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_suddendeath(LWTCP* tcp, int battle_id, unsigned int token) {
    LOGI("Sending LWPSUDDENDEATH");
    NEW_TCP_PACKET_CAPITAL(LWPSUDDENDEATH, p);
    p.Battle_id = battle_id;
    p.Token = token;
    memcpy(tcp->sendbuf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int tcp_send_get_leaderboard_reveal_player(LWTCP* tcp, int backoffMs, const LWUNIQUEID* user_id, int count) {
    LOGI("Sending LWPGETLEADERBOARDREVEALPLAYER");
    if (tcp == 0) {
        LOGE("tcp null");
        return -1;
    }
    NEW_TCP_PACKET_CAPITAL(LWPGETLEADERBOARDREVEALPLAYER, p);
    memcpy(p.Id, user_id, sizeof(p.Id));
    p.Count = count;
    memcpy(tcp->sendbuf, &p, sizeof(p));
    tcp->on_leaderboard_packet = 0; // do nothing when reply packet received
    int send_result = (int)send(tcp->ConnectSocket, tcp->sendbuf, sizeof(p), 0);
    if (send_result < 0) {
        LOGI("Send result error: %d", send_result);
        if (backoffMs > 10 * 1000 /* 10 seconds */) {
            LOGE(LWLOGPOS "failed");
            return -1;
        } else if (backoffMs > 0) {
#if LW_PLATFORM_WIN32
            Sleep(backoffMs);
#else
            usleep(backoffMs * 1000);
#endif
        }
        tcp_connect(tcp);
        return tcp_send_get_leaderboard_reveal_player(tcp, backoffMs * 2, user_id, count);
    }
    return send_result;
}

int tcp_send_get_leaderboard(LWTCP* tcp, int backoffMs, int start_index, int count, void (*on_leaderboard_packet)(LWCONTEXT* pLwc)) {
    LOGI("Sending LWPGETLEADERBOARD");
    if (tcp == 0) {
        LOGE("tcp null");
        return -1;
    }
    NEW_TCP_PACKET_CAPITAL(LWPGETLEADERBOARD, p);
    memcpy(p.Id, &tcp->user_id, sizeof(p.Id));
    p.Start_index = start_index;
    p.Count = count;
    memcpy(tcp->sendbuf, &p, sizeof(p));
    tcp->on_leaderboard_packet = on_leaderboard_packet;
    int send_result = (int)send(tcp->ConnectSocket, tcp->sendbuf, sizeof(p), 0);
    if (send_result < 0) {
        LOGI("Send result error: %d", send_result);
        if (backoffMs > 10 * 1000 /* 10 seconds */) {
            LOGE(LWLOGPOS "failed");
            return -1;
        } else if (backoffMs > 0) {
#if LW_PLATFORM_WIN32
            Sleep(backoffMs);
#else
            usleep(backoffMs * 1000);
#endif
        }
        tcp_connect(tcp);
        return tcp_send_get_leaderboard(tcp, backoffMs * 2, start_index, count, on_leaderboard_packet);
    }
    return send_result;
}

int tcp_send_push_token(LWTCP* tcp, int backoffMs, int domain, const char* push_token) {
    LOGI("Sending LWPPUSHTOKEN");
    if (tcp == 0) {
        LOGE("tcp null");
        return -1;
    }
    NEW_TCP_PACKET_CAPITAL(LWPPUSHTOKEN, p);
    p.Domain = domain;
    strncpy(p.Push_token, push_token, sizeof(p.Push_token) - 1);
    memcpy(p.Id, tcp->user_id.v, sizeof(p.Id));
    p.Push_token[sizeof(p.Push_token) - 1] = '\0';
    memcpy(tcp->sendbuf, &p, sizeof(p));
    int send_result = (int)send(tcp->ConnectSocket, tcp->sendbuf, sizeof(p), 0);
    if (send_result < 0) {
        LOGI("Send result error: %d", send_result);
        if (backoffMs > 10 * 1000 /* 10 seconds */) {
            LOGE("tcp_send_push_token: failed");
            return -1;
        } else if (backoffMs > 0) {
#if LW_PLATFORM_WIN32
            Sleep(backoffMs);
#else
            usleep(backoffMs * 1000);
#endif
        }
        tcp_connect(tcp);
        return tcp_send_push_token(tcp, backoffMs * 2, domain, push_token);
    }
    return send_result;
}

int tcp_send_setnickname(LWTCP* tcp, const LWUNIQUEID* id, const char* nickname) {
    LOGI("Sending LWPSETNICKNAME");
    NEW_TCP_PACKET_CAPITAL(LWPSETNICKNAME, p);
    memcpy(p.Id, id->v, sizeof(p.Id));
    memcpy(p.Nickname, nickname, sizeof(p.Nickname));
    memcpy(tcp->sendbuf, &p, sizeof(p));
    return tcp_send_sendbuf(tcp, sizeof(p));
}

int parse_recv_packets(LWTCP* tcp) {
    LWCONTEXT* pLwc = tcp->pLwc;
    // too small for parsing
    if (tcp->recvbufnotparsed < 2) {
        return -1;
    }
    int parsed_bytes = 0;
    char* cursor = tcp->recvbuf;
    while (1) {
        unsigned short packet_size = *(unsigned short*)(cursor + 0);
        // still incomplete packet
        if (packet_size == 0 || packet_size > tcp->recvbufnotparsed - parsed_bytes) {
            return parsed_bytes;
        }
        unsigned short packet_type = *(unsigned short*)(cursor + 2);
        if (CHECK_PACKET(packet_type, packet_size, LWPMATCHED2)) {
            LWPMATCHED2* p = (LWPMATCHED2*)cursor;
            LOGI("LWPMATCHED2 - bid:%d", p->battle_id);
            pLwc->puck_game->world_roll_dir *= -1;
            puck_game_roll_to_battle(pLwc->puck_game);
            pLwc->puck_game->battle_id = p->battle_id;
            pLwc->puck_game->token = p->token;
            pLwc->puck_game->player_no = p->player_no;
            pLwc->tcp->state = LUS_MATCHED;
            // create UDP socket for battle sync
            pLwc->udp = new_udp();
            // make a copy of target nickname
            memcpy(pLwc->puck_game->target_nickname, p->target_nickname, sizeof(p->target_nickname));
            pLwc->puck_game->target_score = p->target_score;
            pLwc->udp_host_addr.host_resolved = *(unsigned long*)p->ipaddr;
            sprintf(pLwc->udp_host_addr.host, "%d.%d.%d.%d",
                    ((int)pLwc->udp_host_addr.host_resolved >> 0) & 0xff,
                    ((int)pLwc->udp_host_addr.host_resolved >> 8) & 0xff,
                    ((int)pLwc->udp_host_addr.host_resolved >> 16) & 0xff,
                    ((int)pLwc->udp_host_addr.host_resolved >> 24) & 0xff);
            pLwc->udp_host_addr.port = p->port;
            udp_update_addr(pLwc->udp,
                            pLwc->udp_host_addr.host_resolved,
                            pLwc->udp_host_addr.port);
            memcpy(&pLwc->puck_game->matched2, p, sizeof(pLwc->puck_game->matched2));
            // Since player_no updated
            puck_game_reset_view_proj(pLwc, pLwc->puck_game);
            //show_sys_msg(pLwc->def_sys_msg, "LWPMATCHED2 received");
        } else if (CHECK_PACKET(packet_type, packet_size, LWPQUEUEOK)) {
            LOGI("LWPQUEUEOK received");
            //show_sys_msg(pLwc->def_sys_msg, "LWPQUEUEOK received");
        } else if (CHECK_PACKET(packet_type, packet_size, LWPCANCELQUEUEOK)) {
            LOGI("LWPCANCELQUEUE received");
            show_sys_msg(pLwc->def_sys_msg, "Searching aborted");
        } else if (CHECK_PACKET(packet_type, packet_size, LWPRETRYQUEUE)) {
            LOGI("LWPRETRYQUEUE received");
            //show_sys_msg(pLwc->def_sys_msg, "LWPRETRYQUEUE received");
            // Resend QUEUE2
            tcp_send_queue2(tcp, &pLwc->tcp->user_id);
        } else if (CHECK_PACKET(packet_type, packet_size, LWPRETRYQUEUE2)) {
            LOGI("LWPRETRYQUEUE2 received");
            LWPRETRYQUEUE2* p = (LWPRETRYQUEUE2*)cursor;
            // Resend QUEUE3
            tcp_send_queue3(tcp, &pLwc->tcp->user_id, p->queueType);
        } else if (CHECK_PACKET(packet_type, packet_size, LWPRETRYQUEUELATER)) {
            LOGI("LWPRETRYQUEUELATER received");
            // match server internal error...
            // queue again only if user touches retry button
            // simulate 'battle finished' state
            show_sys_msg(pLwc->def_sys_msg, "server internal error");
            pLwc->puck_game->battle_id = 99999;
            pLwc->puck_game->token = 99999;
            pLwc->puck_game_state.bf.phase = LSP_FINISHED_DRAW;
        } else if (CHECK_PACKET(packet_type, packet_size, LWPMAYBEMATCHED)) {
            LOGI("LWPMAYBEMATCHED received");
            //show_sys_msg(pLwc->def_sys_msg, "LWPMAYBEMATCHED received");
        } else if (CHECK_PACKET(packet_type, packet_size, LWPNEWUSERDATA)) {
            LOGI("LWPNEWUSERDATA received");
            LWPNEWUSERDATA* p = (LWPNEWUSERDATA*)cursor;
            save_cached_user_id(pLwc->user_data_path, (LWUNIQUEID*)p->id);
            get_cached_user_id(pLwc->user_data_path, &pLwc->tcp->user_id);
            LOGI("[NEW] Cached user nick: %s, id: %08x-%08x-%08x-%08x",
                 p->nickname,
                 pLwc->tcp->user_id.v[0],
                 pLwc->tcp->user_id.v[1],
                 pLwc->tcp->user_id.v[2],
                 pLwc->tcp->user_id.v[3]);
            memcpy(pLwc->puck_game->nickname, p->nickname, sizeof(char) * LW_NICKNAME_MAX_LEN);
            pLwc->puck_game->rank = p->rank;
            pLwc->puck_game->score = p->score;
        } else if (CHECK_PACKET(packet_type, packet_size, LWPNICK)) {
            LOGI("LWPNICK received");
            LWPNICK* p = (LWPNICK*)cursor;
            LOGI("Cached user nick: %s", p->nickname);
            memcpy(pLwc->puck_game->nickname, p->nickname, sizeof(char) * LW_NICKNAME_MAX_LEN);
            pLwc->puck_game->score = p->score;
            pLwc->puck_game->rank = p->rank;
        } else if (CHECK_PACKET(packet_type, packet_size, LWPSYSMSG)) {
            LOGI("LWPSYSMSG received");
            LWPSYSMSG* p = (LWPSYSMSG*)cursor;
            show_sys_msg(pLwc->def_sys_msg, p->message);
        } else if (CHECK_PACKET(packet_type, packet_size, LWPLEADERBOARD)) {
            LOGI("LWPLEADERBOARD received");
            LWPLEADERBOARD* p = (LWPLEADERBOARD*)cursor;
            // Cache it first
            memcpy(&pLwc->last_leaderboard, p, sizeof(LWPLEADERBOARD));
            LOGI("Count: %d", p->Count);
            LOGI("First Item Rank: %d", p->First_item_rank);
            LOGI("First Item Tie Count: %d", p->First_item_tie_count);
            int rank = p->First_item_rank;
            int tieCount = 1;
            for (int i = 0; i < p->Count; i++) {
                LOGI("  rank.%d %s %d", rank, p->Nickname[i], p->Score[i]);
                if (i < p->Count - 1) {
                    if (p->Score[i] == p->Score[i + 1]) {
                        tieCount++;
                    } else {
                        if (rank == p->First_item_rank) {
                            rank += p->First_item_tie_count;
                        } else {
                            rank += tieCount;
                        }
                        tieCount = 1;
                    }
                }
            }
            if (tcp->on_leaderboard_packet) {
                tcp->on_leaderboard_packet(pLwc);
            }
        } else {
            LOGE("Unknown TCP packet");
        }
        parsed_bytes += packet_size;
        cursor += packet_size;
    }
    return parsed_bytes;
}

const char* lw_tcp_addr(const LWCONTEXT* pLwc) {
    return pLwc->tcp_host_addr.host;
}

const char* lw_tcp_port_str(const LWCONTEXT* pLwc) {
    return pLwc->tcp_host_addr.port_str;
}

int lw_tcp_port(const LWCONTEXT* pLwc) {
    return pLwc->tcp_host_addr.port;
}

