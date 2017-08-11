print('testfield.lua visible')
local M = {
	objs = {}
}
M.__index = M
local c = lo.script_context()

function M:new(name)
	o = {}
	o.orig_string = tostring(o)
	o.name = name
	setmetatable(o, self)
	return o
end

function M:test()
	-- Always reload field module
	package.loaded.field = nil
	local Field = require('field')
	print('Field loaded!')
	local field = Field:new('test field')
	field:test()
	field:start_updating()
	-- Lua handler for anim marker events emitted from C.
	-- This function is called from C, thus should not have 'local' specifier.
	-- Also note that this function implicitly captures 'field'.
	function on_anim_marker(key, name)
		--print('on_anim_marker key:',key,', name:', name)
		field:on_anim_marker(key, name)
		return 0
	end
	-- Lua handler for collision events (near events) emitted from C.
	function on_near(key1, key2)
		--print('on_near key1', key1, 'key2', key2)
		field.objs[key2].obj:collide_with(key1)
		--field:delayed_despawn_by_key(key2)
		return 0
	end
	-- Lua handler for logc frame finish events emitted from C
	function on_logic_frame_finish()
		field:despawn_zombie_obj_keys()
		return 0
	end
	
	-- Always reload guntower module
	package.loaded.guntower = nil
	local Guntower = require('guntower')
	local Faction1 = 1
	local guntower1 = Guntower:new('gt1', 3, 0, 0)
	field:spawn(guntower1, Faction1)
	guntower1:start_thinking()
	
	guntower1:spawn_bullet()
	
	return 'testfield'
end

return M
