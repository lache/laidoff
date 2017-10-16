#include "puckgame.h"
#include "lwmacro.h"
#include "lwlog.h"

static void testgo_move_callback(dBodyID b) {
	const dReal* p = dBodyGetPosition(b);
	LWPUCKGAME* puck_game = (LWPUCKGAME*)dBodyGetData(b);
	puck_game->testgo.pos[0] = (float)p[0];
	puck_game->testgo.pos[1] = (float)p[1];
	puck_game->testgo.pos[2] = (float)p[2];
}

LWPUCKGAME* new_puck_game() {
	LWPUCKGAME* puck_game = malloc(sizeof(LWPUCKGAME));
	memset(puck_game, 0, sizeof(LWPUCKGAME));

	puck_game->world_size = 4.0f;
	puck_game->world_size_half = puck_game->world_size / 2.0f;

	// Initialize OpenDE
	dInitODE2(0);
	puck_game->world = dWorldCreate();
	puck_game->space = dHashSpaceCreate(0);
	puck_game->boundary[LPGB_GROUND] = dCreatePlane(puck_game->space, 0, 0, 1, 0);
	puck_game->boundary[LPGB_E] = dCreatePlane(puck_game->space, -1, 0, 0, -puck_game->world_size_half);
	puck_game->boundary[LPGB_W] = dCreatePlane(puck_game->space, 1, 0, 0, -puck_game->world_size_half);
	puck_game->boundary[LPGB_S] = dCreatePlane(puck_game->space, 0, -1, 0, -puck_game->world_size_half);
	puck_game->boundary[LPGB_N] = dCreatePlane(puck_game->space, 0, 1, 0, -puck_game->world_size_half);
	dWorldSetGravity(puck_game->world, 0, 0, -0.2f);
	dWorldSetCFM(puck_game->world, 1e-5f);
	const float testgo_mass = 0.1f;
	puck_game->testgo.radius = 0.125f;
	const float testgo_radius = puck_game->testgo.radius;
	const float initial_height = testgo_radius * 1;
	puck_game->testgo.geom = dCreateSphere(puck_game->space, testgo_radius);
	puck_game->testgo.body = dBodyCreate(puck_game->world);
	dMass m;
	dMassSetSphereTotal(&m, testgo_mass, testgo_radius);
	dBodySetMass(puck_game->testgo.body, &m);
	
	dBodySetPosition(puck_game->testgo.body, 0, 0, initial_height);
	dBodySetData(puck_game->testgo.body, puck_game);
	dBodySetMovedCallback(puck_game->testgo.body, testgo_move_callback);
	dBodySetLinearVel(puck_game->testgo.body, -2.0f, 3.0f, 0);
	dGeomSetBody(puck_game->testgo.geom, puck_game->testgo.body);
	//dBodySetDamping(puck_game->testgo.body, 1e-2f, 1e-2f);
	//dBodySetAutoDisableLinearThreshold(puck_game->testgo.body, 0.05f);
	puck_game->contact_joint_group = dJointGroupCreate(0);
	return puck_game;
}

void delete_puck_game(LWPUCKGAME** puck_game) {
	for (int i = 0; i < LPGB_COUNT; i++) {
		dGeomDestroy((*puck_game)->boundary[i]);
	}
	dJointGroupDestroy((*puck_game)->contact_joint_group);
	dSpaceDestroy((*puck_game)->space);
	dWorldDestroy((*puck_game)->world);

	free(*puck_game);
	*puck_game = 0;
}

static void near_callback(void *data, dGeomID o1, dGeomID o2)
{
	LWPUCKGAME* puck_game = (LWPUCKGAME*)data;

	if (dGeomIsSpace(o1) || dGeomIsSpace(o2)) {

		// colliding a space with something :
		dSpaceCollide2(o1, o2, data, &near_callback);

		// collide all geoms internal to the space(s)
		if (dGeomIsSpace(o1))
			dSpaceCollide((dSpaceID)o1, data, &near_callback);
		if (dGeomIsSpace(o2))
			dSpaceCollide((dSpaceID)o2, data, &near_callback);

	} else {
		const int max_contacts = 5;
		dContact contact[5];
		// colliding two non-space geoms, so generate contact
		// points between o1 and o2
		int num_contact = dCollide(o1, o2, max_contacts, &contact->geom, sizeof(dContact));
		// add these contact points to the simulation ...
		for (int i = 0; i < num_contact; i++) {

			if (o1 == puck_game->boundary[LPGB_GROUND] || o2 == puck_game->boundary[LPGB_GROUND]) {
				contact[i].surface.mode = dContactSoftCFM | dContactRolling | dContactFDir1;
				contact[i].surface.rho = 0.0001f;
				contact[i].surface.rho2 = 0.0001f;
				contact[i].surface.rhoN = 0.0001f;
				contact[i].fdir1[0] = 1.0f;
				contact[i].fdir1[1] = 0.0f;
				contact[i].fdir1[2] = 0.0f;
			}
			else {
				contact[i].surface.mode = dContactSoftCFM | dContactBounce;
			}
			

			
			// friction parameter
			contact[i].surface.mu = 1.9f;

			// bounce is the amount of "bouncyness".
			contact[i].surface.bounce = 0.9f;
			// bounce_vel is the minimum incoming velocity to cause a bounce
			contact[i].surface.bounce_vel = 0.1f;
			// constraint force mixing parameter
			contact[i].surface.soft_cfm = 0.001f;
			dJointID c = dJointCreateContact(puck_game->world, puck_game->contact_joint_group, &contact[i]);
			dBodyID b1 = dGeomGetBody(o1);
			dBodyID b2 = dGeomGetBody(o2);

			dJointAttach(c, b1, b2);
		}

	}
}

void update_puck_game(LWPUCKGAME* puck_game) {
	if (!puck_game->world) {
		return;
	}
	dSpaceCollide(puck_game->space, puck_game, near_callback);
	//dWorldStep(puck_game->world, 0.005f);
	dWorldQuickStep(puck_game->world, 0.02f);
	dJointGroupEmpty(puck_game->contact_joint_group);
	const dReal* p = dBodyGetPosition(puck_game->testgo.body);
	LOGI("pos %.2f %.2f %.2f", p[0], p[1], p[2]);
}

void puck_game_push(LWPUCKGAME* puck_game) {
	//puck_game->push = 1;

	dBodySetLinearVel(puck_game->testgo.body, -2.0f, 3.0f, 0);
}