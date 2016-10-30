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
end

function setup(entities)
   math.randomseed(1234)
   local length = getLength(entities)
   for i=1, length do
      local e = getEntity(entities, i)
      activate(e)
   end
end

function updateSingle(e)
   local p = getP(e)
   local dp = getDP(e)
   p.x = p.x + dp.x
   p.y = p.y + dp.y
   setP(e, p)
end
