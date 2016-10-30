--print(collectgarbage('count'))
--collectgarbage('stop')
--print('start')
function f(y)
   local x = y + 1
   return function() return f(x) end
end
g = f(0)
for i=1,10000 do
   g = g()
end
--print(collectgarbage('count'))
--if collectgarbage('step', 5000000000000000000000000000000000000000000000000) then
   --print('collected')
   --print(collectgarbage('setpause', 1000000000))
   --print(collectgarbage('setstepmul', 200))
   --print('start')
   --print(collectgarbage('count'))
--end
