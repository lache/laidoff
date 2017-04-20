#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lwcontext.h"
#include "script.h"
#include "file.h"
#include "lwlog.h"
#include "laidoff.h"

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
	lua_pushlightuserdata(L, pLwc);
	lua_setglobal(L, "pLwc");

	{
		char* script = create_string_from_file(ASSETS_BASE_PATH "l" PATH_SEPARATOR "spawn.lua");
        if (script) {
            int result = luaL_dostring(L, script);
            release_string(script);
            if (result)
            {
                fprintf(stderr, "Failed to run lua: %s\n", lua_tostring(L, -1));
            }
            else
            {
                printf("Lua result: %lld\n", lua_tointeger(L, -1));
            }
            lua_pop(L, 1);
        } else {
            LOGE("init_lua: loading script failed");
        }
		
	}

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

	lua_close(L);

}
