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
    lo.show_sys_msg(c.def_sys_msg, 'hello 1')
    yield_wait_ms(1000)
    lo.show_sys_msg(c.def_sys_msg, 'hello 2')
    yield_wait_ms(1000)
    lo.show_sys_msg(c.def_sys_msg, 'hello 3')
end)

start_coro(function ()
    yield_wait_ms(5000)
    lo.show_sys_msg(c.def_sys_msg, 'hello x 1')
    yield_wait_ms(1000)
    lo.show_sys_msg(c.def_sys_msg, 'hello x 2')
    yield_wait_ms(1000)
    lo.show_sys_msg(c.def_sys_msg, 'hello x 3')
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
    print('...!!!')
    for i=1, 10 do
      cw_pq = spawn_oil_truck(pLwc, -8, -8, 6)
      pq1 = lo.nav_new_path_query(nav)
      lo.nav_update_output_path_query(nav, pq1, 1)
      lo.nav_bind_path_query_output_location(nav, pq1, c.field, cw_pq)
    end
    
    --[[
    yield_wait_ms(1000)
    lo.field_update_output_path_query(c.field, pq1, 0)
    yield_wait_ms(10000)
    lo.field_update_output_path_query(c.field, pq1, 1)
    yield_wait_ms(1000)
    lo.field_update_output_path_query(c.field, pq1, 0)
    yield_wait_ms(1000)
    lo.field_update_output_path_query(c.field, pq1, 1)
    yield_wait_ms(1000)
    lo.field_update_output_path_query(c.field, pq1, 0)
    yield_wait_ms(1000)
    lo.field_update_output_path_query(c.field, pq1, 1)
    ]]--
  print('new pq', pq1)
end)

return 1
