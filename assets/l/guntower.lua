print('guntower.lua visible ^__^;')
local Bullet = require('bullet')

local M = {
	objtype = 1,
	fired = 0, -- last fired field time
	turnspeed = 12, -- rad/sec
	fireinterval = 1, -- seconds
	angle = 0, -- current angle
	targetangle = 0, -- target angle
	firerange = 100, -- fire range
	firearcangle = 0.05, -- rad
	bulletspeed = 30,
	x = 0, -- x position
	y = 0, -- y position
	target = nil, -- current target
	hp = 100,
}
M.__index = M

function M:new(name, x, y)
	o = {}
	o.orig_string = tostring(o)
	o.name = name
	o.x = x
	o.y = y
	setmetatable(o, self)
	return o
end

function M:__tostring()
	return self.name..'<Guntower['..tostring(self.orig_string:sub(8))..']>' --..debug.getinfo(1).source
end

function M:update(dt)
	if self.hp <= 0 then
		self.dead_flag = true
	end
end

function M:test()
	print(self, 'guntower test successful', self.x, self.y, self.turnspeed, self.target, self.xxx)
end

function M:start_thinking()
	start_coro(function() self:think_coro() end)
end

function M:nearest_target_in_range_coro(range)
	while true do
		local target = self.field:query_nearest_target_in_range(self, range)
		if target == nil then
			yield_wait_ms(100)
		else
			return target
		end
	end
end

function M:play_fire_anim()
	--play_skin_anim(self, 'Recoil')
	--print(self, 'play Recoil anim')
end

function M:bullet_spawn_pos()
	return {x=self.x, y=self.y}
end

function M:spawn_bullet()
	local bsp = self:bullet_spawn_pos()
	local bullet = Bullet:new('', bsp.x, bsp.y, self.angle, self.bulletspeed)
	self.field:spawn(bullet, self.faction)
end

function M:try_fire(target)
	local ft = self.field.time()
	local last_fire_elapsed = ft - self.fired
	--print('ft',ft, 'self.fired', self.fired, 'last_fire_elapsed', last_fire_elapsed, 'self.fireinterval', self.fireinterval)
	if self.fired ~= 0 and last_fire_elapsed < self.fireinterval then
		-- No fire.
		-- Just end this think tick and try again at next tick.
	else
		self.fired = ft
		self:play_fire_anim()
		self:spawn_bullet()
	end
end

function M:turn_barrel(target)

	self.targetangle = math.atan(target.y - self.y, target.x - self.x)
	local angle_diff = self.targetangle - self.angle
	local move_angle = angle_diff * self.turnspeed * self.field.dt
	self.angle = self.angle + move_angle;
	if math.abs(self.angle - self.targetangle) < self.firearcangle then
		return true
	else
		return false
	end
end

function M:think_coro()
	while true do
		--print(self, 'Finding target...')
		local target = self:nearest_target_in_range_coro(self.firerange)
		--print(self, 'Target found!', target, 'Turning the barrel...')
		local barrel_aligned = self:turn_barrel(target)
		if barrel_aligned then
			--print(self, 'Barrel aligned to target!', target, 'Try fire...')
			self:try_fire(target)
			--print(self, 'Wait for next think tick.. (long)')
			yield_wait_ms(100)
		else
			--print(self, 'Barrel NOT aligned to target. current', self.angle, 'target', self.targetangle, 'No fire.')
			--print(self, 'Wait for next think tick.. (short)')
			yield_wait_ms(100)
		end
    end
end

return M