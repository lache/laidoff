print("post init lua")

print("LVT_ENEMY_SCOPE: " .. lo.LVT_ENEMY_SCOPE)

c = lo.script_context()
--print("-- cxt --")
--print(inspect(getmetatable(cxt)))

lo.show_sys_msg(c.def_sys_msg, 'I AM SCRIPT 5 한글')

-- Always reload test module
package.loaded.testmod = nil
tm = require('testmod')
print('testmod.foo()', tm.foo())

return 1
