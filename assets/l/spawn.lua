print("Welcome to lua!")

spawn_blue_cube_wall(pLwc, 3, 3)
spawn_blue_cube_wall(pLwc, -3, 3)
spawn_blue_cube_wall(pLwc, 3, -3)
spawn_blue_cube_wall(pLwc, -3, -3)

spawn_blue_cube_wall(pLwc, 6, 6)

spawn_red_cube_wall(pLwc, 3, 0, 1)

spawn_red_cube_wall(pLwc, 6, 0, 2)

spawn_red_cube_wall(pLwc, 9, 0, 3)

spawn_red_cube_wall(pLwc,12, 0, 4)

spawn_red_cube_wall(pLwc,15, 0, 5)

spawn_red_cube_wall(pLwc, -3, 0, 6)

return ink(15)