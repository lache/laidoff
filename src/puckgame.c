#include "puckgame.h"
#include "lwmacro.h"
#include "lwlog.h"
#include "lwtimepoint.h"
#include "numcomp.h"
#include <assert.h>

static void call_collision_callback(LWPUCKGAME* puck_game,
                                    const dContact* contact,
                                    void(*on_collision)(LWPUCKGAME*, float, float)) {
    dVector3 zero = { 0,0,0 };
    const dReal* v1 = dGeomGetBody(contact->geom.g1) ? dBodyGetLinearVel(dGeomGetBody(contact->geom.g1)) : zero;
    const dReal* v2 = dGeomGetBody(contact->geom.g2) ? dBodyGetLinearVel(dGeomGetBody(contact->geom.g2)) : zero;
    dVector3 vd;
    dSubtractVectors3(vd, v1, v2);
    const float vdlen = (float)dLENGTH(vd);
    const float depth = (float)contact->geom.depth;

    if (vdlen > 0.5f && depth > LWEPSILON && puck_game->puck_owner_player_no > 0) {
        int idx = puck_game->puck_owner_player_no - 1;
        assert(idx == 0 || idx == 1);
        puck_game->battle_stat[idx].PuckWallHit++;
    }

    if (on_collision) {
        on_collision(puck_game, vdlen, depth);
    }
}

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
    assert(go->geom == 0);
    go->geom = dCreateSphere(puck_game->space, testgo_radius);
    assert(go->body == 0);
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

static void destroy_go(LWPUCKGAME* puck_game, LW_PUCK_GAME_OBJECT lpgo) {
    LWPUCKGAMEOBJECT* go = &puck_game->go[lpgo];
    assert(go->geom);
    dGeomDestroy(go->geom);
    go->geom = 0;
    assert(go->body);
    dBodyDestroy(go->body);
    go->body = 0;
}

static void create_tower_geom(LWPUCKGAME* puck_game, int i) {
    LWPUCKGAMETOWER* tower = &puck_game->tower[i];
    assert(tower->geom == 0);
    tower->geom = dCreateCapsule(puck_game->space, puck_game->tower_radius, 10.0f);
    dGeomSetPosition(tower->geom,
                        puck_game->tower_pos * puck_game->tower_pos_multiplier[i][0],
                        puck_game->tower_pos * puck_game->tower_pos_multiplier[i][1],
                        0.0f);
    dGeomSetData(tower->geom, tower);
    // Tower #0(NW), #1(NE) --> player 1
    // Tower #2(SW), #3(SE) --> player 2
    tower->owner_player_no = i < LW_PUCK_GAME_TOWER_COUNT / 2 ? 1 : 2;
}

static void destroy_tower_geom(LWPUCKGAME* puck_game, int i) {
    LWPUCKGAMETOWER* tower = &puck_game->tower[i];
    assert(tower->geom);
    dGeomDestroy(tower->geom);
    tower->geom = 0;
}

static void create_control_joint(LWPUCKGAME* puck_game, LW_PUCK_GAME_OBJECT attach_target, dJointGroupID* joint_group, dJointID* control_joint) {
    assert(*control_joint == 0);
    *joint_group = dJointGroupCreate(0);
    *control_joint = dJointCreateLMotor(puck_game->world, *joint_group);
    dJointSetLMotorNumAxes(*control_joint, 2); // XY plane
    dJointSetLMotorAxis(*control_joint, 0, 0, 1, 0, 0); // x-axis actuator
    dJointSetLMotorAxis(*control_joint, 1, 0, 0, 1, 0); // y-axis actuator
    dJointAttach(*control_joint, puck_game->go[attach_target].body, 0);
    dJointSetLMotorParam(*control_joint, dParamFMax1, 10.0f);
    dJointSetLMotorParam(*control_joint, dParamFMax2, 10.0f);
}

static void destroy_control_joint(LWPUCKGAME* puck_game, dJointGroupID* joint_group, dJointID* control_joint) {
    assert(joint_group);
    dJointGroupDestroy(*joint_group);
    *joint_group = 0;
    // all joints within joint_group destroyed as a whole
    *control_joint = 0;
}

// This function setup all dynamic objects needed for 1vs1 battle.
// It allows a 'partial' creation state.
// (can be called safely even if subset of battle objects exist)
static void create_all_battle_objects(LWPUCKGAME* puck_game) {
    // create tower geoms
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (puck_game->tower[i].geom == 0) {
            create_tower_geom(puck_game, i);
        }
    }
    // create game objects (puck, player, target)
    if (puck_game->go[LPGO_PUCK].geom == 0) {
        create_go(puck_game, LPGO_PUCK, puck_game->sphere_mass, puck_game->sphere_radius);
    }
    if (puck_game->go[LPGO_PLAYER].geom == 0) {
        create_go(puck_game, LPGO_PLAYER, puck_game->sphere_mass, puck_game->sphere_radius);
    }
    if (puck_game->go[LPGO_TARGET].geom == 0) {
        create_go(puck_game, LPGO_TARGET, puck_game->sphere_mass, puck_game->sphere_radius);
    }
    // Create target control joint
    if (puck_game->target_control_joint_group == 0) {
        create_control_joint(puck_game, LPGO_TARGET, &puck_game->target_control_joint_group, &puck_game->target_control_joint);
    }
    // Create player control joint
    if (puck_game->player_control_joint_group == 0) {
        create_control_joint(puck_game, LPGO_PLAYER, &puck_game->player_control_joint_group, &puck_game->player_control_joint);
    }
}

// This function is a reverse of create_all_battle_objects().
static void destroy_all_battle_objects(LWPUCKGAME* puck_game) {
    // destroy tower geoms
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (puck_game->tower[i].geom) {
            destroy_tower_geom(puck_game, i);
        }
    }
    // destroy game objects (puck, player, target)
    if (puck_game->go[LPGO_PUCK].geom) {
        destroy_go(puck_game, LPGO_PUCK);
    }
    if (puck_game->go[LPGO_PLAYER].geom) {
        destroy_go(puck_game, LPGO_PLAYER);
    }
    if (puck_game->go[LPGO_TARGET].geom) {
        destroy_go(puck_game, LPGO_TARGET);
    }
    // destroy target control joint
    if (puck_game->target_control_joint_group) {
        destroy_control_joint(puck_game, &puck_game->target_control_joint_group, &puck_game->target_control_joint);
    }
    // destroy player control joint
    if (puck_game->player_control_joint_group) {
        destroy_control_joint(puck_game, &puck_game->player_control_joint_group, &puck_game->player_control_joint);
    }
}

LWPUCKGAME* new_puck_game(int update_frequency) {
    // Static game data
    LWPUCKGAME* puck_game = malloc(sizeof(LWPUCKGAME));
    memset(puck_game, 0, sizeof(LWPUCKGAME));
    // datasheet begin
    puck_game->world_size = 4.0f;
    puck_game->dash_interval = 1.2f;
    puck_game->dash_duration = 0.1f;
    puck_game->dash_shake_time = 0.3f;
    puck_game->hp_shake_time = 0.3f;
    puck_game->jump_force = 35.0f;
    puck_game->jump_interval = 0.5f;
    puck_game->jump_shake_time = 0.5f;
    puck_game->puck_damage_contact_speed_threshold = 1.1f;
    puck_game->sphere_mass = 0.1f;
    puck_game->sphere_radius = 0.12f; //0.16f;
    puck_game->total_time = 60.0f;
    puck_game->fire_max_force = 35.0f;
    puck_game->fire_max_vel = 5.0f;
    puck_game->fire_interval = 1.5f;
    puck_game->fire_duration = 0.2f;
    puck_game->fire_shake_time = 0.5f;
    puck_game->tower_pos = 1.1f;
    puck_game->tower_radius = 0.3f; //0.825f / 2;
    puck_game->tower_mesh_radius = 0.825f / 2; // Check tower.blend file
    puck_game->tower_total_hp = 5;
    puck_game->tower_shake_time = 0.2f;
    puck_game->go_start_pos = 0.6f;
    puck_game->hp = 5;
    puck_game->player_max_move_speed = 1.0f;
    puck_game->player_dash_speed = 6.0f;
    puck_game->boundary_impact_falloff_speed = 10.0f;
    puck_game->boundary_impact_start = 3.0f;
    puck_game->prepare_step_wait_tick = 2 * update_frequency;
    // datasheet end
    puck_game->world_size_half = puck_game->world_size / 2;
    puck_game->player.total_hp = puck_game->hp;
    puck_game->player.current_hp = puck_game->hp;
    puck_game->target.total_hp = puck_game->hp;
    puck_game->target.current_hp = puck_game->hp;
    puck_game->puck_reflect_size = 1.0f;
    int tower_pos_multiplier_index = 0;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][0] = -1;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][1] = -1;
    puck_game->tower_collapsing_z_rot_angle[tower_pos_multiplier_index] = (float)LWDEG2RAD(180);
    tower_pos_multiplier_index++;
    /*puck_game->tower_pos_multiplier[tower_pos_multiplier_index][0] = -1;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][1] = +1;
    tower_pos_multiplier_index++;*/
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][0] = +1;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][1] = +1;
    puck_game->tower_collapsing_z_rot_angle[tower_pos_multiplier_index] = (float)LWDEG2RAD(0);
    tower_pos_multiplier_index++;
    /*puck_game->tower_pos_multiplier[tower_pos_multiplier_index][0] = +1;
    puck_game->tower_pos_multiplier[tower_pos_multiplier_index][1] = -1;
    tower_pos_multiplier_index++;*/
    if (tower_pos_multiplier_index != LW_PUCK_GAME_TOWER_COUNT) {
        LOGE("Runtime assertion error");
        exit(-1);
    }

    // ------

    // Initialize OpenDE
    dInitODE2(0);
    puck_game->world = dWorldCreate();
    puck_game->space = dHashSpaceCreate(0);
    // dCreatePlane(..., a, b, c, d); ==> plane equation: a*x + b*y + c*z = d
    puck_game->boundary[LPGB_GROUND] = dCreatePlane(puck_game->space, 0, 0, 1, 0);
    puck_game->boundary[LPGB_E] = dCreatePlane(puck_game->space, -1, 0, 0, -puck_game->world_size_half);
    puck_game->boundary[LPGB_W] = dCreatePlane(puck_game->space, 1, 0, 0, -puck_game->world_size_half);
    puck_game->boundary[LPGB_S] = dCreatePlane(puck_game->space, 0, 1, 0, -puck_game->world_size_half);
    puck_game->boundary[LPGB_N] = dCreatePlane(puck_game->space, 0, -1, 0, -puck_game->world_size_half);
    //puck_game->boundary[LPGB_DIAGONAL_1] = dCreatePlane(puck_game->space, -1, -1, 0, 0);
    //puck_game->boundary[LPGB_DIAGONAL_2] = dCreatePlane(puck_game->space, +1, +1, 0, 0);
    for (int i = 0; i < LPGB_COUNT; i++) {
        if (puck_game->boundary[i]) {
            dGeomSetData(puck_game->boundary[i], (void*)i);
        }
    }
    // set global physics engine parameters
    dWorldSetGravity(puck_game->world, 0, 0, -9.81f);
    dWorldSetCFM(puck_game->world, 1e-5f);
    // joint group for physical contacts
    puck_game->contact_joint_group = dJointGroupCreate(0);
    // only puck has a red overlay light for indicating ownership update
    puck_game->go[LPGO_PUCK].red_overlay = 1;
    // create all battle dynamic objects (dynamic objects can be created/destroyed during runtime)
    create_all_battle_objects(puck_game);
    // Puck game runtime reset
    puck_game_reset(puck_game);
    // flag this instance is ready to run simulation
    puck_game->init_ready = 1;
    return puck_game;
}

void delete_puck_game(LWPUCKGAME** puck_game) {
    for (int i = 0; i < LPGB_COUNT; i++) {
        if ((*puck_game)->boundary[i]) {
            dGeomDestroy((*puck_game)->boundary[i]);
        }
    }
    destroy_all_battle_objects(*puck_game);
    dSpaceDestroy((*puck_game)->space);
    dWorldDestroy((*puck_game)->world);
    free(*puck_game);
    *puck_game = 0;
}

LWPUCKGAMEDASH* puck_game_single_play_dash_object(LWPUCKGAME* puck_game) {
    return &puck_game->remote_dash[puck_game->player_no == 2 ? 1 : 0];
}

LWPUCKGAMEJUMP* puck_game_single_play_jump_object(LWPUCKGAME* puck_game) {
    return &puck_game->remote_jump[puck_game->player_no == 2 ? 1 : 0];
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
    // custom collision callback
    call_collision_callback(puck_game, contact, puck_game->on_puck_player_collision);
}

static void near_puck_player(LWPUCKGAME* puck_game, dContact* contact) {
    near_puck_go(puck_game, 1, contact);
}

static void near_puck_target(LWPUCKGAME* puck_game, dContact* contact) {
    near_puck_go(puck_game, 2, contact);
}

int is_wall_geom(LWPUCKGAME* puck_game, dGeomID maybe_wall_geom) {
    return puck_game->boundary[LPGB_E] == maybe_wall_geom
        || puck_game->boundary[LPGB_W] == maybe_wall_geom
        || puck_game->boundary[LPGB_S] == maybe_wall_geom
        || puck_game->boundary[LPGB_N] == maybe_wall_geom;
}

LWPUCKGAMETOWER* get_tower_from_geom(LWPUCKGAME* puck_game, dGeomID maybe_tower_geom) {
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        if (maybe_tower_geom == puck_game->tower[i].geom) {
            return &puck_game->tower[i];
        }
    }
    return 0;
}

void near_puck_wall(LWPUCKGAME* puck_game, dGeomID puck_geom, dGeomID wall_geom, const dContact* contact) {
    LW_PUCK_GAME_BOUNDARY boundary = (LW_PUCK_GAME_BOUNDARY)dGeomGetData(wall_geom);
    if (boundary < LPGB_E || boundary > LPGB_N) {
        LOGE("boundary geom data corrupted");
        return;
    }
    puck_game->wall_hit_bit |= 1 << (boundary - LPGB_E);
    puck_game->wall_hit_bit_send_buf_1 |= puck_game->wall_hit_bit;
    puck_game->wall_hit_bit_send_buf_2 |= puck_game->wall_hit_bit;
    // custom collision callback
    call_collision_callback(puck_game, contact, puck_game->on_puck_wall_collision);
}

void near_puck_tower(LWPUCKGAME* puck_game, dGeomID puck_geom, LWPUCKGAMETOWER* tower, dContact* contact, double now) {
    // Puck - tower contacts
    // Set basic contact parameters first
    contact->surface.mode = dContactSoftCFM | dContactBounce;
    contact->surface.mu = 1.9f;
    contact->surface.bounce_vel = 0;
    // custom collision callback
    call_collision_callback(puck_game, contact, puck_game->on_puck_tower_collision);
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
        int* player_hp_ptr = tower->owner_player_no == 1 ? &puck_game->player.current_hp : &puck_game->target.current_hp;
        if (tower->hp > 0 || *player_hp_ptr > 0) {
            if (tower->hp > 0) {
                tower->hp--;
            }
            if (*player_hp_ptr > 0) {
                const int old_hp = *player_hp_ptr;
                (*player_hp_ptr)--;
                const int new_hp = *player_hp_ptr;
                if (old_hp > 0 && new_hp == 0) {
                    // on_death...
                    tower->collapsing = 1;
                    tower->collapsing_time = 0;
                    puck_game->battle_phase = tower->owner_player_no == 1 ? LSP_FINISHED_VICTORY_P2 : LSP_FINISHED_VICTORY_P1;
                    puck_game->battle_control_ui_alpha = 0;
                }
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
    // Early pruning
    // LPGB_DIAGONAL_1 should collided only with LPGO_PLAYER
    // LPGB_DIAGONAL_2 should collided only with LPGO_TARGET
    if (puck_game->boundary[LPGB_DIAGONAL_1]) {
        if (geom1 == puck_game->boundary[LPGB_DIAGONAL_1] || geom2 == puck_game->boundary[LPGB_DIAGONAL_1]) {
            if (geom1 == puck_game->go[LPGO_PLAYER].geom || geom2 == puck_game->go[LPGO_PLAYER].geom) {

            } else {
                return;
            }
        }
    }
    if (puck_game->boundary[LPGB_DIAGONAL_2]) {
        if (geom1 == puck_game->boundary[LPGB_DIAGONAL_2] || geom2 == puck_game->boundary[LPGB_DIAGONAL_2]) {
            if (geom1 == puck_game->go[LPGO_TARGET].geom || geom2 == puck_game->go[LPGO_TARGET].geom) {

            } else {
                return;
            }
        }
    }
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
        
        //if (geom1 == puck_game->boundary[LPGB_DIAGONAL_2] || geom2 == puck_game->boundary[LPGB_DIAGONAL_2]) {
        //    if (geom1 == puck_game->go[LPGO_TARGET].geom || puck_game->go[LPGO_TARGET].geom) {

        //    } else {
        //        return;
        //    }
        //}

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
            } else if (geom1 == puck_game->go[LPGO_PUCK].geom && is_wall_geom(puck_game, geom2)) {
                // Puck - wall contacts
                contact[i].surface.mode = dContactSoftCFM | dContactBounce;
                contact[i].surface.mu = 0;//1.9f;

                near_puck_wall(puck_game, geom1, geom2, &contact[i]);
            } else if (geom2 == puck_game->go[LPGO_PUCK].geom && is_wall_geom(puck_game, geom1)) {
                // Puck - wall contacts
                contact[i].surface.mode = dContactSoftCFM | dContactBounce;
                contact[i].surface.mu = 0;//1.9f;

                near_puck_wall(puck_game, geom2, geom1, &contact[i]);
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

float puck_game_dash_gauge_ratio(LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash) {
    return LWMIN(1.0f, puck_game_dash_elapsed_since_last(puck_game, dash) / puck_game->dash_interval);
}

float puck_game_dash_elapsed_since_last(const LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash) {
    return puck_game->time - dash->last_time;
}

int puck_game_dash_can_cast(const LWPUCKGAME* puck_game, const LWPUCKGAMEDASH* dash) {
    return puck_game_dash_elapsed_since_last(puck_game, dash) >= puck_game->dash_interval;
}

float puck_game_jump_cooltime(LWPUCKGAME* puck_game) {
    LWPUCKGAMEJUMP* jump = puck_game_single_play_jump_object(puck_game);
    return puck_game->time - jump->last_time;
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
    assert(player_no == 1 || player_no == 2);
    puck_game->battle_stat[player_no - 1].Dash++;
}

float puck_game_elapsed_time(int update_tick, int update_frequency) {
    return update_tick * 1.0f / update_frequency;
}

float puck_game_remain_time(float total_time, int update_tick, int update_frequency) {
    return LWMAX(0, total_time - puck_game_elapsed_time(update_tick, update_frequency));
}

int puck_game_remain_time_floor(float total_time, int update_tick, int update_frequency) {
    return (int)floorf(puck_game_remain_time(total_time, update_tick, update_frequency));
}

void puck_game_commit_fire(LWPUCKGAME* puck_game, LWPUCKGAMEFIRE* fire, int player_no, float puck_fire_dx, float puck_fire_dy, float puck_fire_dlen) {
    fire->remain_time = puck_game->fire_duration;
    fire->last_time = puck_game->time;
    fire->dir_x = puck_fire_dx;
    fire->dir_y = puck_fire_dy;
    fire->dir_len = puck_fire_dlen;
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

void puck_game_reset_go(LWPUCKGAME* puck_game, LWPUCKGAMEOBJECT* go, float x, float y, float z) {
    // reset physics engine values
    if (go->body) {
        dBodySetPosition(go->body, x, y, z);
        dMatrix3 rot_identity = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
        };
        dBodySetRotation(go->body, rot_identity);
        dBodySetLinearVel(go->body, 0, 0, 0);
        dBodySetAngularVel(go->body, 0, 0, 0);
        dBodySetForce(go->body, 0, 0, 0);
        dBodySetTorque(go->body, 0, 0, 0);
    }
    // reset cached values
    go->move_rad = 0;
    go->pos[0] = x;
    go->pos[1] = y;
    go->pos[2] = z;
    mat4x4_identity(go->rot);
    go->speed = 0;
    go->wall_hit_count = 0;
}

void puck_game_reset_battle_state(LWPUCKGAME* puck_game) {
    puck_game->update_tick = 0;
    puck_game->prepare_step_waited_tick = 0;
    puck_game->battle_phase = LSP_READY;
    // recreate all battle objects if not exists
    create_all_battle_objects(puck_game);
    for (int i = 0; i < LW_PUCK_GAME_TOWER_COUNT; i++) {
        puck_game->tower[i].hp = puck_game->tower_total_hp;
        puck_game->tower[i].collapsing = 0;
    }
    puck_game_reset_go(puck_game, &puck_game->go[LPGO_PUCK], 0.0f, 0.0f, puck_game->go[LPGO_PUCK].radius);
    puck_game_reset_go(puck_game, &puck_game->go[LPGO_PLAYER], -puck_game->go_start_pos, -puck_game->go_start_pos, puck_game->go[LPGO_PUCK].radius);
    puck_game_reset_go(puck_game, &puck_game->go[LPGO_TARGET], +puck_game->go_start_pos, +puck_game->go_start_pos, puck_game->go[LPGO_PUCK].radius);
    puck_game->player.total_hp = puck_game->hp;
    puck_game->player.current_hp = puck_game->hp;
    puck_game->target.total_hp = puck_game->hp;
    puck_game->target.current_hp = puck_game->hp;
    memset(puck_game->remote_control, 0, sizeof(puck_game->remote_control));
    puck_game->control_flags &= ~LPGCF_HIDE_TIMER;
}

void puck_game_reset_tutorial_state(LWPUCKGAME* puck_game) {
    puck_game->update_tick = 0;
    puck_game->prepare_step_waited_tick = 0;
    puck_game->battle_phase = LSP_TUTORIAL;
    // tutorial starts with an empty scene
    destroy_all_battle_objects(puck_game);
    puck_game->player.total_hp = puck_game->hp;
    puck_game->player.current_hp = puck_game->hp;
    puck_game->target.total_hp = puck_game->hp;
    puck_game->target.current_hp = puck_game->hp;
    memset(puck_game->remote_control, 0, sizeof(puck_game->remote_control));
    puck_game->control_flags |= LPGCF_HIDE_TIMER;
}

void puck_game_reset(LWPUCKGAME* puck_game) {
    puck_game_reset_battle_state(puck_game);
    puck_game->game_state = LPGS_MAIN_MENU;
    puck_game->world_roll = (float)LWDEG2RAD(180);
    puck_game->world_roll_dir = 1;
    puck_game->world_roll_axis = 1;
    puck_game->world_roll_target = puck_game->world_roll;
    puck_game->world_roll_target_follow_ratio = 0.075f;
}

void puck_game_remote_state_reset(LWPUCKGAME* puck_game, LWPSTATE* state) {
    state->bf.puck_owner_player_no = 0;
    state->bf.player_current_hp = puck_game->hp;
    state->bf.player_total_hp = puck_game->hp;
    state->bf.target_current_hp = puck_game->hp;
    state->bf.target_total_hp = puck_game->hp;
    state->bf.player_pull = 0;
    state->bf.target_pull = 0;
}

void puck_game_tower_pos(vec4 p_out, const LWPUCKGAME* puck_game, int owner_player_no) {
    if (owner_player_no != 1 && owner_player_no != 2) {
        LOGE(LWLOGPOS "invalid owner_player_no: %d", owner_player_no);
        return;
    }
    p_out[0] = puck_game->tower_pos * puck_game->tower_pos_multiplier[owner_player_no - 1][0];
    p_out[1] = puck_game->tower_pos * puck_game->tower_pos_multiplier[owner_player_no - 1][1];
    p_out[2] = 0.0f;
    p_out[3] = 1.0f;
}

void puck_game_control_bogus(LWPUCKGAME* puck_game) {
    const float target_follow_agility = 0.0075f; // 0 ~ 1
    const float dash_detect_radius = 0.75f;
    const float dash_frequency = 0.5f; // 0.0f ~ 1.0f
    const float dash_cooltime_lag_min = 0.4f;
    const float dash_cooltime_lag_max = 0.6f;

    float ideal_target_dx = puck_game->go[LPGO_PUCK].pos[0] - puck_game->go[LPGO_TARGET].pos[0];
    float ideal_target_dy = puck_game->go[LPGO_PUCK].pos[1] - puck_game->go[LPGO_TARGET].pos[1];
    puck_game->target_dx = (1.0f - target_follow_agility) * puck_game->target_dx + target_follow_agility * ideal_target_dx;
    puck_game->target_dy = (1.0f - target_follow_agility) * puck_game->target_dy + target_follow_agility * ideal_target_dy;
    
    float ideal_target_dx2 = ideal_target_dx * ideal_target_dx;
    float ideal_target_dy2 = ideal_target_dy * ideal_target_dy;
    float ideal_target_dlen = sqrtf(ideal_target_dx2 + ideal_target_dy2);

    float ideal_target_dlen_ratio = 1.0f;
    if (ideal_target_dlen < 0.5f) {
        ideal_target_dlen_ratio = 0.5f;
    }
    puck_game->target_dlen_ratio = (1.0f - target_follow_agility) * puck_game->target_dlen_ratio + target_follow_agility * ideal_target_dlen_ratio;

    puck_game->remote_control[1].dir_pad_dragging = 1;
    puck_game->remote_control[1].dx = puck_game->target_dx;
    puck_game->remote_control[1].dy = puck_game->target_dy;
    puck_game->remote_control[1].dlen = puck_game->target_dlen_ratio;
    puck_game->remote_control[1].pull_puck = 0;

    // dash
    if (ideal_target_dlen < dash_detect_radius) {
        if (numcomp_float_random_01() < dash_frequency) {
            int bogus_player_no = puck_game->player_no == 2 ? 1 : 2;
            LWPUCKGAMEDASH* dash = &puck_game->remote_dash[bogus_player_no - 1];
            const float dash_cooltime_aware_lag = numcomp_float_random_range(dash_cooltime_lag_min, dash_cooltime_lag_max);
            if (puck_game_dash_elapsed_since_last(puck_game, dash) >= puck_game->dash_interval + dash_cooltime_aware_lag) {
                puck_game_dash(puck_game, dash, bogus_player_no);
            }
        }
    }
}

void puck_game_update_remote_player(LWPUCKGAME* puck_game, float delta_time, int i) {
    dJointID pcj[2] = {
        puck_game->player_control_joint,
        puck_game->target_control_joint,
    };
    LW_PUCK_GAME_OBJECT control_enum[2] = {
        LPGO_PLAYER,
        LPGO_TARGET,
    };
    
    if (pcj[i]) {
        if (puck_game->remote_control[i].dir_pad_dragging) {
            float dx, dy, dlen;
            dx = puck_game->remote_control[i].dx;
            dy = puck_game->remote_control[i].dy;
            dlen = puck_game->remote_control[i].dlen;
            if (dlen > 1.0f) {
                dlen = 1.0f;
            }
            dJointEnable(pcj[i]);
            dJointSetLMotorParam(pcj[i], dParamVel1, puck_game->player_max_move_speed * dx * dlen);
            dJointSetLMotorParam(pcj[i], dParamVel2, puck_game->player_max_move_speed * dy * dlen);
        } else {
            dJointSetLMotorParam(pcj[i], dParamVel1, 0);
            dJointSetLMotorParam(pcj[i], dParamVel2, 0);
        }
        // Move direction fixed while dashing
        if (puck_game->remote_dash[i].remain_time > 0) {
            float dx, dy;
            dx = puck_game->remote_dash[i].dir_x;
            dy = puck_game->remote_dash[i].dir_y;
            dJointSetLMotorParam(pcj[i], dParamVel1, puck_game->player_dash_speed * dx);
            dJointSetLMotorParam(pcj[i], dParamVel2, puck_game->player_dash_speed * dy);
            puck_game->remote_dash[i].remain_time = LWMAX(0,
                                                          puck_game->remote_dash[i].remain_time - delta_time);
        }
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
        if (pcj[i]) {
            dJointDisable(pcj[i]);
        }
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

int puck_game_dash(LWPUCKGAME* puck_game, LWPUCKGAMEDASH* dash, int player_no) {
    // Check params
    if (!puck_game || !dash) {
        return -1;
    }
    // Check already effective dash
    if (puck_game_dashing(dash)) {
        return -2;
    }
    // Check effective move input
    //float dx, dy, dlen;
    /*if (!lw_get_normalized_dir_pad_input(pLwc, &dx, &dy, &dlen)) {
    return;
    }*/

    // Check cooltime
    if (puck_game_dash_can_cast(puck_game, dash) == 0) {
        dash->shake_remain_time = puck_game->dash_shake_time;
        return -3;
    }

    // Start dash!
    puck_game_commit_dash_to_puck(puck_game, dash, player_no);
    //puck_game_commit_dash(puck_game, &puck_game->dash, dx, dy);
    return 0;
}

void puck_game_roll_world(LWPUCKGAME* puck_game, int dir, int axis, float target) {
    if (puck_game->world_roll_dirty == 0) {
        LOGI("World roll began...");
        puck_game->world_roll_dir = dir;
        puck_game->world_roll_axis = axis;
        puck_game->world_roll_target = target;
        puck_game->world_roll_dirty = 1;
    }
}

void puck_game_roll_to_battle(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty == 0) {
        puck_game->game_state = LPGS_BATTLE;
        LOGI("World roll to battle began...");
        puck_game->world_roll_target = 0;
        puck_game->world_roll_dirty = 1;
    }
}

void puck_game_roll_to_practice(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty == 0) {
        puck_game->game_state = LPGS_PRACTICE;
        LOGI("World roll to practice began...");
        puck_game->world_roll_target = 0;
        puck_game->world_roll_dirty = 1;
    }
}

void puck_game_roll_to_tutorial(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty == 0) {
        puck_game->game_state = LPGS_TUTORIAL;
        LOGI("World roll to tutorial began...");
        puck_game->world_roll_target = 0;
        puck_game->world_roll_dirty = 1;
    }
}

void puck_game_roll_to_main_menu(LWPUCKGAME* puck_game) {
    if (puck_game->world_roll_dirty == 0) {
        puck_game->game_state = LPGS_MAIN_MENU;
        puck_game_roll_world(puck_game, 1, 1, (float)LWDEG2RAD(180));
    }
}

void puck_game_set_searching_str(LWPUCKGAME* puck_game, const char* str) {
    strcpy(puck_game->searching_str, str);
}

void puck_game_set_tutorial_guide_str(LWPUCKGAME* puck_game, const char* str) {
    strcpy(puck_game->tutorial_guide_str, str);
}

void puck_game_update_tick(LWPUCKGAME* puck_game, int update_frequency, float delta_time) {
    // reset per-frame caches
    puck_game->player.puck_contacted = 0;
    puck_game->target.puck_contacted = 0;
    // reset wall hit bits
    puck_game->wall_hit_bit = 0;
    // check timeout condition
    if (puck_game_remain_time(puck_game->total_time, puck_game->update_tick, update_frequency) <= 0) {
        const int hp_diff = puck_game->player.current_hp - puck_game->target.current_hp;
        puck_game->battle_phase = hp_diff > 0 ? LSP_FINISHED_VICTORY_P1 : hp_diff < 0 ? LSP_FINISHED_VICTORY_P2 : LSP_FINISHED_DRAW;
        puck_game->battle_control_ui_alpha = 0;
    }
    // transition from READY to STEADY to GO
    if (puck_game->battle_phase == LSP_READY) {
        puck_game->prepare_step_waited_tick++;
        if (puck_game->prepare_step_waited_tick >= puck_game->prepare_step_wait_tick) {
            puck_game->battle_phase = LSP_STEADY;
            puck_game->prepare_step_waited_tick = 0;
            puck_game->battle_control_ui_alpha = 0.2f;
        }
    } else if (puck_game->battle_phase == LSP_STEADY) {
        puck_game->prepare_step_waited_tick++;
        if (puck_game->prepare_step_waited_tick >= puck_game->prepare_step_wait_tick) {
            puck_game->battle_phase = LSP_GO;
            puck_game->prepare_step_waited_tick = 0;
            puck_game->battle_control_ui_alpha = 1.0f;
        }
    }
    // stepping physics only if battling
    if (puck_game_state_phase_battling(puck_game->battle_phase)) {
        puck_game->update_tick++;
        dSpaceCollide(puck_game->space, puck_game, puck_game_near_callback);
        dWorldQuickStep(puck_game->world, 1.0f / 60);
        dJointGroupEmpty(puck_game->contact_joint_group);
    }
    // update last contact puck body
    if (puck_game->player.puck_contacted == 0) {
        puck_game->player.last_contact_puck_body = 0;
    }
    if (puck_game->target.puck_contacted == 0) {
        puck_game->target.last_contact_puck_body = 0;
    }
    // update battle stat stat (max puck speed)
    const float puck_speed_int = puck_game->go[LPGO_PUCK].speed;
    if (puck_game->puck_owner_player_no == 1) {
        puck_game->battle_stat[0].MaxPuckSpeed = LWMAX(puck_speed_int, puck_game->battle_stat[0].MaxPuckSpeed);
    } else if (puck_game->puck_owner_player_no == 2) {
        puck_game->battle_stat[1].MaxPuckSpeed = LWMAX(puck_speed_int, puck_game->battle_stat[1].MaxPuckSpeed);
    }
    // update battle stat stat (travel distance)
    puck_game->battle_stat[0].TravelDistance += puck_game->go[LPGO_PLAYER].speed / update_frequency;
    puck_game->battle_stat[1].TravelDistance += puck_game->go[LPGO_TARGET].speed / update_frequency;
}
