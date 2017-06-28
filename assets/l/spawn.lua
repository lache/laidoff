print("Welcome to lua!")

--[[
spawn_blue_cube_wall(pLwc, 3, 3)
spawn_blue_cube_wall(pLwc, -3, 3)
spawn_blue_cube_wall(pLwc, 3, -3)
spawn_blue_cube_wall(pLwc, -3, -3)

spawn_blue_cube_wall(pLwc, 6, 6)
]]--

spawn_blue_cube_wall_2(9, 9, 1)

-- 1 enemy
spawn_red_cube_wall(pLwc, 3, -7, 1)
-- 2 enemies
spawn_red_cube_wall(pLwc, 6, -7, 2)
-- 3 enemies
spawn_red_cube_wall(pLwc, 9, -7, 3)
-- 4 enemies
spawn_red_cube_wall(pLwc,12, -7, 4)
-- 5 enemies
spawn_red_cube_wall(pLwc,15, -7, 5)
-- Dialog event trigger
spawn_red_cube_wall(pLwc, -3, -7, 6)

--spawn_pump(pLwc, -5, -5, 6)


return ink(15)