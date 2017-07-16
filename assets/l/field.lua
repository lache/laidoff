print('field.lua visible')
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

function M:__tostring()
	return self.name..'<Field['..tostring(self.orig_string:sub(8))..']>' --..debug.getinfo(1).source
end

function M:test()
	print(self, 'name=', self.name)
end

function M:spawn(obj, faction)
	table.insert(self.objs, { obj = obj, faction = faction, })
	obj.key = #self.objs
	obj.faction = faction
	obj.field = self
	--print(self, 'Spawn', obj, 'key', obj.key, 'faction', obj.faction)
	
	lo.rmsg_spawn(c, obj.key, obj.objtype, obj.x, obj.y, obj.angle)
	
end

function M:despawn(obj)
	self.objs[obj.key] = 'dead'
	obj.field = nil
	--print(self, 'Despawn', obj, 'key', obj.key, 'faction', obj.faction)
	
	lo.rmsg_despawn(c, obj.key)
end

function M:query_nearest_target_in_range(caller, range)
	for k,v in pairs(self.objs) do
		--print('k',k,'v',v)
		--print(inspect(v))
		local obj = v.obj
		if obj and obj ~= caller and obj.faction ~= caller.faction then
			local dx = obj.x - caller.x
			local dy = obj.y - caller.y
			local dist = math.sqrt(dx*dx + dy*dy)
			if dist < range then return obj end
		end
	end
end

function M:time()
	return lo.lwtimepoint_now_seconds()
end

function M:start_updating()
	self.last_update = self:time()
	start_coro(function() self:update_coro() end)
end

function M:update_coro()
	while true do
		local t = self:time()
		self.dt = t - self.last_update
		self:update(self.dt)
		self.last_update = t
		yield_wait_ms(8)
	end
end

function M:update(dt)
	-- flush out dead objects
	for k,v in pairs(self.objs) do
		local obj = v.obj
		if obj and obj.dead_flag then
			self:despawn(obj)
		end
	end
	-- call update
	for k,v in pairs(self.objs) do
		local obj = v.obj
		if obj and obj ~= 'dead' then
			obj:update(dt)
		end
	end
end

return M
