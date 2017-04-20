print("Welcome to lua!")

spawn_blue_cube_wall(pLwc, 3, 3)
spawn_blue_cube_wall(pLwc, -3, 3)
spawn_blue_cube_wall(pLwc, 3, -3)
spawn_blue_cube_wall(pLwc, -3, -3)

spawn_blue_cube_wall(pLwc, 6, 6)

spawn_red_cube_wall(pLwc, 3, 0, 1)

spawn_red_cube_wall(pLwc, -3, 0, 2)

return ink(15)