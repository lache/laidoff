inspect = require('inspect')

print("post init lua")

print("LVT_ENEMY_SCOPE: " .. lo.LVT_ENEMY_SCOPE)

local c = lo.script_context()
--print(inspect(getmetatable(c)))

lo.show_sys_msg(c.def_sys_msg, 'I AM SCRIPT 5 한글')

-- Always reload test module by clearing the previous loaded instance
package.loaded.testmod = nil
tm = require('testmod')
print('testmod.foo()', tm.foo())

-- Always reload test module by clearing the previous loaded instance
package.loaded.bullet = nil
require('bullet')

-- Always reload field module
package.loaded.field = nil
local Field = require('field')
print('Field loaded!')
local field = Field:new('test field')
local field2 = Field:new('test field another')
field:test()
field2:test(0)
field:start_updating()
-- Always reload guntower module
package.loaded.guntower = nil
local Guntower = require('guntower')
print('Guntower loaded! ^__^')
local Faction1 = 1
local Faction2 = 2

local guntower1 = Guntower:new('gt1', 0, 0)
--print(inspect(guntower1))
--guntower1:test()
field:spawn(guntower1, Faction1)
guntower1:start_thinking()

start_coro(function()
	local idx = 1
	while true do
		local guntower2 = Guntower:new('gt-enemy-'..idx, math.random(-10, 10), math.random(-10, 0))
		idx = idx + 1
		--guntower2:test()
		--print(inspect(guntower2))
		field:spawn(guntower2, Faction2)
		yield_wait_ms(3 * 1000)
	end
end)

--field:despawn(guntower1)
--field:despawn(guntower2)



function foo()
  print("foo", 1)
  coroutine.yield('yielded value')
  print("foo", 2)
end

co = coroutine.create(foo)
print(type(co))
print(coroutine.status(co))
print(coroutine.resume(co))
print(coroutine.resume(co))
print(coroutine.status(co))
print(coroutine.resume(co))

--lo.change_to_font_test(c)

tm.testcoro()

start_coro(function ()
    for i=1,4 do
      lo.show_sys_msg(c.def_sys_msg, 'hello ' .. i)
      yield_wait_ms(1000)
    end
end)
--print(coro1)

start_coro(function ()
    yield_wait_ms(5000)
    lo.show_sys_msg(c.def_sys_msg, 'hello x 1')
    yield_wait_ms(1000)
    lo.show_sys_msg(c.def_sys_msg, 'hello x 2')
    yield_wait_ms(1000)
    lo.show_sys_msg(c.def_sys_msg, 'hello x 3')
	
    -- Collect garbage forcefully
    collectgarbage()
    print('Garbage collected.')
end)

start_coro(function ()
    --while true do
      cw1 = spawn_blue_cube_wall_2(10, 10, 1)
      yield_wait_ms(1000)
      lo.despawn_field_object(c, cw1)
      cw2 = spawn_blue_cube_wall_2(11, 11, 1)
      yield_wait_ms(1000)
      lo.despawn_field_object(c, cw2)
      cw3 = spawn_blue_cube_wall_2(12, 12, 1)
      yield_wait_ms(1000)
      lo.despawn_field_object(c, cw3)
      yield_wait_ms(1000)
    --end
end)

nav = lo.field_nav(c.field)

start_coro(function ()
    for i=1, 10 do
      -- Truck field object
      truck = spawn_oil_truck(pLwc, -8, -8, 6)
      -- Path query
      pq = lo.nav_new_path_query(nav)
      -- Activate path query update
      lo.nav_update_output_path_query(nav, pq, 1)
      -- Bind path query to truck
      lo.nav_bind_path_query_output_location(nav, pq, c.field, truck)
    end
end)

return 1
