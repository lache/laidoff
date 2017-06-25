-- Script for testing runtime script errors are properly shown in the console

-- 1. Error within coroutine
start_coro(function ()
    xxxxx()
end)

-- 2. Error within coroutine within coroutine
start_coro(function ()
    start_coro(function ()
      xxxxx()
    end)
end)

-- 3. Error within coroutine within function
function testfunc()
  start_coro(function ()
      xxxxx()
  end)  
end
testfunc()

-- 4. Error within coroutine within function within function
function testfuncparent()
  testfunc()
end
testfuncparent()

-- 5. Error within function within coroutine
start_coro(function ()
  testfunc()
end)

-- 6. Error within function within function
function testfunc2()
  xxxxx()
end
function testfunc3()
  testfunc2()
end
testfunc3()

return 1
