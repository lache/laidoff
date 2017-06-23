#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lwcontext.h"
#include "script.h"
#include "file.h"
#include "lwlog.h"
#include "laidoff.h"

#define LW_SCRIPT_PREFIX_PATH ASSETS_BASE_PATH "l" PATH_SEPARATOR
#define LW_MAX_CORO (32)

// Defined at lo_wrap.c
int luaopen_lo(lua_State* L);

static LWCONTEXT* context;

typedef struct _LWCORO {
	int valid;
	lua_State* L;
	double yield_remain;
} LWCORO;

typedef struct _LWSCRIPT {
	LWCORO coro[LW_MAX_CORO];
} LWSCRIPT;

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

static int lua_cb(lua_State *L) {
	printf("called lua_cb\n");
	return 0;
}

int l_yield_wait_ms(lua_State* L) {
	if (!lua_isinteger(L, 1)) {
		lua_pushliteral(L, "incorrect argument");
	}
	// Get wait ms from function argument (lua integer is 64-bit)
	int wait_ms = (int)lua_tointeger(L, 1);
	// Get coro_index from registry
	lua_pushlightuserdata(L, L);
	lua_gettable(L, LUA_REGISTRYINDEX);
	if (!lua_isinteger(L, -1)) {
		lua_pushliteral(L, "incorrect argument");
		lua_error(L);
	}
	int coro_index = (int)lua_tointeger(L, -1);
	LWCONTEXT* pLwc = context;
	LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
	script->coro[coro_index].yield_remain = wait_ms / 1000.0;
	LOGI("Coro[%d] Set yield remain %f", coro_index, script->coro[coro_index].yield_remain);
	lua_pushnumber(L, 1);
	lua_pushnumber(L, 2);
	lua_pushcfunction(L, lua_cb);
	return lua_yield(L, 3);
}

int l_start_coro(lua_State* L) {
	if (lua_gettop(L) >= 1) {
		LWCONTEXT* pLwc = lua_touserdata(L, lua_upvalueindex(1));
		if (!lua_isfunction(L, 1)) {
			lua_pushliteral(L, "incorrect argument");
			lua_error(L);
		}
		LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
		for (int i = 0; i < LW_MAX_CORO; i++) {
			if (!script->coro[i].valid) {
				script->coro[i].valid = 1;
				lua_State* L_coro = lua_newthread(L);
				script->coro[i].L = L_coro;
				// Set coroutine array index to registry
				lua_pushlightuserdata(L_coro, L_coro);
				lua_pushinteger(L_coro, i);
				lua_settable(L_coro, LUA_REGISTRYINDEX);
				// Copy coroutine entry function (first argument of 'start_coro')
				// and add onto the top of stack.
				lua_pushvalue(L, 1);
				// Move coroutine entry function to new thread's stack
				lua_xmove(L, L_coro, 1);
				// Run coroutine
				/*int status = lua_resume(L_coro, L, 0);
				if (status == LUA_YIELD) {
					lua_CFunction f = lua_tocfunction(L_coro, -1);
					f(L_coro);
				}*/
				break;
			}
		}
		//lua_pushinteger(L, 0);
	}
	return 0;
}

int l_spawn_blue_cube_wall_2(lua_State* L) {
	if (lua_gettop(L) >= 2) {
		LWCONTEXT* pLwc = lua_touserdata(L, lua_upvalueindex(1));
		float x = (float)lua_tonumber(L, 1);
		float y = (float)lua_tonumber(L, 2);
		int r = spawn_field_object(pLwc, x, y, 1, 1, LVT_CUBE_WALL, pLwc->tex_programmed[LPT_SOLID_BLUE], 1, 1, 1, 0);
		lua_pushinteger(L, r);
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
	LWSCRIPT* script = (LWSCRIPT*)calloc(1, sizeof(LWSCRIPT));
	pLwc->script = script;


	lua_State* L = luaL_newstate();
	// Load lua standard libs
	luaL_openlibs(L);
	// ink
	lua_register(L, "ink", l_ink);
	// spawn_blue_cube_wall
	lua_register(L, "spawn_blue_cube_wall", l_spawn_blue_cube_wall);
	// spawn_blue_cube_wall_2
	lua_pushlightuserdata(L, pLwc);
	lua_pushcclosure(L, l_spawn_blue_cube_wall_2, 1);
	lua_setglobal(L, "spawn_blue_cube_wall_2");
	// start_coro
	lua_pushlightuserdata(L, pLwc);
	lua_pushcclosure(L, l_start_coro, 1);
	lua_setglobal(L, "start_coro");
	// wait_ms
	lua_pushlightuserdata(L, pLwc);
	lua_pushcclosure(L, l_yield_wait_ms, 1);
	lua_setglobal(L, "yield_wait_ms");
	// spawn_red_cube_wall
	lua_register(L, "spawn_red_cube_wall", l_spawn_red_cube_wall);
	// spawn_pump
	lua_register(L, "spawn_pump", l_spawn_pump);
	// spawn_oil_truck
	lua_register(L, "spawn_oil_truck", l_spawn_oil_truck);
	// load_module
	lua_register(L, "load_module", l_load_module);
	// 'pLwc'
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

void script_set_context(LWCONTEXT* pLwc) {
	context = pLwc;
}

LWCONTEXT* script_context() {
	return context;
}

const char* script_prefix_path() {
	return LW_SCRIPT_PREFIX_PATH;
}

void script_update(LWCONTEXT* pLwc) {
	if (!pLwc->script) {
		return;
	}
	LWSCRIPT* script = (LWSCRIPT*)pLwc->script;
	for (int i = 0; i < LW_MAX_CORO; i++) {
		if (script->coro[i].valid) {
			// Only process if yieldable
			if (script->coro[i].yield_remain > 0) {
				// Wait
				script->coro[i].yield_remain -= lwcontext_delta_time(pLwc);
			} else {
				// Resume immediately
				lua_State* L_coro = script->coro[i].L;
				int status = lua_resume(L_coro, pLwc->L, 0);
				if (status == LUA_YIELD) {
					lua_CFunction f = lua_tocfunction(L_coro, -1);
					f(L_coro);
				} else {
					// Coroutine execution completed
					script->coro[i].valid = 0;
				}
			}
		}
	}
}
