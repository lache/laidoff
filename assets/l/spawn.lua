print("Welcome to lua!")

--[[
spawn_blue_cube_wall(pLwc, 3, 3)
spawn_blue_cube_wall(pLwc, -3, 3)
spawn_blue_cube_wall(pLwc, 3, -3)
spawn_blue_cube_wall(pLwc, -3, -3)

spawn_blue_cube_wall(pLwc, 6, 6)
]]--

spawn_red_cube_wall(pLwc, 3, -7, 1)

spawn_red_cube_wall(pLwc, 6, -7, 2)

spawn_red_cube_wall(pLwc, 9, -7, 3)

spawn_red_cube_wall(pLwc,12, -7, 4)

spawn_red_cube_wall(pLwc,15, -7, 5)

spawn_red_cube_wall(pLwc, -3, -7, 6)

spawn_pump(pLwc, -5, -5, 6)


return ink(15)