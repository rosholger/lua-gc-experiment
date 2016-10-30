--require "math"
function update(entities)
   local length = getLength(entities)
   for i=1, length do
      local e = getEntity(entities, i)
      updateSingle(e)
   end
end
