#include "puckgame.h"
#include "lwmacro.h"
#include "lwlog.h"
#include "lwtimepoint.h"
//#include "lwcontext.h"
//#include "input.h"

static void testgo_move_callback(dBodyID b) {
    LWPUCKGAMEOBJECT* go = (LWPUCKGAMEOBJECT*)dBodyGetData(b);
    LWPUCKGAME* puck_game = go->puck_game;

    // Position
    const dReal* p = dBodyGetPosition(b);
    go->pos[0] = (float)p[0];
    go->pos[1] = (float)p[1];
    go->pos[2] = (float)p[2];
    // Orientation
    const dReal* r = dBodyGetRotation(b); // dMatrix3
    mat4x4 rot;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            rot[i][j] = (float)r[4 * i + j];
        }
    }
    rot[3][0] = 0;
    rot[3][1] = 0;
    rot[3][2] = 0;
    rot[3][3] = 1;

    mat4x4_transpose(go->rot, rot);
    const dReal* vel = dBodyGetLinearVel(go->body);
    go->speed = (float)dLENGTH(vel);
    if (vel[0]) {
        go->move_rad = atan2f((float)vel[1], (float)vel[0]) + (float)M_PI / 2;
    }
}

static void create_go(LWPUCKGAME* puck_game, LW_PUCK_GAME_OBJECT lpgo, float mass, float radius) {
    LWPUCKGAMEOBJECT* go = &puck_game->go[lpgo];
    go->puck_game = puck_game;
    go->radius = radius;
    const float testgo_radius = go->radius;
    go->geom = dCreateSphere(puck_game->space, testgo_radius);
    go->body = dBodyCreate(puck_game->world);
    dMass m;
    dMassSetSphereTotal(&m, mass, testgo_radius);
    dBodySetMass(go->body, &m);
    dBodySetData(go->body, go);
    dBodySetMovedCallback(go->body, testgo_move_callback);
    //dBodySetLinearVel(go->body, -2.0f, 3.0f, 0);
    dGeomSetBody(go->geom, go->body);
    //dBodySetDamping(go->body, 1e-2f, 1e-2f);
    //dBodySetAutoDisableLinearThreshold(go->body, 0.05f);
}

LWPUCKGAME* new_puck_game() {
    // Static game data
    LWPUCKGAME* puck_game = malloc(sizeof(LWPUCKGAME));
    memset(puck_game, 0, sizeof(LWPUCKGAME));
    const int hp = 10;
    puck_game->world_size = 4.0f;
    puck_game->world_size_half = puck_game->world_size / 2.0f;
    puck_game->dash_interval = 1.2f;
    puck_game->dash_duration = 0.1f;
    puck_game->dash_shake_time = 0.3f;
    puck_game->hp_shake_time = 0.3f;
    puck_game->puck_damage_contact_speed_threshold = 1.1f;
    puck_game->player.total_hp = hp;
    puck_game->player.current_hp = hp;
    puck_game->target.total_hp = hp;
    puck_game->target.current_hp = hp;
    puck_game->sphere_mass = 0.1f;
    puck_game->sphere_radius = 0.16f;
    puck_game->total_time = 80.0f;
    puck_game->jump_force = 35.0f;
    puck_game->jump_interval = 0.5f;
    puck_game->jump_shake_time = 0.5f;
    puck_game->fire_max_force = 35.0f;
    puck_game->fire_max_vel = 5.0f;
    puck_game->fire_interval = 1.5f;
    puck_game->fire_duration = 0.2f;
    puck_game->fire_shake_time = 0.5f;
    puck_game->tower_pos = 1.1f;
    puck_game->tower_radius = 0.825f / 2; // Check tower.blend file
    puck_game->puck_reflect_size = 1.0f;
    int tower_pos_multiplier_index = 0;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][0] = -1;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][1] = -1;
    tower_pos_multiplier_index++;
    /*puck_game->tower_pos_multiplier[tower_pos_multiplier_index][0] = -1;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][1] = +1;
    tower_pos_multiplier_index++;*/
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][0] = +1;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][1] = +1;
    tower_pos_multiplier_index++;
    /*puck_game->tower_pos_multiplier[tower_pos_multiplier_index][0] = +1;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][1] = -1;
    tower_pos_multiplier_index++;*/
    if (tower_pos_multiplier_index != LW_PUCK_GAME_TOWER_COUNT) {
        LOGE("Runtime assertion error");
        exit(-1);
    }
    puck_game->tower_total_hp = 5;
    puck_game->tower_shake_time = 0.2f;
    puck_game->go_start_pos = 0.6f;
    // ------

    // Initialize OpenDE
    dInitODE2(0);
    puck_game->world = dWorldCreate();
    puck_game->space = dHashSpaceCreate(0);
    puck_game->boundary[LPGB_GROUND] = dCreatePlane(puck_game->space, 0, 0, 1, 0);
    puck_game->boundary[LPGB_E] = dCreatePlane(puck_game->space, -1, 0, 0, -puck_game->world_size_half);
    puck_game->boundary[LPGB_W] = dCreatePlane(puck_game->space, 1, 0, 0, -puck_game->world_size_half);
    puck_game->boundary[LPGB_S] = dCreatePlane(puck_game->space, 0, -1, 0, -puck_game->world_size_half);
    puck_game->boundary[LPGB_N] = dCreatePlane(puck_game->space, 0, 1, 0, -puck_game->world_size_half);
    dWorldSetGravity(puck_game->world, 0, 0, -9.81f);
    dWorldSetCFM(puck_game->world, 1e-5f);

    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        puck_game->tower[i].geom = dCreateCapsule(puck_game->space, puck_game->tower_radius, 10.0f);
        dGeomSetPosition(puck_game->tower[i].geom,
                         puck_game->tower_pos * puck_game->tower_pos_multiplier[i][0],
                         puck_game->tower_pos * puck_game->tower_pos_multiplier[i][1],
                         0.0f);
        dGeomSetData(puck_game->tower[i].geom, &puck_game->tower[i]);
        // Tower #0(NW), #1(NE) --> player 1
        // Tower #2(SW), #3(SE) --> player 2
        puck_game->tower[i].owner_player_no = i < LW_PUCK_GAME_TOWER_COUNT / 2 ? 1 : 2;
    }
    
    create_go(puck_game, LPGO_PUCK, puck_game->sphere_mass, puck_game->sphere_radius);
    create_go(puck_game, LPGO_PLAYER, puck_game->sphere_mass, puck_game->sphere_radius);
    create_go(puck_game, LPGO_TARGET, puck_game->sphere_mass, puck_game->sphere_radius);

    puck_game->contact_joint_group = dJointGroupCreate(0);
    puck_game->player_control_joint_group = dJointGroupCreate(0);

    // Create target control joint
    puck_game->target_control_joint = dJointCreateLMotor(puck_game->world, puck_game->target_control_joint_group);
    dJointID tcj = puck_game->target_control_joint;
    dJointSetLMotorNumAxes(tcj, 2);
    dJointSetLMotorAxis(tcj, 0, 0, 1, 0, 0); // x-axis actuator
    dJointSetLMotorAxis(tcj, 1, 0, 0, 1, 0); // y-axis actuator
    dJointAttach(tcj, puck_game->go[LPGO_TARGET].body, 0);
    dJointSetLMotorParam(tcj, dParamFMax1, 10.0f);
    dJointSetLMotorParam(tcj, dParamFMax2, 10.0f);

    // Create player control joint
    puck_game->player_control_joint = dJointCreateLMotor(puck_game->world, puck_game->player_control_joint_group);
    dJointID pcj = puck_game->player_control_joint;
    dJointSetLMotorNumAxes(pcj, 2);
    dJointSetLMotorAxis(pcj, 0, 0, 1, 0, 0); // x-axis actuator
    dJointSetLMotorAxis(pcj, 1, 0, 0, 1, 0); // y-axis actuator
    dJointAttach(pcj, puck_game->go[LPGO_PLAYER].body, 0);
    dJointSetLMotorParam(pcj, dParamFMax1, 10.0f);
    dJointSetLMotorParam(pcj, dParamFMax2, 10.0f);

    puck_game->go[LPGO_PUCK].red_overlay = 1;

    // Puck game runtime reset
    puck_game_reset(puck_game);

    puck_game->init_ready = 1;
    return puck_game;
}

void delete_puck_game(LWPUCKGAME** puck_game) {
    for (int i = 0; i < LPGB_COUNT; i++) {
        dGeomDestroy((*puck_game)->boundary[i]);
    }
    dJointGroupDestroy((*puck_game)->player_control_joint_group);
    dJointGroupDestroy((*puck_game)->contact_joint_group);
    dSpaceDestroy((*puck_game)->space);
    dWorldDestroy((*puck_game)->world);

    free(*puck_game);
    *puck_game = 0;
}

void puck_game_go_decrease_hp_test(LWPUCKGAME* puck_game, LWPUCKGAMEPLAYER* go, LWPUCKGAMEDASH* dash, LWPUCKGAMETOWER* tower) {
    LWPUCKGAMEOBJECT* puck = &puck_game->go[LPGO_PUCK];
    const float puck_speed = (float)dLENGTH(dBodyGetLinearVel(puck->body));

    if (go->last_contact_puck_body != puck->body
        && puck_speed > puck_game->puck_damage_contact_speed_threshold
        && !puck_game_dashing(dash)) {
        // Decrease player hp
        go->last_contact_puck_body = puck->body;
        go->current_hp--;
        if (go->current_hp < 0) {
            //go->current_hp = go->total_hp;
        }
        go->hp_shake_remain_time = puck_game->hp_shake_time;
    }

    if (tower->hp > 0) {
        tower->hp--;
    } else if (tower->hp <= 0) {
        tower->hp = puck_game->tower_total_hp;
    }
}

void puck_game_player_decrease_hp_test(LWPUCKGAME* puck_game) {
    puck_game_go_decrease_hp_test(puck_game, &puck_game->player, &puck_game->remote_dash[0], &puck_game->tower[0]);
}

void puck_game_target_decrease_hp_test(LWPUCKGAME* puck_game) {
    puck_game_go_decrease_hp_test(puck_game, &puck_game->target, &puck_game->remote_dash[1], &puck_game->tower[1]);
}

static void near_puck_go(LWPUCKGAME* puck_game, int player_no, dContact* contact) {
    //LWPUCKGAMEOBJECT* puck = &puck_game->go[LPGO_PUCK];
    //LWPUCKGAMEOBJECT* go = &puck_game->go[player_no == 1 ? LPGO_PLAYER : LPGO_TARGET];
    contact->surface.mode = dContactSoftCFM | dContactBounce;
    contact->surface.mu = 1.9f;

    if (puck_game->puck_owner_player_no == 0) {
        puck_game->puck_owner_player_no = player_no;
        puck_game->puck_reflect_size = 2.0f;
    } else if (puck_game->puck_owner_player_no != player_no) {
        puck_game->puck_reflect_size = 2.0f;
        puck_game->puck_owner_player_no = player_no;
        if (player_no == 1) {
            puck_game->player.puck_contacted = 1;
        } else {
            puck_game->target.puck_contacted = 1;
        }
    }
}

static void near_puck_player(LWPUCKGAME* puck_game, dContact* contact) {
    near_puck_go(puck_game, 1, contact);
}

static void near_puck_target(LWPUCKGAME* puck_game, dContact* contact) {
    near_puck_go(puck_game, 2, contact);
}

LWPUCKGAMETOWER* get_tower_from_geom(LWPUCKGAME* puck_game, dGeomID maybe_tower_geom) {
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (maybe_tower_geom == puck_game->tower[i].geom) {
            return &puck_game->tower[i];
        }
    }
    return 0;
}

void near_puck_tower(LWPUCKGAME* puck_game, dGeomID puck_geom, LWPUCKGAMETOWER* tower, dContact* contact, double now) {
    // Puck - tower contacts
    // Set basic contact parameters first
    contact->surface.mode = dContactSoftCFM | dContactBounce;
    contact->surface.mu = 1.9f;
    contact->surface.bounce_vel = 0;
    // Check puck ownership
    if (puck_game->puck_owner_player_no == tower->owner_player_no) {
        return;
    }
    if (puck_game->puck_owner_player_no == 0) {
        return;
    }
    // Check minimum contact damage speed threshold
    dBodyID puck_body = dGeomGetBody(puck_geom);
    dReal puck_speed = dLENGTH(dBodyGetLinearVel(puck_body));
    if (puck_speed < (dReal)1.0) {
        return;
    }
    // Check last damaged cooltime
    if (now - tower->last_damaged_at > 1.0f) {
        int* player_hp = tower->owner_player_no == 1 ? &puck_game->player.current_hp : &puck_game->target.current_hp;
        if (tower->hp > 0 || *player_hp > 0) {
            if (tower->hp > 0) {
                tower->hp--;
            }
            if (*player_hp > 0) {
                (*player_hp)--;
            }
            tower->shake_remain_time = puck_game->tower_shake_time;
            tower->last_damaged_at = now;

            if (tower->owner_player_no == 1) {
                if (puck_game->on_player_damaged) {
                    puck_game->on_player_damaged(puck_game);
                }
            } else {
                if (puck_game->on_target_damaged) {
                    puck_game->on_target_damaged(puck_game);
                }
            }
        }
    }
}

void puck_game_near_callback(void* data, dGeomID geom1, dGeomID geom2) {
    LWPUCKGAME* puck_game = (LWPUCKGAME*)data;
    const double now = lwtimepoint_now_seconds();

    if (dGeomIsSpace(geom1) || dGeomIsSpace(geom2)) {

        // colliding a space with something :
        dSpaceCollide2(geom1, geom2, data, &puck_game_near_callback);

        // collide all geoms internal to the space(s)
        if (dGeomIsSpace(geom1))
            dSpaceCollide((dSpaceID)geom1, data, &puck_game_near_callback);
        if (dGeomIsSpace(geom2))
            dSpaceCollide((dSpaceID)geom2, data, &puck_game_near_callback);

    } else {
        const int max_contacts = 5;
        dContact contact[5];
        // colliding two non-space geoms, so generate contact
        // points between geom1 and geom2
        int num_contact = dCollide(geom1, geom2, max_contacts, &contact->geom, sizeof(dContact));
        // add these contact points to the simulation ...
        LWPUCKGAMETOWER* tower = 0;
        for (int i = 0; i < num_contact; i++) {
            // bounce is the amount of "bouncyness".
            contact[i].surface.bounce = 0.9f;
            // bounce_vel is the minimum incoming velocity to cause a bounce
            contact[i].surface.bounce_vel = 0.1f;
            // constraint force mixing parameter
            contact[i].surface.soft_cfm = 0.001f;

            if (geom1 == puck_game->boundary[LPGB_GROUND] || geom2 == puck_game->boundary[LPGB_GROUND]) {
                // All objects - ground contacts
                contact[i].surface.mode = dContactSoftCFM | dContactRolling;// | dContactFDir1;
                contact[i].surface.rho = 0.001f;
                contact[i].surface.rho2 = 0.001f;
                contact[i].surface.rhoN = 0.001f;
                //contact[i].fdir1[0] = 1.0f;
                //contact[i].fdir1[1] = 0.0f;
                //contact[i].fdir1[2] = 0.0f;
                contact[i].surface.mu = 100.9f;
            } else if ((geom1 == puck_game->go[LPGO_PUCK].geom && geom2 == puck_game->go[LPGO_PLAYER].geom)
                       || (geom1 == puck_game->go[LPGO_PLAYER].geom && geom2 == puck_game->go[LPGO_PUCK].geom)) {
                // Player - puck contacts
                near_puck_player(puck_game, &contact[i]);
            } else if ((geom1 == puck_game->go[LPGO_PUCK].geom && geom2 == puck_game->go[LPGO_TARGET].geom)
                       || (geom1 == puck_game->go[LPGO_TARGET].geom && geom2 == puck_game->go[LPGO_PUCK].geom)) {
                // Target - puck contacts
                near_puck_target(puck_game, &contact[i]);
            } else if (geom1 == puck_game->go[LPGO_PUCK].geom && (tower = get_tower_from_geom(puck_game, geom2))) {
                // Puck - tower contacts
                near_puck_tower(puck_game, geom1, tower, &contact[i], now);
            } else if (geom2 == puck_game->go[LPGO_PUCK].geom && (tower = get_tower_from_geom(puck_game, geom1))) {
                // Puck - tower contacts
                near_puck_tower(puck_game, geom2, tower, &contact[i], now);
            } else {
                // Other contacts
                contact[i].surface.mode = dContactSoftCFM | dContactBounce;
                contact[i].surface.mu = 0;//1.9f;
            }

            dJointID c = dJointCreateContact(puck_game->world, puck_game->contact_joint_group, &contact[i]);
            dBodyID b1 = dGeomGetBody(geom1);
            dBodyID b2 = dGeomGetBody(geom2);
            dJointAttach(c, b1, b2);
        }
    }
}

void puck_game_push(LWPUCKGAME* puck_game) {
    //puck_game->push = 1;

    dBodySetLinearVel(puck_game->go[LPGO_PUCK].body, 1, 1, 0);
}

float puck_game_dash_gauge_ratio(LWPUCKGAME* puck_game) {
    return LWMIN(1.0f, puck_game_dash_cooltime(puck_game) / puck_game->dash_interval);
}

float puck_game_dash_cooltime(LWPUCKGAME* puck_game) {
    return puck_game->time - puck_game->dash.last_time;
}

float puck_game_jump_cooltime(LWPUCKGAME* puck_game) {
    return puck_game->time - puck_game->jump.last_time;
}

int puck_game_jumping(LWPUCKGAMEJUMP* jump) {
    return jump->remain_time > 0;
}

int puck_game_dashing(LWPUCKGAMEDASH* dash) {
    return dash->remain_time > 0;
}

void puck_game_commit_jump(LWPUCKGAME* puck_game, LWPUCKGAMEJUMP* jump, int player_no) {
    jump->remain_time = puck_game->jump_interval;
    jump->last_time = puck_game->time;
}

void puck_game_commit_dash(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, float dx, float dy) {
    dash->remain_time = puck_game->dash_duration;
    dash->dir_x = dx;
    dash->dir_y = dy;
    dash->last_time = puck_game->time;
}

void puck_game_commit_dash_to_puck(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, int player_no) {
    float dx = puck_game->go[LPGO_PUCK].pos[0] - puck_game->go[player_no == 1 ? LPGO_PLAYER : LPGO_TARGET].pos[0];
    float dy = puck_game->go[LPGO_PUCK].pos[1] - puck_game->go[player_no == 1 ? LPGO_PLAYER : LPGO_TARGET].pos[1];
    const float ddlen = sqrtf(dx * dx + dy * dy);
    dx /= ddlen;
    dy /= ddlen;
    puck_game_commit_dash(puck_game, dash, dx, dy);
}

float puck_game_remain_time(float total_time, int update_tick) {
    return floorf(LWMAX(0, total_time - update_tick * 1.0f / 125));
}

void puck_game_commit_fire(LWPUCKGAME* puck_game, LWPUCKGAMEFIRE* fire, int player_no, float puck_fire_dx, float puck_fire_dy, float puck_fire_dlen) {
    fire->remain_time = puck_game->fire_duration;
    fire->last_time = puck_game->time;
    fire->dir_x = puck_fire_dx;
    fire->dir_y = puck_fire_dy;
    fire->dir_len = puck_fire_dlen;
}

float puck_game_player_max_speed() {
    return 1.0f;
}

float puck_game_player_dash_speed() {
    return 6.0f;
}

void update_puck_reflect_size(LWPUCKGAME* puck_game, float delta_time) {
    if (puck_game->puck_reflect_size > 1.0f) {
        puck_game->puck_reflect_size = LWMAX(1.0f, puck_game->puck_reflect_size - (float)delta_time * 2);
    }
}

void update_puck_ownership(LWPUCKGAME* puck_game) {
    const float speed = puck_game->go[LPGO_PUCK].speed;
    const float red_overlay_ratio = LWMIN(1.0f, speed / puck_game->puck_damage_contact_speed_threshold);
    if (puck_game->puck_owner_player_no != 0 && red_overlay_ratio < 0.5f) {
        puck_game->puck_owner_player_no = 0;
        //puck_game->puck_reflect_size = 2.0f;
    }
}

void puck_game_reset(LWPUCKGAME* puck_game) {
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        puck_game->tower[i].hp = puck_game->tower_total_hp;
    }
    dBodySetPosition(puck_game->go[LPGO_PUCK].body, 0.0f, 0.0f, puck_game->go[LPGO_PUCK].radius);
    dBodySetPosition(puck_game->go[LPGO_PLAYER].body, -puck_game->go_start_pos, -puck_game->go_start_pos, puck_game->go[LPGO_PUCK].radius);
    dBodySetPosition(puck_game->go[LPGO_TARGET].body, +puck_game->go_start_pos, +puck_game->go_start_pos, puck_game->go[LPGO_PUCK].radius);
    for (int i = 0; i < LPGO_COUNT; i++) {
        dBodySetLinearVel(puck_game->go[i].body, 0, 0, 0);
        dBodySetAngularVel(puck_game->go[i].body, 0, 0, 0);
        dBodySetForce(puck_game->go[i].body, 0, 0, 0);
        dBodySetTorque(puck_game->go[i].body, 0, 0, 0);
    }
}
