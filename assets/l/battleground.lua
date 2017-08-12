print('battleground.lua visible')
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
	return 'battleground'
end

return M
