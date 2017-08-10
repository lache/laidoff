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

-- Utility functions end

local c = lo.script_context()

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

-- Always reload test module by clearing the previous loaded instance
package.loaded[field_module_name] = nil
local FieldModule = require(field_module_name)
local field_module = FieldModule:new()
print('field_module:test()', field_module:test())

return 1
