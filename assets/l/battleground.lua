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
	-- Lua handler for logc frame finish events emitted from C
	function on_logic_frame_finish()
		return 0
	end
	return 'battleground'
end

return M
