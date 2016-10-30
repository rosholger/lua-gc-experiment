--require "math"
function luaTest(p)
   print(cTest(p))
end

function dist(v1, v2)
   return math.sqrt((v1.x-v2.x)*(v1.x-v2.x)+(v1.y-v2.y)*(v1.y-v2.y))
end

function activate(e)
   setColor(e,
            {r=math.random(),
             g=math.random(),
             b=math.random()})
   setDP(e,
         {x=(math.random()*2-1)*0.2,
          y=(math.random()*2-1)*0.2})
   setP(e, {x=math.random(0,200), y=math.random(0,200)})
   setFramesToBeActive(e, math.random(30,120))
   local p = getP(e)
end

function setup(entities)
   math.randomseed(1234)
end

function update(entities)
   local length = getLength(entities)
   for i=1, length do
      local e = getEntity(entities, i)
      local framesToBeActive = getFramesToBeActive(e)
      if framesToBeActive > 0 then
         local p = getP(e)
         local dp = getDP(e)
         p.x = p.x + dp.x
         p.y = p.y + dp.y
         setP(e, p)
      else
        if math.random() < 1.1 then
           activate(e)
        end
      end
   end
end
