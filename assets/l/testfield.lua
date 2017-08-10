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
	
	-- Always reload guntower module
	package.loaded.guntower = nil
	local Guntower = require('guntower')
	local Faction1 = 1
	local guntower1 = Guntower:new('gt1', 4, 0, 0)
	field:spawn(guntower1, Faction1)
	guntower1:start_thinking()
	
	return 'testfield'
end

return M
