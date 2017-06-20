#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lwcontext.h"
#include "script.h"
#include "file.h"
#include "lwlog.h"
#include "laidoff.h"

#define LW_SCRIPT_PREFIX_PATH ASSETS_BASE_PATH "l" PATH_SEPARATOR

// Defined at lo_wrap.c
int luaopen_lo(lua_State* L);

static LWCONTEXT* context;

int l_ink(lua_State *L)
{
	int x;
	if (lua_gettop(L) >= 0)
	{
		x = (int)lua_tonumber(L, -1);
		lua_pushnumber(L, 2 * x + 1);
	}
	return 1;
}

int l_spawn_blue_cube_wall(lua_State* L)
{
	if (lua_gettop(L) >= 3)
	{
		LWCONTEXT* pLwc = lua_touserdata(L, 1);
		float x = (float)lua_tonumber(L, 2);
		float y = (float)lua_tonumber(L, 3);
		int r = spawn_field_object(pLwc, x, y, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_BLUE], 1, 1, 1, 0);
		lua_pushinteger(L, r);
	}
	return 1;
}

int l_spawn_red_cube_wall(lua_State* L) {
	if (lua_gettop(L) >= 4) {
		LWCONTEXT* pLwc = lua_touserdata(L, 1);
		float x = (float)lua_tonumber(L, 2);
		float y = (float)lua_tonumber(L, 3);
		int field_event_id = (int)lua_tonumber(L, 4);
		int r = spawn_field_object(pLwc, x, y, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_RED], 1, 1, 0.5f, field_event_id);
		lua_pushinteger(L, r);
	}
	return 1;
}

int l_spawn_pump(lua_State* L) {
	if (lua_gettop(L) >= 4) {
		LWCONTEXT* pLwc = lua_touserdata(L, 1);
		float x = (float)lua_tonumber(L, 2);
		float y = (float)lua_tonumber(L, 3);
		int field_event_id = (int)lua_tonumber(L, 4);
		int r = spawn_field_object(pLwc, x, y, 1, 1, LVT_PUMP, pLwc->tex_programmed[LPT_SOLID_RED], 1, 1, 0.5f, field_event_id);
		lua_pushinteger(L, r);
	}
	return 1;
}

int l_spawn_oil_truck(lua_State* L) {
	if (lua_gettop(L) >= 4) {
		LWCONTEXT* pLwc = lua_touserdata(L, 1);
		float x = (float)lua_tonumber(L, 2);
		float y = (float)lua_tonumber(L, 3);
		int field_event_id = (int)lua_tonumber(L, 4);
		int r = spawn_field_object(pLwc, x, y, 1, 1, LVT_OIL_TRUCK, pLwc->tex_atlas[LAE_3D_OIL_TRUCK_TEX_KTX], 1, 1, 1.0f, field_event_id);
		lua_pushinteger(L, r);
	}
	return 1;
}

int l_load_module(lua_State* L) {
	if (lua_gettop(L) >= 2) {
		LWCONTEXT* pLwc = lua_touserdata(L, 1);
		const char* filename = lua_tostring(L, 2);
		// returns 0 if no error, -1 otherwise
		int result = script_run_file_ex(pLwc, filename, 0);
		// returning the return value count
		return result == 0 ? 1 : 0;
	}
	return 0;
}

void init_lua(LWCONTEXT* pLwc)
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	lua_pushcfunction(L, l_ink);
	lua_setglobal(L, "ink");
	lua_pushcfunction(L, l_spawn_blue_cube_wall);
	lua_setglobal(L, "spawn_blue_cube_wall");
	lua_pushcfunction(L, l_spawn_red_cube_wall);
	lua_setglobal(L, "spawn_red_cube_wall");
	lua_pushcfunction(L, l_spawn_pump);
	lua_setglobal(L, "spawn_pump");
	lua_pushcfunction(L, l_spawn_oil_truck);
	lua_setglobal(L, "spawn_oil_truck");
	lua_pushcfunction(L, l_load_module);
	lua_setglobal(L, "load_module");
	lua_pushlightuserdata(L, pLwc);
	lua_setglobal(L, "pLwc");
	// Load 'lo' library generated from swig
	luaopen_lo(L);

	script_set_context(pLwc);

	{
		int result = luaL_dostring(L, "return ink(1000)");
		if (result)
		{
			fprintf(stderr, "Failed to run lua: %s\n", lua_tostring(L, -1));
		}
		else
		{
			printf("Lua result: %lld\n", lua_tointeger(L, -1));
		}
		lua_pop(L, 1);
	}

	pLwc->L = L;
	//lua_close(L);

	script_run_file(pLwc, ASSETS_BASE_PATH "l" PATH_SEPARATOR "post_init_once.lua");
}

int script_run_file_ex(LWCONTEXT* pLwc, const char* filename, int pop_result) {
	char* script = create_string_from_file(filename);
	if (script) {
		int result = luaL_dostring(pLwc->L, script);
		release_string(script);
		if (result) {
			fprintf(stderr, "Failed to run lua: %s\n", lua_tostring(pLwc->L, -1));
		} else {
			printf("Lua result: %lld\n", lua_tointeger(pLwc->L, -1));
		}
		if (pop_result) {
			lua_pop(pLwc->L, 1);
		}
		return 0;
	}

	LOGE("script_run_file: loading script %s failed", filename);
	return -1;
}

void script_run_file(LWCONTEXT* pLwc, const char* filename) {
	script_run_file_ex(pLwc, filename, 1);
}

void spawn_all_field_object(LWCONTEXT* pLwc) {
	script_run_file(pLwc, LW_SCRIPT_PREFIX_PATH "spawn.lua");
}

void despawn_all_field_object(LWCONTEXT* pLwc) {
	for (int i = 0; i < ARRAY_SIZE(pLwc->field_object); i++) {
		pLwc->field_object[i].valid = 0;
	}
	
	for (int i = 0; i < ARRAY_SIZE(pLwc->box_collider); i++) {
		pLwc->box_collider[i].valid = 0;
	}
}

void script_set_context(LWCONTEXT* pLwc) {
	context = pLwc;
}

LWCONTEXT* script_context() {
	return context;
}

const char* script_prefix_path() {
	return LW_SCRIPT_PREFIX_PATH;
}
