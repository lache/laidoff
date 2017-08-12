print('testfield.lua visible')
local M = {
	objs = {}
}
M.__index = M
local c = lo.script_context()

function M:new(name, field)
	o = {}
	o.orig_string = tostring(o)
	o.name = name
	o.field = field
	setmetatable(o, self)
	return o
end

function M:test()
	local Guntower = reload_require('guntower')
	local Faction1 = 1
	local guntower1 = Guntower:new('gt1', 3, 0, 0)
	self.field:spawn(guntower1, Faction1)
	guntower1:start_thinking()
	
	guntower1:spawn_bullet()
	
	return 'testfield'
end

return M
