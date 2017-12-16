print('post_init.lua visible')
local inspect = require('inspect')

-- Utility functions begin

function lo.new_vec3(x, y, z)
	local v = lo.new_float(3)
	lo.float_setitem(v, 0, x)
	lo.float_setitem(v, 1, y)
	lo.float_setitem(v, 2, z)
	return v
end

function lo.print_vec3(v)
	print('x', lo.float_getitem(v, 0), 'y', lo.float_getitem(v, 1), 'z', lo.float_getitem(v, 2))
end

function lo.get_vec3(v)
	return {
		['x'] = lo.float_getitem(v, 0),
		['y'] = lo.float_getitem(v, 1),
		['z'] = lo.float_getitem(v, 2)
	}
end

function lo.delete_vec3(v)
	lo.delete_float(v)
end

function reload_require(modname)
	package.loaded[modname] = nil
	return require(modname)
end

-- Utility functions end

local c = lo.script_context()

local Data = reload_require('data')
print('Data loaded!')
local data = Data:new()

local Field = reload_require('field')
print('Field loaded!')
local field = Field:new('test field')
field:test()
field:start_updating()
-- Lua handler for anim marker events emitted from C.
-- This function is called from C, thus should not have 'local' specifier.
-- Also note that this function implicitly captures 'field'.
function on_anim_marker(key, name)
	--print('on_anim_marker key:',key,', name:', name)
	field:on_anim_marker(key, name)
	return 0
end
-- Lua handler for collision events (near events) emitted from C.
function on_near(key1, key2)
	--print('on_near key1', key1, 'key2', key2)
	field.objs[key2].obj:collide_with(key1)
	return 0
end
-- Lua handler for logc frame finish events emitted from C
function on_logic_frame_finish(dt)
	field:update(dt)
	return 0
end

local last_construct_gtid = ''

function copy(obj, seen)
  if type(obj) ~= 'table' then return obj end
  if seen and seen[obj] then return seen[obj] end
  local s = seen or {}
  local res = setmetatable({}, getmetatable(obj))
  s[obj] = res
  for k, v in pairs(obj) do res[copy(k, s)] = copy(v, s) end
  return res
end

-- Lua handler for logc frame finish events emitted from C
function on_ui_event(id)
	print('ui event emitted from C:' .. id)
	local gtid = 'catapult'
	if id == 'seltower0' then gtid = 'catapult'
	elseif id == 'seltower1' then gtid = 'crossbow'
	elseif id == 'seltower2' then gtid = 'guntower'
	elseif id == 'seltower3' then gtid = 'pyro'
	elseif id == 'pull_button' then
		print('[script]button_pull')
		lo.puck_game_pull_puck_toggle(c, c.puck_game)
		return 0
	elseif id == 'dash_button' then
		print('[script]button_dash')
		lo.puck_game_dash_and_send(c, c.puck_game)
		return 0
	elseif id == 'jump_button' then
		print('[script]button_jump')
		lo.puck_game_jump(c, c.puck_game)
		-- temporarily for rematch
		lo.puck_game_rematch(c, c.puck_game)
		return 0
	elseif id == 'practice_button' then
		print('[script]practice_button')
		lo.puck_game_reset_battle_state(c.puck_game)
		lo.puck_game_clear_match_data(c, c.puck_game)
		lo.puck_game_reset_view_proj(c, c.puck_game)
		lo.puck_game_roll_to_practice(c.puck_game)
	elseif id == 'back_button' then
		if c.puck_game.game_state == lo.LPGS_SEARCHING then
			lo.tcp_send_cancelqueue(c.tcp, c.tcp.user_id)
			c.puck_game.game_state = lo.LPGS_MAIN_MENU
		elseif c.game_scene == lo.LGS_LEADERBOARD then
			lo.change_to_physics(c)
		else
			lo.lw_go_back(c, nil)
		end
	elseif id == 'tutorial_button' then
		--lo.show_sys_msg(c.def_sys_msg, 'TUTORIAL: under construction')
		print('[script]tutorial_button')
		lo.puck_game_reset_tutorial_state(c.puck_game)
		lo.puck_game_clear_match_data(c, c.puck_game)
		lo.puck_game_reset_view_proj(c, c.puck_game)
		lo.puck_game_roll_to_tutorial(c.puck_game)
	elseif id == 'online_button' then
		c.puck_game.game_state = lo.LPGS_SEARCHING
		lo.puck_game_set_searching_str(c.puck_game, 'SEARCHING OPPONENT...')
		lo.puck_game_clear_match_data(c, c.puck_game)
		lo.puck_game_reset_battle_state(c.puck_game)
		lo.tcp_send_queue2(c.tcp, c.tcp.user_id)
	elseif id == 'leaderboard_button' then
		lo.show_leaderboard(c)
	else 
		lo.construct_set_preview_enable(c.construct, 0)
		return 0
	end
	local gt = data.guntower[gtid]
	if last_construct_gtid ~= gtid then
		lo.construct_set_preview_enable(c.construct, 1)
		lo.construct_set_preview(c.construct, gt.atlas, gt.skin_vbo, gt.armature, gt.anim_action_id)
	else
		local Faction1 = 1
		local gtinst = copy(gt)
		print(inspect(gtinst))
		local pt = lo.new_vec3(0, 0, 0)
		lo.field_get_player_position(c.field, pt)
		local ptvec = lo.get_vec3(pt)
		gtinst.x = ptvec.x
		gtinst.y = ptvec.y
		gtinst.z = ptvec.z
		field:spawn(gtinst, Faction1)
		gtinst:start_thinking()
	end
	last_construct_gtid = gtid
	return 0
end

local function split_path(p)
	return string.match(p, "(.-)([^\\/]-%.?([^%.\\/]*))$")
end
local field_filename = lo.field_filename(c.field)
print('Field filename:' .. field_filename)
local field_filename_path, field_filename_name, field_filename_ext = split_path(field_filename)
local field_module_name = string.sub(field_filename_name, 1, string.len(field_filename_name) - string.len(field_filename_ext) - 1)
print('Field filename path:' .. field_filename_path)
print('Field filename name:' .. field_filename_name)
print('Field filename name only:' .. field_module_name)
print('Field filename ext:' .. field_filename_ext)

local FieldModule = reload_require(field_module_name)
local field_module = FieldModule:new(field_filename_name, field)
print('field_module:test()', field_module:test())

return 1
