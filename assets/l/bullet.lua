print('bullet.lua visible')
local M = {
	objtype = 2,
}
M.__index = M
local c = lo.script_context()

function M:new(name, x, y, z, angle, speed)
	o = {}
	o.orig_string = tostring(o)
	o.name = name
	o.x = x
	o.y = y
	o.z = z
	o.angle = angle
	o.speed = speed
	o.age = 0
	o.max_age = 3
	o.range = 1
	o.damage = 50
	setmetatable(o, self)
	--print(self, 'bullet spawned')
	return o
end

function M:__tostring()
	return self.name..'<Bullet['..tostring(self.orig_string:sub(8))..']>' --..debug.getinfo(1).source
end

function M:update(dt)
	--print(self, 'update')
	self.x = self.x + dt * self.speed * math.cos(self.angle)
	self.y = self.y + dt * self.speed * math.sin(self.angle)
	lo.rmsg_pos(c, self.key, self.x, self.y, self.z)
	
	self.age = self.age + dt
	--print(self, 'x', self.x, 'y', self.y)
	if self.age > self.max_age then
		self.dead_flag = true
	end
	local target = self.field:query_nearest_target_in_range(self, self.range)
	if target then
		target.hp = target.hp - self.damage
		--print(self, 'target HP reduced to ', target.hp, 'damage', self.damage)
		self.dead_flag = true
	end
end

return M
